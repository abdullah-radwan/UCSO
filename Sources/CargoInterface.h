#pragma once
#pragma warning( disable : 26495 )
#include <Orbitersdk.h>
#include <string>

extern "C"  { __declspec(dllexport) double GetUCSOBuild() { return 0.07; } }

typedef struct
{
	int type;
	double netMass;

	std::string resourceType;

	int unpackType;
	bool unpacked = false;

	std::string spawnName;
	std::string spawnModule;
	int unpackMode;
	int unpackDelay;
} DataStruct;

class CargoInterface : public VESSEL4
{
public:
	CargoInterface(OBJHANDLE hObj, int fmodel) : VESSEL4(hObj, fmodel) {};

	virtual DataStruct GetDataStruct() const = 0;
	virtual void SetAttachmentState(bool attached) = 0;
	virtual void PackCargo() = 0;
	virtual bool UnpackCargo() = 0;
	virtual double UseResource(double requiredMass) = 0;
};

static MATRIX3 RotationMatrix(VECTOR3 angles)
{
	MATRIX3 m;
	MATRIX3 RM_X, RM_Y, RM_Z;

	RM_X = _M(1, 0, 0, 0, cos(angles.x), -sin(angles.x), 0, sin(angles.x), cos(angles.x));
	RM_Y = _M(cos(angles.y), 0, sin(angles.y), 0, 1, 0, -sin(angles.y), 0, cos(angles.y));
	RM_Z = _M(cos(angles.z), -sin(angles.z), 0, sin(angles.z), cos(angles.z), 0, 0, 0, 1);

	m = mul(RM_X, mul(RM_Y, RM_Z));

	return m;
}

static void SetGroundRotation(VESSELSTATUS2& status, double spawnHeight)
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

