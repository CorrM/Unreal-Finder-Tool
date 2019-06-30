#pragma once
#include <Windows.h>

struct NativeJsonVar
{
	TCHAR* Name;
	TCHAR* Type;
	int Size, Offset;
};

struct NativeJsonStruct
{
	TCHAR* StructName;
	TCHAR* StructSuper;
	NativeJsonVar* Members;
	int MemberSize;
	int MembersCount;
};
