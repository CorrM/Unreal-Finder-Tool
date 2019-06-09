#include "pch.h"
#include "Memory.h"
#include "GObjectsFinder.h"
#include "Scanner.h"

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

	MEMORY_BASIC_INFORMATION info;

	// Cycle through memory based on RegionSize
	for (uintptr_t i = dwStart; (VirtualQueryEx(Utils::MemoryObj->ProcessHandle, LPVOID(i), &info, sizeof info) == sizeof info && i < dwEnd); i += easyMethod ? info.RegionSize : si.dwPageSize)
	{
		// Bad Memory
		if (info.State != MEM_COMMIT) continue;
		if (info.Protect != PAGE_EXECUTE_READWRITE && info.Protect != PAGE_READWRITE) continue;

		if (Utils::IsValidGObjectsAddress(i))
			ret.push_back(i);
	}

	// Check if there a GObjects Chunks
	{
		using namespace Hyperscan;
		for (size_t index = 0; index < ret.size(); ++index)
		{
			auto address_holder = HYPERSCAN_SCANNER::Scan(Utils::MemoryObj->ProcessId, ret[index],
				Utils::MemoryObj->Is64Bit ? HyperscanAllignment8Bytes : HyperscanAllignment4Bytes, HyperscanTypeExact);

			if (address_holder.empty())
			{
				ret.erase(index == 0 ? ret.begin() : ret.begin() + index);
				continue;
			}

			for (uintptr_t address_ptr : address_holder)
			{
				if (!Memory::IsStaticAddress(address_ptr))
					ret.push_back(address_ptr);
			}
		}
	}

	return ret;
}
