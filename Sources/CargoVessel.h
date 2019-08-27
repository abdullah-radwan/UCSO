#pragma once
#pragma warning( disable : 26495 )
#include <Orbitersdk.h>
#include <string>

extern "C" { __declspec(dllexport) double GetUCSOBuild(); }

class CargoVessel : public VESSEL4
{
public:
	struct DataStruct 
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
	};

	static void SetGroundRotation(VESSELSTATUS2& status, double spawnHeight);

	CargoVessel(OBJHANDLE hObj, int fmodel) : VESSEL4(hObj, fmodel) {};

	DataStruct GetDataStruct();
	void SetAttachmentState(bool attached);
	void PackCargo();
	bool UnpackCargo(bool displayMessage = false);
	double UseResource(double requiredMass);

protected:
	enum UnpackType
	{
		UCSO_MODULE = 0,
		ORBITER_VESSEL
	};

	struct DataStruct dataStruct;

	UINT meshIndex;
	double unpackedSize;
	std::string cargoMesh, unpackedMesh;
	double spawnHeight, unpackedHeight;
	VECTOR3 unpackedPMI, unpackedCS;

	bool released = false;
	bool attached = false;
	double containerMass = 0;

	static MATRIX3 RotationMatrix(VECTOR3 angles);

	void SetCargoCaps(bool isSimInit = true);
	void SetUnpackedCaps(bool isSimInit = true);

	std::string SetSpawnName();
};

