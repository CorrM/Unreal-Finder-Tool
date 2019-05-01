#include "pch.h"
#include "Driver.h"

Driver::Driver(const std::string& strDriverFile,
	const std::string& strDeviceName,
	const std::string& strDriverName,
	const unsigned char* pRawDriver,
	const size_t szRawDriver)
{
	// Using either absolute path or getting relative path
	m_strDriverFile = strDriverFile;
	if (strDriverFile.find_first_of(':') == std::string::npos)
	{
		char currentPath[MAX_PATH];
		GetCurrentDirectoryA(MAX_PATH, currentPath);
		std::string strCurrentPath = currentPath;
		strCurrentPath += "\\";
		m_strDriverFile = strCurrentPath + strDriverFile;
	}

	m_strDeviceName = strDeviceName;
	m_strDriverName = strDriverName;
	m_pRawDriver = pRawDriver;
	m_szRawDriver = szRawDriver;

	Load();
}

Driver::~Driver() { Unload(); }

bool Driver::Connect()
{
	// Attempts to connect (get a handle) to the driver, first with the classic way
	const auto hDevice = CreateFileA(m_strDeviceName.c_str(), GENERIC_READ | GENERIC_WRITE, 0, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
	if (hDevice != INVALID_HANDLE_VALUE)
	{
		m_hDriver = hDevice;
		return true;
	}

	// If that failed, trying to get a handle directly with NtOpenFile like Process Hacker does (see their source, in kph.c)
	// Thank to sen66 @ UnknownCheats.me for this code snippet
	UNICODE_STRING objectName;
	OBJECT_ATTRIBUTES objectAttributes;
	IO_STATUS_BLOCK isb;
	HANDLE hNtDriver;
	const auto strObjectName = str2wstr(m_strDeviceName);

	RtlInitUnicodeString(&objectName, strObjectName.c_str());
	InitializeObjectAttributes(&objectAttributes, &objectName, FILE_NON_DIRECTORY_FILE, NULL, NULL);
	const auto status = NtOpenFile(&hNtDriver, FILE_GENERIC_READ | FILE_GENERIC_WRITE, &objectAttributes, &isb, FILE_SHARE_READ | FILE_SHARE_WRITE, FILE_NON_DIRECTORY_FILE);
	if (status == STATUS_SUCCESS)
	{
		m_hDriver = hNtDriver;
		return true;
	}
	return false;
}

bool Driver::Load()
{
	// Try to get a handle on the driver (if already loaded)
	if (Connect())
	{
		m_wasRunning = true;
		return true;
	}
	m_wasRunning = false;

	// Getting ready to interact with the service
	const auto hScm = OpenSCManagerA(nullptr, nullptr, SC_MANAGER_CREATE_SERVICE);
	if (!hScm)
		return false;

	// Trying to get a handle on the service if it exist, otherwise we create it
	auto hSc = OpenServiceA(hScm, m_strDriverName.c_str(), SERVICE_START);
	if (!hSc)
	{
		// If driver file doesn't exist on disk and we have an embedded driver, we write it on disk
		if (WriteDataToFile(m_pRawDriver, m_szRawDriver, m_strDriverFile))
			m_fileWasCreated = true;
		else
			m_fileWasCreated = false;

		hSc = CreateServiceA(hScm, m_strDriverName.c_str(), "", SERVICE_START, SERVICE_KERNEL_DRIVER, SERVICE_DEMAND_START, SERVICE_ERROR_IGNORE, m_strDriverFile.c_str(), nullptr, nullptr, nullptr, nullptr, nullptr);
		if (!hSc)
		{
			// Service doesn't exist and can't create it, aborting
			CloseServiceHandle(hScm);
			return false;
		}
		m_wasInstalled = false;
	}
	else
		m_wasInstalled = true; // The service was already installed

	// Starting service
	auto bStartService = StartServiceA(hSc, NULL, nullptr);
	if (!bStartService && GetLastError() == ERROR_SERVICE_DISABLED)
	{
		// Couldn't start service because it is disabled, upgrading handle to modify servcie configuration and starting it
		CloseServiceHandle(hSc);
		hSc = OpenServiceA(hScm, m_strDriverName.c_str(), SERVICE_START | SERVICE_CHANGE_CONFIG);
		const auto bChangeServiceConfig = ChangeServiceConfigA(hSc, SERVICE_NO_CHANGE, SERVICE_DEMAND_START, SERVICE_NO_CHANGE, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr);
		std::cout << "GLE = " << std::dec << GetLastError() << std::endl;
		if (bChangeServiceConfig)
		{
			bStartService = StartServiceA(hSc, NULL, nullptr);
			if (bStartService)
				m_wasDisabled = true;
			else
				m_wasDisabled = false;
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
	CloseHandle(m_hDriver);
	m_hDriver = nullptr;

	// If we want to forcibly stop the driver's service and uninstall it, we ignore if it was running and installed
	if (removeAllOnExit)
	{
		m_wasRunning = false;
		m_wasInstalled = false;
	}

	// If the service was running we just leave it be just as it was
	if (m_wasRunning && !removeAllOnExit)
		return true;

	// Getting ready to interact with the service to stop, disable, and/or delete it
	SERVICE_STATUS scStatus;
	const auto hScm = OpenSCManagerA(nullptr, nullptr, SC_MANAGER_CONNECT);
	if (!hScm)
		return false;
	DWORD dwServicePermission = SERVICE_STOP;
	if (!m_wasInstalled)
		dwServicePermission |= DELETE;
	if (m_wasDisabled)
		dwServicePermission |= SERVICE_CHANGE_CONFIG;
	const auto hSc = OpenServiceA(hScm, m_strDriverName.c_str(), dwServicePermission);
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
	if (m_wasDisabled)
		if (!ChangeServiceConfigA(hSc, SERVICE_NO_CHANGE, SERVICE_DISABLED, SERVICE_NO_CHANGE, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr))
		{
			CloseServiceHandle(hSc);
			CloseServiceHandle(hScm);
			return false;
		}

	// If the service was installed we stop here
	if (m_wasInstalled && !removeAllOnExit)
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
	if (m_fileWasCreated && removeAllOnExit)
		return DeleteFileA(m_strDriverFile.c_str());

	return bDeleteService;
}