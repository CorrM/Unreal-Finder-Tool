#pragma once
#include "Memory.h"
#include <vector>

template <class T> struct TArray
{
	T*		Data;
	DWORD   Num;
	DWORD   Max;
};

#pragma region GObject
struct UObject
{
	void*	VfTable;
	int		Flags;
	int		InternalIndex;
	void*	Unknown00;
	DWORD	FNameIndex; // struct FName
	DWORD	FNameUnknown;

	uintptr_t	OriginalAddress; // Must be in last
};

class FUObjectItem
{
public:
	UObject*	Object;
	int			Flags;
	int			ClusterIndex;
	int			SerialNumber;
	char		Unknowndata00[0x4];

	~FUObjectItem()
	{
		delete Object;
	}
};

class TUObjectArray
{
public:
	FUObjectItem* Objects;
	int MaxElements;
	int NumElements;
};

class FUObjectArray
{
public:
	int ObjFirstGcIndex;				//0x0000
	int ObjLastNonGcIndex;				//0x0004
	int MaxObjectsNotConsideredByGc;	//0x0008
	int OpenForDisregardForGc;			//0x000C

	TUObjectArray ObjObjects;
};
#pragma endregion

#pragma region GNames
struct FName
{
	int Index;
	//char pad_0x0004[0x4];
	uintptr_t HashNext; // FName*
	string AnsiName;
};

class GNameArray
{
public:
	std::vector<FName> Names;
	int NumElements = 16384;
};
#pragma endregion

class InstanceLogger
{
	FUObjectArray gObjObjects;
	GNameArray gNames;

	Memory* _memory;
	uintptr_t gObjObjectsOffset;
	uintptr_t gNamesOffset;
	int ptrSize;

	bool DataCompare(const BYTE* pData, const BYTE* bMask, const char* szMask);
	char* GetName(UObject* object, bool& success);
	void ObjectDump();
	void NameDump();
	bool ReadUObjectArray(uintptr_t address, FUObjectArray& objectArray);
	bool ReadGNameArray(uintptr_t address, GNameArray& gNames);
	DWORD BufToInteger(void* buffer);
	DWORD64 BufToInteger64(void* buffer);
	bool IsValidAddress(const uintptr_t address);
	template <class ElementType>
	void FixStructPointer(void* structBase, int varOffsetEach4Byte);

public:
	InstanceLogger(Memory* memory, uintptr_t gObjObjectsAddress, uintptr_t gNamesAddress);
	bool ProgramIs64();
	void Start();
};

