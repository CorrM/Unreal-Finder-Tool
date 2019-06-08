#pragma once

#define OBJECT_ERROR 1
#define VFTABLE_ERROR 2
#define INDEX_ERROR 3
#include <vector>

class GObjectsFinder
{
	bool easyMethod = false;
	int ptrSize = 0x0;
	uintptr_t dwStart, dwEnd;

public:
	explicit GObjectsFinder(bool easyMethod);
	std::vector<uintptr_t> Find();
};
