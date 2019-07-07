#include "pch.h"
#include "Memory.h"
#include "Scanner.h"
#include "ParallelWorker.h"
#include "Utils.h"
#include "GObjectsFinder.h"
#include "JsonReflector.h"

/*
 * #NOTES
 * Some GObject use this syntax every 4byte/8byte (pointer) there is UObject
 */

GObjectsFinder::GObjectsFinder(const bool easyMethod) : easyMethod(easyMethod)
{
	dwStart = 0;
	dwEnd = 0;
	ptrSize = !Utils::MemoryObj->Is64 ? 0x4 : 0x8;
}

void GObjectsFinder::Find(std::vector<uintptr_t>& out)
{
	// dwStart = !_memory->Is64Bit ? 0x100000 : static_cast<uintptr_t>(0x7FF00000);
	dwEnd = !Utils::MemoryObj->Is64 ? 0x7FEFFFFF : uintptr_t(0x7fffffffffff);

	// Start scan for TArrays
	SYSTEM_INFO si;
	GetSystemInfo(&si);

	if (dwStart < reinterpret_cast<uintptr_t>(si.lpMinimumApplicationAddress))
		dwStart = reinterpret_cast<uintptr_t>(si.lpMinimumApplicationAddress);

	if (dwEnd > reinterpret_cast<uintptr_t>(si.lpMaximumApplicationAddress))
		dwEnd = reinterpret_cast<uintptr_t>(si.lpMaximumApplicationAddress);

	MEMORY_BASIC_INFORMATION info = { nullptr };
	std::vector<uintptr_t> mem_block;

	// Cycle through memory based on RegionSize
	{
		uintptr_t currentAddress = dwStart;
		bool exitLoop;
		// TODO: Change HARD method to be faster
		// BODY: to make HARD method faster create a vector and add address for each 0x1000, then use Parallel Class
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

		ParallelQueue<std::vector<uintptr_t>, uintptr_t> worker2(mem_block, 0, Utils::Settings.SdkGen.Threads, [&out](uintptr_t& address, ParallelOptions& options)
		{
			// Insert region information on Regions Holder
			if (Utils::IsValidGObjectsAddress(address))
			{
				std::lock_guard lock(options.Locker);
				out.push_back(address);
			}
		});
		worker2.Start();
		worker2.WaitAll();
	}

	// Check if there a GObjects Chunks
	{
		using namespace Hyperscan;
		std::vector<uintptr_t> search_result;
		for (size_t index = 0; index < out.size(); ++index)
		{
			auto address_holder = HYPERSCAN_SCANNER::Scan(Utils::MemoryObj->ProcessId, out[index],
				Utils::MemoryObj->Is64 ? HyperscanAllignment8Bytes : HyperscanAllignment4Bytes, HyperscanTypeExact);

			if (address_holder.empty())
			{
				out.erase(index == 0 ? out.begin() : out.begin() + index);
				continue;
			}

			for (uintptr_t address_ptr : address_holder)
			{
				if (!Utils::MemoryObj->IsStaticAddress(address_ptr))
				{
					auto address_holder2 = HYPERSCAN_SCANNER::Scan(Utils::MemoryObj->ProcessId, address_ptr,
						Utils::MemoryObj->Is64 ? HyperscanAllignment8Bytes : HyperscanAllignment4Bytes, HyperscanTypeExact);

					for (auto holder2Address : address_holder2)
						search_result.push_back(holder2Address);

					if (!address_holder2.empty())
						continue;
				}
				
				search_result.push_back(address_ptr);
			}
		}
		out.insert(out.end(), search_result.begin(), search_result.end());
	}
}
