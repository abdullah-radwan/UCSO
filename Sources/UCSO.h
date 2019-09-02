#pragma once
#include "CargoInterface.h"
#include <vector>

class UCSO : public CargoInterface
{
public:
	UCSO(OBJHANDLE hObj, int fmodel) : CargoInterface(hObj, fmodel) { LoadConfig(); }

	void clbkSetClassCaps(FILEHANDLE cfg);
	void clbkLoadStateEx(FILEHANDLE scn, void* status);
	void clbkPostCreation();
	void clbkPreStep(double simt, double simdt, double mjd);
	void clbkSaveState(FILEHANDLE scn);

	DataStruct GetDataStruct() const override;
	void SetAttachmentState(bool attached) override;
	void PackCargo() override;
	bool UnpackCargo() override;
	double UseResource(double requiredMass) override;

private:
	enum CargoType 
	{
		STATIC = 0,
		RESOURCE,
		UNPACKABLE
	};

	enum UnpackType
	{
		UCSO_MODULE = 0,
		ORBITER_VESSEL
	};

	enum UnpackMode 
	{
		LANDED = 0,
		DELAYED,
		MANUAL
	};

	/*
	struct CustomPropertiesStruct
	{
		struct SpotLightStruct 
		{
			bool valid = false;
			SpotLight* spotLight;
			BEACONLIGHTSPEC beaconSpec;
			VECTOR3 color;
			VECTOR3 pos, dir;
			double range, att0, att1, att2, umbra, penumbra;
		};

		std::vector<SpotLightStruct> spotLightList;
	};


	struct CustomPropertiesStruct customProperties;
	*/

	DataStruct dataStruct;

	std::string cargoMesh, unpackedMesh;
	double spawnHeight, unpackedHeight, unpackedSize;
	VECTOR3 unpackedPMI, unpackedCS;

	bool released = false;
	bool attached = false;

	double containerMass = 0;
	bool enableFocus = false;

	void SetCargoCaps(bool init = true);
	void SetUnpackedCaps(bool init = true);

	std::string SetSpawnName();

	double timer = 0;
	bool landing = false;
	bool timing = false;

	void LoadConfig();
	// void SetCustomProperties(FILEHANDLE cfg);
	void ThrowWarning(const char* warning);
};