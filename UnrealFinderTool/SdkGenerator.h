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
					const std::string& gameName, const std::string& gameVersion, SdkType sdkType,
					std::string& state, std::vector<std::string>& packagesDone);
private:
	void Dump(const fs::path& path);
	void ProcessPackages(const fs::path& path, size_t* pPackagesCount, size_t* pPackagesDone, std::string& state, std::vector<std::string>& packagesDone);
	void SaveSdkHeader(const fs::path& path, const std::unordered_map<uintptr_t, bool>& processedObjects, const std::vector<std::unique_ptr<Package>>& packages);
};