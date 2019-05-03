#pragma once
#include "Memory.h"

#define OBJECT_ERROR 1
#define VFTABLE_ERROR 2
#define INDEX_ERROR 3

class TArrayFinder
{
	Memory* _memory = nullptr;
	int ptr_size = 0x0;
	//const int min_size = 0x1;
	uintptr_t dwStart, dwEnd;

	bool IsValidPointer(uintptr_t address, uintptr_t& pointer, bool checkIsAllocationBase);
	DWORD IsValidTArray(uintptr_t address);
	DWORD IsValidTArray2(uintptr_t address);
public:
	TArrayFinder(Memory* memory);
	void Find();
};
