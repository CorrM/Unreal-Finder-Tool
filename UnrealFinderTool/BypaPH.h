#pragma once
#include "Driver.h"
#include "KProcessHacker.h"
#include <string>

#define KPH_DEVICE_TYPE 0x9999
#define KPH_CTL_CODE(x) CTL_CODE(KPH_DEVICE_TYPE, 0x800 + x, METHOD_NEITHER, FILE_ANY_ACCESS)
#define KPH_READVIRTUALMEMORYUNSAFE KPH_CTL_CODE(58)
#define KPH_READVIRTUALMEMORY KPH_CTL_CODE(56)
#define KPH_WRITEVIRTUALMEMORY KPH_CTL_CODE(57)

#define PHACKER_DRIVER_FILE  "kprocesshacker.sys"
#define PHACKER_SERVICE_NAME "KProcessHacker2"
#define PHACKER_DEVICE_NAME  "\\Device\\KProcessHacker2"

class BypaPH
{
public:
	HANDLE m_hTarget = nullptr;
	DWORD pID = 0;

	BypaPH(DWORD dwTargetPid = NULL)
	{
		pID = dwTargetPid;
		SetPrivilege(SE_DEBUG_NAME, TRUE);
		m_drv = new Driver(PHACKER_DRIVER_FILE, PHACKER_DEVICE_NAME, PHACKER_SERVICE_NAME, KProcessHacker, KProcessHackerSize);
		if (dwTargetPid)
			Attach(dwTargetPid);
	}

	~BypaPH()
	{
		delete m_drv;
		if (m_hTarget)
			CloseHandle(m_hTarget);
		SetPrivilege(SE_DEBUG_NAME, FALSE);
	}

	NTSTATUS RWVM(HANDLE ProcessHandle, PVOID BaseAddress, PVOID Buffer, SIZE_T BufferSize, PSIZE_T NumberOfBytesReadOrWritten = nullptr, bool read = true, bool unsafe = false)
	{
		// If status == 0xC0000004 (STATUS_INFO_LENGTH_MISMATCH) you are most likely using the x64 driver from an x86 process.
		// ProcessHandle may be NULL if reading kernel memory >= 0x8*
		if (!m_drv->GetHandle())
			return STATUS_ABANDONED;

		struct
		{
			HANDLE ProcessHandle;
			PVOID BaseAddress;
			PVOID Buffer;
			SIZE_T BufferSize;
			PSIZE_T NumberOfBytesRead;
		} input = { ProcessHandle, BaseAddress, Buffer, BufferSize, NumberOfBytesReadOrWritten };

		IO_STATUS_BLOCK isb;

		ULONG ioctrlCode = 0;
		if (!read && unsafe) // There is no write unsafe
			return STATUS_ABANDONED;
		if (read && unsafe)
			ioctrlCode = KPH_READVIRTUALMEMORYUNSAFE;
		if (read && !unsafe)
			ioctrlCode = KPH_READVIRTUALMEMORY;
		if (!read && !unsafe)
			ioctrlCode = KPH_WRITEVIRTUALMEMORY;

		return NtDeviceIoControlFile(m_drv->GetHandle(), nullptr, nullptr, nullptr, &isb, ioctrlCode, &input, sizeof(input), nullptr, 0);
		// If status == 0xC0000004 (STATUS_INFO_LENGTH_MISMATCH) you are most likely using the x64 driver from an x86 process.
		// We could have it working by having the struct at the right size though.
	}

	template <class T> T RVMu(HANDLE ProcessHandle, PVOID BaseAddress, PSIZE_T NumberOfBytesRead = nullptr)
	{
		T response = {};
		RWVM(ProcessHandle, BaseAddress, &response, sizeof(T), NumberOfBytesRead, true, true);
		return response;
	}

	template <class T> T qRVMu(PVOID BaseAddress, PSIZE_T NumberOfBytesRead = nullptr)
	{
		if (!m_hTarget)
		{
			*NumberOfBytesRead = 0;
			return T{};
		}
		return RVMu<T>(m_hTarget, BaseAddress, NumberOfBytesRead);
	}

	template <class T> T RVM(HANDLE ProcessHandle, PVOID BaseAddress, PSIZE_T NumberOfBytesRead = nullptr)
	{
		T response = {};
		RWVM(ProcessHandle, BaseAddress, &response, sizeof(T), NumberOfBytesRead, true, false);
		return response;
	}

	template <class T> T qRVM(PVOID BaseAddress, PSIZE_T NumberOfBytesRead = nullptr)
	{
		if (!m_hTarget)
		{
			*NumberOfBytesRead = 0;
			return T{};
		}
		return RVM<T>(m_hTarget, BaseAddress, NumberOfBytesRead);
	}

	std::string qRVM_String(PVOID BaseAddress, SIZE_T ReadSize, PSIZE_T NumberOfBytesRead = nullptr)
	{
		if (!m_hTarget)
		{
			*NumberOfBytesRead = 0;
			return "";
		}

		char* buf = new char[ReadSize];
		ZeroMemory(buf, sizeof(buf));
		RWVM(m_hTarget, BaseAddress, buf, ReadSize, NumberOfBytesRead);
		std::string ret(buf);

		delete buf;
		return ret;
	}

	std::wstring qRVM_WString(PVOID BaseAddress, SIZE_T ReadSize, PSIZE_T NumberOfBytesRead = nullptr)
	{
		if (!m_hTarget)
		{
			*NumberOfBytesRead = 0;
			return L"";
		}

		wchar_t* buf = new wchar_t[ReadSize];
		ZeroMemory(buf, sizeof(buf));
		NTSTATUS gg = RWVM(m_hTarget, BaseAddress, buf, ReadSize, NumberOfBytesRead);
		std::wstring ret(buf);

		delete buf;
		return ret;
	}

	NTSTATUS WVM(HANDLE ProcessHandle, PVOID BaseAddress, PVOID Buffer, SIZE_T BufferSize, PSIZE_T NumberOfBytesWritten = nullptr)
	{
		return RWVM(ProcessHandle, BaseAddress, Buffer, BufferSize, NumberOfBytesWritten, false, false);
	}

	NTSTATUS qWVM(PVOID BaseAddress, PVOID Buffer, SIZE_T BufferSize, PSIZE_T NumberOfBytesWritten = nullptr)
	{
		if (!m_hTarget)
		{
			*NumberOfBytesWritten = 0;
			return STATUS_ABANDONED;
		}
		return WVM(m_hTarget, BaseAddress, Buffer, BufferSize, NumberOfBytesWritten);
	}

	bool Attach(DWORD dwPid)
	{
		if (m_hTarget)
			if (!CloseHandle(m_hTarget))
				return false;
		m_hTarget = OpenProcess(SYNCHRONIZE, FALSE, dwPid); // Read will work whatever the handle permission.
		return m_hTarget != nullptr;
	}

	bool Detach(DWORD dwPid)
	{
		if (m_hTarget)
			if (!CloseHandle(m_hTarget))
				return false;
		m_hTarget = nullptr;
		return true;
	}

private:
	Driver* m_drv = nullptr;
};
