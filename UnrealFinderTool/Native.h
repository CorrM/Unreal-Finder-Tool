#pragma once
#include <Windows.h>

template<typename ArrayType>
struct StructArray
{
	ArrayType* Ptr;
	size_t Count;
	size_t ItemSize;
};

struct StringArray
{
	TCHAR** Ptr;
	size_t Count;
};

struct NativeJsonVar
{
	TCHAR* Name;
	TCHAR* Type;
	size_t Size, Offset;
};

struct NativeJsonStruct
{
	TCHAR* StructName;
	TCHAR* StructSuper;
	StructArray<NativeJsonVar> Members;
};

struct NativePredefinedMethod
{
	enum class Type
	{
		Default,
		Inline
	};

	TCHAR* Signature;
	TCHAR* Body;
	Type MethodType;
};

struct NativeConstant
{
	TCHAR* Name;
	TCHAR* Value;
};

struct NativeEnum
{
	TCHAR* Name;
	TCHAR* FullName;
	StringArray Values;
};

struct NativeMember
{
	TCHAR* Name;
	TCHAR* Type;
	bool IsStatic;

	size_t Offset;
	size_t Size;

	size_t Flags;
	TCHAR* FlagsString;

	TCHAR* Comment;
};

struct NativeScriptStruct
{
	TCHAR* Name;
	TCHAR* Type;
	TCHAR* FullName;
	TCHAR* NameCpp;
	TCHAR* NameCppFull;

	size_t Size;
	size_t InheritedSize;

	StructArray<NativeMember> Members;
	StructArray<NativePredefinedMethod> PredefinedMethods;
};

struct NativeMethod
{
	struct Parameter
	{
		enum class Type
		{
			Default,
			Out,
			Return
		};

		Type ParamType;
		bool PassByReference;
		TCHAR* CppType;
		TCHAR* Name;
		TCHAR* FlagsString;
	};

	size_t Index;
	TCHAR* Name;
	TCHAR* FullName;
	StructArray<Parameter> Parameters;
	TCHAR* FlagsString;
	bool IsNative;
	bool IsStatic;
};

struct NativeClass : NativeScriptStruct
{
	StringArray VirtualFunctions;
	StructArray<NativeMethod> Methods;
};

struct NativePackage
{
	TCHAR* Name;
	StructArray<NativeConstant> Constants;
	StructArray<NativeClass> Classes;
	StructArray<NativeScriptStruct> ScriptStructs;
	StructArray<NativeEnum> Enums;
};
