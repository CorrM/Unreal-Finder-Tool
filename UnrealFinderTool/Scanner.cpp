#include "pch.h"
#include "Scanner.h"
#include <assert.h>
#include <Tlhelp32.h>
#include "ParallelWorker.h"

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

typedef NTSTATUS(NTAPI *hsNtQueryVirtualMemory)(
	HANDLE ProcessHandle,
	PVOID BaseAddress, MEMORY_INFORMATION_CLASS MemoryInformationClass,
	PVOID Buffer, SIZE_T Length, PSIZE_T ResultLength
	);

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

	if (FALSE == IsHandleValid(ProcessHandle))
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

std::vector<uintptr_t> HYPERSCAN_SCANNER::ScanMemory(DWORD ProcessID, uintptr_t ModuleBaseAddress, uintptr_t ModuleSize, uintptr_t ScanValue,
	ScanAllignment AllignmentOfScan, ScanType TypeOfScan)
{
	std::vector<uintptr_t> AddressHolder;
	AddressHolder.clear();

	if (NULL == ProcessID || NULL == ModuleBaseAddress || NULL == ModuleSize || NULL == AllignmentOfScan
		|| NULL == TypeOfScan)
	{
		return AddressHolder;
	}

	_MEMORY_BASIC_INFORMATION BasicInformation;
	uintptr_t AddressForScan = ModuleBaseAddress;
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

	std::vector<std::pair<uintptr_t, _MEMORY_BASIC_INFORMATION>> mem_blocks;

	while (VirtualQueryEx(QueryHandle, reinterpret_cast<void*>(AddressForScan), &BasicInformation, sizeof(BasicInformation))
		&& AddressForScan < (ModuleBaseAddress + ModuleSize))
	{
		if (BasicInformation.State & MEM_COMMIT)
			mem_blocks.emplace_back(AddressForScan, BasicInformation);
		AddressForScan = reinterpret_cast<uintptr_t>(BasicInformation.BaseAddress) + BasicInformation.RegionSize;
	}

	for (std::pair<uintptr_t, _MEMORY_BASIC_INFORMATION> block : mem_blocks)
	{
		auto memory_block = new UCHAR[block.second.RegionSize];
		if (ReadProcessMemory(QueryHandle, reinterpret_cast<void*>(block.first), memory_block, block.second.RegionSize,
			nullptr))
		{
			for (unsigned int scanIndex = 0; scanIndex != block.second.RegionSize / AllignmentOfScan; ++scanIndex)
			{
				if (HyperscanTypeAll == TypeOfScan)
				{
					AddressHolder.push_back(block.first + scanIndex * AllignmentOfScan);
				}
				else if (HyperscanTypeExact == TypeOfScan)
				{
					if (*reinterpret_cast<uintptr_t*>(memory_block + scanIndex * AllignmentOfScan) == ScanValue)
					{
						AddressHolder.push_back(block.first + scanIndex * AllignmentOfScan);
					}
				}
				else if (HyperscanTypeSmaller == TypeOfScan)
				{
					if (*reinterpret_cast<uintptr_t*>(memory_block + scanIndex * AllignmentOfScan) < ScanValue)
					{
						AddressHolder.push_back(block.first + scanIndex * AllignmentOfScan);
					}
				}
				else if (HyperscanTypeBigger == TypeOfScan)
				{
					if (*reinterpret_cast<uintptr_t*>(memory_block + scanIndex * AllignmentOfScan) > ScanValue)
					{
						AddressHolder.push_back(block.first + scanIndex * AllignmentOfScan);
					}
				}
				else if (HyperscanTypeDifferent == TypeOfScan)
				{
					if (*reinterpret_cast<uintptr_t*>(memory_block + scanIndex * AllignmentOfScan) != ScanValue)
					{
						AddressHolder.push_back(block.first + scanIndex * AllignmentOfScan);
					}
				}
				else
				{
					if (*reinterpret_cast<uintptr_t*>(memory_block + scanIndex * AllignmentOfScan) == ScanValue)
					{
						AddressHolder.push_back(block.first + scanIndex * AllignmentOfScan);
					}
				}
			}
		}
		delete[] memory_block;
	}
	CloseHandle(QueryHandle);
	return AddressHolder;
}

std::vector<uintptr_t> HYPERSCAN_SCANNER::ScanModules(DWORD ProcessID, uintptr_t ScanValue, ScanAllignment AllignmentOfScan,
	ScanType TypeOfScan)
{
	std::vector<uintptr_t> AddressHolder;
	AddressHolder.clear();

	if (NULL == ProcessID || NULL == AllignmentOfScan || NULL == TypeOfScan)
		return AddressHolder;

	HANDLE ModuleSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE | TH32CS_SNAPMODULE32, ProcessID);

	if (INVALID_HANDLE_VALUE == ModuleSnapshot)
		return AddressHolder;

	MODULEENTRY32 ModuleEntry;
	ModuleEntry.dwSize = sizeof(MODULEENTRY32);

	if (Module32First(ModuleSnapshot, &ModuleEntry))
	{
		do
		{
			std::vector<uintptr_t> TemporaryAddressHolder;
			TemporaryAddressHolder.clear();

			TemporaryAddressHolder = ScanMemory(ProcessID, reinterpret_cast<uintptr_t>(ModuleEntry.modBaseAddr), ModuleEntry.modBaseSize,
				ScanValue, AllignmentOfScan, TypeOfScan);

			AddressHolder.insert(AddressHolder.end(), TemporaryAddressHolder.begin(), TemporaryAddressHolder.end());
		} while (Module32Next(ModuleSnapshot, &ModuleEntry));
	}

	CloseHandle(ModuleSnapshot);
	return AddressHolder;
}

std::vector<uintptr_t> HYPERSCAN_SCANNER::ScanWholeMemoryWithDelimiters(DWORD ProcessID, uintptr_t ScanValue, ScanAllignment AllignmentOfScan,
	ScanType TypeOfScan, uintptr_t BeginAddress, uintptr_t EndAddress)
{
	std::vector<uintptr_t> AddressHolder;
	AddressHolder.clear();

	if (NULL == ProcessID || NULL == EndAddress || NULL == AllignmentOfScan || NULL == TypeOfScan)
		return AddressHolder;

	_MEMORY_BASIC_INFORMATION BasicInformation;
	uintptr_t AddressForScan = BeginAddress;
	HANDLE QueryHandle = OpenProcess(PROCESS_ALL_ACCESS, FALSE, ProcessID);

#ifndef _WIN64
	if (TRUE == HYPERSCAN_CHECK::IsProcess64Bit(QueryHandle))
	{
		assert("Incompatibility in architectures!");
	}
#endif

	if (INVALID_HANDLE_VALUE == QueryHandle)
		return AddressHolder;

	std::vector<std::pair<uintptr_t, _MEMORY_BASIC_INFORMATION>> mem_blocks;
	while (VirtualQueryEx(QueryHandle, reinterpret_cast<VOID*>(AddressForScan), &BasicInformation, sizeof(BasicInformation))
		&& AddressForScan < EndAddress)
	{
		if ((BasicInformation.State & MEM_COMMIT))
		{
			mem_blocks.emplace_back(AddressForScan, BasicInformation);
		}
		AddressForScan = reinterpret_cast<uintptr_t>(BasicInformation.BaseAddress) + BasicInformation.RegionSize;
	}

	ParallelQueue<std::vector<std::pair<uintptr_t, _MEMORY_BASIC_INFORMATION>>, std::pair<uintptr_t, _MEMORY_BASIC_INFORMATION>>
	worker(mem_blocks, 0, Utils::Settings.SdkGen.Threads, [&](const std::pair<uintptr_t, _MEMORY_BASIC_INFORMATION>& block, ParallelOptions& options)
	{
		auto MemoryBlock = new UCHAR[block.second.RegionSize];
		if (ReadProcessMemory(QueryHandle, reinterpret_cast<void*>(block.first), MemoryBlock, block.second.RegionSize,
			nullptr))
		{
			for (unsigned int scanIndex = 0; scanIndex != block.second.RegionSize / AllignmentOfScan; ++scanIndex)
			{
				if (HyperscanTypeAll == TypeOfScan)
				{
					std::lock_guard lock(options.Locker);
					AddressHolder.push_back(block.first + scanIndex * AllignmentOfScan);
				}
				else if (HyperscanTypeExact == TypeOfScan)
				{
					if (*reinterpret_cast<uintptr_t*>(MemoryBlock + scanIndex * AllignmentOfScan) == ScanValue)
					{
						std::lock_guard lock(options.Locker);
						AddressHolder.push_back(block.first + scanIndex * AllignmentOfScan);
					}
				}
				else if (HyperscanTypeSmaller == TypeOfScan)
				{
					if (*reinterpret_cast<uintptr_t*>(MemoryBlock + scanIndex * AllignmentOfScan) < ScanValue)
					{
						std::lock_guard lock(options.Locker);
						AddressHolder.push_back(block.first + scanIndex * AllignmentOfScan);
					}
				}
				else if (HyperscanTypeBigger == TypeOfScan)
				{
					if (*reinterpret_cast<uintptr_t*>(MemoryBlock + scanIndex * AllignmentOfScan) > ScanValue)
					{
						std::lock_guard lock(options.Locker);
						AddressHolder.push_back(block.first + scanIndex * AllignmentOfScan);
					}
				}
				else if (HyperscanTypeDifferent == TypeOfScan)
				{
					if (*reinterpret_cast<uintptr_t*>(MemoryBlock + scanIndex * AllignmentOfScan) != ScanValue)
					{
						std::lock_guard lock(options.Locker);
						AddressHolder.push_back(block.first + scanIndex * AllignmentOfScan);
					}
				}
				else
				{
					if (*reinterpret_cast<uintptr_t*>(MemoryBlock + scanIndex * AllignmentOfScan) == ScanValue)
					{
						std::lock_guard lock(options.Locker);
						AddressHolder.push_back(block.first + scanIndex * AllignmentOfScan);
					}
				}
			}
		}
		delete[] MemoryBlock;
	});
	worker.Start();
	worker.WaitAll();

	CloseHandle(QueryHandle);
	return AddressHolder;
}

std::vector<uintptr_t> HYPERSCAN_SCANNER::Scan(DWORD ProcessID, uintptr_t ScanValue, ScanAllignment allignmentOfScan, ScanType TypeOfScan)
{
	std::vector<uintptr_t> AddressHolder;
	AddressHolder.clear();

	if (NULL == ProcessID || NULL == allignmentOfScan || NULL == TypeOfScan)
	{
		return AddressHolder;
	}

	std::vector<uintptr_t> ModuleScan;
	ModuleScan.clear();

	std::vector<uintptr_t> MemoryScan;
	MemoryScan.clear();

	ModuleScan = ScanModules(ProcessID, ScanValue, allignmentOfScan, TypeOfScan);
	MemoryScan = ScanWholeMemoryWithDelimiters(ProcessID, ScanValue, allignmentOfScan, TypeOfScan);

	AddressHolder.insert(AddressHolder.end(), ModuleScan.begin(), ModuleScan.end());
	AddressHolder.insert(AddressHolder.end(), MemoryScan.begin(), MemoryScan.end());

	return AddressHolder;
}