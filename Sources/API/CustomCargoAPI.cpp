// =======================================================================================
// CustomCargoAPI.cpp : The internal class of the custom cargoes' API.
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

#include "CustomCargoAPI.h"

UCSO::CustomCargoAPI::CustomCargoAPI(CustomCargo* customCargo)
{
	this->customCargo = customCargo;

	// Load the custom cargo DLL
	customCargoDll = LoadLibraryA("Modules/UCSO/CustomCargo.dll");

	if (customCargoDll)
	{
		AddCustomCargo = reinterpret_cast<CustomCargoFunction>(GetProcAddress(customCargoDll, "AddCustomCargo"));

		DeleteCustomCargo = reinterpret_cast<CustomCargoFunction>(GetProcAddress(customCargoDll, "DeleteCustomCargo"));
	}

	// If the DLL is loaded and both functions are found
	if (AddCustomCargo && DeleteCustomCargo) AddCustomCargo(customCargo);
	else
	{
		if (customCargoDll) FreeLibrary(customCargoDll);
		oapiWriteLog("UCSO Fatal Error: Couldn't load the custom cargo API");
		// Kill Orbiter
		throw;
	}
}

UCSO::CustomCargoAPI::~CustomCargoAPI() { DeleteCustomCargo(customCargo); FreeLibrary(customCargoDll); }