#pragma once
#include <Windows.h>
#include <string>
#include <iostream>
#include <winternl.h>
#include <ntstatus.h>
#include "Tools.h"

#pragma comment (lib, "ntdll.lib")

class Driver
{
public:
	Driver(const std::string& strDriverFile,
		const std::string& strDeviceName,
		const std::string& strDriverName,
		const unsigned char* pRawDriver = nullptr,
		const size_t szRawDriver = 0);

	~Driver();

	HANDLE GetHandle() const { return m_hDriver; }
	bool removeAllOnExit = false;

private:
	bool Connect();
	bool Load();
	bool Unload();

	std::string m_strDriverFile = "";
	std::string m_strDeviceName = "";
	std::string m_strDriverName = "";

	const unsigned char* m_pRawDriver = nullptr;
	size_t m_szRawDriver = 0;

	bool m_wasRunning = false;
	bool m_wasInstalled = false;
	bool m_wasDisabled = false;
	bool m_fileWasCreated = false;

	HANDLE m_hDriver = nullptr;
};

