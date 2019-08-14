#include "UCSO.h"
#include <sstream>

DLLCLBK VESSEL* ovcInit(OBJHANDLE hvessel, int flightmodel)
{
	return new UCSO(hvessel, flightmodel);
}

DLLCLBK void ovcExit(VESSEL* vessel)
{
	if (vessel) delete (UCSO*)vessel;
}

UCSO::UCSO(OBJHANDLE hObj, int fmodel) : CargoVessel(hObj, fmodel)
{
}

void UCSO::clbkSetClassCaps(FILEHANDLE cfg)
{
	// Read the mass to add the container mass
	oapiReadItem_float(cfg, "Mass", dataStruct.resourceMass);

	if (!oapiReadItem_int(cfg, "Type", dataStruct.type)) { oapiWriteLog("UCSO Warning: Type not specifed."); return; }

	switch (dataStruct.type)
	{
	case STATIC:
		break;
	case RESOURCE:
		oapiReadItem_string(cfg, "ResourceType", &dataStruct.resourceType[0]);
		break;
	case UNPACKABLE:
		oapiReadItem_string(cfg, "SpawnModule", &dataStruct.spawnModule[0]);
		oapiReadItem_string(cfg, "SpawnName", &dataStruct.spawnName[0]);

		oapiReadItem_int(cfg, "UnpackMode", dataStruct.unpackMode);
		oapiReadItem_int(cfg, "UnpackDelay", dataStruct.unpackDelay);
		break;
	default:
		break;
	}
}

void UCSO::LoadConfig()
{
	FILEHANDLE configFile = oapiOpenFile("UCSOConfig.cfg", FILE_IN_ZEROONFAIL, CONFIG);
	if (configFile) {
		if (!oapiReadItem_float(configFile, "ContainerMass", containerMass)) containerMass = 85;
		if (!oapiReadItem_bool(configFile, "DisplayInfo", displayInfo)) displayInfo = true;
		oapiCloseFile(configFile, FILE_IN_ZEROONFAIL);
	}
	else {
		oapiWriteLog("UCSO: Couldn't load config file, using default config");
		containerMass = 85;
		displayInfo = true;
	}
}

void UCSO::clbkLoadStateEx(FILEHANDLE scn, void* status)
{
	LoadConfig();

	char* line;

	while (oapiReadScenario_nextline(scn, line)) {
		std::istringstream ss;
		ss.str(line);
		std::string data;

		if (ss >> data) {
			switch (dataStruct.type)
			{
			case RESOURCE:
				if (data == "ResourceMass") { 
					ss >> dataStruct.resourceMass;
					SetEmptyMass(dataStruct.resourceMass); 
				}
				else ParseScenarioLineEx(line, status);
				break;
			case UNPACKABLE:
				if (data == "Landing") ss >> landing;
				else if (data == "Timing") ss >> timing;
				else if (data == "Timer") ss >> timer;
				else ParseScenarioLineEx(line, status);
				break;
			default:
				ParseScenarioLineEx(line, status);
				break;
			}
		}
		else ParseScenarioLineEx(line, status);;
	}
	
	SetEmptyMass(GetEmptyMass() + containerMass);
}

void UCSO::clbkPostCreation()
{
	isAttached = GetAttachmentStatus(GetAttachmentHandle(true, 0));
}

void UCSO::clbkPreStep(double simt, double simdt, double mjd)
{
	if (isAttached) {
		// Disable timing and landing if attached
		if (landing) landing = false;
		if (timing) timing = false;
	}
	else if (isReleased) {
		if (dataStruct.type == UNPACKABLE) {
			if (dataStruct.unpackMode == DELAYED) {
				timer = simt;
				timing = true;
			}
			else if (dataStruct.unpackMode == LANDED) landing = true;
		}
		isReleased = false;
	}

	if (landing) {
		// If the status is landed
		if (GetFlightStatus() & 1) { 
			UnpackCargo(); 
			landing = false; 
		}
	}
	else if (timing && simt - timer >= dataStruct.unpackDelay) {
		UnpackCargo();
		timing = false;
	}
}

void UCSO::clbkSaveState(FILEHANDLE scn)
{
	VESSEL4::clbkSaveState(scn);
	switch (dataStruct.type)
	{
	case RESOURCE:
		oapiWriteScenario_float(scn, "ResourceMass", GetEmptyMass() - containerMass);
		break;
	case UNPACKABLE:
		oapiWriteScenario_int(scn, "Landing", landing);
		oapiWriteScenario_int(scn, "Timing", timing);
		oapiWriteScenario_float(scn, "Timer", timer);
		break;
	default:
		break;
	}
}