#pragma once
#include "Package.hpp"

class SdkGenerator
{
	Memory* _memory;
	uintptr_t gObjAddress, gNamesAddress;
public:
	SdkGenerator(Memory* memory, uintptr_t gObjAddress, uintptr_t gNamesAddress);
	void Start();
private:
	void ProcessPackages(const fs::path& path);
	DWORD GetModuleBase(DWORD processId, LPSTR lpModuleName, int* sizeOut);
	void Dump(const fs::path& path);
	void SaveSdkHeader(const fs::path& path, const std::unordered_map<UEObject, bool>& processedObjects, const std::vector<std::unique_ptr<Package>>& packages);
};

