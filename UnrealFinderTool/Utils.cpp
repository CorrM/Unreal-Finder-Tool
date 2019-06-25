#include "pch.h"
#include "JsonReflector.h"
#include "PatternScan.h"
#include "Memory.h"
#include "UiWindow.h"
#include "Utils.h"

#include <vector>
#include <sstream>
#include <algorithm>
#include <cctype>
#include <tchar.h>

Memory* Utils::MemoryObj = nullptr;
UiWindow* Utils::UiMainWindow = nullptr;
MySettings Utils::Settings;
WorkingTools Utils::WorkingNow;
std::mutex Utils::MainMutex;

#pragma region Json
bool Utils::LoadEngineCore(std::vector<std::string>& ue_versions_container)
{
	// Get all engine files and load it's structs
	if (!JsonReflector::ReadAndLoadFile("Config\\EngineCore\\EngineBase.json", &JsonReflector::JsonBaseObj))
	{
		MessageBox(nullptr, "Can't read EngineBase file.", "Error", MB_OK);
		ExitProcess(-1);
		// return false;
	}
	ue_versions_container.emplace_back("EngineBase");

	for (auto& file : fs::directory_iterator("Config\\EngineCore\\"))
	{
		std::string ue_ver = file.path().filename().string();
		auto pos = ue_ver.rfind('.');
		ue_ver = ue_ver.substr(0, pos);
		if (ue_ver != "EngineBase")
			ue_versions_container.push_back(ue_ver);
	}

	return true;
}

void Utils::OverrideLoadedEngineCore(const std::string& engineVersion)
{
	// Get engine files and Override it's structs on EngineBase, if return false then there is no override for target engine and use the EngineBase
	try
	{
		if (engineVersion != "EngineBase")
			JsonReflector::ReadAndLoadFile("Config\\EngineCore\\" + engineVersion + ".json", true);
	}
	catch (...)
	{
		std::string error = "Can't find/parse json file '" + engineVersion + ".json'.";
		MessageBoxA(nullptr, error.c_str(), "Critical Problem", MB_OK | MB_ICONERROR);
		ExitProcess(-1);
	}
}

bool Utils::LoadSettings()
{
	if (!JsonReflector::ReadJsonFile("Config\\Settings.json"))
	{
		MessageBox(nullptr, "Can't read Settings file.", "Error", MB_OK);
		return false;
	}

	auto j = JsonReflector::JsonObj;

	// Sdk Generator Settings
	auto sdkParas = j.at("sdkGenerator");
	Settings.SdkGen.CorePackageName = sdkParas["core Name"];
	Settings.SdkGen.MemoryHeader = sdkParas["memory header"];
	Settings.SdkGen.MemoryRead = sdkParas["memory read"];
	Settings.SdkGen.MemoryWrite = sdkParas["memory write"];
	Settings.SdkGen.MemoryWriteType = sdkParas["memory write type"];
	Settings.SdkGen.Threads = sdkParas["threads"];

	Settings.SdkGen.DumpObjects = sdkParas["dump Objects"];
	Settings.SdkGen.DumpNames = sdkParas["dump Names"];

	Settings.SdkGen.LoggerShowSkip = sdkParas["logger ShowSkip"];
	Settings.SdkGen.LoggerShowClassSaveFileName = sdkParas["logger ShowClassSaveFileName"];
	Settings.SdkGen.LoggerShowStructSaveFileName = sdkParas["logger ShowStructSaveFileName"];
	Settings.SdkGen.LoggerSpaceCount = sdkParas["logger SpaceCount"];
	return true;
}
#pragma endregion

#pragma region FileManager
bool Utils::FileExists(const std::string& filePath)
{
	fs::path path(std::wstring(filePath.begin(), filePath.end()));
	return fs::exists(path);
}

bool Utils::FileDelete(const std::string& filePath)
{
	if (!FileExists(filePath))
		return false;
	
	fs::path path(std::wstring(filePath.begin(), filePath.end()));
	return fs::remove(path);
}

bool Utils::DirectoryDelete(const std::string& dirPath)
{
	if (!FileExists(dirPath))
		return false;

	fs::path path(std::wstring(dirPath.begin(), dirPath.end()));
	return fs::remove_all(path) > 0;
}

std::string Utils::GetWorkingDirectory()
{
	// Returned cached copy of path
	static std::string curDir(260, '\0');
	if (curDir[0] != '\0')
		return curDir;

	// Get working directory path
	GetModuleFileName(nullptr, curDir.data(), static_cast<DWORD>(curDir.length()));

	fs::path curPath(curDir);
	curDir = curPath.parent_path().string();

	return curDir;
}
#pragma endregion

#pragma region String
std::vector<std::string> Utils::SplitString(const std::string& str, const std::string& delimiter)
{
	std::vector<std::string> strings;

	std::string::size_type pos;
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
	if (to_find.empty())
		return str;

	for (size_t position = str.find(to_find); position != std::string::npos; position = str.find(to_find, position))
		str.replace(position, to_find.length(), to_replace);
	return str;
}

bool Utils::ContainsString(const std::string& str, const std::string& strToFind)
{
	return str.find(strToFind) != std::string::npos;
}

bool Utils::EndsWith(const std::string& value, const std::string& ending)
{
	if (ending.size() > value.size()) return false;
	return std::equal(ending.rbegin(), ending.rend(), value.rbegin());
}

bool Utils::IsNumber(const std::string& s)
{
	return !s.empty() && std::find_if(s.begin(),
	                                  s.end(), [](const char c) { return !std::isdigit(c); }) == s.end();
}

bool Utils::IsHexNumber(const std::string& s)
{
	return std::all_of(s.begin(), s.end(), [](const unsigned char c) { return std::isxdigit(c); });
}
#pragma endregion

#pragma region Types Converter
int Utils::BufToInteger(void* buffer)
{
	return *reinterpret_cast<DWORD*>(buffer);
}

int64_t Utils::BufToInteger64(void* buffer)
{
	return *reinterpret_cast<DWORD64*>(buffer);
}

uintptr_t Utils::CharArrayToUintptr(const std::string& str)
{
	if (str.empty())
		return 0;

	uintptr_t retVal;
	std::stringstream ss;
	ss << std::hex << str;
	ss >> retVal;

	return retVal;
}

std::string Utils::AddressToHex(const uintptr_t address)
{
	std::stringstream ss;
	ss << std::hex << address;
	return ss.str();
}
#pragma endregion

#pragma region Address stuff
bool Utils::IsValidRemoteAddress(Memory* mem, const uintptr_t address)
{
	if (INVALID_POINTER_VALUE(address))
		return false;

	// Check memory state, type and permission
	MEMORY_BASIC_INFORMATION info;

	//const uintptr_t pointerVal = _memory->ReadInt(pointer);
	if (VirtualQueryEx(mem->ProcessHandle, LPVOID(address), &info, sizeof info) == sizeof info)
	{
		// Bad Memory
		return !(info.Protect & PAGE_NOACCESS);
	}

	return false;
}

bool Utils::IsValidLocalAddress(const uintptr_t address)
{
	if (INVALID_POINTER_VALUE(address))
		return false;

	// Check memory state, type and permission
	MEMORY_BASIC_INFORMATION info;

	//const uintptr_t pointerVal = _memory->ReadInt(pointer);
	if (VirtualQuery(LPVOID(address), &info, sizeof info) == sizeof info)
	{
		// Bad Memory
		return /*(info.State & MEM_COMMIT) && */!(info.Protect & PAGE_NOACCESS);
	}

	return false;
}

bool Utils::IsValidRemotePointer(const uintptr_t pointer, uintptr_t *address)
{
	uintptr_t tmp;
	if (!address) address = &tmp;

	*address = MemoryObj->ReadAddress(pointer);
	if (INVALID_POINTER_VALUE(*address))
		return false;

	// Check memory state, type and permission
	MEMORY_BASIC_INFORMATION info;

	//const uintptr_t pointerVal = _memory->ReadInt(pointer);
	if (VirtualQueryEx(MemoryObj->ProcessHandle, LPVOID(pointer), &info, sizeof info) == sizeof info)
	{
		return (info.State & MEM_COMMIT) && !(info.Protect & PAGE_NOACCESS);
	}

	return false;
}

bool Utils::IsValidGNamesAddress(const uintptr_t address)
{
	if (MemoryObj == nullptr || !IsValidRemoteAddress(MemoryObj, address))
		return false;

	int null_count = 0;

	// Chunks array must have null pointers, if not then it's not valid
	for (int read_address = 0; read_address <= 50; ++read_address)
	{
		// Read Chunk Address
		auto offset = size_t(read_address * PointerSize());
		uintptr_t chunk_address = MemoryObj->ReadAddress(address + offset);
		if (chunk_address == NULL)
			++null_count;
	}

	if (null_count <= 3)
		return false;

	// Read First FName Address
	uintptr_t noneFName = MemoryObj->ReadAddress(MemoryObj->ReadAddress(address));
	if (!IsValidRemoteAddress(MemoryObj, noneFName)) return false;

	// Search for none FName
	auto pattern = PatternScan::Parse("NoneSig", 0, "4E 6F 6E 65 00", 0xFF);
	auto result = PatternScan::FindPattern(MemoryObj, noneFName, noneFName + 0x50, { pattern }, true);
	auto resVec = result.find("NoneSig")->second;
	return !resVec.empty();
}

bool Utils::IsValidGObjectsAddress(const uintptr_t address, bool* isChunks)
{
	uintptr_t addressHolder = address;
	if (isChunks)
		*isChunks = false;

	if (MemoryObj == nullptr || !IsValidRemoteAddress(MemoryObj, addressHolder))
		return false;

	bool firstCheck = true;
	uintptr_t ptrUObject0, ptrUObject1, ptrUObject2, ptrUObject3, ptrUObject4, ptrUObject5;
	uintptr_t ptrVfTableObject0, ptrVfTableObject1, ptrVfTableObject2, ptrVfTableObject3, ptrVfTableObject4,
	          ptrVfTableObject5;

CheckAgian:
	for (int i = 0x0; i <= 0x20; i += 0x4)
	{
		// Check (UObject*) Is Valid Pointer
		if (!IsValidRemotePointer(addressHolder + (i * 0), &ptrUObject0)) continue;
		if (!IsValidRemotePointer(addressHolder + (i * 1), &ptrUObject1)) continue;
		if (!IsValidRemotePointer(addressHolder + (i * 2), &ptrUObject2)) continue;
		if (!IsValidRemotePointer(addressHolder + (i * 3), &ptrUObject3)) continue;
		if (!IsValidRemotePointer(addressHolder + (i * 4), &ptrUObject4)) continue;
		if (!IsValidRemotePointer(addressHolder + (i * 5), &ptrUObject5)) continue;

		// Check vfTableObject Is Valid Pointer
		if (!IsValidRemotePointer(ptrUObject0, &ptrVfTableObject0)) continue;
		if (!IsValidRemotePointer(ptrUObject1, &ptrVfTableObject1)) continue;
		if (!IsValidRemotePointer(ptrUObject2, &ptrVfTableObject2)) continue;
		if (!IsValidRemotePointer(ptrUObject3, &ptrVfTableObject3)) continue;
		if (!IsValidRemotePointer(ptrUObject4, &ptrVfTableObject4)) continue;
		if (!IsValidRemotePointer(ptrUObject5, &ptrVfTableObject5)) continue;

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

			// Check if it's chunks address
			if (isChunks && !firstCheck)
			{
				int skipCount = 0;
				for (size_t uIndex = 0; uIndex <= 20 && skipCount <= 5; ++uIndex)
				{
					uintptr_t curAddress = address + uIndex * PointerSize();
					uintptr_t chunk = MemoryObj->ReadAddress(curAddress);

					// Skip null address and bad address
					if (chunk == 0)
					{
						++skipCount;
						continue;
					}
					if (!IsValidRemoteAddress(MemoryObj, chunk)) break;

					skipCount = 0;
				}

				if (skipCount >= 5)
				{
					*isChunks = true;
				}
				else
				{
					*isChunks = false;
				}
			}

			return bFoundNameIndex;
		}
	}

	// If it's GObjects Chunks
	if (firstCheck)
	{
		firstCheck = false;
		addressHolder = MemoryObj->ReadAddress(addressHolder);
		goto CheckAgian;
	}

	if (isChunks)
		*isChunks = !firstCheck;

	return false;
}

void Utils::FixStructPointer(void* structBase, const int varOffset, const size_t structSize)
{
	{
		// Check next 4byte of pointer equal 0 in local tool memory
		// in 32bit game pointer is 4byte so i must zero the second part of uintptr_t in the tool memory
		// so here i check if second part of uintptr_t is zero so it's didn't need fix
		// this check is good for solve some problem when cast UObject to something like UEnum
		// the base (UObject) is fixed but the other (UEnum members) not fixed so, this wil fix really members need to fix
		// That's it :D
		//bool needFix = *reinterpret_cast<int*>(reinterpret_cast<uintptr_t>(structBase) + varOffset + 0x4) != 0;
		//if (!needFix) return;
	}

	if (ProgramIs64() && MemoryObj->Is64Bit)
		throw std::exception("FixStructPointer only work for 32bit games with 64bit tool version.");

	const size_t destSize = abs(varOffset - static_cast<long long>(structSize));
	const size_t srcSize = abs(varOffset - static_cast<long long>(structSize)) - 0x4;

	char* src = static_cast<char*>(structBase) + varOffset;
	char* dest = src + 0x4;

	memcpy_s(dest, destSize, src, srcSize);
	memset(dest, 0x0, 0x4);
}

int Utils::PointerSize()
{
	return MemoryObj->Is64Bit ? 0x8 : 0x4;
}
#pragma endregion

#pragma region Tool stuff
bool Utils::ProgramIs64()
{
#if _WIN64
	return true;
#else
		return false;
#endif
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
#pragma endregion

#pragma region UnrealEngine
DWORD Utils::DetectUnrealGame(HWND* windowHandle, std::string& windowTitle)
{
	HWND childControl = FindWindowEx(HWND_DESKTOP, nullptr, UNREAL_WINDOW_CLASS, nullptr);
	retry:
	if (childControl != nullptr)
	{
		DWORD pId;
		GetWindowThreadProcessId(childControl, &pId);

		if (Memory::GetProcessNameById(pId) == "EpicGamesLauncher.exe")
		{
			childControl = FindWindowEx(HWND_DESKTOP, childControl, UNREAL_WINDOW_CLASS, nullptr);
			goto retry;
		}

		if (windowHandle != nullptr)
			*windowHandle = childControl;

		windowTitle.resize(27);
		GetWindowText(childControl, windowTitle.data(), 27);
		return pId;
	}

	return 0;
}

DWORD Utils::DetectUnrealGame(std::string& windowTitle)
{
	return DetectUnrealGame(nullptr, windowTitle);
}

DWORD Utils::DetectUnrealGame()
{
	std::string tmp;
	return DetectUnrealGame(nullptr, tmp);
}

bool Utils::UnrealEngineVersion(std::string& ver)
{
	auto ret = false;

	auto process = MemoryObj->ProcessHandle;
	if (!process)
		return ret;

	std::string path;
	path.resize(MAX_PATH, '\0');

	DWORD path_size = MAX_PATH;
	if (!QueryFullProcessImageName(process, 0, path.data(), &path_size))
		return ret;

	unsigned long version_size = GetFileVersionInfoSize(path.c_str(), &version_size);
	if (version_size > 0)
	{
		auto pData = new BYTE[version_size];
		GetFileVersionInfo(path.c_str(), 0, version_size, static_cast<LPVOID>(pData));

		void* fixed = nullptr;
		unsigned int size = 0;

		VerQueryValue(pData, _T("\\"), &fixed, &size);
		if (fixed)
		{
			auto ffi = static_cast<VS_FIXEDFILEINFO*>(fixed);

			auto v1 = HIWORD(ffi->dwFileVersionMS);
			auto v2 = LOWORD(ffi->dwFileVersionMS);
			auto v3 = HIWORD(ffi->dwFileVersionLS);

			std::string engine_version = std::to_string(v1) + "." + std::to_string(v2) + "." + std::to_string(v3);
			ver = engine_version;
			ret = true;
		}
		delete[] pData;
	}
	return ret;
}
#pragma endregion
