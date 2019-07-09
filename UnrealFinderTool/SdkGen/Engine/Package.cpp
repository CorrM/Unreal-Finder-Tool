#include "pch.h"

#include <fstream>
#include <unordered_set>
#include <cinttypes>

#include "tinyformat.h"
#include "cpplinq.hpp"
#include "IGenerator.h"
#include "Logger.h"
#include "NameValidator.h"
#include "PatternScan.h"
#include "ObjectsStore.h"
#include "PropertyFlags.h"
#include "FunctionFlags.h"
#include "PrintHelper.h"
#include "ParallelWorker.h"
#include "Package.h"

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

		ParallelQueue<GObjects, ObjectItem>
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

	for (auto& obj : objsInPack)
	{
		if (obj->IsA<UEEnum>())
		{
			GenerateEnum(obj->Cast<UEEnum>());
		}
		/* in UE4 there is no UEConst
		else if (obj.IsA<UEConst>())
		{
			GenerateConst(obj.Cast<UEConst>());
		}
		*/
		else if (obj->IsA<UEClass>())
		{
			GeneratePrerequisites(*obj, processedObjects);
		}
		else if (obj->IsA<UEScriptStruct>())
		{
			GeneratePrerequisites(*obj, processedObjects);
		}

		Utils::SleepEvery(1, process_sleep_counter, Utils::Settings.Parallel.SleepEvery);
	}
}

bool Package::Save(const fs::path& path) const
{
	extern IGenerator* generator;

	using namespace cpplinq;

	//check if package is empty (no enums, structs or classes without members)
	if (generator->ShouldGenerateEmptyFiles()
		|| (from(Enums) >> where([](auto && e) { return !e.Values.empty(); }) >> any()
			|| from(ScriptStructs) >> where([](auto && s) { return !s.Members.empty() || !s.PredefinedMethods.empty(); }) >> any()
			|| from(Classes) >> where([](auto && c) {return !c.Members.empty() || !c.PredefinedMethods.empty() || !c.Methods.empty(); }) >> any()
			)
		)
	{
		SaveStructs(path);
		SaveClasses(path);
		SaveFunctions(path);

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
	if (package != packageObj)
	{
		dependencies.insert(*package);

		return true;
	}
	return false;
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
	extern IGenerator* generator;

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
	const auto alignment = generator->GetClassAlignas(ss.FullName);
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

	if (generator->GetSdkType() == SdkType::External)
	{
		ss.PredefinedMethods.push_back(IGenerator::PredefinedMethod::Inline(tfm::format(R"(	static %s ReadAsMe(uintptr_t address)
	{
		%s ret;
		%s(address, ret);
		return ret;
	})", scriptStructObj.GetNameCpp(), scriptStructObj.GetNameCpp(), Utils::Settings.SdkGen.MemoryRead)));

		ss.PredefinedMethods.push_back(IGenerator::PredefinedMethod::Inline(tfm::format(R"(	static %s WriteAsMe(const uintptr_t address, %s& toWrite)
	{
		return %s(address, toWrite);
	})", Utils::Settings.SdkGen.MemoryWriteType, scriptStructObj.GetNameCpp(), Utils::Settings.SdkGen.MemoryWrite)));
	}

	generator->GetPredefinedClassMethods(scriptStructObj.GetFullName(), ss.PredefinedMethods);

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
	auto name = MakeUniqueCppName(constObj);

	if (name.find("Default__") != std::string::npos
		|| name.find("PLACEHOLDER-CLASS") != std::string::npos)
	{
		return;
	}

	constants[name] = constObj.GetValue();
}

void Package::GenerateClass(const UEClass& classObj)
{
	extern IGenerator* generator;

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

	std::vector<IGenerator::PredefinedMember> predefinedStaticMembers;
	if (generator->GetPredefinedClassStaticMembers(c.FullName, predefinedStaticMembers))
	{
		for (auto&& prop : predefinedStaticMembers)
		{
			Member p;
			p.Offset = 0;
			p.Size = 0;
			p.Name = prop.Name;
			p.Type = "static " + prop.Type;
			c.Members.push_back(std::move(p));
		}
	}

	std::vector<IGenerator::PredefinedMember> predefinedMembers;
	if (generator->GetPredefinedClassMembers(c.FullName, predefinedMembers))
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

	generator->GetPredefinedClassMethods(c.FullName, c.PredefinedMethods);
	
	if (generator->GetSdkType() == SdkType::External)
	{
		c.PredefinedMethods.push_back(IGenerator::PredefinedMethod::Inline(tfm::format(R"(	static %s ReadAsMe(const uintptr_t address)
	{
		%s ret;
		%s(address, ret);
		return ret;
	})", classObj.GetNameCpp(), classObj.GetNameCpp(), Utils::Settings.SdkGen.MemoryRead)));
		c.PredefinedMethods.push_back(IGenerator::PredefinedMethod::Inline(tfm::format(R"(	static %s WriteAsMe(const uintptr_t address, %s& toWrite)
	{
		return %s(address, toWrite);
	})", Utils::Settings.SdkGen.MemoryWriteType, classObj.GetNameCpp(), Utils::Settings.SdkGen.MemoryWrite)));
	}
	else
	{
		if (generator->ShouldUseStrings())
		{
			c.PredefinedMethods.push_back(IGenerator::PredefinedMethod::Inline(tfm::format(R"(	static UClass* StaticClass()
	{
		static auto ptr = UObject::FindClass(%s);
		return ptr;
	})", generator->ShouldXorStrings() ? tfm::format("_xor_(\"%s\")", c.FullName) : tfm::format("\"%s\"", c.FullName))));
		}
		else
		{
			c.PredefinedMethods.push_back(IGenerator::PredefinedMethod::Inline(tfm::format(R"(	static UClass* StaticClass()
	{
		static auto ptr = UObject::GetObjectCasted<UClass>(%d);
		return ptr;
	})", classObj.GetIndex())));
		}

		GenerateMethods(classObj, c.Methods);

		//search virtual functions
		IGenerator::VirtualFunctionPatterns patterns;
		if (generator->GetVirtualFunctionPatterns(c.FullName, patterns))
		{
			int ptrSize = Utils::PointerSize();
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
							c.PredefinedMethods.push_back(IGenerator::PredefinedMethod::Inline(tfm::format(std::get<1>(pattern), i)));
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
	extern IGenerator* generator;

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

			sp.Name = generator->GetSafeKeywordsName(sp.Name);
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
	extern IGenerator* generator;

	//some classes (AnimBlueprintGenerated...) have multiple members with the same name, so filter them out
	std::unordered_set<std::string> uniqueMethods;

	for (auto prop = classObj.GetChildren().Cast<UEField>(); prop.IsValid(); prop = prop.GetNext())
	{
		if (!prop.IsA<UEFunction>())
			continue;

		auto function = prop.Cast<UEFunction>();

		Method m;
		m.Index = function.GetIndex();
		m.FullName = function.GetFullName();
		m.Name = MakeValidName(function.GetName());

		m.Name = generator->GetSafeKeywordsName(m.Name);
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
					p.CppType = generator->GetOverrideType("bool");
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

				p.Name = generator->GetSafeKeywordsName(p.Name);

				parameters.emplace_back(std::make_pair(param, std::move(p)));
			}
		}

		std::sort(std::begin(parameters), std::end(parameters), [](auto && lhs, auto && rhs) { return ComparePropertyLess(lhs.first, rhs.first); });

		for (auto& param : parameters)
		{
			m.Parameters.emplace_back(std::move(param.second));
		}

		methods.emplace_back(std::move(m));
	}
}

void Package::SaveStructs(const fs::path & path) const
{
	extern IGenerator* generator;

	std::ofstream os(path / GenerateFileName(FileContentType::Structs, *this));

	PrintFileHeader(os, true);

	if (!constants.empty())
	{
		PrintSectionHeader(os, "Constants");
		for (auto&& c : constants) { PrintConstant(os, c); }

		os << "\n";
	}

	if (!Enums.empty())
	{
		PrintSectionHeader(os, "Enums");
		for (auto&& e : Enums) { PrintEnum(os, e); os << "\n"; }

		os << "\n";
	}

	if (!ScriptStructs.empty())
	{
		PrintSectionHeader(os, "Script Structs");
		for (auto&& s : ScriptStructs) { PrintStruct(os, s); os << "\n"; }
	}

	PrintFileFooter(os);
}

void Package::SaveClasses(const fs::path& path) const
{
	extern IGenerator* generator;

	std::ofstream os(path / GenerateFileName(FileContentType::Classes, *this));

	PrintFileHeader(os, true);

	if (!Classes.empty())
	{
		PrintSectionHeader(os, "Classes");
		for (auto&& c : Classes) { PrintClass(os, c); os << "\n"; }
	}

	PrintFileFooter(os);
}

void Package::SaveFunctions(const fs::path & path) const
{
	extern IGenerator* generator;
	using namespace cpplinq;

	// Skip Functions if it's external
	if (generator->GetSdkType() == SdkType::External)
		return;

	if (generator->ShouldGenerateFunctionParametersFile())
		SaveFunctionParameters(path);

	std::ofstream os(path / GenerateFileName(FileContentType::Functions, *this));

	PrintFileHeader(os, { "\"../SDK.h\"" }, false);

	PrintSectionHeader(os, "Functions");

	for (auto&& s : ScriptStructs)
	{
		for (auto&& m : s.PredefinedMethods)
		{
			if (m.MethodType != IGenerator::PredefinedMethod::Type::Inline)
			{
				os << m.Body << "\n\n";
			}
		}
	}

	for (auto&& c : Classes)
	{
		for (auto&& m : c.PredefinedMethods)
		{
			if (m.MethodType != IGenerator::PredefinedMethod::Type::Inline)
			{
				os << m.Body << "\n\n";
			}
		}

		for (auto&& m : c.Methods)
		{
			//Method Info
			os << "// " << m.FullName << "\n"
				<< "// (" << m.FlagsString << ")\n";
			if (!m.Parameters.empty())
			{
				os << "// Parameters:\n";
				for (auto&& param : m.Parameters)
				{
					tfm::format(os, "// %-30s %-30s (%s)\n", param.CppType, param.Name, param.FlagsString);
				}
			}

			os << "\n";
			os << BuildMethodSignature(m, c, false) << "\n";
			os << BuildMethodBody(c, m) << "\n\n";
		}
	}

	PrintFileFooter(os);
}

void Package::SaveFunctionParameters(const fs::path & path) const
{
	using namespace cpplinq;

	std::ofstream os(path / GenerateFileName(FileContentType::FunctionParameters, *this));

	PrintFileHeader(os, { "\"../SDK.h\"" }, true);

	PrintSectionHeader(os, "Parameters");

	for (auto&& c : Classes)
	{
		for (auto&& m : c.Methods)
		{
			os << "// " << m.FullName << "\n";
			tfm::format(os, "struct %s_%s_Params\n{\n", c.NameCpp, m.Name);
			for (auto&& param : m.Parameters)
			{
				tfm::format(os, "\t%-50s %-58s// (%s)\n", param.CppType, param.Name + ";", param.FlagsString);
			}
			os << "};\n\n";
		}
	}

	PrintFileFooter(os);
}

void Package::PrintConstant(std::ostream & os, const std::pair<std::string, std::string> & c) const
{
	tfm::format(os, "#define CONST_%-50s %s\n", c.first, c.second);
}

void Package::PrintEnum(std::ostream & os, const Enum & e) const
{
	using namespace cpplinq;

	os << "// " << e.FullName << "\nenum class " << e.Name << " : uint8_t\n{\n";
	os << (from(e.Values)
		>> select([](auto && name, auto && i) { return tfm::format("\t%-30s = %d", name, i); })
		>> concatenate(",\n"))
		<< "\n};\n\n";
}

void Package::PrintStruct(std::ostream & os, const ScriptStruct & ss) const
{
	using namespace cpplinq;

	os << "// " << ss.FullName << "\n// ";
	if (ss.InheritedSize)
	{
		os << tfm::format("0x%04X (0x%04X - 0x%04X)\n", ss.Size - ss.InheritedSize, ss.Size, ss.InheritedSize);
	}
	else
	{
		os << tfm::format("0x%04X\n", ss.Size);
	}

	os << ss.NameCppFull << "\n{\n";

	//Member
	os << (from(ss.Members)
		>> select([](auto && m) {
			return tfm::format("\t%-50s %-58s// 0x%04X(0x%04X)", m.Type, m.Name + ";", m.Offset, m.Size)
				+ (!m.Comment.empty() ? " " + m.Comment : "")
				+ (!m.FlagsString.empty() ? " (" + m.FlagsString + ")" : "");
			})
		>> concatenate("\n"))
		<< "\n";

			//Predefined Methods
			if (!ss.PredefinedMethods.empty())
			{
				os << "\n";
				for (auto&& m : ss.PredefinedMethods)
				{
					if (m.MethodType == IGenerator::PredefinedMethod::Type::Inline)
					{
						os << m.Body;
					}
					else
					{
						os << "\t" << m.Signature << ";";
					}
					os << "\n\n";
				}
			}

			os << "};\n";
}

void Package::PrintClass(std::ostream & os, const Class & c) const
{
	using namespace cpplinq;

	os << "// " << c.FullName << "\n// ";
	if (c.InheritedSize)
	{
		tfm::format(os, "0x%04X (0x%04X - 0x%04X)\n", c.Size - c.InheritedSize, c.Size, c.InheritedSize);
	}
	else
	{
		tfm::format(os, "0x%04X\n", c.Size);
	}

	os << c.NameCppFull << "\n{\npublic:\n";

	//Member
	for (auto&& m : c.Members)
	{
		tfm::format(os, "\t%-50s %-58s// 0x%04X(0x%04X)", m.Type, m.Name + ";", m.Offset, m.Size);
		if (!m.Comment.empty())
		{
			os << " " << m.Comment;
		}
		if (!m.FlagsString.empty())
		{
			os << " (" << m.FlagsString << ")";
		}
		os << "\n";
	}

	//Predefined Methods
	if (!c.PredefinedMethods.empty())
	{
		os << "\n";
		for (auto&& m : c.PredefinedMethods)
		{
			if (m.MethodType == IGenerator::PredefinedMethod::Type::Inline)
			{
				os << m.Body;
			}
			else
			{
				os << "\t" << m.Signature << ";";
			}

			os << "\n\n";
		}
	}

	//Methods
	if (!c.Methods.empty())
	{
		os << "\n";
		for (auto&& m : c.Methods)
		{
			os << "\t" << BuildMethodSignature(m, {}, true) << ";\n";
		}
	}

	os << "};\n\n";
}

std::string Package::BuildMethodSignature(const Method & m, const Class & c, bool inHeader) const
{
	extern IGenerator* generator;

	using namespace cpplinq;
	using Type = Method::Parameter::Type;

	std::ostringstream ss;

	if (m.IsStatic && inHeader && !generator->ShouldConvertStaticMethods())
	{
		ss << "static ";
	}

	//Return Type
	auto retn = from(m.Parameters) >> where([](auto && param) { return param.ParamType == Type::Return; });
	if (retn >> any())
	{
		ss << (retn >> first()).CppType;
	}
	else
	{
		ss << "void";
	}
	ss << " ";

	if (!inHeader)
	{
		ss << c.NameCpp << "::";
	}
	if (m.IsStatic && generator->ShouldConvertStaticMethods())
	{
		ss << "STATIC_";
	}
	ss << m.Name;

	//Parameters
	ss << "(";
	ss << (from(m.Parameters)
		>> where([](auto && param) { return param.ParamType != Type::Return; })
		>> orderby([](auto && param) { return param.ParamType; })
		>> select([](auto && param) { return (param.PassByReference ? "const " : "") + param.CppType + (param.PassByReference ? "& " : param.ParamType == Type::Out ? "* " : " ") + param.Name; })
		>> concatenate(", "));
	ss << ")";

	return ss.str();
}

std::string Package::BuildMethodBody(const Class & c, const Method & m) const
{
	extern IGenerator* generator;

	using namespace cpplinq;
	using Type = Method::Parameter::Type;

	std::ostringstream ss;

	//Function Pointer
	ss << "{\n\tstatic auto fn";

	if (generator->ShouldUseStrings())
	{
		ss << " = UObject::FindObject<UFunction>(";

		if (generator->ShouldXorStrings())
		{
			ss << "_xor_(\"" << m.FullName << "\")";
		}
		else
		{
			ss << "\"" << m.FullName << "\"";
		}

		ss << ");\n\n";
	}
	else
	{
		ss << " = UObject::GetObjectCasted<UFunction>(" << m.Index << ");\n\n";
	}

	//Parameters
	if (generator->ShouldGenerateFunctionParametersFile())
	{
		ss << "\t" << c.NameCpp << "_" << m.Name << "_Params params;\n";
	}
	else
	{
		ss << "\tstruct\n\t{\n";
		for (auto&& param : m.Parameters)
		{
			tfm::format(ss, "\t\t%-30s %s;\n", param.CppType, param.Name);
		}
		ss << "\t} params;\n";
	}

	auto defaultParameters = from(m.Parameters) >> where([](auto && param) { return param.ParamType == Type::Default; });
	if (defaultParameters >> any())
	{
		for (auto&& param : defaultParameters >> experimental::container())
		{
			ss << "\tparams." << param.Name << " = " << param.Name << ";\n";
		}
	}

	ss << "\n";

	//Function Call
	ss << "\tauto flags = fn->FunctionFlags;\n";
	if (m.IsNative)
	{
		ss << "\tfn->FunctionFlags |= 0x" << tfm::format("%X", static_cast<std::underlying_type_t<UEFunctionFlags>>(UEFunctionFlags::Native)) << ";\n";
	}

	ss << "\n";

	if (m.IsStatic && !generator->ShouldConvertStaticMethods())
	{
		ss << "\tstatic auto defaultObj = StaticClass()->CreateDefaultObject();\n";
		ss << "\tdefaultObj->ProcessEvent(fn, &params);\n\n";
	}
	else
	{
		ss << "\tUObject::ProcessEvent(fn, &params);\n\n";
	}

	ss << "\tfn->FunctionFlags = flags;\n";

	//Out Parameters
	auto out = from(m.Parameters) >> where([](auto && param) { return param.ParamType == Type::Out; });
	if (out >> any())
	{
		ss << "\n";

		for (auto&& param : out >> experimental::container())
		{
			ss << "\tif (" << param.Name << " != nullptr)\n";
			ss << "\t\t*" << param.Name << " = params." << param.Name << ";\n";
		}
	}

	//Return Value
	auto retn = from(m.Parameters) >> where([](auto && param) { return param.ParamType == Type::Return; });
	if (retn >> any())
	{
		ss << "\n\treturn params." << (retn >> first()).Name << ";\n";
	}

	ss << "}\n";

	return ss.str();
}