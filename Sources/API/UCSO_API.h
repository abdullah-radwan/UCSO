// =======================================================================================
// UCSO_API.h : Defines the Universal Cargo System for Orbiter (UCSO) Alpha 7 public API.
// 
// Copyright © 2019 Abdullah Radwan
// All rights reserved.
//
// =======================================================================================

#pragma once
#include "UCSO.h"
#include <map>
#include <vector>
#include "../CargoInterface.h"

class UCSO_API : public UCSO
{
public:
	UCSO_API(VESSEL* vessel);

	double GetUCSOVersion() const override;

	bool SetSlotAttachment(int slot, ATTACHMENTHANDLE attachmentHandle, bool opened = true) override;

	void SetSlotDoor(bool opened, int slot = -1) override;

	void SetMaxCargoMass(double maxCargoMass) override;

	void SetMaxTotalCargoMass(double maxTotalCargoMass) override;

	void SetMaxGrappleDistance(double grappleDistance) override;

	void SetUnpackedGrapple(bool grappleUnpacked) override;

	void SetReleaseVelocity(double releaseVelocity) override;

	void SetCargoRowLength(int rowLength) override;

	void SetMaxUnpackDistance(double unpackDistance) override;

	int GetAvailableCargoCount() const override;

	const char* GetAvailableCargoName(int index) const override;

	CargoInfo GetCargoInfo(int slot) override;

	double GetCargoMass(int slot) override;

	double GetTotalCargoMass() override;

	int AddCargo(int index, int slot = -1) override;

	GrappleResult GrappleCargo(int slot = -1) override;

	ReleaseResult ReleaseCargo(int slot = -1) override;

	bool PackCargo() override;

	UnpackResult UnpackCargo() override;

	int DeleteCargo(int slot) override;

	double UseResource(std::string resourceType, double requiredMass, int slot = -1) override;

private:
	VESSEL* vessel;
	std::map<int, ATTACHMENTHANDLE> attachsMap;
	std::map<int, bool> doorsMap;
	std::vector<std::string> availableCargoList;

	double version;

	double maxCargoMass;
	double maxTotalCargoMass;
	double grappleDistance;
	bool grappleUnpacked;

	double releaseVelocity;
	int rowLength;

	double unpackDistance;

	void InitAvailableCargo();
	bool CheckAttachment(ATTACHMENTHANDLE attachHandle);

	std::vector<VECTOR3> SetGroundList();
	bool GetNearestEmptyLocation(std::vector<VECTOR3> groundList, VECTOR3& initialPos);

	void SetSpawnName(std::string& spawnName);

	OBJHANDLE VerifySlot(int slot);
	int GetEmptySlot();
	std::pair<int, OBJHANDLE> GetOccupiedSlot();

	CargoInterface* GetResourceCargo(std::string resourceType);
};