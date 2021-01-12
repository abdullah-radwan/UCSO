// =======================================================================================
// CustomCargo.cpp : The custom cargoes' public API class.
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

#include "CustomCargo.h"
#include "CustomCargoAPI.h"
#include "Helper.h"

UCSO::CustomCargo::CustomCargo() { customCargoAPI = new CustomCargoAPI(this); }

void UCSO::CustomCargo::CargoGrappled() { }

void UCSO::CustomCargo::CargoReleased() { }

bool UCSO::CustomCargo::PackCargo() { return false; }

bool UCSO::CustomCargo::UnpackCargo() { return false; }

double UCSO::CustomCargo::DrainResource(double mass) { return 0; }

UCSO::CustomCargo::~CustomCargo() { delete customCargoAPI; }

const char* UCSO::CustomCargo::SetSpawnName(const char* spawnName) 
{ 
	std::string name = spawnName;
	UCSO::SetSpawnName(name);
	return _strdup(name.c_str());
}

void UCSO::CustomCargo::SetGroundRotation(VESSELSTATUS2& status, double spawnHeight) { UCSO::SetGroundRotation(status, spawnHeight); }