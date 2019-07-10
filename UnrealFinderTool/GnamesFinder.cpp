#include "pch.h"
#include "Scanner.h"
#include "Memory.h"
#include "Utils.h"
#include "GnamesFinder.h"

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
	dwEnd = !Utils::MemoryObj->Is64 ? 0x7FEFFFFF : static_cast<uintptr_t>(0x7fffffffffff);

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

	size_t nameOffset = Utils::MemoryObj->Is64 ? 0x10 : 0x8;
	uintptr_t byteAddress = 0;

	// Calc Name Offset
	if (!cmp3.empty())
		nameOffset = Utils::CalcNameOffset(cmp3[0]);

	// Get Static Address
	{
		using namespace Hyperscan;
		std::vector<uintptr_t> search_result;
		for (size_t index = 0; index < cmp3.size(); ++index)
		{
			auto scanVal = GetChunksAddress(cmp3[index] - nameOffset);

			auto address_holder = HYPERSCAN_SCANNER::Scan(Utils::MemoryObj->ProcessId, scanVal,
				Utils::MemoryObj->Is64 ? HyperscanAllignment8Bytes : HyperscanAllignment4Bytes, HyperscanTypeExact);

			if (address_holder.empty())
			{
				ret.erase(index == 0 ? ret.begin() : ret.begin() + index);
				continue;
			}

			for (size_t iStatic = 0; iStatic < address_holder.size() && iStatic < 3; ++iStatic)
			{
				search_result.push_back(address_holder[iStatic]);
			}
		}

		ret.insert(ret.end(), search_result.begin(), search_result.end());
	}

	return ret;
}

uintptr_t GNamesFinder::GetChunksAddress(const uintptr_t fNameAddress)
{
	using namespace Hyperscan;
	uintptr_t ret = fNameAddress;

	// Get GName array address
	auto address_holder = HYPERSCAN_SCANNER::Scan(Utils::MemoryObj->ProcessId, fNameAddress,
		Utils::MemoryObj->Is64 ? HyperscanAllignment8Bytes : HyperscanAllignment4Bytes, HyperscanTypeExact);

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
				Utils::MemoryObj->Is64 ? HyperscanAllignment8Bytes : HyperscanAllignment4Bytes, HyperscanTypeExact);
			for (uintptr_t chunk_address : gname_array_address)
			{
				if (Utils::IsValidGNamesChunksAddress(chunk_address))
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