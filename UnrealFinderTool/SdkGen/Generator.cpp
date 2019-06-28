#include "pch.h"
#include "Utils.h"
#include "JsonReflector.h"
#include "Generator.h"

bool Generator::Initialize()
{
	keywordsName =
	{
		{"return", "returnValue"},
		{"continue", "continueValue"},
		{"break", "breakValue"},
		{"int", "intValue"}
	};

	badChars =
	{
		{",", ""},
		{"!", ""},
		{"-", ""}
	};

	alignasClasses =
	{
		{"ScriptStruct CoreUObject.Plane", 16},
		{"ScriptStruct CoreUObject.Quat", 16},
		{"ScriptStruct CoreUObject.Transform", 16},
		{"ScriptStruct CoreUObject.Vector4", 16},

		{"ScriptStruct Engine.RootMotionSourceGroup", 8}
	};

	virtualFunctionPattern["Class CoreUObject.Object"] =
	{
		{
			PatternScan::Parse("ProcessEvent", 0, "40 55 56 57 41 54 41 55 41 56 41 57", 0xFF),
			R"(	inline void ProcessEvent(class UFunction* function, void* parms)
	{
		GetVFunction<void(*)(UObject*, class UFunction*, void*)>(this, %d)(this, function, parms);
	})"
		}
	};
	virtualFunctionPattern["Class CoreUObject.Class"] =
	{
		{
			PatternScan::Parse("CreateDefaultObject", 0, "4C 8B DC 57 48 81 EC", 0xFF),
			R"(	inline UObject* CreateDefaultObject()
	{
		GetVFunction<UObject*(*)(UClass*)>(this, %d)(this);
	})"
		}
	};

	predefinedMembers["Class CoreUObject.Object"] =
	{
		{"void*", "Vtable"},
		{"int32_t", "ObjectFlags"},
		{"int32_t", "InternalIndex"},
		{"class UClass*", "Class"},
		{"FName", "Name"},
		{"class UObject*", "Outer"}
	};

	predefinedStaticMembers["Class CoreUObject.Object"] =
	{
		{"FUObjectArray*", "GObjects"}
	};

	predefinedMembers["Class CoreUObject.Field"] =
	{
		{"class UField*", "Next"}
	};

	predefinedMembers["Class CoreUObject.Struct"] =
	{
		{"class UStruct*", "SuperField"},
		{"class UField*", "Children"},
		{"int32_t", "PropertySize"},
		{"int32_t", "MinAlignment"},
		{"unsigned char", "UnknownData0x0048[0x40]"}
	};

	predefinedMembers["Class CoreUObject.Function"] =
	{
		{"int32_t", "FunctionFlags"},
		{"int16_t", "RepOffset"},
		{"int8_t", "NumParms"},
		{"int16_t", "ParmsSize"},
		{"int16_t", "ReturnValueOffset"},
		{"int16_t", "RPCId"},
		{"int16_t", "RPCResponseId"},
		{"class UProperty*", "FirstPropertyToInit"},
		{"class UFunction*", "EventGraphFunction"},
		{"int32_t", "EventGraphCallOffset"},
		{"void*", "Func"}
	};

	predefinedMethods["ScriptStruct CoreUObject.Vector2D"] =
	{
		PredefinedMethod::Inline(R"(	inline FVector2D()
		: X(0), Y(0)
	{ })"),
		PredefinedMethod::Inline(R"(	inline FVector2D(float x, float y)
		: X(x),
		  Y(y)
	{ })")
	};
	predefinedMethods["ScriptStruct CoreUObject.LinearColor"] =
	{
		PredefinedMethod::Inline(R"(	inline FLinearColor()
		: R(0), G(0), B(0), A(0)
	{ })"),
		PredefinedMethod::Inline(
			R"(	inline FLinearColor(float r, float g, float b, float a)
		: R(r),
		  G(g),
		  B(b),
		  A(a)
	{ })")
	};

	predefinedMethods["Class CoreUObject.Object"] =
	{
		PredefinedMethod::Inline(
			R"(	static inline TUObjectArray& GetGlobalObjects()
	{
		return GObjects->ObjObjects;
	})"),
		PredefinedMethod::Default("std::string GetName() const",
		                          R"(std::string UObject::GetName() const
{
	std::string name(Name.GetName());
	if (Name.Number > 0)
	{
		name += '_' + std::to_string(Name.Number);
	}

	auto pos = name.rfind('/');
	if (pos == std::string::npos)
	{
		return name;
	}
	
	return name.substr(pos + 1);
})"),
		PredefinedMethod::Default("std::string GetFullName() const",
		                          R"(std::string UObject::GetFullName() const
{
	std::string name;

	if (Class != nullptr)
	{
		std::string temp;
		for (auto p = Outer; p; p = p->Outer)
		{
			temp = p->GetName() + "." + temp;
		}

		name = Class->GetName();
		name += " ";
		name += temp;
		name += GetName();
	}

	return name;
})"),
		PredefinedMethod::Inline(
			R"(	template<typename T>
	static T* FindObject(const std::string& name)
	{
		for (int i = 0; i < GetGlobalObjects().Num(); ++i)
		{
			auto object = GetGlobalObjects().GetByIndex(i);
	
			if (object == nullptr)
			{
				continue;
			}
	
			if (object->GetFullName() == name)
			{
				return static_cast<T*>(object);
			}
		}
		return nullptr;
	})"),
		PredefinedMethod::Inline(
			R"(	template<typename T>
	static T* FindObject()
	{
		auto v = T::StaticClass();
		for (int i = 0; i < SDK::UObject::GetGlobalObjects().Num(); ++i)
		{
			auto object = SDK::UObject::GetGlobalObjects().GetByIndex(i);

			if (object == nullptr)
			{
				continue;
			}

			if (object->IsA(v))
			{
				return static_cast<T*>(object);
			}
		}
		return nullptr;
	})"),
		PredefinedMethod::Inline(
			R"(	template<typename T>
	static std::vector<T*> FindObjects(const std::string& name)
	{
		std::vector<T*> ret;
		for (int i = 0; i < GetGlobalObjects().Num(); ++i)
		{
			auto object = GetGlobalObjects().GetByIndex(i);

			if (object == nullptr)
			{
				continue;
			}

			if (object->GetFullName() == name)
			{
				ret.push_back(static_cast<T*>(object));
			}
		}
		return ret;
	})"),
		PredefinedMethod::Inline(
			R"(	template<typename T>
	static std::vector<T*> FindObjects()
	{
		std::vector<T*> ret;
		auto v = T::StaticClass();
		for (int i = 0; i < SDK::UObject::GetGlobalObjects().Num(); ++i)
		{
			auto object = SDK::UObject::GetGlobalObjects().GetByIndex(i);

			if (object == nullptr)
			{
				continue;
			}

			if (object->IsA(v))
			{
				ret.push_back(static_cast<T*>(object));
			}
		}
		return ret;
	})"),
		PredefinedMethod::Inline(
			R"(	static UClass* FindClass(const std::string& name)
	{
		return FindObject<UClass>(name);
	})"),
		PredefinedMethod::Inline(
			R"(	template<typename T>
	static T* GetObjectCasted(std::size_t index)
	{
		return static_cast<T*>(GetGlobalObjects().GetByIndex(index));
	})"),
		PredefinedMethod::Default("bool IsA(UClass* cmp) const",
		                          R"(bool UObject::IsA(UClass* cmp) const
{
	for (auto super = Class; super; super = static_cast<UClass*>(super->SuperField))
	{
		if (super == cmp)
		{
			return true;
		}
	}

	return false;
})")
	};
	predefinedMethods["Class CoreUObject.Class"] =
	{
		PredefinedMethod::Inline(
			R"(	template<typename T>
	inline T* CreateDefaultObject()
	{
		return static_cast<T*>(CreateDefaultObject());
	})")
	};

	/*
	predefinedMethods["Class Engine.GameViewportClient"] =
	{
		PredefinedMethod::Inline(R"(	inline void PostRender(UCanvas* Canvas)
{
	return GetVFunction<void(*)(UGameViewportClient*, UCanvas*)>(this, %d)(this, Canvas);
})")
	};
	*/
	return true;
}

std::string Generator::GetOutputDirectory() const
{
	return "C:\\SDK";
}

std::string Generator::GetGameName() const
{
	return gameName;
}

void Generator::SetGameName(const std::string& gameName) const
{
	this->gameName = gameName;
}

std::string Generator::GetGameVersion() const
{
	return this->gameVersion;
}

void Generator::SetGameVersion(const std::string& gameVersion) const
{
	this->gameVersion = gameVersion;
}

SdkType Generator::GetSdkType() const
{
	return this->sdkType;
}

void Generator::SetSdkType(const SdkType sdkType) const
{
	this->sdkType = sdkType;
}

std::string Generator::GetNamespaceName() const
{
	return "SDK";
}

std::vector<std::string> Generator::GetIncludes() const
{
	return {};
}

size_t Generator::GetGlobalMemberAlignment() const
{
	return sizeof(size_t);
}

size_t Generator::GetClassAlignas(const std::string& name) const
{
	auto it = alignasClasses.find(name);
	if (it != std::end(alignasClasses))
	{
		return it->second;
	}
	return 0;
}

std::string Generator::GetOverrideType(const std::string& type) const
{
	auto it = overrideTypes.find(type);
	if (it == std::end(overrideTypes))
	{
		return type;
	}
	return it->second;
}

std::string Generator::GetSafeKeywordsName(const std::string& name) const
{
	std::string ret = name;
	auto it = keywordsName.find(ret);
	if (it == std::end(keywordsName))
	{
		for (const auto& badChar : badChars)
			ret = Utils::ReplaceString(ret, badChar.first, badChar.second);
		return ret;
	}

	ret = it->second;
	for (const auto& badChar : badChars)
		ret = Utils::ReplaceString(ret, badChar.first, badChar.second);

	return ret;
}

bool Generator::GetPredefinedClassMembers(const std::string& name, std::vector<PredefinedMember>& members) const
{
	auto it = predefinedMembers.find(name);
	if (it != std::end(predefinedMembers))
	{
		std::copy(std::begin(it->second), std::end(it->second), std::back_inserter(members));

		return true;
	}

	return false;
}

bool Generator::GetPredefinedClassStaticMembers(const std::string& name, std::vector<PredefinedMember>& members) const
{
	auto it = predefinedStaticMembers.find(name);
	if (it != std::end(predefinedStaticMembers))
	{
		std::copy(std::begin(it->second), std::end(it->second), std::back_inserter(members));

		return true;
	}

	return false;
}

bool Generator::GetVirtualFunctionPatterns(const std::string& name, VirtualFunctionPatterns& patterns) const
{
	auto it = virtualFunctionPattern.find(name);
	if (it != std::end(virtualFunctionPattern))
	{
		std::copy(std::begin(it->second), std::end(it->second), std::back_inserter(patterns));

		return true;
	}

	return false;
}

bool Generator::GetPredefinedClassMethods(const std::string& name, std::vector<PredefinedMethod>& methods) const
{
	auto it = predefinedMethods.find(name);
	if (it != std::end(predefinedMethods))
	{
		std::copy(std::begin(it->second), std::end(it->second), std::back_inserter(methods));

		return true;
	}

	return false;
}

bool Generator::ShouldGenerateFunctionParametersFile() const
{
	return sdkType == SdkType::Internal;
}

void Generator::SetIsGObjectsChunks(const bool isChunks) const
{
	isGObjectsChunks = isChunks;
}

bool Generator::GetIsGObjectsChunks() const
{
	return isGObjectsChunks;
}

bool Generator::ShouldDumpArrays() const
{
	return false;
}

bool Generator::ShouldGenerateEmptyFiles() const
{
	return false;
}

bool Generator::ShouldUseStrings() const
{
	return true;
}

bool Generator::ShouldXorStrings() const
{
	return false;
}

bool Generator::ShouldConvertStaticMethods() const
{
	return true;
}
