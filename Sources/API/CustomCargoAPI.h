// =======================================================================================
// CustomCargoAPI.h : The internal header of the custom cargo's API.
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
#include "CustomCargo.h"

namespace UCSO
{
	class CustomCargoAPI
	{
	public:
		CustomCargoAPI(CustomCargo* customCargo);
		~CustomCargoAPI();

	private:
		CustomCargo* customCargo;

		typedef void (*CustomCargoFunction)(CustomCargo*);

		HINSTANCE customCargoDll;
		CustomCargoFunction AddCustomCargo = nullptr;
		CustomCargoFunction DeleteCustomCargo = nullptr;
	};
}