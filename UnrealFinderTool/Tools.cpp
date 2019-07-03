#include "pch.h"
#include <Windows.h>
#include "Tools.h"

std::wstring Str2Wstr(std::string  in)
{
	std::wstring out(in.begin(), in.end());
	return out;
}
std::string  Wstr2Str(std::wstring in)
{
	std::string out(in.begin(), in.end());
	return out;
}

bool CustomSetPrivilegeW(const LPCWSTR lpszPrivilege, const BOOL bEnablePrivilege) {
	TOKEN_PRIVILEGES priv = { 0,0,0,0 };
	HANDLE hToken = nullptr;
	LUID luid = { 0,0 };
	if (!OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES, &hToken)) {
		if (hToken)
			CloseHandle(hToken);
		return false;
	}
	if (!LookupPrivilegeValueW(nullptr, lpszPrivilege, &luid)) {
		if (hToken)
			CloseHandle(hToken);
		return false;
	}
	priv.PrivilegeCount = 1;
	priv.Privileges[0].Luid = luid;
	priv.Privileges[0].Attributes = bEnablePrivilege ? SE_PRIVILEGE_ENABLED : SE_PRIVILEGE_REMOVED;
	if (!AdjustTokenPrivileges(hToken, false, &priv, 0, nullptr, nullptr)) {
		if (hToken)
			CloseHandle(hToken);
		return false;
	}
	if (hToken)
		CloseHandle(hToken);
	return true;
}

bool CustomSetPrivilegeA(const LPCSTR lpszPrivilege, const BOOL bEnablePrivilege) {
	TOKEN_PRIVILEGES priv = { 0,0,0,0 };
	HANDLE hToken = nullptr;
	LUID luid = { 0, 0 };
	if (!OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES, &hToken)) {
		if (hToken)
			CloseHandle(hToken);
		return false;
	}
	if (!LookupPrivilegeValueA(nullptr, lpszPrivilege, &luid)) {
		if (hToken)
			CloseHandle(hToken);
		return false;
	}
	priv.PrivilegeCount = 1;
	priv.Privileges[0].Luid = luid;
	priv.Privileges[0].Attributes = bEnablePrivilege ? SE_PRIVILEGE_ENABLED : SE_PRIVILEGE_REMOVED;
	if (!AdjustTokenPrivileges(hToken, false, &priv, 0, nullptr, nullptr)) {
		if (hToken)
			CloseHandle(hToken);
		return false;
	}
	if (hToken)
		CloseHandle(hToken);
	return true;
}

bool WriteDataToFile(const UCHAR pBuffer[], const size_t dwSize, const std::string& strFileName, const DWORD dwCreationDisposition)
{
	const auto hFile = CreateFileA(strFileName.c_str(), GENERIC_READ | GENERIC_WRITE, 0, nullptr, dwCreationDisposition, FILE_ATTRIBUTE_NORMAL, nullptr);
	if (hFile == INVALID_HANDLE_VALUE) return false;
	DWORD dwNumberOfBytesWritten = NULL;
	const auto bWriteFile = WriteFile(hFile, pBuffer, static_cast<DWORD>(dwSize), &dwNumberOfBytesWritten, nullptr);
	CloseHandle(hFile);
	return !(!bWriteFile || dwNumberOfBytesWritten != dwSize);
}