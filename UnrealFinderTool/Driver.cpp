#include "pch.h"
#include <Windows.h>

#include "Tools.h"
#include "Driver.h"

Driver::Driver(const std::string& strDriverFile,
	const std::string& strDeviceName,
	const std::string& strDriverName,
	const unsigned char* pRawDriver,
	const size_t szRawDriver)
{
	// Using either absolute path or getting relative path
	mStrDriverFile = strDriverFile;
	if (strDriverFile.find_first_of(':') == std::string::npos)
	{
		char currentPath[MAX_PATH];
		GetCurrentDirectoryA(MAX_PATH, currentPath);
		std::string strCurrentPath = currentPath;
		strCurrentPath += "\\";
		mStrDriverFile = strCurrentPath + strDriverFile;
	}

	mStrDeviceName = strDeviceName;
	mStrDriverName = strDriverName;
	mPRawDriver = pRawDriver;
	mSzRawDriver = szRawDriver;

	Load();
}

Driver::~Driver() { Unload(); }

bool Driver::Connect()
{
	// Attempts to connect (get a handle) to the driver, first with the classic way
	const auto hDevice = CreateFileA(mStrDeviceName.c_str(), GENERIC_READ | GENERIC_WRITE, 0, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
	if (hDevice != INVALID_HANDLE_VALUE)
	{
		mHDriver = hDevice;
		return true;
	}

	// If that failed, trying to get a handle directly with NtOpenFile like Process Hacker does (see their source, in kph.c)
	// Thank to sen66 @ UnknownCheats.me for this code snippet
	UNICODE_STRING objectName;
	OBJECT_ATTRIBUTES objectAttributes;
	IO_STATUS_BLOCK isb;
	HANDLE hNtDriver;
	const auto strObjectName = Str2Wstr(mStrDeviceName);

	RtlInitUnicodeString(&objectName, strObjectName.c_str());
	InitializeObjectAttributes(&objectAttributes, &objectName, FILE_NON_DIRECTORY_FILE, NULL, NULL);
	const auto status = NtOpenFile(&hNtDriver, FILE_GENERIC_READ | FILE_GENERIC_WRITE, &objectAttributes, &isb, FILE_SHARE_READ | FILE_SHARE_WRITE, FILE_NON_DIRECTORY_FILE);
	if (NT_SUCCESS(status))
	{
		mHDriver = hNtDriver;
		return true;
	}
	return false;
}

bool Driver::Load()
{
	// Try to get a handle on the driver (if already loaded)
	if (Connect())
	{
		mWasRunning = true;
		return true;
	}
	mWasRunning = false;

	// Getting ready to interact with the service
	const auto hScm = OpenSCManagerA(nullptr, nullptr, SC_MANAGER_CREATE_SERVICE);
	if (!hScm)
		return false;

	// Trying to get a handle on the service if it exist, otherwise we create it
	auto hSc = OpenServiceA(hScm, mStrDriverName.c_str(), SERVICE_START);
	if (!hSc)
	{
		// If driver file doesn't exist on disk and we have an embedded driver, we write it on disk
		if (WriteDataToFile(mPRawDriver, mSzRawDriver, mStrDriverFile))
			mFileWasCreated = true;
		else
			mFileWasCreated = false;

		hSc = CreateServiceA(hScm, mStrDriverName.c_str(), "", SERVICE_START, SERVICE_KERNEL_DRIVER, SERVICE_DEMAND_START, SERVICE_ERROR_IGNORE, mStrDriverFile.c_str(), nullptr, nullptr, nullptr, nullptr, nullptr);
		if (!hSc)
		{
			// Service doesn't exist and can't create it, aborting
			CloseServiceHandle(hScm);
			return false;
		}
		mWasInstalled = false;
	}
	else
		mWasInstalled = true; // The service was already installed

	// Starting service
	auto bStartService = StartServiceA(hSc, NULL, nullptr);
	if (!bStartService && GetLastError() == ERROR_SERVICE_DISABLED)
	{
		// Couldn't start service because it is disabled, upgrading handle to modify servcie configuration and starting it
		CloseServiceHandle(hSc);
		hSc = OpenServiceA(hScm, mStrDriverName.c_str(), SERVICE_START | SERVICE_CHANGE_CONFIG);
		const auto bChangeServiceConfig = ChangeServiceConfigA(hSc, SERVICE_NO_CHANGE, SERVICE_DEMAND_START, SERVICE_NO_CHANGE, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr);
		std::cout << "GLE = " << std::dec << GetLastError() << std::endl;
		if (bChangeServiceConfig)
		{
			bStartService = StartServiceA(hSc, NULL, nullptr);
			if (bStartService)
				mWasDisabled = true;
			else
				mWasDisabled = false;
		}
	}
	CloseServiceHandle(hSc);
	CloseServiceHandle(hScm);

	// Getting handle
	return Connect();
}

bool Driver::Unload()
{
	// Closing handle
	CloseHandle(mHDriver);
	mHDriver = nullptr;

	// If we want to forcibly stop the driver's service and uninstall it, we ignore if it was running and installed
	if (RemoveAllOnExit)
	{
		mWasRunning = false;
		mWasInstalled = false;
	}

	// If the service was running we just leave it be just as it was
	if (mWasRunning && !RemoveAllOnExit)
		return true;

	// Getting ready to interact with the service to stop, disable, and/or delete it
	SERVICE_STATUS scStatus;
	const auto hScm = OpenSCManagerA(nullptr, nullptr, SC_MANAGER_CONNECT);
	if (!hScm)
		return false;
	DWORD dwServicePermission = SERVICE_STOP;
	if (!mWasInstalled)
		dwServicePermission |= DELETE;
	if (mWasDisabled)
		dwServicePermission |= SERVICE_CHANGE_CONFIG;
	const auto hSc = OpenServiceA(hScm, mStrDriverName.c_str(), dwServicePermission);
	if (!hSc)
	{
		CloseServiceHandle(hScm);
		return false;
	}

	// If the service was not running we stop it
	if (!ControlService(hSc, SERVICE_CONTROL_STOP, &scStatus))
	{
		CloseServiceHandle(hSc);
		CloseServiceHandle(hScm);
		return false;
	}

	// If the service was disabled we re-disable it
	if (mWasDisabled)
		if (!ChangeServiceConfigA(hSc, SERVICE_NO_CHANGE, SERVICE_DISABLED, SERVICE_NO_CHANGE, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr))
		{
			CloseServiceHandle(hSc);
			CloseServiceHandle(hScm);
			return false;
		}

	// If the service was installed we stop here
	if (mWasInstalled && !RemoveAllOnExit)
	{
		CloseServiceHandle(hSc);
		CloseServiceHandle(hScm);
		return true;
	}

	// If the service was not installed, we uninstall it
	const auto bDeleteService = DeleteService(hSc);
	CloseServiceHandle(hSc);
	CloseServiceHandle(hScm);

	// If we should delete the file we delete it
	if (mFileWasCreated && RemoveAllOnExit)
		return DeleteFileA(mStrDriverFile.c_str());

	return bDeleteService;
}