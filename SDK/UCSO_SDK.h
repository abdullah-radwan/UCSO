#include <orbitersdk.h>
#include <map>
#include "../CargoVessel.h"

#pragma comment(lib,"UCSO_SDK.lib")
class UCSO
{
public:
	enum GrappleResult {
		CARGO_GRAPPLED = 0, /** Cargo is grappled successfully. */
		NO_CARGO_IN_RANGE, /** No cargo in range. */
		MAX_MASS_EXCEEDED, /** Maximum one cargo mass will be exceeded if the cargo is added. */
		MAX_TOTAL_MASS_EXCEEDED, /** Maximum total cargo mass will be exceeded if the cargo is added. */
		SLOT_OCCUPIED,     /** The given slot is occupied. */
		SLOT_UNDEFINED,    /** The given slot is undefiend. */
		GRAPPLE_FAILED     /** Grapple has failed. */
	};
	enum ReleaseResult
	{
		CARGO_RELEASED = 0, /** Cargo is released successfully. */
		SLOT_EMPTY,        /** The given slot is empty, or all slots are empty if -1 is passed. */
		SLOT_UNDEF,        /** The given slot is undefined, or the no slot is defined. */
		RELEASE_FAILED     /** Release has failed. */
	};
	enum UnpackResult
	{
		CARGO_UNPACKED = 4, /** Cargo is unpacked successfully. */
		NOT_UNPACKABLE, /** Cargo is unpackable. */
		UNPACK_FAILED  /** Unpack has failed. */
	};
	enum DeleteResult
	{
		CARGO_DELETED = 4, /** Cargo is deleted successfully. */
		DELETE_FAILED      /** Delete has failed. */
	};

	/**
	 * \brief Create UCSO instance.
	 * \param vessel pointer to the vessel.
	 * \return a UCSO instance.
	*/
	static UCSO* Init(VESSEL* vessel);

	/**
	 * \brief Set the slot number for a given attachment, to use with the SDK functions.
	 * \param slot The slot number.
     * You can use any number but not -1, as it's reserved.
	 * You can use the number multiple times to update the slot attachment handle.
	 * \param attachmentHandle The attachment handle.
	*/
	void SetSlotAttachment(int slot, ATTACHMENTHANDLE attachmentHandle);

	/**
	 * \brief Set the maximum one cargo mass for the vessel.
	 * If any cargo mass exceeded this mass, the cargo won't be added.
	 * \param maxCargoMass the maximum mass in kilograms. Set -1 for unlimited mass. The default value is -1.
	*/
	void SetMaxCargoMass(double maxCargoMass);

	/**
	 * \brief Set the maximum total cargo mass for the vessel.
	 * If the total cargo mass will exceed this mass if the cargo is added, the cargo won't be added.
	 * \param maxTotalCargoMass the maximum mass in kilograms. Set -1 for unlimited mass. The default value is -1.
	*/
	void SetMaxTotalCargoMass(double maxTotalCargoMass);

	/**
	 * \brief Set the maximum grapple distance in meters.
	 * \param grappleDistance the distance in meters. The default value is 50 meters.
	*/
	void SetMaxGrappleDistance(double grappleDistance);

	/**
	 * \brief Set the cargo release velocity if released in space.
	 * \param releaseVel the release velocity in m/s. The default value is 0.05 m/s.
	*/
	void SetReleaseVelocity(double releaseVel);

	/**
	 * \brief Grapple the cargo in the passed slot.
	 * \param slot the slot number. If no slot is passed, the first empty slot will be used.
	 * \return the result as the GrappleResult enum.
	*/
	int GrappleCargo(int slot = -1);

	/**
	 * \brief Release the cargo in the passed slot.
	 * \param slot the slot number. If no slot is passed, the first occupied slot will be used.
	 * \return the result as the ReleaseResult enum.
	*/
	int ReleaseCargo(int slot = -1);

	/**
	 * \brief Unpack the cargo in the passed slot.
	 * \param slot the slot number. If no slot is passed, the first occupied slot will be used.
	 * \return the result as the GrappleResult and ReleaseResult enum.
	*/
	int UnpackCargo(int slot = -1);

	/**
	* \brief Delete the cargo in the passed slot.
	* \param slot the slot number.
	* \return the delete result as the DeleteResult and ReleaseResult enum.
	*/
	int DeleteCargo(int slot);

	/**
	  * \brief Use the available resource in the cargo in the passed slot.
	  * \param resourceType the cargo type (see the standard cargo types in the manual).
	  * \param requiredMass the needed mass in kilograms.
      * \param slot the slot number. If no slot is passed, the first available cargo will be used.
	  * \return the required mass or less based on the available mass, or 0 if:
      * The cargo isn't a resource.
	  * The cargo resource type doesn't match the passed type.
	  * The cargo is empty.
	  * The slot is empty or undefined.
	*/
	double UseResource(std::string resourceType, double requiredMass, int slot = -1);

	/**
	 * \brief Get the cargo mass in the passed slot.
	 * \param slot the slot number.
	 * \return the cargo mass in kilograms, or -1 if the slot is empty or undefined.
	*/
	double GetCargoMass(int slot);

	/**
	  * \brief Get the total cargo mass.
	  * \return The total cargo mass in kilograms.
	*/
	double GetCargoTotalMass();

private:
	UCSO() {};
	UCSO(VESSEL* vessel);

	VESSEL* vessel;
	std::map<int, ATTACHMENTHANDLE> attachsMap;

	double maxCargoMass;
	double maxTotalCargoMass;
	double grappleDistance;
	double releaseVel;
	double totalCargoMass;

	bool isTotalMassGet = false;

	OBJHANDLE VerifySlot(int slot);
	int GetEmptySlot();
	std::pair<int, OBJHANDLE> GetOccupiedSlot();

	void GetTotalCargoMass();

	CargoVessel* GetResourceCargo(std::string resourceType);
};

