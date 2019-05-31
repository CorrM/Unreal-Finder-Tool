#include "pch.h"
#include "UWorldFinder.h"
#include "ObjectsStore.h"
#include "NamesStore.h"

std::vector<uintptr_t> UWorldFinder::Find(const uintptr_t gObjAddress, const uintptr_t gNamesAddress)
{
	std::vector<uintptr_t> ret;

	// Dump GObjects
	ObjectsStore::Initialize(gObjAddress, false);

	// Dump GNames
	NamesStore::Initialize(gNamesAddress, false);

	for (size_t i = 0; i < ObjectsStore().GetObjectsNum(); ++i)
	{
		const UEObject& obj = ObjectsStore().GetById(i);
		if (obj.GetName() == "World")
		{
			auto worldAddress = reinterpret_cast<uintptr_t>(obj.GetAddress());

			// Not Contains the same address before
			if (std::find(ret.begin(), ret.end(), worldAddress) == ret.end())
				ret.push_back(worldAddress);
		}
	}

	return ret;
}
