#pragma once

class UWorldFinder
{
public:
	std::vector<uintptr_t> Find(uintptr_t gObjAddress, uintptr_t gNamesAddress);
};

