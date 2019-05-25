#pragma once
#include "JsonReflector.h"

struct GObject
{
	uintptr_t ObjAddress;
	int FNameIndexs[20];
	int OuterCount;
};

class GName
{
public:
	int Index;
	std::string AnsiName;
};

class InstanceLogger
{
	GObject* gObjObjects; int gObjectsCount;
	GName* gNames; int gNamesChunkCount, gNamesChunks;

	uintptr_t gObjectsAddress, gNamesAddress;

	std::string GetName(int fNameIndex, bool& success);
	std::string GetName(GObject obj);
	bool FetchData();
	bool ReadUObjectArray(uintptr_t address, JsonStruct& objectArray);
	bool ReadUObject(uintptr_t uObjectAddress, JsonStruct& uObject, GObject& retObj);
	bool ReadGNameArray(uintptr_t address);
	bool ObjectDump();
	bool NameDump();
public:
	InstanceLogger(uintptr_t gObjObjectsAddress, uintptr_t gNamesAddress);
	~InstanceLogger();
	void Start();
};