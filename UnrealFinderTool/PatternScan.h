#pragma once
#include "Utils.h"
#include "Process.h"
#include <vector>
#include "Memory.h"
#include <map>

typedef unsigned char uchar_t;

struct Pattern
{
	std::string Name;
	std::vector<uchar_t> Sig;
	int Len;
	int Offset;
	uchar_t Wildcard;
};

struct MemoryRegion
{
	uintptr_t Address;
	SIZE_T RegionSize;
};

class PatternScan
{
	static Pattern Parse(const std::string& name, int offset, std::string hexStr, uchar_t wildcard, const std::string& delimiter);
public:
	static Pattern Parse(const std::string& name, int offset, const std::string& patternStr, uchar_t wildcard);
	static std::map<string, std::vector<uintptr_t>> FindPattern(Memory* mem, uintptr_t dwStart, uintptr_t dwEnd, std::vector<Pattern> patterns, bool firstOnly = false);
};