#pragma once
#include "Memory.h"
#include <vector>
#include "JsonReflector.h"

#pragma region GNames
struct FName
{
	int Index;
	char pad_0x0004[0x4];
	uintptr_t HashNext; // FName*
	string AnsiName;
};

#pragma endregion

struct GObject
{
	uintptr_t ObjAddress;
	int FNameIndex;
};

class GName
{
public:
	int Index;
	string AnsiName;
};

class InstanceLogger
{
	// FUObjectArray gObjObjects;
	GObject* gObjObjects; int gObjectsCount;
	GName* gNames; int gNamesChunkCount, gNamesChunks;

	Memory* _memory;
	uintptr_t gObjObjectsOffset;
	uintptr_t gNamesOffset;
	int ptrSize;

	std::string GetName(int fNameIndex, bool& success);
	void ObjectDump();
	void NameDump();
	bool ReadUObjectArray(uintptr_t address, JsonStruct& objectArray);
	bool ReadGNameArray(uintptr_t address);
	static DWORD BufToInteger(void* buffer);
	static DWORD64 BufToInteger64(void* buffer);
	bool IsValidAddress(uintptr_t address);
	/// <summary>
	/// Fix pointer size in struct or class. used for convert 64bit to 32bit pointer.
	/// Must pick the right `ElementType`, else will case some memory problems in the hole program
	/// </summary>
	/// <param name="structBase">Pointer to instance of `ElementType`</param>
	/// <param name="varOffsetEach4Byte">Offset to variable based on `ElementType`</param>
	template <typename ElementType> void FixStructPointer(void* structBase, int varOffsetEach4Byte);
	/// <summary>
	/// Fix pointer size in struct or class. used for convert 64bit to 32bit pointer.
	/// Must pick the right struct Size, else will case some memory problems in the hole program
	/// </summary>
	/// <param name="structBase">Pointer to instance of struct</param>
	/// <param name="varOffsetEach4Byte">Offset to variable based on `structBase`</param>
	/// <param name="structSize">Size of struct</param>
	void FixStructPointer(void* structBase, int varOffsetEach4Byte, int structSize);

public:
	InstanceLogger(Memory* memory, uintptr_t gObjObjectsAddress, uintptr_t gNamesAddress);
	void Start();
};