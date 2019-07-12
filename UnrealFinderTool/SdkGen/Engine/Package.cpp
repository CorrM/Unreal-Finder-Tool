#include "pch.h"
#include "tinyformat.h"
#include "cpplinq.hpp"
#include "Logger.h"
#include "NameValidator.h"
#include "PatternScan.h"
#include "ObjectsStore.h"
#include "PropertyFlags.h"
#include "FunctionFlags.h"
#include "ParallelWorker.h"
#include "DotNetConnect.h"
#include "Package.h"
#include "Native.h"
#include "Utils.h"

#include <unordered_set>
#include <cinttypes>

std::unordered_map<UEObject, const Package*> Package::PackageMap;

/// <summary>
/// Compare two properties.
/// </summary>
/// <param name="lhs">The first property.</param>
/// <param name="rhs">The second property.</param>
/// <returns>true if the first property compares less, else false.</returns>
bool ComparePropertyLess(const UEProperty& lhs, const UEProperty& rhs)
{
	if (lhs.GetOffset() == rhs.GetOffset()
		&& lhs.IsA<UEBoolProperty>()
		&& rhs.IsA<UEBoolProperty>())
	{
		return lhs.Cast<UEBoolProperty>() < rhs.Cast<UEBoolProperty>();
	}

	return lhs.GetOffset() < rhs.GetOffset();
}

Package::Package(UEObject* packageObj)
	: packageObj(packageObj)
{
}

void Package::Process(std::unordered_map<uintptr_t, bool>& processedObjects, std::mutex& packageLocker)
{
	using ObjectItem = std::pair<uintptr_t, std::unique_ptr<UEObject>>;
	static int process_sleep_counter = 0;
	std::vector<UEObject*> objsInPack;

	// Get all objects in this package
	{
		// Wait to get package object's, just to use 6 threads instead of 12 thread
		std::lock_guard lock(packageLocker);

		ParallelQueue<GObjectContainer<UEObject>, ObjectItem>
		worker(ObjectsStore::GObjObjects, 0, Utils::Settings.SdkGen.Threads, [&](ObjectItem& curObj, ParallelOptions& options)
		{
			UEObject* obj = curObj.second.get();
			UEObject* package = obj->GetPackageObject();

			if (packageObj == package)
			{
				std::lock_guard push_lock(options.Locker);
				objsInPack.push_back(obj);
			}
		});
		worker.Start();
		worker.WaitAll();
	}

	for (UEObject* obj : objsInPack)
	{
		if (obj->IsA<UEEnum>())
		{
			GenerateEnum(obj->Cast<UEEnum>());
		}
		else if (obj->IsA<UEClass>())
		{
			GeneratePrerequisites(*obj, processedObjects);
		}
		else if (obj->IsA<UEScriptStruct>())
		{
			GeneratePrerequisites(*obj, processedObjects);
		}
		else if (obj->IsA<UEConst>())
		{
			GenerateConst(obj->Cast<UEConst>());
		}
		Utils::SleepEvery(1, process_sleep_counter, Utils::Settings.Parallel.SleepEvery);
	}
}

bool Package::Save() const
{
	using namespace cpplinq;

	//check if package is empty (no enums, structs or classes without members)
	if (Utils::GenObj->ShouldGenerateEmptyFiles()
		|| (from(Enums) >> where([](Enum&& e) { return !e.Values.empty(); }) >> any()
		|| from(ScriptStructs) >> where([](ScriptStruct&& s) { return !s.Members.empty() || !s.PredefinedMethods.empty(); }) >> any()
		|| from(Classes) >> where([](Class&& c) {return !c.Members.empty() || !c.PredefinedMethods.empty() || !c.Methods.empty(); }) >> any())
		)
	{
		SaveStructs();
		SaveClasses();
		SaveFunctions();

		return true;
	}

	if (Utils::Settings.SdkGen.LoggerShowSkip)
	{
		Logger::Log("Skip Empty:    %s", packageObj->GetFullName());
	}
	
	return false;
}

bool Package::AddDependency(UEObject* package) const
{
	if (package == packageObj) return false;

	dependencies.insert(*package);
	return true;
}

void Package::GeneratePrerequisites(const UEObject& obj, std::unordered_map<uintptr_t, bool>& processedObjects)
{
	if (!obj.IsValid())
		return;

	const auto isClass = obj.IsA<UEClass>();
	const auto isScriptStruct = obj.IsA<UEScriptStruct>();
	if (!isClass && !isScriptStruct)
		return;

	const auto name = obj.GetName();
	if (name.find("Default__") != std::string::npos
		|| name.find("<uninitialized>") != std::string::npos
		|| name.find("PLACEHOLDER-CLASS") != std::string::npos)
	{
		return;
	}

	{
		std::lock_guard mainLocker(Utils::MainMutex);
		processedObjects[obj.GetAddress()] |= false;
	}

	auto classPackage = obj.GetPackageObject();
	if (!classPackage->IsValid())
		return;

	if (AddDependency(classPackage))
		return;

	if (!processedObjects[obj.GetAddress()])
	{
		processedObjects[obj.GetAddress()] = true;

		auto outer = obj.GetOuter();
		if (outer->IsValid() && *outer != obj)
		{
			GeneratePrerequisites(*outer, processedObjects);
		}

		auto structObj = obj.Cast<UEStruct>();
		auto super = structObj.GetSuper();
		if (super.IsValid() && super != obj)
		{
			GeneratePrerequisites(super, processedObjects);
		}

		GenerateMemberPrerequisites(structObj.GetChildren().Cast<UEProperty>(), processedObjects);

		if (isClass)
		{
			GenerateClass(obj.Cast<UEClass>());
		}
		else
		{
			GenerateScriptStruct(obj.Cast<UEScriptStruct>());
		}
	}
}

void Package::GenerateMemberPrerequisites(const UEProperty& first, std::unordered_map<uintptr_t, bool>& processedObjects)
{
	using namespace cpplinq;

	for (UEProperty prop = first; prop.IsValid(); prop = prop.GetNext().Cast<UEProperty>())
	{
		const auto info = prop.GetInfo();
		if (info.Type == UEProperty::PropertyType::Primitive)
		{
			if (prop.IsA<UEByteProperty>())
			{
				auto byteProperty = prop.Cast<UEByteProperty>();
				if (byteProperty.IsEnum())
				{
					AddDependency(byteProperty.GetEnum().GetPackageObject());
				}
			}
			else if (prop.IsA<UEEnumProperty>())
			{
				auto enumProperty = prop.Cast<UEEnumProperty>();
				AddDependency(enumProperty.GetEnum().GetPackageObject());
			}
		}
		else if (info.Type == UEProperty::PropertyType::CustomStruct)
		{
			GeneratePrerequisites(prop.Cast<UEStructProperty>().GetStruct(), processedObjects);
		}
		else if (info.Type == UEProperty::PropertyType::Container)
		{
			std::vector<UEProperty> innerProperties;

			if (prop.IsA<UEArrayProperty>())
			{
				innerProperties.push_back(prop.Cast<UEArrayProperty>().GetInner());
			}
			else if (prop.IsA<UEMapProperty>())
			{
				auto mapProp = prop.Cast<UEMapProperty>();
				innerProperties.push_back(mapProp.GetKeyProperty());
				innerProperties.push_back(mapProp.GetValueProperty());
			}

			for (const auto& innerProp : from(innerProperties)
				>> where([](UEProperty && p) { return p.GetInfo().Type == UEProperty::PropertyType::CustomStruct; })
				>> experimental::container())
			{
				GeneratePrerequisites(innerProp.Cast<UEStructProperty>().GetStruct(), processedObjects);
			}
		}
		else if (prop.IsA<UEFunction>())
		{
			auto function = prop.Cast<UEFunction>();

			GenerateMemberPrerequisites(function.GetChildren().Cast<UEProperty>(), processedObjects);
		}
	}
}

void Package::GenerateScriptStruct(const UEScriptStruct& scriptStructObj)
{
	ScriptStruct ss;
	ss.Name = scriptStructObj.GetName();
	ss.FullName = scriptStructObj.GetFullName();

	{
		static std::string script_struct_format = std::string("Struct:  %-") + std::to_string(Utils::Settings.SdkGen.LoggerSpaceCount) + "s - instance: 0x%" PRIXPTR;
		std::string logStructName = Utils::Settings.SdkGen.LoggerShowStructSaveFileName ? this->GetName() + "." + ss.Name : ss.Name;
		Logger::Log(script_struct_format.c_str(), logStructName, scriptStructObj.GetAddress());
	}

	ss.NameCpp = MakeValidName(scriptStructObj.GetNameCpp());
	ss.NameCppFull = "struct ";

	//some classes need special alignment
	const auto alignment = Utils::GenObj->GetClassAlignas(ss.FullName);
	if (alignment != 0)
	{
		ss.NameCppFull += tfm::format("alignas(%d) ", alignment);
	}

	ss.NameCppFull += MakeUniqueCppName(scriptStructObj);

	ss.Size = scriptStructObj.GetPropertySize();
	ss.InheritedSize = 0;

	size_t offset = 0;

	auto super = scriptStructObj.GetSuper();
	if (super.IsValid() && super != scriptStructObj)
	{
		ss.InheritedSize = offset = super.GetPropertySize();

		ss.NameCppFull += " : public " + MakeUniqueCppName(super.Cast<UEScriptStruct>());
	}

	std::vector<UEProperty> properties;
	for (auto prop = scriptStructObj.GetChildren().Cast<UEProperty>(); prop.IsValid(); prop = prop.GetNext().Cast<UEProperty>())
	{
		if (prop.GetElementSize() > 0
			&& !prop.IsA<UEScriptStruct>()
			&& !prop.IsA<UEFunction>()
			&& !prop.IsA<UEEnum>()
			&& !prop.IsA<UEConst>()
			)
		{
			properties.push_back(prop);
		}
	}
	std::sort(std::begin(properties), std::end(properties), ComparePropertyLess);

	GenerateMembers(scriptStructObj, offset, properties, ss.Members);

	if (Utils::GenObj->GetSdkType() == SdkType::External)
	{
		ss.PredefinedMethods.push_back(PredefinedMethod::Inline(tfm::format(R"(	static %s ReadAsMe(uintptr_t address)
	{
		%s ret;
		%s(address, ret);
		return ret;
	})", scriptStructObj.GetNameCpp(), scriptStructObj.GetNameCpp(), Utils::Settings.SdkGen.MemoryRead)));

		ss.PredefinedMethods.push_back(PredefinedMethod::Inline(tfm::format(R"(	static %s WriteAsMe(const uintptr_t address, %s& toWrite)
	{
		return %s(address, toWrite);
	})", Utils::Settings.SdkGen.MemoryWriteType, scriptStructObj.GetNameCpp(), Utils::Settings.SdkGen.MemoryWrite)));
	}

	Utils::GenObj->GetPredefinedClassMethods(scriptStructObj.GetFullName(), ss.PredefinedMethods);

	ScriptStructs.emplace_back(std::move(ss));
}

void Package::GenerateEnum(const UEEnum& enumObj)
{
	Enum e;
	e.Name = MakeUniqueCppName(enumObj);

	if (e.Name.find("Default__") != std::string::npos
		|| e.Name.find("PLACEHOLDER-CLASS") != std::string::npos)
	{
		return;
	}

	e.FullName = enumObj.GetFullName();

	std::unordered_map<std::string, int> conflicts;
	for (auto&& s : enumObj.GetNames())
	{
		const auto clean = MakeValidName(std::move(s));

		const auto it = conflicts.find(clean);
		if (it == std::end(conflicts))
		{
			e.Values.push_back(clean);
			conflicts[clean] = 1;
		}
		else
		{
			e.Values.push_back(clean + tfm::format("%02d", it->second));
			conflicts[clean]++;
		}
	}

	Enums.emplace_back(std::move(e));
}

void Package::GenerateConst(const UEConst& constObj)
{
	//auto name = MakeValidName(constObj.GetName());
	const auto name = MakeUniqueCppName(constObj);

	if (name.find("Default__") != std::string::npos
		|| name.find("PLACEHOLDER-CLASS") != std::string::npos)
	{
		return;
	}

	// Constants[name] = Constant{ name, constObj.GetValue() };
	Constants.push_back({ name, constObj.GetValue() });
}

void Package::GenerateClass(const UEClass& classObj)
{
	Class c;
	c.Name = classObj.GetName();
	c.FullName = classObj.GetFullName();

	{
		static std::string class_format = std::string("Class:   %-") + std::to_string(Utils::Settings.SdkGen.LoggerSpaceCount) + "s - instance: 0x%" PRIXPTR;
		std::string logClassName = Utils::Settings.SdkGen.LoggerShowClassSaveFileName ? this->GetName() + "." + c.Name : c.Name;
		Logger::Log(class_format.c_str(), logClassName, classObj.GetAddress());
	}

	c.NameCpp = MakeValidName(classObj.GetNameCpp());
	c.NameCppFull = "class " + c.NameCpp;

	c.Size = classObj.GetPropertySize();
	c.InheritedSize = 0;

	size_t offset = 0;

	auto super = classObj.GetSuper();
	if (super.IsValid() && super != classObj)
	{
		c.InheritedSize = offset = super.GetPropertySize();
		c.NameCppFull += " : public " + MakeValidName(super.GetNameCpp());
	}

	std::vector<PredefinedMember> predefinedStaticMembers;
	if (Utils::GenObj->GetPredefinedClassStaticMembers(c.FullName, predefinedStaticMembers))
	{
		for (auto&& prop : predefinedStaticMembers)
		{
			Member p;
			p.Offset = 0;
			p.Size = 0;
			p.Name = prop.Name;
			p.Type = prop.Type;
			p.IsStatic = true;
			c.Members.push_back(std::move(p));
		}
	}

	std::vector<PredefinedMember> predefinedMembers;
	if (Utils::GenObj->GetPredefinedClassMembers(c.FullName, predefinedMembers))
	{
		for (auto&& prop : predefinedMembers)
		{
			Member p;
			p.Offset = 0;
			p.Size = 0;
			p.Name = prop.Name;
			p.Type = prop.Type;
			p.Comment = "NOT AUTO-GENERATED PROPERTY";
			c.Members.push_back(std::move(p));
		}
	}
	else
	{
		std::vector<UEProperty> properties;
		for (auto prop = classObj.GetChildren().Cast<UEProperty>(); prop.IsValid(); prop = prop.GetNext().Cast<UEProperty>())
		{
			if (prop.GetElementSize() > 0
				&& !prop.IsA<UEScriptStruct>()
				&& !prop.IsA<UEFunction>()
				&& !prop.IsA<UEEnum>()
				&& !prop.IsA<UEConst>()
				&& (!super.IsValid() || (super != classObj && prop.GetOffset() >= super.GetPropertySize()))
				)
			{
				properties.push_back(prop);
			}
		}
		std::sort(std::begin(properties), std::end(properties), ComparePropertyLess);

		GenerateMembers(classObj, offset, properties, c.Members);
	}

	Utils::GenObj->GetPredefinedClassMethods(c.FullName, c.PredefinedMethods);
	
	if (Utils::GenObj->GetSdkType() == SdkType::External)
	{
		c.PredefinedMethods.push_back(PredefinedMethod::Inline(tfm::format(R"(	static %s ReadAsMe(const uintptr_t address)
	{
		%s ret;
		%s(address, ret);
		return ret;
	})", classObj.GetNameCpp(), classObj.GetNameCpp(), Utils::Settings.SdkGen.MemoryRead)));
		c.PredefinedMethods.push_back(PredefinedMethod::Inline(tfm::format(R"(	static %s WriteAsMe(const uintptr_t address, %s& toWrite)
	{
		return %s(address, toWrite);
	})", Utils::Settings.SdkGen.MemoryWriteType, classObj.GetNameCpp(), Utils::Settings.SdkGen.MemoryWrite)));
	}
	else
	{
		if (Utils::GenObj->ShouldUseStrings())
		{
			c.PredefinedMethods.push_back(PredefinedMethod::Inline(tfm::format(R"(	static UClass* StaticClass()
	{
		static auto ptr = UObject::FindClass(%s);
		return ptr;
	})", Utils::GenObj->ShouldXorStrings() ? tfm::format("_xor_(\"%s\")", c.FullName) : tfm::format("\"%s\"", c.FullName))));
		}
		else
		{
			c.PredefinedMethods.push_back(PredefinedMethod::Inline(tfm::format(R"(	static UClass* StaticClass()
	{
		static auto ptr = UObject::GetObjectCasted<UClass>(%d);
		return ptr;
	})", classObj.GetIndex())));
		}

		GenerateMethods(classObj, c.Methods);

		//search virtual functions
		VirtualFunctionPatterns patterns;
		if (Utils::GenObj->GetVirtualFunctionPatterns(c.FullName, patterns))
		{
			size_t ptrSize = Utils::PointerSize();
			uintptr_t vTableAddress = classObj.Object->VfTable;
			std::vector<uintptr_t> vTable;

			size_t methodCount = 0;
			while (methodCount < 150)
			{
				MEMORY_BASIC_INFORMATION info;
				uintptr_t vAddress;

				// Dereference Pointer
				vAddress = Utils::MemoryObj->ReadAddress(vTableAddress + (methodCount * ptrSize));

				// Check valid address
				auto res = VirtualQueryEx(Utils::MemoryObj->ProcessHandle, reinterpret_cast<LPVOID>(vAddress), &info, sizeof info);
				if (res == NULL || info.Protect & PAGE_NOACCESS)
					break;

				vTable.push_back(vAddress);
				++methodCount;
			}

			for (auto&& pattern : patterns)
			{
				for (auto i = 0u; i < methodCount; ++i)
				{
					if (vTable[i] != NULL)
					{
						auto scanResult = PatternScan::FindPattern(Utils::MemoryObj, vTable[i], vTable[i] + 0x300, { std::get<0>(pattern) }, true);
						auto toFind = scanResult.find(std::get<0>(pattern).Name);
						if (toFind != scanResult.end() && !toFind->second.empty())
						{
							c.PredefinedMethods.push_back(PredefinedMethod::Inline(tfm::format(std::get<1>(pattern), i)));
							break;
						}
					}
				}
			}
		}
	}

	Classes.emplace_back(std::move(c));
}

Package::Member Package::CreatePadding(const size_t id, const size_t offset, const size_t size, std::string reason)
{
	Member ss;
	ss.Name = tfm::format("UnknownData%02d[0x%X]", id, size);
	ss.Type = "unsigned char";
	ss.Flags = 0;
	ss.Offset = offset;
	ss.Size = size;
	ss.Comment = std::move(reason);
	return ss;
}

Package::Member Package::CreateBitfieldPadding(const size_t id, const size_t offset, std::string type, const size_t bits)
{
	Member ss;
	ss.Name = tfm::format("UnknownData%02d : %d", id, bits);
	ss.Type = std::move(type);
	ss.Flags = 0;
	ss.Offset = offset;
	ss.Size = 1;
	return ss;
}

void Package::GenerateMembers(const UEStruct& structObj, size_t offset, const std::vector<UEProperty>& properties, std::vector<Member>& members) const
{
	std::unordered_map<std::string, size_t> uniqueMemberNames;
	size_t unknownDataCounter = 0;
	UEBoolProperty previousBitfieldProperty;

	for (auto&& prop : properties)
	{
		if (offset < prop.GetOffset())
		{
			previousBitfieldProperty = UEBoolProperty();

			const auto size = prop.GetOffset() - offset;
			members.emplace_back(CreatePadding(unknownDataCounter++, offset, size, "MISSED OFFSET"));
		}

		const auto info = prop.GetInfo();
		if (info.Type != UEProperty::PropertyType::Unknown)
		{
			Member sp;
			sp.Offset = prop.GetOffset();
			sp.Size = info.Size;

			sp.Type = info.CppType;
			sp.Name = MakeValidName(prop.GetName());

			const auto it = uniqueMemberNames.find(sp.Name);
			if (it == std::end(uniqueMemberNames))
			{
				uniqueMemberNames[sp.Name] = 1;
			}
			else
			{
				++uniqueMemberNames[sp.Name];
				sp.Name += tfm::format("%02d", it->second);
			}

			if (prop.GetArrayDim() > 1)
			{
				sp.Name += tfm::format("[0x%X]", prop.GetArrayDim());
			}

			if (prop.IsA<UEBoolProperty>() && prop.Cast<UEBoolProperty>().IsBitfield())
			{
				auto boolProp = prop.Cast<UEBoolProperty>();

				const auto missingBits = boolProp.GetMissingBitsCount(previousBitfieldProperty);
				if (missingBits[1] != -1)
				{
					if (missingBits[0] > 0)
					{
						members.emplace_back(CreateBitfieldPadding(unknownDataCounter++, previousBitfieldProperty.GetOffset(), info.CppType, missingBits[0]));
					}
					if (missingBits[1] > 0)
					{
						members.emplace_back(CreateBitfieldPadding(unknownDataCounter++, sp.Offset, info.CppType, missingBits[1]));
					}
				}
				else if (missingBits[0] > 0)
				{
					members.emplace_back(CreateBitfieldPadding(unknownDataCounter++, sp.Offset, info.CppType, missingBits[0]));
				}

				previousBitfieldProperty = boolProp;

				sp.Name += " : 1";
			}
			else
			{
				previousBitfieldProperty = UEBoolProperty();
			}

			sp.Name = Utils::GenObj->GetSafeKeywordsName(sp.Name);
			sp.Flags = static_cast<size_t>(prop.GetPropertyFlags());
			sp.FlagsString = StringifyFlags(prop.GetPropertyFlags());

			members.emplace_back(std::move(sp));

			const auto sizeMismatch = static_cast<int>(prop.GetElementSize() * prop.GetArrayDim()) - static_cast<int>(info.Size * prop.GetArrayDim());
			if (sizeMismatch > 0)
			{
				members.emplace_back(CreatePadding(unknownDataCounter++, offset, sizeMismatch, "FIX WRONG TYPE SIZE OF PREVIOUS PROPERTY"));
			}
		}
		else
		{
			const auto size = prop.GetElementSize() * prop.GetArrayDim();
			members.emplace_back(CreatePadding(unknownDataCounter++, offset, size, "UNKNOWN PROPERTY: " + prop.GetFullName()));
		}

		offset = prop.GetOffset() + prop.GetElementSize() * prop.GetArrayDim();
	}

	if (offset < structObj.GetPropertySize())
	{
		const auto size = structObj.GetPropertySize() - offset;
		members.emplace_back(CreatePadding(unknownDataCounter++, offset, size, "MISSED OFFSET"));
	}
}

void Package::GenerateMethods(const UEClass& classObj, std::vector<Method>& methods) const
{
	//some classes (AnimBlueprintGenerated...) have multiple members with the same name, so filter them out
	std::unordered_set<std::string> uniqueMethods;

	// prop can be UEObject, UEField, UEProperty
	for (auto prop = classObj.GetChildren().Cast<UEField>(); prop.IsValid(); prop = prop.GetNext())
	{
		if (!prop.IsA<UEFunction>())
			continue;

		auto function = prop.Cast<UEFunction>();

		Method m;
		m.Index = function.GetIndex();
		m.FullName = function.GetFullName();
		m.Name = MakeValidName(function.GetName());

			m.Name = Utils::GenObj->GetSafeKeywordsName(m.Name);
			if (uniqueMethods.find(m.FullName) != std::end(uniqueMethods))
				continue;

		uniqueMethods.insert(m.FullName);

		m.IsNative = function.GetFunctionFlags() & UEFunctionFlags::Native;
		m.IsStatic = function.GetFunctionFlags() & UEFunctionFlags::Static;
		m.FlagsString = StringifyFlags(function.GetFunctionFlags());

		std::vector<std::pair<UEProperty, Method::Parameter>> parameters;

		std::unordered_map<std::string, size_t> unique;
		for (auto param = function.GetChildren().Cast<UEProperty>(); param.IsValid(); param = param.GetNext().Cast<UEProperty>())
		{
			if (param.GetElementSize() == 0)
				continue;

			const auto info = param.GetInfo();
			if (info.Type != UEProperty::PropertyType::Unknown)
			{
				using Type = Method::Parameter::Type;

				Method::Parameter p;

				if (!Method::Parameter::MakeType(param.GetPropertyFlags(), p.ParamType))
				{
					//child isn't a parameter
					continue;
				}

				p.PassByReference = false;
				p.Name = MakeValidName(param.GetName());

				const auto it = unique.find(p.Name);
				if (it == std::end(unique))
				{
					unique[p.Name] = 1;
				}
				else
				{
					++unique[p.Name];

					p.Name += tfm::format("%02d", it->second);
				}

				p.FlagsString = StringifyFlags(param.GetPropertyFlags());

					p.CppType = info.CppType;
					if (param.IsA<UEBoolProperty>())
					{
						p.CppType = Utils::GenObj->GetOverrideType("bool");
					}

				if (p.ParamType == Type::Default)
				{
					if (param.GetArrayDim() > 1)
					{
						p.CppType = p.CppType + "*";
					}
					else if (info.CanBeReference)
					{
						p.PassByReference = true;
					}
				}

				p.Name = Utils::GenObj->GetSafeKeywordsName(p.Name);
				parameters.emplace_back(std::make_pair(param, std::move(p)));
			}
		}

		std::sort(parameters.begin(), parameters.end(), [](auto && lhs, auto && rhs) { return ComparePropertyLess(lhs.first, rhs.first); });

		for (auto& param : parameters)
			m.Parameters.emplace_back(std::move(param.second));

		methods.emplace_back(std::move(m));
	}
}

void Package::SaveStructs() const
{
	static auto uftLangSaveStructs = Utils::Dnc->GetFunction<void(_cdecl *)(NativePackage*)>("UftLangSaveStructs");
	if (!uftLangSaveStructs)
	{
		MessageBox(nullptr, "Can't Load Func `UftLangSaveStructs`", "ERROR", MB_OK | MB_ICONERROR);
		throw std::exception("Can't Load Func `UftLangSaveStructs`.");
	}

	NativePackage pack(*this);
	uftLangSaveStructs(&pack);
}

void Package::SaveClasses() const
{
	static auto uftLangSaveClasses = Utils::Dnc->GetFunction<void(_cdecl*)(NativePackage*)>("UftLangSaveClasses");
	if (!uftLangSaveClasses)
	{
		MessageBox(nullptr, "Can't Load Func `UftLangSaveClasses`", "ERROR", MB_OK | MB_ICONERROR);
		throw std::exception("Can't Load Func `UftLangSaveClasses`.");
	}

	NativePackage pack(*this);
	uftLangSaveClasses(&pack);
}

void Package::SaveFunctions() const
{
	static auto uftSaveFunctions = Utils::Dnc->GetFunction<void(_cdecl*)(NativePackage*)>("UftLangSaveFunctions");
	if (!uftSaveFunctions)
	{
		MessageBox(nullptr, "Can't Load Func `UftLangSaveFunctions`", "ERROR", MB_OK | MB_ICONERROR);
		throw std::exception("Can't Load Func `UftLangSaveFunctions`.");
	}

	NativePackage pack(*this);
	uftSaveFunctions(&pack);
}

void Package::SaveFunctionParameters() const
{
	static auto uftLangSaveFunctionParameters = Utils::Dnc->GetFunction<void(_cdecl*)(NativePackage*)>("UftLangSaveFunctionParameters");
	if (!uftLangSaveFunctionParameters)
	{
		MessageBox(nullptr, "Can't Load Func `UftLangSaveFunctionParameters`", "ERROR", MB_OK | MB_ICONERROR);
		throw std::exception("Can't Load Func `UftLangSaveFunctionParameters`.");
	}

	NativePackage pack(*this);
	uftLangSaveFunctionParameters(&pack);
}
