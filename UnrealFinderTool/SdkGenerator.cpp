#include "pch.h"
#include "cpplinq.hpp"
#include "Logger.h"
#include "ObjectsStore.h"
#include "NamesStore.h"
#include "ParallelWorker.h"
#include "Native.h"
#include "SdkGenerator.h"
#include "DotNetConnect.h"

// ToDo: Instated of crash just popup a msg that's tell the user to use another GObjects address

SdkGenerator::SdkGenerator(const uintptr_t gObjAddress, const uintptr_t gNamesAddress) :
	gObjAddress(gObjAddress),
	gNamesAddress(gNamesAddress)
{
}

SdkInfo SdkGenerator::Start(size_t* pObjCount, size_t* pNamesCount, size_t* pPackagesCount, size_t* pPackagesDone,
								   const std::string& gameName, const std::string& gameVersion, const SdkType sdkType,
								   std::string& state, std::vector<std::string>& packagesDone, const std::string& sdkLang)
{
	// Check Address
	if (!Utils::IsValidGNamesAddress(gNamesAddress))
		return { GeneratorState::BadGName };
	if (!Utils::IsValidGObjectsAddress(gObjAddress))
		return { GeneratorState::BadGObject };

	// Dump GNames
	if (!NamesStore::Initialize(gNamesAddress))
		return { GeneratorState::BadGName };
	*pNamesCount = NamesStore().GetNamesNum();

	// Dump GObjects
	if (!ObjectsStore::Initialize(gObjAddress))
		return { GeneratorState::BadGObject };
	*pObjCount = ObjectsStore().GetObjectsNum();

	// Init Generator Settings
	if (!Utils::GenObj->Initialize())
	{
		MessageBoxA(nullptr, "Initialize failed", "Error", 0);
		return { GeneratorState::Bad };
	}
	Utils::GenObj->SetGameName(gameName);
	Utils::GenObj->SetGameVersion(gameVersion);
	Utils::GenObj->SetSdkType(sdkType);
	Utils::GenObj->SetIsGObjectsChunks(ObjectsStore::GInfo.IsChunksAddress);
	Utils::GenObj->SetSdkLang(sdkLang);

	// Get Current Dir
	fs::path outputDirectory = fs::path(Utils::GetWorkingDirectoryA());

	outputDirectory /= "Results";
	fs::create_directories(outputDirectory);
	
	std::ofstream log(outputDirectory / "Generator.log");
	Logger::SetStream(&log);

	fs::create_directories(outputDirectory);

	// Init SdkLang
	if (!InitSdkLang((outputDirectory / "SDK").string(), Utils::GetWorkingDirectoryA() + R"(\Config\Langs\)" + Utils::GenObj->GetSdkLang()))
		return { GeneratorState::BadSdkLang };

	// Dump To Files
	if (Utils::GenObj->ShouldDumpArrays())
	{
		state = "Dumping (GNames/GObjects).";
		Dump(outputDirectory, state);
		state = "Dump (GNames/GObjects) Done.";
		Sleep(2 * 1000);
	}

	// Dump Packages
	const auto begin = std::chrono::system_clock::now();
	ProcessPackages(outputDirectory, pPackagesCount, pPackagesDone, state, packagesDone);

	// Get Time
	std::time_t took_seconds = std::time_t(std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now() - begin).count());
	std::tm took_time;
	gmtime_s(&took_time, &took_seconds);

	Logger::Log("Finished, took %d seconds.", took_seconds);
	Logger::SetStream(nullptr);

	return { GeneratorState::Good, took_time };
}

bool SdkGenerator::InitSdkLang(const std::string& sdkPath, const std::string& langPath)
{
	if (!Utils::Dnc->Load(Utils::GetWorkingDirectoryW() + L"\\" VER_BIT_STR "\\SdkLang.dll"))
	{
		MessageBox(nullptr, "Can't Load `SdkLang.dll`.", "ERROR", MB_OK | MB_ICONERROR);
		return false;
	}

	const auto sdkLangInit = Utils::Dnc->GetFunction<bool(__cdecl*)(NativeGenInfo*)>("UftLangInit");
	if (!sdkLangInit)
	{
		MessageBox(nullptr, "Can't Load Function `SdkLangInit`.", "ERROR", MB_OK | MB_ICONERROR);
		return false;
	}

	// Init Struct
	NativeGenInfo genInfo
	(
		Utils::GetExePathA(),
		sdkPath,
		langPath,
		Utils::GenObj->GetSdkLang(),
		Utils::GenObj->GetGameName(),
		Utils::GenObj->GetGameVersion(),
		Utils::GenObj->GetNamespaceName(),
		Utils::GenObj->GetGlobalMemberAlignment(),
		Utils::PointerSize(),
		Utils::GenObj->GetSdkType() == SdkType::External,
		Utils::GenObj->GetIsGObjectsChunks(),
		Utils::GenObj->ShouldConvertStaticMethods(),
		Utils::GenObj->ShouldUseStrings(),
		Utils::GenObj->ShouldXorStrings(),
		Utils::GenObj->ShouldGenerateFunctionParametersFile()
	);

	return sdkLangInit(&genInfo);
}

void SdkGenerator::Dump(const fs::path& path, std::string& state) const
{
	if (Utils::Settings.SdkGen.DumpNames)
	{
		state = "Dumping Names.";

		std::ofstream o(path / "NamesDump.txt");
		tfm::format(o, "Address: 0x%" PRIXPTR "\n\n", NamesStore::GetAddress());

		const size_t vecSize = NamesStore().GetNamesNum();
		for (size_t i = 0; i < vecSize; ++i)
		{
			std::string str = NamesStore().GetByIndex(i);
			if (!str.empty())
				tfm::format(o, "[%06i] %s\n", int(i), NamesStore().GetByIndex(i).c_str());
			state = "Names [ " + std::to_string(i) + " / " + std::to_string(vecSize) + " ].";
		}
	}

	if (Utils::Settings.SdkGen.DumpObjects)
	{
		state = "Dumping Objects.";

		std::ofstream o(path / "ObjectsDump.txt");
		tfm::format(o, "Address: 0x%" PRIXPTR "\n\n", ObjectsStore::GInfo.GObjAddress);

		const size_t vecSize = ObjectsStore().GetObjectsNum();
		for (size_t i = 0; i < vecSize; ++i)
		{
			if (ObjectsStore().GetByIndex(i)->IsValid())
			{
				const UEObject* obj = ObjectsStore().GetByIndex(i);
				tfm::format(o, "[%06i] %-100s 0x%" PRIXPTR "\n", obj->GetIndex(), obj->GetFullName(), obj->GetAddress());
			}
			state = "Objects [ " + std::to_string(i) + " / " + std::to_string(vecSize) + " ].";
		}
	}
}

void SdkGenerator::ProcessPackages(const fs::path& path, size_t* pPackagesCount, size_t* pPackagesDone, std::string& state, std::vector<std::string>& packagesDone)
{
	int threadCount = Utils::Settings.SdkGen.Threads;
	const auto sdkPath = path / "SDK";
	fs::create_directories(sdkPath);
	
	std::vector<std::unique_ptr<Package>> packages;
	std::unordered_map<uintptr_t, bool> processedObjects;

	state = "Start Collecting Packages.";
	std::vector<UEObject*> packageObjects;

	// Collecting Packages
	{
		size_t index = 0;
		ParallelSingleShot worker(threadCount, [&](ParallelOptions& options)
		{
			const size_t vecSize = ObjectsStore::GObjObjects.size();
			while (index < vecSize)
			{
				UEObject* curObj;
				// Get current object
				{
					std::lock_guard lock(options.Locker);
					curObj = ObjectsStore::GObjObjects[index].second.get();
					++index;
				}

				if (!curObj)
					continue;

				UEObject* package = curObj->GetPackageObject();
				if (package->IsValid())
				{
					std::lock_guard lock(options.Locker);
					packageObjects.push_back(package);
					state = "Progress [ " + std::to_string(index) + " / " + std::to_string(vecSize) + " ].";
				}
			}
		});
		worker.Start();
		worker.WaitAll();

		// Make vector distinct
		std::sort(packageObjects.begin(), packageObjects.end());
		packageObjects.erase(std::unique(packageObjects.begin(), packageObjects.end()), packageObjects.end());
	}

	state = "Getting Packages Done.";
	*pPackagesCount = packageObjects.size();

	/*
	 * First we must complete Core Package.
	 * it's contains all important stuff, (like we need it in 'StaticClass' function)
	 * so before go parallel we must get 'CoreUObject'
	 * it's the first package always (packageObjects[0])
	*/
	{
		state = "Dumping '" + Utils::Settings.SdkGen.CorePackageName + "'.";

		// Get CoreUObject, Some times CoreUObject not the first Package
		UEObject* coreUObject;
		size_t coreUObjectIndex = 0;
		for (size_t i = 0; i < packageObjects.size(); ++i)
		{
			auto pack = packageObjects[i];

			if(pack->GetName() == Utils::Settings.SdkGen.CorePackageName)
			{
				coreUObject = pack;
				coreUObjectIndex = i;
				break;
			}
		}

		// Process CoreUObject
		auto package = std::make_unique<Package>(coreUObject);
		std::mutex tmp_lock;
		package->Process(processedObjects, tmp_lock);
		if (package->Save())
		{
			packagesDone.emplace_back(std::string("(") + std::to_string(1) + ") " + package->GetName() + " [ " +
				"C: " + std::to_string(package->Classes.size()) + ", " +
				"S: " + std::to_string(package->ScriptStructs.size()) + ", " +
				"E: " + std::to_string(package->Enums.size()) + " ]"
			);

			Package::PackageMap[*coreUObject] = package.get();
			packages.emplace_back(std::move(package));
		}

		// Set Sleep Every
		Utils::Settings.Parallel.SleepEvery = 30;

		// Remove CoreUObject Package to not dump it twice
		packageObjects.erase(packageObjects.begin() + coreUObjectIndex);
	}

	++*pPackagesDone;
	state = "Dumping with " + std::to_string(threadCount) + " Threads.";

	// Start From 1 because core package is already done
	ParallelQueue<std::vector<UEObject*>, UEObject*>packageProcess(packageObjects, 0, threadCount, [&](UEObject* obj, ParallelOptions& options)
	{
		auto package = std::make_unique<Package>(obj);
		package->Process(processedObjects, Utils::MainMutex);
		
		std::lock_guard lock(Utils::MainMutex);
		++*pPackagesDone;

		packagesDone.emplace_back(std::string("(") + std::to_string(*pPackagesDone) + ") " + package->GetName() + " [ " +
			"C: " + std::to_string(package->Classes.size()) + ", " +
			"S: " + std::to_string(package->ScriptStructs.size()) + ", " +
			"E: " + std::to_string(package->Enums.size()) + " ]"
		);

		if (package->Save())
		{
			Package::PackageMap[*obj] = package.get();
			packages.emplace_back(std::move(package));
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

	SdkAfterFinish(processedObjects, packages);
}

void SdkGenerator::SdkAfterFinish(const std::unordered_map<uintptr_t, bool>& processedObjects, const std::vector<std::unique_ptr<Package>>& packages) const
{
	const auto uftLangSdkAfterFinish = Utils::Dnc->GetFunction<void(_cdecl*)(StructArray<NativePackage>, StructArray<NativeUStruct>)>("UftLangSdkAfterFinish");
	if (!uftLangSdkAfterFinish)
	{
		MessageBox(nullptr, "Can't Load Func `UftLangSdkAfterFinish`", "ERROR", MB_OK | MB_ICONERROR);
		throw std::exception("Can't Load Func `UftLangSdkAfterFinish`.");
	}

	// Init Packages
	StructArray<NativePackage> packagesToPush;
	std::vector<Package> packagesList;
	for (auto& pack : packages)
		packagesList.push_back(*pack);
	NativeHelper::HeapNativeStructArray(packagesList, packagesToPush);

	// Init Missing
	StructArray<NativeUStruct> missingToPush;
	std::vector<UEStruct> missingList;

	using namespace cpplinq;
	//check for missing structs
	const auto missing = from(processedObjects) >> where([](std::pair<uintptr_t, bool>&& kv) { return !kv.second; });
	if (missing >> any())
	{
		auto missed = missing >> select([](std::pair<uintptr_t, bool> && kv) { return ObjectsStore::GetByAddress(kv.first)->Cast<UEStruct>(); }) >> experimental::container();
		for (UEStruct&& s : missed)
			missingList.push_back(s);
	}
	NativeHelper::HeapNativeStructArray(missingList, missingToPush);

	// Call Function
	uftLangSdkAfterFinish(packagesToPush, missingToPush);

	// Free
	NativeHelper::FreeNativeStructArray(packagesToPush);
	NativeHelper::FreeNativeStructArray(missingToPush);
}
