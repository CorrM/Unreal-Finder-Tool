#pragma once
#include "Memory.h"
#include <vector>

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

	~TUObjectArray()
	{
		delete[] Objects;
	}
};

class FUObjectArray
{
public:
	int ObjFirstGcIndex;
	int ObjLastNonGcIndex;
	int MaxObjectsNotConsideredByGc;
	int OpenForDisregardForGc;

	TUObjectArray ObjObjects;
};
#pragma endregion

#pragma region GNames
struct FName
{
	int Index;
	char pad_0x0004[0x4];
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

	char* GetName(UObject* object, bool& success);
	void ObjectDump();
	void NameDump();
	bool ReadUObjectArray(uintptr_t address, FUObjectArray& objectArray);
	bool ReadGNameArray(uintptr_t address, GNameArray& gNames);
	static DWORD BufToInteger(void* buffer);
	static DWORD64 BufToInteger64(void* buffer);
	bool IsValidAddress(uintptr_t address) const;
	template <class ElementType>
	void FixStructPointer(void* structBase, int varOffsetEach4Byte);

public:
	InstanceLogger(Memory* memory, uintptr_t gObjObjectsAddress, uintptr_t gNamesAddress);
	static bool ProgramIs64();
	void Start();
};

