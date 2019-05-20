#include "pch.h"
#include "JsonReflector.h"
#include "Utils.h"
#include <vector>
#include <algorithm>

Memory* Utils::MemoryObj = nullptr;

bool Utils::LoadJsonCore()
{
	// Read core GNames
	if (!JsonReflector::ReadAndLoadFile("Config\\Core\\GNames.json"))
	{
		std::cout << red << "[*] " << def << "Can't read GNames file." << std::endl << def;
		return false;
	}

	// Read core GObjects
	if (!JsonReflector::ReadAndLoadFile("Config\\Core\\GObjects.json"))
	{
		std::cout << red << "[*] " << def << "Can't read GObject file." << std::endl << def;
		return false;
	}

	// Read core CoreStructs
	if (!JsonReflector::ReadAndLoadFile("Config\\Core\\CoreStructs.json"))
	{
		std::cout << red << "[*] " << def << "Can't read CoreStructs file." << std::endl << def;
		return false;
	}

	return true;
}

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

DWORD Utils::BufToInteger(void* buffer)
{
	return *reinterpret_cast<DWORD*>(buffer);
}

DWORD64 Utils::BufToInteger64(void* buffer)
{
	return *reinterpret_cast<DWORD64*>(buffer);
}

bool Utils::IsNumber(const std::string& s)
{
	return !s.empty() && std::find_if(s.begin(),
		s.end(), [](const char c) { return !std::isdigit(c); }) == s.end();
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

template<typename ElementType>
void Utils::FixStructPointer(void* structBase, const int varOffsetEach4Byte)
{
	FixStructPointer(structBase, varOffsetEach4Byte, sizeof(ElementType));
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

uintptr_t Utils::ReadSafePointer(JsonStruct* structBase, const string& varName, const bool is64BitGame)
{
	uintptr_t ret = 0;
	uintptr_t calcAddress = reinterpret_cast<uintptr_t>(structBase->GetAllocPointer()) + structBase->GetVar(varName).Offset;

	if (!is64BitGame)
	{
		ret = *reinterpret_cast<DWORD*>(calcAddress); // 4byte
		if (ProgramIs64())
			FixStructPointer(structBase->GetAllocPointer(), int(structBase->GetVar(varName).Offset / 4), structBase->StructSize);
	}
	else
	{
		ret = *reinterpret_cast<DWORD64*>(calcAddress); // 8byte
	}

	return ret;
}

void Utils::FixPointersInJsonStruct(JsonStruct* structBase, const bool is64BitGame)
{
	if (structBase == nullptr)
		return;

	if (!is64BitGame && ProgramIs64())
	{
		for (auto& var : structBase->Vars)
		{
			if (var.second.Type == "pointer")
				ReadSafePointer(structBase, var.first, is64BitGame);
			else if (var.second.IsStruct)
			{
				if (var.second.Struct == nullptr)
					var.second.ReadAsStruct();

				if (var.second.Struct != nullptr)
					FixPointersInJsonStruct(var.second.Struct, is64BitGame);
			}
		}
	}
}

int Utils::PointerSize()
{
	return MemoryObj->Is64Bit ? 0x8 : 0x4;
}

bool Utils::IsValidAddress(Memory* mem, const uintptr_t address)
{
	if (INVALID_POINTER_VALUE(address))
		return false;

	// Check memory state, type and permission
	MEMORY_BASIC_INFORMATION info;

	//const uintptr_t pointerVal = _memory->ReadInt(pointer);
	if (VirtualQueryEx(mem->ProcessHandle, LPVOID(address), &info, sizeof info) == sizeof info)
	{
		// Bad Memory
		return info.Protect != PAGE_NOACCESS;
	}

	return false;
}

bool Utils::IsValidAddress(const uintptr_t address)
{
	if (INVALID_POINTER_VALUE(address))
		return false;

	// Check memory state, type and permission
	MEMORY_BASIC_INFORMATION info;

	//const uintptr_t pointerVal = _memory->ReadInt(pointer);
	if (VirtualQuery(LPVOID(address), &info, sizeof info) == sizeof info)
	{
		// Bad Memory
		return info.Protect != PAGE_NOACCESS;
	}

	return false;
}