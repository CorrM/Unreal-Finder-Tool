#pragma once
#include <iostream>
#include <windows.h>
#include <vector>

#ifndef _WIN64
#undef INT
#define INT int32_t
#else
#undef INT
#define INT int64_t
#endif

namespace Hyperscan
{
	enum ScanAllignment : DWORD
	{
		HyperscanAllignmentByte = 0x1,
		HyperscanAllignment2Bytes = 0x2,
		HyperscanAllignment4Bytes = 0x4,
		HyperscanAllignment8Bytes = 0x8
	};

	enum ScanType : DWORD
	{
		HyperscanTypeExact = 0x00E,
		HyperscanTypeSmaller = 0x0E,
		HyperscanTypeBigger = 0x000E,
		HyperscanTypeDifferent = 0x0000A,
		HyperscanTypeAll = 0xABCDEF
	};

	enum ScanMode : DWORD
	{
		HyperscanScanFirst = 0xFF0,
		HyperscanScanNext = 0x0FF
	};

	typedef class HYPERSCAN_CHECK
	{
	public: static BOOL IsHandleValid(HANDLE ProcessHandle);

	public: static BOOL IsProcess64Bit(HANDLE ProcessHandle);

	public: static BOOL IsAddressStatic(DWORD ProcessID, uintptr_t Address);
	} *PHYPERSCAN_CHECK;

	typedef class HYPERSCAN_SCANNER
	{
	private: static std::vector<UINT_PTR> ScanMemory(DWORD ProcessID, UINT_PTR ModuleBaseAddress, UINT_PTR ModuleSize, uintptr_t ScanValue,
		ScanAllignment AllignmentOfScan, ScanType TypeOfScan);

	private: static std::vector<UINT_PTR> ScanModules(DWORD ProcessID, uintptr_t ScanValue, ScanAllignment AllignmentOfScan,
		ScanType TypeOfScan);

	private: static std::vector<UINT_PTR> ScanWholeMemoryWithDelimiters(DWORD ProcessID, uintptr_t ScanValue, ScanAllignment AllignmentOfScan,
		ScanType TypeOfScan, uintptr_t BeginAddress = 0x000000000, uintptr_t EndAddress = 0x7fffffffffff);

	public: static std::vector<UINT_PTR> Scan(DWORD ProcessID, uintptr_t ScanValue, ScanAllignment allignmentOfScan, ScanType TypeOfScan);
	} *PHYPERSCAN_SCANNER;
}
