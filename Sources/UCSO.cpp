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

void UCSO::clbkSetClassCaps(FILEHANDLE cfg)
{
	// Read the mass to add the container mass
	oapiReadItem_float(cfg, "Mass", dataStruct.resourceMass);

	if (!oapiReadItem_int(cfg, "Type", dataStruct.type)) { oapiWriteLog("UCSO Warning: Type not specifed."); return; }

	switch (dataStruct.type)
	{
	case RESOURCE:
		oapiReadItem_string(cfg, "ResourceType", &dataStruct.resourceType[0]);
		break;
	case UNPACKABLE:
		oapiReadItem_string(cfg, "SpawnModule", &dataStruct.spawnModule[0]);
		oapiReadItem_string(cfg, "SpawnName", &dataStruct.spawnName[0]);
		if (!oapiReadItem_float(cfg, "SpawnHeight", dataStruct.spawnHeight)) dataStruct.spawnHeight = 0;

		oapiReadItem_int(cfg, "UnpackMode", dataStruct.unpackMode);
		if(!oapiReadItem_int(cfg, "UnpackDelay", dataStruct.unpackDelay)) dataStruct.unpackDelay = 0;
		break;
	default:
		break;
	}

	SetSize(1.3);

	CreateAttachment(true, { 0, 0.65, 0 }, { 0, 1, 0 }, { 0, 0, 1 }, "UCSO");

	double stiffness = -((dataStruct.resourceMass + containerMass) * 9.80655) / (3 * -0.001);
	double damping = 0.9 * (2 * sqrt((dataStruct.resourceMass + containerMass) * stiffness));

	static TOUCHDOWNVTX tdvtx[4] = {
	{{ 1.125833, -0.65, -0.65 }, stiffness, damping, 3, 3},
	{{ 0, -0.65, 1.3 }, stiffness, damping, 3, 3},
	{{ -1.125833, -0.65, -0.65 }, stiffness, damping, 3, 3},
	{{ 0, 19.5, 0 }, stiffness, damping, 3, 3}
	};

	SetTouchdownPoints(tdvtx, 4);
}

void UCSO::LoadConfig()
{
	FILEHANDLE configFile = oapiOpenFile("UCSO_Config.cfg", FILE_IN_ZEROONFAIL, CONFIG);
	if (configFile) {
		if (!oapiReadItem_float(configFile, "ContainerMass", containerMass)) containerMass = 85;
		if (!oapiReadItem_bool(configFile, "DisplayInfo", displayInfo)) displayInfo = true;
		oapiCloseFile(configFile, FILE_IN_ZEROONFAIL);
	}
	else {
		oapiWriteLog("UCSO: Couldn't load the configurations file, will use the default configurations");
		containerMass = 85;
		displayInfo = true;
	}
}

void UCSO::clbkLoadStateEx(FILEHANDLE scn, void* status)
{
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
}

void UCSO::clbkPostCreation()
{
	SetEmptyMass(GetEmptyMass() + containerMass);
	isAttached = GetAttachmentStatus(GetAttachmentHandle(true, 0));
}

void UCSO::clbkPreStep(double simt, double simdt, double mjd)
{
	if (isAttached) {
		// Disable timing and landing if attached
		if (landing) landing = false;
		if (timing) { timer = 0; timing = false; }
	}
	else if (isReleased) {
		if (dataStruct.type == UNPACKABLE) {
			if (dataStruct.unpackMode == DELAYED) timing = true;
			else if (dataStruct.unpackMode == LANDED) landing = true;
		}
		isReleased = false;
	}

	if (landing) {
		// If the status is landed
		if (GroundContact()) { 
			UnpackCargo(); 
			landing = false; 
		}
	}
	else if (timing) {
		timer += simdt;
		if (timer >= dataStruct.unpackDelay) {
			UnpackCargo();
			timer = 0;
			timing = false;
		}
	}
}

void UCSO::clbkSaveState(FILEHANDLE scn)
{
	VESSEL4::clbkSaveState(scn);
	switch (dataStruct.type)
	{
	case RESOURCE:
		oapiWriteScenario_float(scn, "ResourceMass", GetMass() - containerMass);
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