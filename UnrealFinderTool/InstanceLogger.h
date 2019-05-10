#pragma once
#include "Memory.h"
#include "JsonReflector.h"

struct GObject
{
	uintptr_t ObjAddress;
	int FNameIndex;
};

class GName
{
public:
	int Index;
	string AnsiName;
};

class InstanceLogger
{
	nlohmann::json jsonCore;
	GObject* gObjObjects; int gObjectsCount;
	GName* gNames; int gNamesChunkCount, gNamesChunks;

	Memory* _memory;
	uintptr_t gObjectsAddress, gNamesAddress;

	std::string GetName(int fNameIndex, bool& success);
	bool IsValidAddress(uintptr_t address);
public:
	InstanceLogger(Memory* memory, uintptr_t gObjObjectsAddress, uintptr_t gNamesAddress);
	void Start();
	bool LoadCore();
	bool FetchData();
	bool ReadUObjectArray(uintptr_t address, JsonStruct& objectArray);
	bool ReadGNameArray(uintptr_t address);
	bool ObjectDump();
	bool NameDump();
};