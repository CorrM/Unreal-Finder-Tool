#pragma once
#include <string>
#include <map>
#include <vector>
#include "Process.h"

class Memory;

typedef unsigned char uchar_t;

struct Pattern
{
	std::string Name;
	std::vector<uchar_t> Sig;
	size_t Len;
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
	static std::map<std::string, std::vector<uintptr_t>> FindPattern(Memory* mem, uintptr_t dwStart, uintptr_t dwEnd, std::vector<Pattern> patterns, bool firstOnly = false);
};