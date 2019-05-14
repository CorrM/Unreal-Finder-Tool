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
#include <bitset>
#include "JsonTypes.h"

extern IGenerator* generator;

SdkGenerator::SdkGenerator(Memory* memory, const uintptr_t gObjAddress, const uintptr_t gNamesAddress)
	: _memory(memory),
	gObjAddress(gObjAddress),
	gNamesAddress(gNamesAddress)
{
}

void SdkGenerator::Start()
{
	std::cout << red << "[*] " << green << "Start Getting Objects." << std::endl;
	if (!ObjectsStore::Initialize(_memory, gObjAddress))
	{
		std::cout << red << "[*] " << def << "Failed To Get Objects." << std::endl << def;
		return;
	}
	std::cout << green << "[+] " << yellow << "Getting Objects Successfully. Found [ " << red << ObjectsStore().GetObjectsNum() << yellow << " ]" << std::endl;

	std::cout << red << "[*] " << green << "Start Getting Names." << std::endl;
	if (!NamesStore::Initialize(_memory, gNamesAddress))
	{
		std::cout << red << "[*] " << def << "Failed To Get Names." << std::endl << def;
		return;
	}
	std::cout << green << "[+] " << yellow << "Getting Names Successfully.   Found [ " << red << NamesStore().GetNamesNum() << yellow << " ]" << std::endl;

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
	
	outputDirectory /= generator->GetGameNameShort();
	fs::create_directories(outputDirectory);
	
	std::ofstream log(outputDirectory / "Generator.log");
	Logger::SetStream(&log);

	
	if (generator->ShouldDumpArrays())
	{
		Dump(outputDirectory);
	}
	
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
	{
		std::ofstream o(path / "ObjectsDump.txt");
		tfm::format(o, "Address: 0x%P\n\n", ObjectsStore::GetAddress());

		for (const auto& obj : ObjectsStore())
		{
			if (obj.IsValid())
				tfm::format(o, "[%06i] %-100s 0x%P\n", obj.GetIndex(), obj.GetFullName(), obj.GetAddress());
		}
	}

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
void SdkGenerator::ProcessPackages(const fs::path& path)
{
	using namespace cpplinq;

	const auto sdkPath = path / "SDK";
	fs::create_directories(sdkPath);

	std::vector<std::unique_ptr<Package>> packages;
	std::unordered_map<UEObject, bool> processedObjects;

	auto packageObjects = from(ObjectsStore())
		>> select([](auto && o) { return o.GetPackageObject(); })
		>> where([](auto && o) { return o.IsValid(); })
		>> distinct()
		>> to_vector();

	for (const auto& obj : packageObjects)
	{
		auto package = std::make_unique<Package>(obj);

		package->Process(processedObjects);
		if (package->Save(sdkPath))
		{
			Package::PackageMap[obj] = package.get();

			packages.emplace_back(std::move(package));
		}
	}

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
		<< tfm::format("// %s (%s) SDK\n\n", generator->GetGameName(), generator->GetGameVersion());

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
			std::ofstream os2(path / "SDK" / tfm::format("%s_Basic.hpp", generator->GetGameNameShort()));

			PrintFileHeader(os2, true);

			os2 << generator->GetBasicDeclarations() << "\n";

			PrintFileFooter(os2);

			os << "\n#include \"SDK/" << tfm::format("%s_Basic.hpp", generator->GetGameNameShort()) << "\"\n";
		}
		{
			std::ofstream os2(path / "SDK" / tfm::format("%s_Basic.cpp", generator->GetGameNameShort()));

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
		std::ofstream os2(path / "SDK" / tfm::format("%s_MISSING.hpp", generator->GetGameNameShort()));

		PrintFileHeader(os2, true);

		for (auto&& s : missing >> select([](auto && kv) { return kv.first.Cast<UEStruct>(); }) >> experimental::container())
		{
			os2 << "// " << s.GetFullName() << "\n// ";
			os2 << tfm::format("0x%04X\n", s.GetPropertySize());

			os2 << "struct " << MakeValidName(s.GetNameCPP()) << "\n{\n";
			os2 << "\tunsigned char UnknownData[0x" << tfm::format("%X", s.GetPropertySize()) << "];\n};\n\n";
		}

		PrintFileFooter(os2);

		os << "\n#include \"SDK/" << tfm::format("%s_MISSING.hpp", generator->GetGameNameShort()) << "\"\n";
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