#pragma once
#include <string>
#include <vector>
#include "Memory.h"
#include "JsonReflector.h"

struct MySettings
{
	struct _SdkGenSettings
	{
		std::string CorePackageName;

		std::string MemoryHeader;
		std::string MemoryRead;
		std::string MemoryWrite;
		std::string MemoryWriteType;

		int Threads = 0;
		bool DumpObjects = false;
		bool DumpNames = false;
		bool LoggerShowSkip = true;
		bool LoggerShowClassSaveFileName = true;
		bool LoggerShowStructSaveFileName = true;
		int LoggerSpaceCount = 85;
	};

	struct _Parallel
	{
		int SleepEvery = 0;
	};

	_SdkGenSettings SdkGen;
	_Parallel Parallel;
};

class Utils
{
public:
	// Tool settings container
	static MySettings Settings;
	// Main Memory reader for read game memory props
	static Memory* MemoryObj;

	static bool LoadJsonCore();
	static bool LoadSettings();
	static std::vector<std::string> SplitString(const std::string& str, const std::string& delimiter);
	static std::string ReplaceString(std::string str, const std::string& to_find, const std::string& to_replace);
	static bool ProgramIs64();
	static int BufToInteger(void* buffer);
	static int64_t BufToInteger64(void* buffer);
	static uintptr_t CharArrayToUintptr(std::string str);
	static bool IsNumber(const std::string& s);
	// Return size of pointer in target game
	static int PointerSize();
	// Check valid address in remote process
	static bool IsValidAddress(Memory* mem, uintptr_t address);
	// Check valid address in local process
	static bool IsValidAddress(uintptr_t address);
	// Check valid pointer in remote process, (Read address and check it's value is valid address)
	static bool IsValidPointer(uintptr_t address, uintptr_t& pointer);
	// Check if Address is point GNames Array
	static bool IsValidGNamesAddress(uintptr_t address);
	// Check if Address is point GObjects Array
	static bool IsValidGObjectsAddress(uintptr_t address);
	// Sleep when counter hit each selected ms
	static void SleepEvery(int ms, int& counter, int every);

	/// <summary>
	/// Fix pointer size in struct or class. used for convert 64bit to 32bit pointer.
	/// </summary>
	/// <param name="structBase">Pointer to instance of `ElementType`</param>
	/// <param name="varOffset">Offset to variable based on `ElementType`</param>
	template <typename ElementType>
	static void FixPointer(ElementType* structBase, const int varOffset)
	{
		if (ProgramIs64() && MemoryObj->Is64Bit)
			return;
		FixStructPointer(structBase, varOffset, sizeof(ElementType));
	}
	/// <summary>
	/// Fix pointers size in struct or class. used for convert 64bit to 32bit pointer.
	/// </summary>
	/// <param name="structBase">Pointer to instance of `ElementType`</param>
	/// <param name="fullStructSize">Full size of struct => (Base Structs + Target struct)</param>
	/// <param name="varsOffsets">Offsets to variables based on `ElementType`</param>
	template <typename ElementType>
	static void FixPointers(ElementType* structBase, const int fullStructSize, std::vector<int> varsOffsets)
	{
		if (ProgramIs64() && MemoryObj->Is64Bit)
			return;

		for (int varOff : varsOffsets)
			FixStructPointer(structBase, varOff, fullStructSize);
	}
	/// <summary>
	/// Fix all pointers in struct to be safe to read as `uintptr_t`
	/// </summary>
	/// <param name="structBase">Pointer to instance of struct</param>
	/// <param name="is64BitGame">Target game is 64bit game</param>
	static void FixPointersInJsonStruct(JsonStruct * structBase, bool is64BitGame);
private:
	/// <summary>
	/// Fix pointer size in struct or class. used for convert 64bit to 32bit pointer.
	/// Must pick the right struct Size, else will case some memory problems in the hole program
	/// </summary>
	/// <param name="structBase">Pointer to instance of struct</param>
	/// <param name="varOffset">Offset to variable based on `structBase`</param>
	/// <param name="structSize">Size of struct</param>
	static void FixStructPointer(void* structBase, int varOffset, int structSize);
	/// <summary>
	/// Read pointer for 32bit and 64bit games
	/// </summary>
	/// <param name="structBase">Pointer to instance of struct</param>
	/// <param name="varOffset">Offset to variable based on `structBase`</param>
	/// <param name="structSize">Size of struct</param>
	/// <param name="is64BitGame">Target game is 64bit game</param>
	static uintptr_t ReadPointer(void* structBase, int varOffset, int structSize, bool is64BitGame);
	/// <summary>
	/// Read pointer for 32bit and 64bit games, Use `FixPointersInStruct` Instead
	/// </summary>
	/// <param name="structBase">Pointer to instance of struct</param>
	/// <param name="varName">Size of struct</param>
	/// <param name="is64BitGame">Target game is 64bit game</param>
	static uintptr_t ReadPointer(JsonStruct* structBase, const std::string& varName, bool is64BitGame);
};
