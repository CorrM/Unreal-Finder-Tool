#include "pch.h"
#include "ObjectsStore.h"
#include "../MainEngineClasses.h"
#include "Utils.h"
#include "Memory.h"
#include <cassert>

SdkGObject* ObjectsStore::gObjObjects;
Memory* ObjectsStore::mem;
int ObjectsStore::gObjectsCount;
uintptr_t ObjectsStore::gObjAddress;

#pragma region ObjectsStore
bool ObjectsStore::Initialize(Memory* mem, const uintptr_t gObjAddress)
{
	ObjectsStore::mem = mem;
	ObjectsStore::gObjAddress = gObjAddress;

	return true;
}

bool ObjectsStore::LoadCore()
{
	// Read core GObjects
	if (!JsonReflector::ReadJsonFile("Config\\Core\\GObjects.json", &jsonCore))
	{
		std::cout << red << "[*] " << def << "Can't read GObject file." << std::endl << def;
		return false;
	}
	// Load all json structs in file
	if (!JsonReflector::Load(&jsonCore))
	{
		std::cout << red << "[*] " << def << "Can't load GObject file." << std::endl << def;
		return false;
	}

	// Read core GNames
	if (!JsonReflector::ReadJsonFile("Config\\Core\\GNames.json", &jsonCore))
	{
		std::cout << red << "[*] " << def << "Can't read GNames file." << std::endl << def;
		return false;
	}
	// Load all json structs in file
	if (!JsonReflector::Load(&jsonCore))
	{
		std::cout << red << "[*] " << def << "Can't load GNames file." << std::endl << def;
		return false;
	}

	return true;
}

bool ObjectsStore::FetchData()
{
	// GObjects
	JsonStruct jsonGObjectsReader;
	if (!JsonReflector::Read("FUObjectArray", jsonGObjectsReader))
	{
		std::cout << red << "[*] " << def << "Can't find struct `FUObjectArray`." << std::endl << def;
		return false;
	}
	if (!ReadUObjectArray(gObjAddress, jsonGObjectsReader))
	{
		std::cout << red << "[*] " << def << "Invalid GObject Address." << std::endl << def;
		return false;
	}

	return true;
}

bool ObjectsStore::ReadUObjectArray(const uintptr_t address, JsonStruct& objectArray)
{
	const size_t sSub = mem->Is64Bit && Utils::ProgramIs64() ? 0x0 : 0x4;
	JsonStruct fUObjectItem, uObject;

	if (!JsonReflector::Read("FUObjectItem", fUObjectItem))
	{
		std::cout << red << "[*] " << def << "Can't find struct `FUObjectItem`." << std::endl << def;
		return false;
	}

	if (!JsonReflector::Read("UObject", uObject))
	{
		std::cout << red << "[*] " << def << "Can't find struct `UObject`." << std::endl << def;
		return false;
	}

	if (mem->ReadBytes(address, objectArray.AllocPointer, objectArray.StructSize - sSub) == 0) return false;

	auto objObjects = objectArray["ObjObjects"].ReadAsStruct();
	gObjectsCount = objObjects["NumElements"].ReadAs<int>();

	uintptr_t dwFUObjectAddress = Utils::ReadSafePointer(objObjects.AllocPointer, 0, objObjects.StructSize, mem->Is64Bit);

	// Alloc all objects
	gObjObjects = new SdkGObject[gObjectsCount];
	ZeroMemory(gObjObjects, sizeof(SdkGObject) * gObjectsCount);

	uintptr_t currentFUObjAddress = dwFUObjectAddress;
	for (int i = 0; i < gObjectsCount; ++i)
	{
		// Read the address as struct
		if (mem->ReadBytes(currentFUObjAddress, fUObjectItem.AllocPointer, fUObjectItem.StructSize - sSub) == 0)
			return false;

		// Convert class pointer to address
		uintptr_t dwUObject = Utils::ReadSafePointer(fUObjectItem.AllocPointer, 0, fUObjectItem.StructSize, mem->Is64Bit);

		// Skip null pointer in GObjects array
		if (dwUObject == 0)
		{
			currentFUObjAddress += fUObjectItem.StructSize - (sSub * 2);
			gObjectsCount++;
			continue;
		}

		// Alloc and Read the address as class
		if (mem->ReadBytes(dwUObject, uObject.AllocPointer, uObject.StructSize - sSub) == 0)
			return false;

		// Fix UObject
		if (!mem->Is64Bit)
		{
			if (Utils::ProgramIs64())
			{
				// Fix VfTable
				Utils::FixStructPointer(uObject.AllocPointer, 0, uObject.StructSize);
			}
		}

		gObjObjects[i].ObjAddress = dwUObject;
		gObjObjects[i].FNameIndex = uObject["FNameIndex"].ReadAs<int>();

		currentFUObjAddress += fUObjectItem.StructSize - (sSub * 2);
		gObjectsCount++;
	}
	return true;
}

void* ObjectsStore::GetAddress()
{
	return reinterpret_cast<void*>(gObjAddress);
}

size_t ObjectsStore::GetObjectsNum() const
{
	return gObjectsCount;
}

UEObject ObjectsStore::GetById(size_t id) const
{
	return GlobalObjects->ObjObjects.Objects[id].Object;
}

UEClass ObjectsStore::FindClass(const std::string& name) const
{
	for (auto obj : *this)
	{
		if (obj.GetFullName() == name)
		{
			return obj.Cast<UEClass>();
		}
	}
	return UEClass(nullptr);
}
#pragma endregion

#pragma region ObjectsIterator
ObjectsIterator ObjectsStore::begin()
{
	return ObjectsIterator(*this, 0);
}

ObjectsIterator ObjectsStore::begin() const
{
	return ObjectsIterator(*this, 0);
}

ObjectsIterator ObjectsStore::end()
{
	return ObjectsIterator(*this);
}

ObjectsIterator ObjectsStore::end() const
{
	return ObjectsIterator(*this);
}

ObjectsIterator::ObjectsIterator(const ObjectsStore& _store)
	: store(_store),
	  index(_store.GetObjectsNum())
{
}

ObjectsIterator::ObjectsIterator(const ObjectsStore& _store, size_t _index)
	: store(_store),
	  index(_index),
	  current(_store.GetById(_index))
{
}

ObjectsIterator::ObjectsIterator(const ObjectsIterator& other)
	: store(other.store),
	  index(other.index),
	  current(other.current)
{
}

ObjectsIterator::ObjectsIterator(ObjectsIterator&& other) noexcept
	: store(other.store),
	  index(other.index),
	  current(other.current)
{
}

ObjectsIterator& ObjectsIterator::operator=(const ObjectsIterator& rhs)
{
	index = rhs.index;
	current = rhs.current;
	return *this;
}

void ObjectsIterator::swap(ObjectsIterator& other) noexcept
{
	std::swap(index, other.index);
	std::swap(current, other.current);
}

ObjectsIterator& ObjectsIterator::operator++()
{
	for (++index; index < store.GetObjectsNum(); ++index)
	{
		current = store.GetById(index);
		if (current.IsValid())
		{
			break;
		}
	}
	return *this;
}

ObjectsIterator ObjectsIterator::operator++(int)
{
	auto tmp(*this);
	++(*this);
	return tmp;
}

bool ObjectsIterator::operator==(const ObjectsIterator& rhs) const
{
	return index == rhs.index;
}

bool ObjectsIterator::operator!=(const ObjectsIterator& rhs) const
{
	return index != rhs.index;
}

UEObject ObjectsIterator::operator*() const
{
	assert(current.IsValid() && "ObjectsIterator::current is not valid!");

	return current;
}

UEObject ObjectsIterator::operator->() const
{
	return operator*();
}
#pragma endregion