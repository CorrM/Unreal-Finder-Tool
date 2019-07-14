#pragma once
#include "Package.h"

enum class GeneratorState
{
	Bad,
	Good,
	BadGObject,
	BadGName,
	BadSdkLang
};

struct SdkInfo
{
	GeneratorState State;
	std::tm TookTime;
};

struct StartInfo
{
	size_t* PObjCount		= nullptr;
	size_t* PNamesCount		= nullptr;
	size_t* PPackagesCount	= nullptr;
	size_t* PPackagesDone	= nullptr;

	std::string GameName;
	std::string GameVersion;
	std::string SdkLang;
	std::string GameModule;

	SdkType TargetSdkType = SdkType::None;
	std::vector<std::string>* PackagesDone = nullptr;

	std::string* State	= nullptr;
};

class SdkGenerator
{
	uintptr_t gObjAddress, gNamesAddress;
public:
	SdkGenerator(uintptr_t gObjAddress, uintptr_t gNamesAddress);
	SdkInfo Start(StartInfo& startInfo);
	bool InitSdkLang(const std::string& sdkPath, const std::string& langPath) const;
private:
	/// <summary>
	/// Dumps the objects and names to files.
	/// </summary>
	/// <param name="path">The path where to create the dumps.</param>
	/// <param name="state"></param>
	void Dump(const fs::path& path, std::string& state) const;

	void CollectPackages(std::vector<UEObject*>& packageObjects, std::string& state);

	/// <summary>
	/// Process the packages.
	/// </summary>
	/// <param name="path">The path where to create the package files.</param>
	/// <param name="pPackagesCount"></param>
	/// <param name="pPackagesDone"></param>
	/// <param name="state"></param>
	/// <param name="packagesDone"></param>
	void ProcessPackages(const fs::path& path, size_t* pPackagesCount, size_t* pPackagesDone, std::string& state, std::vector<std::string>& packagesDone);

	void SdkAfterFinish(const std::unordered_map<uintptr_t, bool>& processedObjects, const std::vector<std::unique_ptr<Package>>& packages) const;
};