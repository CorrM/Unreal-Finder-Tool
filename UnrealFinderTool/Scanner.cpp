#include "pch.h"
#include "Scanner.h"
#include <assert.h>
#include <algorithm>
#include <Tlhelp32.h>

using namespace Hyperscan;

BOOL HYPERSCAN_CHECK::IsHandleValid(HANDLE ProcessHandle)
{
	if (NULL == ProcessHandle || INVALID_HANDLE_VALUE == ProcessHandle)
	{
		return FALSE;
	}

	DWORD HandleInformation;

	if (TRUE == GetHandleInformation(ProcessHandle, &HandleInformation))
	{
		return TRUE;
	}

	return FALSE;
}

BOOL HYPERSCAN_CHECK::IsProcess64Bit(HANDLE ProcessHandle)
{
	if (NULL == ProcessHandle || INVALID_HANDLE_VALUE == ProcessHandle)
	{
		return FALSE;
	}

	if (TRUE == HYPERSCAN_CHECK::IsHandleValid(ProcessHandle))
	{
		BOOL CheckResult = FALSE;

		if (TRUE == IsWow64Process(ProcessHandle, &CheckResult))
		{
			return CheckResult;
		}

		return FALSE;
	}

	return FALSE;
}

struct SECTION_INFO
{
	WORD Len;
	WORD MaxLen;
	wchar_t * szData;
	BYTE pData[MAX_PATH * 2];
};

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

typedef NTSTATUS(NTAPI * hsNtQueryVirtualMemory)(
	HANDLE ProcessHandle,
	PVOID BaseAddress, MEMORY_INFORMATION_CLASS MemoryInformationClass,
	PVOID Buffer, SIZE_T Length, PSIZE_T ResultLength
	);

#define NT_SUCCESS(Status) ((Status) >= 0)

BOOL HYPERSCAN_CHECK::IsAddressStatic(DWORD ProcessID, BYTE * &Address)
{
	if (NULL == ProcessID || nullptr == Address)
	{
		return FALSE;
	}

	LPVOID QueryVirtualMemoryAddress = GetProcAddress(LoadLibraryW(L"ntdll.dll"), "NtQueryVirtualMemory");

	if (nullptr == QueryVirtualMemoryAddress)
	{
		return FALSE;
	}

	auto QueryVirtualMemory = reinterpret_cast<hsNtQueryVirtualMemory>(QueryVirtualMemoryAddress);

	HANDLE ProcessHandle = OpenProcess(PROCESS_ALL_ACCESS, FALSE, ProcessID);

	if (FALSE == HYPERSCAN_CHECK::IsHandleValid(ProcessHandle))
	{
		return FALSE;
	}

	SECTION_INFO SectionInformation;

	NTSTATUS ReturnStatus = QueryVirtualMemory(ProcessHandle, Address, MemoryMappedFilenameInformation, &SectionInformation, sizeof(SectionInformation), nullptr);

	if (!NT_SUCCESS(ReturnStatus))
	{
		CloseHandle(ProcessHandle);
		return FALSE;
	}

	wchar_t * DeviceName = SectionInformation.szData;
	wchar_t * FilePath = DeviceName;

	while (*(FilePath++) != '\\');
	while (*(FilePath++) != '\\');
	while (*(FilePath++) != '\\');
	*(FilePath - 1) = 0;

	wchar_t * DriveLetters = new wchar_t[MAX_PATH + 1];
	auto DriveSize = GetLogicalDriveStringsW(MAX_PATH, DriveLetters);

	if (DriveSize > MAX_PATH)
	{
		delete[] DriveLetters;
		DriveLetters = new wchar_t[DriveSize + 1];
		DriveSize = GetLogicalDriveStringsW(DriveSize, DriveLetters);
	}

	for (int i = 0; i != DriveSize / 4; ++i)
	{
		DriveLetters[i * 4 + 2] = 0;
		wchar_t Buffer[64]{ 0 };

		QueryDosDeviceW(&DriveLetters[i * 4], Buffer, sizeof(Buffer));

		if (!wcscmp(Buffer, DeviceName))
		{
			FilePath -= 3;
			FilePath[2] = '\\';
			FilePath[1] = ':';
			FilePath[0] = DriveLetters[i * 4];

			delete[] DriveLetters;

			BYTE * Ret = reinterpret_cast<BYTE*>(GetModuleHandleW(FilePath));

			if (nullptr == Ret)
			{
				CloseHandle(ProcessHandle);
				return FALSE;
			}

			Address = Ret;

			CloseHandle(ProcessHandle);
			return TRUE;
		}
	}

	delete[] DriveLetters;
	CloseHandle(ProcessHandle);

	return FALSE;
}

std::vector<UINT_PTR> HYPERSCAN_SCANNER::ScanMemory(DWORD ProcessID, UINT_PTR ModuleBaseAddress, UINT_PTR ModuleSize, INT ScanValue,
	ScanAllignment AllignmentOfScan, ScanType TypeOfScan)
{
	std::vector<UINT_PTR> AddressHolder;
	AddressHolder.clear();

	if (NULL == ProcessID || NULL == ModuleBaseAddress || NULL == ModuleSize || NULL == AllignmentOfScan
		|| NULL == TypeOfScan)
	{
		return AddressHolder;
	}

	_MEMORY_BASIC_INFORMATION BasicInformation;
	UINT_PTR AddressForScan = ModuleBaseAddress;
	HANDLE QueryHandle = OpenProcess(PROCESS_ALL_ACCESS, FALSE, ProcessID);

#ifndef _WIN64
	if (TRUE == HYPERSCAN_CHECK::IsProcess64Bit(QueryHandle))
	{
		assert("Incompatibility in architectures!");
	}
#endif


	if (INVALID_HANDLE_VALUE == QueryHandle)
	{
		return AddressHolder;
	}

	while (VirtualQueryEx(QueryHandle, reinterpret_cast<VOID*>(AddressForScan), &BasicInformation, sizeof(BasicInformation))
		&& AddressForScan < (ModuleBaseAddress + ModuleSize))
	{
		if ((BasicInformation.State & MEM_COMMIT))
		{
			UCHAR * MemoryBlock = new UCHAR[BasicInformation.RegionSize];
			if (ReadProcessMemory(QueryHandle, reinterpret_cast<VOID*>(AddressForScan), MemoryBlock, BasicInformation.RegionSize,
				nullptr))
			{
				for (unsigned int scanIndex = 0; scanIndex != BasicInformation.RegionSize / AllignmentOfScan; ++scanIndex)
				{
					if (HyperscanTypeAll == TypeOfScan)
					{
						AddressHolder.push_back(AddressForScan + scanIndex * AllignmentOfScan);
					}
					else if (HyperscanTypeExact == TypeOfScan)
					{
						if (*reinterpret_cast<INT*>(MemoryBlock + scanIndex * AllignmentOfScan) == ScanValue)
						{
							AddressHolder.push_back(AddressForScan + scanIndex * AllignmentOfScan);
						}
					}
					else if (HyperscanTypeSmaller == TypeOfScan)
					{
						if (*reinterpret_cast<INT*>(MemoryBlock + scanIndex * AllignmentOfScan) < ScanValue)
						{
							AddressHolder.push_back(AddressForScan + scanIndex * AllignmentOfScan);
						}
					}
					else if (HyperscanTypeBigger == TypeOfScan)
					{
						if (*reinterpret_cast<INT*>(MemoryBlock + scanIndex * AllignmentOfScan) > ScanValue)
						{
							AddressHolder.push_back(AddressForScan + scanIndex * AllignmentOfScan);
						}
					}
					else if (HyperscanTypeDifferent == TypeOfScan)
					{
						if (*reinterpret_cast<INT*>(MemoryBlock + scanIndex * AllignmentOfScan) != ScanValue)
						{
							AddressHolder.push_back(AddressForScan + scanIndex * AllignmentOfScan);
						}
					}
					else
					{
						if (*reinterpret_cast<INT*>(MemoryBlock + scanIndex * AllignmentOfScan) == ScanValue)
						{
							AddressHolder.push_back(AddressForScan + scanIndex * AllignmentOfScan);
						}
					}
				}
			}
			delete[] MemoryBlock;
		}
		AddressForScan = reinterpret_cast<UINT_PTR>(BasicInformation.BaseAddress) + BasicInformation.RegionSize;
	}

	CloseHandle(QueryHandle);
	return AddressHolder;
}

std::vector<UINT_PTR> HYPERSCAN_SCANNER::ScanModules(DWORD ProcessID, INT ScanValue, ScanAllignment AllignmentOfScan,
	ScanType TypeOfScan)
{
	std::vector<UINT_PTR> AddressHolder;
	AddressHolder.clear();

	if (NULL == ProcessID || NULL == AllignmentOfScan || NULL == TypeOfScan)
	{
		return AddressHolder;
	}

	HANDLE ModuleSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE | TH32CS_SNAPMODULE32, ProcessID);

	if (INVALID_HANDLE_VALUE == ModuleSnapshot)
	{
		return AddressHolder;
	}

	MODULEENTRY32 ModuleEntry;
	ModuleEntry.dwSize = sizeof(MODULEENTRY32);

	if (Module32First(ModuleSnapshot, &ModuleEntry))
	{
		do
		{
			std::vector<UINT_PTR> TemporaryAddressHolder;
			TemporaryAddressHolder.clear();

			TemporaryAddressHolder = ScanMemory(ProcessID, reinterpret_cast<UINT_PTR>(ModuleEntry.modBaseAddr), ModuleEntry.modBaseSize,
				ScanValue, AllignmentOfScan, TypeOfScan);

			AddressHolder.insert(AddressHolder.end(), TemporaryAddressHolder.begin(), TemporaryAddressHolder.end());
		} while (Module32Next(ModuleSnapshot, &ModuleEntry));
	}

	CloseHandle(ModuleSnapshot);
	return AddressHolder;
}

std::vector<UINT_PTR> HYPERSCAN_SCANNER::ScanWholeMemoryWithDelimiters(DWORD ProcessID, INT ScanValue, ScanAllignment AllignmentOfScan,
	ScanType TypeOfScan, DWORD BeginAddress, DWORD EndAddress)
{
	std::vector<UINT_PTR> AddressHolder;
	AddressHolder.clear();

	if (NULL == ProcessID || NULL == EndAddress || NULL == AllignmentOfScan || NULL == TypeOfScan)
	{
		return AddressHolder;
	}

	_MEMORY_BASIC_INFORMATION BasicInformation;
	UINT_PTR AddressForScan = BeginAddress;
	HANDLE QueryHandle = OpenProcess(PROCESS_ALL_ACCESS, FALSE, ProcessID);

#ifndef _WIN64
	if (TRUE == HYPERSCAN_CHECK::IsProcess64Bit(QueryHandle))
	{
		assert("Incompatibility in architectures!");
	}
#endif

	if (INVALID_HANDLE_VALUE == QueryHandle)
	{
		return AddressHolder;
	}

	while (VirtualQueryEx(QueryHandle, reinterpret_cast<VOID*>(AddressForScan), &BasicInformation, sizeof(BasicInformation))
		&& AddressForScan < EndAddress)
	{
		if ((BasicInformation.State & MEM_COMMIT))
		{
			UCHAR * MemoryBlock = new UCHAR[BasicInformation.RegionSize];
			if (ReadProcessMemory(QueryHandle, reinterpret_cast<VOID*>(AddressForScan), MemoryBlock, BasicInformation.RegionSize,
				nullptr))
			{
				for (unsigned int scanIndex = 0; scanIndex != BasicInformation.RegionSize / AllignmentOfScan; ++scanIndex)
				{
					if (HyperscanTypeAll == TypeOfScan)
					{
						AddressHolder.push_back(AddressForScan + scanIndex * AllignmentOfScan);
					}
					else if (HyperscanTypeExact == TypeOfScan)
					{
						if (*reinterpret_cast<INT*>(MemoryBlock + scanIndex * AllignmentOfScan) == ScanValue)
						{
							AddressHolder.push_back(AddressForScan + scanIndex * AllignmentOfScan);
						}
					}
					else if (HyperscanTypeSmaller == TypeOfScan)
					{
						if (*reinterpret_cast<INT*>(MemoryBlock + scanIndex * AllignmentOfScan) < ScanValue)
						{
							AddressHolder.push_back(AddressForScan + scanIndex * AllignmentOfScan);
						}
					}
					else if (HyperscanTypeBigger == TypeOfScan)
					{
						if (*reinterpret_cast<INT*>(MemoryBlock + scanIndex * AllignmentOfScan) > ScanValue)
						{
							AddressHolder.push_back(AddressForScan + scanIndex * AllignmentOfScan);
						}
					}
					else if (HyperscanTypeDifferent == TypeOfScan)
					{
						if (*reinterpret_cast<INT*>(MemoryBlock + scanIndex * AllignmentOfScan) != ScanValue)
						{
							AddressHolder.push_back(AddressForScan + scanIndex * AllignmentOfScan);
						}
					}
					else
					{
						if (*reinterpret_cast<INT*>(MemoryBlock + scanIndex * AllignmentOfScan) == ScanValue)
						{
							AddressHolder.push_back(AddressForScan + scanIndex * AllignmentOfScan);
						}
					}
				}
			}
			delete[] MemoryBlock;
		}
		AddressForScan = reinterpret_cast<UINT_PTR>(BasicInformation.BaseAddress) + BasicInformation.RegionSize;
	}

	CloseHandle(QueryHandle);
	return AddressHolder;
}

std::vector<UINT_PTR> HYPERSCAN_SCANNER::Scan(DWORD ProcessID, INT ScanValue, ScanAllignment AllignmentOfScan, ScanType TypeOfScan)
{
	std::vector<UINT_PTR> AddressHolder;
	AddressHolder.clear();

	if (NULL == ProcessID || NULL == AllignmentOfScan || NULL == TypeOfScan)
	{
		return AddressHolder;
	}

	std::vector<UINT_PTR> ModuleScan;
	ModuleScan.clear();

	std::vector<UINT_PTR> MemoryScan;
	MemoryScan.clear();

	ModuleScan = HYPERSCAN_SCANNER::ScanModules(ProcessID, ScanValue, AllignmentOfScan, TypeOfScan);
	MemoryScan = HYPERSCAN_SCANNER::ScanWholeMemoryWithDelimiters(ProcessID, ScanValue, AllignmentOfScan, TypeOfScan);

	AddressHolder.insert(AddressHolder.end(), ModuleScan.begin(), ModuleScan.end());
	AddressHolder.insert(AddressHolder.end(), MemoryScan.begin(), MemoryScan.end());

	return AddressHolder;
}