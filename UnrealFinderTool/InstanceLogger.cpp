// Based on dano20zoo @ unknowncheats
// Converted to external by CorrM @ Me
// 
#include "pch.h"
#include "InstanceLogger.h"
#include <iostream>
#include <string>
#include <cinttypes>

InstanceLogger::InstanceLogger(Memory* memory, const uintptr_t gObjObjectsAddress, const uintptr_t gNamesAddress) :
	gObjObjects(),
	_memory(memory),
	gObjObjectsOffset(gObjObjectsAddress),
	gNamesOffset(gNamesAddress)
{
	ptrSize = _memory->Is64Bit ? 0x8 : 0x4;
}

bool InstanceLogger::ProgramIs64()
{
#if _WIN64
	return true;
#else
	return false;
#endif
}

bool InstanceLogger::DataCompare(const BYTE *pData, const BYTE *bMask, const char *szMask)
{
	for (; *szMask; ++szMask, ++pData, ++bMask) if (*szMask == 'x' && *pData != *bMask) return false;
	return (*szMask) == NULL;
}

char* InstanceLogger::GetName(UObject* object, bool& success)
{
	if (object->FNameIndex < 0 || object->FNameIndex > gNames.Names.size())
	{
		success = false;
		static char ret[256];
		sprintf_s(ret, "INVALID NAME INDEX : %i > %i", object->FNameIndex, gNames.Names.size());
		return ret;
	}
	success = true;
	return const_cast<char*>(gNames.Names[object->FNameIndex].AnsiName.c_str());
}

void InstanceLogger::ObjectDump()
{
	FILE* log = nullptr;
	fopen_s(&log, "ObjectDump.txt", "w+");

	for (DWORD64 i = 0x0; i < gObjObjects.ObjObjects.NumElements; i++)
	{
		if (gObjObjects.ObjObjects.Objects[i].Object->OriginalAddress == NULL) { continue; }

		bool state;
		char* name = GetName(gObjObjects.ObjObjects.Objects[i].Object, state);
		if (state) fprintf(log, "UObject[%06i] %-50s 0x%" PRIXPTR "\n", int(i), name, gObjObjects.ObjObjects.Objects[i].Object->OriginalAddress);
	}

	fclose(log);
}

void InstanceLogger::NameDump()
{
	FILE* log = nullptr;
	fopen_s(&log, "NameDump.txt", "w+");

	for (DWORD64 i = 0x0; i < gNames.Names.size(); i++)
	{
		// if (!gNames.Names[i]) { continue; }

		fprintf(log, "Name[%06i] %s\n", int(i), gNames.Names[i].AnsiName.c_str());
	}

	fclose(log);
}

void InstanceLogger::Start()
{
	const bool bObjectConvert = ReadUObjectArray(gObjObjectsOffset, gObjObjects);
	if (!bObjectConvert)
	{
		std::cout << red << "[*] " << def << "Invalid GObject Address." << std::endl << def;
		return;
	}

	const bool bNamesConvert = ReadGNameArray(gNamesOffset, gNames);
	if (!bNamesConvert)
	{
		std::cout << red << "[*] " << def << "Invalid GNames Address." << std::endl << def;
		return;
	}

	NameDump();
	ObjectDump();
}

bool InstanceLogger::ReadUObjectArray(const uintptr_t address, FUObjectArray& objectArray)
{
	const int sSub = _memory->Is64Bit && ProgramIs64() ? 0x0 : 0x4;
	SIZE_T len = _memory->ReadBytes(address, &objectArray, sizeof(FUObjectArray) - sSub);
	if (len == NULL) return false;
	uintptr_t dwUObjectItem, dwUObject;

	// ############################################
	// Convert class pointer to address
	// ############################################
	if (!_memory->Is64Bit)
	{
		if (ProgramIs64())
		{
			dwUObjectItem = reinterpret_cast<DWORD>(objectArray.ObjObjects.Objects); // 4byte
			FixStructPointer<UObject>(static_cast<void*>(&objectArray.ObjObjects), 0);
		}
		else
		{
			dwUObjectItem = reinterpret_cast<DWORD>(objectArray.ObjObjects.Objects); // 4byte
		}
	}
	else
	{
		dwUObjectItem = reinterpret_cast<DWORD64>(objectArray.ObjObjects.Objects); // 8byte
	}

	// ############################################
	// Alloc
	// ############################################
	objectArray.ObjObjects.Objects = new FUObjectItem[objectArray.ObjObjects.NumElements];
	objectArray.ObjObjects.NumElements *= 2;

	uintptr_t currentUObjAddress = dwUObjectItem;
	for (int i = 0; i < objectArray.ObjObjects.NumElements; ++i)
	{
		// Read the address as class
		len = _memory->ReadBytes(currentUObjAddress, &objectArray.ObjObjects.Objects[i], sizeof FUObjectItem);
		if (len == NULL) return false;

		// ############################################
		// Convert class pointer to address
		// ############################################
		if (!_memory->Is64Bit)
		{
			if (ProgramIs64())
			{
				dwUObject = reinterpret_cast<DWORD>(objectArray.ObjObjects.Objects[i].Object); // 4byte
				FixStructPointer<UObject>(static_cast<void*>(&objectArray.ObjObjects.Objects[i]), 0);
			}
			else
			{
				dwUObject = reinterpret_cast<DWORD>(objectArray.ObjObjects.Objects[i].Object); // 4byte
			}
		}
		else
		{
			dwUObject = reinterpret_cast<DWORD64>(objectArray.ObjObjects.Objects[i].Object); // 8byte
		}

		// ############################################
		// Alloc and Read the address as class
		// ############################################
		objectArray.ObjObjects.Objects[i].Object = new UObject();
		len = _memory->ReadBytes(dwUObject, objectArray.ObjObjects.Objects[i].Object, sizeof(UObject));
		if (len == NULL) return false;

		// Fix UObject
		if (!_memory->Is64Bit)
		{
			if (ProgramIs64())
			{
				// Fix VfTable
				FixStructPointer<UObject>(static_cast<void*>(objectArray.ObjObjects.Objects[i].Object), 0);

				// Fix Unknown00
				FixStructPointer<UObject>(static_cast<void*>(objectArray.ObjObjects.Objects[i].Object), 4);
			}
		}

		objectArray.ObjObjects.Objects[i].Object->OriginalAddress = dwUObject;
		currentUObjAddress += sizeof(FUObjectItem) - (sSub * 2);
	}
	return true;
}

bool InstanceLogger::ReadGNameArray(uintptr_t address, GNameArray& gNames)
{
	const int sSub = _memory->Is64Bit && ProgramIs64() ? 0x0 : 0x4;
	if (!_memory->Is64Bit)
	{
		address = _memory->ReadInt(address);
		address = _memory->ReadInt(address);
	}
	else
	{
		address = _memory->ReadInt64(address);
		address = _memory->ReadInt64(address);
	}

	uintptr_t fnameAddress, firstEntity = 0;
	SIZE_T len = 0;
	int i = 0;


	while (true)
	{
		FName name;
		if (!_memory->Is64Bit)
			fnameAddress = _memory->ReadInt(address + i); // 4byte
		else
			fnameAddress = _memory->ReadInt64(address + i); // 8byte

		if (firstEntity == NULL)
			firstEntity = fnameAddress;

		if (address + i >= firstEntity)
			break;

		if (!IsValidAddress(fnameAddress))
		{
			gNames.Names.push_back(name);
			i += ptrSize;
			continue;
		}

		// Read FName
		len = _memory->ReadBytes(fnameAddress, &name, sizeof(FName) - sizeof(string));
		if (len == NULL) return false;

		// Set The Name
		name.AnsiName = _memory->ReadText(fnameAddress + (sizeof(FName) - sizeof(string) - (sSub * 2)));
		gNames.Names.push_back(name);
		i += ptrSize;
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

template<typename ElementType>
void InstanceLogger::FixStructPointer(void* structBase, const int varOffsetEach4Byte)
{
	const int x1 = 0x4 * (varOffsetEach4Byte + 1);
	const int x2 = 0x4 * varOffsetEach4Byte;

	const int x1x = abs(x1 - int(sizeof(ElementType)));
	const int x2x = abs(x2 - int(sizeof(ElementType)));
	memcpy_s
	(
		static_cast<char*>(structBase) + x1,
		x1x,
		static_cast<char*>(structBase) + x2,
		x1x
	);
	memset(static_cast<char*>(structBase) + x1, 0, 0x4);
}