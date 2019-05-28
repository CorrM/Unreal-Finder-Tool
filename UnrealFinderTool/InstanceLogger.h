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

enum class LoggerState
{
	Good,
	BadGObject,
	BadGName
};

struct LoggerRetState
{
	LoggerState State;
	int GObjectsCount;
	int GNamesCount;
};

class InstanceLogger
{
	std::vector<std::unique_ptr<GObject>> gObjObjects; 
	int gObjectsCount, maxZeroAddress;

	GName* gNames;
	int gNamesChunkCount, gNamesChunks;

	uintptr_t gObjectsAddress, gNamesAddress;

	std::string GetName(int fNameIndex, bool& success);
	std::string GetName(GObject obj);
	LoggerState FetchData();
	bool ReadUObjectArray(uintptr_t address);
	bool ReadUObjectArrayPnP(uintptr_t address);
	bool ReadUObjectArrayNormal(uintptr_t address);
	bool ReadUObject(uintptr_t uObjectAddress, JsonStruct& uObject, GObject& retObj);
	bool ReadGNameArray(uintptr_t address);
	bool ObjectDump();
	bool NameDump();
public:
	InstanceLogger(uintptr_t gObjObjectsAddress, uintptr_t gNamesAddress);
	LoggerRetState Start();
};