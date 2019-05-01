#include "pch.h"
#include "TArrayFinder.h"
#include "Color.h"
#include <vector>


TArrayFinder::TArrayFinder(Memory* memory) : _memory(memory)
{
	dwStart = 0;
	dwEnd = 0;
	ptr_size = !_memory->Is64Bit ? 0x4 : 0x8;
}

void TArrayFinder::Find()
{
	Color green(LightGreen, Black);
	Color def(White, Black);
	Color yellow(LightYellow, Black);
	Color purple(LightPurple, Black);
	Color red(LightRed, Black);
	Color dgreen(Green, Black);

	dwStart = !_memory->Is64Bit ? 0x300000 : static_cast<uintptr_t>(0x7FF00000);
	dwEnd = !_memory->Is64Bit ? 0x7FEFFFFF : static_cast<uintptr_t>(0x7fffffffffff);

	std::cout << dgreen << "[!] " << def << "Start scan for TArrays. (at 0x" << std::hex << dwStart << ". to 0x" << std::hex << dwEnd << ")" << std::endl << def;

	// Start scan for TArrays
	SYSTEM_INFO si = { 0 };
	GetSystemInfo(&si);

	if (dwStart < reinterpret_cast<uintptr_t>(si.lpMinimumApplicationAddress))
		dwStart = reinterpret_cast<uintptr_t>(si.lpMinimumApplicationAddress);

	if (dwEnd > reinterpret_cast<uintptr_t>(si.lpMaximumApplicationAddress))
		dwEnd = reinterpret_cast<uintptr_t>(si.lpMaximumApplicationAddress);

	int found_count = 0;
	MEMORY_BASIC_INFORMATION info;

	// Cycle through memory based on RegionSize
	for (uintptr_t i = dwStart; (VirtualQueryEx(_memory->ProcessHandle, LPVOID(i), &info, sizeof info) == sizeof info && i < dwEnd); i += info.RegionSize)
	{
		// Bad Memory
		if (info.State != MEM_COMMIT) continue;
		if (info.Type != MEM_PRIVATE) continue;
		if (info.Protect != PAGE_READWRITE && info.Protect != PAGE_EXECUTE_READ) continue;

		// Alloc memory
		const auto pBuf = static_cast<PBYTE>(malloc(info.RegionSize));

		// Read one page or skip if failed
		const SIZE_T dwOut = _memory->ReadBytes(i, pBuf, info.RegionSize);
		if (dwOut == 0)
		{
			free(pBuf);
			continue;
		}

		// loop through memory region based on 4 byte (even it 64bit pointer)
		for (SIZE_T j = 0; j < info.RegionSize; j += 0x4)
		{
			const uintptr_t address = i + j;

			if (IsValidTArray(address))
			{
				std::cout << green << "[+] " << def << "\t" << red << "0x" << std::hex << address << std::endl;
				found_count++;
			}
		}

		free(pBuf);
	}

	std::cout << purple << "[!] " << yellow << "Found " << found_count << " Address." << std::endl << def;
}

bool TArrayFinder::IsValidPointer(const uintptr_t address, uintptr_t& pointerVal)
{
	uintptr_t pointer = NULL;
	pointerVal = NULL;
	if (!_memory->Is64Bit)
		pointer = _memory->ReadUInt(address);
	else
		pointer = _memory->ReadUInt64(address);

	if (INVALID_POINTER_VALUE(pointer) || pointer < dwStart || pointer > dwEnd || pointer < 0x10000000)
		return false;

	// Check memory state, type and permission
	MEMORY_BASIC_INFORMATION info;
	pointerVal = _memory->ReadInt(pointer);
	if (VirtualQueryEx(_memory->ProcessHandle, LPVOID(pointerVal), &info, sizeof info) == sizeof info)
	{
		// Bad Memory
		if (info.State != MEM_COMMIT) return false;
		if (info.Type != MEM_PRIVATE) return false;
		if (info.Protect != PAGE_READWRITE) return false;

		return true;
	}

	return false;
}

bool TArrayFinder::IsValidTArray(const uintptr_t address)
{
	// Is Valid Pointer
	uintptr_t nTArray;
	if (!IsValidPointer(address, nTArray)) return false;

	// Check MaxElements and NumElements
	const int max_elements = _memory->ReadInt(address + ptr_size + 0x0);
	const int num_elements = _memory->ReadInt(address + ptr_size + 0x4);

	if (max_elements <= 0 || num_elements <= 0) return false;
	if (max_elements == num_elements) return false;
	if (max_elements > max_size || num_elements < min_size) return false;
	// if (num_elements > max_elements) return false;
	if (abs(max_elements - num_elements) >= max_size_min) return false;

	// Check GObjects
	// ObjFirstGCIndex = (ObjLastNonGCIndex - 1) & MaxObjectsNotConsideredByGC = ObjFirstGCIndex, OpenForDisregardForGC = 0(not sure)
	const int obj_first_gc_index = _memory->ReadInt(address - 0x10);
	const int obj_last_non_gc_index = _memory->ReadInt(address - 0xC);
	const int max_objects_not_considered_by_gc = _memory->ReadInt(address - 0x8);
	//const int openForDisregardForGC = _memory->ReadInt(address - 0x4);

	if (obj_first_gc_index < 0x10) return false;
	// if (obj_first_gc_index != obj_last_non_gc_index + 1) return false;
	// if (max_objects_not_considered_by_gc != obj_first_gc_index) return false;
	// if (openForDisregardForGC != 0) return false;

	// Check (UObject*) Is Valid Pointer
	uintptr_t addressUObject;
	if (!IsValidPointer(nTArray, addressUObject)) return false;
	
	// Check (ClusterIndex)
	const int clusterIndex = _memory->ReadInt(addressUObject + ptr_size + 0x8); // Skip Flag
	if (clusterIndex < 0 || uintptr_t(clusterIndex) > dwEnd) return false;

	// Check (SerialNumber)
	const int serialNumber = _memory->ReadInt(addressUObject + ptr_size + 0xC);
	if (serialNumber < 0 || uintptr_t(serialNumber) > dwEnd) return false;

	return true;
}