#include "pch.h"
#include "InstanceLogger.h"
#include "Utils.h"
#include <iostream>
#include <string>
#include <cinttypes>

InstanceLogger::InstanceLogger(Memory* memory, const uintptr_t gObjObjectsAddress, const uintptr_t gNamesAddress) :
	gObjObjects(nullptr), gObjectsCount(0),
	gNames(nullptr), gNamesChunkCount(16384), gNamesChunks(0),
	_memory(memory),
	gObjectsAddress(gObjObjectsAddress),
	gNamesAddress(gNamesAddress)
{
}

InstanceLogger::~InstanceLogger()
{
	delete[] gObjObjects;
	delete[] gNames;
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
		if (gObjObjects[i].ObjAddress != NULL)
			fprintf(log, "[%06i] %-80s 0x%" PRIXPTR "\n", int(i), GetName(gObjObjects[i]).c_str(), gObjObjects[i].ObjAddress);
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

void InstanceLogger::Start()
{
	if (!FetchData())
		return;

	NameDump();
	ObjectDump();

	std::cout << std::endl;
	std::cout << green << "[+] " << purple << "Found [ " << green << gObjectsCount << purple << " ] Object." << std::endl;
	std::cout << green << "[+] " << purple << "Found [ " << green << gNamesChunkCount * gNamesChunks << purple << " ] Name." << std::endl;
}

bool InstanceLogger::FetchData()
{
	// GObjects
	JsonFUObjectArray jsonGObjectsReader;
	if (!JsonReflector::ReadStruct("FUObjectArray", jsonGObjectsReader))
	{
		std::cout << red << "[*] " << def << "Can't find struct `FUObjectArray`." << std::endl << def;
		return false;
	}
	if (!ReadUObjectArray(gObjectsAddress, jsonGObjectsReader))
	{
		std::cout << red << "[*] " << def << "Invalid GObject Address." << std::endl << def;
		return false;
	}

	// GNames
	if (!ReadGNameArray(gNamesAddress))
	{
		std::cout << red << "[*] " << def << "Invalid GNames Address." << std::endl << def;
		return false;
	}

	return true;
}

bool InstanceLogger::ReadUObjectArray(const uintptr_t address, JsonFUObjectArray& objectArray)
{
	const size_t sSub = _memory->Is64Bit && Utils::ProgramIs64() ? 0x0 : 0x4;
	JsonFUObjectItem fUObjectItem;
	JsonUObject uObject;

	if (_memory->ReadBytes(address, objectArray.GetAllocPointer(), objectArray.StructSize - sSub) == 0) return false;

	auto objObjects = objectArray.ObjObjects();
	int num = objObjects.NumElements();

	// Fix objObjects
	Utils::FixPointersInStruct(&objObjects, _memory->Is64Bit);

	auto dwFUObjectAddress = objObjects.Objects();

	// Alloc all objects
	gObjObjects = new GObject[num];
	ZeroMemory(gObjObjects, sizeof(GObject) * num);

	uintptr_t currentFUObjAddress = dwFUObjectAddress;
	for (int i = 0; i < num; ++i)
	{
		// Read the address as struct
		if (_memory->ReadBytes(currentFUObjAddress, fUObjectItem.GetAllocPointer(), fUObjectItem.StructSize - sSub) == 0)
			return false;

		// Fix fUObjectItem
		Utils::FixPointersInStruct(&fUObjectItem, _memory->Is64Bit);

		// Convert class pointer to address
		auto dwUObject = fUObjectItem.Object();

		// Skip null pointer in GObjects array
		if (dwUObject == 0)
		{
			currentFUObjAddress += fUObjectItem.StructSize - (sSub * 2);
			gObjectsCount++;
			continue;
		}

		ReadUObject(dwUObject, uObject, gObjObjects[i]);

		currentFUObjAddress += fUObjectItem.StructSize - (sSub * 2);
		gObjectsCount++;
	}
	return true;
}

bool InstanceLogger::ReadUObject(const uintptr_t uObjectAddress, JsonUObject& uObject, GObject& retObj)
{
	const size_t sSub = _memory->Is64Bit && Utils::ProgramIs64() ? 0x0 : 0x4;

	int i = 0;
	uintptr_t outer = uObjectAddress;
	do
	{
		// Alloc and Read the address as class
		if (_memory->ReadBytes(outer, uObject.GetAllocPointer(), uObject.StructSize - sSub) == 0)
			return false;

		// Fix UObject
		Utils::FixPointersInStruct(&uObject, _memory->Is64Bit);

		outer = uObject["Outer"].ReadAs<uintptr_t>();
		retObj.FNameIndexs[i] = uObject.Name().Index();
		i++;
	} while (outer != NULL);

	retObj.ObjAddress = uObjectAddress;
	retObj.OuterCount = i - 1;
	return true;
}

bool InstanceLogger::ReadGNameArray(uintptr_t address)
{
	int ptrSize = Utils::PointerSize(_memory);
	int sSub = _memory->Is64Bit && Utils::ProgramIs64() ? 0x0 : 0x4;
	JsonFNameEntity fName;

	if (!_memory->Is64Bit)
		address = _memory->ReadInt(address);
	else
		address = _memory->ReadInt64(address);

	// Get GNames Chunks
	std::vector<uintptr_t> gChunks;
	for (int i = 0; i < ptrSize * 15; ++i)
	{
		uintptr_t addr;
		const int offset = ptrSize * i;

		if (!_memory->Is64Bit)
			addr = _memory->ReadInt(address + offset); // 4byte
		else
			addr = _memory->ReadInt64(address + offset); // 8byte

		if (!Utils::IsValidAddress(_memory, addr)) break;
		gChunks.push_back(addr);
		gNamesChunks++;
	}

	// Alloc Size
	gNames = new GName[gNamesChunkCount * gNamesChunks];

	// Dump GNames
	int i = 0;
	for (uintptr_t chunkAddress : gChunks)
	{
		uintptr_t fNameAddress;
		for (int j = 0; j < gNamesChunkCount; ++j)
		{
			const int offset = ptrSize * j;

			if (!_memory->Is64Bit)
				fNameAddress = _memory->ReadInt(chunkAddress + offset); // 4byte
			else
				fNameAddress = _memory->ReadInt64(chunkAddress + offset); // 8byte

			if (!Utils::IsValidAddress(_memory, fNameAddress))
			{
				gNames[i].Index = i;
				gNames[i].AnsiName = "";
				i++;
				continue;
			}

			// Read FName
			if (_memory->ReadBytes(fNameAddress, fName.GetAllocPointer(), fName.StructSize) == 0)
				return false;

			// Set The Name
			gNames[i].Index = i;
			gNames[i].AnsiName = _memory->ReadText(fNameAddress + fName["AnsiName"].Offset - sSub);
			i++;
		}
	}

	return true;
}