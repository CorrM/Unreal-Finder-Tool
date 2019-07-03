#pragma once
#include <Windows.h>
#include <string>

#ifdef UNICODE
#define CustomSetPrivilege  CustomSetPrivilegeW
#else
#define CustomSetPrivilege  CustomSetPrivilegeA
#endif

std::wstring Str2Wstr(std::string  in);
std::string  Wstr2Str(std::wstring in);

bool CustomSetPrivilegeW(LPCWSTR lpszPrivilege, BOOL bEnablePrivilege);

bool CustomSetPrivilegeA(LPCSTR lpszPrivilege, BOOL bEnablePrivilege);

bool WriteDataToFile(const UCHAR pBuffer[], size_t dwSize, const std::string& strFileName, DWORD dwCreationDisposition = CREATE_NEW);