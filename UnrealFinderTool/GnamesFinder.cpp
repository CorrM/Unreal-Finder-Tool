#include "pch.h"
#include "GnamesFinder.h"
#include "Color.h"

Pattern GnamesFinder::none_sig = PatternScan::Parse("None", 0, "4E 6F 6E 65 00", 0xFF);
Pattern GnamesFinder::byte_sig = PatternScan::Parse("Byte", 0, "42 79 74 65 50 72 6F 70 65 72 74 79 00", 0xFF);
Pattern GnamesFinder::int_sig = PatternScan::Parse("Int", 0, "49 6E 74 50 72 6F 70 65 72 74 79 00", 0xFF);
Pattern GnamesFinder::multicast_sig = PatternScan::Parse("MulticastDelegate", 0, "4D 75 6C 74 69 63 61 73 74 44 65 6C 65 67 61 74 65 50 72 6F 70 65 72 74 79", 0xFF);

GnamesFinder::GnamesFinder(Memory* memory) : _memory(memory) { }

void GnamesFinder::Find()
{
	Color green(LightGreen, Black);
	Color def(White, Black);
	Color yellow(LightYellow, Black);
	Color purple(LightPurple, Black);
	Color red(LightRed, Black);
	Color dgreen(Green, Black);

	const uintptr_t start = !_memory->Is64Bit ? 0x300000 : static_cast<uintptr_t>(0x7FF00000);
	const uintptr_t end = !_memory->Is64Bit ? 0x7FEFFFFF : static_cast<uintptr_t>(0x7fffffffffff);

	std::cout << dgreen << "[!] " << def << "Start scan for GNames. (at 0x" << std::hex << start << ". to 0x" << std::hex << end  << ")" << std::endl << def;

	// Scan
	std::vector<Pattern> inputs;
	inputs.push_back(none_sig);
	inputs.push_back(byte_sig);
	inputs.push_back(int_sig);
	inputs.push_back(multicast_sig);

	const auto searcher = PatternScan::FindPattern(_memory, start, end, inputs);

	if (searcher.find(none_sig.Name) == searcher.end())
	{
		std::cout << red << "[-] " << yellow << "Not found anything" << std::endl << def;
		return;
	}

	const auto none_r = searcher.find(none_sig.Name)->second;
	const auto byte_r = searcher.find(byte_sig.Name)->second;
	const auto int_r = searcher.find(int_sig.Name)->second;
	const auto multicast_r = searcher.find(multicast_sig.Name)->second;

	// Get smallest address
	const auto cmp1 = GetNearNumbers(none_r, byte_r, 0x150);
	const auto cmp2 = GetNearNumbers(cmp1, int_r, 0x150);
	const auto cmp3 = GetNearNumbers(cmp2, multicast_r, 0x400);

	for (long long i : cmp3)
	{
		i = i - (!_memory->Is64Bit ? 0x8 : 0x10);
		std::cout << green << "[+] " << def << "\t" << red << "0x" << std::hex << i << std::endl;
	}
	std::cout << purple << "[!] " << yellow << "Found " << cmp3.size() << " Address." << std::endl << def;

	if (!cmp3.empty())
	{
		std::cout << red << "[*] " << green << "Address is first FName." << std::endl;
		std::cout << red << "[*] " << green << "So you must get the pointer how point the address. (FNameEntity)" << std::endl;
		std::cout << red << "[*] " << green << "And then need to find the pointer how point the pointer you get. (GNames)" << std::endl;
	}
}

std::vector<uintptr_t> GnamesFinder::GetNearNumbers(const std::vector<uintptr_t>& list1, const std::vector<uintptr_t>& list2, int maxValue)
{
	std::vector<uintptr_t> ret;
	for (long long i : list1)
		for (long long i2 : list2)
			if (abs(i - i2) <= maxValue)
				ret.push_back(i);
	return ret;
}