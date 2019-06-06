#include "pch.h"
#include "PatternScan.h"
#include "ObjectsStore.h"
#include "NamesStore.h"
#include "EngineClasses.h"
#include "InstanceLogger.h"

#include <string>
#include <cinttypes>

InstanceLogger::InstanceLogger(const uintptr_t gObjObjectsAddress, const uintptr_t gNamesAddress) :
	gObjectsCount(0), maxZeroAddress(150),
	gNames(nullptr), gNamesChunkCount(16384), gNamesChunks(0),
	gObjectsAddress(gObjObjectsAddress), gNamesAddress(gNamesAddress)
{
}

std::string InstanceLogger::GetName(const int fNameIndex, bool& success)
{
	if (fNameIndex < 0 || fNameIndex > gNamesChunkCount * gNamesChunks)
	{
		success = false;
		static char ret[256];
		sprintf_s(ret, "INVALID NAME INDEX : %i > %i", fNameIndex, gNamesChunkCount * gNamesChunks);
		return ret;
	}
	success = true;
	return gNames[fNameIndex].AnsiName;
}

std::string InstanceLogger::GetName(GObject obj)
{
	std::string retStr;
	for (int i = obj.OuterCount; i >= 0; --i)
	{
		int fNameIndex = obj.FNameIndexs[i];
		retStr.append(gNames[fNameIndex].AnsiName);
		if (i != 0 && !gNames[fNameIndex].AnsiName.empty()) retStr.append(".");
	}

	auto pos = retStr.rfind('/');
	if (pos == std::string::npos)
		return retStr;

	return retStr.substr(pos + 1);
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

	for (int i = 0; i < gObjectsCount; ++i)
	{
		if (gObjObjects[i]->ObjAddress != NULL)
			fprintf(log, "[%06i] %-80s 0x%" PRIXPTR "\n", int(i), GetName(*gObjObjects[i]).c_str(), gObjObjects[i]->ObjAddress);
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

	for (int i = 0; i < gNamesChunkCount * gNamesChunks; i++)
		fprintf(log, "[%06i] %s\n", int(i), gNames[i].AnsiName.c_str());

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

	delete[] gNames;
	return { LoggerState::Good, gObjectsCount, gNamesChunkCount * gNamesChunks };
}

LoggerState InstanceLogger::FetchData()
{
	if (!Utils::IsValidGNamesAddress(gNamesAddress))
		return LoggerState::BadGName;

	if (!Utils::IsValidGObjectsAddress(gObjectsAddress))
		return LoggerState::BadGObject;

	// GObjects
	if (!ObjectsStore::Initialize(gObjectsAddress, false))
		return LoggerState::BadGObject;

	// GNames
	if (!NamesStore::Initialize(gNamesAddress, false))
		return LoggerState::BadGName;

	return LoggerState::Good;
}