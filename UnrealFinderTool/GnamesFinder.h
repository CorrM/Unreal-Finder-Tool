#pragma once
#include "Memory.h"
#include "PatternScan.h"
#include <ostream>

class GnamesFinder
{
	Memory* _memory = nullptr;
	static Pattern none_sig, byte_sig, int_sig, multicast_sig;
	std::vector<uintptr_t> GetNearNumbers(const std::vector<uintptr_t>& list1, const std::vector<uintptr_t>& list2, int maxValue);
public:
	GnamesFinder(Memory* memory);
	void Find();
};

