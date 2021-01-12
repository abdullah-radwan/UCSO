// =======================================================================================
// CustomCargo.h : Defines the Universal Cargo System for Orbiter (UCSO) 1.1 custom cargoes' public API.
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
#include <Orbitersdk.h>

namespace UCSO
{
	class CustomCargoAPI; // UCSO internal custom cargo class

	class CustomCargo
	{
	public:
		enum CargoType
		{
			STATIC = 0,
			RESOURCE,
			PACKABLE_UNPACKABLE,
			UNPACKABLE_ONLY
		};

		// The cargo information as returned from GetCargoInfo method.
		typedef struct
		{
			CargoType type;           // The cargo type as the CargoType enum.

			// Set only if the cargo type is RESOURCE.
			const char* resource;     // The resource name.
			double resourceMass;      // The resource mass in kilograms.

			// Set only if the cargo type is PACKABLE_UNPACKABLE or UNPACKABLE_ONLY.
			bool unpacked;            // True if the cargo is unpacked, false if not.
			int spawnCount;           // The count of cargoes that will be spawned when unpacking the cargo. Set only if the cargo type is UNPACKABLE_ONLY.
									  // Note: this is informational only. You have to spawn the cargoes on the UnpackCargo method.
			bool breathable;          // True if the cargo can be breathed in when unpacked, false if not.
			double unpackedHeight;    // The unpacked cargo height if released unpacked on the ground.
		} CargoInfo;

		CustomCargo();

		// Cargo information method.
		// It should return the CargoInfo struct filled with suitable information.
		virtual CargoInfo GetCargoInfo() = 0;

		// Cargo vessel object handle method.
		// It should return the cargo's object handle, which can be got from GetHandle() method.
		virtual OBJHANDLE GetCargoHandle() = 0;

		// Cargo attachment handle method.
		// It should return the cargo attachment handle, which is used when the cargo is grappled.
		virtual ATTACHMENTHANDLE GetCargoAttachmentHandle() = 0;

		// Optional: Cargo grapple callback method.
		// It'll be called when the cargo is grappled successfully.
		virtual void CargoGrappled();

		// Optional: Cargo release callback method.
		// It'll be called when the cargo is released successfully.
		virtual void CargoReleased();

		// Cargo packing command method.
		// It'll be called when the cargo should be packed. Implement your cargo packing code here.
		// It'll be called only if the cargo type is PACKABLE_UNPACKABLE, as set by GetCargoInfo method.
		// It should return true if the cargo is packed successfully, false if not.
		virtual bool PackCargo();

		// Cargo unpacking command method.
		// It'll be called when the cargo should be unpacked. Implement your cargo unpacking code here.
		// It'll be called only if the cargo type is PACKABLE_UNPACKABLE or UNPACKABLE_ONLY, as set by GetCargoInfo method.
		// It should return true if the cargo is unpacked successfully, false if not.
		virtual bool UnpackCargo();

		// Cargo resource drainage method. Implement the resource drain code here.
		// It'll be called only if the cargo type is RESOURCE, as set by GetCargoInfo method.
		// Parameters:
		//	mass: the resource needed mass by the requester vessel.
		// It should return the drained mass, which should be <= mass.
		virtual double DrainResource(double mass);

		// Helper methods.

		// This method will set a spawn name to the cargo, which is useful for unpacking a cargo with multiple items.
		// The method will add numbers starting from 1 (e.g. Cargo1, Cargo2, Cargo3, etc.) until no vessel with the name is found.
		// Parameters:
		//	spawnName: the vessel initial spawn name.
		// Returns the spawn name with a number as detailed above.
		virtual const char* SetSpawnName(const char* spawnName);

		// This method will set the rotation (arot and vrot) to set the vessel status to landed.
		// After setting and filling the vessel status, pass it to this method. The method will set the rotation.
		// Then set the status to 1 to force the aircraft landed.
		// This useful for unpacking cargoes on the ground, as SetTouchdownPoints might cause an upset if set on the ground.
		// Parameters:
		//	status: the vessel status. Must be set and filled (by calling GetStatusEx).
		//	spawnHeight: the vessel height above the ground. See the spawn height in the manual.
		virtual void SetGroundRotation(VESSELSTATUS2& status, double spawnHeight);

		virtual ~CustomCargo();

	private:
		CustomCargoAPI* customCargoAPI;
	};
}