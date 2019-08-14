#include "UCSO_SDK.h"

UCSO* UCSO::Init(VESSEL* vessel) {
	return new UCSO(vessel);
}

UCSO::UCSO(VESSEL* vessel)
{
	this->vessel = vessel;
	attachsMap = std::map<int, ATTACHMENTHANDLE>();
	maxCargoMass = -1;
	maxTotalCargoMass = -1;
	totalCargoMass = 0;
	grappleDistance = 50;
	releaseVel = 1;
}

void UCSO::SetSlotAttachment(int slot, ATTACHMENTHANDLE attachmentHandle)
{
	attachsMap[slot] = attachmentHandle;
}

void UCSO::SetMaxCargoMass(double maxCargoMass)
{
	this->maxCargoMass = maxCargoMass;
}

void UCSO::SetMaxTotalCargoMass(double maxTotalCargoMass)
{
	this->maxTotalCargoMass = maxTotalCargoMass;
}

void UCSO::SetMaxGrappleDistance(double grappleDistance)
{
	this->grappleDistance = grappleDistance;
}

void UCSO::SetReleaseVelocity(double releaseVel)
{
	this->releaseVel = releaseVel;
}

int UCSO::GrappleCargo(int slot)
{
	if (attachsMap.empty()) return SLOT_UNDEFINED;

	else if (slot == -1) {
		slot = GetEmptySlot();
		if (slot == -1) return SLOT_OCCUPIED;
	}
	// If the slot doesn't exists
	else if (attachsMap.find(slot) == attachsMap.end()) return SLOT_UNDEFINED;
	else if (VerifySlot(slot)) return SLOT_OCCUPIED;

	for (DWORD vesselIndex = 0; vesselIndex < oapiGetVesselCount(); vesselIndex++)
	{
		VESSEL* vCargo = oapiGetVesselInterface(oapiGetVesselByIndex(vesselIndex));
		if (strncmp(vCargo->GetClassNameA(), "UCSO", 4) == 0) {
			VECTOR3 pos;
			vessel->GetRelativePos(vCargo->GetHandle(), pos);

			if (length(pos) <= grappleDistance) {
				CargoVessel* cargo = static_cast<CargoVessel*>(vCargo);

				double cargoMass = cargo->GetEmptyMass();

				if (maxCargoMass != -1) { if (cargoMass > maxCargoMass) return MAX_MASS_EXCEEDED; }

				if (maxTotalCargoMass != -1) {
					if (!isTotalMassGet) GetTotalCargoMass();

					if (totalCargoMass + cargoMass > maxTotalCargoMass) return MAX_TOTAL_MASS_EXCEEDED;
					else totalCargoMass += cargoMass;
				}

				if (vessel->AttachChild(cargo->GetHandle(), attachsMap[slot], cargo->GetAttachmentHandle(true, 0))) {
					cargo->SetAttachmentState(true);
					return CARGO_GRAPPLED;
				}
				else return GRAPPLE_FAILED;
			}
		}
	}

	return NO_CARGO_IN_RANGE;
}

int UCSO::ReleaseCargo(int slot)
{
	if (attachsMap.empty()) return SLOT_UNDEF;

	std::pair<int, OBJHANDLE> pair;

	if (slot == -1) {
		pair = GetOccupiedSlot();
		if (!pair.second) return SLOT_EMPTY;
	}
	// If the slot doesn't exists
	else if (attachsMap.find(slot) == attachsMap.end()) return SLOT_UNDEF;
	else {
		pair.second = VerifySlot(slot);
		if(!pair.second) return SLOT_EMPTY;
		pair.first = slot;
	}

	bool cargoReleased = false;

	CargoVessel* cargo = static_cast<CargoVessel*>(oapiGetVesselInterface(pair.second));

	if (vessel->GetFlightStatus() & 1); // Release when on ground
	else {
		auto objectHandle = GetAttachmentStatus(attachsMap[pair.first]);
		cargoReleased = vessel->DetachChild(attachsMap[pair.first], releaseVel);

		/* Vessel Handle is presumeably non-null at this point, but it's safer to check */
		if (objectHandle != NULL) {
			/* Retrieve vessel handle of the released cargo and retrive its VESSELSTATUS struct */
			auto vesselHandle = oapiGetVesselInterface(objectHandle);
			VESSELSTATUS2 vs2;
			memset(&vs2, 0, sizeof(vs2));
			vs2.version = 2;
			GetStatusEx(&vs2);

			/* Convert the release velocity to the ecliptic frame & set the vessel state */
			VECTOR3 relativeVelocity;
			Local2Rel(_V(0,releaseVel,0), relativeVelocity);
			vs2.rvel += relativeVelocity;
			DefSetStateEx(&vs2);
		}

	}

	if (cargoReleased) { cargo->SetAttachmentState(false); return CARGO_RELEASED; }

	return RELEASE_FAILED;
}

int UCSO::UnpackCargo(int slot)
{
	if (attachsMap.empty()) return SLOT_UNDEF;

	OBJHANDLE cargoHandle;
	if (slot == -1) {
		cargoHandle = GetOccupiedSlot().second;
		if (!cargoHandle) return SLOT_EMPTY;
	}
	else if (attachsMap.find(slot) == attachsMap.end()) return SLOT_UNDEF;
	else {
		cargoHandle = VerifySlot(slot);
		if (!cargoHandle) return SLOT_EMPTY;
	}

	CargoVessel* cargo = static_cast<CargoVessel*>(oapiGetVesselInterface(cargoHandle));
	struct DataStruct dataStruct = cargo->GetDataStruct();

	if (dataStruct.type != UNPACKABLE) return NOT_UNPACKABLE;
	else if (!cargo->UnpackCargo()) return UNPACK_FAILED;

	return CARGO_UNPACKED;
}

int UCSO::DeleteCargo(int slot)
{
	if (attachsMap.empty()) return SLOT_UNDEF;

	OBJHANDLE cargoHandle;
	if (slot == -1) {
		cargoHandle = GetOccupiedSlot().second;
		if (!cargoHandle) return SLOT_EMPTY;
	}
	else if (attachsMap.find(slot) == attachsMap.end()) return SLOT_UNDEF;
	else {
		cargoHandle = VerifySlot(slot);
		if (!cargoHandle) return SLOT_EMPTY;
	}

	if (oapiDeleteVessel(cargoHandle)) return CARGO_DELETED;
	return DELETE_FAILED;
}

double UCSO::UseResource(std::string resourceType, double requiredMass, int slot)
{
	if (attachsMap.empty() || requiredMass <= 0 || resourceType.empty()) return 0;

	CargoVessel* cargo;
	if (slot == -1) {
		cargo = GetResourceCargo(resourceType);
		if (!cargo) return 0;
	}
	else if (attachsMap.find(slot) == attachsMap.end()) return 0;
	else {
		OBJHANDLE cargoHandle = VerifySlot(slot);
		if (!cargoHandle) return 0;
		cargo = static_cast<CargoVessel*>(oapiGetVesselInterface(cargoHandle));
	}

	return cargo->UseResource(requiredMass);
}

double UCSO::GetCargoMass(int slot)
{
	if (attachsMap.empty() || attachsMap.find(slot) == attachsMap.end()) return -1;

	OBJHANDLE cargoHandle = VerifySlot(slot);
	if (!cargoHandle) return -1;

	return oapiGetEmptyMass(cargoHandle);
}

double UCSO::GetCargoTotalMass()
{
	if (!isTotalMassGet) GetTotalCargoMass();
	return totalCargoMass;
}

OBJHANDLE UCSO::VerifySlot(int slot)
{
	return vessel->GetAttachmentStatus(attachsMap[slot]);
}

int UCSO::GetEmptySlot()
{
	for (auto const& it : attachsMap) if (!VerifySlot(it.first)) return it.first;
	return -1;
}

std::pair<int, OBJHANDLE> UCSO::GetOccupiedSlot()
{
	for (auto const& it : attachsMap) if (VerifySlot(it.first)) return { it.first, VerifySlot(it.first) };
	return {-1, NULL};
}

void UCSO::GetTotalCargoMass()
{
	for (auto const& it : attachsMap)
	{
		OBJHANDLE cargo = VerifySlot(it.first);
		if (cargo) totalCargoMass += oapiGetEmptyMass(cargo);
	}
	isTotalMassGet = true;
}

CargoVessel* UCSO::GetResourceCargo(std::string resourceType)
{
	for (auto const& it : attachsMap) {
		OBJHANDLE cargoHandle = VerifySlot(it.first);
		if (cargoHandle) {
			CargoVessel* cargo = static_cast<CargoVessel*>(oapiGetVesselInterface(cargoHandle));
			struct DataStruct dataStruct = cargo->GetDataStruct();
			if (dataStruct.type == RESOURCE &&
				_strcmpi(dataStruct.resourceType.c_str(), resourceType.c_str()) == 0 &&
				dataStruct.resourceMass > 0) return cargo;
		}
	};
	return nullptr;
}