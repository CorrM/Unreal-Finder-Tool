#include "pch.h"
#include "Memory.h"
#include "UiWindow.h"
#include "JsonReflector.h"
#include "PatternScan.h"
#include "DotNetConnect.h"
#include "Utils.h"

Memory* Utils::MemoryObj = nullptr;
UiWindow* Utils::UiMainWindow = nullptr;
Generator* Utils::GenObj = new Generator();
DotNetConnect* Utils::Dnc = new DotNetConnect();
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
		const auto pos = ue_ver.rfind('.');
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
		const std::string error = "Can't find/parse json file '" + engineVersion + ".json'.";
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
	const fs::path path(std::wstring(filePath.begin(), filePath.end()));
	return fs::exists(path);
}

bool Utils::FileExists(const std::wstring& filePath)
{
	const fs::path path(filePath);
	return fs::exists(path);
}

bool Utils::FileDelete(const std::string& filePath)
{
	if (!FileExists(filePath))
		return false;

	const fs::path path(std::wstring(filePath.begin(), filePath.end()));
	return fs::remove(path);
}

std::vector<fs::path> Utils::FileList(const std::string& dirPath)
{
	std::vector<fs::path> ret;

	for (auto& file : fs::directory_iterator(dirPath))
		ret.push_back(file.path());

	return ret;
}

void Utils::FileCreate(const std::string& filePath)
{
	const fs::path path(filePath);

	fs::create_directories(path.parent_path());
	std::ofstream ofs(path);
	ofs.close();
}

bool Utils::FileCopy(const std::string& src, const std::string& dest, const bool overwriteExisting)
{
	return fs::copy_file(fs::path(src), fs::path(dest), overwriteExisting ? fs::copy_options::overwrite_existing : fs::copy_options::none);
}

bool Utils::FileRead(const std::string& filePath, std::string& fileText)
{
	if (!FileExists(filePath)) return false;

	std::ifstream ifs(filePath.c_str(), std::ios::in | std::ios::binary | std::ios::ate);

	const std::ifstream::pos_type fileSize = ifs.tellg();
	ifs.seekg(0, std::ios::beg);

	std::vector<char> bytes(fileSize);
	ifs.read(&bytes[0], fileSize);

	fileText = std::string(&bytes[0], fileSize);

	return true;
}

bool Utils::FileWrite(const std::string& filePath, const std::string& fileText)
{
	const auto path = fs::path(filePath);
	if (!FileExists(filePath)) return false;

	std::ofstream ofs(path, std::ios::out | std::ios::binary);
	ofs << fileText;
	ofs.close();

	return true;
}

bool Utils::DirectoryDelete(const std::string& dirPath)
{
	if (!FileExists(dirPath))
		return false;

	const fs::path path(std::wstring(dirPath.begin(), dirPath.end()));
	return fs::remove_all(path) > 0;
}

std::wstring Utils::GetWorkingDirectoryW()
{
	return fs::path(GetExePathW()).parent_path().wstring();
}

std::string Utils::GetWorkingDirectoryA()
{
	return fs::path(GetExePathA()).parent_path().string();
}

std::wstring Utils::GetExePathW()
{
	// Returned cached copy of path
	std::wstring curExePath(1024, '\0');
	if (curExePath[0] != '\0')
		return curExePath;

	// Get working directory path
	GetModuleFileNameW(nullptr, curExePath.data(), static_cast<DWORD>(curExePath.length()));

	curExePath.resize(curExePath.find(L'\0'));

	const fs::path curPath(curExePath);
	return curPath.wstring();
}

std::string Utils::GetExePathA()
{
	// Returned cached copy of path
	std::string curExePath(1024, '\0');
	if (curExePath[0] != '\0')
		return curExePath;

	// Get working directory path
	GetModuleFileName(nullptr, curExePath.data(), static_cast<DWORD>(curExePath.length()));

	curExePath.resize(curExePath.find('\0'));

	const fs::path curPath(curExePath);
	return curPath.string();
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

std::string Utils::RemoveStringBetween(std::string str, const std::string& between1, const std::string& between2)
{
	if (str.empty() || between1.empty() || between2.empty())
		return str;

	size_t position1 = 0, position2 = 0;
	do
	{
		position1 = str.find(between1, position1);
		position2 = str.find(between2, position2);

		if (position1 == std::string::npos || position2 == std::string::npos)
			break;

		str.replace(position1, position2 - position1 + 1, "");

	} while (true);

	return str;
}

bool Utils::ContainsString(const std::string& str, const std::string& strToFind)
{
	return str.find(strToFind) != std::string::npos;
}

bool Utils::StartsWith(const std::string& value, const std::string& starting)
{
	if (starting.size() > value.size()) return false;
	return std::equal(starting.begin(), starting.begin() + starting.length(), value.begin());
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

std::wstring Utils::ReplaceString(std::wstring str, const std::wstring& to_find, const std::wstring& to_replace)
{
	if (to_find.empty())
		return str;

	for (size_t position = str.find(to_find); position != std::wstring::npos; position = str.find(to_find, position))
		str.replace(position, to_find.length(), to_replace);

	return str;
}

std::wstring Utils::RemoveStringBetween(std::wstring str, const std::wstring& between1, const std::wstring& between2)
{
	if (str.empty() || between1.empty() || between2.empty())
		return str;

	size_t position1 = 0;
	do
	{
		position1 = str.find(between1, position1);
		const size_t position2 = str.find(between2, position1);
		const size_t strSize = position2 - position1;

		if (position1 == std::wstring::npos || position2 == std::wstring::npos)
			break;

		str.replace(position1, strSize + 1, L"");

	} while (true);

	return str;
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
		return (info.State & MEM_COMMIT) && !(info.Protect & PAGE_NOACCESS);
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
		return (info.State & MEM_COMMIT) && !(info.Protect & PAGE_NOACCESS)/* && info.RegionSize < 0x300000*/;
	}

	return false;
}

bool Utils::IsValidGNamesAddress(uintptr_t address, const bool chunkCheck)
{
	if (MemoryObj == nullptr || !IsValidRemoteAddress(MemoryObj, address))
		return false;

	if (!chunkCheck && !IsValidRemoteAddress(MemoryObj, MemoryObj->ReadAddress(address)))
		return false;

	if (!chunkCheck)
		address = MemoryObj->ReadAddress(address);

	int null_count = 0;

	// Chunks array must have null pointers, if not then it's not valid
	for (int read_address = 0; read_address <= 50; ++read_address)
	{
		// Read Chunk Address
		const auto offset = size_t(read_address * PointerSize());
		const uintptr_t chunk_address = MemoryObj->ReadAddress(address + offset);
		if (chunk_address == NULL)
			++null_count;
	}

	if (null_count <= 3)
		return false;

	// Read First FName Address
	const uintptr_t noneFName = MemoryObj->ReadAddress(MemoryObj->ReadAddress(address));
	if (!IsValidRemoteAddress(MemoryObj, noneFName)) return false;

	// Search for none FName
	const auto pattern = PatternScan::Parse("NoneSig", 0, "4E 6F 6E 65 00", 0xFF);
	const auto result = PatternScan::FindPattern(MemoryObj, noneFName, noneFName + 0x50, { pattern }, true);
	const auto resVec = result.find("NoneSig")->second;
	return !resVec.empty();
}

bool Utils::IsValidGNamesAddress(const uintptr_t address)
{
	return IsValidGNamesAddress(address, false);
}

bool Utils::IsValidGNamesChunksAddress(const uintptr_t address)
{
	return IsValidGNamesAddress(address, true);
}

size_t Utils::CalcNameOffset(const uintptr_t address)
{
	uintptr_t curAddress = address;
	MEMORY_BASIC_INFORMATION info;

	while (
		VirtualQueryEx(MemoryObj->ProcessHandle, reinterpret_cast<LPVOID>(curAddress), &info, sizeof info) == sizeof(info) &&
		info.BaseAddress != reinterpret_cast<PVOID>(curAddress) &&
		curAddress >= address - 0x10)
	{
		--curAddress;
	}

	return address - curAddress;
}

bool Utils::IsTArray(const uintptr_t address)
{
	// Check PreAllocatedObjects it's always null, it's only on new TUObjectArray then it's good to check
	return MemoryObj->ReadAddress(address + PointerSize()) != NULL;
}

bool Utils::IsTUobjectArray(const uintptr_t address)
{
	// if game have chunks, then it's not TArray
	uintptr_t gObjectArray = MemoryObj->ReadAddress(MemoryObj->ReadAddress(address));
	if (!IsTArray(address) && IsValidGObjectsAddress(gObjectArray))
		return true;

	// if game don't use chunks, then it's must be TArray
	gObjectArray = MemoryObj->ReadAddress(address);
	return IsTArray(address) && IsValidGObjectsAddress(gObjectArray);
}

bool Utils::IsValidGObjectsAddress(const uintptr_t address)
{
	// => Get information
	static auto objectItem = JsonReflector::GetStruct("FUObjectItem");
	static auto objectItemSize = objectItem.GetSize();

	static auto objectInfo = JsonReflector::GetStruct("UObject");
	static auto objOuter = objectInfo["Outer"].Offset;
	static auto objInternalIndex = objectInfo["InternalIndex"].Offset;
	static auto objNameIndex = objectInfo["Name"].Offset;
	// => Get information

	uintptr_t addressHolder = address;
	if (MemoryObj == nullptr || !IsValidRemoteAddress(MemoryObj, addressHolder))
		return false;
	/*
	 * NOTE:
	 * Nested loops will be slow, spitted best.
	 */
	const size_t objCount = 2;
	uintptr_t objects[objCount] = { 0 }; // Store Object Address
	uintptr_t vTables[objCount] = { 0 }; // Store VTable Address

	// Check (UObject*) Is Valid Pointer
	for (size_t i = 0; i <= objCount - 1; ++i)
	{
		size_t offset = objectItemSize * i;
		if (!IsValidRemotePointer(addressHolder + offset, &objects[i]))
			return false;
	}

	// Check (VTable) Is Valid Pointer
	for (size_t i = 0; i <= objCount - 1; ++i)
	{
		if (!IsValidRemotePointer(objects[i], &vTables[i]))
			return false;
	}

	// Check (InternalIndex) Is Valid
	for (size_t i = 0; i <= objCount - 1; ++i)
	{
		size_t internalIndex = MemoryObj->ReadInt(objects[i] + objInternalIndex);
		if (internalIndex != i)
			return false;
	}

	// Check (Outer) Is Valid
	// first object must have Outer == nullptr(0x0000000000)
	const int uOuter = MemoryObj->ReadInt(objects[0] + objOuter);
	if (uOuter != NULL)
		return false;

	// Check (FName_index) Is Valid
	// 2nd object must have FName_index == 100
	const int uFNameIndex = MemoryObj->ReadInt(objects[1] + objNameIndex);
	return uFNameIndex == 100;
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

	if (ProgramIs64() && MemoryObj->Is64)
		throw std::exception("FixStructPointer only work for 32bit games with 64bit tool version.");

	const size_t destSize = abs(varOffset - static_cast<long long>(structSize));
	const size_t srcSize = abs(varOffset - static_cast<long long>(structSize)) - 0x4;

	char* src = static_cast<char*>(structBase) + varOffset;
	char* dest = src + 0x4;

	memcpy_s(dest, destSize, src, srcSize);
	memset(dest, 0x0, 0x4);
}

size_t Utils::PointerSize()
{
	return MemoryObj->Is64 ? 0x8 : 0x4;
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

		char windowText[30];
		GetWindowText(childControl, windowText, 30);
		windowTitle = windowText;
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

	const auto process = MemoryObj->ProcessHandle;
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
		const auto pData = new BYTE[version_size];
		GetFileVersionInfo(path.c_str(), 0, version_size, static_cast<LPVOID>(pData));

		void* fixed = nullptr;
		unsigned int size = 0;

		VerQueryValue(pData, _T("\\"), &fixed, &size);
		if (fixed)
		{
			const auto ffi = static_cast<VS_FIXEDFILEINFO*>(fixed);

			const auto v1 = HIWORD(ffi->dwFileVersionMS);
			const auto v2 = LOWORD(ffi->dwFileVersionMS);
			const auto v3 = HIWORD(ffi->dwFileVersionLS);

			const std::string engine_version = std::to_string(v1) + "." + std::to_string(v2) + "." + std::to_string(v3);
			ver = engine_version;
			ret = true;
		}
		delete[] pData;
	}
	return ret;
}
#pragma endregion

void Utils::CleanUp()
{
	if (MemoryObj)
	{
		MemoryObj->ResumeProcess();
		CloseHandle(MemoryObj->ProcessHandle);
	}

	delete MemoryObj;
	delete UiMainWindow;
	delete GenObj;
	delete Dnc;
}
