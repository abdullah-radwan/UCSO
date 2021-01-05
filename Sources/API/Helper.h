#pragma once
#include <Orbitersdk.h>
#include <string>

namespace UCSO
{
	typedef struct Data
	{
		int type;
		double netMass;

		std::string resource;

		int unpackingType;
		int spawnCount = 1;
		bool unpacked = false;
		bool breathable = false;
		double unpackedHeight;

		std::string spawnName;
		std::string spawnModule;
		int unpackingMode;
		int unpackingDelay;
	} DataStruct;

	static void SetSpawnName(std::string& name)
	{
		// If the initial spawn name doesn't exists, use it
		if (!oapiGetVesselByName(&name[0])) return;

		for (int index = 1; index++;)
		{
			// Add the index to the string. .c_str() is used to avoid a bug
			std::string spawnName = name.c_str() + std::to_string(index);
			// If the spawn name doesn't exists
			if (!oapiGetVesselByName(&spawnName[0])) { name = spawnName; return; }
		}
	}

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

	static void SetGroundRotation(VESSELSTATUS2& status, double height)
	{
		MATRIX3 rot1 = RotationMatrix({ 0, PI05 - status.surf_lng, 0 });
		MATRIX3 rot2 = RotationMatrix({ -status.surf_lat, 0, 0 });
		MATRIX3 rot3 = RotationMatrix({ 0, 0, PI + status.surf_hdg });
		MATRIX3 rot4 = RotationMatrix({ PI05, 0, 0 });
		MATRIX3 RotMatrix_Def = mul(rot1, mul(rot2, mul(rot3, rot4)));

		status.arot.x = atan2(RotMatrix_Def.m23, RotMatrix_Def.m33);
		status.arot.y = -asin(RotMatrix_Def.m13);
		status.arot.z = atan2(RotMatrix_Def.m12, RotMatrix_Def.m11);

		status.vrot.x = height;
	}
}
