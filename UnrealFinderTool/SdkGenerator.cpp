#include "pch.h"
#include "Memory.h"
#include "cpplinq.hpp"
#include "NameValidator.h"
#include "PrintHelper.h"
#include "Logger.h"
#include "ObjectsStore.h"
#include "NamesStore.h"
#include "JsonReflector.h"
#include "SdkGenerator.h"

#include <fstream>
#include <chrono>
#include <bitset>
#include <unordered_set>
#include <tlhelp32.h>
#include <bitset>
#include "ParallelWorker.h"

extern IGenerator* generator;

// TODO: Optimize `UEObject::GetNameCPP` and `UEObject::IsA()` and `ObjectsStore().CountObjects` and `ObjectsStore::FindClass`

SdkGenerator::SdkGenerator(const uintptr_t gObjAddress, const uintptr_t gNamesAddress) :
	gObjAddress(gObjAddress - 0x10),
	gNamesAddress(gNamesAddress)
{
}

void SdkGenerator::Start()
{
	std::cout << red << "[*] " << yellow << "Start Getting Objects : ";
	if (!ObjectsStore::Initialize(gObjAddress))
	{
		std::cout << red << "Failed To Get Objects." << std::endl;
		return;
	}
	std::cout << green << "Found [ " << red << std::setw(6) << ObjectsStore().GetObjectsNum() << green << " ]" << std::endl;

	std::cout << red << "[*] " << yellow << "Start Getting Names   : ";
	if (!NamesStore::Initialize(gNamesAddress))
	{
		std::cout << red << "Failed To Get Names." << std::endl;
		return;
	}
	std::cout << green << "Found [ " << red << std::setw(6) << NamesStore().GetNamesNum() << green << " ]" << std::endl;

	if (!generator->Initialize())
	{
		MessageBoxA(nullptr, "Initialize failed", "Error", 0);
		return;
	}

	char buffer[2048];
	if (GetModuleFileNameA(GetModuleHandle(nullptr), buffer, sizeof(buffer)) == 0)
	{
		MessageBoxA(nullptr, "GetModuleFileName failed", "Error", 0);
		return;
	}
	fs::path outputDirectory = fs::path(buffer).remove_filename();

	outputDirectory /= "Results";
	fs::create_directories(outputDirectory);
	
	std::ofstream log(outputDirectory / "Generator.log");
	Logger::SetStream(&log);

	
	if (generator->ShouldDumpArrays())
		Dump(outputDirectory);
	
	fs::create_directories(outputDirectory);
	
	const auto begin = std::chrono::system_clock::now();
	ProcessPackages(outputDirectory);
	
	Logger::Log("Finished, took %d seconds.", std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now() - begin).count());
	Logger::SetStream(nullptr);
	
	MessageBoxA(nullptr, "Finished!", "Info", 0);
}

/// <summary>
/// Dumps the objects and names to files.
/// </summary>
/// <param name="path">The path where to create the dumps.</param>
void SdkGenerator::Dump(const fs::path& path)
{
	if (Utils::Settings.SdkGen.DumpObjects)
	{
		std::cout << red << "[*] " << yellow << "Start Dumping Objects : ";
		std::ofstream o(path / "ObjectsDump.txt");
		tfm::format(o, "Address: 0x%P\n\n", ObjectsStore::GetAddress());

		for (const auto& obj : ObjectsStore())
		{
			if (obj.IsValid())
				tfm::format(o, "[%06i] %-100s 0x%P\n", obj.GetIndex(), obj.GetFullName(), obj.GetAddress());
		}
		std::cout << green << "Saved in [ " << red << "'ObjectsDump.txt'." << green << " ]" << std::endl;
	}

	if (Utils::Settings.SdkGen.DumpNames)
	{
		std::cout << red << "[*] " << yellow << "Start Dumping Names    : ";
		std::ofstream o(path / "NamesDump.txt");
		tfm::format(o, "Address: 0x%P\n\n", NamesStore::GetAddress());

		for (const auto& name : NamesStore())
		{
			tfm::format(o, "[%06i] %s\n", name.Index, name.AnsiName);
		}
		std::cout << green << "Saved in [ " << red << "'NamesDump.txt'." << green << " ]" << std::endl;
	}
}

/// <summary>
/// Process the packages.
/// </summary>
/// <param name="path">The path where to create the package files.</param>
void SdkGenerator::ProcessPackages(const fs::path& path)
{
	using namespace cpplinq;

	const auto sdkPath = path / "SDK";
	fs::create_directories(sdkPath);

	std::vector<std::unique_ptr<Package>> packages;
	std::unordered_map<UEObject, bool> processedObjects;

	std::cout << red << "[*] " << yellow << "Start Calc Packages   : ";

	auto packageObjects =
		from(ObjectsStore())
		>> select([](auto && o) { return o.GetPackageObject(); })
		>> where([](auto && o) { return o.IsValid(); })
		>> distinct()
		>> to_vector();

	std::cout << green << "Found [ " << red << std::setw(6) << packageObjects.size() << green << " ]" << std::endl;
	std::cout << def << "===================================" << std::endl;

	int counter = 1;
	int threadCount = Utils::Settings.SdkGen.Threads;
	//omp_set_dynamic(false);

	/*
	 * First we must complete Core Package.
	 * it's contains all important stuff, (like we need it in 'StaticClass' function)
	 * so before go parallel we must get 'CoreUObject'
	 * it's the first package always (packageObjects[0])
	*/
	{
		std::cout << red << "[*] " << yellow << "Dumping '" << Utils::Settings.SdkGen.CorePackageName << "'" << red << ". That's may take while." << std::endl;
		const UEObject& obj = packageObjects[0];

		auto package = std::make_unique<Package>(obj);
		package->Process(processedObjects);
		if (package->Save(sdkPath))
		{
			std::cout << green << "[+] " << "(" << std::setfill('0') << std::setw(4) << 0 << ") " << std::setfill(' ') <<
				purple << std::setw(50) << std::left << package->GetName() << std::right << yellow << " [ " << std::setfill('0') <<
				"C: " << lblue << std::setw(4) << package->Classes.size() << yellow << ", " <<
				"S: " << lblue << std::setw(4) << package->ScriptStructs.size() << yellow << ", " <<
				"E: " << lblue << std::setw(4) << package->Enums.size() << yellow << " ]" << std::setfill(' ') << std::endl;

			Package::PackageMap[obj] = package.get();
			packages.emplace_back(std::move(package));
		}

		// Set Sleep Every
		Utils::Settings.Parallel.SleepEvery = 50;
	}

	std::cout << red << "[*] " << yellow << "Dumping Packages with " << red << threadCount << yellow << " Threads." << std::endl;


	// Start From 1 because core package is already done
	ParallelWorker<UEObject> packageProcess(packageObjects, 1, threadCount, [&](const UEObject& obj, std::mutex & gMutex)
	{
		auto package = std::make_unique<Package>(obj);
		package->Process(processedObjects);
		if (package->Save(sdkPath))
		{
			{
				std::lock_guard lock(gMutex);
				std::cout << green << "[+] " << "(" << std::setfill('0') << std::setw(4) << counter << ") " << std::setfill(' ') <<
					purple << std::setw(50) << std::left << package->GetName() << std::right << yellow << " [ " << std::setfill('0') <<
					"C: " << lblue << std::setw(4) << package->Classes.size() << yellow << ", " <<
					"S: " << lblue << std::setw(4) << package->ScriptStructs.size() << yellow << ", " <<
					"E: " << lblue << std::setw(4) << package->Enums.size() << yellow << " ]" << std::setfill(' ') << std::endl;
			}

			Package::PackageMap[obj] = package.get();
			packages.emplace_back(std::move(package));
		}

		{
			std::lock_guard lock(gMutex);
			++counter;
		}
	});
	packageProcess.Start();
	packageProcess.WaitAll();

	std::cout << std::endl;

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

	os << "#pragma once\n\n"
		<< tfm::format("// %s SDK\n\n", generator->GetGameName());

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