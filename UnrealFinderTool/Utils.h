#pragma once
#include <string>
#include <vector>
#include "Memory.h"
#include "JsonReflector.h"

class Utils
{
public:
	// Main Memory reader for read game memory props
	static Memory* MemoryObj;

	static bool LoadJsonCore();
	static std::vector<std::string> SplitString(const std::string& str, const std::string& delimiter);
	static std::string ReplaceString(std::string str, const std::string& to_find, const std::string& to_replace);
	static bool ProgramIs64();
	static DWORD BufToInteger(void* buffer);
	static DWORD64 BufToInteger64(void* buffer);
	static bool IsNumber(const std::string& s);
	/// <summary>
	/// Fix pointer size in struct or class. used for convert 64bit to 32bit pointer.
	/// Must pick the right `ElementType`, else will case some memory problems in the hole program
	/// </summary>
	/// <param name="structBase">Pointer to instance of `ElementType`</param>
	/// <param name="varOffsetEach4Byte">Offset to variable based on `ElementType`</param>
	template <typename ElementType> static void FixStructPointer(void* structBase, int varOffsetEach4Byte);
	/// <summary>
	/// Fix all pointers in struct to be safe to read as `uintptr_t`
	/// </summary>
	/// <param name="structBase">Pointer to instance of struct</param>
	/// <param name="is64BitGame">Target game is 64bit game</param>
	static void FixPointersInJsonStruct(JsonStruct* structBase, bool is64BitGame);
	static int PointerSize();
	// Check valid address in remote process
	static bool IsValidAddress(Memory* mem, uintptr_t address);
	// Check valid address in local process
	static bool IsValidAddress(uintptr_t address);

private:
	/// <summary>
	/// Fix pointer size in struct or class. used for convert 64bit to 32bit pointer.
	/// Must pick the right struct Size, else will case some memory problems in the hole program
	/// </summary>
	/// <param name="structBase">Pointer to instance of struct</param>
	/// <param name="varOffsetEach4Byte">Offset to variable based on `structBase`</param>
	/// <param name="structSize">Size of struct</param>
	static void FixStructPointer(void* structBase, int varOffsetEach4Byte, int structSize);
	/// <summary>
	/// Read pointer for 32bit and 64bit games
	/// </summary>
	/// <param name="structBase">Pointer to instance of struct</param>
	/// <param name="varOffsetEach4Byte">Offset to variable based on `structBase`</param>
	/// <param name="structSize">Size of struct</param>
	/// <param name="is64BitGame">Target game is 64bit game</param>
	static uintptr_t ReadSafePointer(void* structBase, int varOffsetEach4Byte, int structSize, bool is64BitGame);
	/// <summary>
	/// Read pointer for 32bit and 64bit games, Use `FixPointersInStruct` Instead
	/// </summary>
	/// <param name="structBase">Pointer to instance of struct</param>
	/// <param name="varName">Size of struct</param>
	/// <param name="is64BitGame">Target game is 64bit game</param>
	static uintptr_t ReadSafePointer(JsonStruct* structBase, const std::string& varName, bool is64BitGame);
};
