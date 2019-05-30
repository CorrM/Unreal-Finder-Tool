#include "pch.h"
#include "Memory.h"
#include "cpplinq.hpp"
#include "NameValidator.h"
#include "PrintHelper.h"
#include "Logger.h"
#include "ObjectsStore.h"
#include "NamesStore.h"
#include "SdkGenerator.h"

#include <fstream>
#include <chrono>
#include <bitset>
#include <unordered_set>
#include <tlhelp32.h>
#include "ParallelWorker.h"

extern IGenerator* generator;

// TODO: Optimize `UEObject::GetNameCPP` and `UEObject::IsA()` and `ObjectsStore().CountObjects` and `ObjectsStore::FindClass`
// TODO: Change Parallel from package loop to another loop like IsA or GetSuperClass 

SdkGenerator::SdkGenerator(const uintptr_t gObjAddress, const uintptr_t gNamesAddress) :
	gObjAddress(gObjAddress),
	gNamesAddress(gNamesAddress)
{
}

GeneratorState SdkGenerator::Start(int* pObjCount, int* pNamesCount, int* pPackagesCount, int* pPackagesDone, std::string gameVersion, const SdkType sdkType, std::string& state, std::vector<std::string>& packagesDone)
{
	// Check Address
	if (!Utils::IsValidGNamesAddress(gNamesAddress))
	{
		return GeneratorState::BadGName;
	}
	if (!Utils::IsValidGObjectsAddress(gObjAddress))
	{
		return GeneratorState::BadGObject;
	}

	// Dump GObjects
	if (!ObjectsStore::Initialize(gObjAddress))
	{
		return GeneratorState::BadGObject;
	}
	*pObjCount = ObjectsStore().GetObjectsNum();

	// Dump GNames
	if (!NamesStore::Initialize(gNamesAddress))
	{
		return GeneratorState::BadGName;
	}
	*pNamesCount = NamesStore().GetNamesNum();

	// Init Generator Settings
	if (!generator->Initialize())
	{
		MessageBoxA(nullptr, "Initialize failed", "Error", 0);
		return GeneratorState::Bad;
	}
	generator->SetGameVersion(gameVersion);
	generator->SetSdkType(sdkType);

	// Get Current Dir
	char buffer[2048];
	if (GetModuleFileNameA(GetModuleHandle(nullptr), buffer, sizeof(buffer)) == 0)
	{
		MessageBoxA(nullptr, "GetModuleFileName failed", "Error", 0);
		return GeneratorState::Bad;
	}
	fs::path outputDirectory = fs::path(buffer).remove_filename();

	outputDirectory /= "Results";
	fs::create_directories(outputDirectory);
	
	std::ofstream log(outputDirectory / "Generator.log");
	Logger::SetStream(&log);

	fs::create_directories(outputDirectory);

	// Dump To Files
	if (generator->ShouldDumpArrays())
	{
		Dump(outputDirectory);
		state = "Dump (GNames/GObjects) Done";
	}

	// Dump Packages
	const auto begin = std::chrono::system_clock::now();
	ProcessPackages(outputDirectory, pPackagesCount, pPackagesDone, state, packagesDone);
	
	Logger::Log("Finished, took %d seconds.", std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now() - begin).count());
	Logger::SetStream(nullptr);
	
	return GeneratorState::Good;
}

/// <summary>
/// Dumps the objects and names to files.
/// </summary>
/// <param name="path">The path where to create the dumps.</param>
void SdkGenerator::Dump(const fs::path& path)
{
	if (Utils::Settings.SdkGen.DumpObjects)
	{
		std::ofstream o(path / "ObjectsDump.txt");
		tfm::format(o, "Address: 0x%P\n\n", ObjectsStore::GetAddress());

		for (const auto& obj : ObjectsStore())
		{
			if (obj.IsValid())
				tfm::format(o, "[%06i] %-100s 0x%P\n", obj.GetIndex(), obj.GetFullName(), obj.GetAddress());
		}
	}

	if (Utils::Settings.SdkGen.DumpNames)
	{
		std::ofstream o(path / "NamesDump.txt");
		tfm::format(o, "Address: 0x%P\n\n", NamesStore::GetAddress());

		for (const auto& name : NamesStore())
		{
			tfm::format(o, "[%06i] %s\n", name.Index, name.AnsiName);
		}
	}
}

/// <summary>
/// Process the packages.
/// </summary>
/// <param name="path">The path where to create the package files.</param>
/// <param name="pPackagesCount"></param>
/// <param name="pPackagesDone"></param>
/// <param name="state"></param>
/// <param name="packagesDone"></param>
void SdkGenerator::ProcessPackages(const fs::path& path, int* pPackagesCount, int* pPackagesDone, std::string& state, std::vector<std::string>& packagesDone)
{
	using namespace cpplinq;

	int threadCount = Utils::Settings.SdkGen.Threads;
	const auto sdkPath = path / "SDK";
	fs::create_directories(sdkPath);

	std::vector<std::unique_ptr<Package>> packages;
	std::unordered_map<UEObject, bool> processedObjects;

	auto packageObjects =
		from(ObjectsStore())
		>> select([](auto && o) { return o.GetPackageObject(); })
		>> where([](auto && o) { return o.IsValid(); })
		>> distinct()
		>> to_vector();

	*pPackagesCount = packageObjects.size();

	/*
	 * First we must complete Core Package.
	 * it's contains all important stuff, (like we need it in 'StaticClass' function)
	 * so before go parallel we must get 'CoreUObject'
	 * it's the first package always (packageObjects[0])
	*/
	{
		state = "Dumping '" + Utils::Settings.SdkGen.CorePackageName + "'.";
		const UEObject& obj = packageObjects[0];

		auto package = std::make_unique<Package>(obj);
		package->Process(processedObjects);
		if (package->Save(sdkPath))
		{
			packagesDone.emplace_back(std::string("(") + std::to_string(1) + ") " + package->GetName() + " [ "
				"C: " + std::to_string(package->Classes.size()) + ", " +
				"S: " + std::to_string(package->ScriptStructs.size()) + ", " +
				"E: " + std::to_string(package->Enums.size()) + " ]"
			);

			Package::PackageMap[obj] = package.get();
			packages.emplace_back(std::move(package));
		}

		// Set Sleep Every
		Utils::Settings.Parallel.SleepEvery = 50;
	}

	state = "Dumping Packages with " + std::to_string(threadCount) + " Threads.";

	*pPackagesDone = 1;
	// Start From 1 because core package is already done
	ParallelWorker<UEObject> packageProcess(packageObjects, 1, threadCount, [&](const UEObject& obj, std::mutex & gMutex)
	{
		auto package = std::make_unique<Package>(obj);
		package->Process(processedObjects);
		if (package->Save(sdkPath))
		{
			{
				std::lock_guard lock(gMutex);
				packagesDone.emplace_back(std::string("(") + std::to_string(*pPackagesDone) + ") " + package->GetName() + " [ "
					"C: " + std::to_string(package->Classes.size()) + ", " +
					"S: " + std::to_string(package->ScriptStructs.size()) + ", " +
					"E: " + std::to_string(package->Enums.size()) + " ]"
				);
			}

			Package::PackageMap[obj] = package.get();
			packages.emplace_back(std::move(package));
		}

		{
			std::lock_guard lock(gMutex);
			++*pPackagesDone;
		}
	});
	packageProcess.Start();
	packageProcess.WaitAll();

	if (!packages.empty())
	{
		// std::sort doesn't work, so use a simple bubble sort
		//std::sort(std::begin(packages), std::end(packages), PackageDependencyComparer());
		const PackageDependencyComparer comparer;
		for (auto i = 0u; i < packages.size() - 1; ++i)
		{
			for (auto j = 0u; j < packages.size() - i - 1; ++j)
			{
				if (!comparer(packages[j], packages[j + 1]))
				{
					std::swap(packages[j], packages[j + 1]);
				}
			}
		}
	}

	SaveSdkHeader(path, processedObjects, packages);
}

/// <summary>
/// Generates the sdk header.
/// </summary>
/// <param name="path">The path where to create the sdk header.</param>
/// <param name="processedObjects">The list of processed objects.</param>
/// <param name="packages">The package order info.</param>
void SdkGenerator::SaveSdkHeader(const fs::path& path, const std::unordered_map<UEObject, bool>& processedObjects, const std::vector<std::unique_ptr<Package>>& packages)
{
	std::ofstream os(path / "SDK.hpp");
	os << "// ------------------------------------------------\n";
	os << "// Sdk Generated By ( Unreal Finder Tool By CorrM )\n";
	os << "// ------------------------------------------------\n";

	os << "#pragma once\n\n"
		<< tfm::format("// Name: %s, Version: %s\n\n", generator->GetGameName(), generator->GetGameVersion());

	//Includes
	os << "#include <set>\n";
	os << "#include <string>\n";
	for (auto&& i : generator->GetIncludes())
	{
		os << "#include " << i << "\n";
	}

	//include the basics
	{
		{
			std::ofstream os2(path / "SDK" / tfm::format("Basic.hpp"));

			PrintFileHeader(os2, true);

			os2 << generator->GetBasicDeclarations() << "\n";

			PrintFileFooter(os2);

			os << "\n#include \"SDK/" << tfm::format("Basic.hpp") << "\"\n";
		}
		{
			std::ofstream os2(path / "SDK" / tfm::format("Basic.cpp"));

			PrintFileHeader(os2, { "\"../SDK.hpp\"" }, false);

			os2 << generator->GetBasicDefinitions() << "\n";

			PrintFileFooter(os2);
		}
	}

	using namespace cpplinq;

	//check for missing structs
	const auto missing = from(processedObjects) >> where([](auto && kv) { return kv.second == false; });
	if (missing >> any())
	{
		std::ofstream os2(path / "SDK" / tfm::format("MISSING.hpp"));

		PrintFileHeader(os2, true);

		for (auto&& s : missing >> select([](auto && kv) { return kv.first.Cast<UEStruct>(); }) >> experimental::container())
		{
			os2 << "// " << s.GetFullName() << "\n// ";
			os2 << tfm::format("0x%04X\n", s.GetPropertySize());

			os2 << "struct " << MakeValidName(s.GetNameCPP()) << "\n{\n";
			os2 << "\tunsigned char UnknownData[0x" << tfm::format("%X", s.GetPropertySize()) << "];\n};\n\n";
		}

		PrintFileFooter(os2);

		os << "\n#include \"SDK/" << tfm::format("MISSING.hpp") << "\"\n";
	}

	os << "\n";

	for (auto&& package : packages)
	{
		os << R"(#include "SDK/)" << GenerateFileName(FileContentType::Structs, *package) << "\"\n";
		os << R"(#include "SDK/)" << GenerateFileName(FileContentType::Classes, *package) << "\"\n";
		if (generator->ShouldGenerateFunctionParametersFile())
		{
			os << R"(#include "SDK/)" << GenerateFileName(FileContentType::FunctionParameters, *package) << "\"\n";
		}
	}
}

uintptr_t SdkGenerator::GetModuleBase(const DWORD processId, const LPSTR lpModuleName, int* sizeOut)
{
	MODULEENTRY32 lpModuleEntry = { 0 };
	HANDLE hSnapShot = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE, processId);
	if (!hSnapShot)
		return NULL;
	lpModuleEntry.dwSize = sizeof(lpModuleEntry);
	BOOL bModule = Module32First(hSnapShot, &lpModuleEntry);
	while (bModule)
	{
		if (_stricmp(lpModuleEntry.szModule, lpModuleName) == 0)
		{
			CloseHandle(hSnapShot);
			*sizeOut = lpModuleEntry.modBaseSize;
			return reinterpret_cast<uintptr_t>(lpModuleEntry.modBaseAddr);
		}
		bModule = Module32Next(hSnapShot, &lpModuleEntry);
	}
	CloseHandle(hSnapShot);
	return NULL;
}