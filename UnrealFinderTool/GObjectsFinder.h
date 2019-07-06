#pragma once

enum class FinderError
{
	ObjectError,
	VfTableError,
	IndexError
};

#include <vector>

class GObjectsFinder
{
	bool easyMethod = false;
	int ptrSize = 0x0;
	uintptr_t dwStart, dwEnd;

public:
	explicit GObjectsFinder(bool easyMethod);
	void Find(std::vector<uintptr_t>& out);
};
