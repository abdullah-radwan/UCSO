#include <Orbitersdk.h>
#include "UCSO_Constants.h"

class CargoVessel : public VESSEL4
{
public:
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

	void SetSpawnName();
	void DisplayMessage(std::string message);
};

