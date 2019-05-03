#include "pch.h"
#include "TArrayFinder.h"
#include "Color.h"


TArrayFinder::TArrayFinder(Memory* memory) : _memory(memory)
{
	dwStart = 0;
	dwEnd = 0;
	ptr_size = !_memory->Is64Bit ? 0x4 : 0x8;
	dwBetweenObjects = ptr_size + (_memory->Is64Bit ? 0x10 : 0xC);
	dwInternalIndex = ptr_size + 0x4;
}

void TArrayFinder::Find()
{
	dwStart = !_memory->Is64Bit ? 0x300000 : static_cast<uintptr_t>(0x7FF00000);
	dwEnd = !_memory->Is64Bit ? 0x7FEFFFFF : static_cast<uintptr_t>(0x7fffffffffff);

	std::cout << dgreen << "[!] " << def << "Start scan for GObjects. (at 0x" << std::hex << dwStart << ". to 0x" << std::hex << dwEnd << ")" << std::endl << def;

	// Start scan for TArrays
	SYSTEM_INFO si = { 0 };
	GetSystemInfo(&si);

	if (dwStart < reinterpret_cast<uintptr_t>(si.lpMinimumApplicationAddress))
		dwStart = reinterpret_cast<uintptr_t>(si.lpMinimumApplicationAddress);

	if (dwEnd > reinterpret_cast<uintptr_t>(si.lpMaximumApplicationAddress))
		dwEnd = reinterpret_cast<uintptr_t>(si.lpMaximumApplicationAddress);

	// dwStart = static_cast<uintptr_t>(0x07350000);
	// dwEnd = static_cast<uintptr_t>(0x7ff7ccc25c68);

	int found_count = 0;
	MEMORY_BASIC_INFORMATION info;
	FUObjectArray gObject;

	// Cycle through memory based on RegionSize
	for (uintptr_t i = dwStart; (VirtualQueryEx(_memory->ProcessHandle, LPVOID(i), &info, sizeof info) == sizeof info && i < dwEnd); i += info.RegionSize)
	{
		// Bad Memory
		if (info.State != MEM_COMMIT) continue;
		if (info.Protect != PAGE_EXECUTE_READWRITE && info.Protect != PAGE_READWRITE) continue;

		if (IsValidTArray(i, gObject))
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
		std::cout << red << "[*] " << green << "So u must get the pointer how point this address." << std::endl;
		std::cout << red << "[*] " << green << "Maybe u need to find the pointer how point the pointer you get." << std::endl;
	}
}

bool TArrayFinder::IsValidPointer(const uintptr_t address, uintptr_t& pointer, const bool checkIsAllocationBase)
{
	pointer = NULL;
	if (!_memory->Is64Bit)
		pointer = _memory->ReadUInt(address);
	else
		pointer = _memory->ReadUInt64(address);

	if (INVALID_POINTER_VALUE(pointer) /*|| pointer < dwStart || pointer > dwEnd*/)
		return false;

	// Check memory state, type and permission
	MEMORY_BASIC_INFORMATION info;

	//const uintptr_t pointerVal = _memory->ReadInt(pointer);
	if (VirtualQueryEx(_memory->ProcessHandle, LPVOID(pointer), &info, sizeof info) == sizeof info)
	{
		// Bad Memory
		if (info.State != MEM_COMMIT) return false;
		if (info.Protect == PAGE_NOACCESS) return false;
		if (checkIsAllocationBase && info.AllocationBase != LPVOID(pointer)) return false;

		return true;
	}

	return false;
}

bool TArrayFinder::IsValidTArray(const uintptr_t address, FUObjectArray& tArray)
{
	tArray = { 0, 0, 0 };

	// Check GObjects
	// ObjFirstGCIndex = (ObjLastNonGCIndex - 1) & MaxObjectsNotConsideredByGC = ObjFirstGCIndex, OpenForDisregardForGC = 0(not sure)
	//const int obj_first_gc_index = _memory->ReadInt(address - 0x10);
	//const int obj_last_non_gc_index = _memory->ReadInt(address - 0xC);
	//const int max_objects_not_considered_by_gc = _memory->ReadInt(address - 0x8);
	//const int openForDisregardForGC = _memory->ReadInt(address - 0x4);
	//if (obj_first_gc_index < 0xF) return false;
	//if (obj_last_non_gc_index < 0xF) return false;
	//if (max_objects_not_considered_by_gc < 0xF) return false;
	//if (openForDisregardForGC > 1 || openForDisregardForGC < 0) return false; // it's bool
	//// if (obj_first_gc_index != obj_last_non_gc_index + 1) return false;
	//// if (max_objects_not_considered_by_gc != obj_first_gc_index) return false;
	//// if (openForDisregardForGC != 0) return false;

	// Check (UObject*) (Object1) Is Valid Pointer
	uintptr_t ptrUObject0;
	if (!IsValidPointer(address, ptrUObject0, false)) return false;

	uintptr_t ptrUObject1;
	if (!IsValidPointer(address + dwBetweenObjects, ptrUObject1, false)) return false;

	uintptr_t ptrUObject2;
	if (!IsValidPointer(address + (dwBetweenObjects * 2), ptrUObject2, false)) return false;

	uintptr_t ptrUObject3;
	if (!IsValidPointer(address + (dwBetweenObjects * 3), ptrUObject3, false)) return false;

	// Check vfTableObject Is Valid Pointer
	uintptr_t ptrVfTableObject0, ptrVfTableObject1, ptrVfTableObject2, ptrVfTableObject3;
	if (!IsValidPointer(ptrUObject0, ptrVfTableObject0, false)) return false;
	if (!IsValidPointer(ptrUObject1, ptrVfTableObject1, false)) return false;
	if (!IsValidPointer(ptrUObject2, ptrVfTableObject2, false)) return false;
	if (!IsValidPointer(ptrUObject3, ptrVfTableObject3, false)) return false;

	// Check Objects (InternalIndex)
	const int uObject0InternalIndex = _memory->ReadInt(ptrUObject0 + dwInternalIndex);
	const int uObject1InternalIndex = _memory->ReadInt(ptrUObject1 + dwInternalIndex);
	const int uObject2InternalIndex = _memory->ReadInt(ptrUObject2 + dwInternalIndex);
	const int uObject3InternalIndex = _memory->ReadInt(ptrUObject3 + dwInternalIndex);

	if (!(uObject0InternalIndex == 0 && (uObject1InternalIndex == 1 || uObject1InternalIndex == 3))) return false;
	if (!(uObject2InternalIndex == 2 || uObject2InternalIndex == 6)) return false;
	if (!(uObject3InternalIndex == 3 || uObject3InternalIndex == 9)) return false;

	tArray.Data = address;
	tArray.Max = 0;
	tArray.Num = 0;
	return true;
}

bool TArrayFinder::IsValidTArray2(const uintptr_t address, FUObjectArray& tArray)
{
	tArray = { 0, 0, 0 };

	// Check (UObject*) (Object1) Is Valid Pointer
	uintptr_t ptrUObject0;
	if (!IsValidPointer(address, ptrUObject0, false)) return false;

	uintptr_t ptrUObject1;
	if (!IsValidPointer(address + dwBetweenObjects, ptrUObject1, false)) return false;

	uintptr_t ptrUObject2;
	if (!IsValidPointer(address + (dwBetweenObjects * 2), ptrUObject2, false)) return false;

	uintptr_t ptrUObject3;
	if (!IsValidPointer(address + (dwBetweenObjects * 3), ptrUObject3, false)) return false;

	// Check Objects (InternalIndex)
	const int uObject0InternalIndex = _memory->ReadInt(ptrUObject0 + dwInternalIndex);
	const int uObject1InternalIndex = _memory->ReadInt(ptrUObject1 + dwInternalIndex);
	const int uObject2InternalIndex = _memory->ReadInt(ptrUObject2 + dwInternalIndex);
	const int uObject3InternalIndex = _memory->ReadInt(ptrUObject3 + dwInternalIndex);

	if (!(uObject0InternalIndex == 0 && (uObject1InternalIndex == 1 || uObject1InternalIndex == 3))) return false;
	if (!(uObject2InternalIndex == 2 || uObject2InternalIndex == 6)) return false;
	if (!(uObject3InternalIndex == 3 || uObject3InternalIndex == 9)) return false;

	tArray.Data = address;
	tArray.Max = 0;
	tArray.Num = 0;
	return true;
}