#pragma once
#include <Windows.h>
#include <string>

#pragma comment(lib, "psapi")
using std::string;

class Memory
{
	bool use_kernal = false;
	BypaPH* bypa_ph = nullptr;
public:
	HANDLE ProcessHandle = nullptr;
	int ProcessId = 0;
	BOOL Is64Bit = false;

	Memory(HANDLE processHandle, bool useKernal = false);
	Memory(int processId, bool useKernal = false);
	~Memory();
	uintptr_t GetModuleBase(string sModuleName);
	BOOL SetPrivilegeM(HANDLE hToken, LPCTSTR lpszPrivilege, BOOL bEnablePrivilege);
	BOOL GetDebugPrivileges();
	SIZE_T ReadBytes(uintptr_t address, BYTE* buf, int len);
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
	string ReadPointerText(uintptr_t address, int offsets[], int offsetCount);
	static int GetProcessIdByName(char* processName);
};