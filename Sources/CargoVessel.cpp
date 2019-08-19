#pragma once
#include "CargoVessel.h"

MATRIX3 CargoVessel::RotationMatrix(VECTOR3 angles)
{
	MATRIX3 m;
	MATRIX3 RM_X, RM_Y, RM_Z;

	RM_X = _M(1, 0, 0, 0, cos(angles.x), -sin(angles.x), 0, sin(angles.x), cos(angles.x));
	RM_Y = _M(cos(angles.y), 0, sin(angles.y), 0, 1, 0, -sin(angles.y), 0, cos(angles.y));
	RM_Z = _M(cos(angles.z), -sin(angles.z), 0, sin(angles.z), cos(angles.z), 0, 0, 0, 1);

	m = mul(RM_X, mul(RM_Y, RM_Z));

	return m;
}

CargoVessel::DataStruct CargoVessel::GetDataStruct()
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

	if (status.status) {
		MATRIX3 rot1 = RotationMatrix({ 0 * RAD, 90 * RAD - status.surf_lng, 0 * RAD });
		MATRIX3 rot2 = RotationMatrix({ -status.surf_lat + 0 * RAD, 0, 0 * RAD });
		MATRIX3 rot3 = RotationMatrix({ 0, 0, 180 * RAD + status.surf_hdg });
		MATRIX3 rot4 = RotationMatrix({ 90 * RAD, 0, 0 });
		MATRIX3 RotMatrix_Def = mul(rot1, mul(rot2, mul(rot3, rot4)));

		status.arot.x = atan2(RotMatrix_Def.m23, RotMatrix_Def.m33);
		status.arot.y = -asin(RotMatrix_Def.m13);
		status.arot.z = atan2(RotMatrix_Def.m12, RotMatrix_Def.m11);
		status.vrot.x = dataStruct.spawnHeight;
	}

	OBJHANDLE cargoHandle = oapiCreateVesselEx(SetSpawnName().c_str(), dataStruct.spawnModule.c_str(), &status);

	if (!cargoHandle) {
		if (displayMessage) DisplayMessage("cargo unpack failed");
		return false;
	}

	if (displayMessage) DisplayMessage("cargo unpacked successfully");
	oapiDeleteVessel(GetHandle(), cargoHandle);
	return true;
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