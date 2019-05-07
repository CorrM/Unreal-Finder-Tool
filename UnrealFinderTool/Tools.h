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

bool SetPrivilegeW(LPCWSTR lpszPrivilege, BOOL bEnablePrivilege);

bool SetPrivilegeA(LPCSTR lpszPrivilege, BOOL bEnablePrivilege);

bool WriteDataToFile(const UCHAR pBuffer[], DWORD dwSize, const std::string& strFileName, DWORD dwCreationDisposition = CREATE_NEW);