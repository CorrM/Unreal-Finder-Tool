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

	static int GetProcessIdByName(const std::string& processName);
	static std::string GetProcessNameById(DWORD pId);
	static bool IsValidProcess(int p_id, HANDLE* pHandle);
	static bool IsValidProcess(int p_id);
	static bool IsStaticAddress(uintptr_t address);

	void UpdateHandle(HANDLE processHandle);
	BOOL SetPrivilegeM(HANDLE hToken, LPCTSTR lpszPrivilege, BOOL bEnablePrivilege);
	BOOL GetDebugPrivileges();
	MODULEINFO GetModuleInfo(LPCTSTR lpModuleName);
	bool SuspendProcess();
	bool ResumeProcess();
	bool IsSuspend();

	template<typename T>
	size_t Read(uintptr_t address, T& ret);
	size_t ReadBytes(uintptr_t address, LPVOID buf, size_t len);
	size_t ReadBytes(uintptr_t baseAddress, JsonVar& jsonVar, LPVOID buf);
	bool ReadBool(uintptr_t address);
	int ReadInt(uintptr_t address);
	INT64 ReadInt64(uintptr_t address);
	UINT32 ReadUInt(uintptr_t address);
	UINT64 ReadUInt64(uintptr_t address);
	uintptr_t ReadAddress(uintptr_t address);
	float ReadFloat(uintptr_t address);
	std::string ReadText(uintptr_t address);
	std::string ReadPointerText(const uintptr_t address, int offsets[], int offsetCount);

	int ReadPointerInt(uintptr_t address, int offsets[], int offsetCount);
	float ReadPointerFloat(uintptr_t address, int offsets[], int offsetCount);
	int GetPointerAddress(uintptr_t address, const int offsets[], int offsetCount);
};

template <typename T>
size_t Memory::Read(const uintptr_t address, T& ret)
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
