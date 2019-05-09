#include "pch.h"
#include "Utils.h"
#include <vector>
#include <algorithm>

std::vector<std::string> Utils::SplitString(const std::string& str, const std::string& delimiter)
{
	std::vector<std::string> strings;

	std::string::size_type pos = 0;
	std::string::size_type prev = 0;
	while ((pos = str.find(delimiter, prev)) != std::string::npos)
	{
		strings.push_back(str.substr(prev, pos - prev));
		prev = pos + 1;
	}

	// To get the last substring (or only, if delimiter is not found)
	strings.push_back(str.substr(prev));

	return strings;
}

std::string Utils::ReplaceString(std::string str, const std::string& to_find, const std::string& to_replace)
{
	size_t position = 0;
	for (position = str.find(to_find); position != std::string::npos; position = str.find(to_find, position))
		str.replace(position, 1, to_replace);
	return str;
}

bool Utils::ProgramIs64()
{
#if _WIN64
	return true;
#else
	return false;
#endif
}

bool Utils::IsNumber(const std::string& s)
{
	return !s.empty() && std::find_if(s.begin(),
		s.end(), [](const char c) { return !std::isdigit(c); }) == s.end();
}

template<typename ElementType>
void Utils::FixStructPointer(void* structBase, const int varOffsetEach4Byte)
{
	const int x1 = 0x4 * (varOffsetEach4Byte + 1);
	const int x2 = 0x4 * varOffsetEach4Byte;

	const int size = abs(x1 - int(sizeof(ElementType)));
	memcpy_s
	(
		static_cast<char*>(structBase) + x1,
		size,
		static_cast<char*>(structBase) + x2,
		size
	);
	memset(static_cast<char*>(structBase) + x1, 0, 0x4);
}

void Utils::FixStructPointer(void* structBase, const int varOffsetEach4Byte, const int structSize)
{
	const int x1 = 0x4 * (varOffsetEach4Byte + 1);
	const int x2 = 0x4 * varOffsetEach4Byte;

	const int size = abs(x1 - structSize);
	memcpy_s
	(
		static_cast<char*>(structBase) + x1,
		size,
		static_cast<char*>(structBase) + x2,
		size
	);
	memset(static_cast<char*>(structBase) + x1, 0, 0x4);
}

uintptr_t Utils::ReadSafePointer(void* structBase, const int varOffsetEach4Byte, const int structSize, const bool is64BitGame)
{
	uintptr_t ret = 0;
	int offset = varOffsetEach4Byte * 4;
	uintptr_t calcAddress = *reinterpret_cast<uintptr_t*>(structBase) + offset;

	if (!is64BitGame)
	{
		ret = static_cast<DWORD>(calcAddress); // 4byte
		if (ProgramIs64())
			FixStructPointer(structBase, varOffsetEach4Byte, structSize);
	}
	else
	{
		ret = static_cast<DWORD64>(calcAddress); // 8byte
	}

	return ret;
}
