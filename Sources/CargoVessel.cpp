#pragma once
#include "CargoVessel.h"

__declspec(dllexport) double GetUCSOBuild()
{
	return 0.06;
}

void CargoVessel::SetGroundRotation(VESSELSTATUS2& status, double spawnHeight)
{
	MATRIX3 rot1 = RotationMatrix({ 0 * RAD, 90 * RAD - status.surf_lng, 0 * RAD });
	MATRIX3 rot2 = RotationMatrix({ -status.surf_lat + 0 * RAD, 0, 0 * RAD });
	MATRIX3 rot3 = RotationMatrix({ 0, 0, 180 * RAD + status.surf_hdg });
	MATRIX3 rot4 = RotationMatrix({ 90 * RAD, 0, 0 });
	MATRIX3 RotMatrix_Def = mul(rot1, mul(rot2, mul(rot3, rot4)));

	status.arot.x = atan2(RotMatrix_Def.m23, RotMatrix_Def.m33);
	status.arot.y = -asin(RotMatrix_Def.m13);
	status.arot.z = atan2(RotMatrix_Def.m12, RotMatrix_Def.m11);
	
	status.vrot.x = spawnHeight;
}

CargoVessel::DataStruct CargoVessel::GetDataStruct()
{
	return dataStruct;
}

void CargoVessel::SetAttachmentState(bool attached)
{
	released = !attached;
	this->attached = attached;
}

void CargoVessel::PackCargo()
{
	dataStruct.unpacked = false;
	SetCargoCaps();
}

bool CargoVessel::UnpackCargo(bool displayMessage)
{
	if(dataStruct.unpackType == UCSO_MODULE) 
	{
		dataStruct.unpacked = true;
		SetUnpackedCaps();
		return true;
	}

	VESSELSTATUS2 status;
	memset(&status, 0, sizeof(status));
	status.version = 2;
	GetStatusEx(&status);

	if (status.status) SetGroundRotation(status, spawnHeight);

	OBJHANDLE cargoHandle = oapiCreateVesselEx(SetSpawnName().c_str(), dataStruct.spawnModule.c_str(), &status);

	if (!cargoHandle) return false;

	oapiDeleteVessel(GetHandle(), cargoHandle);

	return true;
}

double CargoVessel::UseResource(double requiredMass)
{
	double drainedMass;

	if (dataStruct.netMass - requiredMass >= 0) drainedMass = requiredMass;
	else drainedMass = dataStruct.netMass;

	dataStruct.netMass -= drainedMass;
	SetEmptyMass(dataStruct.netMass + containerMass);

	return drainedMass;
}

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

void CargoVessel::SetCargoCaps(bool isSimInit)
{
	if (dataStruct.unpacked) return;

	VESSELSTATUS2 status;

	if (isSimInit)
	{
		memset(&status, 0, sizeof(status));
		status.version = 2;
		GetStatusEx(&status);

		SetEmptyMass(dataStruct.netMass + containerMass);

		ClearAttachments();
		CreateAttachment(true, { 0, -0.65, 0 }, { 0, 1, 0 }, { 0, 0, 1 }, "UCSO");
	}

	InsertMesh(cargoMesh.c_str(), 0);

	SetSize(0.65);

	SetPMI({ 0.28, 0.28, 0.28 });

	SetCrossSections({ 1.69, 1.69, 1.69 });

	double stiffness = -((dataStruct.netMass + containerMass) * 9.80655) / -0.001;
	double damping = 0.9 * (2 * sqrt((dataStruct.netMass + containerMass) * stiffness));

	TOUCHDOWNVTX tdvtx[4] = {
	{{ 0.5629, -0.65, -0.325 }, stiffness, damping, 3, 3},
	{{ 0, -0.65, 0.65 }, stiffness, damping, 3, 3},
	{{ -0.5629, -0.65, -0.325 }, stiffness, damping, 3, 3},
	{{ 0, 9.75, 0 }, stiffness, damping, 3, 3}
	};

	SetTouchdownPoints(tdvtx, 4);

	if (isSimInit && status.status)
	{
		SetGroundRotation(status, 0.65);

		DefSetStateEx(&status);
	}
}

void CargoVessel::SetUnpackedCaps(bool isSimInit)
{
	VESSELSTATUS2 status;

	if (isSimInit) 
	{
		memset(&status, 0, sizeof(status));
		status.version = 2;
		GetStatusEx(&status);
	}

	ClearAttachments();
	CreateAttachment(true, { 0, -unpackedHeight, 0 }, { 0, 1, 0 }, { 0, 0, 1 }, "UCSO");

	InsertMesh(unpackedMesh.c_str(), 0);

	SetSize(unpackedSize);

	SetEmptyMass(dataStruct.netMass);

	if (unpackedPMI.x && unpackedPMI.y && unpackedPMI.z) SetPMI(unpackedPMI);

	if (unpackedCS.x && unpackedCS.y && unpackedCS.z) SetCrossSections(unpackedCS);

	double stiffness = -(dataStruct.netMass * 9.80655) / -0.001;
	double damping = 0.9 * (2 * sqrt(dataStruct.netMass * stiffness));

	TOUCHDOWNVTX tdvtx[4] = {
	{ { cos(30 * RAD) * unpackedSize, -unpackedHeight, -sin(30 * RAD) * unpackedSize }, stiffness, damping, 3, 3},
	{ { 0, -unpackedHeight, unpackedSize }, stiffness, damping, 3, 3},
	{ { -cos(30 * RAD) * unpackedSize, -unpackedHeight, -sin(30 * RAD) * unpackedSize }, stiffness, damping, 3, 3},
	{ { 0, 15 * unpackedSize, 0 }, stiffness, damping, 3, 3}
	};

	SetTouchdownPoints(tdvtx, 4);

	if (isSimInit && status.status) 
	{
		SetGroundRotation(status, unpackedHeight);

		DefSetStateEx(&status);
	}
}

std::string CargoVessel::SetSpawnName()
{
	if (!oapiGetVesselByName(&dataStruct.spawnName[0])) return dataStruct.spawnName;

	for (int index = 1; index++;)
	{
		std::string spawnName = dataStruct.spawnName.c_str() + std::to_string(index);
 		if (!oapiGetVesselByName(&spawnName[0])) return spawnName;
	}

	return dataStruct.spawnName; // Useless, as the above loop is infinte, but to avoid warnings
}