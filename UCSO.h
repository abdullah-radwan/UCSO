#pragma once
#include <Orbitersdk.h>
#include "CargoVessel.h"

class UCSO : public CargoVessel {
public:
	UCSO(OBJHANDLE hObj, int fmodel) : CargoVessel(hObj, fmodel) { LoadConfig(); };
	~UCSO() {}
	void clbkSetClassCaps(FILEHANDLE cfg);
	void clbkLoadStateEx(FILEHANDLE scn, void* status);
	void clbkPostCreation();
	void clbkPreStep(double simt, double simdt, double mjd);
	void clbkSaveState(FILEHANDLE scn);

private:
	double timer = 0;
	bool landing = false;
	bool timing = false;

	void LoadConfig();
};