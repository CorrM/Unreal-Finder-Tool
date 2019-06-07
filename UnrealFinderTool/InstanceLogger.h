#pragma once

class UObject;

enum class LoggerState
{
	Good,
	BadGObjectAddress,
	BadGNameAddress,
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
	uintptr_t gObjectsAddress, gNamesAddress;

	LoggerState FetchData();
	bool ObjectDump();
	bool NameDump();
public:
	InstanceLogger(uintptr_t gObjObjectsAddress, uintptr_t gNamesAddress);
	LoggerRetState Start();
};