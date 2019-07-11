#include "pch.h"
#include "JsonReflector.h"
#include "Memory.h"

#include <TlHelp32.h>
#include <psapi.h>

BypaPH* Memory::bypaPh = nullptr;

Memory::Memory(HANDLE processHandle, const bool useKernel)
{
	if (processHandle == nullptr || processHandle == INVALID_HANDLE_VALUE)
		throw std::exception("processId can't be NULL");

	ProcessHandle = processHandle;
	ProcessId = GetProcessId(processHandle);
	this->useKernel = useKernel;
	if (useKernel && bypaPh == nullptr)
		bypaPh = new BypaPH(ProcessId);

	IsWow64Process(ProcessHandle, &Is64);
	Is64 = !Is64;
}

Memory::Memory(const int processId, const bool useKernel)
{
	if (processId == 0)
		return;

	ProcessHandle = OpenProcess(PROCESS_ALL_ACCESS, false, processId);
	ProcessId = GetProcessId(ProcessHandle);
	this->useKernel = useKernel;
	if (useKernel && bypaPh == nullptr)
		bypaPh = new BypaPH(ProcessId);

	IsWow64Process(ProcessHandle, &Is64);
	Is64 = !Is64;
}

void Memory::UpdateHandle(HANDLE processHandle)
{
	ProcessHandle = processHandle;
}

int Memory::GetProcessIdByName(const std::string& processName)
{
	PROCESSENTRY32 pe32;
	pe32.dwSize = sizeof(PROCESSENTRY32);
	HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);

	if (Process32First(hSnapshot, &pe32))
	{
		do
		{
			if (processName == pe32.szExeFile)
				break;
		}
		while (Process32Next(hSnapshot, &pe32));
	}

	if (hSnapshot != INVALID_HANDLE_VALUE)
		CloseHandle(hSnapshot);

	if (GetLastError() != 0) return 0;
	return pe32.th32ProcessID;
}

std::string Memory::GetProcessNameById(const DWORD pId)
{
	std::string ret;
	PROCESSENTRY32 pe32;
	pe32.dwSize = sizeof(PROCESSENTRY32);
	HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);

	if (Process32First(hSnapshot, &pe32))
	{
		do
		{
			if (pe32.th32ProcessID == pId)
			{
				ret = pe32.szExeFile;
				break;
			}
		}
		while (Process32Next(hSnapshot, &pe32));
	}

	if (hSnapshot != INVALID_HANDLE_VALUE)
		CloseHandle(hSnapshot);

	const int err = GetLastError();
	//std::cout << err << std::endl;
	if (err != 0)
		return ret;
	return ret;
}

std::vector<MODULEENTRY32> Memory::GetModuleList() const
{
	std::vector<MODULEENTRY32> ret;
	MODULEENTRY32 mod;
	mod.dwSize = sizeof(MODULEENTRY32);

	HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE, ProcessId);

	if (Module32First(hSnapshot, &mod))
	{
		do
		{
			ret.push_back(mod);
		} while (Module32Next(hSnapshot, &mod));
	}

	if (hSnapshot != INVALID_HANDLE_VALUE)
		CloseHandle(hSnapshot);

	return ret;
}

bool Memory::GetModuleInfo(const std::string& lpModuleName, MODULEENTRY32& moduleInfo) const
{
	auto modules = GetModuleList();
	for (auto& module : modules)
	{
		if (lpModuleName == module.szModule)
		{
			moduleInfo = module;
			return true;
		}
	}

	return false;
}

bool Memory::IsHandleValid(HANDLE processHandle)
{
	if (!processHandle || processHandle == INVALID_HANDLE_VALUE)
		return false;

	DWORD handleInformation;
	return GetHandleInformation(processHandle, &handleInformation) == TRUE;
}

bool Memory::IsValidProcess(const int p_id, PHANDLE pHandle)
{
	DWORD exitCode;
	const HANDLE p = OpenProcess(PROCESS_ALL_ACCESS, false, p_id);
	const bool valid = p_id != 0 && GetExitCodeProcess(p, &exitCode) != FALSE && exitCode == STILL_ACTIVE;

	if (pHandle && valid)
		*pHandle = p;
	else if (valid)
		CloseHandle(p);

	return valid;
}

bool Memory::IsValidProcess(const int p_id)
{
	return IsValidProcess(p_id, nullptr);
}

bool Memory::IsStaticAddress(const uintptr_t address) const
{
	if (ProcessId == NULL || address == NULL)
		return false;

	const auto queryVirtualMemory = reinterpret_cast<hsNtQueryVirtualMemory>(GetProcAddress(LoadLibraryW(L"ntdll.dll"), "NtQueryVirtualMemory"));

	if (!queryVirtualMemory)
		return false;

	if (!IsHandleValid(ProcessHandle))
		return false;

	SECTION_INFO sectionInformation;

	const NTSTATUS returnStatus = queryVirtualMemory(ProcessHandle, reinterpret_cast<PVOID>(address), MemoryMappedFilenameInformation, &sectionInformation, sizeof(sectionInformation), nullptr);

	if (!NT_SUCCESS(returnStatus))
		return false;

	wchar_t* deviceName = sectionInformation.szData;
	wchar_t* filePath = deviceName;

	while (*(filePath++) != '\\') {} 
	while (*(filePath++) != '\\') {}
	while (*(filePath++) != '\\') {}
	*(filePath - 1) = 0;

	auto driveLetters = new wchar_t[MAX_PATH + 1];
	auto driveSize = GetLogicalDriveStringsW(MAX_PATH, driveLetters);

	if (driveSize > MAX_PATH)
	{
		delete[] driveLetters;
		driveLetters = new wchar_t[size_t(driveSize + 1)];
		driveSize = GetLogicalDriveStringsW(driveSize, driveLetters);
	}

	for (int i = 0; i != driveSize / 4; ++i)
	{
		driveLetters[i * 4 + 2] = 0;
		wchar_t buffer[64]{ 0 };

		QueryDosDeviceW(&driveLetters[size_t(i * 4)], buffer, 64 * 2);

		if (!wcscmp(buffer, deviceName))
		{
			filePath -= 3;
			filePath[2] = '\\';
			filePath[1] = ':';
			filePath[0] = driveLetters[i * 4];

			delete[] driveLetters;

			/*
			HMODULE Ret = GetModuleHandleW(FilePath);

			if (nullptr == Ret)
			{
				return FALSE;
			}
			Address = reinterpret_cast<uintptr_t>(Ret);
			*/

			return true;
		}
	}

	delete[] driveLetters;
	return false;
}

bool Memory::SuspendProcess() const
{
	typedef LONG (NTAPI *NtSuspendProcess)(HANDLE ProcessHandle);
	static auto pfnNtSuspendProcess = reinterpret_cast<NtSuspendProcess>(GetProcAddress(GetModuleHandle("ntdll"), "NtSuspendProcess"));

	return NT_SUCCESS(pfnNtSuspendProcess(ProcessHandle));
}

bool Memory::ResumeProcess() const
{
	typedef LONG(NTAPI *NtResumeProcess)(HANDLE ProcessHandle);
	static auto pfnNtResumeProcess = reinterpret_cast<NtResumeProcess>(GetProcAddress(GetModuleHandle("ntdll"), "NtResumeProcess"));

	return NT_SUCCESS(pfnNtResumeProcess(ProcessHandle));
}

bool Memory::TerminateProcess() const
{
	typedef LONG(NTAPI* NtTerminateProcess)(IN HANDLE ProcessHandle, NTSTATUS ExitStatus);
	static auto pfnNtTerminateProcess = reinterpret_cast<NtTerminateProcess>(GetProcAddress(GetModuleHandle("ntdll"), "NtTerminateProcess"));

	return NT_SUCCESS(pfnNtTerminateProcess(ProcessHandle, 0));
}

bool Memory::IsSuspend()
{
	return false;
}

BOOL Memory::SetPrivilegeM(HANDLE hToken, const LPCTSTR lpszPrivilege, const BOOL bEnablePrivilege)
{
	TOKEN_PRIVILEGES tp;
	LUID luid;

	if (!LookupPrivilegeValue(nullptr, lpszPrivilege, &luid))
	{
		//printf("LookupPrivilegeValue error: %u\n", GetLastError() );
		return FALSE;
	}

	tp.PrivilegeCount = 1;
	tp.Privileges[0].Luid = luid;
	if (bEnablePrivilege)
		tp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
	else
		tp.Privileges[0].Attributes = 0;

	if (!AdjustTokenPrivileges(hToken, FALSE, &tp, sizeof(TOKEN_PRIVILEGES), static_cast<PTOKEN_PRIVILEGES>(nullptr),
	                           static_cast<PDWORD>(nullptr)))
	{
		//printf("AdjustTokenPrivileges error: %u\n", GetLastError() );
		return FALSE;
	}

	if (GetLastError() == ERROR_NOT_ALL_ASSIGNED)
	{
		//printf("The token does not have the specified privilege. \n");
		return FALSE;
	}

	return TRUE;
}

BOOL Memory::GetDebugPrivileges()
{
	HANDLE hToken = nullptr;
	if (!OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES, &hToken))
		return FALSE;

	return SetPrivilegeM(hToken, SE_DEBUG_NAME, TRUE);
}

size_t Memory::ReadBytes(const uintptr_t address, const LPVOID buf, const size_t len)
{
	if (address == static_cast<uintptr_t>(-1))
		return 0;

	SIZE_T numberOfBytesActuallyRead = 0;
	const SIZE_T numberOfBytesToRead = len;

	if (useKernel)
	{
		const auto state = bypaPh->RWVM(bypaPh->m_hTarget,
		                                reinterpret_cast<LPVOID>(address),
		                                buf,
		                                numberOfBytesToRead,
		                                &numberOfBytesActuallyRead);
		/*if (state != STATUS_PARTIAL_COPY && state != STATUS_SUCCESS)
			std::cout << "Memory Error! " << GetLastError() << std::endl;*/
	}
	else
	{
		const auto success = ReadProcessMemory(ProcessHandle, reinterpret_cast<LPCVOID>(address), buf,
		                                       numberOfBytesToRead, &numberOfBytesActuallyRead);
		/*if (!success && GetLastError() != 299)
			std::cout << "Memory Error! " << GetLastError() << std::endl;*/
	}

	return numberOfBytesActuallyRead;
}

size_t Memory::ReadBytes(const uintptr_t baseAddress, JsonVar& jsonVar, const LPVOID buf)
{
	if (baseAddress == static_cast<uintptr_t>(-1))
		return 0;

	SIZE_T numberOfBytesActuallyRead = 0;
	const SIZE_T numberOfBytesToRead = jsonVar.Size;

	if (useKernel)
	{
		const bool state = bypaPh->RWVM(bypaPh->m_hTarget,
		                                reinterpret_cast<LPVOID>(baseAddress + jsonVar.Offset),
		                                buf,
		                                numberOfBytesToRead,
		                                &numberOfBytesActuallyRead);

		/*if (state != STATUS_PARTIAL_COPY && state != STATUS_SUCCESS)
			std::cout << "Memory Error! " << GetLastError() << std::endl;*/
	}
	else
	{
		const bool success = ReadProcessMemory(ProcessHandle,
		                                       reinterpret_cast<LPCVOID>(baseAddress + jsonVar.Offset),
		                                       buf,
		                                       numberOfBytesToRead,
		                                       &numberOfBytesActuallyRead);

		/*if (!success && GetLastError() != 299)
			std::cout << "Memory Error! " << GetLastError() << std::endl;*/
	}

	return numberOfBytesActuallyRead;
}

bool Memory::ReadBool(const uintptr_t address)
{
	if (address == static_cast<uintptr_t>(-1))
		return false;

	bool buffer = true;
	const SIZE_T numberOfBytesToRead = sizeof buffer; //this is equal to 1
	SIZE_T numberOfBytesActuallyRead;

	if (useKernel)
	{
		const auto state = bypaPh->RWVM(bypaPh->m_hTarget,
		                                reinterpret_cast<LPVOID>(address),
		                                &buffer,
		                                numberOfBytesToRead,
		                                &numberOfBytesActuallyRead);
		if (state != STATUS_SUCCESS)
			return false;
	}
	else
	{
		const auto state = ReadProcessMemory(ProcessHandle, reinterpret_cast<LPCVOID>(address), &buffer,
		                                     numberOfBytesToRead, &numberOfBytesActuallyRead);
		if (!state)
		{
			return false;
		}
	}

	return buffer;
}

int Memory::ReadInt(const uintptr_t address)
{
	if (address == static_cast<uintptr_t>(-1))
		return -1;

	int buffer = 0;
	const SIZE_T numberOfBytesToRead = sizeof buffer; //this is equal to 4
	SIZE_T numberOfBytesActuallyRead;

	if (useKernel)
	{
		const auto state = bypaPh->RWVM(bypaPh->m_hTarget,
		                                reinterpret_cast<LPVOID>(address),
		                                &buffer,
		                                numberOfBytesToRead,
		                                &numberOfBytesActuallyRead);
		if (state != STATUS_SUCCESS)
			return -1;
	}
	else
	{
		const auto state = ReadProcessMemory(ProcessHandle, reinterpret_cast<LPCVOID>(address), &buffer,
		                                     numberOfBytesToRead, &numberOfBytesActuallyRead);
		if (!state)
		{
			return -1;
		}
	}

	return buffer;
}

int64_t Memory::ReadInt64(const uintptr_t address)
{
	if (address == static_cast<uintptr_t>(-1))
		return -1;
	INT64 buffer = 0;
	const SIZE_T numberOfBytesToRead = sizeof(buffer); //this is equal to 8
	SIZE_T numberOfBytesActuallyRead;
	if (useKernel)
	{
		const auto state = bypaPh->RWVM(bypaPh->m_hTarget,
		                                reinterpret_cast<LPVOID>(address),
		                                &buffer,
		                                numberOfBytesToRead,
		                                &numberOfBytesActuallyRead);
		if (state != STATUS_SUCCESS)
			return -1;
	}
	else
	{
		const auto state = ReadProcessMemory(ProcessHandle, reinterpret_cast<LPCVOID>(address), &buffer,
		                                     numberOfBytesToRead, &numberOfBytesActuallyRead);
		if (!state)
		{
			return -1;
		}
	}
	return buffer;
}

uint32_t Memory::ReadUInt(const uintptr_t address)
{
	if (address == static_cast<uintptr_t>(-1))
		return -1;

	UINT32 buffer = 0;
	const SIZE_T numberOfBytesToRead = sizeof buffer; //this is equal to 4
	SIZE_T numberOfBytesActuallyRead;
	if (useKernel)
	{
		const auto state = bypaPh->RWVM(bypaPh->m_hTarget,
		                                reinterpret_cast<LPVOID>(address),
		                                &buffer,
		                                numberOfBytesToRead,
		                                &numberOfBytesActuallyRead);
		if (state != STATUS_SUCCESS)
			return -1;
	}
	else
	{
		const auto state = ReadProcessMemory(ProcessHandle, reinterpret_cast<LPCVOID>(address), &buffer,
		                                     numberOfBytesToRead, &numberOfBytesActuallyRead);
		if (!state)
		{
			return -1;
		}
	}
	return buffer;
}

uint64_t Memory::ReadUInt64(const uintptr_t address)
{
	if (address == static_cast<uintptr_t>(-1))
		return -1;
	UINT64 buffer = 0;
	const SIZE_T numberOfBytesToRead = sizeof(buffer); //this is equal to 8
	SIZE_T numberOfBytesActuallyRead;
	if (useKernel)
	{
		const auto state = bypaPh->RWVM(bypaPh->m_hTarget,
		                                reinterpret_cast<LPVOID>(address),
		                                &buffer,
		                                numberOfBytesToRead,
		                                &numberOfBytesActuallyRead);
		if (state != STATUS_SUCCESS)
			return -1;
	}
	else
	{
		const auto state = ReadProcessMemory(ProcessHandle, reinterpret_cast<LPCVOID>(address), &buffer,
		                                     numberOfBytesToRead, &numberOfBytesActuallyRead);
		if (!state)
		{
			// std::cout << "Memory Error! " << GetLastError() << std::endl;
			return -1;
		}
	}
	return buffer;
}

float Memory::ReadFloat(const uintptr_t address)
{
	if (address == static_cast<uintptr_t>(-1))
		return -1;

	float buffer = 0.0;
	const SIZE_T numberOfBytesToRead = sizeof(buffer); //this is equal to 4
	SIZE_T numberOfBytesActuallyRead;
	if (useKernel)
	{
		const auto state = bypaPh->RWVM(bypaPh->m_hTarget,
		                                reinterpret_cast<LPVOID>(address),
		                                &buffer,
		                                numberOfBytesToRead,
		                                &numberOfBytesActuallyRead);
		if (state != STATUS_SUCCESS)
			return -1;
	}
	else
	{
		const auto state = ReadProcessMemory(ProcessHandle, reinterpret_cast<LPCVOID>(address), &buffer,
		                                     numberOfBytesToRead, &numberOfBytesActuallyRead);
		if (!state)
		{
			return -1;
		}
	}
	return buffer;
}

int Memory::GetPointerAddress(const uintptr_t address, const int offsets[], const int offsetCount)
{
	if (address == static_cast<uintptr_t>(-1))
		return -1;

	auto ptr = ReadInt(address);
	for (auto i = 0; i < offsetCount - 1; i++)
	{
		ptr += offsets[i];
		ptr = ReadInt(ptr);
	}
	ptr += offsets[offsetCount - 1];
	return ptr;
}

uintptr_t Memory::ReadAddress(const uintptr_t address)
{
	return Is64 ? ReadInt64(address) : ReadInt(address);
}

int Memory::ReadPointerInt(const uintptr_t address, int offsets[], const int offsetCount)
{
	if (address == static_cast<uintptr_t>(-1))
		return -1;

	return ReadInt(GetPointerAddress(address, offsets, offsetCount));
}

float Memory::ReadPointerFloat(const uintptr_t address, int offsets[], int offsetCount)
{
	if (address == static_cast<uintptr_t>(-1))
		return -1;
	return ReadFloat(GetPointerAddress(address, offsets, offsetCount));
}

std::string Memory::ReadText(uintptr_t address)
{
	std::string ret;
	if (address == static_cast<uintptr_t>(-1))
		return "";

	char buffer = 0;
	const SIZE_T numberOfBytesToRead = sizeof buffer;
	SIZE_T numberOfBytesActuallyRead;

	while (true)
	{
		if (useKernel)
		{
			const auto state = bypaPh->RWVM(bypaPh->m_hTarget,
			                                reinterpret_cast<LPVOID>(address),
			                                &buffer,
			                                numberOfBytesToRead,
			                                &numberOfBytesActuallyRead);
			if (state != STATUS_SUCCESS)
				return "";
		}
		else
		{
			const auto state = ReadProcessMemory(ProcessHandle, reinterpret_cast<LPCVOID>(address), &buffer,
			                                     numberOfBytesToRead, &numberOfBytesActuallyRead);
			if (!state)
				return "";
		}

		if (buffer == 0)
			break;
		ret.push_back(buffer);
		address++;
	}
	return ret;
}

std::string Memory::ReadPointerText(const uintptr_t address, int offsets[], int offsetCount)
{
	if (address == static_cast<uintptr_t>(-1))
		return "";
	return ReadText(GetPointerAddress(address, offsets, offsetCount));
}
