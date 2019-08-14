#pragma once
#include "CargoVessel.h"

DataStruct CargoVessel::GetDataStruct()
{
	return dataStruct;
}

void CargoVessel::SetAttachmentState(bool isAttached)
{
	isReleased = !isAttached;
	this->isAttached = isAttached;
}

bool CargoVessel::UnpackCargo(bool displayMessage)
{
	VESSELSTATUS2 status;
	memset(&status, 0, sizeof(status));
	status.version = 2;
	GetStatusEx(&status);

	OBJHANDLE cargo = oapiCreateVesselEx(SetSpawnName().c_str(), dataStruct.spawnModule.c_str(), &status);

	if (cargo) {
		if (displayMessage) DisplayMessage("cargo unpacked successfully");
		oapiDeleteVessel(GetHandle(), cargo);
		return true;
	}
	else if (displayMessage) DisplayMessage("cargo unpack failed");
	return false;
}

double CargoVessel::UseResource(double requiredMass)
{
	double drainedMass;
	if (dataStruct.resourceMass - requiredMass >= 0) drainedMass = requiredMass;
	else drainedMass = dataStruct.resourceMass;
	dataStruct.resourceMass -= drainedMass;
	SetEmptyMass(dataStruct.resourceMass + containerMass);

	return drainedMass;
}

std::string CargoVessel::SetSpawnName()
{
	if (!oapiGetVesselByName(&dataStruct.spawnName[0])) return dataStruct.spawnName;

	for (int index = 1; index++;)
	{
		std::string spawnName = dataStruct.spawnName.c_str() + std::to_string(index - 1);
 		if (!oapiGetVesselByName(&spawnName[0])) return spawnName;
	}

	return std::string();
}

void CargoVessel::DisplayMessage(std::string message)
{
	std::string cargoName = GetName();
	cargoName += " " + message;
	sprintf(oapiDebugString(), cargoName.c_str());
}