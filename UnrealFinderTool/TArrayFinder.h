#pragma once
#include "Memory.h"

struct FUObjectArray
{
	uintptr_t Data;
	int Max, Num;
};

class TArrayFinder
{
	Memory* _memory = nullptr;
	int ptr_size = 0x0;
	//const int min_size = 0x1;
	int dwBetweenObjects, dwInternalIndex;
	uintptr_t dwStart, dwEnd;

	bool IsValidPointer(uintptr_t address, uintptr_t& pointer, bool checkIsAllocationBase);
	bool IsValidTArray(uintptr_t address, FUObjectArray& tArray);
	bool IsValidTArray2(uintptr_t address, FUObjectArray& tArray);
public:
	TArrayFinder(Memory* memory);
	void Find();
};
