// =======================================================================================
// UCSO_Vessel.h : Defines the Universal Cargo System for Orbiter (UCSO) vessel's 1.0 public API.
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

#pragma once
#include <Orbitersdk.h>

namespace UCSO
{
	class Vessel
	{
	public:
		// The grapple result as returned from GrappleCargo and AddCargo methods.
		enum GrappleResult
		{
			GRAPPLE_SUCCEEDED = 0,     // The cargo is grappled or added successfully.
			NO_CARGO_IN_RANGE,         // No cargo in the grapple range for GrappleCargo, or the passed index is invalid for AddCargo.
			MAX_MASS_EXCEEDED,         // The maximum one cargo mass will be exceeded if the cargo is grappled or added.
			MAX_TOTAL_MASS_EXCEEDED,   // The maximum total cargo mass will be exceeded if the cargo is grappled or added.
			GRAPPLE_SLOT_OCCUPIED,     // The passed slot (or all slots) is occupied.
			GRAPPLE_SLOT_CLOSED,	   // The passed slot (or all slots) door is closed.
			GRAPPLE_SLOT_UNDEFINED,    // The passed slot (or all slots) is undefiend or invalid.
			GRAPPLE_FAILED             // The grapple or addition failed.
		};

		// The release result as returned from ReleaseCargo, UnpackCargo, and DeleteCargo methods.
		enum ReleaseResult
		{
			RELEASE_SUCCEEDED = 0,   // The cargo is released, unpacked, or deleted successfully.
			NO_EMPTY_POSITION,       // There is no empty position near the vessel for release on the ground for ReleaseCargo,
									 // Or no unpackable cargo in the unpacking range for UnpackCargo.
									 // Or no cargo is found in the deletion range for DeleteCargo.
		    RELEASE_SLOT_EMPTY,      // The passed slot is empty, or all slots are empty if -1 is passed. Not for UnpackCargo.
			RELEASE_SLOT_CLOSED,     // The passed slot (or all slots) door is closed. Not for UnpackCargo.
			RELEASE_SLOT_UNDEFINED,  // The passed slot (or all slots) is undefiend or invalid. Not for UnpackCargo.
			RELEASE_FAILED           // The release, unpacking, or deletion failed.
		};

		// Cargo type as returned from GetCargoInfo method.
		enum CargoType
		{
			STATIC = 0,
			RESOURCE,
			PACKABLE_UNPACKABLE,
			UNPACKABLE_ONLY
		};

		// Unpacking type as returned from GetCargoInfo method.
		enum UnpackingType
		{
			UCSO_RESOURCE = 0,
			UCSO_MODULE,
			ORBITER_VESSEL,
			CUSTOM_CARGO
		};

		// Unpacking mode as returned from GetCargoInfo method.
		enum UnpackingMode
		{
			LANDING = 0,
			DELAYING,
			MANUAL
		};

		// The cargo information as returned from GetCargoInfo method.
		typedef struct
		{
			bool valid;                  // True if the struct contains information, false if not.

			const char* name;            // The cargo name in the scenario.
			double mass;                 // The cargo mass in kilograms.
			CargoType type;              // The cargo type as the CargoType enum.

			// Defined only if the cargo type is RESOURCE or (PACKABLE_UNPACKABLE or UNPACKABLE_ONLY and the unpacking type is UCSO_RESOURCE).
			const char* resource;        // The resource name.
			double resourceMass;         // The available resource mass in kilograms.

			// Defined only if the cargo type is PACKABLE_UNPACKABLE or UNPACKABLE_ONLY.
			UnpackingType unpackingType; // The unpacking type as the UnpackingType enum.
			int spawnCount;              // The count of cargoes that will be spawned when unpacking the cargo. Defined only if the cargo type is UNPACKABLE_ONLY.
			bool breathable;             // True if the cargo can be breathed in when unpacked, false if not. Defined only if the unpacking type is UCSO_MODULE or CUSTOM_CARGO.

			// Defined only if the unpacking type is ORBITER_VESSEL.
			const char* spawnModule;     // The spawn module. 
			UnpackingMode unpackingMode; // The unpacking mode as the UnpackingMode enum.
			int unpackingDelay;          // The unpacking delay in seconds.
		} CargoInfo;

		// Performs one-time initialization of UCSO vessel API. It can be called from your vessel's constructor.
		// Parameters:
		//	vessel: pointer to the calling vessel.
		// Returns a UCSO vessel API instance.
		// NOTE: Don't forget to delete the returned object when you no longer need it (e.g. in your vessel's destructor).
		static UCSO::Vessel* CreateInstance(VESSEL* vessel);

		// Returns the UCSO version, or nullptr if UCSO isn't installed.
		virtual const char* GetUCSOVersion() = 0;

		// Sets the slot number for a given attachment, to use with the API methods. It must be called before any other calls to the API.
		// To delete a slot, pass the slot number and nullptr.
		// Parameters:
		//	slot: the slot number, which can be any number except -1. You can use the number multiple times to update the slot attachment handle.
		//	attachmentHandle: the attachment handle. If nullptr, the passed slot will be removed.
		//	opened: true if the slot door is opened, false if not.
		// Returns true if the slot is added or removed, or false if the attachment handle is invalid.
		// NOTE: DO NOT ATTACH ANY VESSEL TO A SLOT ATTACHMENT HANDLE OUTSIDE UCSO API.
		//	IF THE ATTACHED VESSEL ISN'T A NORMAL OR CUSTOM UCSO CARGO, ORBITER WILL CRASH.
		virtual bool SetSlotAttachment(int slot, ATTACHMENTHANDLE attachmentHandle, bool opened = true) = 0;

		// Sets the slot door status.
		// Parameters:
		//	opened: the door status. True if opened, false if closed.
		//	slot: the slot number. If -1 is passed all slots door status will be changed.
		virtual void SetSlotDoor(bool opened, int slot = -1) = 0;

		// Sets the maximum one cargo mass for the vessel for GrappleCargo and AddCargo methods.
		// If any cargo mass exceeded this mass, the cargo won't be grappled or added.
		// Parameters:
		//	maxCargoMass: the maximum mass in kilograms. Set -1 for unlimited mass. The default value is -1.
		virtual void SetMaxCargoMass(double maxCargoMass) = 0;

		// Sets the maximum total cargo mass for the vessel for GrappleCargo and AddCargo methods.
		// If the total cargo mass will exceed this mass if the cargo is grappled or added, the cargo won't be grappled or added.
		// Parameters:
		//	maxTotalCargoMass: the maximum mass in kilograms. Set -1 for unlimited mass. The default value is -1.
		virtual void SetMaxTotalCargoMass(double maxTotalCargoMass) = 0;

		// Sets the extravehicular activity mode.
		// In this mode, unpacked cargoes can be grappled, and cargoes will be released without velocity in space,
		// And directly in the vessel current position on the ground. 
		// Normally, unpacked cargoes can't be grappled, cargoes are released with the set release velocity in space,
		// And away from the vessel in columns and rows on the ground.
		// Parameters:
		//	evaMode: true to enable the EVA mode, false to disable. The default value is false.
		virtual void SetEVAMode(bool evaMode) = 0;

		// Sets the grapple range for GrappleCargo method.
		// Parameters:
		//	grappleRange: the grapple range in meters. The default value is 50 meters.
		virtual void SetGrappleRange(double grappleRange) = 0;

		// Sets the cargo release velocity for space release for ReleaseCargo method.
		// Parameters:
		//	releaseVelocity: the release velocity in m/s. The default value is 0.05 m/s.
		virtual void SetReleaseVelocity(double releaseVelocity) = 0;

		// Sets the cargo row length for ground release for ReleaseCargo method.
		// Parameters:
		//	rowLength: the cargo row length. The default value is 4 cargoes.
		// The column length and the distance between cargoes are constant and cannot be changed due to accuracy problems.
		virtual void SetCargoRowLength(int rowLength) = 0;

		// Sets the unpacking range for PackCargo and UnpackCargo methods.
		// Parameters:
		//	unpackingRange: the unpacking range in meters. The default value is 5 meters.
		virtual void SetUnpackingRange(double unpackingRange) = 0;

		// Set the cargo deletion range for DeleteCargo method.
		// Parameters:
		//	cargoRange: the deletion range in meters. The default value is 100 meter.
		virtual void SetCargoDeletionRange(double deletionRange) = 0;

		// Sets the resource drainage search range for DrainStationOrUnpackedResource method.
		// Parameters:
		//	resourceRange: the search range in meters. The default value is 100 meter.
		virtual void SetResourceRange(double resourceRange) = 0;

		// Sets the nearest breathable cargo search range for GetNearestBreathableCargo method.
		// Parameters:
		//	breathableRange: the search range in meters. The default value is 1000 meter.
		virtual void SetBreathableRange(double breathableRange) = 0;

		// Returns the available cargo count, which is the number of cargoes in Config\Vessels\UCSO folder.
		virtual int GetAvailableCargoCount() = 0;

		// Returns the cargo name from the passed index, which is the filename from Config\Vessels\UCSO folder without .cfg,
		// or nullptr if the index is invalid.
		// Parameters:
		//	index: the cargo index. It must be >= 0 and lower than the available cargo count.
		virtual const char* GetAvailableCargoName(int index) = 0;

		// Returns cargo information as the CargoInfo struct, or an empty struct if the passed slot is invalid.
		// Parameters:
		//	slot: the slot number.
		virtual CargoInfo GetCargoInfo(int slot) = 0;

		// Gets the cargo mass in the passed slot.
		// Parameters:
		//	slot: the slot number.
		// Returns the cargo mass in kilograms, or -1 if the slot is empty, undefined, or its attachment handle is invalid.
		virtual double GetCargoMass(int slot) = 0;

		// Gets the total cargo mass.
		// Returns the total cargo mass in kilograms.
		virtual double GetTotalCargoMass() = 0;

		// Adds the passed cargo to the passed slot.
		// Parameters:
		//	index: the cargo index. It must be >= 0 and lower than the available cargo count.
		//  slot: the slot number. If -1 is passed, the first empty slot will be used.
		// Returns the result as the GrappleResult enum.
		virtual GrappleResult AddCargo(int index, int slot = -1) = 0;

		// Grapples the nearest cargo to the passed slot in the range set by SetGrappleRange method.
		// Unpacked cargoes won't be grappled by default. You can change this with SetUnpackedGrapple method.
		// Parameters:
		//	slot: the slot number. If -1 is passed, the first empty slot will be used.
		// Returns the result as the GrappleResult enum. 
		virtual GrappleResult GrappleCargo(int slot = -1) = 0;

		// Releases the cargo in the passed slot.
		// The release velocity for space release can be set with SetReleaseVelocity method.
		// The cargo row length for ground release can be set with SetCargoRowLength method.
		// Parameters:
		//	slot: the slot number. If -1 is passed, the first occupied slot will be used.
		// Returns the result as the ReleaseResult enum.
		virtual ReleaseResult ReleaseCargo(int slot = -1) = 0;

		// Packs the nearest cargo in the unpacking range which can be set with SetUnpackingRange method.
		// Returns true if the cargo is packed, or false if the packing failed or no packable cargo in the unpacking range is found.
		virtual bool PackCargo() = 0;

		// Unpacks the nearest cargo in the unpacking range which can be set with SetUnpackingRange method.
		// Returns true if the unpacking succeeded, or false if the unpacking failed or no cargo in the unpacking range is found.
		virtual bool UnpackCargo() = 0;

		// Deletes the cargo in the passed slot, or the nearest cargo if -1 is passed.
		// Parameters:
		//	slot: the slot number. If -1 is passed, the nearest cargo in the range set by SetCargoDeletionRange will be deleted.
		// Returns the result as the ReleaseResult enum.
		virtual ReleaseResult DeleteCargo(int slot = -1) = 0;

		// Drains the available resource from the cargo in the passed slot.
		// Parameters:
		//	resource: the resource name, must be lowercase (see the standard resource names in the manual).
		//	mass: the needed mass in kilograms.
		//	slot: the slot number. If -1 is passed, the first available cargo will be used.
		// Returns the required mass or less, based on the cargo available resource mass, or 0 if:
		//	The passed slot (or all slots) cargo isn't a resource.
		//	The passed slot (or all slots) cargo resource name doesn't match the passed type.
		//	The passed slot (or all slots) cargo is empty,.
		//	The passed slot (or all slots) is empty, undefined, or its attachment handle is invalid.
		virtual double DrainCargoResource(const char* resource, double mass, int slot = -1) = 0;

		// Drains the resource from the nearest station or unpacked resource cargo. 
		// The search range can be set with SetResourceSearchRange method.
		// Parameters:
		//	resource: the resource name, must be lowercase (see the standard resource names in the manual).
		//	mass: the needed mass in kilograms.
		// Returns the required mass, or 0 if no station or cargo with the passed resource name in the search range is found.
		virtual double DrainStationOrUnpackedResource(const char* resource, double mass) = 0;

		// Returns true if the vessel is in a breathable cargo, false if not.
		virtual bool InBreathableCargo() = 0;

		// Gets the nearest breathable cargo. The search range can be set with SetBreathableSearchRange method.
		// Returns the nearest breathable cargo, or nullptr if no cargo is found.
		virtual VESSEL* GetNearestBreathableCargo() = 0;

		// Helper methods.

		// This method will set a spawn name to the cargo, which is useful for unpacking a cargo with multiple items.
		// If there is no vessel with the passed spawn name, the method will return it.
		// Otherwise, the method will add numbers starting from 2 (e.g. Cargo2, Cargo3, Cargo4, etc.).
		// Parameters:
		//	spawnName: the vessel initial spawn name.
		virtual const char* SetSpawnName(const char* spawnName) = 0;

		// This method will set the rotation (arot and vrot) to set the vessel status to landed.
		// After setting and filling the vessel status, pass it to this method. The method will set the rotation.
		// Then set the status to 1 to force the aircraft landed.
		// This useful for unpacking cargoes on the ground, as SetTouchdownPoints might cause an upset if set on the ground.
		// Parameters:
		//	status: the vessel status. Must be set and filled (by calling GetStatusEx).
		//	spawnHeight: the vessel height above the ground. See the spawn height in the manual.
		virtual void SetGroundRotation(VESSELSTATUS2& status, double spawnHeight) = 0;

		virtual ~Vessel() { }

	protected:
		// The constructor has a protected access to prevent incorrect instantiation by the vessel code.
		// Always use the static CreateInstance method to create an instance of UCSO.
		Vessel() { };
	};
}