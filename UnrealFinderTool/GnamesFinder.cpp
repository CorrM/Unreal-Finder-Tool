#include "pch.h"
#include "GnamesFinder.h"
#include "Scanner.h"
#include "Memory.h"

Pattern GNamesFinder::noneSig = PatternScan::Parse("None", 0, "4E 6F 6E 65 00", 0xFF);
Pattern GNamesFinder::byteSig = PatternScan::Parse("Byte", 0, "42 79 74 65 50 72 6F 70 65 72 74 79 00", 0xFF);
Pattern GNamesFinder::intSig = PatternScan::Parse("Int", 0, "49 6E 74 50 72 6F 70 65 72 74 79 00", 0xFF);
Pattern GNamesFinder::multicastSig = PatternScan::Parse("MulticastDelegate", 0, "4D 75 6C 74 69 63 61 73 74 44 65 6C 65 67 61 74 65 50 72 6F 70 65 72 74 79", 0xFF);

GNamesFinder::GNamesFinder() : dwStart(0), dwEnd(0)
{
}

std::vector<uintptr_t> GNamesFinder::Find()
{
	std::vector<uintptr_t> ret;
	// dwStart = !_memory->Is64Bit ? 0x300000 : static_cast<uintptr_t>(0x7FF00000);
	dwEnd = !Utils::MemoryObj->Is64Bit ? 0x7FEFFFFF : static_cast<uintptr_t>(0x7fffffffffff);

	// Scan
	std::vector<Pattern> inputs = { noneSig, byteSig, intSig, multicastSig };
	const auto searcher = PatternScan::FindPattern(Utils::MemoryObj, dwStart, dwEnd, inputs, false, true);

	if (searcher.find(noneSig.Name) == searcher.end())
		return ret;

	const auto none_r = searcher.find(noneSig.Name)->second;
	const auto byte_r = searcher.find(byteSig.Name)->second;
	const auto int_r = searcher.find(intSig.Name)->second;
	const auto multicast_r = searcher.find(multicastSig.Name)->second;

	// Get smallest address
	const auto cmp1 = GetNearNumbers(none_r, byte_r, 0x150);
	const auto cmp2 = GetNearNumbers(cmp1, int_r, 0x150);
	const auto cmp3 = GetNearNumbers(cmp2, multicast_r, 0x400);

	size_t nameOffset = Utils::MemoryObj->Is64Bit ? 0x10 : 0x8;
	uintptr_t byteAddress = 0;

	// Calc Name Offset
	if (!cmp3.empty())
	{
		// None == 0
		auto byte = PatternScan::Parse("Byte", 0, "42 79 74 65 50 72 6F 70 65 72 74 79 00", 0xFF);
		auto result = PatternScan::FindPattern(Utils::MemoryObj, cmp3[0], cmp3[0] + 0x200, { byte }, true);
		auto it = result.find("Byte");
		if (it != result.end() && !it->second.empty())
		{
			byteAddress = it->second.front();
			auto byte_index = PatternScan::Parse("ByteIndex", 0, "02 00 00 00", 0xFF);
			result = PatternScan::FindPattern(Utils::MemoryObj, byteAddress - 0x20, byteAddress, { byte_index }, true);
			it = result.find("ByteIndex");
			if (it != result.end() && !it->second.empty())
				nameOffset = byteAddress - it->second.front();
		}
	}

	for (uintptr_t i : cmp3)
	{
		i = i - nameOffset;
		ret.push_back(GetChunksAddress(i));
	}

	return ret;
}

uintptr_t GNamesFinder::GetChunksAddress(const uintptr_t fname_address)
{
	using namespace Hyperscan;
	uintptr_t ret = fname_address;

	// Get GName array address
	auto address_holder = HYPERSCAN_SCANNER::Scan(Utils::MemoryObj->ProcessId, fname_address,
		Utils::MemoryObj->Is64Bit ? HyperscanAllignment8Bytes : HyperscanAllignment4Bytes, HyperscanTypeExact);

	// Nothing returned quit
	if (!address_holder.empty())
	{
		for (uintptr_t i : address_holder)
		{
			// Any address larger than this is usually garbage
			if (i > uintptr_t(0x7ff000000000))
				continue;

			// Scan for Gnames chunks address
			auto gname_array_address = HYPERSCAN_SCANNER::Scan(Utils::MemoryObj->ProcessId, i,
				Utils::MemoryObj->Is64Bit ? HyperscanAllignment8Bytes : HyperscanAllignment4Bytes, HyperscanTypeExact);
			for (uintptr_t chunk_address : gname_array_address)
			{
				if (Utils::IsValidGNamesAddress(chunk_address))
				{
					ret = chunk_address;
					break;
				}
			}
		}
	}

	return ret;
}

std::vector<uintptr_t> GNamesFinder::GetNearNumbers(const std::vector<uintptr_t>& list1, const std::vector<uintptr_t>& list2, const int maxValue)
{
	std::vector<uintptr_t> ret;
	for (long long i : list1)
		for (long long i2 : list2)
			if (abs(i - i2) <= maxValue)
				ret.push_back(i);
	return ret;
}