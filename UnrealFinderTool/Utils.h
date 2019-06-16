#pragma once
#include "Memory.h"

#include <string>
#include <vector>
#include <filesystem>

namespace fs = std::filesystem;
#define UNREAL_WINDOW_CLASS "UnrealWindow"
class UiWindow;

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
	// Main Window
	static UiWindow* UiMainWindow;
	// Tool settings container
	static MySettings Settings;
	// Main Memory reader for read game memory props
	static Memory* MemoryObj;

	// Load settings form the file
	static bool LoadSettings();
	// Check file Exists
	static bool FileExists(const std::string& filePath);
	// Get Current Directory
	static std::string GetWorkingDirectory();
	// Load engine structs from `EngineBase.json`
	static bool LoadEngineCore(std::vector<std::string>& ue_versions_container);
	// Override engine structs that load form another engine structs, `engineVersion` must look like '4.0.0'
	static void OverrideLoadedEngineCore(const std::string& engineVersion);
	// Split string by other string
	static std::vector<std::string> SplitString(const std::string& str, const std::string& delimiter);
	// Replace string
	static std::string ReplaceString(std::string str, const std::string& to_find, const std::string& to_replace);
	// Check if string ends with other string
	static bool EndsWith(const std::string& value, const std::string& ending);
	// Determine if tool is working on x64 version. (Not Target game version)
	static bool ProgramIs64();
	// Convert Bytes to Int
	static int BufToInteger(void* buffer);
	// Convert Bytes to Int64
	static int64_t BufToInteger64(void* buffer);
	// Convert string to uintptr_t
	static uintptr_t CharArrayToUintptr(const std::string& str);
	// Convert uintptr_t to Hex string
	static std::string AddressToHex(uintptr_t address);
	// Determine if string is number
	static bool IsNumber(const std::string& s);
	// Determine if string is HEX number
	static bool IsHexNumber(const std::string& s);
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
	static bool IsValidGObjectsAddress(uintptr_t address, bool* isChunks = nullptr);
	// Sleep when counter hit each selected ms
	static void SleepEvery(int ms, int& counter, int every);

	/// <summary>
	/// Fix pointer size in struct or class. used for convert 64bit to 32bit pointer.
	/// </summary>
	/// <param name="structBase">Pointer to instance of `ElementType`</param>
	/// <param name="varOffset">Offset to variable based on `ElementType`</param>
	template <typename ElementType> static void FixPointer(ElementType* structBase, const int varOffset);
	/// <summary>
	/// Fix pointers size in struct or class. used for convert 64bit to 32bit pointer.
	/// </summary>
	/// <param name="structBase">Pointer to instance of `ElementType`</param>
	/// <param name="fullCppStructSize">Full size of struct => (Base Structs + Target struct)</param>
	/// <param name="varsOffsets">Offsets to variables based on `ElementType`</param>
	template <typename ElementType> static void FixPointers(ElementType* structBase, size_t fullCppStructSize, std::vector<int> varsOffsets);

	static int DetectUnrealGameId(HWND* windowHandle);
	static int DetectUnrealGameId();
	static bool UnrealEngineVersion(std::string& ver);

private:
	/// <summary>
	/// Fix pointer size in struct or class. used for convert 64bit to 32bit pointer.
	/// Must pick the right struct Size, else will case some memory problems in the hole program
	/// </summary>
	/// <param name="structBase">Pointer to instance of struct</param>
	/// <param name="varOffset">Offset to variable based on `structBase`</param>
	/// <param name="structSize">Size of struct</param>
	static void FixStructPointer(void* structBase, int varOffset, size_t structSize);
};

template <typename ElementType>
void Utils::FixPointer(ElementType* structBase, const int varOffset)
{
	if (ProgramIs64() && MemoryObj->Is64Bit)
		return;
	FixStructPointer(structBase, varOffset, sizeof(ElementType));
}

template <typename ElementType>
void Utils::FixPointers(ElementType* structBase, const size_t fullCppStructSize, std::vector<int> varsOffsets)
{
	if (ProgramIs64() && MemoryObj->Is64Bit)
		return;

	for (int varOff : varsOffsets)
		FixStructPointer(structBase, varOff, fullCppStructSize);
}
