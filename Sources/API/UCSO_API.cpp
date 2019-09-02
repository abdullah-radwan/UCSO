#include "UCSO_API.h"
#include <filesystem>

void AttachmentException(unsigned int u, EXCEPTION_POINTERS* pExp) { throw; }

UCSO* UCSO::CreateInstance(VESSEL* vessel)
{
	// Set the attachment exception handler
	_set_se_translator(AttachmentException);

	return new UCSO_API(vessel);
}

UCSO_API::UCSO_API(VESSEL* vessel)
{
	// Initiate variables here, as inititing them in the header doesn't work
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

	// Set the available cargo list
	InitAvailableCargo();

	// Load UCSO DLL
	HINSTANCE hinstDLL = LoadLibrary("Modules\\UCSO.dll");

	// If the DLL is loaded
	if (hinstDLL) 
	{
		typedef double (*GetUCSOBuildPointer)();

		GetUCSOBuildPointer GetUCSOBuild = reinterpret_cast<GetUCSOBuildPointer>(GetProcAddress(hinstDLL, "GetUCSOBuild"));

		// If the function is found, set the version
		if (GetUCSOBuild) version = GetUCSOBuild();

		FreeLibrary(hinstDLL);
	}
}

double UCSO_API::GetUCSOVersion() const
{
	return version;
}

bool UCSO_API::SetSlotAttachment(int slot, ATTACHMENTHANDLE attachmentHandle, bool opened)
{
	// If the passed slot exists and attachment handle is NULL
	if (attachsMap.find(slot) != attachsMap.end() && !attachmentHandle) 
	{
		attachsMap.erase(slot);
		doorsMap.erase(slot);
		return true;
	}
	// If the attachment handle isn't NULL and the attachment is valid
	else if (attachmentHandle && CheckAttachment(attachmentHandle)) 
	{ 
		attachsMap[slot] = attachmentHandle;
		doorsMap[slot] = opened;
		return true;
	}

	return false;
}

void UCSO_API::SetSlotDoor(bool opened, int slot)
{
	// Set every slot door status if -1 is passed
	if(slot == -1) for (auto & [slot, doorOpened] : doorsMap) doorOpened = opened;
	// If the slot exists
	else if(doorsMap.find(slot) != doorsMap.end()) doorsMap[slot] = opened;
}

void UCSO_API::SetMaxCargoMass(double maxCargoMass)
{
	this->maxCargoMass = maxCargoMass;
}

void UCSO_API::SetMaxTotalCargoMass(double maxTotalCargoMass)
{
	this->maxTotalCargoMass = maxTotalCargoMass;
}

void UCSO_API::SetMaxGrappleDistance(double grappleDistance)
{
	this->grappleDistance = grappleDistance;
}

void UCSO_API::SetUnpackedGrapple(bool grappleUnpacked)
{
	this->grappleUnpacked = grappleUnpacked;
}

void UCSO_API::SetReleaseVelocity(double releaseVelocity)
{
	this->releaseVelocity = releaseVelocity;
}

void UCSO_API::SetCargoRowLength(int rowLength)
{
	this->rowLength = rowLength;
}

void UCSO_API::SetMaxUnpackDistance(double unpackDistance)
{
	this->unpackDistance = unpackDistance;
}

int UCSO_API::GetAvailableCargoCount() const
{
	return availableCargoList.size();
}

const char* UCSO_API::GetAvailableCargoName(int index) const
{
	// If the index is invalid (lower than 0 or higher than list count
	if (index < 0 || index >= static_cast<int>(availableCargoList.size())) return "";

	return availableCargoList[index].c_str();
}

UCSO::CargoInfo UCSO_API::GetCargoInfo(int slot)
{
	// If the slot isn't defined, isn't valid, or empty
	if (attachsMap.empty() || attachsMap.find(slot) == attachsMap.end() || 
		!CheckAttachment(attachsMap[slot]) || !VerifySlot(slot)) return CargoInfo();

	CargoInterface* cargoInterface = static_cast<CargoInterface*>(oapiGetVesselInterface(vessel->GetAttachmentStatus(attachsMap[slot])));

	CargoInfo cargoInfo;
	DataStruct dataStruct = cargoInterface->GetDataStruct();

	cargoInfo.valid = true;
	cargoInfo.name = cargoInterface->GetName();
	cargoInfo.type = static_cast<CargoType>(dataStruct.type);
	cargoInfo.mass = cargoInterface->GetMass();

	switch (cargoInfo.type)
	{
	case RESOURCE:
		// _strdup is used to have a 'permanet' string
		// If not used, the string will be removed as soon as the function ends
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

double UCSO_API::GetCargoMass(int slot)
{
	if (attachsMap.empty() || attachsMap.find(slot) == attachsMap.end() || !CheckAttachment(attachsMap[slot])) return -1;

	OBJHANDLE cargoHandle = VerifySlot(slot);
	if (!cargoHandle) return -1;

	return oapiGetMass(cargoHandle);
}

double UCSO_API::GetTotalCargoMass()
{
	double totalCargoMass = 0;

	for (auto const& [slot, attachmetHandle] : attachsMap) 
	{
		// If the slot isn't valid
		if (!CheckAttachment(attachmetHandle)) continue;

		OBJHANDLE cargo = VerifySlot(slot);

		// If the slot is occupied
		if (cargo) totalCargoMass += oapiGetMass(cargo);
	}

	return totalCargoMass;
}

int UCSO_API::AddCargo(int index, int slot)
{
	if (index < 0 || index >= static_cast<int>(availableCargoList.size())) return INVALID_INDEX;
	else if (attachsMap.empty()) return GRAPPLE_SLOT_UNDEFINED;
	else if (slot == -1)
	{
		// Get the first empty slot
		slot = GetEmptySlot();
		// If no slot is empty
		if (slot == -1) return GRAPPLE_SLOT_OCCUPIED;
	}
	else if (attachsMap.find(slot) == attachsMap.end() || !CheckAttachment(attachsMap[slot])) return GRAPPLE_SLOT_UNDEFINED;
	// If the slot is closed
	else if (!doorsMap[slot]) return GRAPPLE_SLOT_CLOSED;
	// If a cargo is already attached to the slot
	else if (VerifySlot(slot)) return GRAPPLE_SLOT_OCCUPIED;

	std::string spawnName = availableCargoList[index] + "1";
	SetSpawnName(spawnName);

	std::string className = "UCSO\\";
	className += availableCargoList[index];

	VESSELSTATUS2 status;
	memset(&status, 0, sizeof(status));
	status.version = 2;
	vessel->GetStatusEx(&status);

	OBJHANDLE cargoHandle = oapiCreateVesselEx(spawnName.c_str(), className.c_str(), &status);

	// If the cargo isn't created
	if (!cargoHandle) return ADD_FAILED;

	CargoInterface* cargo = static_cast<CargoInterface*>(oapiGetVesselInterface(cargoHandle));
	
	// If the cargo couldn't be attached
	if (!vessel->AttachChild(cargo->GetHandle(), attachsMap[slot], cargo->GetAttachmentHandle(true, 0))) return ADD_FAILED;

	cargo->SetAttachmentState(true);

	return CARGO_ADDED;
}

UCSO::GrappleResult UCSO_API::GrappleCargo(int slot)
{
	if (attachsMap.empty()) return GRAPPLE_SLOT_UNDEFINED;

	else if (slot == -1) 
	{
		slot = GetEmptySlot();
		if (slot == -1) return GRAPPLE_SLOT_OCCUPIED;
	}
	else if (attachsMap.find(slot) == attachsMap.end() || !CheckAttachment(attachsMap[slot])) return GRAPPLE_SLOT_UNDEFINED;
	else if (VerifySlot(slot)) return GRAPPLE_SLOT_OCCUPIED;

	std::map<double, CargoInterface*> cargoMap;
	GrappleResult result = NO_CARGO_IN_RANGE;

	for (DWORD vesselIndex = 0; vesselIndex < oapiGetVesselCount(); vesselIndex++)
	{
		VESSEL* cargo = oapiGetVesselInterface(oapiGetVesselByIndex(vesselIndex));

		// If the vessel is UCSO cargo
		if (strncmp(cargo->GetClassNameA(), "UCSO", 4) == 0) 
		{
			VECTOR3 pos;
			vessel->GetRelativePos(cargo->GetHandle(), pos);

			double distance = length(pos);

			if (distance <= grappleDistance) 
			{
				// If the cargo is attached to another vessel
				if (cargo->GetAttachmentStatus(cargo->GetAttachmentHandle(true, 0))) continue;

				// If the maximum cargo mass is set and the cargo mass is higher than it
				if (maxCargoMass != -1) if (cargo->GetMass() > maxCargoMass) { result = MAX_MASS_EXCEEDED; continue; }

				// If the maximum total cargo mass is set and the cargo mass plus the total mass is higher than it
				if (maxTotalCargoMass != -1)
					if (GetTotalCargoMass() + cargo->GetMass() > maxTotalCargoMass) { result = MAX_TOTAL_MASS_EXCEEDED; continue; }

				CargoInterface* vCargo = static_cast<CargoInterface*>(cargo);
				DataStruct dataStruct = vCargo->GetDataStruct();

				// If grapple unpacked is true, or if it's false, check if the cargo is packed
				if(grappleUnpacked || !dataStruct.unpacked) cargoMap[distance] = vCargo;
			}
		}
	}

	// If no cargo is added, return the latest cargo error
	if (cargoMap.empty()) return result;

	for (auto const& [distance, cargo] : cargoMap) 
	{
		// If the cargo is attached
		if (vessel->AttachChild(cargo->GetHandle(), attachsMap[slot], cargo->GetAttachmentHandle(true, 0))) 
		{
			cargo->SetAttachmentState(true);
			return CARGO_GRAPPLED;
		}
	}

	return GRAPPLE_FAILED;
}

UCSO::ReleaseResult UCSO_API::ReleaseCargo(int slot)
{
	if (attachsMap.empty()) return RELEASE_SLOT_UNDEFINED;

	std::pair<int, OBJHANDLE> pair;

	if (slot == -1) {
		// Get the first occupied slot and OBJHANDLE for the attached cargo
		pair = GetOccupiedSlot();
		// If the handle is NULL
		if (!pair.second) return RELEASE_SLOT_EMPTY;
		slot = pair.first;
	}
	// If the slot doesn't exists or it's invalid
	else if (attachsMap.find(slot) == attachsMap.end() || !CheckAttachment(attachsMap[slot])) return RELEASE_SLOT_UNDEFINED;
	else if (!doorsMap[slot]) return RELEASE_SLOT_CLOSED;
	else {
		pair.second = VerifySlot(slot);
		if (!pair.second) return RELEASE_SLOT_EMPTY;
	}

	CargoInterface* cargo = static_cast<CargoInterface*>(oapiGetVesselInterface(pair.second));

	// If the vessel is landed
	if (vessel->GetFlightStatus() & 1) 
	{
		VESSELSTATUS2 status;
		memset(&status, 0, sizeof(status));
		status.version = 2;
		vessel->GetStatusEx(&status);

		VECTOR3 pos;
		// Get the attachment position
		vessel->GetAttachmentParams(attachsMap[slot], pos, VECTOR3(), VECTOR3());
		// Get the nearest empty location on the ground
		if(!GetNearestEmptyLocation(SetGroundList(), pos)) return NO_EMPTY_POSITION;
		// Rotate to the horizon frame
		vessel->HorizonRot(pos, pos);

		// Get the meter per degree
		double metersPerDegree = (oapiGetSize(status.rbody) * 2 * PI) / 360;
		status.surf_lng += (pos.x / metersPerDegree) * RAD;
		status.surf_lat += (pos.z / metersPerDegree) * RAD;

		SetGroundRotation(status, 0.65);

		// If the cargo is detached, set the status
		if (vessel->DetachChild(attachsMap[slot])) cargo->DefSetStateEx(&status);
		else return RELEASE_FAILED;
	}
	// If released in space, release with the release velocity
	else if (!vessel->DetachChild(attachsMap[slot], releaseVelocity)) return RELEASE_FAILED;

	cargo->SetAttachmentState(false);
	
	return CARGO_RELEASED;
}

bool UCSO_API::PackCargo()
{
	for (DWORD vesselIndex = 0; vesselIndex < oapiGetVesselCount(); vesselIndex++)
	{
		VESSEL* cargo = oapiGetVesselInterface(oapiGetVesselByIndex(vesselIndex));

		// If the vessel is UCSO cargo
		if (strncmp(cargo->GetClassNameA(), "UCSO", 4) == 0) 
		{
			VECTOR3 pos;
			vessel->GetRelativePos(cargo->GetHandle(), pos);

			if (length(pos) <= unpackDistance) 
			{
				if (cargo->GetAttachmentStatus(cargo->GetAttachmentHandle(true, 0))) continue;

				CargoInterface* vCargo = static_cast<CargoInterface*>(cargo);
				DataStruct dataStruct = vCargo->GetDataStruct();

				// If the cargo is packed
				if (dataStruct.unpacked) { vCargo->PackCargo(); return true; }
			}
		}
	}

	return false;
}

UCSO::UnpackResult UCSO_API::UnpackCargo()
{
	std::map<double, CargoInterface*> cargoMap;

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

				CargoInterface* vCargo = static_cast<CargoInterface*>(cargo);
				DataStruct dataStruct = vCargo->GetDataStruct();

				// If the cargo is unpackable and not unpacked
				if (dataStruct.type == UNPACKABLE && !dataStruct.unpacked) cargoMap[distance] = vCargo;
			}
		}
	}

	if (cargoMap.empty()) return NO_UNPACKABLE_IN_RANGE;

	for (auto const& [distance, cargo] : cargoMap) if (cargo->UnpackCargo()) return CARGO_UNPACKED;

	return UNPACK_FAILED;
}

int UCSO_API::DeleteCargo(int slot)
{
	if (attachsMap.empty()) return RELEASE_SLOT_UNDEFINED;

	OBJHANDLE cargoHandle;

	if (attachsMap.find(slot) == attachsMap.end() || !CheckAttachment(attachsMap[slot])) return RELEASE_SLOT_UNDEFINED;
	else 
	{
		cargoHandle = VerifySlot(slot);
		// If no cargo is attached to the slot
		if (!cargoHandle) return RELEASE_SLOT_EMPTY;
	}

	if (!oapiDeleteVessel(cargoHandle)) return DELETE_FAILED;

	return CARGO_DELETED;
}

double UCSO_API::UseResource(std::string resourceType, double requiredMass, int slot)
{
	if (attachsMap.empty() || requiredMass <= 0 || resourceType.empty()) return 0;

	CargoInterface* cargo;

	if (slot == -1) 
	{
		// Get the first resource cargo
		cargo = GetResourceCargo(resourceType);
		if (!cargo) return 0;
	}
	else if (attachsMap.find(slot) == attachsMap.end() || !CheckAttachment(attachsMap[slot])) return 0;
	else 
	{
		OBJHANDLE cargoHandle = VerifySlot(slot);
		if (!cargoHandle) return 0;
		cargo = static_cast<CargoInterface*>(oapiGetVesselInterface(cargoHandle));
	}

	return cargo->UseResource(requiredMass);
}

void UCSO_API::InitAvailableCargo()
{
	// Itereate through every file in Config/Vessels/UCSO
	for (const std::filesystem::directory_entry& entry :
					std::filesystem::directory_iterator(std::filesystem::current_path().string() + "\\Config\\Vessels\\UCSO")) 
	{
		// Get the filename
		std::string path = entry.path().filename().string();
		// Remove .cfg from the filename
		availableCargoList.push_back(path.substr(0, path.find(".cfg")));
	}
}

bool UCSO_API::CheckAttachment(ATTACHMENTHANDLE attachHandle)
{
	// If the attachment is invalid, this will throw an error which will be redirect to AttachmentException
	// Which will throw a normal exception
	try { vessel->GetAttachmentIndex(attachHandle); return true; }
	catch (...) { return false; }
}

std::vector<VECTOR3> UCSO_API::SetGroundList()
{
	std::vector<VECTOR3> groundList;

	for (DWORD vesselIndex = 0; vesselIndex < oapiGetVesselCount(); vesselIndex++)
	{
		VESSEL* cargo = oapiGetVesselInterface(oapiGetVesselByIndex(vesselIndex));

		if (strncmp(cargo->GetClassNameA(), "UCSO", 4) == 0) 
		{
			VECTOR3 cargoPos;

			// Get the cargo position and convert it to local
			cargo->GetGlobalPos(cargoPos);
			vessel->Global2Local(cargoPos, cargoPos);

			// If the cargo is within the release distance (5 meters) plus the column length
			// And the cargo is lower than or equal to the row length
			if (cargoPos.x <= 11 && cargoPos.x >= 3.5 && cargoPos.z <= rowLength) groundList.push_back(cargoPos);
		}
	}

	return groundList;
}

bool UCSO_API::GetNearestEmptyLocation(std::vector<VECTOR3> groundList, VECTOR3& initialPos)
{
	// Add the release distance
	initialPos.x += 5;

	double length = 0;

	VECTOR3 pos = initialPos;

loop:
	for (VECTOR3 cargoPos : groundList) 
	{
		// Orbiter SDK function length isn't used, as the elevetion (Y-axis) doesn't matter here
		// It'll cause problems
		VECTOR3 subtract = pos - cargoPos;

		// If the distance is lower than 1 meter
		if (sqrt(subtract.x * subtract.x + subtract.z * subtract.z) < 1)
		{
			// Reset the position to the initial position
			pos = initialPos;
			// Add distance between cargo which 1.5 meter
			length += 1.5;
			// If the distance will exceed the column length (which is 6 meters), add new row
			// Integer division here so only add if it exceeds (otherwise it won't increase)
			pos.z += static_cast<int>(length / 6) * 1.5;
			// Don't ask me how I made this, it just works
			pos.x += length - (static_cast<int>(length / 6) * 6.0);
			// Run the loop again, as the new positon could be interfering with a previous cargo
			goto loop;
		}
	}

	// If the availale position is too far
	if (pos.z > rowLength) return false;

	initialPos = pos;

	return true;
}

void UCSO_API::SetSpawnName(std::string& spawnName)
{
	// if there is a vessel with the same name
	if (oapiGetVesselByName(&spawnName[0])) 
	{
		for (int index = 1; index++;)
		{
			// Add the index to the string. c_str() is used to avoid a bug
			std::string name = spawnName.c_str() + std::to_string(index);
			if (!oapiGetVesselByName(&name[0])) { spawnName = name; break; }
		}
	}
}

OBJHANDLE UCSO_API::VerifySlot(int slot)
{
	// Return the attached vessel. It can be used as true/false as it'll be NULL if no vessel is attached
	return vessel->GetAttachmentStatus(attachsMap[slot]);
}

int UCSO_API::GetEmptySlot()
{
	for (auto const& [slot, attachmentHandle] : attachsMap) 
	{ 
		// If the slot is invalid
		if (!CheckAttachment(attachmentHandle)) continue;

		// If no vessel is attached
		if (!VerifySlot(slot)) return slot;
	}

	return -1;
}

std::pair<int, OBJHANDLE> UCSO_API::GetOccupiedSlot()
{
	for (auto const& [slot, attachmentHandle] : attachsMap)
	{ 
		if (!CheckAttachment(attachmentHandle)) continue;

		if (VerifySlot(slot)) return { slot, VerifySlot(slot) };
	}

	return { -1, NULL };
}

CargoInterface* UCSO_API::GetResourceCargo(std::string resourceType)
{
	for (auto const& [slot, attachmentHandle] : attachsMap)
	{
		if (!CheckAttachment(attachmentHandle)) continue;

		OBJHANDLE cargoHandle = VerifySlot(slot);

		if (cargoHandle) 
		{
			CargoInterface* cargo = static_cast<CargoInterface*>(oapiGetVesselInterface(cargoHandle));

			DataStruct dataStruct = cargo->GetDataStruct();

			// If the cargo is a resource, and its resource mass isn't empty, and the resource type is the required type
			// _strcmpi is used to be case insenstice
			if (dataStruct.type == RESOURCE && dataStruct.netMass > 0 &&
				_strcmpi(dataStruct.resourceType.c_str(), resourceType.c_str()) == 0) return cargo;
		}
	}

	return nullptr;
}
