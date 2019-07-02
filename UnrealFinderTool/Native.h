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

class Helper
{
public:
	static TCHAR* CreateHeapStr(const std::string& baseStr)
	{
		if (baseStr.empty())
			return nullptr;

		auto ptr = new TCHAR[baseStr.length() + 1];
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
		auto ptr = Helper::HeapSdkNativeArray<TNativeType>(baseValue);
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
		UftPath = Helper::CreateHeapStr(uftPath);
		SdkPath = Helper::CreateHeapStr(sdkPath);
		LangPath = Helper::CreateHeapStr(langPath);
		SdkLang = Helper::CreateHeapStr(sdkLang);
		GameName = Helper::CreateHeapStr(gameName);
		GameVersion = Helper::CreateHeapStr(gameVersion);
		NamespaceName = Helper::CreateHeapStr(namespaceName);

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
		Helper::FreeHeapStr(UftPath);
		Helper::FreeHeapStr(SdkPath);
		Helper::FreeHeapStr(LangPath);
		Helper::FreeHeapStr(SdkLang);
		Helper::FreeHeapStr(GameName);
		Helper::FreeHeapStr(GameVersion);
		Helper::FreeHeapStr(NamespaceName);
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
		Name = Helper::CreateHeapStr(jVar.Name);
		Type = Helper::CreateHeapStr(jVar.Type);
		Size = jVar.Size;
		Offset = jVar.Offset;
	}
	~NativeJsonVar()
	{
		Helper::FreeHeapStr(Name);
		Helper::FreeHeapStr(Type);
	}
};

struct NativeJsonStruct
{
	TCHAR* StructName;
	TCHAR* StructSuper;
	StructArray<NativeJsonVar> Members;

	NativeJsonStruct() = default;
	explicit NativeJsonStruct(const JsonStruct& jStruct)
	{
		StructName = Helper::CreateHeapStr(jStruct.StructName);
		StructSuper = Helper::CreateHeapStr(jStruct.StructSuper);

		auto members = new NativeJsonVar*[jStruct.Vars.size()];
		for (size_t i = 0; i < jStruct.Vars.size(); i++)
		{
			auto& curVar = jStruct.Vars[i];
			members[i] = new NativeJsonVar(curVar.second);
		}
		Members = StructArray<NativeJsonVar>(members, jStruct.Vars.size());
	}
	~NativeJsonStruct()
	{
		Helper::FreeHeapStr(StructName);
		Helper::FreeHeapStr(StructSuper);
		Helper::FreeNativeStructArray(Members);
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
		Signature = Helper::CreateHeapStr(method.Signature);
		Body = Helper::CreateHeapStr(method.Body);
		MethodType = method.MethodType;
	}
	~NativePredefinedMethod()
	{
		Helper::FreeHeapStr(Signature);
		Helper::FreeHeapStr(Body);
	}
};

struct NativeConstant
{
	TCHAR* Name;
	TCHAR* Value;

	NativeConstant() = default;
	explicit NativeConstant(const Package::Constant& constant)
	{
		Name = Helper::CreateHeapStr(constant.Name);
		Value = Helper::CreateHeapStr(constant.Value);
	}
	~NativeConstant()
	{
		Helper::FreeHeapStr(Name);
		Helper::FreeHeapStr(Value);
	}
};

struct NativeEnum
{
	TCHAR* Name;
	TCHAR* FullName;
	StringArray Values;

	NativeEnum() = default;
	explicit NativeEnum(const Package::Enum& sdkEnum)
	{
		Name = Helper::CreateHeapStr(sdkEnum.Name);
		FullName = Helper::CreateHeapStr(sdkEnum.FullName);
		Values = StringArray(sdkEnum.Values);
	}
	~NativeEnum()
	{
		Helper::FreeHeapStr(Name);
		Helper::FreeHeapStr(FullName);
		Helper::FreeHeapArrayStr(Values);
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
		Name = Helper::CreateHeapStr(member.Name);
		Type = Helper::CreateHeapStr(member.Type);
		IsStatic = member.IsStatic;
		Offset = member.Offset;
		Size = member.Size;
		Flags = member.Flags;
		FlagsString = Helper::CreateHeapStr(member.FlagsString);
		Comment = Helper::CreateHeapStr(member.Comment);
	}
	~NativeMember()
	{
		Helper::FreeHeapStr(Name);
		Helper::FreeHeapStr(Type);
		Helper::FreeHeapStr(FlagsString);
		Helper::FreeHeapStr(Comment);
	}
};

struct NativeScriptStruct
{
	TCHAR* Name;
	TCHAR* FullName;
	TCHAR* NameCpp;
	TCHAR* NameCppFull;

	size_t Size;
	size_t InheritedSize;

	StructArray<NativeMember> Members;
	StructArray<NativePredefinedMethod> PredefinedMethods;

	NativeScriptStruct() = default;
	explicit NativeScriptStruct(const Package::ScriptStruct& ss)
	{
		Name = Helper::CreateHeapStr(ss.Name);
		FullName = Helper::CreateHeapStr(ss.FullName);
		NameCpp = Helper::CreateHeapStr(ss.NameCpp);
		NameCppFull = Helper::CreateHeapStr(ss.NameCppFull);
		Size = ss.Size;
		InheritedSize = ss.InheritedSize;

		Helper::HeapNativeStructArray(ss.Members, Members);
		Helper::HeapNativeStructArray(ss.PredefinedMethods, PredefinedMethods);
	}
	~NativeScriptStruct()
	{
		Helper::FreeHeapStr(Name);
		Helper::FreeHeapStr(FullName);
		Helper::FreeHeapStr(NameCpp);
		Helper::FreeHeapStr(NameCppFull);
		Helper::FreeNativeStructArray(Members);
		Helper::FreeNativeStructArray(PredefinedMethods);
	}
};

struct NativeMethod
{
	struct Parameter
	{
		Package::Method::Parameter::Type ParamType;
		BOOL PassByReference;
		TCHAR* CppType;
		TCHAR* Name;
		TCHAR* FlagsString;

		Parameter() = default;
		explicit Parameter(const Package::Method::Parameter& param)
		{
			ParamType = param.ParamType;
			PassByReference = param.PassByReference;
			CppType = Helper::CreateHeapStr(param.CppType);
			Name = Helper::CreateHeapStr(param.Name);
			FlagsString = Helper::CreateHeapStr(param.FlagsString);
		}
		~Parameter()
		{
			Helper::FreeHeapStr(CppType);
			Helper::FreeHeapStr(Name);
			Helper::FreeHeapStr(FlagsString);
		}
	};

	size_t Index;
	TCHAR* Name;
	TCHAR* FullName;
	StructArray<Parameter> Parameters;
	TCHAR* FlagsString;
	BOOL IsNative;
	BOOL IsStatic;

	NativeMethod() = default;
	explicit NativeMethod(const Package::Method& method)
	{
		Name = Helper::CreateHeapStr(method.Name);
		FlagsString = Helper::CreateHeapStr(method.FlagsString);
		FullName = Helper::CreateHeapStr(method.FullName);
		Index = method.Index;
		IsNative = method.IsNative;
		IsStatic = method.IsStatic;
		Helper::HeapNativeStructArray(method.Parameters, Parameters);
	}
	~NativeMethod()
	{
		Helper::FreeHeapStr(Name);
		Helper::FreeHeapStr(FlagsString);
		Helper::FreeHeapStr(FullName);
		Helper::FreeNativeStructArray(Parameters);
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
		Helper::HeapNativeStructArray(sdkClass.Methods, Methods);
	}
	~NativeClass()
	{
		Helper::FreeHeapArrayStr(VirtualFunctions);
		Helper::FreeNativeStructArray(Methods);
	}
};

struct NativePackage
{
	TCHAR* Name;
	StructArray<NativeConstant> Constants;
	StructArray<NativeClass> Classes;
	StructArray<NativeScriptStruct> ScriptStructs;
	StructArray<NativeEnum> Enums;

	NativePackage() = default;
	explicit NativePackage(const Package& sdkPackage)
	{
		Name = Helper::CreateHeapStr(sdkPackage.GetName());
		Helper::HeapNativeStructArray(sdkPackage.Constants, Constants);
		Helper::HeapNativeStructArray(sdkPackage.Classes, Classes);
		Helper::HeapNativeStructArray(sdkPackage.ScriptStructs, ScriptStructs);
		Helper::HeapNativeStructArray(sdkPackage.Enums, Enums);
	}
	~NativePackage()
	{
		Helper::FreeHeapStr(Name);
		Helper::FreeNativeStructArray(Constants);
		Helper::FreeNativeStructArray(Classes);
		Helper::FreeNativeStructArray(ScriptStructs);
		Helper::FreeNativeStructArray(Enums);
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
		Name = Helper::CreateHeapStr(sdkUStruct.GetName());
		FullName = Helper::CreateHeapStr(sdkUStruct.GetFullName());
		CppName = Helper::CreateHeapStr(sdkUStruct.GetNameCpp());
		PropertySize = sdkUStruct.GetPropertySize();
	}
	~NativeUStruct()
	{
		Helper::FreeHeapStr(Name);
		Helper::FreeHeapStr(FullName);
		Helper::FreeHeapStr(CppName);
	}
};
