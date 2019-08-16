#pragma once
#pragma warning( disable : 26495 )
#include <string>

enum CargoType {
	STATIC = 0,
	RESOURCE,
	UNPACKABLE
};

enum UnpackMode {
	LANDED = 0,
	DELAYED,
	MANUAL
};

struct DataStruct {
	int type;

	std::string resourceType;
	double resourceMass;

	std::string spawnName;
	std::string spawnModule;
	int unpackMode;
	int unpackDelay;
};

