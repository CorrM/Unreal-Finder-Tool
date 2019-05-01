#pragma once
#include <Windows.h>
#include <string>

#ifdef UNICODE
#define SetPrivilege  SetPrivilegeW
#else
#define SetPrivilege  SetPrivilegeA
#endif

std::wstring str2wstr(std::string  in);
std::string  wstr2str(std::wstring in);

bool SetPrivilegeW(const LPCWSTR lpszPrivilege, const BOOL bEnablePrivilege);

bool SetPrivilegeA(const LPCSTR lpszPrivilege, const BOOL bEnablePrivilege);

bool WriteDataToFile(const UCHAR pBuffer[], const DWORD dwSize, const std::string& strFileName, const DWORD dwCreationDisposition = CREATE_NEW);