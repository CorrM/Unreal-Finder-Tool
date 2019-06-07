#pragma once
#include <Windows.h>
#include <string>

#ifdef UNICODE
#define CustomSetPrivilege  CustomSetPrivilegeW
#else
#define CustomSetPrivilege  CustomSetPrivilegeA
#endif

std::wstring str2wstr(std::string  in);
std::string  wstr2str(std::wstring in);

bool CustomSetPrivilegeW(LPCWSTR lpszPrivilege, BOOL bEnablePrivilege);

bool CustomSetPrivilegeA(LPCSTR lpszPrivilege, BOOL bEnablePrivilege);

bool WriteDataToFile(const UCHAR pBuffer[], size_t dwSize, const std::string& strFileName, DWORD dwCreationDisposition = CREATE_NEW);