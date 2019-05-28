#include "pch.h"
#include "JsonReflector.h"
#include "Utils.h"
#include <vector>
#include <sstream>
#include <algorithm>
#include "PatternScan.h"

Memory* Utils::MemoryObj = nullptr;
MySettings Utils::Settings;

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

bool Utils::LoadSettings()
{
	if (!JsonReflector::ReadJsonFile("Config\\Settings.json"))
	{
		std::cout << red << "[*] " << def << "Can't read Settings file." << std::endl << def;
		return false;
	}

	auto j = JsonReflector::JsonObj;

	// Sdk Generator Settings
	auto sdkParas = j.at("sdkGenerator");
	Settings.SdkGen.CorePackageName = sdkParas["core Name"];
	Settings.SdkGen.Threads = sdkParas["threads"];

	Settings.SdkGen.DumpObjects = sdkParas["dump Objects"];
	Settings.SdkGen.DumpNames = sdkParas["dump Names"];

	Settings.SdkGen.LoggerShowSkip = sdkParas["logger ShowSkip"];
	Settings.SdkGen.LoggerShowClassSaveFileName = sdkParas["logger ShowClassSaveFileName"];
	Settings.SdkGen.LoggerShowStructSaveFileName = sdkParas["logger ShowStructSaveFileName"];
	Settings.SdkGen.LoggerSpaceCount = sdkParas["logger SpaceCount"];
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
		str.replace(position, to_find.length(), to_replace);
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

int Utils::BufToInteger(void* buffer)
{
	return *reinterpret_cast<DWORD*>(buffer);
}

int64_t Utils::BufToInteger64(void* buffer)
{
	return *reinterpret_cast<DWORD64*>(buffer);
}

uintptr_t Utils::CharArrayToUintptr(const std::string str)
{
	if (str.empty())
		return 0;

	//try
	//{
	//	return std::stoull(str);
	//}
	//catch (std::exception const& e)
	//{
	//	// This could not be parsed into a number so an exception is thrown.
	//	// atoi() would return 0, which is less helpful if it could be a valid value.
	//}

	uintptr_t retVal;
	std::stringstream ss;
	ss << std::hex << str;
	ss >> retVal;

	return retVal;
}

bool Utils::IsNumber(const std::string& s)
{
	return !s.empty() && std::find_if(s.begin(),
		s.end(), [](const char c) { return !std::isdigit(c); }) == s.end();
}

void Utils::FixStructPointer(void* structBase, const int varOffset, const int structSize)
{
	// Check next 4byte of pointer equal 0 in local tool memory
	// in 32bit game pointer is 4byte so i must zero the second part of uintptr_t in the tool memory
	// so here i check if second part of uintptr_t is zero so it's didn't need fix
	// this check is good for solve some problem when cast UObject to something like UEnum
	// the base (UObject) is fixed but the other (UEnum members) not fixed so, this wil fix really members need to fix
	// That's it :D
	//bool needFix = *reinterpret_cast<int*>(reinterpret_cast<uintptr_t>(structBase) + varOffset + 0x4) != 0;
	//if (!needFix) return;

	if (ProgramIs64() && MemoryObj->Is64Bit)
		throw std::exception("FixStructPointer only work for 32bit games with 64bit tool version.");

	const int destSize = abs(varOffset - structSize);
	const int srcSize = abs(varOffset - structSize) - 0x4;

	char* src = static_cast<char*>(structBase) + varOffset;
	char* dest = src + 0x4;

	memcpy_s(dest, destSize, src, srcSize);
	memset(dest, 0x0, 0x4);
}

uintptr_t Utils::ReadPointer(void* structBase, const int varOffset, const int structSize, const bool is64BitGame)
{
	uintptr_t ret = 0;
	uintptr_t calcAddress = *reinterpret_cast<uintptr_t*>(structBase) + varOffset;

	if (!is64BitGame)
	{
		ret = static_cast<DWORD>(calcAddress); // 4byte
		if (ProgramIs64())
			FixStructPointer(structBase, varOffset, structSize);
	}
	else
	{
		ret = static_cast<DWORD64>(calcAddress); // 8byte
	}

	return ret;
}

uintptr_t Utils::ReadPointer(JsonStruct* structBase, const string& varName, const bool is64BitGame)
{
	uintptr_t ret = 0;
	uintptr_t calcAddress = reinterpret_cast<uintptr_t>(structBase->GetAllocPointer()) + structBase->GetVar(varName).Offset;

	if (!is64BitGame)
	{
		ret = *reinterpret_cast<DWORD*>(calcAddress); // 4byte
		if (ProgramIs64())
			FixStructPointer(structBase->GetAllocPointer(), structBase->GetVar(varName).Offset, structBase->StructSize);
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
				ReadPointer(structBase, var.first, is64BitGame);
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

bool Utils::IsValidPointer(const uintptr_t address, uintptr_t& pointer)
{
	pointer = NULL;
	if (!MemoryObj->Is64Bit)
		pointer = MemoryObj->ReadUInt(address);
	else
		pointer = MemoryObj->ReadUInt64(address);

	if (INVALID_POINTER_VALUE(pointer) /*|| pointer <= dwStart || pointer > dwEnd*/)
		return false;

	// Check memory state, type and permission
	MEMORY_BASIC_INFORMATION info;

	//const uintptr_t pointerVal = _memory->ReadInt(pointer);
	if (VirtualQueryEx(MemoryObj->ProcessHandle, LPVOID(pointer), &info, sizeof info) == sizeof info)
	{
		// Bad Memory
		// if (info.State != MEM_COMMIT) return false;
		return info.Protect != PAGE_NOACCESS;
	}

	return false;
}

bool Utils::IsValidGNamesAddress(const uintptr_t address)
{
	// Read First Chunk Address
	uintptr_t firstChunk = MemoryObj->ReadAddress(address);
	if (!IsValidAddress(MemoryObj, firstChunk)) return false;

	// Read First FName Address
	uintptr_t noneFName = MemoryObj->ReadAddress(firstChunk);
	if (!Utils::IsValidAddress(MemoryObj, noneFName)) return false;

	// Search for none FName
	auto pattern = PatternScan::Parse("NoneSig", 0, "4E 6F 6E 65 00", 0xFF);
	auto result = PatternScan::FindPattern(MemoryObj, noneFName, noneFName + 0x50, { pattern }, true);
	auto resVec = result.find("NoneSig")->second;
	return !resVec.empty();
}

bool Utils::IsValidGObjectsAddress(const uintptr_t address)
{
	uintptr_t ptrUObject0, ptrUObject1, ptrUObject2, ptrUObject3, ptrUObject4, ptrUObject5;
	uintptr_t ptrVfTableObject0, ptrVfTableObject1, ptrVfTableObject2, ptrVfTableObject3, ptrVfTableObject4, ptrVfTableObject5;

	for (int i = 0x0; i <= 0x20; i += 0x4)
	{
		// Check (UObject*) Is Valid Pointer
		if (!IsValidPointer(address + (i * 0), ptrUObject0)) continue;
		if (!IsValidPointer(address + (i * 1), ptrUObject1)) continue;
		if (!IsValidPointer(address + (i * 2), ptrUObject2)) continue;
		if (!IsValidPointer(address + (i * 3), ptrUObject3)) continue;
		if (!IsValidPointer(address + (i * 4), ptrUObject4)) continue;
		if (!IsValidPointer(address + (i * 5), ptrUObject5)) continue;

		// Check vfTableObject Is Valid Pointer
		if (!IsValidPointer(ptrUObject0, ptrVfTableObject0)) continue;
		if (!IsValidPointer(ptrUObject1, ptrVfTableObject1)) continue;
		if (!IsValidPointer(ptrUObject2, ptrVfTableObject2)) continue;
		if (!IsValidPointer(ptrUObject3, ptrVfTableObject3)) continue;
		if (!IsValidPointer(ptrUObject4, ptrVfTableObject4)) continue;
		if (!IsValidPointer(ptrUObject5, ptrVfTableObject5)) continue;

		// Check Objects (InternalIndex)
		for (int io = 0x0; io < 0x1C; io += 0x4)
		{
			const int uObject0InternalIndex = MemoryObj->ReadInt(ptrUObject0 + io);
			const int uObject1InternalIndex = MemoryObj->ReadInt(ptrUObject1 + io);
			const int uObject2InternalIndex = MemoryObj->ReadInt(ptrUObject2 + io);
			const int uObject3InternalIndex = MemoryObj->ReadInt(ptrUObject3 + io);
			const int uObject4InternalIndex = MemoryObj->ReadInt(ptrUObject4 + io);
			const int uObject5InternalIndex = MemoryObj->ReadInt(ptrUObject5 + io);

			if (uObject0InternalIndex != 0) continue;
			if (!(uObject1InternalIndex == 1 || uObject1InternalIndex == 3)) continue;
			if (!(uObject2InternalIndex == 2 || uObject2InternalIndex == 6)) continue;
			if (!(uObject3InternalIndex == 3 || uObject3InternalIndex == 9)) continue;
			if (!(uObject4InternalIndex == 4 || uObject4InternalIndex == 12)) continue;
			if (!(uObject5InternalIndex == 5 || uObject5InternalIndex == 15)) continue;

			// Check if 2nd UObject have FName_Index == 100
			bool bFoundNameIndex = false;
			for (int j = 0x4; j < 0x1C; j += 0x4)
			{
				const int uFNameIndex = MemoryObj->ReadInt(ptrUObject1 + j);
				if (uFNameIndex == 100)
				{
					bFoundNameIndex = true;
					break;
				}
			}

			return bFoundNameIndex;
		}
	}

	return false;
}

void Utils::SleepEvery(const int ms, int& counter, const int every)
{
	if (every == 0)
	{
		counter = 0;
		return;
	}

	if (counter >= every)
	{
		Sleep(ms);
		counter = 0;
	}
	else
	{
		++counter;
	}
}