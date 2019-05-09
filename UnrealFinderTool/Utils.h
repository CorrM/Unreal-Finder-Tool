#pragma once
#include <string>
#include <vector>

class Utils
{
public:
	static std::vector<std::string> SplitString(const std::string& str, const std::string& delimiter);
	static std::string ReplaceString(std::string str, const std::string& to_find, const std::string& to_replace);
	static bool ProgramIs64();
	static bool IsNumber(const std::string& s);
	/// <summary>
	/// Fix pointer size in struct or class. used for convert 64bit to 32bit pointer.
	/// Must pick the right `ElementType`, else will case some memory problems in the hole program
	/// </summary>
	/// <param name="structBase">Pointer to instance of `ElementType`</param>
	/// <param name="varOffsetEach4Byte">Offset to variable based on `ElementType`</param>
	template <typename ElementType> static void FixStructPointer(void* structBase, int varOffsetEach4Byte);
	/// <summary>
	/// Fix pointer size in struct or class. used for convert 64bit to 32bit pointer.
	/// Must pick the right struct Size, else will case some memory problems in the hole program
	/// </summary>
	/// <param name="structBase">Pointer to instance of struct</param>
	/// <param name="varOffsetEach4Byte">Offset to variable based on `structBase`</param>
	/// <param name="structSize">Size of struct</param>
	static void FixStructPointer(void* structBase, int varOffsetEach4Byte, int structSize);
	static uintptr_t ReadSafePointer(void* structBase, int varOffsetEach4Byte, int structSize, bool is64BitGame);
};