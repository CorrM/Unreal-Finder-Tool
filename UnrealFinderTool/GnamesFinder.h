#pragma once
#include "PatternScan.h"

class GNamesFinder
{
	uintptr_t dwStart, dwEnd;
	static Pattern noneSig, byteSig, intSig, multicastSig;
	static std::vector<uintptr_t> GetNearNumbers(const std::vector<uintptr_t>& list1, const std::vector<uintptr_t>& list2, int maxValue);
public:
	GNamesFinder();
	std::vector<uintptr_t> Find();
};

