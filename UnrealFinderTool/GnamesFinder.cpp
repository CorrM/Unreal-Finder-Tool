#include "pch.h"
#include "Color.h"
#include "Utils.h"
#include "GnamesFinder.h"

Pattern GNamesFinder::noneSig = PatternScan::Parse("None", 0, "4E 6F 6E 65 00", 0xFF);
Pattern GNamesFinder::byteSig = PatternScan::Parse("Byte", 0, "42 79 74 65 50 72 6F 70 65 72 74 79 00", 0xFF);
Pattern GNamesFinder::intSig = PatternScan::Parse("Int", 0, "49 6E 74 50 72 6F 70 65 72 74 79 00", 0xFF);
Pattern GNamesFinder::multicastSig = PatternScan::Parse("MulticastDelegate", 0, "4D 75 6C 74 69 63 61 73 74 44 65 6C 65 67 61 74 65 50 72 6F 70 65 72 74 79", 0xFF);

GNamesFinder::GNamesFinder() : dwStart(0), dwEnd(0)
{
}

void GNamesFinder::Find()
{
	// dwStart = !_memory->Is64Bit ? 0x300000 : static_cast<uintptr_t>(0x7FF00000);
	dwEnd = !Utils::MemoryObj->Is64Bit ? 0x7FEFFFFF : static_cast<uintptr_t>(0x7fffffffffff);

	std::cout << dgreen << "[!] " << def << "Start scan for GNames. (at 0x" << std::hex << dwStart << ". to 0x" << std::hex << dwEnd << ")" << std::endl << def;

	// Scan
	std::vector<Pattern> inputs = { noneSig, byteSig, intSig, multicastSig };
	const auto searcher = PatternScan::FindPattern(Utils::MemoryObj, dwStart, dwEnd, inputs);

	if (searcher.find(noneSig.Name) == searcher.end())
	{
		std::cout << red << "[-] " << yellow << "Not found anything" << std::endl << def;
		return;
	}

	const auto none_r = searcher.find(noneSig.Name)->second;
	const auto byte_r = searcher.find(byteSig.Name)->second;
	const auto int_r = searcher.find(intSig.Name)->second;
	const auto multicast_r = searcher.find(multicastSig.Name)->second;

	// Get smallest address
	const auto cmp1 = GetNearNumbers(none_r, byte_r, 0x150);
	const auto cmp2 = GetNearNumbers(cmp1, int_r, 0x150);
	const auto cmp3 = GetNearNumbers(cmp2, multicast_r, 0x400);

	uint32_t nameOffset = Utils::MemoryObj->Is64Bit ? 0x10 : 0x8;
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

		std::cout << green << "[+] " << def << "\t" << red << "0x" << std::hex << i << std::endl;
	}
	std::cout << purple << "[!] " << yellow << "Found " << cmp3.size() << " Address." << std::endl << def;

	if (!cmp3.empty())
	{
		std::cout << red << "[*] " << green << "Address is first FName." << std::endl;
		std::cout << red << "[*] " << green << "So you must get the pointer how point the address. (GNames Array)" << std::endl;
		std::cout << red << "[*] " << green << "And then need to find the pointer how point the pointer you get. (GNames Chunks)" << std::endl;
	}
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