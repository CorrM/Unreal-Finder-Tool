#pragma once
#include <string>
#include <Psapi.h>
#include <TlHelp32.h>
#include <vector>
#pragma comment(lib, "psapi")

class JsonVar;

class Memory
{
	typedef enum _MEMORY_INFORMATION_CLASS
	{
		MemoryBasicInformation,
		MemoryWorkingSetInformation,
		MemoryMappedFilenameInformation,
		MemoryRegionInformation,
		MemoryWorkingSetExInformation,
		MemorySharedCommitInformation,
		MemoryImageInformation,
		MemoryRegionInformationEx,
		MemoryPrivilegedBasicInformation,
		MemoryEnclaveImageInformation,
		MemoryBasicInformationCapped
	} MEMORY_INFORMATION_CLASS;

	typedef NTSTATUS(NTAPI* hsNtQueryVirtualMemory)(
		HANDLE ProcessHandle,
		PVOID BaseAddress, MEMORY_INFORMATION_CLASS MemoryInformationClass,
		PVOID Buffer, SIZE_T Length, PSIZE_T ResultLength
		);

	struct SECTION_INFO
	{
		WORD Len;
		WORD MaxLen;
		wchar_t* szData;
		BYTE pData[MAX_PATH * 2];
	};

	bool useKernel = false;
	static BypaPH* bypaPh;
public:
	HANDLE ProcessHandle = nullptr;
	int ProcessId = 0;
	BOOL Is64 = false;

	explicit Memory(HANDLE processHandle, bool useKernel = false);
	explicit Memory(int processId, bool useKernel = false);

	static int GetProcessIdByName(const std::string& processName);
	static std::string GetProcessNameById(DWORD pId);
	static bool IsValidProcess(int p_id, PHANDLE pHandle);
	static bool IsValidProcess(int p_id);
	static bool IsHandleValid(HANDLE processHandle);

	bool IsStaticAddress(uintptr_t address);
	void UpdateHandle(HANDLE processHandle);
	BOOL SetPrivilegeM(HANDLE hToken, LPCTSTR lpszPrivilege, BOOL bEnablePrivilege);
	BOOL GetDebugPrivileges();
	std::vector<MODULEENTRY32> GetModuleList();
	MODULEINFO GetModuleInfo(LPCTSTR lpModuleName);
	bool SuspendProcess();
	bool ResumeProcess();
	bool TerminateProcess();
	bool IsSuspend();

	template<typename T>
	size_t Read(uintptr_t address, T& ret);
	size_t ReadBytes(uintptr_t address, LPVOID buf, size_t len);
	size_t ReadBytes(uintptr_t baseAddress, JsonVar& jsonVar, LPVOID buf);
	bool ReadBool(uintptr_t address);
	int ReadInt(uintptr_t address);
	int64_t ReadInt64(uintptr_t address);
	uint32_t ReadUInt(uintptr_t address);
	uint64_t ReadUInt64(uintptr_t address);
	uintptr_t ReadAddress(uintptr_t address);
	float ReadFloat(uintptr_t address);
	std::string ReadText(uintptr_t address);
	std::string ReadPointerText(uintptr_t address, int offsets[], int offsetCount);

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

	if (useKernel)
		bypaPh->RWVM(bypaPh->m_hTarget, reinterpret_cast<LPVOID>(address), reinterpret_cast<LPVOID>(remoteAddress), numberOfBytesToRead, &numberOfBytesActuallyRead);
	else
		ReadProcessMemory(ProcessHandle, reinterpret_cast<LPVOID>(address), reinterpret_cast<LPVOID>(remoteAddress), numberOfBytesToRead, &numberOfBytesActuallyRead);

	return numberOfBytesActuallyRead;
}
