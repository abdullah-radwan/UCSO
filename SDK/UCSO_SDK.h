// ==============================================================
// UCSO_SDK.h : Defines the Universal Cargo System for Orbiter public SDK.
// 
// Copyright © 2019 Abdullah Radwan
// All rights reserved.
//
// ==============================================================

#pragma once
#include <orbitersdk.h>
#include <map>
#include "../CargoVessel.h"

class UCSO
{
public:
	// The grapple result, as returned from GrappleCargo function.
	enum GrappleResult {
		CARGO_GRAPPLED = 0,      // The cargo is grappled successfully.
		NO_CARGO_IN_RANGE,       // No cargo in the grapple range.
		MAX_MASS_EXCEEDED,       // The maximum one cargo mass will be exceeded if the cargo is added.
		MAX_TOTAL_MASS_EXCEEDED, // The maximum total cargo mass will be exceeded if the cargo is added.
		SLOT_OCCUPIED,           // The passed slot is occupied.
		SLOT_UNDEFINED,          // The passed slot is undefiend.
		GRAPPLE_FAILED           // The grapple failed.
	};

	// The release result, as returned from ReleaseCargo, UnpackCargo, and DeleteCargo functions.
	enum ReleaseResult
	{
		CARGO_RELEASED = 0, // The cargo is released successfully. For ReleaseCargo function only.
		SLOT_EMPTY,         // The passed slot is empty, or all slots are empty if -1 is passed.
		SLOT_UNDEF,         // The passed slot is undefined, or the no slots are defined.
		RELEASE_FAILED      // The release failed. For ReleaseCargo function only.
	};

	// The unpack result, as returned from UnpackCargo function.
	enum UnpackResult
	{
		CARGO_UNPACKED = 4, // The cargo is unpacked successfully.
		NOT_UNPACKABLE,     // The cargo is unpackable.
		UNPACK_FAILED       // The unpack failed.
	};

	// The delete result as returned from DeleteCargo function.
	enum DeleteResult
	{
		CARGO_DELETED = 4, // The cargo is deleted successfully.
		DELETE_FAILED      // The delete failed.
	};

	// Performs one-time initialization of UCSO. It should be called from your vessel's constructor.
	// Parameters:
	//	vessel: pointer to the calling vessel.
	// Returns a UCSO instance.
	// NOTE: Don't forget to delete the returned object when you no longer need it (e.g., in your vessel's destructor).
	static UCSO* CreateInstance(VESSEL* vessel);

	// Sets the slot number for a given attachment, to use with the SDK functions. It must be called before any other calls to the SDK.
	// To delete a slot, pass the slot number and NULL for the attachment handle.
	// Parameters:
	//	slot: the slot number. You can use any number but not -1, as it's reserved.
	//		You can use the number multiple times to update the slot attachment handle.
	//	attachmentHandle: the attachment handle. If NULL, the slot will be removed.
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
	//	releaseVel: the release velocity in m/s. The default value is 0.05 m/s.
	void SetReleaseVelocity(double releaseVel);

	// Grapples the cargo in the passed slot.
	// Parameters:
	//	slot: the slot number. If no slot is passed, the first empty slot will be used.
	// Returns the result as the GrappleResult enum.
	int GrappleCargo(int slot = -1);

	// Releases the cargo in the passed slot.
	// Parameters:
	//	slot: the slot number. If no slot is passed, the first occupied slot will be used.
	// Returns the result as the ReleaseResult enum.
	int ReleaseCargo(int slot = -1);

	// Unpacks the cargo in the passed slot.
	// Parameters:
	//	slot the slot number. If no slot is passed, the first occupied slot will be used.
	// Returns the result as the GrappleResult and ReleaseResult enum.
	int UnpackCargo(int slot = -1);

	// Deletes the cargo in the passed slot.
	// Parameters:
	//	slot the slot number.
	// Returns the delete result as the DeleteResult and ReleaseResult enum.
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

	// Gets the cargo mass in the passed slot.
	// Parameters:
	//	slot: the slot number.
	// Returns the cargo mass in kilograms, or -1 if the slot is empty or undefined.
	double GetCargoMass(int slot);

	// Gets the total cargo mass.
	// Returns the total cargo mass in kilograms.
	double GetCargoTotalMass();

private:
	UCSO(VESSEL* vessel);

	VESSEL* vessel;
	std::map<int, ATTACHMENTHANDLE> attachsMap;

	double maxCargoMass;
	double maxTotalCargoMass;
	double grappleDistance;
	double releaseVel;
	double totalCargoMass;

	bool isTotalMassGet;

	OBJHANDLE VerifySlot(int slot);
	int GetEmptySlot();
	std::pair<int, OBJHANDLE> GetOccupiedSlot();

	void GetTotalCargoMass();

	CargoVessel* GetResourceCargo(std::string resourceType);
};