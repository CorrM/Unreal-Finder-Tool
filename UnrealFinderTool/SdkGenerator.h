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

class SdkGenerator
{
	uintptr_t gObjAddress, gNamesAddress;
public:
	SdkGenerator(uintptr_t gObjAddress, uintptr_t gNamesAddress);
	SdkInfo Start(size_t* pObjCount, size_t* pNamesCount, size_t* pPackagesCount, size_t* pPackagesDone,
	              const std::string& gameName, const std::string& gameVersion, const SdkType sdkType,
	              std::string& state, std::vector<std::string>& packagesDone, const std::string& sdkLang);
	static bool InitSdkLang(const std::string& sdkPath, const std::string& langPath);
private:
	/// <summary>
	/// Dumps the objects and names to files.
	/// </summary>
	/// <param name="path">The path where to create the dumps.</param>
	/// <param name="state"></param>
	void Dump(const fs::path& path, std::string& state) const;
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