#include "pch.h"
#include "InstanceLogger.h"
#include "Utils.h"

#include <iostream>
#include <string>
#include <cinttypes>
using namespace nlohmann;

InstanceLogger::InstanceLogger(Memory* memory, const uintptr_t gObjObjectsAddress, const uintptr_t gNamesAddress) :
	gObjObjects(nullptr), gObjectsCount(0),
	gNames(nullptr), gNamesChunkCount(16384), gNamesChunks(0),
	_memory(memory),
	gObjObjectsOffset(gObjObjectsAddress),
	gNamesOffset(gNamesAddress)
{
	ptrSize = _memory->Is64Bit ? 0x8 : 0x4;
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

void InstanceLogger::ObjectDump()
{
	FILE* log = nullptr;
	fopen_s(&log, "ObjectDump.txt", "w+");

	if (log == nullptr)
	{
		MessageBoxA(nullptr, "Can't open 'ObjectDump.txt' for write.", "Wrong", MB_OK);
		return;
	}

	for (int i = 0; i < gObjectsCount; i++)
	{
		if (gObjObjects[i].ObjAddress == NULL) { continue; }

		bool state;
		std::string name = GetName(gObjObjects[i].FNameIndex, state);
		/*if (state) */
		fprintf(log, "UObject[%06i] %-80s 0x%" PRIXPTR "\n", int(i), name.c_str(), gObjObjects[i].ObjAddress);
	}

	fclose(log);
}

void InstanceLogger::NameDump()
{
	FILE* log = nullptr;
	fopen_s(&log, "NameDump.txt", "w+");

	if (log == nullptr)
	{
		MessageBoxA(nullptr, "Can't open 'NameDump.txt' for write.", "Wrong", MB_OK);
		return;
	}

	for (int i = 0; i < gNamesChunkCount * gNamesChunks; i++)
		fprintf(log, "Name[%06i] %s\n", int(i), gNames[i].AnsiName.c_str());

	fclose(log);
}

void InstanceLogger::Start()
{
	// Read core GObjects
	json jsonCore;
	if (!JsonReflector::ReadJsonFile("Config\\InstanceLogger\\GObjects.json", &jsonCore))
	{
		std::cout << red << "[*] " << def << "Can't read GObject file." << std::endl << def;
		return;
	}
	// Load all json structs in file
	if (!JsonReflector::Load(&jsonCore))
	{
		std::cout << red << "[*] " << def << "Can't load GObject file." << std::endl << def;
		return;
	}

	// Read core GNames
	if (!JsonReflector::ReadJsonFile("Config\\InstanceLogger\\GNames.json", &jsonCore))
	{
		std::cout << red << "[*] " << def << "Can't read GNames file." << std::endl << def;
		return;
	}
	// Load all json structs in file
	if (!JsonReflector::Load(&jsonCore))
	{
		std::cout << red << "[*] " << def << "Can't load GNames file." << std::endl << def;
		return;
	}

	// GObjects
	JsonStruct jsonGObjectsReader;
	if (!JsonReflector::Read("FUObjectArray", jsonGObjectsReader))
	{
		std::cout << red << "[*] " << def << "Can't find struct `FUObjectArray`." << std::endl << def;
		return;
	}
	if (!ReadUObjectArray(gObjObjectsOffset, jsonGObjectsReader))
	{
		std::cout << red << "[*] " << def << "Invalid GObject Address." << std::endl << def;
		return;
	}

	// GNames
	if (!ReadGNameArray(gNamesOffset))
	{
		std::cout << red << "[*] " << def << "Invalid GNames Address." << std::endl << def;
		return;
	}

	NameDump();
	ObjectDump();

	std::cout << green << "[+] " << purple << "Found [" << green << gObjectsCount << purple << "] Object." << std::endl;
	std::cout << green << "[+] " << purple << "Found [" << green << gNamesChunkCount * gNamesChunks << purple << "] Name." << std::endl;
}

bool InstanceLogger::ReadUObjectArray(const uintptr_t address, JsonStruct& objectArray)
{
	const size_t sSub = _memory->Is64Bit && Utils::ProgramIs64() ? 0x0 : 0x4;
	JsonStruct fUObjectItem, uObject;

	if (!JsonReflector::Read("FUObjectItem", fUObjectItem))
	{
		std::cout << red << "[*] " << def << "Can't find struct `FUObjectItem`." << std::endl << def;
		return false;
	}

	if (!JsonReflector::Read("UObject", uObject))
	{
		std::cout << red << "[*] " << def << "Can't find struct `UObject`." << std::endl << def;
		return false;
	}

	if (_memory->ReadBytes(address, objectArray.AllocPointer, objectArray.StructSize - sSub) == NULL) return false;

	auto objObjects = objectArray["ObjObjects"].ReadAsStruct();
	int num = objObjects["NumElements"].ReadAs<int>();

	uintptr_t dwFUObjectAddress = Utils::ReadSafePointer(objObjects.AllocPointer, 0, objObjects.StructSize, _memory->Is64Bit);

	// Alloc all objects
	gObjObjects = new GObject[num];
	ZeroMemory(gObjObjects, sizeof(GObject) * num);

	uintptr_t currentFUObjAddress = dwFUObjectAddress;
	for (int i = 0; i < num; ++i)
	{
		// Read the address as struct
		if (_memory->ReadBytes(currentFUObjAddress, fUObjectItem.AllocPointer, fUObjectItem.StructSize - sSub) == NULL)
			return false;

		// Convert class pointer to address
		uintptr_t dwUObject = Utils::ReadSafePointer(fUObjectItem.AllocPointer, 0, fUObjectItem.StructSize, _memory->Is64Bit);


		// Skip null pointer in GObjects array
		if (dwUObject == NULL)
		{
			currentFUObjAddress += fUObjectItem.StructSize - (sSub * 2);
			gObjectsCount++;
			continue;
		}

		// Alloc and Read the address as class
		if (_memory->ReadBytes(dwUObject, uObject.AllocPointer, uObject.StructSize - sSub) == NULL)
			return false;

		// Fix UObject
		if (!_memory->Is64Bit)
		{
			if (Utils::ProgramIs64())
			{
				// Fix VfTable
				Utils::FixStructPointer(uObject.AllocPointer, 0, uObject.StructSize);
			}
		}

		gObjObjects[i].ObjAddress = dwUObject;
		gObjObjects[i].FNameIndex = uObject["FNameIndex"].ReadAs<int>();

		currentFUObjAddress += fUObjectItem.StructSize - (sSub * 2);
		gObjectsCount++;
	}
	return true;
}

bool InstanceLogger::ReadGNameArray(uintptr_t address)
{
	int sSub = _memory->Is64Bit && Utils::ProgramIs64() ? 0x0 : 0x4;
	JsonStruct fName;
	if (!JsonReflector::Read("FName", fName))
	{
		std::cout << red << "[*] " << def << "Can't find struct `FName`." << std::endl << def;
		return false;
	}

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

		if (!IsValidAddress(addr)) break;
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

			if (!IsValidAddress(fNameAddress))
			{
				gNames[i].Index = i;
				gNames[i].AnsiName = "";
				i++;
				continue;
			}

			// Read FName
			if (_memory->ReadBytes(fNameAddress, fName.AllocPointer, fName.StructSize) == NULL)
				return false;

			// Set The Name
			gNames[i].Index = i;
			gNames[i].AnsiName = _memory->ReadText(fNameAddress + fName["AnsiName"].Offset - sSub);
			i++;
		}
	}

	return true;
}

DWORD InstanceLogger::BufToInteger(void* buffer)
{
	return *reinterpret_cast<DWORD*>(buffer);
}

DWORD64 InstanceLogger::BufToInteger64(void* buffer)
{
	return *reinterpret_cast<DWORD64*>(buffer);
}

bool InstanceLogger::IsValidAddress(const uintptr_t address)
{
	if (INVALID_POINTER_VALUE(address) /*|| pointer <= dwStart || pointer > dwEnd*/)
		return false;

	// Check memory state, type and permission
	MEMORY_BASIC_INFORMATION info;

	//const uintptr_t pointerVal = _memory->ReadInt(pointer);
	if (VirtualQueryEx(_memory->ProcessHandle, LPVOID(address), &info, sizeof info) == sizeof info)
	{
		// Bad Memory
		return info.Protect != PAGE_NOACCESS;
	}

	return false;
}