// =======================================================================================
// XR2_UCSOPlatform.h : XR-2 Ravenstar UCSO cargoes platform.
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
#include <XRSound.h>
#include <XRVesselCtrl.h>
#include <UCSO_Vessel.h>

#define SFX_BAY_DOORS_CLOSED 1
#define SFX_SLOT_EMPTY 2
#define SFX_SLOT_OCCUPIED 3
#define SFX_CARGO_RELEASED 4
#define SFX_CARGO_RELEASE_FAILED 5
#define SFX_CARGO_GRAPPLED 6
#define SFX_CARGO_GRAPPLE_NORANGE 7

class XR2_UCSOPlatform : public VESSEL4
{
public:
	XR2_UCSOPlatform(OBJHANDLE hVessel, int flightmodel);
	~XR2_UCSOPlatform();

	void clbkSetClassCaps(FILEHANDLE cfg);
	void clbkPostCreation();

	void clbkPreStep(double simt, double simdt, double mjd);
	bool clbkDrawHUD(int mode, const HUDPAINTSPEC* hps, oapi::Sketchpad* skp);

	int clbkConsumeBufferedKey(DWORD key, bool down, char* kstate);
	
private:
	UCSO::Vessel* ucso;
	XRSound* xrSound;

	ATTACHMENTHANDLE xr2AttachHandle = nullptr;
	XRVesselCtrl* xr2Vessel = nullptr;
	ATTACHMENTHANDLE cargoSlots[6];
	bool realismMode = true;

	int cargoIndex = 0;  // For the cargo selection on the HUD
	int slotIndex = 0;   // For the slot selection on the HUD
	int fuelIndex = 0;   // 0: Main, 1: RCS, 2: SCRAM
	char buffer[256];    // To draw on the HUD
	const char* message; // To show messages on the HUD
	double timer = 0;    // To show messages for 5 seconds on the HUD

	void SetStatusLanded();
};

static TOUCHDOWNVTX tdVtx[11] =
{
	{ { 0, -0.8096, 2.5 }, 2e4, 1e3, 3.0 },
	{ { -1.5, -0.8096, -2.5 }, 2e4, 1e3, 3.0 },
	{ { 1.5, -0.8096, -2.5 }, 2e4, 1e3, 3.0 },

	{ { -1.5, -0.6096, -2.5 }, 2e4, 1e3, 3.0 },
	{ { 1.5, -0.6096, -2.5 }, 2e4, 1e3, 3.0 },

	{ { -1.47323, 0.69297, -2.47454 }, 2e4, 1e3, 3.0 },
	{ { 1.47323, 0.69297, -2.47454 }, 2e4, 1e3, 3.0 },

	{ { -1.5, -0.6096, 2.5 }, 2e4, 1e3, 3.0 },
	{ { 1.5, -0.6096, 2.5 }, 2e4, 1e3, 3.0 },

	{ { -1.47323, 0.69297, 2.47454 }, 2e4, 1e3, 3.0 },
	{ { 1.47323, 0.69297, 2.47454 }, 2e4, 1e3, 3.0 }
};