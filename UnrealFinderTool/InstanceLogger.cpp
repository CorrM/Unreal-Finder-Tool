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

char* InstanceLogger::GetName(UObject* object, bool& success)
{
	if (object->FNameIndex < 0 || object->FNameIndex > gNames.Names.size())
	{
		success = false;
		static char ret[256];
		sprintf_s(ret, "INVALID NAME INDEX : %i > %zu", object->FNameIndex, gNames.Names.size());
		return ret;
	}
	success = true;
	return const_cast<char*>(gNames.Names[object->FNameIndex].AnsiName.c_str());
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

	for (int i = 0x0; i < gObjObjects.ObjObjects.NumElements; i++)
	{
		if (gObjObjects.ObjObjects.Objects[i].Object->OriginalAddress == NULL) { continue; }

		bool state;
		char* name = GetName(gObjObjects.ObjObjects.Objects[i].Object, state);
		/*if (state) */fprintf(log, "UObject[%06i] %-80s 0x%" PRIXPTR "\n", int(i), name, gObjObjects.ObjObjects.Objects[i].Object->OriginalAddress);
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
	const uint32_t sSub = _memory->Is64Bit && ProgramIs64() ? 0x0 : 0x4;
	if (_memory->ReadBytes(address, &objectArray, sizeof(FUObjectArray) - sSub) == NULL) return false;
	uintptr_t dwUObjectItem, dwUObject;

	// ############################################
	// Convert class pointer to address
	// ############################################
	if (!_memory->Is64Bit)
	{
		if (ProgramIs64())
		{
			dwUObjectItem = reinterpret_cast<DWORD>(objectArray.ObjObjects.Objects); // 4byte
			FixStructPointer<TUObjectArray>(static_cast<void*>(&objectArray.ObjObjects), 0);
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

	uintptr_t currentUObjAddress = dwUObjectItem;
	for (int i = 0; i < objectArray.ObjObjects.NumElements; ++i)
	{
		// Read the address as class
		if (_memory->ReadBytes(currentUObjAddress, &objectArray.ObjObjects.Objects[i], sizeof FUObjectItem) == NULL) return false;

		// ############################################
		// Convert class pointer to address
		// ############################################
		if (!_memory->Is64Bit)
		{
			if (ProgramIs64())
			{
				dwUObject = reinterpret_cast<DWORD>(objectArray.ObjObjects.Objects[i].Object); // 4byte
				FixStructPointer<FUObjectItem>(static_cast<void*>(&objectArray.ObjObjects.Objects[i]), 0);
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
		if (_memory->ReadBytes(dwUObject, objectArray.ObjObjects.Objects[i].Object, sizeof(UObject) - sizeof(uintptr_t)) == NULL) return false;

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
	int sSub = _memory->Is64Bit && ProgramIs64() ? 0x0 : 0x4;
	if (!_memory->Is64Bit)
		address = _memory->ReadInt(address);
	else
		address = _memory->ReadInt64(address);

	// Get GNames Chunks
	std::vector<uintptr_t> gNameChunks;
	for (int i = 0; i < ptrSize * 15; ++i)
	{
		uintptr_t addr;
		const int offset = ptrSize * i;

		if (!_memory->Is64Bit)
			addr = _memory->ReadInt(address + offset); // 4byte
		else
			addr = _memory->ReadInt64(address + offset); // 8byte

		if (!IsValidAddress(addr)) break;
		gNameChunks.push_back(addr);
	}

	// Dump GNames
	const auto sizeToRead = sizeof(FName) - sizeof(string);
	for (uintptr_t chunkAddress : gNameChunks)
	{
		uintptr_t fNameAddress;

		for (int j = 0; j < gNames.NumElements; ++j)
		{
			FName name;
			const int offset = ptrSize * j;

			if (!_memory->Is64Bit)
				fNameAddress = _memory->ReadInt(chunkAddress + offset); // 4byte
			else
				fNameAddress = _memory->ReadInt64(chunkAddress + offset); // 8byte

			if (!IsValidAddress(fNameAddress))
			{
				gNames.Names.push_back(name);
				continue;
			}

			// Read FName
			const SIZE_T len = _memory->ReadBytes(fNameAddress, &name, sizeToRead);
			if (len == NULL) return false;

			// Set The Name
			name.AnsiName = _memory->ReadText(fNameAddress + sizeToRead - sSub);
			gNames.Names.push_back(name);
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

bool InstanceLogger::IsValidAddress(const uintptr_t address) const
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

/// <summary>
/// Fix pointer size in struct or class. used for convert 64bit to 32bit pointer.
/// Must pick the right `ElementType`, else will case some memory problems in the hole program
/// </summary>
/// <param name="structBase">Pointer to instance of `ElementType`</param>
/// <param name="varOffsetEach4Byte">Pointer position of `ElementType`</param>
template<typename ElementType>
void InstanceLogger::FixStructPointer(void* structBase, const int varOffsetEach4Byte) const
{
	const int x1 = 0x4 * (varOffsetEach4Byte + 1);
	const int x2 = 0x4 * varOffsetEach4Byte;

	const int size = abs(x1 - int(sizeof(ElementType)));
	memcpy_s
	(
		static_cast<char*>(structBase) + x1,
		size,
		static_cast<char*>(structBase) + x2,
		size
	);
	memset(static_cast<char*>(structBase) + x1, 0, 0x4);
}
