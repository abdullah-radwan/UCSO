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
	releaseVelocity = 0.05;
	releaseDistance = 5;
	isTotalMassGet = false;
	cargoCount = 0;

	InitCargo();
}

void UCSO::SetSlotAttachment(int slot, ATTACHMENTHANDLE attachmentHandle)
{
	// If the passed slot exists and attachment handle is NULL
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

void UCSO::SetReleaseVelocity(double releaseVelocity)
{
	this->releaseVelocity = releaseVelocity;
}

void UCSO::SetReleaseDistance(double releaseDistance)
{
	this->releaseDistance = releaseDistance;
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

	VECTOR3 rpos;
	vessel->GetAttachmentParams(attachsMap[slot], rpos, VECTOR3(), VECTOR3());
	vessel->Local2Rel(rpos, rpos);
	status.rpos = rpos;
	OBJHANDLE cargoHandle = oapiCreateVesselEx(spawnName.c_str(), className.c_str(), &status);

	if (!cargoHandle) return ADD_FAILED;

	CargoVessel* cargo = static_cast<CargoVessel*>(oapiGetVesselInterface(cargoHandle));
	
	if (!vessel->AttachChild(cargo->GetHandle(), attachsMap[slot], cargo->GetAttachmentHandle(true, 0))) return ADD_FAILED;

	if (isTotalMassGet) totalCargoMass += cargo->GetMass();

	cargo->SetAttachmentState(true);

	return CARGO_ADDED;
}

UCSO::GrappleResult UCSO::GrappleCargo(int slot)
{
	if (attachsMap.empty()) return SLOT_UNDEFINED;

	else if (slot == -1) {
		slot = GetEmptySlot();
		if (slot == -1) return SLOT_OCCUPIED;
	}
	else if (attachsMap.find(slot) == attachsMap.end()) return SLOT_UNDEFINED;
	else if (VerifySlot(slot)) return SLOT_OCCUPIED;

	std::map<double, VESSEL*> cargoMap;
	GrappleResult result = NO_CARGO_IN_RANGE;

	for (DWORD vesselIndex = 0; vesselIndex < oapiGetVesselCount(); vesselIndex++)
	{
		VESSEL* cargo = oapiGetVesselInterface(oapiGetVesselByIndex(vesselIndex));

		if (strncmp(cargo->GetClassNameA(), "UCSO", 4) == 0) {
			VECTOR3 pos;
			vessel->GetRelativePos(cargo->GetHandle(), pos);

			double distance = length(pos);
			if (distance <= grappleDistance) {
				
				if (cargo->GetAttachmentStatus(cargo->GetAttachmentHandle(true, 0))) continue;

				double cargoMass = cargo->GetMass();

				if (maxCargoMass != -1) { if (cargoMass > maxCargoMass) result = MAX_MASS_EXCEEDED; continue; }

				if (maxTotalCargoMass != -1) {
					if (!isTotalMassGet) GetTotalCargoMass();

					if (totalCargoMass + cargoMass > maxTotalCargoMass) { result = MAX_TOTAL_MASS_EXCEEDED; continue; }
				}

				cargoMap[distance] = cargo;
			}
		}
	}

	if (cargoMap.empty()) return result;

	for (auto const& [distance, cargo] : cargoMap) {
		if (vessel->AttachChild(cargo->GetHandle(), attachsMap[slot], cargo->GetAttachmentHandle(true, 0))) {
			CargoVessel* vCargo = static_cast<CargoVessel*>(cargo);
			totalCargoMass += vCargo->GetMass();
			vCargo->SetAttachmentState(true);
			return CARGO_GRAPPLED;
		}
	}

	return GRAPPLE_FAILED;
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
		if (!pair.second) return SLOT_EMPTY;
		pair.first = slot;
	}

	CargoVessel* cargo = static_cast<CargoVessel*>(oapiGetVesselInterface(pair.second));

	if (vessel->GetFlightStatus() & 1) {
		VESSELSTATUS2 status;
		memset(&status, 0, sizeof(status));
		status.version = 2;
		vessel->GetStatusEx(&status);

		double angularDistance = releaseDistance / oapiGetSize(status.rbody);
		double trueCourse = 90 * RAD;

		double releaseLatitude = asin(sin(status.surf_lat) * cos(angularDistance) +
			cos(status.surf_lat) * sin(angularDistance) * cos(trueCourse));

		double releaseLongitude = status.surf_lng + atan2(sin(trueCourse) * sin(angularDistance) * cos(status.surf_lat),
			cos(angularDistance) - sin(status.surf_lat) * sin(releaseLatitude));

		status.surf_lng = releaseLongitude;
		status.surf_lat = releaseLatitude;

		MATRIX3 rot1 = CargoVessel::RotationMatrix({ 0 * RAD, 90 * RAD - releaseLongitude, 0 * RAD });
		MATRIX3 rot2 = CargoVessel::RotationMatrix({ -releaseLatitude + 0 * RAD, 0, 0 * RAD });
		MATRIX3 rot3 = CargoVessel::RotationMatrix({ 0, 0, 180 * RAD + status.surf_hdg });
		MATRIX3 rot4 = CargoVessel::RotationMatrix({ 90 * RAD, 0, 0 });
		MATRIX3 RotMatrix_Def = mul(rot1, mul(rot2, mul(rot3, rot4)));

		status.arot.x = atan2(RotMatrix_Def.m23, RotMatrix_Def.m33);
		status.arot.y = -asin(RotMatrix_Def.m13);
		status.arot.z = atan2(RotMatrix_Def.m12, RotMatrix_Def.m11);
		status.vrot.x = 0.65;

		if (vessel->DetachChild(attachsMap[pair.first])) {
			cargo->DefSetStateEx(&status);
			if (isTotalMassGet) totalCargoMass -= cargo->GetMass();
		}
		else return RELEASE_FAILED;
	}
	else if (!vessel->DetachChild(attachsMap[pair.first], releaseVelocity)) return RELEASE_FAILED;

	cargo->SetAttachmentState(false);
	
	return CARGO_RELEASED;
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
		struct CargoVessel::DataStruct dataStruct = cargo->GetDataStruct();

		if (dataStruct.type != UNPACKABLE) return NOT_UNPACKABLE;
		else if (!cargo->UnpackCargo()) return UNPACK_FAILED;

		if (isTotalMassGet) totalCargoMass -= cargo->GetMass();
		return CARGO_UNPACKED;
	}
	else {
		std::map<double, CargoVessel*> cargoMap;

		for (DWORD vesselIndex = 0; vesselIndex < oapiGetVesselCount(); vesselIndex++)
		{
			VESSEL* cargo = oapiGetVesselInterface(oapiGetVesselByIndex(vesselIndex));

			if (strncmp(cargo->GetClassNameA(), "UCSO", 4) == 0) {
				VECTOR3 pos;
				vessel->GetRelativePos(cargo->GetHandle(), pos);

				double distance = length(pos);
				if (distance <= unpackDistance) { 
					OBJHANDLE attachedVessel = cargo->GetAttachmentStatus(cargo->GetAttachmentHandle(true, 0));
					if (attachedVessel != NULL && attachedVessel != vessel->GetHandle()) continue;

					CargoVessel* vCargo = static_cast<CargoVessel*>(cargo);
					struct CargoVessel::DataStruct dataStruct = vCargo->GetDataStruct();

					if (dataStruct.type == UNPACKABLE) cargoMap[distance] = vCargo;
				}
			}
		}

		if (cargoMap.empty()) return NO_UNPACKABLE_IN_RANGE;

		for (auto const& [distance, cargo] : cargoMap) {
			bool isCargoAttached = cargo->GetAttachmentStatus(cargo->GetAttachmentHandle(true, 0));
			if (cargo->UnpackCargo()) { 
				if(isCargoAttached) if (isTotalMassGet) totalCargoMass -= cargo->GetMass();
				return CARGO_UNPACKED;
			}
		}

		return UNPACK_FAILED;
	}
}

int UCSO::DeleteCargo(int slot)
{
	if (attachsMap.empty()) return SLOT_UNDEF;

	OBJHANDLE cargoHandle;

	if (attachsMap.find(slot) == attachsMap.end()) return SLOT_UNDEF;
	else {
		cargoHandle = VerifySlot(slot);
		if (!cargoHandle) return SLOT_EMPTY;
	}

	double cargoMass = oapiGetMass(cargoHandle);
	if (!oapiDeleteVessel(cargoHandle)) return DELETE_FAILED;

	if (isTotalMassGet) totalCargoMass -= cargoMass;
	return CARGO_DELETED;
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

	struct CargoInfo cargoInfo;
	struct CargoVessel::DataStruct dataStruct = cargoVessel->GetDataStruct();

	cargoInfo.valid = true;
	cargoInfo.type = static_cast<CargoType>(dataStruct.type);
	cargoInfo.mass = cargoVessel->GetMass();

	switch (cargoInfo.type)
	{
	case RESOURCE:
		cargoInfo.resourceType = _strdup(dataStruct.resourceType.c_str());
		cargoInfo.resourceMass = dataStruct.resourceMass;
		break;
	case UNPACKABLE:
		cargoInfo.spawnModule = _strdup(dataStruct.spawnModule.c_str());
		cargoInfo.unpackMode = static_cast<UnpackMode>(dataStruct.unpackMode);
		if(cargoInfo.unpackMode == DELAYED) cargoInfo.unpackDelay = dataStruct.unpackDelay;
		break;
	default:
		break;
	}

	return cargoInfo;
}

double UCSO::GetCargoMass(int slot)
{
	if (attachsMap.empty() || attachsMap.find(slot) == attachsMap.end()) return -1;

	OBJHANDLE cargoHandle = VerifySlot(slot);
	if (!cargoHandle) return -1;

	return oapiGetMass(cargoHandle);
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
	return { -1, NULL };
}

void UCSO::GetTotalCargoMass()
{
	for (auto const& [slot, attachmetHandle] : attachsMap) {
		OBJHANDLE cargo = VerifySlot(slot);
		if (cargo) totalCargoMass += oapiGetMass(cargo);
	}

	isTotalMassGet = true;
}

CargoVessel* UCSO::GetResourceCargo(std::string resourceType)
{
	for (auto const& [slot, attachmetHandle] : attachsMap) {
		OBJHANDLE cargoHandle = VerifySlot(slot);
		if (cargoHandle) {
			CargoVessel* cargo = static_cast<CargoVessel*>(oapiGetVesselInterface(cargoHandle));
			struct CargoVessel::DataStruct dataStruct = cargo->GetDataStruct();
			if (dataStruct.type == RESOURCE && dataStruct.resourceMass > 0 &&
				_strcmpi(dataStruct.resourceType.c_str(), resourceType.c_str()) == 0) return cargo;
		}
	}

	return nullptr;
}
