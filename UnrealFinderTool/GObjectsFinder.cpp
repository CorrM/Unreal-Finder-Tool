#include "pch.h"
#include "Memory.h"
#include "GObjectsFinder.h"

/*
 * #NOTES
 * Some GObject use this syntax every 4byte/8byte (pointer) there is UObject
 */

GObjectsFinder::GObjectsFinder(const bool easyMethod) : easyMethod(easyMethod)
{
	dwStart = 0;
	dwEnd = 0;
	ptrSize = !Utils::MemoryObj->Is64Bit ? 0x4 : 0x8;
}

std::vector<uintptr_t> GObjectsFinder::Find()
{
	std::vector<uintptr_t> ret;
	// dwStart = !_memory->Is64Bit ? 0x100000 : static_cast<uintptr_t>(0x7FF00000);
	dwEnd = !Utils::MemoryObj->Is64Bit ? 0x7FEFFFFF : static_cast<uintptr_t>(0x7fffffffffff);

	// Start scan for TArrays
	SYSTEM_INFO si = { 0 };
	GetSystemInfo(&si);

	if (dwStart < reinterpret_cast<uintptr_t>(si.lpMinimumApplicationAddress))
		dwStart = reinterpret_cast<uintptr_t>(si.lpMinimumApplicationAddress);

	if (dwEnd > reinterpret_cast<uintptr_t>(si.lpMaximumApplicationAddress))
		dwEnd = reinterpret_cast<uintptr_t>(si.lpMaximumApplicationAddress);

	// dwStart = static_cast<uintptr_t>(0x48356000);
	// dwEnd = static_cast<uintptr_t>(0x7ff7ccc25c68);

	MEMORY_BASIC_INFORMATION info;

	// Cycle through memory based on RegionSize
	for (uintptr_t i = dwStart; (VirtualQueryEx(Utils::MemoryObj->ProcessHandle, LPVOID(i), &info, sizeof info) == sizeof info && i < dwEnd); i += easyMethod ? info.RegionSize : si.dwPageSize)
	{
		// Bad Memory
		if (info.State != MEM_COMMIT) continue;
		if (info.Protect != PAGE_EXECUTE_READWRITE && info.Protect != PAGE_READWRITE) continue;

		if (IsValidGObjects(i) == ERROR_SUCCESS)
			ret.push_back(i);
	}

	return ret;
}

bool GObjectsFinder::IsValidPointer(const uintptr_t address, uintptr_t& pointer, const bool checkIsAllocationBase)
{
	pointer = NULL;
	if (!Utils::MemoryObj->Is64Bit)
		pointer = Utils::MemoryObj->ReadUInt(address);
	else
		pointer = Utils::MemoryObj->ReadUInt64(address);

	if (INVALID_POINTER_VALUE(pointer) /*|| pointer <= dwStart || pointer > dwEnd*/)
		return false;

	// Check memory state, type and permission
	MEMORY_BASIC_INFORMATION info;

	//const uintptr_t pointerVal = _memory->ReadInt(pointer);
	if (VirtualQueryEx(Utils::MemoryObj->ProcessHandle, LPVOID(pointer), &info, sizeof info) == sizeof info)
	{
		// Bad Memory
		// if (info.State != MEM_COMMIT) return false;
		if (info.Protect == PAGE_NOACCESS) return false;
		if (checkIsAllocationBase && info.AllocationBase != LPVOID(pointer)) return false;

		return true;
	}

	return false;
}

DWORD GObjectsFinder::IsValidGObjects(const uintptr_t address)
{
	DWORD ret = 1; // OBJECT_ERROR
	uintptr_t ptrUObject0, ptrUObject1, ptrUObject2, ptrUObject3, ptrUObject4, ptrUObject5;
	uintptr_t ptrVfTableObject0, ptrVfTableObject1, ptrVfTableObject2, ptrVfTableObject3, ptrVfTableObject4, ptrVfTableObject5;

	for (int i = 0x0; i <= 0x20; i += 0x4)
	{
		// Check (UObject*) Is Valid Pointer
		if (!IsValidPointer(address + (i * 0), ptrUObject0, false)) continue;
		if (!IsValidPointer(address + (i * 1), ptrUObject1, false)) continue;
		if (!IsValidPointer(address + (i * 2), ptrUObject2, false)) continue;
		if (!IsValidPointer(address + (i * 3), ptrUObject3, false)) continue;
		if (!IsValidPointer(address + (i * 4), ptrUObject4, false)) continue;
		if (!IsValidPointer(address + (i * 5), ptrUObject5, false)) continue;
		ret += 1; // VFTABLE_ERROR

		// Check vfTableObject Is Valid Pointer
		if (!IsValidPointer(ptrUObject0, ptrVfTableObject0, false)) continue;
		if (!IsValidPointer(ptrUObject1, ptrVfTableObject1, false)) continue;
		if (!IsValidPointer(ptrUObject2, ptrVfTableObject2, false)) continue;
		if (!IsValidPointer(ptrUObject3, ptrVfTableObject3, false)) continue;
		if (!IsValidPointer(ptrUObject4, ptrVfTableObject4, false)) continue;
		if (!IsValidPointer(ptrUObject5, ptrVfTableObject5, false)) continue;
		ret += 1; // INDEX_ERROR

		// Check Objects (InternalIndex)
		for (int io = 0x0; io < 0x1C; io += 0x4)
		{
			const int uObject0InternalIndex = Utils::MemoryObj->ReadInt(ptrUObject0 + io);
			const int uObject1InternalIndex = Utils::MemoryObj->ReadInt(ptrUObject1 + io);
			const int uObject2InternalIndex = Utils::MemoryObj->ReadInt(ptrUObject2 + io);
			const int uObject3InternalIndex = Utils::MemoryObj->ReadInt(ptrUObject3 + io);
			const int uObject4InternalIndex = Utils::MemoryObj->ReadInt(ptrUObject4 + io);
			const int uObject5InternalIndex = Utils::MemoryObj->ReadInt(ptrUObject5 + io);

			if (uObject0InternalIndex != 0) continue;
			if (!(uObject1InternalIndex == 1 || uObject1InternalIndex == 3)) continue;
			if (!(uObject2InternalIndex == 2 || uObject2InternalIndex == 6)) continue;
			if (!(uObject3InternalIndex == 3 || uObject3InternalIndex == 9)) continue;
			if (!(uObject4InternalIndex == 4 || uObject4InternalIndex == 12)) continue;
			if (!(uObject5InternalIndex == 5 || uObject5InternalIndex == 15)) continue;

			// Check if 2nd UObject have FName_Index == 100
			bool bFoundNameIndex = false;
			for (int j = 0x4; j < 0x1C; j += 0x4)
			{
				const int uFNameIndex = Utils::MemoryObj->ReadInt(ptrUObject1 + j);
				if (uFNameIndex == 100)
				{
					bFoundNameIndex = true;
					break;
				}
			}

			return bFoundNameIndex ? ERROR_SUCCESS : ret;
		}
	}
	return ret;
}