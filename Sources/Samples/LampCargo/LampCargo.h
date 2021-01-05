// =======================================================================================
// LampCargo.h : A UCSO custom cargo example. It's a cargo which emits light when unpacked.
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

#pragma once
#include <UCSO_CustomCargo.h>

class LampCargo : public VESSEL4, public UCSO::CustomCargo
{
public:
	LampCargo(OBJHANDLE hVessel, int flightmodel);

	void clbkSetClassCaps(FILEHANDLE cfg) override;
	void clbkLoadStateEx(FILEHANDLE scn, void* status) override;
	void clbkPreStep(double simt, double simdt, double mjd) override;
	void clbkSaveState(FILEHANDLE scn) override;

	CargoInfo GetCargoInfo() override;
	OBJHANDLE GetCargoHandle() override;
	ATTACHMENTHANDLE GetCargoAttachmentHandle() override;

	bool UnpackCargo() override;
	bool UnpackCargo(bool once);

private:
	// This struct is used to hold the beacon information, as AddBeacon method takes a pointer.
	// The pointer must be valid during the vessel life. If the variable is local, it will be invalid as soon as the method ends.
	// A class variable will be valid for the vessel life.
	// The beacon is added to have the 'lamp light' effect, but spotlight is actually lighting.
	struct
	{
		VECTOR3 pos = { 0, 2.55, 0.65 };
		VECTOR3 color = { 1,1,1 };
		BEACONLIGHTSPEC beaconSpec = { BEACONSHAPE_DIFFUSE, &pos, &color, 2, 0.2, 0, 0, 0, false };
	} beaconStruct;

	// It is not necessary to use a struct for the spotlight, but to keep things organized.
	// This struct is static because AddSpotLight method will make a copy of it, so it can be the same for all instances.
	// In the beacon struct, AddBeacon method takes a pointer so every instance must have its own data to enable and disable the beacon.
	static const struct SpotStruct
	{
		const VECTOR3 pos = { 0, 2.55, 0.5 };
		// Tilt the light 20 degrees down
		const double tilt = -20 * RAD;
		const VECTOR3 dir = { 0, sin(tilt), cos(tilt) };
		const double range = 10;
		const double att0 = 0.001;
		const double att1 = 0;
		const double att2 = 0.005;
		const double umbra = 45 * RAD;
		const double penumbra = PI05;
		const COLOUR4 diffuse = { 1,1,1,0 };
		const COLOUR4 specular = { 1,1,1,0 };
		const COLOUR4 ambient = { 0,0,0,0 };
	} spotStruct;

	static const double unpackedSize;

	SpotLight* spotLight = nullptr;

	UCSO::CustomCargo::CargoInfo cargoInfo;
	ATTACHMENTHANDLE attachmentHandle = nullptr;

	void SetPackedCaps();
	void SetUnpackedCaps(bool init = true);
};