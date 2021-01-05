// =======================================================================================
// Cargo.cpp : The cargo vessel's class.
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

#include "Cargo.h"
#include <sstream>

bool UCSO::Cargo::configLoaded = false;
double UCSO::Cargo::containerMass = 85;
bool UCSO::Cargo::enableFocus = false;
bool UCSO::Cargo::drainUnpackedResources = false;

DLLCLBK VESSEL* ovcInit(OBJHANDLE hvessel, int flightmodel) { return new UCSO::Cargo(hvessel, flightmodel); }

DLLCLBK void ovcExit(VESSEL* vessel) { if (vessel) delete static_cast<UCSO::Cargo*>(vessel); }

UCSO::Cargo::Cargo(OBJHANDLE hObj, int fmodel) : VESSEL4(hObj, fmodel) { if(!configLoaded) LoadConfig(); }

void UCSO::Cargo::LoadConfig()
{
	configLoaded = true;

	// Open the config file
	FILEHANDLE configFile = oapiOpenFile("UCSO_Config.cfg", FILE_IN_ZEROONFAIL, CONFIG);

	// If the file is opened
	if (configFile)
	{
		if (!oapiReadItem_float(configFile, "ContainerMass", containerMass)) 
			oapiWriteLog("UCSO Warning: Couldn't read the container mass setting, will use the default mass");

		if (!oapiReadItem_bool(configFile, "EnableFocus", enableFocus))
			oapiWriteLog("UCSO Warning: Couldn't read the focus enabling setting, will use the default setting");

		if (!oapiReadItem_bool(configFile, "DrainUnpackedResources", drainUnpackedResources))
			oapiWriteLog("UCSO Warning: Couldn't read the unpacked resources drainage setting, will use the default setting");

		oapiCloseFile(configFile, FILE_IN_ZEROONFAIL);
	}
	else oapiWriteLog("UCSO Warning: Couldn't load the configurations file, will use the default configurations");
}

void UCSO::Cargo::clbkSetClassCaps(FILEHANDLE cfg)
{
	std::string mesh;

	if (!oapiReadItem_string(cfg, "PackedMesh", &mesh[0])) ThrowWarning("mesh");
	packedMesh += mesh.c_str();

	if (!oapiReadItem_float(cfg, "CargoMass", dataStruct.netMass)) ThrowWarning("mass");

	if (!oapiReadItem_int(cfg, "CargoType", dataStruct.type)) ThrowWarning("type");

	switch (dataStruct.type)
	{
	case RESOURCE:
		if (!oapiReadItem_string(cfg, "CargoResource", &dataStruct.resource[0])) ThrowWarning("resource");
		dataStruct.resource = dataStruct.resource.c_str();

		CreatePropellantResource(dataStruct.netMass);

		break;
	case UNPACKABLE_ONLY:
		oapiReadItem_int(cfg, "SpawnCount", dataStruct.spawnCount);
	case PACKABLE_UNPACKABLE:
		if (!oapiReadItem_int(cfg, "UnpackingType", dataStruct.unpackingType)) ThrowWarning("unpacking type");

		switch (dataStruct.unpackingType)
		{
		case UCSO_RESOURCE:
			if (!oapiReadItem_string(cfg, "CargoResource", &dataStruct.resource[0])) ThrowWarning("resource");
			dataStruct.resource = dataStruct.resource.c_str();

			oapiReadItem_float(cfg, "ResourceContainerMass", resourceContainerMass);

			CreatePropellantResource(dataStruct.netMass);
		case UCSO_MODULE:
			if (!oapiReadItem_string(cfg, "UnpackedMesh", &mesh[0])) ThrowWarning("unpacked mesh");
			unpackedMesh += mesh.c_str();

			if (!oapiReadItem_float(cfg, "UnpackedSize", unpackedSize)) ThrowWarning("unpacked size");

			if (oapiReadItem_float(cfg, "UnpackedHeight", dataStruct.unpackedHeight)) dataStruct.unpackedHeight = abs(dataStruct.unpackedHeight);
			else ThrowWarning("unpacked height");

			oapiReadItem_vec(cfg, "UnpackedAttachPos", unpackedAttachPos);

			oapiReadItem_vec(cfg, "UnpackedPMI", unpackedPMI);

			oapiReadItem_vec(cfg, "UnpackedCS", unpackedCS);

			oapiReadItem_bool(cfg, "Breathable", dataStruct.breathable);

			break;
		case ORBITER_VESSEL:
			if (!oapiReadItem_string(cfg, "SpawnName", &dataStruct.spawnName[0])) ThrowWarning("spawn name");
			dataStruct.spawnName = dataStruct.spawnName.c_str();

			if (!oapiReadItem_string(cfg, "SpawnModule", &dataStruct.spawnModule[0])) ThrowWarning("spawn module");
			dataStruct.spawnModule = dataStruct.spawnModule.c_str();

			if (!oapiReadItem_int(cfg, "UnpackingMode", dataStruct.unpackingMode)) ThrowWarning("unpacking mode");

			if (dataStruct.unpackingMode == DELAYING)
			{
				if (!oapiReadItem_int(cfg, "UnpackingDelay", dataStruct.unpackingDelay)) ThrowWarning("unpacking delay");

				if (oapiReadItem_float(cfg, "SpawnHeight", dataStruct.unpackedHeight))  dataStruct.unpackedHeight = abs(dataStruct.unpackedHeight);
				else ThrowWarning("spawn height");
			}

			break;
		}

		break;
	default:
		break;
	}

	SetEnableFocus(enableFocus);

	SetPackedCaps(false);
}

void UCSO::Cargo::ThrowWarning(const char* warning)
{
	oapiWriteLogV("UCSO Fatal Error: The %s of %s cargo isn't specified", warning, GetClassNameA());

	throw;
}

void UCSO::Cargo::clbkLoadStateEx(FILEHANDLE scn, void* status)
{
	char* line;

	while (oapiReadScenario_nextline(scn, line))
	{
		std::istringstream ss;
		ss.str(line);
		std::string data;

		if (ss >> data) 
		{
			switch (dataStruct.type)
			{
			case UNPACKABLE_ONLY:
			case PACKABLE_UNPACKABLE:
				switch (dataStruct.unpackingType)
				{
				case UCSO_RESOURCE:
				case UCSO_MODULE:
					if (data == "Unpacked") ss >> dataStruct.unpacked;
					else ParseScenarioLineEx(line, status);

					if (dataStruct.unpacked) SetUnpackedCaps(false);

					break;
				case ORBITER_VESSEL:
					if (data == "Landing") ss >> landing;
					else if (data == "Timing") ss >> timing;
					else if (data == "Timer") ss >> timer;
					else ParseScenarioLineEx(line, status);

					break;
				}

				break;
			default:
				ParseScenarioLineEx(line, status);

				break;
			}
		}
		else ParseScenarioLineEx(line, status);
	}
}

void UCSO::Cargo::clbkPreStep(double simt, double simdt, double mjd)
{
	// If not landed but contacted the ground
	if (GroundContact() && !(GetFlightStatus() & 1))
	{
		VESSELSTATUS2 status;
		memset(&status, 0, sizeof(status));
		status.version = 2;
		GetStatusEx(&status);

		status.status = 1;
		SetGroundRotation(status, dataStruct.unpacked ? dataStruct.unpackedHeight : 0.65);

		DefSetStateEx(&status);
	}

	// Don't continue if the cargo is not unpackable or not Orbiter vessel
	if (!(dataStruct.type == PACKABLE_UNPACKABLE || dataStruct.type == UNPACKABLE_ONLY) || dataStruct.unpackingType != ORBITER_VESSEL) return;

	bool attached = GetAttachmentStatus(attachmentHandle);

	bool released = this->attached && !attached;

	this->attached = attached;

	// Cancel the landing and timing if attached
	if (attached)
	{
		if (landing) landing = false;
		if (timing) { timer = 0; timing = false; }
	}
	else if (released)
	{
		if (dataStruct.unpackingMode == DELAYING) timing = true;
		else if (dataStruct.unpackingMode == LANDING) landing = true;
	}

	// If landing flag is on and contacted the ground
	if (landing && GroundContact())
	{
		UnpackCargo();
		landing = false;
	}
	else if (timing) 
	{
		timer += simdt;
		if (timer >= dataStruct.unpackingDelay) 
		{
			UnpackCargo();
			timer = 0;
			timing = false;
		}
	}
}

UCSO::DataStruct UCSO::Cargo::GetDataStruct()
{
	if (dataStruct.type == RESOURCE ||
		((dataStruct.type == PACKABLE_UNPACKABLE || dataStruct.type == UNPACKABLE_ONLY) && dataStruct.unpackingType == UCSO_RESOURCE))
		dataStruct.netMass = GetFuelMass();

	return dataStruct;
}

bool UCSO::Cargo::PackCargo()
{
	dataStruct.unpacked = false;

	SetPackedCaps();

	return true;
}

bool UCSO::Cargo::UnpackCargo(bool once)
{
	if (dataStruct.unpackingType != ORBITER_VESSEL)
	{
		dataStruct.unpacked = true;

		SetUnpackedCaps();

		if (once || dataStruct.type != UNPACKABLE_ONLY) return true;

		VESSELSTATUS2 status;
		memset(&status, 0, sizeof(status));
		status.version = 2;
		GetStatusEx(&status);

		for (int cargo = 1; cargo < dataStruct.spawnCount; cargo++)
		{
			std::string spawnName = GetClassNameA();
			spawnName.erase(0, 5);
			SetSpawnName(spawnName);

			OBJHANDLE cargoHandle = oapiCreateVesselEx(spawnName.c_str(), GetClassNameA(), &status);

			if (!cargoHandle) return false;

			if(!(static_cast<UCSO::Cargo*>(oapiGetVesselInterface(cargoHandle))->UnpackCargo(true))) return false;
		}

		return true;
	}

	VESSELSTATUS2 status;
	memset(&status, 0, sizeof(status));
	status.version = 2;
	GetStatusEx(&status);

	// Set the ground rotation if landed
	if (status.status) SetGroundRotation(status, dataStruct.unpackedHeight);

	OBJHANDLE cargoHandle = nullptr;

	for (int cargo = 0; cargo < dataStruct.spawnCount; cargo++)
	{
		std::string spawnName = dataStruct.spawnName;
		SetSpawnName(spawnName);
	
		cargoHandle = oapiCreateVesselEx(spawnName.c_str(), dataStruct.spawnModule.c_str(), &status);

		if (!cargoHandle) return false;

		if (once) break;
	}

	// Delete the cargo and move the camera to the unpacked one
	oapiDeleteVessel(GetHandle(), cargoHandle);

	return true;
}

double UCSO::Cargo::DrainResource(double mass)
{
	double fuelMass = GetFuelMass();

	if (fuelMass == 0) return 0;

	if (!drainUnpackedResources && (dataStruct.type == PACKABLE_UNPACKABLE || dataStruct.type == UNPACKABLE_ONLY) && !dataStruct.unpacked)
		return 0;

	double drainedMass;
	// If the cargo net mass is lower than or equal to the required mass
	if (fuelMass - mass >= 0) drainedMass = mass;
	// If the required mass is higher than the available mass, use the full mass
	else drainedMass = fuelMass;

	SetFuelMass(fuelMass - drainedMass);

	return drainedMass;
}

void UCSO::Cargo::SetPackedCaps(bool init)
{
	// Don't proceed if unpacked
	if (dataStruct.unpacked) return;

	VESSELSTATUS2 status;

	// If Orbiter is initiated, which means the cargo was unpacked
	if (init)
	{
		memset(&status, 0, sizeof(status));
		status.version = 2;
		GetStatusEx(&status);

		ClearAttachments();
	}

	// Replace the unpacked mesh with the packed mesh
	InsertMesh(packedMesh.c_str(), 0);

	if (dataStruct.type == RESOURCE) SetEmptyMass(containerMass); 
	else if ((dataStruct.type == PACKABLE_UNPACKABLE || dataStruct.type == UNPACKABLE_ONLY) && dataStruct.unpackingType == UCSO_RESOURCE)
		SetEmptyMass(containerMass + resourceContainerMass);
	else SetEmptyMass((dataStruct.netMass *  dataStruct.spawnCount) + containerMass);

	SetSize(0.65);

	attachmentHandle = CreateAttachment(true, { 0, -0.65, 0 }, { 0, -1, 0 }, { 0, 0, 1 }, "UCSO");

	SetPMI({ 0.28, 0.28, 0.28 });

	SetCrossSections({ 1.69, 1.69, 1.69 });

	double stiffness = GetMass() * G * 1000;
	double damping = 0.9 * (2 * sqrt(GetMass() * stiffness));

	// Values are pre-set for 1.3m size
	TOUCHDOWNVTX tdvtx[4] =
	{
	{{ 1.3, -0.65, -0.012 }, stiffness, damping, 3, 3},
	{{ 0, -0.65, 0.65 }, stiffness, damping, 3, 3},
	{{ -1.3, -0.65, -0.012 }, stiffness, damping, 3, 3},
	{{ 0, 19.5, 0 }, stiffness, damping, 3, 3}
	};

	SetTouchdownPoints(tdvtx, 4);

	// If the cargo was unpacked and it is landed
	if (init && status.status)
	{
		SetGroundRotation(status, 0.65);

		DefSetStateEx(&status);
	}
}

void UCSO::Cargo::SetUnpackedCaps(bool init)
{
	VESSELSTATUS2 status;

	if (init)
	{
		memset(&status, 0, sizeof(status));
		status.version = 2;
		GetStatusEx(&status);
	}

	ClearAttachments();
	attachmentHandle = CreateAttachment(true, unpackedAttachPos, { 0, 1, 0 }, { 0, 0, 1 }, "UCSO");

	InsertMesh(unpackedMesh.c_str(), 0);

	SetSize(unpackedSize);

	if (dataStruct.unpackingType == UCSO_RESOURCE) SetEmptyMass(resourceContainerMass);
	else SetEmptyMass(dataStruct.netMass);

	// If the unpacked PMI is set
	if (unpackedPMI.x != -99 && unpackedPMI.y != -99 && unpackedPMI.z != -99) SetPMI(unpackedPMI);

	// If the unpacked cross sections is set
	if (unpackedCS.x != -99 && unpackedCS.y != -99 && unpackedCS.z != -99) SetCrossSections(unpackedCS);

	double stiffness = GetMass() * G * 1000;
	double damping = 0.9 * (2 * sqrt(GetMass() * stiffness));

	double sizeSin = -sin(30 * RAD) * unpackedSize;
	double sizeCos = cos(30 * RAD) * unpackedSize;

	TOUCHDOWNVTX tdvtx[4] = 
	{
	{ { sizeCos, -dataStruct.unpackedHeight, sizeSin}, stiffness, damping, 3, 3},
	{ { 0, -dataStruct.unpackedHeight, unpackedSize }, stiffness, damping, 3, 3},
	{ { -sizeCos, -dataStruct.unpackedHeight, sizeSin }, stiffness, damping, 3, 3},
	{ { 0, 15 * unpackedSize, 0 }, stiffness, damping, 3, 3}
	};

	SetTouchdownPoints(tdvtx, 4);

	if (init && status.status)
	{
		SetGroundRotation(status, dataStruct.unpackedHeight);

		DefSetStateEx(&status);
	}
}

void UCSO::Cargo::clbkSaveState(FILEHANDLE scn)
{
	// Set the default state
	VESSEL4::clbkSaveState(scn);

	switch (dataStruct.type)
	{
	case UNPACKABLE_ONLY:
	case PACKABLE_UNPACKABLE:
		switch (dataStruct.unpackingType)
		{
		case UCSO_RESOURCE:
		case UCSO_MODULE:
			oapiWriteScenario_int(scn, "Unpacked", dataStruct.unpacked);

			break;
		case ORBITER_VESSEL:
			oapiWriteScenario_int(scn, "Landing", landing);
			oapiWriteScenario_int(scn, "Timing", timing);
			oapiWriteScenario_float(scn, "Timer", timer);

			break;
		}

		break;
	default:
		break;
	}
}

