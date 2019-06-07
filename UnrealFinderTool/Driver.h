#pragma once
#include <string>
#include <iostream>
#include <winternl.h>

#pragma comment (lib, "ntdll.lib")

class Driver
{
public:
	Driver(const std::string& strDriverFile,
		const std::string& strDeviceName,
		const std::string& strDriverName,
		const unsigned char* pRawDriver = nullptr,
		size_t szRawDriver = 0);

	~Driver();

	HANDLE GetHandle() const { return mHDriver; }
	bool RemoveAllOnExit = false;

private:
	bool Connect();
	bool Load();
	bool Unload();

	std::string mStrDriverFile = "";
	std::string mStrDeviceName = "";
	std::string mStrDriverName = "";

	const unsigned char* mPRawDriver = nullptr;
	size_t mSzRawDriver = 0;

	bool mWasRunning = false;
	bool mWasInstalled = false;
	bool mWasDisabled = false;
	bool mFileWasCreated = false;

	HANDLE mHDriver = nullptr;
};

