#pragma once
#include "Package.h"

enum class GeneratorState
{
	Bad,
	Good,
	BadGObject,
	BadGName
};


class SdkGenerator
{
	uintptr_t gObjAddress, gNamesAddress;
public:
	SdkGenerator(uintptr_t gObjAddress, uintptr_t gNamesAddress);
	GeneratorState Start(int* pObjCount, int* pNamesCount, int* pPackagesCount, int* pPackagesDone, std::string& state, std::vector<std::string>& packagesDone);
private:
	uintptr_t GetModuleBase(DWORD processId, LPSTR lpModuleName, int* sizeOut);
	void Dump(const fs::path& path);
	void ProcessPackages(const fs::path& path, int* pPackagesCount, int* pPackagesDone, std::string& state, std::vector<std::string>&
	                     packagesDone);
	void SaveSdkHeader(const fs::path& path, const std::unordered_map<UEObject, bool>& processedObjects, const std::vector<std::unique_ptr<Package>>& packages);
};