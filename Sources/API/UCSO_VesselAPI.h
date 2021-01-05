// =======================================================================================
// UCSO_VesselAPI.h : The internal header of the vessels' API.
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
#include <string>
#include <vector>
#include <map>

#include "UCSO_Vessel.h"
#include "UCSO_CustomCargo.h"
#include "..\Cargo\Cargo.h"

class VesselAPI : public UCSO::Vessel
{
public:
	VesselAPI(VESSEL* vessel);

	~VesselAPI();

	const char* GetUCSOVersion() override;

	bool SetSlotAttachment(int slot, ATTACHMENTHANDLE attachmentHandle, bool opened = true) override;

	void SetSlotDoor(bool opened, int slot = -1) override;

	void SetMaxCargoMass(double maxCargoMass) override;

	void SetMaxTotalCargoMass(double maxTotalCargoMass) override;

	void SetEVAMode(bool evaMode) override;

	void SetGrappleRange(double grappleRange) override;

	void SetReleaseVelocity(double releaseVelocity) override;

	void SetCargoRowLength(int rowLength) override;

	void SetUnpackingRange(double unpackingRange) override;

	void SetCargoDeletionRange(double deletionRange) override;

	void SetResourceRange(double resourceRange) override;

	void SetBreathableRange(double breathableRange) override;

	int GetAvailableCargoCount() override;

	const char* GetAvailableCargoName(int index) override;

	CargoInfo GetCargoInfo(int slot) override;

	double GetCargoMass(int slot) override;

	double GetTotalCargoMass() override;

	GrappleResult AddCargo(int index, int slot = -1) override;

	GrappleResult GrappleCargo(int slot = -1) override;

	ReleaseResult ReleaseCargo(int slot = -1) override;

	bool PackCargo() override;

	bool UnpackCargo() override;

	ReleaseResult DeleteCargo(int slot = -1) override;

	double DrainCargoResource(const char* resource, double mass, int slot = -1) override;

	double DrainStationOrUnpackedResource(const char* resource, double mass) override;

	bool InBreathableCargo() override;

	VESSEL* GetNearestBreathableCargo() override;

	const char* SetSpawnName(const char* spawnName) override;

	void SetGroundRotation(VESSELSTATUS2& status, double spawnHeight) override;

private:
	VESSEL* vessel;

	struct SlotData 
	{
		bool opened;
		ATTACHMENTHANDLE attachHandle;
	};

	std::map<int, SlotData> attachsMap = std::map<int, SlotData>();

	std::vector<std::string> availableCargoList;

	typedef const char* (*GetVersionFunction)();
	const char* version = nullptr;

	typedef UCSO::CustomCargo* (*CustomCargoFunction)(OBJHANDLE);
	HINSTANCE customCargoDll = nullptr;
	CustomCargoFunction GetCustomCargo = nullptr;

	struct EmptyResult
	{
		int slot;
		bool opened;
	};

	struct OccupiedResult
	{
		int slot;
		bool opened;
		OBJHANDLE handle;
	};

	struct ResourceResult
	{
		bool normalCargo;
		void* cargo;
	};

	double maxCargoMass = -1;
	double maxTotalCargoMass = -1;
	bool evaMode = false;
	double grappleRange = 50;

	double releaseVelocity = 0.05;
	int rowLength = 4;
	double unpackingRange = 5;
	double deletionRange = 100;
	double resourceRange = 100;
	double breathableRange = 1000;

	void InitAvailableCargo();

	std::vector<VECTOR3> GetGroundList(VECTOR3 initialPos);
	bool GetNearestEmptyLocation(VECTOR3& initialPos);

	bool CheckAttachment(ATTACHMENTHANDLE attachHandle);
	OBJHANDLE VerifySlot(int slot);
	EmptyResult GetEmptySlot();
	OccupiedResult GetOccupiedSlot();
	ResourceResult GetResourceCargo(std::string resource);
};