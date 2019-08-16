#include "UCSO_SDK.h"
#include <filesystem>

UCSO* UCSO::CreateInstance(VESSEL* vessel)
{
	return new UCSO(vessel);
}

UCSO::UCSO(VESSEL* vessel)
{
	this->vessel = vessel;
	attachsMap = std::map<int, ATTACHMENTHANDLE>();
	maxCargoMass = -1;
	maxTotalCargoMass = -1;
	totalCargoMass = 0;
	grappleDistance = 50;
	unpackDistance = 3;
	releaseVel = 0.05;
	isTotalMassGet = false;
	cargoCount = 0;

	InitCargo();
}

void UCSO::SetSlotAttachment(int slot, ATTACHMENTHANDLE attachmentHandle)
{
	if (attachsMap.find(slot) != attachsMap.end() && !attachmentHandle) attachsMap.erase(slot);
	else if(attachmentHandle) attachsMap[slot] = attachmentHandle;
}

void UCSO::SetMaxCargoMass(double maxCargoMass)
{
	this->maxCargoMass = maxCargoMass;
}

void UCSO::SetMaxTotalCargoMass(double maxTotalCargoMass)
{
	this->maxTotalCargoMass = maxTotalCargoMass;
}

void UCSO::SetMaxGrappleDistance(double grappleDistance)
{
	this->grappleDistance = grappleDistance;
}

void UCSO::SetMaxUnpackDistance(double unpackDistance)
{
	this->unpackDistance = unpackDistance;
}

void UCSO::SetReleaseVelocity(double releaseVel)
{
	this->releaseVel = releaseVel;
}

int UCSO::GetCargoCount()
{
	return cargoCount;
}

const char* UCSO::GetCargoName(int index)
{
	if (index < 0 || index >= static_cast<int>(cargoList.size())) return "";

	return cargoList[index].c_str();
}

int UCSO::AddCargo(int index, int slot)
{
	if (index < 0 || index >= static_cast<int>(cargoList.size())) return INVALID_INDEX;
	else if (attachsMap.empty()) return SLOT_UNDEFINED;
	else if (slot == -1) {
		slot = GetEmptySlot();
		if (slot == -1) return SLOT_OCCUPIED;
	}
	else if (attachsMap.find(slot) == attachsMap.end()) return SLOT_UNDEFINED;
	else if (VerifySlot(slot)) return SLOT_OCCUPIED;

	std::string spawnName = cargoList[index];
	SetSpawnName(spawnName);

	std::string className = "UCSO\\";
	className += cargoList[index];

	VESSELSTATUS2 status;
	memset(&status, 0, sizeof(status));
	status.version = 2;
	vessel->GetStatusEx(&status);

	CargoVessel* cargo = static_cast<CargoVessel*>(oapiGetVesselInterface(oapiCreateVesselEx(spawnName.c_str(), className.c_str(), &status)));

	if (!cargo) return ADD_FAILED;
	else if (!vessel->AttachChild(cargo->GetHandle(), attachsMap[slot], cargo->GetAttachmentHandle(true, 0))) return ADD_FAILED;
	else cargo->SetAttachmentState(true);

	return CARGO_ADDED;
}

UCSO::GrappleResult UCSO::GrappleCargo(int slot)
{
	if (attachsMap.empty()) return SLOT_UNDEFINED;

	else if (slot == -1) {
		slot = GetEmptySlot();
		if (slot == -1) return SLOT_OCCUPIED;
	}
	// If the slot doesn't exists
	else if (attachsMap.find(slot) == attachsMap.end()) return SLOT_UNDEFINED;
	else if (VerifySlot(slot)) return SLOT_OCCUPIED;

	for (DWORD vesselIndex = 0; vesselIndex < oapiGetVesselCount(); vesselIndex++)
	{
		VESSEL* vCargo = oapiGetVesselInterface(oapiGetVesselByIndex(vesselIndex));
		if (strncmp(vCargo->GetClassNameA(), "UCSO", 4) == 0) {
			VECTOR3 pos;
			vessel->GetRelativePos(vCargo->GetHandle(), pos);

			if (length(pos) <= grappleDistance) {
				CargoVessel* cargo = static_cast<CargoVessel*>(vCargo);
				if (cargo->GetAttachmentStatus(cargo->GetAttachmentHandle(true, 0))) continue;

				double cargoMass = cargo->GetEmptyMass();

				if (maxCargoMass != -1) { if (cargoMass > maxCargoMass) return MAX_MASS_EXCEEDED; }

				if (maxTotalCargoMass != -1) {
					if (!isTotalMassGet) GetTotalCargoMass();

					if (totalCargoMass + cargoMass > maxTotalCargoMass) return MAX_TOTAL_MASS_EXCEEDED;
					else totalCargoMass += cargoMass;
				}

				if (vessel->AttachChild(cargo->GetHandle(), attachsMap[slot], cargo->GetAttachmentHandle(true, 0))) {
					cargo->SetAttachmentState(true);
					return CARGO_GRAPPLED;
				}
				else return GRAPPLE_FAILED;
			}
		}
	}

	return NO_CARGO_IN_RANGE;
}

UCSO::ReleaseResult UCSO::ReleaseCargo(int slot)
{
	if (attachsMap.empty()) return SLOT_UNDEF;

	std::pair<int, OBJHANDLE> pair;

	if (slot == -1) {
		pair = GetOccupiedSlot();
		if (!pair.second) return SLOT_EMPTY;
	}
	// If the slot doesn't exists
	else if (attachsMap.find(slot) == attachsMap.end()) return SLOT_UNDEF;
	else {
		pair.second = VerifySlot(slot);
		if(!pair.second) return SLOT_EMPTY;
		pair.first = slot;
	}

	bool cargoReleased = false;

	CargoVessel* cargo = static_cast<CargoVessel*>(oapiGetVesselInterface(pair.second));

	if (vessel->GroundContact()); // Release when on ground
	else cargoReleased = vessel->DetachChild(attachsMap[pair.first], releaseVel);
	

	if (cargoReleased) { cargo->SetAttachmentState(false); return CARGO_RELEASED; }

	return RELEASE_FAILED;
}

int UCSO::UnpackCargo(bool isAttached, int slot)
{
	if (isAttached) {
		if (attachsMap.empty()) return SLOT_UNDEF;

		OBJHANDLE cargoHandle;
		if (slot == -1) {
			cargoHandle = GetOccupiedSlot().second;
			if (!cargoHandle) return SLOT_EMPTY;
		}
		else if (attachsMap.find(slot) == attachsMap.end()) return SLOT_UNDEF;
		else {
			cargoHandle = VerifySlot(slot);
			if (!cargoHandle) return SLOT_EMPTY;
		}

		CargoVessel* cargo = static_cast<CargoVessel*>(oapiGetVesselInterface(cargoHandle));
		struct DataStruct dataStruct = cargo->GetDataStruct();

		if (dataStruct.type != UNPACKABLE) return NOT_UNPACKABLE;
		else if (!cargo->UnpackCargo()) return UNPACK_FAILED;

		return CARGO_UNPACKED;
	}
	else {
		for (DWORD vesselIndex = 0; vesselIndex < oapiGetVesselCount(); vesselIndex++)
		{
			VESSEL* vCargo = oapiGetVesselInterface(oapiGetVesselByIndex(vesselIndex));
			if (strncmp(vCargo->GetClassNameA(), "UCSO", 4) == 0) {
				VECTOR3 pos;
				vessel->GetRelativePos(vCargo->GetHandle(), pos);

				if (length(pos) <= unpackDistance) {
					CargoVessel* cargo = static_cast<CargoVessel*>(vCargo);
					OBJHANDLE attachedVessel = cargo->GetAttachmentStatus(cargo->GetAttachmentHandle(true, 0));
					if (attachedVessel != NULL && attachedVessel != vessel->GetHandle()) continue;

					struct DataStruct dataStruct = cargo->GetDataStruct();

					if (dataStruct.type != UNPACKABLE) continue;
					else if (!cargo->UnpackCargo()) return UNPACK_FAILED;
				}
			}
		}
		return NO_UNPACKABLE_IN_RANGE;
	}
}

int UCSO::DeleteCargo(int slot)
{
	if (attachsMap.empty()) return SLOT_UNDEF;

	OBJHANDLE cargoHandle;
	if (slot == -1) {
		cargoHandle = GetOccupiedSlot().second;
		if (!cargoHandle) return SLOT_EMPTY;
	}
	else if (attachsMap.find(slot) == attachsMap.end()) return SLOT_UNDEF;
	else {
		cargoHandle = VerifySlot(slot);
		if (!cargoHandle) return SLOT_EMPTY;
	}

	if (oapiDeleteVessel(cargoHandle)) return CARGO_DELETED;
	return DELETE_FAILED;
}

double UCSO::UseResource(std::string resourceType, double requiredMass, int slot)
{
	if (attachsMap.empty() || requiredMass <= 0 || resourceType.empty()) return 0;

	CargoVessel* cargo;
	if (slot == -1) {
		cargo = GetResourceCargo(resourceType);
		if (!cargo) return 0;
	}
	else if (attachsMap.find(slot) == attachsMap.end()) return 0;
	else {
		OBJHANDLE cargoHandle = VerifySlot(slot);
		if (!cargoHandle) return 0;
		cargo = static_cast<CargoVessel*>(oapiGetVesselInterface(cargoHandle));
	}

	return cargo->UseResource(requiredMass);
}

UCSO::CargoInfo UCSO::GetCargoInfo(int slot)
{
	if (attachsMap.empty() || attachsMap.find(slot) == attachsMap.end() || !VerifySlot(slot)) return CargoInfo();

	CargoVessel* cargoVessel = static_cast<CargoVessel*>(oapiGetVesselInterface(vessel->GetAttachmentStatus(attachsMap[slot])));
	struct DataStruct dataStruct = cargoVessel->GetDataStruct();
	struct CargoInfo cargoInfo;

	switch (dataStruct.type)
	{
	case STATIC:
		cargoInfo.type = "Static";
		break;
	case RESOURCE:
		cargoInfo.type = "Resource";
		break;
	case UNPACKABLE:
		cargoInfo.type = "Unpackable";
		break;
	}
	cargoInfo.mass = cargoVessel->GetMass();

	cargoInfo.resourceType = _strdup(dataStruct.resourceType.c_str());
	cargoInfo.resourceMass = dataStruct.resourceMass;
	
	cargoInfo.spawnModule = _strdup(dataStruct.spawnModule.c_str());
	switch (dataStruct.unpackMode)
	{
	case LANDED:
		cargoInfo.unpackMode = "Landed";
		break;
	case DELAYED:
		cargoInfo.unpackMode = "Delayed";
		break;
	case MANUAL:
		cargoInfo.unpackMode = "Manual";
		break;
	}
	cargoInfo.unpackDelay = dataStruct.unpackDelay;

	return cargoInfo;
}

double UCSO::GetCargoMass(int slot)
{
	if (attachsMap.empty() || attachsMap.find(slot) == attachsMap.end()) return -1;

	OBJHANDLE cargoHandle = VerifySlot(slot);
	if (!cargoHandle) return -1;

	return oapiGetEmptyMass(cargoHandle);
}

double UCSO::GetCargoTotalMass()
{
	if (!isTotalMassGet) GetTotalCargoMass();
	return totalCargoMass;
}

void UCSO::InitCargo()
{
	for (const std::filesystem::directory_entry& entry :
					std::filesystem::directory_iterator(std::filesystem::current_path().string() + "\\Config\\Vessels\\UCSO")) {
		std::string path = entry.path().filename().string();
		cargoList.push_back(path.substr(0, path.find(".cfg")));
		cargoCount++;
	}
}

void UCSO::SetSpawnName(std::string& spawnName)
{
	if (oapiGetVesselByName(&spawnName[0])) {
		for (int index = 1; index++;)
		{
			std::string name = spawnName.c_str() + std::to_string(index - 1);
			if (!oapiGetVesselByName(&name[0])) { spawnName = name; break; }
		}
	}
}

OBJHANDLE UCSO::VerifySlot(int slot)
{
	return vessel->GetAttachmentStatus(attachsMap[slot]);
}

int UCSO::GetEmptySlot()
{
	for (auto const& [slot, attachmentHandle] : attachsMap) if (!VerifySlot(slot)) return slot;
	return -1;
}

std::pair<int, OBJHANDLE> UCSO::GetOccupiedSlot()
{
	for (auto const& [slot, attachmetHandle] : attachsMap) if (VerifySlot(slot)) return { slot, VerifySlot(slot) };
	return {-1, NULL};
}

void UCSO::GetTotalCargoMass()
{
	for (auto const& [slot, attachmetHandle] : attachsMap) {
		OBJHANDLE cargo = VerifySlot(slot);
		if (cargo) totalCargoMass += oapiGetEmptyMass(cargo);
	}
	isTotalMassGet = true;
}

CargoVessel* UCSO::GetResourceCargo(std::string resourceType)
{
	for (auto const& [slot, attachmetHandle] : attachsMap) {
		OBJHANDLE cargoHandle = VerifySlot(slot);
		if (cargoHandle) {
			CargoVessel* cargo = static_cast<CargoVessel*>(oapiGetVesselInterface(cargoHandle));
			struct DataStruct dataStruct = cargo->GetDataStruct();
			if (dataStruct.type == RESOURCE && dataStruct.resourceMass > 0 &&
				_strcmpi(dataStruct.resourceType.c_str(), resourceType.c_str()) == 0) return cargo;
		}
	}
	return nullptr;
}