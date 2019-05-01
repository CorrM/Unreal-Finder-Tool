#pragma once
#include "Memory.h"

#define INVALID_POINTER_VALUE(x) (x == (uintptr_t)-1) || x == NULL

class TArrayFinder
{
	Memory* _memory = nullptr;
	int ptr_size = 0x0;
	int max_size_min = 0x500;

	bool IsValidPointer(uintptr_t address, uintptr_t& pointerVal);
	bool IsValidTArray(uintptr_t address);
public:
	TArrayFinder(Memory* memory);
	void Find();
};
