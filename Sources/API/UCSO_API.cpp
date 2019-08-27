#include "UCSO_API.h"
#include <filesystem>

void AttachmentException(unsigned int u, EXCEPTION_POINTERS* pExp) { throw; }

UCSO* UCSO::CreateInstance(VESSEL* vessel)
{
	_set_se_translator(AttachmentException);

	return new UCSO(vessel);
}

UCSO::UCSO(VESSEL* vessel)
{
	this->vessel = vessel;
	attachsMap = std::map<int, ATTACHMENTHANDLE>();
	doorsMap = std::map<int, bool>();

	version = 0;

	maxCargoMass = -1;
	maxTotalCargoMass = -1;
	grappleDistance = 50;
	grappleUnpacked = false;

	releaseVelocity = 0.05;
	rowLength = 4;

	unpackDistance = 3;

	InitCargo();

	HINSTANCE hinstDLL = LoadLibrary("Modules\\UCSO.dll");

	if (hinstDLL) {
		typedef double (*GetUCSOBuildPointer)();

		GetUCSOBuildPointer GetUCSOBuild = reinterpret_cast<GetUCSOBuildPointer>(GetProcAddress(hinstDLL, "GetUCSOBuild"));

		if (GetUCSOBuild) version = GetUCSOBuild();

		FreeLibrary(hinstDLL);
	}
}

double UCSO::GetUCSOVersion()
{
	return version;
}

bool UCSO::SetSlotAttachment(int slot, ATTACHMENTHANDLE attachmentHandle, bool opened)
{
	// If the passed slot exists and attachment handle is NULL
	if (attachsMap.find(slot) != attachsMap.end() && !attachmentHandle) 
	{
		attachsMap.erase(slot);
		doorsMap.erase(slot);
		return true;
	}
	else if (attachmentHandle && CheckAttachment(attachmentHandle)) 
	{ 
		attachsMap[slot] = attachmentHandle;
		doorsMap[slot] = opened;
		return true;
	}

	return false;
}

void UCSO::SetSlotDoor(bool opened, int slot)
{
	if(slot == -1) for (auto const& [slot, opened] : doorsMap) doorsMap[slot] = opened;
	else if(doorsMap.find(slot) != doorsMap.end()) doorsMap[slot] = opened;
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

void UCSO::SetUnpackedGrapple(bool grappleUnpacked)
{
	this->grappleUnpacked = grappleUnpacked;
}

void UCSO::SetReleaseVelocity(double releaseVelocity)
{
	this->releaseVelocity = releaseVelocity;
}

void UCSO::SetCargoRowLength(int rowLength)
{
	this->rowLength = rowLength;
}

void UCSO::SetMaxUnpackDistance(double unpackDistance)
{
	this->unpackDistance = unpackDistance;
}

int UCSO::GetAvailableCargoCount()
{
	return availableCargoList.size();
}

const char* UCSO::GetAvailableCargoName(int index)
{
	if (index < 0 || index >= static_cast<int>(availableCargoList.size())) return "";

	return availableCargoList[index].c_str();
}

UCSO::CargoInfo UCSO::GetCargoInfo(int slot)
{
	if (attachsMap.empty() || attachsMap.find(slot) == attachsMap.end() || !VerifySlot(slot)) return CargoInfo();

	CargoVessel* cargoVessel = static_cast<CargoVessel*>(oapiGetVesselInterface(vessel->GetAttachmentStatus(attachsMap[slot])));

	struct CargoInfo cargoInfo;
	struct CargoVessel::DataStruct dataStruct = cargoVessel->GetDataStruct();

	cargoInfo.valid = true;
	cargoInfo.name = cargoVessel->GetName();
	cargoInfo.type = static_cast<CargoType>(dataStruct.type);
	cargoInfo.mass = cargoVessel->GetMass();

	switch (cargoInfo.type)
	{
	case RESOURCE:
		cargoInfo.resourceType = _strdup(dataStruct.resourceType.c_str());
		cargoInfo.resourceMass = dataStruct.netMass;

		break;
	case UNPACKABLE:
		cargoInfo.unpackType = static_cast<UnpackType>(dataStruct.unpackType);

		if (cargoInfo.unpackType == ORBITER_VESSEL) 
		{
			cargoInfo.spawnModule = _strdup(dataStruct.spawnModule.c_str());
			cargoInfo.unpackMode = static_cast<UnpackMode>(dataStruct.unpackMode);
			if (cargoInfo.unpackMode == DELAYED) cargoInfo.unpackDelay = dataStruct.unpackDelay;
		}

		break;
	default:
		break;
	}

	return cargoInfo;
}

double UCSO::GetCargoMass(int slot)
{
	if (attachsMap.empty() || attachsMap.find(slot) == attachsMap.end() || !CheckAttachment(attachsMap[slot])) return -1;

	OBJHANDLE cargoHandle = VerifySlot(slot);
	if (!cargoHandle) return -1;

	return oapiGetMass(cargoHandle);
}

double UCSO::GetTotalCargoMass()
{
	double totalCargoMass = 0;

	for (auto const& [slot, attachmetHandle] : attachsMap) 
	{
		if (!CheckAttachment(attachsMap[slot])) continue;
		OBJHANDLE cargo = VerifySlot(slot);
		if (cargo) totalCargoMass += oapiGetMass(cargo);
	}

	return totalCargoMass;
}

int UCSO::AddCargo(int index, int slot)
{
	if (index < 0 || index >= static_cast<int>(availableCargoList.size())) return INVALID_INDEX;
	else if (attachsMap.empty()) return GRAPPLE_SLOT_UNDEFINED;
	else if (slot == -1)
	{
		slot = GetEmptySlot();
		if (slot == -1) return GRAPPLE_SLOT_OCCUPIED;
	}
	else if (attachsMap.find(slot) == attachsMap.end() || !CheckAttachment(attachsMap[slot])) return GRAPPLE_SLOT_UNDEFINED;
	else if (!doorsMap[slot]) return GRAPPLE_SLOT_CLOSED;
	else if (VerifySlot(slot)) return GRAPPLE_SLOT_OCCUPIED;

	std::string spawnName = availableCargoList[index];
	SetSpawnName(spawnName);

	std::string className = "UCSO\\";
	className += availableCargoList[index];

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

	cargo->SetAttachmentState(true);

	return CARGO_ADDED;
}

UCSO::GrappleResult UCSO::GrappleCargo(int slot)
{
	if (attachsMap.empty()) return GRAPPLE_SLOT_UNDEFINED;

	else if (slot == -1) 
	{
		slot = GetEmptySlot();
		if (slot == -1) return GRAPPLE_SLOT_OCCUPIED;
	}
	else if (attachsMap.find(slot) == attachsMap.end() || !CheckAttachment(attachsMap[slot])) return GRAPPLE_SLOT_UNDEFINED;
	else if (VerifySlot(slot)) return GRAPPLE_SLOT_OCCUPIED;

	std::map<double, CargoVessel*> cargoMap;
	GrappleResult result = NO_CARGO_IN_RANGE;

	for (DWORD vesselIndex = 0; vesselIndex < oapiGetVesselCount(); vesselIndex++)
	{
		VESSEL* cargo = oapiGetVesselInterface(oapiGetVesselByIndex(vesselIndex));

		if (strncmp(cargo->GetClassNameA(), "UCSO", 4) == 0) 
		{
			VECTOR3 pos;
			vessel->GetRelativePos(cargo->GetHandle(), pos);

			double distance = length(pos);

			if (distance <= grappleDistance) 
			{
				if (cargo->GetAttachmentStatus(cargo->GetAttachmentHandle(true, 0))) continue;

				if (maxCargoMass != -1) if (cargo->GetMass() > maxCargoMass) { result = MAX_MASS_EXCEEDED; continue; }

				if (maxTotalCargoMass != -1)
					if (GetTotalCargoMass() + cargo->GetMass() > maxTotalCargoMass) { result = MAX_TOTAL_MASS_EXCEEDED; continue; }

				CargoVessel* vCargo = static_cast<CargoVessel*>(cargo);
				struct CargoVessel::DataStruct dataStruct = vCargo->GetDataStruct();

				if(grappleUnpacked || !dataStruct.unpacked) cargoMap[distance] = vCargo;
			}
		}
	}

	if (cargoMap.empty()) return result;

	for (auto const& [distance, cargo] : cargoMap) 
	{
		if (vessel->AttachChild(cargo->GetHandle(), attachsMap[slot], cargo->GetAttachmentHandle(true, 0))) 
		{
			cargo->SetAttachmentState(true);
			return CARGO_GRAPPLED;
		}
	}

	return GRAPPLE_FAILED;
}

UCSO::ReleaseResult UCSO::ReleaseCargo(int slot)
{
	if (attachsMap.empty()) return RELEASE_SLOT_UNDEFINED;

	std::pair<int, OBJHANDLE> pair;

	if (slot == -1) {
		pair = GetOccupiedSlot();
		if (!pair.second) return RELEASE_SLOT_EMPTY;
		slot = pair.first;
	}
	// If the slot doesn't exists
	else if (attachsMap.find(slot) == attachsMap.end() || !CheckAttachment(attachsMap[slot])) return RELEASE_SLOT_UNDEFINED;
	else if (!doorsMap[slot]) return RELEASE_SLOT_CLOSED;
	else {
		pair.second = VerifySlot(slot);
		if (!pair.second) return RELEASE_SLOT_EMPTY;
	}

	CargoVessel* cargo = static_cast<CargoVessel*>(oapiGetVesselInterface(pair.second));

	if (vessel->GetFlightStatus() & 1) 
	{
		VESSELSTATUS2 status;
		memset(&status, 0, sizeof(status));
		status.version = 2;
		vessel->GetStatusEx(&status);

		VECTOR3 pos;
		vessel->GetAttachmentParams(attachsMap[slot], pos, VECTOR3(), VECTOR3());
		if(!GetNearestEmptyLocation(SetGroundList(), pos)) return NO_EMPTY_POSITION;
		vessel->HorizonRot(pos, pos);

		double metersPerDegree = (oapiGetSize(status.rbody) * 2 * PI) / 360;
		status.surf_lng += (pos.x / metersPerDegree) * RAD;
		status.surf_lat += (pos.z / metersPerDegree) * RAD;

		CargoVessel::SetGroundRotation(status, 0.65);

		if (vessel->DetachChild(attachsMap[slot])) cargo->DefSetStateEx(&status);
		else return RELEASE_FAILED;
	}
	else if (!vessel->DetachChild(attachsMap[slot], releaseVelocity)) return RELEASE_FAILED;

	cargo->SetAttachmentState(false);
	
	return CARGO_RELEASED;
}

bool UCSO::PackCargo()
{
	for (DWORD vesselIndex = 0; vesselIndex < oapiGetVesselCount(); vesselIndex++)
	{
		VESSEL* cargo = oapiGetVesselInterface(oapiGetVesselByIndex(vesselIndex));

		if (strncmp(cargo->GetClassNameA(), "UCSO", 4) == 0) 
		{
			VECTOR3 pos;
			vessel->GetRelativePos(cargo->GetHandle(), pos);

			if (length(pos) <= unpackDistance) 
			{
				if (cargo->GetAttachmentStatus(cargo->GetAttachmentHandle(true, 0))) continue;

				CargoVessel* vCargo = static_cast<CargoVessel*>(cargo);
				struct CargoVessel::DataStruct dataStruct = vCargo->GetDataStruct();

				if (dataStruct.unpacked) { vCargo->PackCargo(); return true; }
			}
		}
	}

	return false;
}

UCSO::UnpackResult UCSO::UnpackCargo()
{
	std::map<double, CargoVessel*> cargoMap;

	for (DWORD vesselIndex = 0; vesselIndex < oapiGetVesselCount(); vesselIndex++)
	{
		VESSEL* cargo = oapiGetVesselInterface(oapiGetVesselByIndex(vesselIndex));

		if (strncmp(cargo->GetClassNameA(), "UCSO", 4) == 0) 
		{
			VECTOR3 pos;
			vessel->GetRelativePos(cargo->GetHandle(), pos);

			double distance = length(pos);
			if (distance <= unpackDistance) 
			{
				if (cargo->GetAttachmentStatus(cargo->GetAttachmentHandle(true, 0))) continue;

				CargoVessel* vCargo = static_cast<CargoVessel*>(cargo);
				struct CargoVessel::DataStruct dataStruct = vCargo->GetDataStruct();

				if (dataStruct.type == UNPACKABLE && !dataStruct.unpacked) cargoMap[distance] = vCargo;
			}
		}
	}

	if (cargoMap.empty()) return NO_UNPACKABLE_IN_RANGE;

	for (auto const& [distance, cargo] : cargoMap) if (cargo->UnpackCargo()) return CARGO_UNPACKED;

	return UNPACK_FAILED;
}

int UCSO::DeleteCargo(int slot)
{
	if (attachsMap.empty()) return RELEASE_SLOT_UNDEFINED;

	OBJHANDLE cargoHandle;

	if (attachsMap.find(slot) == attachsMap.end() || !CheckAttachment(attachsMap[slot])) return RELEASE_SLOT_UNDEFINED;
	else 
	{
		cargoHandle = VerifySlot(slot);
		if (!cargoHandle) return RELEASE_SLOT_EMPTY;
	}

	double cargoMass = oapiGetMass(cargoHandle);
	if (!oapiDeleteVessel(cargoHandle)) return DELETE_FAILED;

	return CARGO_DELETED;
}

double UCSO::UseResource(std::string resourceType, double requiredMass, int slot)
{
	if (attachsMap.empty() || requiredMass <= 0 || resourceType.empty()) return 0;

	CargoVessel* cargo;

	if (slot == -1) 
	{
		cargo = GetResourceCargo(resourceType);
		if (!cargo) return 0;
	}
	else if (attachsMap.find(slot) == attachsMap.end() || !CheckAttachment(attachsMap[slot])) return 0;
	else 
	{
		OBJHANDLE cargoHandle = VerifySlot(slot);
		if (!cargoHandle) return 0;
		cargo = static_cast<CargoVessel*>(oapiGetVesselInterface(cargoHandle));
	}

	return cargo->UseResource(requiredMass);
}

void UCSO::InitCargo()
{
	for (const std::filesystem::directory_entry& entry :
					std::filesystem::directory_iterator(std::filesystem::current_path().string() + "\\Config\\Vessels\\UCSO")) 
	{
		std::string path = entry.path().filename().string();
		availableCargoList.push_back(path.substr(0, path.find(".cfg")));
	}
}

bool UCSO::CheckAttachment(ATTACHMENTHANDLE attachHandle)
{
	try { vessel->GetAttachmentIndex(attachHandle); return true; }
	catch (...) { return false; }
}

std::vector<VECTOR3> UCSO::SetGroundList()
{
	std::vector<VECTOR3> groundList;

	for (DWORD vesselIndex = 0; vesselIndex < oapiGetVesselCount(); vesselIndex++)
	{
		VESSEL* cargo = oapiGetVesselInterface(oapiGetVesselByIndex(vesselIndex));

		if (strncmp(cargo->GetClassNameA(), "UCSO", 4) == 0) 
		{
			VECTOR3 cargoPos;

			cargo->GetGlobalPos(cargoPos);
			vessel->Global2Local(cargoPos, cargoPos);

			if (cargoPos.x <= 11 && cargoPos.x >= 3.5 && cargoPos.z <= rowLength) groundList.push_back(cargoPos);
		}
	}

	return groundList;
}

bool UCSO::GetNearestEmptyLocation(std::vector<VECTOR3> groundList, VECTOR3& initialPos)
{
	initialPos.x += 5;
	double length = 0;
	VECTOR3 pos = initialPos;

loop:
	for (VECTOR3 cargoPos : groundList) 
	{
		VECTOR3 subtract = pos - cargoPos;

		if (sqrt(subtract.x * subtract.x + subtract.z * subtract.z) < 1)
		{
			pos = initialPos;
			length += 1.5;
			pos.z += static_cast<int>(length / 6) * 1.5;
			pos.x += length - (static_cast<int>(length / 6) * 6.0);
			goto loop;
		}
	}

	if (pos.z > rowLength) return false;

	initialPos = pos;

	return true;
}

void UCSO::SetSpawnName(std::string& spawnName)
{
	if (oapiGetVesselByName(&spawnName[0])) 
	{
		for (int index = 1; index++;)
		{
			std::string name = spawnName.c_str() + std::to_string(index);
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
	for (auto const& [slot, attachmentHandle] : attachsMap) 
	{ 
		if (!CheckAttachment(attachmentHandle)) continue;
		if (!VerifySlot(slot)) return slot;
	}
	return -1;
}

std::pair<int, OBJHANDLE> UCSO::GetOccupiedSlot()
{
	for (auto const& [slot, attachmentHandle] : attachsMap)
	{ 
		if (!CheckAttachment(attachmentHandle)) continue;
		if (VerifySlot(slot)) return { slot, VerifySlot(slot) };
	}
	return { -1, NULL };
}

CargoVessel* UCSO::GetResourceCargo(std::string resourceType)
{
	for (auto const& [slot, attachmentHandle] : attachsMap)
	{
		if (!CheckAttachment(attachmentHandle)) continue;

		OBJHANDLE cargoHandle = VerifySlot(slot);

		if (cargoHandle) 
		{
			CargoVessel* cargo = static_cast<CargoVessel*>(oapiGetVesselInterface(cargoHandle));
			struct CargoVessel::DataStruct dataStruct = cargo->GetDataStruct();
			if (dataStruct.type == RESOURCE && dataStruct.netMass > 0 &&
				_strcmpi(dataStruct.resourceType.c_str(), resourceType.c_str()) == 0) return cargo;
		}
	}

	return nullptr;
}
