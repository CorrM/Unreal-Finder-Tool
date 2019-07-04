#pragma once
#include <Windows.h>
#include "Package.h"
#include "GenericTypes.h"

// Use `BOOL` not `bool` for dotnet marshaling -_-
// https://stackoverflow.com/a/39251864/3351489

template<typename NativeArrayType>
struct StructArray
{
	NativeArrayType** Ptr = nullptr;
	size_t Count = 0;
private:
	size_t itemSize = 0;

public:
	StructArray() = default;
	StructArray(NativeArrayType** ptr, const size_t count)
	{
		Ptr = ptr;
		Count = count;
		itemSize = sizeof(NativeArrayType);
	}
};

struct StringArray
{
	TCHAR** Ptr = nullptr;
	size_t Count = 0;

	StringArray() = default;
	explicit StringArray(const std::vector<std::string>& baseStr)
	{
		Count = baseStr.size();
		if (Count == 0)
			return;

		Ptr = new TCHAR * [baseStr.size()];

		for (size_t i = 0; i < baseStr.size(); ++i)
		{
			Ptr[i] = new TCHAR[baseStr[i].length() + 1];
			strcpy_s(Ptr[i], baseStr[i].length() + 1, baseStr[i].c_str());
		}
	}
};

class NativeHelper
{
public:
	static TCHAR* CreateHeapStr(const std::string& baseStr)
	{
		if (baseStr.empty())
			return nullptr;

		const auto ptr = new TCHAR[baseStr.length() + 1];
		strcpy_s(ptr, baseStr.length() + 1, baseStr.c_str());

		return ptr;
	}

	template<typename TNativeType, typename TBaseType>
	static TNativeType** HeapSdkNativeArray(std::vector<TBaseType>& baseValue)
	{
		if (baseValue.empty())
			return nullptr;

		auto** ptr = new TNativeType*[baseValue.size()];
		for (size_t i = 0; i < baseValue.size(); ++i)
		{
			TBaseType& curVar = baseValue[i];
			ptr[i] = new TNativeType(curVar);
		}
		return ptr;
	}

	template<typename TNativeType, typename TBaseType>
	static void HeapNativeStructArray(std::vector<TBaseType> baseValue, StructArray<TNativeType>& out)
	{
		auto ptr = NativeHelper::HeapSdkNativeArray<TNativeType>(baseValue);
		out = StructArray<TNativeType>(ptr, baseValue.size());
	}

	template<typename TNativeType>
	static void FreeNativeStructArray(StructArray<TNativeType>& target)
	{
		for (size_t i = 0; i < target.Count; i++)
		{
			delete target.Ptr[i];
			target.Ptr[i] = nullptr;
		}
		delete[] target.Ptr;
		target.Ptr = nullptr;
	}

	static void FreeHeapStr(const TCHAR* ptr)
	{
		delete[] ptr;
		ptr = nullptr;
	}

	static void FreeHeapArrayStr(StringArray& strArray)
	{
		if (strArray.Count == 0)
			return;

		for (size_t i = 0; i < strArray.Count; i++)
		{
			delete[] strArray.Ptr[i];
			strArray.Ptr[i] = nullptr;
		}

		delete[] strArray.Ptr;
		strArray.Ptr = nullptr;
	}
};

struct NativeGenInfo
{
	TCHAR* UftPath;
	TCHAR* SdkPath;
	TCHAR* LangPath;

	TCHAR* SdkLang;
	TCHAR* GameName;
	TCHAR* GameVersion;
	TCHAR* NamespaceName;

	size_t MemberAlignment;
	size_t PointerSize;

	BOOL IsExternal;
	BOOL IsGObjectsChunks;
	BOOL ShouldConvertStaticMethods;
	BOOL ShouldUseStrings;
	BOOL ShouldXorStrings;
	BOOL ShouldGenerateFunctionParametersFile;

	NativeGenInfo() = default;
	explicit NativeGenInfo(
		const std::string& uftPath, const std::string& sdkPath, const std::string& langPath, const std::string& sdkLang,
		const std::string& gameName, const std::string& gameVersion, const std::string& namespaceName,
		const size_t memberAlignment, const size_t pointerSize,
		const BOOL isExternal, const BOOL isGObjectsChunks, const BOOL shouldConvertStaticMethods, const BOOL shouldUseStrings,
		const BOOL shouldXorStrings, const BOOL shouldGenerateFunctionParametersFile
	)
	{
		UftPath = NativeHelper::CreateHeapStr(uftPath);
		SdkPath = NativeHelper::CreateHeapStr(sdkPath);
		LangPath = NativeHelper::CreateHeapStr(langPath);
		SdkLang = NativeHelper::CreateHeapStr(sdkLang);
		GameName = NativeHelper::CreateHeapStr(gameName);
		GameVersion = NativeHelper::CreateHeapStr(gameVersion);
		NamespaceName = NativeHelper::CreateHeapStr(namespaceName);

		MemberAlignment = memberAlignment;
		PointerSize = pointerSize;

		IsExternal = isExternal;
		IsGObjectsChunks = isGObjectsChunks;
		ShouldConvertStaticMethods = shouldConvertStaticMethods;
		ShouldUseStrings = shouldUseStrings;
		ShouldXorStrings = shouldXorStrings;
		ShouldGenerateFunctionParametersFile = shouldGenerateFunctionParametersFile;
	}
	~NativeGenInfo()
	{
		NativeHelper::FreeHeapStr(UftPath);
		NativeHelper::FreeHeapStr(SdkPath);
		NativeHelper::FreeHeapStr(LangPath);
		NativeHelper::FreeHeapStr(SdkLang);
		NativeHelper::FreeHeapStr(GameName);
		NativeHelper::FreeHeapStr(GameVersion);
		NativeHelper::FreeHeapStr(NamespaceName);
	}
};

struct NativeJsonVar
{
	TCHAR* Name;
	TCHAR* Type;
	size_t Size, Offset;

	NativeJsonVar() = default;
	explicit NativeJsonVar(const JsonVar& jVar)
	{
		Name = NativeHelper::CreateHeapStr(jVar.Name);
		Type = NativeHelper::CreateHeapStr(jVar.Type);
		Size = jVar.Size;
		Offset = jVar.Offset;
	}
	~NativeJsonVar()
	{
		NativeHelper::FreeHeapStr(Name);
		NativeHelper::FreeHeapStr(Type);
	}
};

struct NativeJsonStruct
{
	TCHAR* StructName = nullptr;
	TCHAR* StructSuper = nullptr;
	StructArray<NativeJsonVar> Members;

	NativeJsonStruct() = default;
	explicit NativeJsonStruct(const JsonStruct& jStruct)
	{
		StructName = NativeHelper::CreateHeapStr(jStruct.StructName);
		StructSuper = NativeHelper::CreateHeapStr(jStruct.StructSuper);

		const auto members = new NativeJsonVar*[jStruct.Vars.size()];
		for (size_t i = 0; i < jStruct.Vars.size(); i++)
		{
			auto& curVar = jStruct.Vars[i];
			members[i] = new NativeJsonVar(curVar.second);
		}
		Members = StructArray<NativeJsonVar>(members, jStruct.Vars.size());
	}
	~NativeJsonStruct()
	{
		NativeHelper::FreeHeapStr(StructName);
		NativeHelper::FreeHeapStr(StructSuper);
		NativeHelper::FreeNativeStructArray(Members);
	}
};

struct NativePredefinedMethod
{
	TCHAR* Signature;
	TCHAR* Body;
	PredefinedMethod::Type MethodType;

	NativePredefinedMethod() = default;
	explicit NativePredefinedMethod(const PredefinedMethod& method)
	{
		Signature = NativeHelper::CreateHeapStr(method.Signature);
		Body = NativeHelper::CreateHeapStr(method.Body);
		MethodType = method.MethodType;
	}
	~NativePredefinedMethod()
	{
		NativeHelper::FreeHeapStr(Signature);
		NativeHelper::FreeHeapStr(Body);
	}
};

struct NativeConstant
{
	TCHAR* Name;
	TCHAR* Value;

	NativeConstant() = default;
	explicit NativeConstant(const Package::Constant& constant)
	{
		Name = NativeHelper::CreateHeapStr(constant.Name);
		Value = NativeHelper::CreateHeapStr(constant.Value);
	}
	~NativeConstant()
	{
		NativeHelper::FreeHeapStr(Name);
		NativeHelper::FreeHeapStr(Value);
	}
};

struct NativeEnum
{
	TCHAR* Name = nullptr;
	TCHAR* FullName = nullptr;
	StringArray Values;

	NativeEnum() = default;
	explicit NativeEnum(const Package::Enum& sdkEnum)
	{
		Name = NativeHelper::CreateHeapStr(sdkEnum.Name);
		FullName = NativeHelper::CreateHeapStr(sdkEnum.FullName);
		Values = StringArray(sdkEnum.Values);
	}
	~NativeEnum()
	{
		NativeHelper::FreeHeapStr(Name);
		NativeHelper::FreeHeapStr(FullName);
		NativeHelper::FreeHeapArrayStr(Values);
	}
};

struct NativeMember
{
	TCHAR* Name;
	TCHAR* Type;
	BOOL IsStatic;

	size_t Offset;
	size_t Size;

	size_t Flags;
	TCHAR* FlagsString;

	TCHAR* Comment;

	NativeMember() = default;
	explicit NativeMember(const Package::Member& member)
	{
		Name = NativeHelper::CreateHeapStr(member.Name);
		Type = NativeHelper::CreateHeapStr(member.Type);
		IsStatic = member.IsStatic;
		Offset = member.Offset;
		Size = member.Size;
		Flags = member.Flags;
		FlagsString = NativeHelper::CreateHeapStr(member.FlagsString);
		Comment = NativeHelper::CreateHeapStr(member.Comment);
	}
	~NativeMember()
	{
		NativeHelper::FreeHeapStr(Name);
		NativeHelper::FreeHeapStr(Type);
		NativeHelper::FreeHeapStr(FlagsString);
		NativeHelper::FreeHeapStr(Comment);
	}
};

struct NativeScriptStruct
{
	TCHAR* Name = nullptr;
	TCHAR* FullName = nullptr;
	TCHAR* NameCpp = nullptr;
	TCHAR* NameCppFull = nullptr;

	size_t Size = 0;
	size_t InheritedSize = 0;

	StructArray<NativeMember> Members;
	StructArray<NativePredefinedMethod> PredefinedMethods;

	NativeScriptStruct() = default;
	explicit NativeScriptStruct(const Package::ScriptStruct& ss)
	{
		Name = NativeHelper::CreateHeapStr(ss.Name);
		FullName = NativeHelper::CreateHeapStr(ss.FullName);
		NameCpp = NativeHelper::CreateHeapStr(ss.NameCpp);
		NameCppFull = NativeHelper::CreateHeapStr(ss.NameCppFull);
		Size = ss.Size;
		InheritedSize = ss.InheritedSize;

		NativeHelper::HeapNativeStructArray(ss.Members, Members);
		NativeHelper::HeapNativeStructArray(ss.PredefinedMethods, PredefinedMethods);
	}
	~NativeScriptStruct()
	{
		NativeHelper::FreeHeapStr(Name);
		NativeHelper::FreeHeapStr(FullName);
		NativeHelper::FreeHeapStr(NameCpp);
		NativeHelper::FreeHeapStr(NameCppFull);
		NativeHelper::FreeNativeStructArray(Members);
		NativeHelper::FreeNativeStructArray(PredefinedMethods);
	}
};

struct NativeMethod
{
	struct Parameter
	{
		Package::Method::Parameter::Type ParamType = Package::Method::Parameter::Type::Default;
		BOOL PassByReference = FALSE;
		TCHAR* CppType = nullptr;
		TCHAR* Name = nullptr;
		TCHAR* FlagsString = nullptr;

		Parameter() = default;
		explicit Parameter(const Package::Method::Parameter& param)
		{
			ParamType = param.ParamType;
			PassByReference = param.PassByReference;
			CppType = NativeHelper::CreateHeapStr(param.CppType);
			Name = NativeHelper::CreateHeapStr(param.Name);
			FlagsString = NativeHelper::CreateHeapStr(param.FlagsString);
		}
		~Parameter()
		{
			NativeHelper::FreeHeapStr(CppType);
			NativeHelper::FreeHeapStr(Name);
			NativeHelper::FreeHeapStr(FlagsString);
		}
	};

	size_t Index = 0;
	TCHAR* Name = nullptr;
	TCHAR* FullName = nullptr;
	StructArray<Parameter> Parameters;
	TCHAR* FlagsString = nullptr;
	BOOL IsNative = FALSE;
	BOOL IsStatic = FALSE;

	NativeMethod() = default;
	explicit NativeMethod(const Package::Method& method)
	{
		Name = NativeHelper::CreateHeapStr(method.Name);
		FlagsString = NativeHelper::CreateHeapStr(method.FlagsString);
		FullName = NativeHelper::CreateHeapStr(method.FullName);
		Index = method.Index;
		IsNative = method.IsNative;
		IsStatic = method.IsStatic;
		NativeHelper::HeapNativeStructArray(method.Parameters, Parameters);
	}
	~NativeMethod()
	{
		NativeHelper::FreeHeapStr(Name);
		NativeHelper::FreeHeapStr(FlagsString);
		NativeHelper::FreeHeapStr(FullName);
		NativeHelper::FreeNativeStructArray(Parameters);
	}
};

struct NativeClass : NativeScriptStruct
{
	StringArray VirtualFunctions;
	StructArray<NativeMethod> Methods;

	NativeClass() = default;
	explicit NativeClass(const Package::Class& sdkClass) : NativeScriptStruct(sdkClass)
	{
		VirtualFunctions = StringArray(sdkClass.VirtualFunctions);
		NativeHelper::HeapNativeStructArray(sdkClass.Methods, Methods);
	}
	~NativeClass()
	{
		NativeHelper::FreeHeapArrayStr(VirtualFunctions);
		NativeHelper::FreeNativeStructArray(Methods);
	}
};

struct NativePackage
{
	TCHAR* Name = nullptr;
	StructArray<NativeConstant> Constants;
	StructArray<NativeClass> Classes;
	StructArray<NativeScriptStruct> ScriptStructs;
	StructArray<NativeEnum> Enums;

	NativePackage() = default;
	explicit NativePackage(const Package& sdkPackage)
	{
		Name = NativeHelper::CreateHeapStr(sdkPackage.GetName());
		NativeHelper::HeapNativeStructArray(sdkPackage.Constants, Constants);
		NativeHelper::HeapNativeStructArray(sdkPackage.Classes, Classes);
		NativeHelper::HeapNativeStructArray(sdkPackage.ScriptStructs, ScriptStructs);
		NativeHelper::HeapNativeStructArray(sdkPackage.Enums, Enums);
	}
	~NativePackage()
	{
		NativeHelper::FreeHeapStr(Name);
		NativeHelper::FreeNativeStructArray(Constants);
		NativeHelper::FreeNativeStructArray(Classes);
		NativeHelper::FreeNativeStructArray(ScriptStructs);
		NativeHelper::FreeNativeStructArray(Enums);
	}
};

struct NativeUStruct
{
	TCHAR* Name;
	TCHAR* FullName;
	TCHAR* CppName;
	size_t PropertySize;

	NativeUStruct() = default;
	explicit NativeUStruct(const UEStruct& sdkUStruct)
	{
		Name = NativeHelper::CreateHeapStr(sdkUStruct.GetName());
		FullName = NativeHelper::CreateHeapStr(sdkUStruct.GetFullName());
		CppName = NativeHelper::CreateHeapStr(sdkUStruct.GetNameCpp());
		PropertySize = sdkUStruct.GetPropertySize();
	}
	~NativeUStruct()
	{
		NativeHelper::FreeHeapStr(Name);
		NativeHelper::FreeHeapStr(FullName);
		NativeHelper::FreeHeapStr(CppName);
	}
};
