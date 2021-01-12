// =======================================================================================
// UCSO_VesselAPI.cpp : The internal class of the vessels' API.
// Copyright © 2020-2021 Abdullah Radwan. All rights reserved.
//
// This file is part of UCSO.
//
// UCSO is free software : you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// UCSO is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with UCSO. If not, see <https://www.gnu.org/licenses/>.
//
// =======================================================================================

#include "VesselAPI.h"
#include <filesystem>

typedef const char* (*GetVersionFunction)();
const char* version = nullptr;

typedef UCSO::CustomCargo* (*CustomCargoFunction)(OBJHANDLE);
HINSTANCE customCargoDll = nullptr;
CustomCargoFunction GetCustomCargo = nullptr;

void ExceptionHandler(unsigned int u, EXCEPTION_POINTERS* pExp) { throw; }

UCSO::Vessel* UCSO::Vessel::CreateInstance(VESSEL* vessel) { return new VesselAPI(vessel); }

VesselAPI::VesselAPI(VESSEL* vessel)
{
	// Set the attachment exception handler
	_set_se_translator(ExceptionHandler);

	this->vessel = vessel;

	// Load cargo DLL
	HINSTANCE ucsoDll = LoadLibraryA("Modules/UCSO/Cargo.dll");

	// If the DLL is loaded
	if (ucsoDll)
	{
		GetVersionFunction GetCargoVersion = reinterpret_cast<GetVersionFunction>(GetProcAddress(ucsoDll, "GetUCSOVersion"));

		// If the function is found, set the version
		if (GetCargoVersion) version = GetCargoVersion();

		FreeLibrary(ucsoDll);
	} 

	if (!version) oapiWriteLog("UCSO API Warning: Couldn't load the cargo API");

	// Load custom cargo DLL
	customCargoDll = LoadLibraryA("Modules/UCSO/CustomCargo.dll");

	if (customCargoDll) GetCustomCargo = reinterpret_cast<CustomCargoFunction>(GetProcAddress(customCargoDll, "GetCustomCargo"));

	// If the DLL isn't loaded or the function couldn't be found
	if (!GetCustomCargo) 
	{
		if (customCargoDll) FreeLibrary(customCargoDll);

		oapiWriteLog("UCSO API Warning: Couldn't load the custom cargo API");
	}

	// Set the available cargo list
	InitAvailableCargo();
}

VesselAPI::~VesselAPI() { if (customCargoDll) FreeLibrary(customCargoDll); }

const char* VesselAPI::GetUCSOVersion() { return version; }

bool VesselAPI::SetSlotAttachment(int slot, ATTACHMENTHANDLE attachmentHandle, bool opened)
{
	// If the passed slot exists and attachment handle is NULL
	if (attachsMap.find(slot) != attachsMap.end() && !attachmentHandle) 
	{
		attachsMap.erase(slot);
		return true;
	}
	// If the attachment handle isn't NULL and the attachment is valid
	else if (attachmentHandle && CheckAttachment(attachmentHandle)) 
	{ 
		attachsMap[slot] = { opened, attachmentHandle };
		return true;
	}

	return false;
}

void VesselAPI::SetSlotDoor(bool opened, int slot)
{
	// Set every slot door status if -1 is passed
	if (slot == -1) for (auto& [slot, data] : attachsMap) data.opened = opened;
	// If the slot exists
	else if (attachsMap.find(slot) != attachsMap.end()) attachsMap[slot].opened = opened;
}

void VesselAPI::SetMaxCargoMass(double maxCargoMass) { this->maxCargoMass = maxCargoMass; }

void VesselAPI::SetMaxTotalCargoMass(double maxTotalCargoMass) { this->maxTotalCargoMass = maxTotalCargoMass; }

void VesselAPI::SetEVAMode(bool evaMode) { this->evaMode = evaMode; }

void VesselAPI::SetGrappleRange(double grappleRange) { this->grappleRange = grappleRange; }

void VesselAPI::SetReleaseVelocity(double releaseVelocity) { this->releaseVelocity = releaseVelocity; }

void VesselAPI::SetCargoRowLength(int rowLength) { this->rowLength = rowLength; }

void VesselAPI::SetUnpackingRange(double unpackingRange) { this->unpackingRange = unpackingRange; }

void VesselAPI::SetResourceRange(double resourceRange) { this->resourceRange = resourceRange; }

void VesselAPI::SetBreathableRange(double breathableRange) { this->breathableRange = breathableRange; }

int VesselAPI::GetAvailableCargoCount() { return availableCargoList.size(); }

const char* VesselAPI::GetAvailableCargoName(int index)
{
	// If the index is invalid (lower than 0 or higher than the list size)
	if (index < 0 || index >= static_cast<int>(availableCargoList.size())) return nullptr;

	return availableCargoList[index].c_str();
}

VesselAPI::CargoInfo VesselAPI::GetCargoInfo(int slot)
{
	// If the slot isn't defined, isn't valid, or empty
	if (attachsMap.empty() || attachsMap.find(slot) == attachsMap.end() || 
		!CheckAttachment(attachsMap[slot].attachHandle) || !VerifySlot(slot)) return CargoInfo();

	// Get the attached cargo
	VESSEL* cargoVessel = oapiGetVesselInterface(vessel->GetAttachmentStatus(attachsMap[slot].attachHandle));

	CargoInfo cargoInfo;

	cargoInfo.valid = true;
	cargoInfo.name = cargoVessel->GetName();
	cargoInfo.mass = cargoVessel->GetMass();

	// If the custom cargo function is loaded
	if (GetCustomCargo)
	{
		UCSO::CustomCargo* customCargo = GetCustomCargo(cargoVessel->GetHandle());

		if (customCargo)
		{
			UCSO::CustomCargo::CargoInfo customInfo = customCargo->GetCargoInfo();

			cargoInfo.type = static_cast<CargoType>(customInfo.type);

			switch (cargoInfo.type)
			{
			case RESOURCE:
				cargoInfo.resource = _strdup(customInfo.resource);
				cargoInfo.resourceMass = customInfo.resourceMass;

				break;
			case UNPACKABLE_ONLY:
				cargoInfo.spawnCount = customInfo.spawnCount;
			case PACKABLE_UNPACKABLE:
				cargoInfo.unpackingType = CUSTOM_CARGO;
				cargoInfo.breathable = customInfo.breathable;
			default:
				break;
			}

			return cargoInfo;
		}
	}

	// Get the cargo interface
	UCSO::Cargo* cargo = static_cast<UCSO::Cargo*>(cargoVessel);

	UCSO::DataStruct dataStruct = cargo->GetDataStruct();

	cargoInfo.type = static_cast<CargoType>(dataStruct.type);

	switch (cargoInfo.type)
	{
	case RESOURCE:
		cargoInfo.resource = _strdup(dataStruct.resource.c_str());
		cargoInfo.resourceMass = dataStruct.netMass;

		break;
	case UNPACKABLE_ONLY:
		cargoInfo.spawnCount = dataStruct.spawnCount;
	case PACKABLE_UNPACKABLE:
		cargoInfo.unpackingType = static_cast<UnpackingType>(dataStruct.unpackingType);
		cargoInfo.breathable = dataStruct.breathable;

		if (cargoInfo.unpackingType == ORBITER_VESSEL) 
		{
			cargoInfo.spawnModule = _strdup(dataStruct.spawnModule.c_str());
			cargoInfo.unpackingMode = static_cast<UnpackingMode>(dataStruct.unpackingMode);

			if (cargoInfo.unpackingMode == DELAYING) cargoInfo.unpackingDelay = dataStruct.unpackingDelay;
		}

		break;
	default:
		break;
	}

	return cargoInfo;
}

double VesselAPI::GetCargoMass(int slot)
{
	if (attachsMap.empty() || attachsMap.find(slot) == attachsMap.end() || !CheckAttachment(attachsMap[slot].attachHandle)) return -1;

	OBJHANDLE cargoHandle = VerifySlot(slot);
	if (!cargoHandle) return -1;

	return oapiGetMass(cargoHandle);
}

double VesselAPI::GetTotalCargoMass()
{
	double totalCargoMass = 0;

	for (auto const& [slot, data] : attachsMap) 
	{
		// If the slot isn't valid
		if (!CheckAttachment(data.attachHandle)) continue;

		OBJHANDLE cargo = VerifySlot(slot);

		// If the slot is occupied
		if (cargo) totalCargoMass += oapiGetMass(cargo);
	}

	return totalCargoMass;
}

VesselAPI::GrappleResult VesselAPI::AddCargo(int index, int slot)
{
	if (index < 0 || index >= static_cast<int>(availableCargoList.size())) return NO_CARGO_IN_RANGE;
	else if (attachsMap.empty()) return GRAPPLE_SLOT_UNDEFINED;
	else if (slot == -1)
	{
		// Get the first empty slot
		EmptyResult result = GetEmptySlot();
		slot = result.slot;
		// If no slot is empty
		if (slot == -1) return result.opened ? GRAPPLE_SLOT_OCCUPIED : GRAPPLE_SLOT_CLOSED;
	}
	else if (attachsMap.find(slot) == attachsMap.end() || !CheckAttachment(attachsMap[slot].attachHandle)) return GRAPPLE_SLOT_UNDEFINED;
	// If the slot is closed
	else if (!attachsMap[slot].opened) return GRAPPLE_SLOT_CLOSED;
	// If a cargo is already attached to the slot
	else if (VerifySlot(slot)) return GRAPPLE_SLOT_OCCUPIED;

	std::string cargoName = availableCargoList[index];

	std::string spawnName = cargoName;
	UCSO::SetSpawnName(spawnName);

	std::string className = "UCSO/";
	className += cargoName;

	VESSELSTATUS2 status;
	memset(&status, 0, sizeof(status));
	status.version = 2;
	vessel->GetStatusEx(&status);

	OBJHANDLE cargoHandle = oapiCreateVesselEx(spawnName.c_str(), className.c_str(), &status);

	// If the maximum cargo mass is set and the cargo mass is higher than it
	if (maxCargoMass != -1) if (oapiGetMass(cargoHandle) > maxCargoMass) 
	{
		oapiDeleteVessel(cargoHandle);

		return MAX_MASS_EXCEEDED;
	}

	// If the maximum total cargo mass is set and the cargo mass plus the total mass is higher than it
	if (maxTotalCargoMass != -1) if (GetTotalCargoMass() + oapiGetMass(cargoHandle) > maxTotalCargoMass)
	{ 
		oapiDeleteVessel(cargoHandle);

		return MAX_TOTAL_MASS_EXCEEDED;
	}

	// If the cargo isn't created
	if (!cargoHandle) return GRAPPLE_FAILED;

	// If the cargo is a custom one
	if (cargoName._Starts_with("CargoCustom"))
	{
		UCSO::CustomCargo* cargo = GetCustomCargo(cargoHandle);

		if (!cargo || !vessel->AttachChild(cargoHandle, attachsMap[slot].attachHandle, cargo->GetCargoAttachmentHandle())) return GRAPPLE_FAILED;
	}
	else
	{
		UCSO::Cargo* cargo = static_cast<UCSO::Cargo*>(oapiGetVesselInterface(cargoHandle));

		// If the cargo couldn't be attached
		if (!vessel->AttachChild(cargo->GetHandle(), attachsMap[slot].attachHandle, cargo->GetAttachmentHandle(true, 0))) return GRAPPLE_FAILED;
	}

	return GRAPPLE_SUCCEEDED;
}

VesselAPI::GrappleResult VesselAPI::GrappleCargo(int slot)
{
	if (attachsMap.empty()) return GRAPPLE_SLOT_UNDEFINED;

	else if (slot == -1) 
	{
		EmptyResult result = GetEmptySlot();
		slot = result.slot;
		if (slot == -1) return result.opened ? GRAPPLE_SLOT_OCCUPIED : GRAPPLE_SLOT_CLOSED;
	}
	else if (attachsMap.find(slot) == attachsMap.end() || !CheckAttachment(attachsMap[slot].attachHandle)) return GRAPPLE_SLOT_UNDEFINED;
	else if (!attachsMap[slot].opened) return GRAPPLE_SLOT_CLOSED;
	else if (VerifySlot(slot)) return GRAPPLE_SLOT_OCCUPIED;

	std::map<double, ResourceResult> cargoMap;
	GrappleResult result = NO_CARGO_IN_RANGE;

	VECTOR3 pos, rot, dir;
	vessel->GetAttachmentParams(attachsMap[slot].attachHandle, pos, rot, dir);

	for (DWORD vesselIndex = 0; vesselIndex < oapiGetVesselCount(); vesselIndex++)
	{
		VESSEL* cargo = oapiGetVesselInterface(oapiGetVesselByIndex(vesselIndex));

		// If the vessel is UCSO cargo
		if (strncmp(cargo->GetClassNameA(), "UCSO", 4) != 0) continue;
		
		VECTOR3 cargoPos;
		// Get the cargo position and convert it to local
		cargo->GetGlobalPos(cargoPos);
		vessel->Global2Local(cargoPos, cargoPos);
		VECTOR3 subtract = pos - cargoPos;

		double range = sqrt(subtract.x * subtract.x + subtract.z * subtract.z) - cargo->GetSize();

		// Proceed if the distance is lower than the grapple range and the cargo radius
		if (range > grappleRange) continue;

		// If the cargo is attached to another vessel
		if (cargo->GetAttachmentStatus(cargo->GetAttachmentHandle(true, 0))) continue;

		// If the maximum cargo mass is set and the cargo mass is higher than it
		if (maxCargoMass != -1) if (cargo->GetMass() > maxCargoMass) { result = MAX_MASS_EXCEEDED; continue; }

		// If the maximum total cargo mass is set and the cargo mass plus the total mass is higher than it
		if (maxTotalCargoMass != -1)
			if (GetTotalCargoMass() + cargo->GetMass() > maxTotalCargoMass) { result = MAX_TOTAL_MASS_EXCEEDED; continue; }

		UCSO::CustomCargo* customCargo = nullptr;

		if (GetCustomCargo) customCargo = GetCustomCargo(cargo->GetHandle());

		if (customCargo)
		{
			UCSO::CustomCargo::CargoInfo cargoInfo = customCargo->GetCargoInfo();

			if (evaMode || cargoInfo.type == STATIC || !cargoInfo.unpacked) cargoMap[range] = { false, customCargo };
		}
		else
		{
			UCSO::Cargo* vCargo = static_cast<UCSO::Cargo*>(cargo);
			UCSO::DataStruct dataStruct = vCargo->GetDataStruct();

			// If grapple unpacked is true, or if it's false, then check if the cargo is packed
			if (evaMode || dataStruct.type == STATIC || !dataStruct.unpacked) cargoMap[range] = { true, cargo };
		}
	}

	// If no cargo is added, return the latest cargo error
	if (cargoMap.empty()) return result;

	for (auto const& [range, data] : cargoMap)
	{
		if (data.normalCargo)
		{
			// If the cargo is attached
			VESSEL* cargo = static_cast<VESSEL*>(data.cargo);

			if (vessel->AttachChild(cargo->GetHandle(), attachsMap[slot].attachHandle, cargo->GetAttachmentHandle(true, 0)))
				return GRAPPLE_SUCCEEDED;	
		}
		else 
		{
			UCSO::CustomCargo* customCargo = static_cast<UCSO::CustomCargo*>(data.cargo);

			if (vessel->AttachChild(customCargo->GetCargoHandle(), attachsMap[slot].attachHandle, customCargo->GetCargoAttachmentHandle()))
			{
				customCargo->CargoGrappled();

				return GRAPPLE_SUCCEEDED;
			}
		}
	}

	return GRAPPLE_FAILED;
}

VesselAPI::ReleaseResult VesselAPI::ReleaseCargo(int slot)
{
	if (attachsMap.empty()) return RELEASE_SLOT_UNDEFINED;

	OccupiedResult result;

	if (slot == -1) 
	{
		// Get the first occupied slot and OBJHANDLE for the attached cargo
		result = GetOccupiedSlot();
		// If the handle is nullptr
		if (!result.handle) return result.opened ? RELEASE_SLOT_EMPTY : RELEASE_SLOT_CLOSED;
		
		slot = result.slot;
	}
	// If the slot doesn't exists or it's invalid
	else if (attachsMap.find(slot) == attachsMap.end() || !CheckAttachment(attachsMap[slot].attachHandle)) return RELEASE_SLOT_UNDEFINED;
	else if (!attachsMap[slot].opened) return RELEASE_SLOT_CLOSED;
	else 
	{
		result.handle = VerifySlot(slot);
		if (!result.handle) return RELEASE_SLOT_EMPTY;
	}

	VESSEL* cargo = oapiGetVesselInterface(result.handle);

	UCSO::CustomCargo* customCargo = nullptr;

	if (GetCustomCargo) customCargo = GetCustomCargo(cargo->GetHandle());

	// If the vessel is landed
	if (vessel->GetFlightStatus() & 1)
	{
		VESSELSTATUS2 status;
		memset(&status, 0, sizeof(status));
		status.version = 2;

		vessel->GetStatusEx(&status);

		VECTOR3 pos, rot, dir;
		// Get the attachment position
		vessel->GetAttachmentParams(attachsMap[slot].attachHandle, pos, rot, dir);

		if (!evaMode && !GetNearestEmptyLocation(pos)) return NO_EMPTY_POSITION;

		// Rotate to the horizon frame
		vessel->HorizonRot(pos, pos);

		// Get meters per degree
		double metersPerDegree = (oapiGetSize(status.rbody) * 2 * PI) / 360;

		status.surf_lng += (pos.x / metersPerDegree) * RAD;
		status.surf_lat += (pos.z / metersPerDegree) * RAD;

		double unpackedHeight = 0.65;

		if (evaMode)
		{
			cargo->GetAttachmentParams(cargo->GetAttachmentHandle(true, 0), pos, rot, dir);

			cargo->HorizonRot(pos, pos);

			status.surf_lng -= (pos.x / metersPerDegree) * RAD;
			status.surf_lat -= (pos.z / metersPerDegree) * RAD;
			status.surf_hdg += PI;

			if (customCargo)
			{
				UCSO::CustomCargo::CargoInfo customInfo = customCargo->GetCargoInfo();

				if ((customInfo.type == UCSO::CustomCargo::PACKABLE_UNPACKABLE || customInfo.type == UCSO::CustomCargo::UNPACKABLE_ONLY)
					&& customInfo.unpacked) unpackedHeight = customInfo.unpackedHeight;
			}
			else
			{
				UCSO::Cargo* vCargo = static_cast<UCSO::Cargo*>(cargo);
				UCSO::DataStruct dataStruct = vCargo->GetDataStruct();

				if (dataStruct.unpacked) unpackedHeight = dataStruct.unpackedHeight;
			}
		}

		SetGroundRotation(status, unpackedHeight);

		// If the cargo is detached, set the status
		if (vessel->DetachChild(attachsMap[slot].attachHandle)) cargo->DefSetStateEx(&status);
		else return RELEASE_FAILED;
	}
	// If released in space, release with the release velocity
	else
	{
		if (evaMode && !vessel->DetachChild(attachsMap[slot].attachHandle)) return RELEASE_FAILED;
		else if (!evaMode && !vessel->DetachChild(attachsMap[slot].attachHandle, releaseVelocity)) return RELEASE_FAILED;
	}

	if (customCargo) customCargo->CargoReleased();

	return RELEASE_SUCCEEDED;
}

bool VesselAPI::PackCargo()
{
	std::map<double, ResourceResult> cargoMap;

	for (DWORD vesselIndex = 0; vesselIndex < oapiGetVesselCount(); vesselIndex++)
	{
		VESSEL* cargo = oapiGetVesselInterface(oapiGetVesselByIndex(vesselIndex));

		// If the vessel isn't a UCSO cargo
		if (strncmp(cargo->GetClassNameA(), "UCSO", 4) != 0) continue;

		VECTOR3 pos;
		vessel->GetRelativePos(cargo->GetHandle(), pos);

		double range = length(pos) - cargo->GetSize();

		if (range > unpackingRange) continue;

		UCSO::CustomCargo* customCargo = nullptr;

		if (GetCustomCargo) customCargo = GetCustomCargo(cargo->GetHandle());

		if (customCargo)
		{
			// If the cargo is attached to another vessel
			if (cargo->GetAttachmentStatus(customCargo->GetCargoAttachmentHandle())) continue;

			UCSO::CustomCargo::CargoInfo customInfo = customCargo->GetCargoInfo();

			if (customInfo.type == UCSO::CustomCargo::PACKABLE_UNPACKABLE && customInfo.unpacked) cargoMap[range] = { false, customCargo };
		}
		else
		{
			if (cargo->GetAttachmentStatus(cargo->GetAttachmentHandle(true, 0))) continue;

			UCSO::Cargo* vCargo = static_cast<UCSO::Cargo*>(cargo);
			UCSO::DataStruct dataStruct = vCargo->GetDataStruct();

			if (dataStruct.type == PACKABLE_UNPACKABLE && dataStruct.unpacked) cargoMap[range] = { true, vCargo };
		}
	}

	if (cargoMap.empty()) return false;

	for (auto const& [range, data] : cargoMap)
	{
		if (data.normalCargo)
		{
			if (static_cast<UCSO::Cargo*>(data.cargo)->PackCargo()) return true;
		}
		else
		{
			if (static_cast<UCSO::CustomCargo*>(data.cargo)->PackCargo()) return true;
		}
	}

	return false;
}

bool VesselAPI::UnpackCargo()
{
	std::map<double, ResourceResult> cargoMap;

	for (DWORD vesselIndex = 0; vesselIndex < oapiGetVesselCount(); vesselIndex++)
	{
		VESSEL* cargo = oapiGetVesselInterface(oapiGetVesselByIndex(vesselIndex));

		// If the vessel is UCSO cargo
		if (strncmp(cargo->GetClassNameA(), "UCSO", 4) != 0) continue;

		VECTOR3 pos;
		vessel->GetRelativePos(cargo->GetHandle(), pos);

		double range = length(pos) - cargo->GetSize();

		if (range > unpackingRange) continue;

		UCSO::CustomCargo* customCargo = nullptr;

		if (GetCustomCargo) customCargo = GetCustomCargo(cargo->GetHandle());

		if (customCargo)
		{
			// If cargo is attached
			if (cargo->GetAttachmentStatus(customCargo->GetCargoAttachmentHandle())) continue;

			UCSO::CustomCargo::CargoInfo customInfo = customCargo->GetCargoInfo();

			if ((customInfo.type == UCSO::CustomCargo::PACKABLE_UNPACKABLE || customInfo.type == UCSO::CustomCargo::UNPACKABLE_ONLY)
				&& !customInfo.unpacked) cargoMap[range] = { false, customCargo };
		}
		else
		{
			if (cargo->GetAttachmentStatus(cargo->GetAttachmentHandle(true, 0))) continue;

			UCSO::Cargo* vCargo = static_cast<UCSO::Cargo*>(cargo);
			UCSO::DataStruct dataStruct = vCargo->GetDataStruct();

			// If the cargo is unpackable and not unpacked
			if ((dataStruct.type == PACKABLE_UNPACKABLE || dataStruct.type == UNPACKABLE_ONLY)
				&& !dataStruct.unpacked) cargoMap[range] = { true, vCargo };
		}
	}

	if (cargoMap.empty()) return false;

	for (auto const& [range, data] : cargoMap)
	{
		if (data.normalCargo)
		{
			UCSO::Cargo* cargo = static_cast<UCSO::Cargo*>(data.cargo);

			if (cargo->UnpackCargo()) return true;
		}
		else
		{
			UCSO::CustomCargo* customCargo = static_cast<UCSO::CustomCargo*>(data.cargo);

			if(customCargo->UnpackCargo()) return true;
		}
	}

	return false;
}

VesselAPI::ReleaseResult VesselAPI::DeleteCargo(int slot)
{
	if (attachsMap.empty()) return RELEASE_SLOT_UNDEFINED;

	OccupiedResult result;

	if (slot == -1)
	{
		// Get the first occupied slot and OBJHANDLE for the attached cargo
		result = GetOccupiedSlot();
		// If the handle is nullptr
		if (!result.handle) return result.opened ? RELEASE_SLOT_EMPTY : RELEASE_SLOT_CLOSED;

		slot = result.slot;
	}
	// If the slot doesn't exists or it's invalid
	else if (attachsMap.find(slot) == attachsMap.end() || !CheckAttachment(attachsMap[slot].attachHandle)) return RELEASE_SLOT_UNDEFINED;
	else if (!attachsMap[slot].opened) return RELEASE_SLOT_CLOSED;
	else
	{
		result.handle = VerifySlot(slot);
		if (!result.handle) return RELEASE_SLOT_EMPTY;
	}

	OBJHANDLE cargoHandle;

	cargoHandle = VerifySlot(slot);
	// If no cargo is attached to the slot
	if (!cargoHandle) return RELEASE_SLOT_EMPTY;

	if (!oapiDeleteVessel(cargoHandle)) return RELEASE_FAILED;

	return RELEASE_SUCCEEDED;
}

double VesselAPI::DrainCargoResource(const char* resource, double mass, int slot)
{
	if (attachsMap.empty() || mass <= 0 || !resource || !*resource) return 0;

	ResourceResult result;

	if (slot == -1) 
	{
		// Get the first resource cargo
		result = GetResourceCargo(resource);
		if (!result.cargo) return 0;
	}
	else if (attachsMap.find(slot) == attachsMap.end() || !CheckAttachment(attachsMap[slot].attachHandle)) return 0;
	else 
	{
		OBJHANDLE cargoHandle = VerifySlot(slot);

		// If no cargo is attached in the given slot
		if (!cargoHandle) return 0;

		UCSO::CustomCargo* customCargo = nullptr;

		if (GetCustomCargo) customCargo = GetCustomCargo(cargoHandle);

		if (customCargo)
		{
			if (customCargo->GetCargoInfo().resource != resource) return 0;
			result = { false, customCargo };
		}

		else
		{ 
			UCSO::Cargo* cargo = static_cast<UCSO::Cargo*>(oapiGetVesselInterface(cargoHandle));

			if (cargo->GetDataStruct().resource != resource) return 0;

			result = { true, cargo };
		}
	}

	if (result.normalCargo) return static_cast<UCSO::Cargo*>(result.cargo)->DrainResource(mass);
	else return static_cast<UCSO::CustomCargo*>(result.cargo)->DrainResource(mass);
}

double VesselAPI::DrainStationOrUnpackedResource(const char* resource, double mass)
{
	if (mass <= 0 || !resource || !*resource) return 0;

	for (DWORD vesselIndex = 0; vesselIndex < oapiGetVesselCount(); vesselIndex++)
	{
		VESSEL* oVessel = oapiGetVesselInterface(oapiGetVesselByIndex(vesselIndex));

		VECTOR3 pos;
		vessel->GetRelativePos(oVessel->GetHandle(), pos);

		if ((length(pos) - oVessel->GetSize()) > resourceRange) continue;

		if (strncmp(oVessel->GetClassNameA(), "UCSO", 4) == 0)
		{
			UCSO::CustomCargo* customCargo = nullptr;

			if (GetCustomCargo) customCargo = GetCustomCargo(oVessel->GetHandle());

			double drainedMass = 0;

			if (customCargo) 
			{
				if(customCargo->GetCargoInfo().resource == resource) drainedMass = customCargo->DrainResource(mass);
			}
			else 
			{
				UCSO::Cargo* cargo = static_cast<UCSO::Cargo*>(oVessel);
				UCSO::DataStruct dataStruct = cargo->GetDataStruct();

				if((dataStruct.type == PACKABLE_UNPACKABLE || dataStruct.type == UNPACKABLE_ONLY) && dataStruct.unpacked
					&& dataStruct.resource == resource) drainedMass = cargo->DrainResource(mass);
			}
			
			if (drainedMass > 0) return drainedMass;

			continue;
		}

		// Search through the vessel attachments to check if it's a station
		for (DWORD attachIndex = 0; attachIndex < oVessel->AttachmentCount(true); attachIndex++)
		{
			if (oVessel->GetAttachmentId(oVessel->GetAttachmentHandle(true, attachIndex)) != "UCSO_ST") continue;

			// Set the vessel configuration file
			std::string configFile = "Vessels/";
			configFile += oVessel->GetClassNameA();
			configFile += ".cfg";

			// Open the file
			FILEHANDLE configHandle = oapiOpenFile(configFile.c_str(), FILE_IN_ZEROONFAIL, CONFIG);

			// If it failed, go to the next vessel.
			if (!configHandle) break;

			char buffer[256];

			// Read the station resources
			if (!oapiReadItem_string(configHandle, "UCSO_Resources", buffer))
			{
				oapiCloseFile(configHandle, FILE_IN_ZEROONFAIL);
				break;
			}

			oapiCloseFile(configHandle, FILE_IN_ZEROONFAIL);

			std::string stationResources = buffer;
			// Add a comma at the end to identify the latest resource by the letter check code
			stationResources.push_back(',');

			std::string stationResource;

			// Check every letter
			for (char& letter : stationResources)
			{
				// If it's a comma, the resource name should be completed
				if (letter == ',')
				{
					// Check if it's the required resource
					if (stationResource == resource) return mass;
					// Clear the resource to begin with a new resource
					stationResource.clear();
				}
				else stationResource += letter;
			}

			break;
		}
	}

	return 0;
}

bool VesselAPI::InBreathableCargo()
{
	double breathableRange = this->breathableRange;

	// Get the nearest cargo in 50 meters
	this->breathableRange = 50;

	VESSEL* cargo = GetNearestBreathableCargo();

	this->breathableRange = breathableRange;

	if (!cargo) return false;

	VECTOR3 pos;
	vessel->GetRelativePos(cargo->GetHandle(), pos);

	// If the distance between the vessel and the cargo is <= the cargo radius (which means the vessel is inside the cargo)
	return length(pos) <= cargo->GetSize();
}

VESSEL* VesselAPI::GetNearestBreathableCargo()
{
	std::pair<double, VESSEL*> pair = { breathableRange, nullptr };

	for (DWORD vesselIndex = 0; vesselIndex < oapiGetVesselCount(); vesselIndex++)
	{
		VESSEL* cargo = oapiGetVesselInterface(oapiGetVesselByIndex(vesselIndex));

		// If the vessel isn't a UCSO cargo
		if (strncmp(cargo->GetClassNameA(), "UCSO", 4) != 0) continue;

		VECTOR3 pos;
		vessel->GetRelativePos(cargo->GetHandle(), pos);

		double distance = length(pos) - cargo->GetSize();

		if (distance > pair.first) continue;

		UCSO::CustomCargo* customCargo = nullptr;

		if (GetCustomCargo) customCargo = GetCustomCargo(cargo->GetHandle());

		if (customCargo)
		{
			UCSO::CustomCargo::CargoInfo customInfo = customCargo->GetCargoInfo();

			if (customInfo.unpacked && customInfo.breathable) pair = { distance, cargo };
		}
		else
		{
			UCSO::Cargo* breathableCargo = static_cast<UCSO::Cargo*>(cargo);
			UCSO::DataStruct dataStruct = breathableCargo->GetDataStruct();

			// If the cargo is unpacked and breathable
			if (dataStruct.unpacked && dataStruct.breathable) pair = { distance, cargo };
		}
	}

	return pair.second;
}

const char* VesselAPI::SetSpawnName(const char* spawnName) 
{
	std::string name = spawnName;
	UCSO::SetSpawnName(name);
	return _strdup(name.c_str());
}

void VesselAPI::SetGroundRotation(VESSELSTATUS2& status, double spawnHeight) { UCSO::SetGroundRotation(status, spawnHeight); }

void VesselAPI::InitAvailableCargo()
{
	// Itereate through every file in Config/Vessels/UCSO
	for (const std::filesystem::directory_entry& entry :
					std::filesystem::directory_iterator(std::filesystem::current_path().string() + "/Config/Vessels/UCSO")) 
	{
		// Get the filename
		std::string path = entry.path().filename().string();

		// If the cargo is a custom one and the custom cargo DLL couldn't be loaded, don't add the cargo
		if (path._Starts_with("CargoCustom") && !GetCustomCargo) continue;

		// Remove .cfg from the filename
		availableCargoList.push_back(path.substr(0, path.find(".cfg")));
	}
}

std::vector<VECTOR3> VesselAPI::GetGroundList(VECTOR3 initialPos)
{
	std::vector<VECTOR3> groundList;

	for (DWORD vesselIndex = 0; vesselIndex < oapiGetVesselCount(); vesselIndex++)
	{
		VESSEL* cargo = oapiGetVesselInterface(oapiGetVesselByIndex(vesselIndex));

		if (!cargo->GroundContact() || strncmp(cargo->GetClassNameA(), "UCSO", 4) != 0) continue;

		VECTOR3 cargoPos;

		// Get the cargo position and convert it to local
		cargo->GetGlobalPos(cargoPos);
		vessel->Global2Local(cargoPos, cargoPos);

		VECTOR3 subtract = cargoPos - initialPos;

		// If the cargo is within the release distance (5 meters) plus the column length
		// And the cargo is lower than or equal to the row length
		if (subtract.x <= 11 && subtract.x >= 3.5 && subtract.z <= rowLength) groundList.push_back(cargoPos);
	}

	return groundList;
}

bool VesselAPI::GetNearestEmptyLocation(VECTOR3& initialPos)
{
	std::vector<VECTOR3> groundList = GetGroundList(initialPos);

	// Add the release distance
	initialPos.x += 5;

	double length = 0;

	VECTOR3 releasePos = initialPos;

loop:
	for (VECTOR3& cargoPos : groundList)
	{
		// Orbiter SDK function length isn't used, as the elevetion (Y-axis) doesn't matter here
		// It'll cause problems
		VECTOR3 subtract = releasePos - cargoPos;

		// Proceed if the distance is lower than 1 meter
		if (sqrt(subtract.x * subtract.x + subtract.z * subtract.z) >= 1.5) continue;

		// Reset the position to the initial position
		releasePos = initialPos;
		// Add distance between cargo which is 1.5 meter
		length += 1.5;
		// If the distance will exceed the column length (which is 6 meters), add new row
		// Integer division here so only add if it exceeds (otherwise it won't increase)
		releasePos.z += static_cast<int>(length / 6) * 1.5;
		// Don't ask me how I made this, it just works
		releasePos.x += length - (static_cast<int>(length / 6) * 6.0);
		// Run the loop again, as the new positon could interfer with a previous cargo
		goto loop;
	}

	// If the availale position is too far
	if (releasePos.z - initialPos.z > rowLength) return false;

	initialPos = releasePos;

	return true;
}

bool VesselAPI::CheckAttachment(ATTACHMENTHANDLE attachHandle)
{
	// If the attachment is invalid, this will throw an error which will be redirect to ExceptionHandler
	// Which will throw a normal exception
	try { vessel->GetAttachmentIndex(attachHandle); return true; }
	catch (...) { return false; }
}

OBJHANDLE VesselAPI::VerifySlot(int slot)
{
	// Return the attached vessel. It can be used as true/false as it'll be NULL if no vessel is attached
	return vessel->GetAttachmentStatus(attachsMap[slot].attachHandle);
}

VesselAPI::EmptyResult VesselAPI::GetEmptySlot()
{
	bool opened = true;

	for (auto const& [slot, data] : attachsMap) 
	{ 
		opened = data.opened;
		// If the slot is invalid
		if (!opened || !CheckAttachment(data.attachHandle)) continue;

		// If no vessel is attached
		if (!VerifySlot(slot)) return { slot, true };
	}

	return { -1, opened };
}

VesselAPI::OccupiedResult VesselAPI::GetOccupiedSlot()
{
	bool opened = true;

	for (auto const& [slot, data] : attachsMap)
	{ 
		opened = data.opened;

		// If the attachment is invalid
		if (!opened || !CheckAttachment(data.attachHandle)) continue;

		OBJHANDLE handle = vessel->GetAttachmentStatus(data.attachHandle);

		// If there is an attached cargo
		if (handle) return { slot, true, handle };
	}

	return { -1, opened, nullptr };
}

VesselAPI::ResourceResult VesselAPI::GetResourceCargo(std::string resource)
{
	for (auto const& [slot, data] : attachsMap)
	{
		if (!CheckAttachment(data.attachHandle)) continue;

		OBJHANDLE cargoHandle = vessel->GetAttachmentStatus(data.attachHandle);

		if (!cargoHandle) continue;

		UCSO::CustomCargo* customCargo = nullptr;

		if (GetCustomCargo) customCargo = GetCustomCargo(cargoHandle);

		if (customCargo)
		{
			UCSO::CustomCargo::CargoInfo cargoInfo = customCargo->GetCargoInfo();

			if (cargoInfo.type == UCSO::CustomCargo::RESOURCE && cargoInfo.resourceMass > 0 &&
				cargoInfo.resource == resource) return { false, customCargo };
		}
		else
		{
			UCSO::Cargo* cargo = static_cast<UCSO::Cargo*>(oapiGetVesselInterface(cargoHandle));

			UCSO::DataStruct dataStruct = cargo->GetDataStruct();

			// If the cargo is a resource, and its resource mass isn't empty, and the resource name is the required type
			if (dataStruct.type == RESOURCE && dataStruct.netMass > 0 && dataStruct.resource == resource) return { true, cargo };
		}
	}

	return { false, nullptr };
}