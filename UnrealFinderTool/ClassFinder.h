#pragma once
#include "ObjectsStore.h"

class ClassFinder
{
public:
	std::vector<uintptr_t> Find(uintptr_t gObjAddress, uintptr_t gNamesAddress, const std::string& objectType);

private:
	static std::vector<UEObject> FindThatObject(const std::string& typeName, bool firstOnly = false);
};
