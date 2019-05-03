#pragma once
#include "Memory.h"

#define SUCCESS_ERROR 0
#define OBJECT_ERROR 1
#define VFTABLE_ERROR 2
#define INDEX_ERROR 3

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
	DWORD IsValidTArray(uintptr_t address, FUObjectArray& tArray);
	DWORD IsValidTArray2(uintptr_t address, FUObjectArray& tArray);
public:
	TArrayFinder(Memory* memory);
	void Find();
};
