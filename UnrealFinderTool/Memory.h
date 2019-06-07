#pragma once
#include <string>
#include <psapi.h>
#pragma comment(lib, "psapi")

class JsonVar;

class Memory
{
	bool useKernal = false;
	static BypaPH* bypaPh;
public:
	HANDLE ProcessHandle = nullptr;
	int ProcessId = 0;
	BOOL Is64Bit = false;

	explicit Memory(HANDLE processHandle, bool useKernal = false);
	explicit Memory(int processId, bool useKernal = false);
	void UpdateHandle(HANDLE processHandle);
	BOOL SetPrivilegeM(HANDLE hToken, LPCTSTR lpszPrivilege, BOOL bEnablePrivilege);
	BOOL GetDebugPrivileges();
	static int GetProcessIdByName(const std::string& processName);
	static std::string GetProcessNameById(DWORD pId);
	MODULEINFO GetModuleInfo(LPCTSTR lpModuleName);
	bool SuspendProcess();
	bool ResumeProcess();
	bool IsSuspend();

	template<typename T>
	int Read(const uintptr_t address, T& ret)
	{
		if (address == static_cast<uintptr_t>(-1))
			return 0;

		SIZE_T numberOfBytesActuallyRead = 0;
		const SIZE_T numberOfBytesToRead = sizeof(T);

		uintptr_t remoteAddress = reinterpret_cast<uintptr_t>(&ret);

		if (useKernal)
			bypaPh->RWVM(bypaPh->m_hTarget, reinterpret_cast<LPVOID>(address), reinterpret_cast<LPVOID>(remoteAddress), numberOfBytesToRead, &numberOfBytesActuallyRead);
		else
			ReadProcessMemory(ProcessHandle, reinterpret_cast<LPVOID>(address), reinterpret_cast<LPVOID>(remoteAddress), numberOfBytesToRead, &numberOfBytesActuallyRead);

		return numberOfBytesActuallyRead;
	}
	int ReadBytes(uintptr_t address, LPVOID buf, uint32_t len);
	int ReadBytes(uintptr_t baseAddress, JsonVar& jsonVar, const LPVOID buf);
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
	std::string ReadText(uintptr_t address);
	std::string ReadPointerText(const uintptr_t address, int offsets[], int offsetCount);
};
