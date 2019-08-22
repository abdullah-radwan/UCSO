// =======================================================================================
// UCSO_API.h : Defines the Universal Cargo System for Orbiter (UCSO) Alpha 5 public API.
// 
// Copyright © 2019 Abdullah Radwan
// All rights reserved.
//
// =======================================================================================

#pragma once
#include <Orbitersdk.h>
#include <map>
#include <vector>
#include "../CargoVessel.h"

class UCSO
{
public:
	// The addition result as returned from AddCargo method.
	enum AddResult {
		CARGO_ADDED = 8, // The cargo is added successfully.
		INVALID_INDEX,   // The passed index is invalid.
		ADD_FAILED       // The addition failed.
	};

	// The grapple result as returned from GrappleCargo and AddCargo method.
	enum GrappleResult {
		CARGO_GRAPPLED = 0,      // The cargo is grappled successfully.
		NO_CARGO_IN_RANGE,       // No cargo in the grapple range.
		MAX_MASS_EXCEEDED,       // The maximum one cargo mass will be exceeded if the cargo is added.
		MAX_TOTAL_MASS_EXCEEDED, // The maximum total cargo mass will be exceeded if the cargo is added.
		SLOT_OCCUPIED,           // The passed slot is occupied, or all slots are occupied.
		SLOT_UNDEFINED,          // The passed slot is undefiend, or no slots are defined.
		GRAPPLE_FAILED           // The grapple failed.
	};

	// The release result as returned from ReleaseCargo, UnpackCargo, and DeleteCargo methods.
	enum ReleaseResult
	{
		CARGO_RELEASED = 0, // The cargo is released successfully. For ReleaseCargo method only.
		SLOT_EMPTY,         // The passed slot is empty, or all slots are empty if -1 is passed.
		SLOT_UNDEF,         // The passed slot is undefined, or no slots are defined.
		NO_EMPTY_POSITION,  // There is no empty position near the vessel for release on the ground. For ReleaseCargo method only.
		RELEASE_FAILED      // The release failed. For ReleaseCargo method only.
	};

	// The unpack result as returned from UnpackCargo method.
	enum UnpackResult
	{
		CARGO_UNPACKED = 4,     // The cargo is unpacked successfully.
		NOT_UNPACKABLE,         // The cargo is unpackable.
		NO_UNPACKABLE_IN_RANGE, // No unpackable cargo in range
		UNPACK_FAILED           // The unpack failed.
	};

	// The delete result as returned from DeleteCargo method.
	enum DeleteResult
	{
		CARGO_DELETED = 4, // The cargo is deleted successfully.
		DELETE_FAILED      // The delete failed.
	};

	// Cargo type as returned from GetCargoInfo method.
	enum CargoType {
		STATIC = 0,
		RESOURCE,
		UNPACKABLE
	};

	// Unpack mode as returned from GetCargoInfo method.
	enum UnpackMode {
		LANDED = 0,
		DELAYED,
		MANUAL
	};

	// The cargo information as returned from GetCargoInfo method.
	struct CargoInfo
	{
		bool valid;               // True if the struct contains information, false if not.
		CargoType type;           // The cargo type as the CargoType enum.
		double mass;              // The cargo mass in kilograms.

		const char* resourceType; // The resource type
		double resourceMass;      // The resource mass in kilograms: the cargo mass minus the container mass.

		const char* spawnModule;  // The spawn module
		UnpackMode unpackMode;    // The unpack mode as the UnpackMode enum.
		int unpackDelay;          // The unpack delay in seconds.
	};

	// Performs one-time initialization of UCSO. It can be called from your vessel's constructor.
	// Parameters:
	//	vessel: pointer to the calling vessel.
	// Returns a UCSO instance.
	// NOTE: Don't forget to delete the returned object when you no longer need it (e.g., in your vessel's destructor).
	static UCSO* CreateInstance(VESSEL* vessel);

	// Sets the slot number for a given attachment, to use with the API methods. It must be called before any other calls to the API.
	// To delete a slot, pass the slot number and NULL for the attachment handle.
	// Parameters:
	//	slot: the slot number. You can use any number but not -1, as it's reserved.
	//		You can use the number multiple times to update the slot attachment handle.
	//	attachmentHandle: the attachment handle. If NULL, the passed slot will be removed.
	void SetSlotAttachment(int slot, ATTACHMENTHANDLE attachmentHandle);

	// Sets the maximum one cargo mass for the vessel.
	// If any cargo mass exceeded this mass, the cargo won't be added.
	// Parameters:
	//	maxCargoMass: the maximum mass in kilograms. Set -1 for unlimited mass. The default value is -1.
	void SetMaxCargoMass(double maxCargoMass);

	// Sets the maximum total cargo mass for the vessel.
	// If the total cargo mass will exceed this mass if the cargo is added, the cargo won't be added.
	// Parameters:
	//	maxTotalCargoMass: the maximum mass in kilograms. Set -1 for unlimited mass. The default value is -1.
	void SetMaxTotalCargoMass(double maxTotalCargoMass);

	// Sets the maximum grapple distance in meters.
	// Parameters:
	//	grappleDistance: the distance in meters. The default value is 50 meters.
	void SetMaxGrappleDistance(double grappleDistance);

	// Sets the cargo release velocity if released in space.
	// Parameters:
	//	releaseVelocity: the release velocity in m/s. The default value is 0.05 m/s.
	void SetReleaseVelocity(double releaseVelocity);

	// Sets the cargo release distance if released on the ground.
	// Parameters:
	//	releaseDistance: the release distance in meters. The default value is 5 meters.
	void SetReleaseDistance(double releaseDistance);

	// Sets the cargo column length if released on the ground.
	// Parameters:
	//	columnLength: the cargo column length. The default value is 4 cargo.
	void SetCargoColumnLength(int columnLength);

	// Sets the cargo row length if released on the ground.
	// Parameters:
	//	rowLength: the cargo row length. The default value is 4 cargo.
	void SetCargoRowLength(int rowLength);

	// Sets the maximum unpack distance in meters.
	// Parameters:
	//	unpackDistance: the distance in meters. The default value is 3 meters.
	void SetMaxUnpackDistance(double unpackDistance);

	// Returns the available cargo count, which is the number of cargoes in Config\Vessels\UCSO folder.
	int GetAvailableCargoCount();

	// Returns the cargo name from the passed index, which is the filename from Config\Vessels\UCSO folder without .cfg,
	// or empty string if the index is invalid.
	// Parameters:
	//	index: the cargo index. It must equal or be higher than 0 and lower than the available cargo count.
	const char* GetAvailableCargoName(int index);

	// Returns the cargo information as the CargoInfo struct, or an empty struct if the passed slot is invalid.
	// Parameters:
	//	slot: the slot number.
	CargoInfo GetCargoInfo(int slot);

	// Gets the cargo mass in the passed slot.
	// Parameters:
	//	slot: the slot number.
	// Returns the cargo mass in kilograms, or -1 if the slot is empty or undefined.
	double GetCargoMass(int slot);

	// Gets the total cargo mass.
	// Returns the total cargo mass in kilograms.
	double GetTotalCargoMass();

	// Adds the passed cargo to the passed slot.
	// Parameters:
	//	index: the cargo index. It must equal or be higher than 0 and lower than the available cargo count.
	//  slot: the slot number. If no slot is passed, the first empty slot will be used.
	// Returns the result as GrappleResult and AddResult enums.
	int AddCargo(int index, int slot = -1);

	// Grapples the cargo in the passed slot.
	// Parameters:
	//	slot: the slot number. If no slot is passed, the first empty slot will be used.
	// Returns the result as the GrappleResult enum.
	GrappleResult GrappleCargo(int slot = -1);

	// Releases the cargo in the passed slot.
	// Parameters:
	//	slot: the slot number. If no slot is passed, the first occupied slot will be used.
	// Returns the result as the ReleaseResult enum.
	ReleaseResult ReleaseCargo(int slot = -1);

	// Unpacks the cargo in the passed slot, or the nearest cargo if no slot is passed.
	// Parameters:
	//	slot: the slot number. If no slot is passed, the first occupied slot will be used.
	//  isAttached: if the cargo must be attached to unpack it. If true, SetUnpackDistance will have no effect.
	// Returns the result as the ReleaseResult and UnpackResult enums.
	int UnpackCargo(bool isAttached = false, int slot = -1);

	// Deletes the cargo in the passed slot.
	// Parameters:
	//	slot: the slot number.
	// Returns the delete result as DeleteResult and ReleaseResult enums.
	int DeleteCargo(int slot);

	// Uses the available resource in the cargo in the passed slot.
	// Parameters:
	//	resourceType: the cargo type (see the standard cargo types in the manual).
	//	requiredMass: the needed mass in kilograms.
	//	slot: the slot number. If no slot is passed, the first available cargo will be used.
	// Returns the required mass or less based on the available mass, or 0 if:
	//	The cargo isn't a resource.
	//	The cargo resource type doesn't match the passed type.
	//	The cargo is empty.
	//	The slot is empty or undefined.
	double UseResource(std::string resourceType, double requiredMass, int slot = -1);

private:
	UCSO(VESSEL* vessel);

	VESSEL* vessel;
	std::map<int, ATTACHMENTHANDLE> attachsMap;
	std::vector<std::string> availableCargoList;

	double maxCargoMass;
	double maxTotalCargoMass;
	double grappleDistance;

	double releaseVelocity;
	double columnLength;
	int rowLength;
	double releaseDistance;

	double unpackDistance;

	void InitCargo();

	std::vector<VECTOR3> SetGroundList();
	bool GetNearestEmptyLocation(std::vector<VECTOR3> groundList, VECTOR3& initialPos);

	void SetSpawnName(std::string& spawnName);

	OBJHANDLE VerifySlot(int slot);
	int GetEmptySlot();
	std::pair<int, OBJHANDLE> GetOccupiedSlot();

	CargoVessel* GetResourceCargo(std::string resourceType);
};