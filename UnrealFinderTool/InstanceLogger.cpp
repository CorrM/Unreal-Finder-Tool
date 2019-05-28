#include "pch.h"
#include "Utils.h"
#include "PatternScan.h"

#include <iostream>
#include <string>
#include <cinttypes>

#include "InstanceLogger.h"

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
	string retStr;
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
		std::cout << red << "[*] " << def << "Can't open 'ObjectDump.txt' for write." << std::endl;
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
		std::cout << red << "[*] " << def << "Can't open 'NameDump.txt' for write." << std::endl;
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
	if (!ReadUObjectArray(gObjectsAddress))
	{
		std::cout << red << "[*] " << def << "Invalid GObject Address." << std::endl << def;
		return LoggerState::BadGObject;
	}

	// GNames
	if (!ReadGNameArray(gNamesAddress))
	{
		std::cout << red << "[*] " << def << "Invalid GNames Address." << std::endl << def;
		return LoggerState::BadGName;
	}

	return LoggerState::Good;
}

bool InstanceLogger::ReadUObjectArray(const uintptr_t address)
{
	// Get Work Method [Pointer Next Pointer or FUObjectItem(Flags, ClusterIndex, etc)]
	bool isPointerNextPointer = false;
	uintptr_t obj1 = Utils::MemoryObj->ReadAddress(address);
	uintptr_t obj2 = Utils::MemoryObj->ReadAddress(address + Utils::PointerSize());

	if (!Utils::IsValidAddress(Utils::MemoryObj, obj1)) return false;
	// Not Valid mean it's not Pointer Next To Pointer, or GObject address is wrong.
	isPointerNextPointer = Utils::IsValidAddress(Utils::MemoryObj, obj2);

	return isPointerNextPointer ? ReadUObjectArrayPnP(address) : ReadUObjectArrayNormal(address);
}

bool InstanceLogger::ReadUObjectArrayPnP(const uintptr_t address)
{
	JsonStruct uObject;
	int skipCount = 0;

	for (size_t uIndex = 0; skipCount <= maxZeroAddress; ++uIndex)
	{
		uintptr_t curAddress = address + uIndex * Utils::PointerSize();
		uintptr_t dwUObject = Utils::MemoryObj->ReadAddress(curAddress);

		// Skip null pointer in GObjects array
		if (dwUObject == 0)
		{
			++skipCount;
			continue;
		}

		auto curObject = std::make_unique<GObject>();
		ReadUObject(dwUObject, uObject, *curObject);

		gObjObjects.push_back(std::move(curObject));
		++gObjectsCount;
		skipCount = 0;
	}
	return true;
}

bool InstanceLogger::ReadUObjectArrayNormal(const uintptr_t address)
{
	JsonStruct fUObjectItem, uObject;
	int skipCount = 0;
	uintptr_t curAddress = 0;

	for (size_t uIndex = 0; skipCount <= maxZeroAddress; ++uIndex)
	{
		curAddress = address + uIndex * (fUObjectItem.StructSize - fUObjectItem.SubUnNeededSize());

		// Read the address as struct
		if (!fUObjectItem.ReadData(curAddress, "FUObjectItem")) return false;
		auto dwUObject = fUObjectItem["Object"].ReadAs<uintptr_t>();

		// Skip null pointer in GObjects array
		if (dwUObject == 0)
		{
			++skipCount;
			continue;
		}

		auto curObject = std::make_unique<GObject>();
		ReadUObject(dwUObject, uObject, *curObject);

		gObjObjects.push_back(std::move(curObject));
		++gObjectsCount;
		skipCount = 0;
	}
	return true;
}

bool InstanceLogger::ReadUObject(const uintptr_t uObjectAddress, JsonStruct & uObject, GObject & retObj)
{
	int i = 0;
	uintptr_t outer = uObjectAddress;
	do
	{
		// Alloc and Read the address as class
		if (!uObject.ReadData(outer, "UObject")) return false;

		outer = uObject["Outer"].ReadAs<uintptr_t>();
		retObj.FNameIndexs[i] = uObject["Name"]["Index"].ReadAs<int>();
		i++;
	} while (outer != NULL);

	retObj.ObjAddress = uObjectAddress;
	retObj.OuterCount = i - 1;
	return true;
}

bool InstanceLogger::ReadGNameArray(const uintptr_t address)
{
	int ptrSize = Utils::PointerSize();
	size_t nameOffset = 0;
	JsonStruct fName;

	// Get GNames Chunks
	std::vector<uintptr_t> gChunks;
	for (int i = 0; i < 15; ++i)
	{
		uintptr_t addr;
		const int offset = ptrSize * i;

		addr = Utils::MemoryObj->ReadAddress(address + offset);

		if (!Utils::IsValidAddress(Utils::MemoryObj, addr)) break;
		gChunks.push_back(addr);
		++gNamesChunks;
	}

	// Alloc Size
	gNames = new GName[gNamesChunkCount * gNamesChunks];

	// Calc AnsiName offset
	{
		uintptr_t noneNameAddress = Utils::MemoryObj->ReadAddress(gChunks[0]);

		Pattern none_sig = PatternScan::Parse("None", 0, "4E 6F 6E 65 00", 0xFF);
		auto result = PatternScan::FindPattern(Utils::MemoryObj, noneNameAddress, noneNameAddress + 0x100, { none_sig }, true);
		auto it = result.find("None");
		if (it != result.end() && !it->second.empty())
			nameOffset = it->second[0] - noneNameAddress;
	}

	// Dump GNames
	int i = 0;
	for (uintptr_t chunkAddress : gChunks)
	{
		for (int j = 0; j < gNamesChunkCount; ++j)
		{
			const int offset = ptrSize * j;
			uintptr_t fNameAddress = Utils::MemoryObj->ReadAddress(chunkAddress + offset);

			if (!Utils::IsValidAddress(Utils::MemoryObj, fNameAddress))
			{
				gNames[i].Index = i;
				gNames[i].AnsiName = "";
				++i;
				continue;
			}

			// Read FName
			if (!fName.ReadData(fNameAddress, "FNameEntity")) return false;

			// Set The Name
			gNames[i].Index = i;
			gNames[i].AnsiName = Utils::MemoryObj->ReadText(fNameAddress + nameOffset);
			++i;
		}
	}

	return true;
}