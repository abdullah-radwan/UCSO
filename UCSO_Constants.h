#include <string>

enum CargoType {
	STATIC = 0,
	RESOURCE,
	UNPACKABLE
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

