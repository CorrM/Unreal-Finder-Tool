#pragma once
#include "Package.h"

class SdkGenerator
{
	uintptr_t gObjAddress, gNamesAddress;
public:
	SdkGenerator(uintptr_t gObjAddress, uintptr_t gNamesAddress);
	void Start();
private:
	void ProcessPackages(const fs::path& path);
	uintptr_t GetModuleBase(DWORD processId, LPSTR lpModuleName, int* sizeOut);
	void Dump(const fs::path& path);
	void SaveSdkHeader(const fs::path& path, const std::unordered_map<UEObject, bool>& processedObjects, const std::vector<std::unique_ptr<Package>>& packages);
};