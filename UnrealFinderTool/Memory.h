#pragma once
#include <Windows.h>
#include <string>
#include <psapi.h>

#pragma comment(lib, "psapi")
using std::string;

class Memory
{
	bool use_kernal = false;
	static BypaPH* bypa_ph;
public:
	HANDLE ProcessHandle = nullptr;
	int ProcessId = 0;
	BOOL Is64Bit = false;

	Memory(HANDLE processHandle, bool useKernal = false);
	Memory(int processId, bool useKernal = false);
	void UpdateHandle(HANDLE processHandle);
	BOOL SetPrivilegeM(HANDLE hToken, LPCTSTR lpszPrivilege, BOOL bEnablePrivilege);
	BOOL GetDebugPrivileges();
	SIZE_T ReadBytes(uintptr_t address, PVOID buf, int len);
	bool ReadBool(uintptr_t address);
	int ReadInt(uintptr_t address);
	INT64 ReadInt64(uintptr_t address);
	UINT32 ReadUInt(uintptr_t address);
	UINT64 ReadUInt64(uintptr_t address);
	int GetPointerAddress(uintptr_t address, const int offsets[], int offsetCount);
	int ReadPointerInt(uintptr_t address, int offsets[], int offsetCount);
	float ReadFloat(uintptr_t address);
	float ReadPointerFloat(uintptr_t address, int offsets[], int offsetCount);
	string ReadText(uintptr_t address);
	string ReadPointerText(const uintptr_t address, int offsets[], int offsetCount);
	static int GetProcessIdByName(char* processName);
	MODULEINFO GetModuleInfo(LPCTSTR lpModuleName);
};