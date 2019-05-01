#pragma once
#include <Windows.h>
#include <TlHelp32.h>
#include <string>
#include <psapi.h>
#include <iostream>
#include "BypaPH.h"

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
	DWORD GetModuleBase(string sModuleName);
	DWORD GetModules();
	BOOL SetPrivilegeM(HANDLE hToken, LPCTSTR lpszPrivilege, BOOL bEnablePrivilege);
	BOOL GetDebugPrivileges();
	int ReadBytes(uintptr_t address, BYTE* buf, int len);
	int ReadInt(uintptr_t address);
	int GetPointerAddress(uintptr_t address, const int offsets[], int offsetCount);
	int ReadPointerInt(uintptr_t address, int offsets[], int offsetCount);
	float ReadFloat(uintptr_t address);
	float ReadPointerFloat(uintptr_t address, int offsets[], int offsetCount);
	string ReadText(uintptr_t address);
	string ReadPointerText(uintptr_t address, int offsets[], int offsetCount);
	static int GetProcessIdByName(char* processName);
};