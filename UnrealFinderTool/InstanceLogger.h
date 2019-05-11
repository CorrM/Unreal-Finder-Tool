#pragma once
#include "Memory.h"
#include "JsonTypes.h"

struct GObject
{
	uintptr_t ObjAddress;
	int FNameIndexs[15];
	int OuterCount;
};

class GName
{
public:
	int Index;
	string AnsiName;
};

class InstanceLogger
{
	GObject* gObjObjects; int gObjectsCount;
	GName* gNames; int gNamesChunkCount, gNamesChunks;

	Memory* _memory;
	uintptr_t gObjectsAddress, gNamesAddress;

	std::string GetName(int fNameIndex, bool& success);
	std::string GetName(GObject obj);
	bool IsValidAddress(uintptr_t address);
	bool FetchData();
	bool ReadUObjectArray(uintptr_t address, JsonFUObjectArray& objectArray);
	bool ReadUObject(uintptr_t uObjectAddress, JsonUObject& uObject, GObject& retObj);
	bool ReadGNameArray(uintptr_t address);
	bool ObjectDump();
	bool NameDump();
public:
	InstanceLogger(Memory* memory, uintptr_t gObjObjectsAddress, uintptr_t gNamesAddress);
	~InstanceLogger();
	void Start();
};