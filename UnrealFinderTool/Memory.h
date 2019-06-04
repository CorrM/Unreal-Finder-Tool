#pragma once
#include <string>
#include <psapi.h>

#pragma comment(lib, "psapi")
using std::string;

class Memory
{
	bool use_kernal = false;
	static BypaPH* bypaPh;
public:
	HANDLE ProcessHandle = nullptr;
	int ProcessId = 0;
	BOOL Is64Bit = false;

	Memory(HANDLE processHandle, bool useKernal = false);
	Memory(int processId, bool useKernal = false);
	void UpdateHandle(HANDLE processHandle);
	BOOL SetPrivilegeM(HANDLE hToken, LPCTSTR lpszPrivilege, BOOL bEnablePrivilege);
	BOOL GetDebugPrivileges();
	template<typename T>
	int Read(const uintptr_t address, T& ret, const int skipLocalBytes = 0)
	{
		if (address == static_cast<uintptr_t>(-1))
			return 0;

		SIZE_T numberOfBytesActuallyRead = 0;
		const SIZE_T numberOfBytesToRead = sizeof(T) - skipLocalBytes;

		uintptr_t remoteAddress = reinterpret_cast<uintptr_t>(&ret) + skipLocalBytes;

		if (use_kernal)
			bypaPh->RWVM(bypaPh->m_hTarget, reinterpret_cast<PVOID>(address), reinterpret_cast<PVOID>(remoteAddress), numberOfBytesToRead, &numberOfBytesActuallyRead);
		else
			ReadProcessMemory(ProcessHandle, reinterpret_cast<PVOID>(address), reinterpret_cast<PVOID>(remoteAddress), numberOfBytesToRead, &numberOfBytesActuallyRead);

		return numberOfBytesActuallyRead;
	}
	int ReadBytes(uintptr_t address, PVOID buf, uint32_t len);
	bool ReadBool(uintptr_t address);
	int ReadInt(uintptr_t address);
	INT64 ReadInt64(uintptr_t address);
	UINT32 ReadUInt(uintptr_t address);
	UINT64 ReadUInt64(uintptr_t address);
	int GetPointerAddress(uintptr_t address, const int offsets[], int offsetCount);
	uintptr_t ReadAddress(uintptr_t address);
	int ReadPointerInt(uintptr_t address, int offsets[], int offsetCount);
	float ReadFloat(uintptr_t address);
	float ReadPointerFloat(uintptr_t address, int offsets[], int offsetCount);
	string ReadText(uintptr_t address);
	string ReadPointerText(const uintptr_t address, int offsets[], int offsetCount);
	static int GetProcessIdByName(char* processName);
	MODULEINFO GetModuleInfo(LPCTSTR lpModuleName);
	bool SuspendProcess();
	bool ResumeProcess();
};
