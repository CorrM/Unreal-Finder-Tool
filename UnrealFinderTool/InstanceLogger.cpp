#include "pch.h"
#include "PatternScan.h"
#include "ObjectsStore.h"
#include "NamesStore.h"
#include "EngineClasses.h"
#include "InstanceLogger.h"

#include <cinttypes>

InstanceLogger::InstanceLogger(const uintptr_t gObjObjectsAddress, const uintptr_t gNamesAddress) :
	gObjectsAddress(gObjObjectsAddress),
	gNamesAddress(gNamesAddress)
{
}

bool InstanceLogger::ObjectDump()
{
	FILE* log = nullptr;
	fopen_s(&log, "ObjectDump.txt", "w+");

	if (log == nullptr)
	{
		OutputDebugString("Can't open 'ObjectDump.txt' for write.");
		return false;
	}

	for (int i = 0; i < ObjectsStore().GetObjectsNum(); ++i)
	{
		if (ObjectsStore().GetByIndex(i).GetAddress() != NULL)
		{
			const UEObject& obj = ObjectsStore().GetByIndex(i);
			fprintf(log, "[%06i] %-100s 0x%" PRIXPTR "\n", int(i), obj.GetName().c_str(), obj.GetAddress());
		}
	}

	fclose(log);
	return true;
}

bool InstanceLogger::NameDump()
{
	FILE* log = nullptr;
	fopen_s(&log, "NameDump.txt", "w+");

	if (log == nullptr)
	{
		OutputDebugString("Can't open 'NameDump.txt' for write.");
		return false;
	}

	for (int i = 0; i < NamesStore().GetNamesNum(); i++)
	{
		std::string str = NamesStore().GetByIndex(i);
		if (!str.empty())
			fprintf(log, "[%06i] %s\n", int(i), NamesStore().GetByIndex(i).c_str());
	}

	fclose(log);
	return true;
}

LoggerRetState InstanceLogger::Start()
{
	LoggerState s = FetchData();
	if (s != LoggerState::Good)
		return { s, 0, 0 };

	NameDump();
	ObjectDump();

	return { LoggerState::Good, ObjectsStore().GetObjectsNum(), NamesStore().GetNamesNum() };
}

LoggerState InstanceLogger::FetchData()
{
	if (!Utils::IsValidGNamesAddress(gNamesAddress))
		return LoggerState::BadGNameAddress;

	if (!Utils::IsValidGObjectsAddress(gObjectsAddress))
		return LoggerState::BadGObjectAddress;

	// GNames
	if (!NamesStore::Initialize(gNamesAddress, false))
		return LoggerState::BadGName;

	// GObjects
	if (!ObjectsStore::Initialize(gObjectsAddress, false))
		return LoggerState::BadGObject;

	return LoggerState::Good;
}