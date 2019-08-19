#pragma once
#pragma warning( disable : 26495 )
#include <Orbitersdk.h>
#include <string>

class CargoVessel : public VESSEL4
{
public:
	struct DataStruct {
		int type;

		std::string resourceType;
		double resourceMass;

		std::string spawnName;
		std::string spawnModule;
		double spawnHeight;
		int unpackMode;
		int unpackDelay;
	};

	static MATRIX3 RotationMatrix(VECTOR3 angles);

	CargoVessel(OBJHANDLE hObj, int fmodel) : VESSEL4(hObj, fmodel) {};

	DataStruct GetDataStruct();
	void SetAttachmentState(bool isAttached);
	bool UnpackCargo(bool displayMessage = false);
	double UseResource(double requiredMass);

protected:
	struct DataStruct dataStruct;

	bool displayInfo = false;
	bool isReleased = false;
	bool isAttached = false;
	double containerMass = 0;

	std::string SetSpawnName();
	void DisplayMessage(std::string message);
};

