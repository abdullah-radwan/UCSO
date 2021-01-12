// =======================================================================================
// XR2_Platform.cpp : The platform class.
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

#include "XR2_Platform.h"
#include <sstream>

DLLCLBK VESSEL* ovcInit(OBJHANDLE hvessel, int flightmodel) { return new XR2_Platform(hvessel, flightmodel); }

DLLCLBK void ovcExit(VESSEL* vessel) { if (vessel) delete static_cast<XR2_Platform*>(vessel); }

XR2_Platform::XR2_Platform(OBJHANDLE hVessel, int flightmodel) : VESSEL4(hVessel, flightmodel)
{
	ucso = UCSO::Vessel::CreateInstance(this);

	sprintf(buffer, "UCSO version: %s", ucso->GetUCSOVersion());
	message = _strdup(buffer);
}

XR2_Platform::~XR2_Platform() { delete ucso; delete xrSound; }

void XR2_Platform::clbkSetClassCaps(FILEHANDLE cfg)
{
	SetTouchdownPoints(tdVtx, 11);

	oapiReadItem_bool(cfg, "RealismMode", realismMode);

	if (realismMode) ucso->SetMaxTotalCargoMass(10.8e3);

	// First column
	cargoSlots[0] = CreateAttachment(false, { 0.7, -0.6096, 1.6 }, { 0, 1, 0 }, { 0, 0, 1 }, "UCSO1");
	cargoSlots[1] = CreateAttachment(false, { 0.7, -0.6096, 0 }, { 0, 1, 0 }, { 0, 0, 1 }, "UCSO2");
	cargoSlots[2] = CreateAttachment(false, { 0.7, -0.6096, -1.6 }, { 0, 1, 0 }, { 0, 0, 1 }, "UCSO3");
	// Second column
	cargoSlots[3] = CreateAttachment(false, { -0.7, -0.6096, 1.6 }, { 0, 1, 0 }, { 0, 0, 1 }, "UCSO4");
	cargoSlots[4] = CreateAttachment(false, { -0.7, -0.6096, 0 }, { 0, 1, 0 }, { 0, 0, 1 }, "UCSO5");
	cargoSlots[5] = CreateAttachment(false, { -0.7, -0.6096, -1.6 }, { 0, 1, 0 }, { 0, 0, 1 }, "UCSO6");

	for (int slot = 0; slot < 6; slot++) ucso->SetSlotAttachment(slot, cargoSlots[slot]);
}

void XR2_Platform::clbkPostCreation() 
{ 
	xr2AttachHandle = GetAttachmentHandle(true, 0);

	xrSound = XRSound::CreateInstance(this);

	xrSound->LoadWav(SFX_BAY_DOORS_CLOSED, "XRSound\\Default\\Bay Doors Are Closed.wav", XRSound::InternalOnly);
	xrSound->LoadWav(SFX_SLOT_EMPTY, "XRSound\\Default\\Slot is Empty.wav", XRSound::InternalOnly);
	xrSound->LoadWav(SFX_SLOT_OCCUPIED, "XRSound\\Default\\Slot Is Full.wav", XRSound::InternalOnly);
	xrSound->LoadWav(SFX_CARGO_RELEASED, "XRSound\\Default\\Cargo Deployed.wav", XRSound::InternalOnly);
	xrSound->LoadWav(SFX_CARGO_RELEASE_FAILED, "XRSound\\Default\\Cargo Deployment Failed.wav", XRSound::InternalOnly);
	xrSound->LoadWav(SFX_CARGO_GRAPPLED, "XRSound\\Default\\Cargo Latched In Bay.wav", XRSound::InternalOnly);
	xrSound->LoadWav(SFX_CARGO_GRAPPLE_NORANGE, "XRSound\\Default\\No Cargo in Grapple Range.wav", XRSound::InternalOnly);
}

void XR2_Platform::clbkPreStep(double simt, double simdt, double mjd)
{
	if (timer < 5) timer += simdt;

	OBJHANDLE xr2Handle = GetAttachmentStatus(xr2AttachHandle);

	if (!xr2Handle)
	{
		xr2Vessel = nullptr;
		if (GroundContact() && !(GetFlightStatus() & 1)) SetStatusLanded();
	}
	else if (!xr2Vessel || xr2Vessel->GetHandle() != xr2Handle)
	{
		VESSEL* vessel = oapiGetVesselInterface(xr2Handle);
		if (XRVesselCtrl::IsXRVesselCtrl(vessel) && _strcmpi(vessel->GetClassNameA(), "XR2Ravenstar") == 0)
			xr2Vessel = static_cast<XRVesselCtrl*>(vessel);
	}
}

bool XR2_Platform::clbkDrawHUD(int mode, const HUDPAINTSPEC* hps, oapi::Sketchpad* skp)
{
	// Draw the default HUD (Surface, Orbit, etc...)
	VESSEL4::clbkDrawHUD(mode, hps, skp);

	// Determine the screen ratio
	int s = hps->H;
	double d = s * 0.00130208;
	int sw = hps->W;
	int lw = static_cast<int>(16 * sw / 1024);
	int x = 0;
	if (s / sw < 0.7284) x = (lw * 10) + 10;
	int y = static_cast<int>((168 * d) + (-88 * d));

	// Set the color to green
	skp->SetTextColor(0x0066FF66);

	sprintf(buffer, "Selected slot to use: %d", slotIndex + 1);
	skp->Text(x, y, buffer, strlen(buffer));
	y += 20;

	sprintf(buffer, "Selected cargo to add: %s", ucso->GetAvailableCargoName(cargoIndex));
	skp->Text(x, y, buffer, strlen(buffer));
	y += 20;

	sprintf(buffer, "Selected tank to drain fuel to: %s", fuelIndex == 2 ? "SCRAM" : fuelIndex == 1 ? "RCS" : "Main");
	skp->Text(x, y, buffer, strlen(buffer));
	y += 36;

	skp->Text(x, y, "S = Select a slot to use", 24);
	y += 20;

	skp->Text(x, y, "Shift + S = Select a cargo to add", 33);
	y += 20;

	skp->Text(x, y, "F = Select a tank to drain fuel to", 34);
	y += 20;

	skp->Text(x, y, "Shift + A = Add the selected cargo", 34);
	y += 20;

	skp->Text(x, y, "Shift + G = Grapple the nearest cargo", 37);
	y += 20;

	skp->Text(x, y, "Shift + R = Release the grappled cargo", 38);
	y += 20;

	skp->Text(x, y, "Shift + F = Drain the nearest fuel resource", 43);
	y += 20;

	skp->Text(x, y, "Shift + D = Delete the grappled cargo", 37);

	// Display the message if the timer is below 5
	if (timer < 5) { y += 36; skp->Text(x, y, message, strlen(message)); }

	y += 36;
	skp->Text(x, y, "Cargoes information", 19);
	y += 20;

	int cargoesCount = 0;
	double totalMass = 0;

	for (int slot = 0; slot < 6; slot++)
	{
		OBJHANDLE slotCargo = GetAttachmentStatus(cargoSlots[slot]);
		if (!slotCargo) continue;

		cargoesCount++;
		totalMass += oapiGetMass(slotCargo);
	}

	sprintf(buffer, "Cargoes count: %d/6", cargoesCount);
	skp->Text(x, y, buffer, strlen(buffer));
	y += 20;

	sprintf(buffer, "Total cargoes mass: %gkg", totalMass);
	skp->Text(x, y, buffer, strlen(buffer));

	UCSO::Vessel::CargoInfo cargoInfo = ucso->GetCargoInfo(slotIndex);
	if (!cargoInfo.valid) return true;

	y += 36;
	skp->Text(x, y, "Selected slot cargo information", 31);
	y += 40;

	sprintf(buffer, "Name: %s", cargoInfo.name);
	skp->Text(x, y, buffer, strlen(buffer));
	y += 20;

	sprintf(buffer, "Mass: %gkg", cargoInfo.mass);
	skp->Text(x, y, buffer, strlen(buffer));

	switch (cargoInfo.type)
	{
	case UCSO::Vessel::STATIC:
		skp->Text(x, y - 40, "Type: Static", 12);
		break;

	case UCSO::Vessel::RESOURCE:
		skp->Text(x, y - 40, "Type: Resource", 14);
		y += 20;

		sprintf(buffer, "Resource: %s", cargoInfo.resource);
		skp->Text(x, y, buffer, strlen(buffer));
		y += 20;

		sprintf(buffer, "Resource mass: %gkg", cargoInfo.resourceMass);
		skp->Text(x, y, buffer, strlen(buffer));
		break;

	case UCSO::Vessel::UNPACKABLE_ONLY:
		skp->Text(x, y - 40, "Type: Unpackable only", 21);
		y += 20;

		sprintf(buffer, "Unpacked spawn count: %d cargo(es)", cargoInfo.spawnCount);
		skp->Text(x, y, buffer, strlen(buffer));
		y += 20;

	case UCSO::Vessel::PACKABLE_UNPACKABLE:
		if (cargoInfo.type == UCSO::Vessel::PACKABLE_UNPACKABLE)
		{
			skp->Text(x, y - 40, "Type: Packable and unpackable", 29);
			y += 20;
		}

		switch (cargoInfo.unpackingType)
		{
		case UCSO::Vessel::UCSO_RESOURCE:
			skp->Text(x, y, "Unpacking type: UCSO Resource", 29);
			y += 20;
			break;

		case UCSO::Vessel::UCSO_MODULE:
			skp->Text(x, y, "Unpacking type: UCSO Module", 27);
			y += 20;

			sprintf(buffer, "Breathable: %s", cargoInfo.breathable ? "Yes" : "No");
			skp->Text(x, y, buffer, strlen(buffer));
			break;

		case UCSO::Vessel::ORBITER_VESSEL:
			skp->Text(x, y, "Unpacking type: Orbiter vessel", 30);
			y += 20;

			sprintf(buffer, "Spawn module: %s", cargoInfo.spawnModule);
			skp->Text(x, y, buffer, strlen(buffer));
			y += 20;

			switch (cargoInfo.unpackingMode)
			{
			case UCSO::Vessel::LANDING:
				skp->Text(x, y, "Unpacking mode: Landing", 23);
				break;

			case UCSO::Vessel::DELAYING:
				skp->Text(x, y, "Unpacking mode: Delaying", 24);
				y += 20;

				sprintf(buffer, "Unpacking delay: %is", cargoInfo.unpackingDelay);
				skp->Text(x, y, buffer, strlen(buffer));
				break;

			case UCSO::Vessel::MANUAL:
				skp->Text(x, y, "Unpacking mode: Manual", 22);
				break;
			}

			break;
		case UCSO::Vessel::CUSTOM_CARGO:
			skp->Text(x, y, "Unpacking type: Custom cargo", 28);
			y += 20;

			sprintf(buffer, "Breathable: %s", cargoInfo.breathable ? "Yes" : "No");
			skp->Text(x, y, buffer, strlen(buffer));
			break;
		}

		break;
	}

	return true;
}

int XR2_Platform::clbkConsumeBufferedKey(DWORD key, bool down, char* kstate)
{
	if (!down) return 0; // If the key is let go (not pressed)

	if (realismMode)
	{
		if (xr2Vessel) ucso->SetSlotDoor(xr2Vessel->GetDoorState(XRD_PayloadBayDoors) == XRDS_Open);
		else ucso->SetSlotDoor(true);
	}

	if (KEYMOD_SHIFT(kstate)) // If Shift key is pressed
	{
		switch (key)
		{
		case OAPI_KEY_S:
			// Reset the index if reached the cargo count, otherwise increase the index
			cargoIndex + 1 < ucso->GetAvailableCargoCount() ? cargoIndex++ : cargoIndex = 0;
			return 1;

		case OAPI_KEY_A:
			switch (ucso->AddCargo(cargoIndex, slotIndex))
			{
			case UCSO::Vessel::GRAPPLE_SUCCEEDED:
				message = "The selected cargo is added successfully.";
				xrSound->PlayWav(SFX_CARGO_GRAPPLED);
				break;

			case UCSO::Vessel::GRAPPLE_SLOT_OCCUPIED:
				message = "Couldn't add the selected cargo: the selected slot is occupied.";
				xrSound->PlayWav(SFX_SLOT_OCCUPIED);
				break;

			case UCSO::Vessel::GRAPPLE_SLOT_CLOSED:
				message = "Couldn't add the selected cargo: the bay doors are closed.";
				xrSound->PlayWav(SFX_BAY_DOORS_CLOSED);
				break;

			case UCSO::Vessel::MAX_TOTAL_MASS_EXCEEDED:
				message = "Couldn't add the selected cargo: the maximum total cargo mass will be exceeded.";
				break;

			case UCSO::Vessel::NO_CARGO_IN_RANGE:
				message = "Couldn't add the selected cargo: the index is invalid.";
				break;

			case UCSO::Vessel::GRAPPLE_FAILED:
				message = "Couldn't add the selected cargo.";
				break;

			default: break;
			}
			timer = 0;
			return 1;

		case OAPI_KEY_G:
			switch (ucso->GrappleCargo(slotIndex))
			{
			case UCSO::Vessel::GRAPPLE_SUCCEEDED:
				message = "The nearest cargo is grappled successfully.";
				xrSound->PlayWav(SFX_CARGO_GRAPPLED);
				break;

			case UCSO::Vessel::NO_CARGO_IN_RANGE:
				message = "Couldn't grapple cargo: no grappleable cargo in range.";
				xrSound->PlayWav(SFX_CARGO_GRAPPLE_NORANGE);
				break;

			case UCSO::Vessel::GRAPPLE_SLOT_OCCUPIED:
				message = "Couldn't grapple cargo: the selected slot is occupied.";
				xrSound->PlayWav(SFX_SLOT_OCCUPIED);
				break;

			case UCSO::Vessel::GRAPPLE_SLOT_CLOSED:
				message = "Couldn't grapple cargo: the bay doors are closed.";
				xrSound->PlayWav(SFX_BAY_DOORS_CLOSED);
				break;

			case UCSO::Vessel::MAX_TOTAL_MASS_EXCEEDED:
				message = "Couldn't grapple cargo: the maximum total cargo mass will be exceeded.";
				break;

			case UCSO::Vessel::GRAPPLE_FAILED:
				message = "Couldn't grapple cargo.";
				break;

			default: break;
			}
			timer = 0;
			return 1;

		case OAPI_KEY_R:
			switch (ucso->ReleaseCargo(slotIndex))
			{
			case UCSO::Vessel::RELEASE_SUCCEEDED:
				message = "The selected cargo is released successfully.";
				xrSound->PlayWav(SFX_CARGO_RELEASED);
				break;

			case UCSO::Vessel::NO_EMPTY_POSITION:
				message = "Couldn't release the selected cargo: no empty position nearby.";
				break;

			case UCSO::Vessel::RELEASE_SLOT_EMPTY:
				message = "Couldn't release the selected cargo: the selected slot is empty.";
				xrSound->PlayWav(SFX_SLOT_EMPTY);
				break;

			case UCSO::Vessel::RELEASE_SLOT_CLOSED:
				message = "Couldn't release the selected cargo: the bay doors are closed.";
				xrSound->PlayWav(SFX_BAY_DOORS_CLOSED);
				break;

			case UCSO::Vessel::RELEASE_FAILED:
				message = "Couldn't release the selected cargo.";
				xrSound->PlayWav(SFX_CARGO_RELEASE_FAILED);
				break;

			default: break;
			}
			timer = 0;
			return 1;

		case OAPI_KEY_F:
		{
			if (!xr2Vessel)
			{
				message = "Couldn't drain fuel: not attached to an XR-2 vessel.";
				timer = 0;
				return 1;
			}

			PROPELLANT_HANDLE tankHandle = xr2Vessel->GetPropellantHandleByIndex(fuelIndex);

			double requiredMass = xr2Vessel->GetPropellantMaxMass(tankHandle) - xr2Vessel->GetPropellantMass(tankHandle);
			const char* requiredResource = realismMode ? (fuelIndex == 2 ? "ramjet fuel" : "fuel") : "fuel";

			// Drain the required mass to fill the tank, by subtracting the maximum mass from the current mass
			double drainedMass = ucso->DrainCargoResource(requiredResource, requiredMass, slotIndex);

			// If no resource cargo is available, drain from the nearest station or unpacked resource
			if (drainedMass == 0) drainedMass = ucso->DrainStationOrUnpackedResource(requiredResource, requiredMass);

			if (drainedMass > 0)
			{
				xr2Vessel->SetPropellantMass(tankHandle, xr2Vessel->GetPropellantMass(tankHandle) + drainedMass);

				sprintf(buffer, "%g kilograms of fuel was drained", drainedMass);
				message = _strdup(buffer);
			}
			else message = "Couldn't drain fuel.";

			timer = 0;
			return 1;
		}
		case OAPI_KEY_D:
			switch (ucso->DeleteCargo(slotIndex))
			{
			case UCSO::Vessel::RELEASE_SUCCEEDED:
				message = "The selected cargo is deleted successfully.";
				break;

			case UCSO::Vessel::RELEASE_SLOT_EMPTY:
				message = "Couldn't delete the selected cargo: the selected slot is empty.";
				xrSound->PlayWav(SFX_SLOT_EMPTY);
				break;

			case UCSO::Vessel::RELEASE_FAILED:
				message = "Couldn't delete the selected cargo.";
				break;

			default: break;
			}
			timer = 0;
			return 1;

		default: break;
		}
	}

	switch (key)
	{
	case OAPI_KEY_S:
		slotIndex + 1 < 6 ? slotIndex++ : slotIndex = 0;
		return 1;

	case OAPI_KEY_F:
		fuelIndex + 1 < 3 ? fuelIndex++ : fuelIndex = 0;
		return 1;

	default: break;
	}

	return 0;
}

void XR2_Platform::SetStatusLanded()
{
	VESSELSTATUS2 status;
	memset(&status, 0, sizeof(status));
	status.version = 2;
	GetStatusEx(&status);
	status.status = 1;

	ucso->SetGroundRotation(status, 0.81);

	DefSetStateEx(&status);
}