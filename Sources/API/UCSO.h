// =======================================================================================
// UCSO.h : Defines the Universal Cargo System for Orbiter (UCSO) Alpha 7 public API.
// 
// Copyright © 2019 Abdullah Radwan
// All rights reserved.
//
// =======================================================================================

#pragma once
#include <Orbitersdk.h>

class UCSO
{
public:
	// The grapple result as returned from GrappleCargo and AddCargo method.
	enum GrappleResult
	{
		CARGO_GRAPPLED = 0,      // The cargo is grappled successfully.
		NO_CARGO_IN_RANGE,       // No cargo in the grapple range.
		MAX_MASS_EXCEEDED,       // The maximum one cargo mass will be exceeded if the cargo is added.
		MAX_TOTAL_MASS_EXCEEDED, // The maximum total cargo mass will be exceeded if the cargo is added.
		GRAPPLE_SLOT_OCCUPIED,   // The passed slot (or all slots) is occupied.
		GRAPPLE_SLOT_CLOSED,	 // The passed slot (or all slots) door is closed.
		GRAPPLE_SLOT_UNDEFINED,  // The passed slot (or all slots) is undefiend or invalid.
		GRAPPLE_FAILED           // The grapple failed.
	};

	// The addition result as returned from AddCargo method.
	enum AddResult
	{
		CARGO_ADDED = 8, // The cargo is added successfully.
		INVALID_INDEX,   // The passed index is invalid.
		ADD_FAILED       // The addition failed.
	};

	// The release result as returned from ReleaseCargo, UnpackCargo, and DeleteCargo methods.
	enum ReleaseResult
	{
		CARGO_RELEASED = 0,      // The cargo is released successfully. For ReleaseCargo method only.
		RELEASE_SLOT_EMPTY,      // The passed slot is empty, or all slots are empty if -1 is passed.
		RELEASE_SLOT_CLOSED,     // The passed slot (or all slots) door is closed.
		RELEASE_SLOT_UNDEFINED,  // The passed slot (or all slots) is undefiend or invalid.
		NO_EMPTY_POSITION,       // There is no empty position near the vessel for release on the ground. For ReleaseCargo method only.
		RELEASE_FAILED           // The release failed. For ReleaseCargo method only.
	};

	// The unpack result as returned from UnpackCargo method.
	enum UnpackResult
	{
		CARGO_UNPACKED = 0,     // The cargo is unpacked successfully.
		NOT_UNPACKABLE,         // The cargo is unpackable.
		NO_UNPACKABLE_IN_RANGE, // No unpackable cargo in range
		UNPACK_FAILED           // The unpack failed.
	};

	// The delete result as returned from DeleteCargo method.
	enum DeleteResult
	{
		CARGO_DELETED = 5, // The cargo is deleted successfully.
		DELETE_FAILED      // The delete failed.
	};


	// Cargo type as returned from GetCargoInfo method.
	enum CargoType
	{
		STATIC = 0,
		RESOURCE,
		UNPACKABLE
	};

	// Unpack type as returned from GetCargoInfo method.
	enum UnpackType
	{
		UCSO_MODULE = 0,
		ORBITER_VESSEL
	};

	// Unpack mode as returned from GetCargoInfo method.
	enum UnpackMode
	{
		LANDED = 0,
		DELAYED,
		MANUAL
	};

	// The cargo information as returned from GetCargoInfo method.
	typedef struct
	{
		bool valid;               // True if the struct contains informations, false if not.

		const char* name;         // The cargo name in the scenario.
		CargoType type;           // The cargo type as the CargoType enum.
		double mass;              // The cargo mass in kilograms.

		const char* resourceType; // The resource type.
		double resourceMass;      // The resource mass in kilograms: the cargo mass minus the container mass.

		UnpackType unpackType;    // The unpack type as the UnpackType enum.

		const char* spawnModule;  // The spawn module. Defined only if the unpack type is ORBITER_VESSEL.
		UnpackMode unpackMode;    // The unpack mode as the UnpackMode enum. Defined only if the unpack type is ORBITER_VESSEL.
		int unpackDelay;          // The unpack delay in seconds. Defined only if the unpack type is ORBITER_VESSEL.
	} CargoInfo;

	// Performs one-time initialization of UCSO. It can be called from your vessel's constructor.
	// Parameters:
	//	vessel: pointer to the calling vessel.
	// Returns a UCSO instance.
	// NOTE: Don't forget to delete the returned object when you no longer need it (e.g. in your vessel's destructor).
	static UCSO* CreateInstance(VESSEL* vessel);

	// Returns the UCSO version, in the following format: (Stable).(Beta)(Alpha), or 0 if UCSO isn't installed.
	// Examples: Alpha 6 = 0.06. Beta 4 = 0.4. Stable 1 = 1.0. Stable 1.1 = 1.1.
	virtual double GetUCSOVersion() const  = 0;

	// Sets the slot number for a given attachment, to use with the API methods. It must be called before any other calls to the API.
	// To delete a slot, pass the slot number and NULL.
	// Parameters:
	//	slot: the slot number. You can use any number but not -1, as it's reserved.
	//		You can use the number multiple times to update the slot attachment handle.
	//	attachmentHandle: the attachment handle. If NULL, the passed slot will be removed.
	//	opened: true if the slot door is opened, false if not.
	//	Returns true if the slot is added or removed, or false if the attachment handle is invalid.
	virtual bool SetSlotAttachment(int slot, ATTACHMENTHANDLE attachmentHandle, bool opened = true) = 0;

	// Sets the slot door status.
	// Parameters:
	//	opened: the door status. True if opened, false if closed.
	//	slot: the slot number. If no slot or -1 is passed all slots door status will be changed.
	// The status won't be set if the passed slot doesn't exists.
	virtual void SetSlotDoor(bool opened, int slot = -1) = 0;

	// Sets the maximum one cargo mass for the vessel.
	// If any cargo mass exceeded this mass, the cargo won't be added.
	// Parameters:
	//	maxCargoMass: the maximum mass in kilograms. Set -1 for unlimited mass. The default value is -1.
	virtual void SetMaxCargoMass(double maxCargoMass) = 0;

	// Sets the maximum total cargo mass for the vessel.
	// If the total cargo mass will exceed this mass if the cargo is added, the cargo won't be added.
	// Parameters:
	//	maxTotalCargoMass: the maximum mass in kilograms. Set -1 for unlimited mass. The default value is -1.
	virtual void SetMaxTotalCargoMass(double maxTotalCargoMass) = 0;

	// Sets the maximum grapple distance in meters.
	// Parameters:
	//	grappleDistance: the distance in meters. The default value is 50 meters.
	virtual void SetMaxGrappleDistance(double grappleDistance) = 0;

	// Allows the cargo to be grappled if it's unpacked.
	// Parameters:
	//	grappleUnpacked: set true to grapple unpacked cargo, false to not. The default value is false.
	virtual void SetUnpackedGrapple(bool grappleUnpacked) = 0;

	// Sets the cargo release velocity if released in space.
	// Parameters:
	//	releaseVelocity: the release velocity in m/s. The default value is 0.05 m/s.
	virtual void SetReleaseVelocity(double releaseVelocity) = 0;

	// Sets the cargo row length if released on the ground.
	// Parameters:
	//	rowLength: the cargo row length. The default value is 4 cargo.
	virtual void SetCargoRowLength(int rowLength) = 0;

	// Sets the maximum unpack distance in meters.
	// Parameters:
	//	unpackDistance: the distance in meters. The default value is 3 meters.
	virtual void SetMaxUnpackDistance(double unpackDistance) = 0;

	// Returns the available cargo count, which is the number of cargoes in Config\Vessels\UCSO folder.
	virtual int GetAvailableCargoCount() const = 0;

	// Returns the cargo name from the passed index, which is the filename from Config\Vessels\UCSO folder without .cfg,
	// or empty string if the index is invalid.
	// Parameters:
	//	index: the cargo index. It must equal or be higher than 0 and lower than the available cargo count.
	virtual const char* GetAvailableCargoName(int index) const = 0;

	// Returns the cargo information as the CargoInfo struct, or an empty struct if the passed slot is invalid.
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
	//	index: the cargo index. It must equal or be higher than 0 and lower than the available cargo count.
	//  slot: the slot number. If no slot or -1 is passed, the first empty slot will be used.
	// Returns the result as GrappleResult and AddResult enums.
	virtual int AddCargo(int index, int slot = -1) = 0;

	// Grapples the cargo in the passed slot.
	// Parameters:
	//	slot: the slot number. If no slot or -1 is passed, the first empty slot will be used.
	// Returns the result as the GrappleResult enum. 
	virtual GrappleResult GrappleCargo(int slot = -1) = 0;

	// Releases the cargo in the passed slot.
	// Parameters:
	//	slot: the slot number. If no slot or -1 is passed, the first occupied slot will be used.
	// Returns the result as the ReleaseResult enum.
	virtual ReleaseResult ReleaseCargo(int slot = -1) = 0;

	// Packs the nearest cargo. Returns true if the cargo is packed, or false if no packable cargo in the unpack range.
	virtual bool PackCargo() = 0;

	// Unpacks the nearest cargo. Returns the result as the UnpackResult enums.
	virtual UnpackResult UnpackCargo() = 0;

	// Deletes the cargo in the passed slot.
	// Parameters:
	//	slot: the slot number.
	// Returns the delete result as DeleteResult and ReleaseResult enums.
	virtual int DeleteCargo(int slot) = 0;

	// Uses the available resource in the cargo in the passed slot.
	// Parameters:
	//	resourceType: the cargo type (see the standard cargo types in the manual).
	//	requiredMass: the needed mass in kilograms.
	//	slot: the slot number. If no slot or -1 is passed, the first available cargo will be used.
	// Returns the required mass or less, based on the cargo available resource mass, or 0 if:
	//	The passed slot (or all slots) cargo isn't a resource.
	//	The passed slot (or all slots) cargo resource type doesn't match the passed type.
	//	The passed slot (or all slots) cargo is empty,.
	//	The passed slot (or all slots) is empty, undefined, or its attachment handle is invalid.
	virtual double UseResource(std::string resourceType, double requiredMass, int slot = -1) = 0;

protected:
		// The constructor has a protected access to prevent incorrect instantiation by the vessel code.
		// Always use the static CreateInstance method to create an instance of UCSO.
		UCSO() {};
};