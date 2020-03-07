// =======================================================================================
// CustomCargo.cpp : The custom cargo DLL class.
// Copyright © 2020 Abdullah Radwan. All rights reserved.
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

std::vector<UCSO::CustomCargo*> customCargoes;

void AddCustomCargo(UCSO::CustomCargo* cargo) { customCargoes.push_back(cargo); }

void DeleteCustomCargo(UCSO::CustomCargo* cargo)
{
	// Find the cargo
	std::vector<UCSO::CustomCargo*>::iterator it = find(customCargoes.begin(), customCargoes.end(), cargo);
	// If found, delete it
	if (it != customCargoes.end()) customCargoes.erase(it);
}

UCSO::CustomCargo* GetCustomCargo(OBJHANDLE handle)
{
	for (auto const& cargo : customCargoes) if (cargo->GetCargoHandle() == handle) return cargo;
	return nullptr;
};