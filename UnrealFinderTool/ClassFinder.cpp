#include "pch.h"
#include "NamesStore.h"
#include "ClassFinder.h"
#include <sstream>

std::vector<std::string> ClassFinder::Find(const uintptr_t gObjAddress, const uintptr_t gNamesAddress, const std::string& objectType)
{
	std::vector<std::string> ret;

	// Dump GNames
	NamesStore::Initialize(gNamesAddress, false);

	// Dump GObjects
	ObjectsStore::Initialize(gObjAddress, false);

	std::string formattedStr = objectType;

	if (formattedStr._Starts_with("0x"))
		formattedStr.erase(0, 2);

	if (Utils::IsHexNumber(formattedStr))
	{
		auto matches = FindThatObjectByAddress(formattedStr, false);
		std::for_each(matches.begin(), matches.end(), [&ret](const UEObject& obj)
		{
			ret.push_back(obj.GetInstanceClassName());
		});
	}
	else
	{
		auto matches = FindThatObject(formattedStr, false);
		std::stringstream ss;
		std::for_each(matches.begin(), matches.end(), [&](const UEObject& obj)
		{
			// Get address, convert to hex string, change chars to upper
			ss << std::hex << obj.GetAddress();
			std::string tmpUpper = ss.str();
			std::transform(tmpUpper.begin(), tmpUpper.end(), tmpUpper.begin(), toupper);
			tmpUpper = "0x" + tmpUpper + " [ " + obj.GetInstanceClassName() + " ]";

			// Clear
			ss.str("");
			ss.clear();

			ret.push_back(tmpUpper);
		});
	}

	return ret;
}

std::vector<UEObject> ClassFinder::FindThatObject(const std::string& typeName, const bool firstOnly)
{
	std::vector<UEObject> ret;

	for (size_t i = 0; i < ObjectsStore().GetObjectsNum(); ++i)
	{
		const UEObject* object = ObjectsStore().GetByIndex(i);

		if (object->IsA(typeName))
		{
			ret.push_back(*object);
			if (firstOnly) break;
		}
	}
	return ret;
}

std::vector<UEObject> ClassFinder::FindThatObjectByAddress(const std::string& typeName, const bool firstOnly)
{
	std::vector<UEObject> ret;
	uintptr_t instanceAddress = Utils::CharArrayToUintptr(typeName);

	for (size_t i = 0; i < ObjectsStore().GetObjectsNum(); ++i)
	{
		const UEObject* object = ObjectsStore().GetByIndex(i);

		if (object->GetAddress() == instanceAddress)
		{
			ret.push_back(*object);
			if (firstOnly) break;
		}
	}
	return ret;
}
