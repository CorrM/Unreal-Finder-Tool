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
	uintptr_t gObjObjectsOffset;
	uintptr_t gNamesOffset;
	int ptrSize;

	std::string GetName(int fNameIndex, bool& success);
	bool ObjectDump();
	bool NameDump();
	bool ReadUObjectArray(uintptr_t address, JsonStruct& objectArray);
	bool ReadGNameArray(uintptr_t address);
	static DWORD BufToInteger(void* buffer);
	static DWORD64 BufToInteger64(void* buffer);
	bool IsValidAddress(uintptr_t address);
public:
	InstanceLogger(Memory* memory, uintptr_t gObjObjectsAddress, uintptr_t gNamesAddress);
	void Start();
	bool LoadCore();
	bool FetchData();
};