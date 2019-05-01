#include "pch.h"
#include "TArrayFinder.h"
#include "Color.h"
#include <vector>


TArrayFinder::TArrayFinder(Memory* memory) : _memory(memory)
{
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

	uintptr_t dwStart = !_memory->Is64Bit ? 0x300000 : static_cast<uintptr_t>(0x7FF00000);
	uintptr_t dwEnd = !_memory->Is64Bit ? 0x7FEFFFFF : static_cast<uintptr_t>(0x7fffffffffff);

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
		if (info.Protect != PAGE_READWRITE) continue;

		// Alloc memory
		const auto pBuf = static_cast<PBYTE>(malloc(info.RegionSize));

		// Read one page or skip if failed
		const int dwOut = _memory->ReadBytes(i, pBuf, info.RegionSize);
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

	if (INVALID_POINTER_VALUE(pointer))
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
	uintptr_t value;
	if (!IsValidPointer(address, value)) return false;

	// Check max value between MaxElements and NumElements;
	const int max_elements = _memory->ReadInt(address + ptr_size + 0x0);
	const int num_elements = _memory->ReadInt(address + ptr_size + 0x4);
	return abs(max_elements - num_elements) <= max_size_min;
}