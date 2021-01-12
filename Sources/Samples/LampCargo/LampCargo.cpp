// =======================================================================================
// LampCargo.cpp : The lamp custom cargo class.
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

#include "LampCargo.h"
#include <sstream>

DLLCLBK VESSEL* ovcInit(OBJHANDLE hvessel, int flightmodel) { return new LampCargo(hvessel, flightmodel); }

DLLCLBK void ovcExit(VESSEL* vessel) { if (vessel) delete static_cast<LampCargo*>(vessel); }

LampCargo::LampCargo(OBJHANDLE hVessel, int flightmodel) : VESSEL4(hVessel, flightmodel)
{
	// Set cargo information
	cargoInfo.type = UCSO::CustomCargo::UNPACKABLE_ONLY;
	cargoInfo.unpacked = false;
	cargoInfo.breathable = false;
	cargoInfo.spawnCount = 5;
	cargoInfo.unpackedHeight = 2.9;
}

void LampCargo::clbkSetClassCaps(FILEHANDLE cfg)
{
	SetEnableFocus(false);

	AddBeacon(&beaconStruct.beaconSpec);

	spotLight = static_cast<SpotLight*>(AddSpotLight(spotStruct.pos, spotStruct.dir, spotStruct.range,
		spotStruct.att0, spotStruct.att1, spotStruct.att2, spotStruct.umbra, spotStruct.penumbra,
		spotStruct.diffuse, spotStruct.specular, spotStruct.ambient));

	// Set the cargo properties. 
	// It is set here (not in clbkLoadStateEx) because that method won't be called if the vessel is spawned in the simulator.
	// So if it is spawned in the simulator, set its default properties which is a packed cargo.
	SetPackedCaps();
}

void LampCargo::clbkLoadStateEx(FILEHANDLE scn, void* status)
{
	char* line;

	while (oapiReadScenario_nextline(scn, line))
	{
		std::istringstream ss;
		ss.str(line);
		std::string data;

		if (ss >> data)
		{
			if (data == "Unpacked") ss >> cargoInfo.unpacked;
			else ParseScenarioLineEx(line, status);

			if (cargoInfo.unpacked) SetUnpackedCaps(false);
		}
		else ParseScenarioLineEx(line, status);
	}
}

void LampCargo::clbkPreStep(double simt, double simdt, double mjd)
{
	// If not landed but contacted the ground
	if (GroundContact() && !(GetFlightStatus() & 1))
	{
		VESSELSTATUS2 status;
		memset(&status, 0, sizeof(status));
		status.version = 2;
		GetStatusEx(&status);

		status.status = 1;
		SetGroundRotation(status, cargoInfo.unpacked ? cargoInfo.unpackedHeight : 0.65);

		DefSetStateEx(&status);
	}
}

void LampCargo::clbkSaveState(FILEHANDLE scn)
{
	VESSEL4::clbkSaveState(scn);

	oapiWriteScenario_int(scn, "Unpacked", cargoInfo.unpacked);
}

UCSO::CustomCargo::CargoInfo LampCargo::GetCargoInfo() { return cargoInfo; }

OBJHANDLE LampCargo::GetCargoHandle() { return GetHandle(); }

ATTACHMENTHANDLE LampCargo::GetCargoAttachmentHandle() { return attachmentHandle; }

bool LampCargo::UnpackCargo() { return UnpackCargo(false); }

bool LampCargo::UnpackCargo(bool once)
{
	cargoInfo.unpacked = true;

	SetUnpackedCaps();

	if (once) return true;

	VESSELSTATUS2 status;
	memset(&status, 0, sizeof(status));
	status.version = 2;
	GetStatusEx(&status);

	for (int cargo = 1; cargo < cargoInfo.spawnCount; cargo++)
	{
		std::string spawnName = "CargoCustomLamp";
		spawnName = SetSpawnName(spawnName.c_str());

		OBJHANDLE cargoHandle = oapiCreateVesselEx(spawnName.c_str(), GetClassNameA(), &status);

		if (!cargoHandle) return false;

		if (!(static_cast<LampCargo*>(oapiGetVesselInterface(cargoHandle))->UnpackCargo(true))) return false;
	}

	return true;
}

void LampCargo::SetPackedCaps()
{
	// Don't proceed if unpacked
	if (cargoInfo.unpacked) return;

	// Disable the spotlight
	spotLight->Activate(false);

	// Replace the unpacked mesh with the packed mesh
	InsertMesh("UCSO/Container3", 0);

	SetEmptyMass(325);

	SetSize(0.65);

	// Create the cargo packed attachment
	attachmentHandle = CreateAttachment(true, { 0, -0.65, 0 }, { 0, -1, 0 }, { 0, 0, 1 }, "UCSO");

	SetPMI({ 0.28, 0.28, 0.28 });

	SetCrossSections({ 1.69, 1.69, 1.69 });

	double stiffness = GetMass() * G * 1000;
	double damping = 0.9 * (2 * sqrt(GetMass() * stiffness));

	// Values are pre-set for 1.3m size and 0.65 height
	TOUCHDOWNVTX tdvtx[4] =
	{
	{{ 1.3, -0.65, -0.012 }, stiffness, damping, 3, 3},
	{{ 0, -0.65, 0.65 }, stiffness, damping, 3, 3},
	{{ -1.3, -0.65, -0.012 }, stiffness, damping, 3, 3},
	{{ 0, 19.5, 0 }, stiffness, damping, 3, 3}
	};

	SetTouchdownPoints(tdvtx, 4);
}

void LampCargo::SetUnpackedCaps(bool init)
{
	VESSELSTATUS2 status;

	if (init)
	{
		memset(&status, 0, sizeof(status));
		status.version = 2;
		GetStatusEx(&status);
	}

	// Enable the beacon and spotlight
	beaconStruct.beaconSpec.active = true;
	spotLight->Activate(true);
		
	InsertMesh("UCSO/Lamp", 0);

	SetSize(UNPACKED_SIZE);

	SetEmptyMass(50);

	ClearAttachments();
	attachmentHandle = CreateAttachment(true, { 0, -cargoInfo.unpackedHeight, -0.2 }, { 0, 1, 0 }, { 0, 0, 1 }, "UCSO");

	SetPMI({ 1.44, 0.2, 1.56 });

	SetCrossSections({ 0.58, 0.24, 0.6 });

	double stiffness = GetMass() * G * 1000;
	double damping = 0.9 * (2 * sqrt(GetMass() * stiffness));

	double sizeSin = -sin(30 * RAD) * UNPACKED_SIZE;
	double sizeCos = cos(30 * RAD) * UNPACKED_SIZE;

	TOUCHDOWNVTX tdvtx[4] =
	{
	{ { sizeCos, -cargoInfo.unpackedHeight, sizeSin}, stiffness, damping, 3, 3},
	{ { 0, -cargoInfo.unpackedHeight, UNPACKED_SIZE }, stiffness, damping, 3, 3},
	{ { -sizeCos, -cargoInfo.unpackedHeight, sizeSin }, stiffness, damping, 3, 3},
	{ { 0, 15 * UNPACKED_SIZE, 0 }, stiffness, damping, 3, 3}
	};

	SetTouchdownPoints(tdvtx, 4);

	if (init && status.status)
	{
		SetGroundRotation(status, cargoInfo.unpackedHeight);

		DefSetStateEx(&status);
	}
}