#pragma once
#include <string>

enum CargoType {
	STATIC = 0,
	RESOURCE,
	UNPACKABLE
};

struct DataStruct {
	DataStruct() { type = -1; resourceMass = 0; unpackDelay = 0; unpackMode = -1; };
	int type;

	std::string resourceType;
	double resourceMass;

	std::string spawnName;
	std::string spawnModule;
	int unpackMode;
	int unpackDelay;
};

