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
	// Open the config file
	FILEHANDLE configFile = oapiOpenFile("UCSO_Config.cfg", FILE_IN_ZEROONFAIL, CONFIG);

	// If the file is opened
	if (configFile)
	{
		if (!oapiReadItem_float(configFile, "ContainerMass", containerMass)) containerMass = 85;

		if (!oapiReadItem_bool(configFile, "EnableFocus", enableFocus)) enableFocus = false;

		oapiCloseFile(configFile, FILE_IN_ZEROONFAIL);
	}
	else
	{
		// Write in log and set the default configuration
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

			if (!oapiReadItem_vec(cfg, "UnpackedPMI", unpackedPMI)) unpackedPMI = { -99,-99,-99 };

			if (!oapiReadItem_vec(cfg, "UnpackedCS", unpackedCS)) unpackedCS = { -99,-99,-99 };

			// SetCustomProperties(cfg);
		}
		else if (dataStruct.unpackType == ORBITER_VESSEL)
		{
			if (!oapiReadItem_string(cfg, "SpawnName", &dataStruct.spawnName[0])) ThrowWarning("spawn name");
			else dataStruct.spawnName += "1";

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

	// Create attachment here to avoid NULL in clbkPostCreation
	CreateAttachment(true, { 0, -0.65, 0 }, { 0, 1, 0 }, { 0, 0, 1 }, "UCSO");

	SetCargoCaps(false);
}

/* Disabled until a solution is found to light the ground
void UCSO::SetCustomProperties(FILEHANDLE cfg)
{
	std::string customPropertiesString;

	if (oapiReadItem_string(cfg, "CustomProperties", &customPropertiesString[0]))
	{
		if (customPropertiesString.find("SpotLight") == std::string::npos)
		{
			int count;

			if (!oapiReadItem_int(cfg, "SpotLightCount", count) || count < 1) ThrowWarning("spotlight count");

			for (int index = 0; index < count; index++)
			{
				std::string spotLight = "SpotLight" + std::to_string(index);

				customProperties.spotLightList.push_back(CustomPropertiesStruct::SpotLightStruct());

				auto& spotLightStruct = customProperties.spotLightList[index];
				
				if (!oapiReadItem_vec(cfg, &(spotLight + "Pos")[0], spotLightStruct.pos)) ThrowWarning("spotlight position");

				if (!oapiReadItem_vec(cfg, &(spotLight + "Dir")[0], spotLightStruct.dir)) ThrowWarning("spotlight direction");

				if (!oapiReadItem_float(cfg, &(spotLight + "Range")[0], spotLightStruct.range)) ThrowWarning("spotlight range");

				if (!oapiReadItem_float(cfg, &(spotLight + "Att0")[0], spotLightStruct.att0)) 
					ThrowWarning("spotlight attenuation coefficient 0");

				if (!oapiReadItem_float(cfg, &(spotLight + "Att1")[0], spotLightStruct.att1)) 
					ThrowWarning("spotlight attenuation coefficient 1");

				if (!oapiReadItem_float(cfg, &(spotLight + "Att2")[0], spotLightStruct.att2)) 
					ThrowWarning("spotlight attenuation coefficient 2");

				if (!oapiReadItem_float(cfg, &(spotLight + "Umbra")[0], spotLightStruct.umbra)) ThrowWarning("spotlight umbra");

				if (!oapiReadItem_float(cfg, &(spotLight + "Penumbra")[0], spotLightStruct.penumbra)) ThrowWarning("spotlight penumbra");

				spotLightStruct.valid = true;

				spotLightStruct.spotLight = static_cast<SpotLight*>(AddSpotLight(spotLightStruct.pos, spotLightStruct.dir,
						spotLightStruct.range, spotLightStruct.att0, spotLightStruct.att1, spotLightStruct.att2,
						spotLightStruct.umbra, spotLightStruct.penumbra, { 1,1,1,0 }, { 1,1,1,0 }, { 0,0,0,0 }));

				spotLightStruct.color = { 1,1,1 };

				spotLightStruct.beaconSpec = { 
					BEACONSHAPE_STAR,
					&spotLightStruct.pos,
					&spotLightStruct.color,
					2, 0.3, 0, 0, 0,
					true
				};

				AddBeacon(&spotLightStruct.beaconSpec);
			}
		}
	}
}
*/

void UCSO::ThrowWarning(const char* warning)
{
	oapiWriteLogV("UCSO Warning: %s cargo %s not specified,", GetClassNameA(), warning);

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
	// If not unpacked, set the empty 
	if(!dataStruct.unpacked) SetEmptyMass(dataStruct.netMass + containerMass);

	attached = GetAttachmentStatus(GetAttachmentHandle(true, 0));
}

void UCSO::clbkPreStep(double simt, double simdt, double mjd)
{
	// If not landed but contacted the ground
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

	// Don't continue if not unpackable or UCSO module
	if (dataStruct.type != UNPACKABLE || dataStruct.unpackType != ORBITER_VESSEL) return;

	// Cancel the landing and timing if attached
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

	// If landing flag is on and contacted the ground
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

void UCSO::SetAttachmentState(bool attached)
{
	// released is necessary (!attached alone won't work) because the vessel maybe unattached and not released
	released = !attached;
	this->attached = attached;
}

DataStruct UCSO::GetDataStruct() const
{
	return dataStruct;
}

void UCSO::PackCargo()
{
	dataStruct.unpacked = false;

	SetCargoCaps();
}

bool UCSO::UnpackCargo()
{
	if (dataStruct.unpackType == UCSO_MODULE)
	{
		dataStruct.unpacked = true;
		SetUnpackedCaps();
		return true;
	}

	VESSELSTATUS2 status;
	memset(&status, 0, sizeof(status));
	status.version = 2;
	GetStatusEx(&status);

	// Set the ground rotation if attached
	if (status.status) SetGroundRotation(status, spawnHeight);

	OBJHANDLE cargoHandle = oapiCreateVesselEx(SetSpawnName().c_str(), dataStruct.spawnModule.c_str(), &status);

	// If the cargo wan't unpacked
	if (!cargoHandle) return false;

	// Delete the cargo and move the camera to the unpacked one
	oapiDeleteVessel(GetHandle(), cargoHandle);

	return true;
}

double UCSO::UseResource(double requiredMass)
{
	double drainedMass;

	// If the cargo net mass is lower than or equal to the required mass
	if (dataStruct.netMass - requiredMass >= 0) drainedMass = requiredMass;
	// If the required mass is higher than the available mass, use the full mass
	else drainedMass = dataStruct.netMass;

	dataStruct.netMass -= drainedMass;
	SetEmptyMass(dataStruct.netMass + containerMass);

	return drainedMass;
}

void UCSO::SetCargoCaps(bool init)
{
	// Don't proceed if unpacked
	if (dataStruct.unpacked) return;

	VESSELSTATUS2 status;

	// If Orbiter is initiated, as if not, status will cause CTD.
	if (init)
	{
		memset(&status, 0, sizeof(status));
		status.version = 2;
		GetStatusEx(&status);

		SetEmptyMass(dataStruct.netMass + containerMass);

		// The attachment is already created in clbkSetClassCaps, so create it only if the sim initiated
		// Which means the cargo was unpacked
		ClearAttachments();
		CreateAttachment(true, { 0, -0.65, 0 }, { 0, 1, 0 }, { 0, 0, 1 }, "UCSO");
	}

	// Replace the unpacked mesh with the cargo mesh
	InsertMesh(cargoMesh.c_str(), 0);

	SetSize(0.65);

	SetPMI({ 0.28, 0.28, 0.28 });

	SetCrossSections({ 1.69, 1.69, 1.69 });

	double stiffness = -((dataStruct.netMass + containerMass) * 9.80655) / -0.001;
	double damping = 0.9 * (2 * sqrt((dataStruct.netMass + containerMass) * stiffness));

	TOUCHDOWNVTX tdvtx[4] = {
	{{ 0.5629, -0.65, -0.325 }, stiffness, damping, 3, 3},
	{{ 0, -0.65, 0.65 }, stiffness, damping, 3, 3},
	{{ -0.5629, -0.65, -0.325 }, stiffness, damping, 3, 3},
	{{ 0, 9.75, 0 }, stiffness, damping, 3, 3}
	};

	SetTouchdownPoints(tdvtx, 4);

	// for (auto& lightSpot : customProperties.spotLightList) if (lightSpot.valid) lightSpot.spotLight->Activate(false);

	// If the Orbiter is initiated and the cargo is landed
	if (init && status.status)
	{
		SetGroundRotation(status, 0.65);

		DefSetStateEx(&status);
	}
}

void UCSO::SetUnpackedCaps(bool init)
{
	VESSELSTATUS2 status;

	if (init)
	{
		memset(&status, 0, sizeof(status));
		status.version = 2;
		GetStatusEx(&status);
	}

	ClearAttachments();
	CreateAttachment(true, { 0, -unpackedHeight, 0 }, { 0, 1, 0 }, { 0, 0, 1 }, "UCSO");

	InsertMesh(unpackedMesh.c_str(), 0);

	SetSize(unpackedSize);

	SetEmptyMass(dataStruct.netMass);

	// If the unpacked PMI is set
	if (unpackedPMI.x != -99 && unpackedPMI.y != -99 && unpackedPMI.z != -99) SetPMI(unpackedPMI);

	// If the unpacked cross sections is set
	if (unpackedCS.x != -99 && unpackedCS.y != -99 && unpackedCS.z != -99) SetCrossSections(unpackedCS);

	double stiffness = -(dataStruct.netMass * 9.80655) / -0.001;
	double damping = 0.9 * (2 * sqrt(dataStruct.netMass * stiffness));

	TOUCHDOWNVTX tdvtx[4] = {
	{ { cos(30 * RAD) * unpackedSize, -unpackedHeight, -sin(30 * RAD) * unpackedSize }, stiffness, damping, 3, 3},
	{ { 0, -unpackedHeight, unpackedSize }, stiffness, damping, 3, 3},
	{ { -cos(30 * RAD) * unpackedSize, -unpackedHeight, -sin(30 * RAD) * unpackedSize }, stiffness, damping, 3, 3},
	{ { 0, 15 * unpackedSize, 0 }, stiffness, damping, 3, 3}
	};

	SetTouchdownPoints(tdvtx, 4);

	// for (auto& lightSpot : customProperties.spotLightList) if (lightSpot.valid) lightSpot.spotLight->Activate(true);

	if (init && status.status)
	{
		SetGroundRotation(status, unpackedHeight);

		DefSetStateEx(&status);
	}
}

std::string UCSO::SetSpawnName()
{
	// If the initial spawn name doesn't exists, use it
	if (!oapiGetVesselByName(&dataStruct.spawnName[0])) return dataStruct.spawnName;

	for (int index = 1; index++;)
	{
		// Add the index to the string. .c_str() is used to avoid a bug
		std::string spawnName = dataStruct.spawnName.c_str() + std::to_string(index);
		// If the spawn name doesn't exists
		if (!oapiGetVesselByName(&spawnName[0])) return spawnName;
	}

	return dataStruct.spawnName; // Useless, as the above loop is infinte, but to avoid compiler warnings
}

void UCSO::clbkSaveState(FILEHANDLE scn)
{
	// Set the default state
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

