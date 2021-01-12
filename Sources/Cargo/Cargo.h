// =======================================================================================
// Cargo.h : The cargo vessel's header.
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
#include "..\API\Helper.h"

DLLCLBK const char* GetUCSOVersion() { return _strdup("1.1"); }

namespace UCSO
{
	class Cargo : public VESSEL4
	{
	public:
		Cargo(OBJHANDLE hObj, int fmodel);

		void clbkSetClassCaps(FILEHANDLE cfg) override;
		void clbkLoadStateEx(FILEHANDLE scn, void* status) override;
		void clbkPreStep(double simt, double simdt, double mjd) override;
		void clbkSaveState(FILEHANDLE scn) override;

		virtual DataStruct GetDataStruct();
		virtual bool PackCargo();
		virtual bool UnpackCargo(bool once = false);
		virtual double DrainResource(double mass);

	private:
		enum CargoType
		{
			STATIC = 0,
			RESOURCE,
			PACKABLE_UNPACKABLE,
			UNPACKABLE_ONLY,
			CUSTOM
		};

		enum UnpackingType
		{
			UCSO_RESOURCE = 0,
			UCSO_MODULE,
			ORBITER_VESSEL
		};

		enum UnpackingMode
		{
			LANDING = 0,
			DELAYING,
			MANUAL
		};

		DataStruct dataStruct;

		std::string packedMesh = "UCSO/";
		std::string unpackedMesh = "UCSO/";

		double resourceContainerMass = 0;
		double unpackedSize;

		VECTOR3 unpackedAttachPos = { 0,0,0 };
		VECTOR3 unpackedPMI = { -99,-99,-99 };
		VECTOR3 unpackedCS = { -99,-99,-99 };

		double timer = 0;
		bool landing = false;
		bool timing = false;

		ATTACHMENTHANDLE attachmentHandle = nullptr;
		bool attached = false;

		void SetPackedCaps(bool init = true);
		void SetUnpackedCaps(bool init = true);

		static void LoadConfig();
		void ThrowWarning(const char* warning);
	};
}

bool configLoaded = false;
double containerMass = 85;
bool enableFocus = false;
bool drainUnpackedResources = false;