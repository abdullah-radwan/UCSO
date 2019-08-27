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

void UCSO::LoadConfig()
{
	FILEHANDLE configFile = oapiOpenFile("UCSO_Config.cfg", FILE_IN_ZEROONFAIL, CONFIG);

	if (configFile)
	{
		if (!oapiReadItem_float(configFile, "ContainerMass", containerMass)) containerMass = 85;

		if (!oapiReadItem_bool(configFile, "EnableFocus", enableFocus)) enableFocus = false;

		oapiCloseFile(configFile, FILE_IN_ZEROONFAIL);
	}
	else
	{
		oapiWriteLog("UCSO: Couldn't load the configurations file, will use the default configurations");

		containerMass = 85;
		enableFocus = false;
	}
}

void UCSO::clbkSetClassCaps(FILEHANDLE cfg)
{
	if (!oapiReadItem_string(cfg, "CargoMesh", &cargoMesh[0])) ThrowWarning("mesh");

	// Read the mass to add the container mass
	if (!oapiReadItem_float(cfg, "Mass", dataStruct.netMass)) ThrowWarning("mass");

	if (!oapiReadItem_int(cfg, "Type", dataStruct.type)) ThrowWarning("type");

	switch (dataStruct.type)
	{
	case RESOURCE:
		if (!oapiReadItem_string(cfg, "ResourceType", &dataStruct.resourceType[0])) ThrowWarning("resource type");

		break;
	case UNPACKABLE:
		if (!oapiReadItem_int(cfg, "UnpackType", dataStruct.unpackType)) ThrowWarning("unpack type");

		if (dataStruct.unpackType == UCSO_MODULE)
		{
			if (!oapiReadItem_string(cfg, "UnpackedMesh", &unpackedMesh[0])) ThrowWarning("unpacked mesh");

			if (!oapiReadItem_float(cfg, "UnpackedSize", unpackedSize)) ThrowWarning("unpacked size");

			if (oapiReadItem_float(cfg, "UnpackedHeight", unpackedHeight)) unpackedHeight = abs(unpackedHeight);
			else unpackedHeight = 0;

			if (!oapiReadItem_vec(cfg, "UnpackedPMI", unpackedPMI)) unpackedPMI = { 0,0,0 };

			if (!oapiReadItem_vec(cfg, "UnpackedCS", unpackedCS)) unpackedCS = { 0,0,0 };
		}
		else if (dataStruct.unpackType == ORBITER_VESSEL)
		{
			if (!oapiReadItem_string(cfg, "SpawnName", &dataStruct.spawnName[0])) ThrowWarning("spawn name");

			if (!oapiReadItem_string(cfg, "SpawnModule", &dataStruct.spawnModule[0])) ThrowWarning("spawn module");

			if (!oapiReadItem_int(cfg, "UnpackMode", dataStruct.unpackMode)) ThrowWarning("unpack mode");

			if (!oapiReadItem_int(cfg, "UnpackDelay", dataStruct.unpackDelay)) dataStruct.unpackDelay = 0;

			if (oapiReadItem_float(cfg, "SpawnHeight", spawnHeight)) spawnHeight = abs(spawnHeight);
			else spawnHeight = 0;
		}

		break;
	default:
		break;
	}

	SetEnableFocus(enableFocus);

	CreateAttachment(true, { 0, -0.65, 0 }, { 0, 1, 0 }, { 0, 0, 1 }, "UCSO");

	SetCargoCaps(false);
}

void UCSO::ThrowWarning(std::string warning)
{
	std::string warningMessage = "UCSO Warning: ";
	warningMessage += GetClassNameA();
	warningMessage += " cargo ";
	warningMessage += warning;
	warningMessage += " not specified.";

	oapiWriteLog(&warningMessage[0]);

	throw;
}

void UCSO::clbkLoadStateEx(FILEHANDLE scn, void* status)
{
	char* line;

	while (oapiReadScenario_nextline(scn, line))
	{
		std::istringstream ss;
		ss.str(line);
		std::string data;

		if (ss >> data) 
		{
			switch (dataStruct.type)
			{
			case RESOURCE:
				if (data == "ResourceMass") 
				{ 
					ss >> dataStruct.netMass;
					SetEmptyMass(dataStruct.netMass);
				}
				else ParseScenarioLineEx(line, status);

				break;
			case UNPACKABLE:
				switch (dataStruct.unpackType)
				{
				case UCSO_MODULE:
					if (data == "Unpacked") ss >> dataStruct.unpacked;
					else ParseScenarioLineEx(line, status);

					if (dataStruct.unpacked) SetUnpackedCaps(false);

					break;
				case ORBITER_VESSEL:
					if (data == "Landing") ss >> landing;
					else if (data == "Timing") ss >> timing;
					else if (data == "Timer") ss >> timer;
					else ParseScenarioLineEx(line, status);

					break;
				default:
					ParseScenarioLineEx(line, status);
					break;
				}
				break;
			default:
				ParseScenarioLineEx(line, status);
				break;
			}
		}
		else ParseScenarioLineEx(line, status);
	}

}

void UCSO::clbkPostCreation()
{
	if(!dataStruct.unpacked) SetEmptyMass(dataStruct.netMass + containerMass);
	attached = GetAttachmentStatus(GetAttachmentHandle(true, 0));
}

void UCSO::clbkPreStep(double simt, double simdt, double mjd)
{
	if (!(GetFlightStatus() & 1) && GroundContact()) 
	{
		VESSELSTATUS2 status;
		memset(&status, 0, sizeof(status));
		status.version = 2;
		GetStatusEx(&status);

		status.status = 1;
		SetGroundRotation(status, dataStruct.unpacked ? unpackedHeight : 0.65);

		DefSetStateEx(&status);
	}

	if (dataStruct.type != UNPACKABLE || dataStruct.unpackType != ORBITER_VESSEL) return;

	if (attached)
	{
		if (landing) landing = false;
		if (timing) { timer = 0; timing = false; }
	}
	else if (released)
	{
		if (dataStruct.unpackMode == DELAYED) timing = true;
		else if (dataStruct.unpackMode == LANDED) landing = true;

		released = false;
	}

	if (landing && GroundContact())
	{
		UnpackCargo();
		landing = false;
	}
	else if (timing) 
	{
		timer += simdt;
		if (timer >= dataStruct.unpackDelay) 
		{
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
		oapiWriteScenario_float(scn, "ResourceMass", dataStruct.netMass);

		break;
	case UNPACKABLE:
		if (dataStruct.unpackType == UCSO_MODULE) oapiWriteScenario_int(scn, "Unpacked", dataStruct.unpacked);
		else if (dataStruct.unpackType == ORBITER_VESSEL) 
		{
			oapiWriteScenario_int(scn, "Landing", landing);
			oapiWriteScenario_int(scn, "Timing", timing);
			oapiWriteScenario_float(scn, "Timer", timer);
		}

		break;
	default:
		break;
	}
}

