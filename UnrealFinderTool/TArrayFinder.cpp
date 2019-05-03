#include "pch.h"
#include "TArrayFinder.h"
#include "Color.h"

/*
 * #NOTES
 * Some GObject use this syntax every 4byte/8byte (pointer) there is UObject
 */

TArrayFinder::TArrayFinder(Memory* memory) : _memory(memory)
{
	dwStart = 0;
	dwEnd = 0;
	ptr_size = !_memory->Is64Bit ? 0x4 : 0x8;
}

void TArrayFinder::Find()
{
	dwStart = !_memory->Is64Bit ? 0x100000 : static_cast<uintptr_t>(0x7FF00000);
	dwEnd = !_memory->Is64Bit ? 0x7FEFFFFF : static_cast<uintptr_t>(0x7fffffffffff);

	std::cout << dgreen << "[!] " << def << "Start scan for GObjects. (at 0x" << std::hex << dwStart << ". to 0x" << std::hex << dwEnd << ")" << std::endl << def;

	// Start scan for TArrays
	SYSTEM_INFO si = { 0 };
	GetSystemInfo(&si);

	if (dwStart < reinterpret_cast<uintptr_t>(si.lpMinimumApplicationAddress))
		dwStart = reinterpret_cast<uintptr_t>(si.lpMinimumApplicationAddress);

	if (dwEnd > reinterpret_cast<uintptr_t>(si.lpMaximumApplicationAddress))
		dwEnd = reinterpret_cast<uintptr_t>(si.lpMaximumApplicationAddress);

	// dwStart = static_cast<uintptr_t>(0x176EDFE0000);
	// dwEnd = static_cast<uintptr_t>(0x7ff7ccc25c68);

	int found_count = 0;
	MEMORY_BASIC_INFORMATION info;

	// Cycle through memory based on RegionSize
	for (uintptr_t i = dwStart; (VirtualQueryEx(_memory->ProcessHandle, LPVOID(i), &info, sizeof info) == sizeof info && i < dwEnd); i += info.RegionSize)
	{
		// Bad Memory
		if (info.State != MEM_COMMIT) continue;
		if (info.Protect != PAGE_EXECUTE_READWRITE && info.Protect != PAGE_READWRITE) continue;

		if (IsValidTArray(i) == ERROR_SUCCESS)
		{
			std::cout << green << "[+] " << def << "\t" << red << "0x" << std::hex << i << std::dec << def << std::endl;
			// std::cout << " (" << yellow << "Max: " << gObject.Max << ", Num: " << gObject.Num << ", Data: " << std::hex << gObject.Data << def << ")" << std::dec << std::endl;
			found_count++;
		}
	}

	std::cout << purple << "[!] " << yellow << "Found " << found_count << " Address." << std::endl << def;

	if (found_count > 0)
	{
		std::cout << red << "[*] " << green << "Address is first UObject in the array." << std::endl;
		std::cout << red << "[*] " << green << "So you must get the pointer how point the address." << std::endl;
		std::cout << red << "[*] " << green << "Maybe you need to find the pointer how point the pointer you get." << std::endl;
	}
}

bool TArrayFinder::IsValidPointer(const uintptr_t address, uintptr_t& pointer, const bool checkIsAllocationBase)
{
	pointer = NULL;
	if (!_memory->Is64Bit)
		pointer = _memory->ReadUInt(address);
	else
		pointer = _memory->ReadUInt64(address);

	if (INVALID_POINTER_VALUE(pointer) /*|| pointer <= dwStart || pointer > dwEnd*/)
		return false;

	// Check memory state, type and permission
	MEMORY_BASIC_INFORMATION info;

	//const uintptr_t pointerVal = _memory->ReadInt(pointer);
	if (VirtualQueryEx(_memory->ProcessHandle, LPVOID(pointer), &info, sizeof info) == sizeof info)
	{
		// Bad Memory
		// if (info.State != MEM_COMMIT) return false;
		if (info.Protect == PAGE_NOACCESS) return false;
		if (checkIsAllocationBase && info.AllocationBase != LPVOID(pointer)) return false;

		return true;
	}

	return false;
}

DWORD TArrayFinder::IsValidTArray(const uintptr_t address)
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
			const int uObject0InternalIndex = _memory->ReadInt(ptrUObject0 + io);
			const int uObject1InternalIndex = _memory->ReadInt(ptrUObject1 + io);
			const int uObject2InternalIndex = _memory->ReadInt(ptrUObject2 + io);
			const int uObject3InternalIndex = _memory->ReadInt(ptrUObject3 + io);
			const int uObject4InternalIndex = _memory->ReadInt(ptrUObject4 + io);
			const int uObject5InternalIndex = _memory->ReadInt(ptrUObject5 + io);

			if (uObject0InternalIndex != 0) continue;
			if (!(uObject1InternalIndex == 1 || uObject1InternalIndex == 3)) continue;
			if (!(uObject2InternalIndex == 2 || uObject2InternalIndex == 6)) continue;
			if (!(uObject3InternalIndex == 3 || uObject3InternalIndex == 9)) continue;
			if (!(uObject4InternalIndex == 4 || uObject4InternalIndex == 12)) continue;
			if (!(uObject5InternalIndex == 5 || uObject5InternalIndex == 15)) continue;

			return ERROR_SUCCESS;
		}
	}
	return ret;
}

DWORD TArrayFinder::IsValidTArray2(const uintptr_t address)
{
	DWORD ret = 1; // OBJECT_ERROR
	uintptr_t ptrUObject0, ptrUObject1, ptrUObject2, ptrUObject3, ptrUObject4, ptrUObject5;

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
		ret += 1; // INDEX_ERROR

		// Check Objects (InternalIndex)
		for (int io = 0x0; io < 0x1C; io += 0x4)
		{
			const int uObject0InternalIndex = _memory->ReadInt(ptrUObject0 + io);
			const int uObject1InternalIndex = _memory->ReadInt(ptrUObject1 + io);
			const int uObject2InternalIndex = _memory->ReadInt(ptrUObject2 + io);
			const int uObject3InternalIndex = _memory->ReadInt(ptrUObject3 + io);
			const int uObject4InternalIndex = _memory->ReadInt(ptrUObject4 + io);
			const int uObject5InternalIndex = _memory->ReadInt(ptrUObject5 + io);

			if (uObject0InternalIndex != 0) continue;
			if (!(uObject1InternalIndex == 1 || uObject1InternalIndex == 3)) continue;
			if (!(uObject2InternalIndex == 2 || uObject2InternalIndex == 6)) continue;
			if (!(uObject3InternalIndex == 3 || uObject3InternalIndex == 9)) continue;
			if (!(uObject4InternalIndex == 4 || uObject4InternalIndex == 12)) continue;
			if (!(uObject5InternalIndex == 5 || uObject5InternalIndex == 15)) continue;

			return ERROR_SUCCESS;
		}
	}
	return ret;
}