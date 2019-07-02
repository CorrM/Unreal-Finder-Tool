#pragma once
#include "Package.h"

enum class GeneratorState
{
	Bad,
	Good,
	BadGObject,
	BadGName
};

struct SdkInfo
{
	GeneratorState State;
	std::tm TookTime;
};

class SdkGenerator
{
	uintptr_t gObjAddress, gNamesAddress;
public:
	SdkGenerator(uintptr_t gObjAddress, uintptr_t gNamesAddress);
	SdkInfo Start(size_t* pObjCount, size_t* pNamesCount, size_t* pPackagesCount, size_t* pPackagesDone,
	              const std::string& gameName, const std::string& gameVersion, const SdkType sdkType,
	              std::string& state, std::vector<std::string>& packagesDone, const std::string& sdkLang);
	bool InitSdkLang(const std::string& sdkPath, const std::string& langPath);
private:
	/// <summary>
	/// Dumps the objects and names to files.
	/// </summary>
	/// <param name="path">The path where to create the dumps.</param>
	/// <param name="state"></param>
	void Dump(const fs::path& path, std::string& state);
	/// <summary>
	/// Process the packages.
	/// </summary>
	/// <param name="path">The path where to create the package files.</param>
	/// <param name="pPackagesCount"></param>
	/// <param name="pPackagesDone"></param>
	/// <param name="state"></param>
	/// <param name="packagesDone"></param>
	void ProcessPackages(const fs::path& path, size_t* pPackagesCount, size_t* pPackagesDone, std::string& state, std::vector<std::string>& packagesDone);
	/// <summary>
	/// Generates the sdk header.
	/// </summary>
	/// <param name="path">The path where to create the sdk header.</param>
	/// <param name="processedObjects">The list of processed objects.</param>
	/// <param name="packages">The package order info.</param>
	void SaveSdkHeader(const fs::path& path, const std::unordered_map<uintptr_t, bool>& processedObjects, const std::vector<std::unique_ptr<Package>>& packages);
};