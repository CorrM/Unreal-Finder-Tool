#include "pch.h"
#include "Memory.h"
#include "GObjectsFinder.h"
#include "Scanner.h"
#include "ParallelWorker.h"

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
	dwEnd = !Utils::MemoryObj->Is64Bit ? 0x7FEFFFFF : uintptr_t(0x7fffffffffff);

	// Start scan for TArrays
	SYSTEM_INFO si = { 0 };
	GetSystemInfo(&si);

	if (dwStart < reinterpret_cast<uintptr_t>(si.lpMinimumApplicationAddress))
		dwStart = reinterpret_cast<uintptr_t>(si.lpMinimumApplicationAddress);

	if (dwEnd > reinterpret_cast<uintptr_t>(si.lpMaximumApplicationAddress))
		dwEnd = reinterpret_cast<uintptr_t>(si.lpMaximumApplicationAddress);

	MEMORY_BASIC_INFORMATION info = { 0 };
	std::vector<uintptr_t> mem_block;

	// Cycle through memory based on RegionSize
	{
		uintptr_t currentAddress = dwStart;
		bool exitLoop = false;

		do
		{
			exitLoop = !(VirtualQueryEx(Utils::MemoryObj->ProcessHandle, reinterpret_cast<LPVOID>(currentAddress), &info, sizeof info) == sizeof info && currentAddress < dwEnd);

			// Size will used to alloc and read memory
			const size_t allocSize = dwEnd - dwStart >= (easyMethod ? info.RegionSize : si.dwPageSize) ? (easyMethod ? info.RegionSize : si.dwPageSize) : dwEnd - dwStart;

			// Bad Memory
			if (info.Protect & PAGE_NOACCESS)
			{
				// Get next address
				currentAddress += allocSize;
				continue;
			}

			// Get next address
			currentAddress += allocSize;
			mem_block.push_back(currentAddress);

		} while (!exitLoop);

		ParallelQueue<std::vector<uintptr_t>, uintptr_t> worker2(mem_block, 0, Utils::Settings.SdkGen.Threads, [&ret](uintptr_t& address, ParallelOptions& options)
		{
			// Insert region information on Regions Holder
			if (Utils::IsValidGObjectsAddress(address))
			{
				std::lock_guard lock(options.Locker);
				ret.push_back(address);
			}
		});
		worker2.Start();
		worker2.WaitAll();
	}

	// Check if there a GObjects Chunks
	{
		using namespace Hyperscan;
		std::vector<uintptr_t> search_result;
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
				bool isChunks;
				if (Utils::IsValidGObjectsAddress(address_ptr, &isChunks) && isChunks && !Utils::MemoryObj->IsStaticAddress(address_ptr))
					search_result.push_back(address_ptr);
			}
		}
		ret.insert(ret.end(), search_result.begin(), search_result.end());
	}

	return ret;
}
