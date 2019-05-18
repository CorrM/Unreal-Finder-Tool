#pragma once
#include "PatternScan.h"

class GnamesFinder
{
	uintptr_t dwStart, dwEnd;
	static Pattern none_sig, byte_sig, int_sig, multicast_sig;
	static std::vector<uintptr_t> GetNearNumbers(const std::vector<uintptr_t>& list1, const std::vector<uintptr_t>& list2, int maxValue);
public:
	GnamesFinder();
	void Find();
};

