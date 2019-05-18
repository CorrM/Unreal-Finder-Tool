#include "pch.h"
#include "Utils.h"
#include "Memory.h"
#include "../MainEngineClasses.h"
#include "ObjectsStore.h"
#include <cassert>

SdkUObject* ObjectsStore::GObjObjects;
Memory* ObjectsStore::mem;
int ObjectsStore::GObjectsCount;
uintptr_t ObjectsStore::gObjAddress;

#pragma region ObjectsStore
bool ObjectsStore::Initialize(Memory* mem, const uintptr_t gObjAddress)
{
	ObjectsStore::mem = mem;
	ObjectsStore::gObjAddress = gObjAddress;

	return FetchData();
}

bool ObjectsStore::FetchData()
{
	// GObjects
	JsonStruct jsonGObjectsReader;
	if (!ReadUObjectArray(gObjAddress, jsonGObjectsReader))
	{
		std::cout << red << "[*] " << def << "Invalid GObject Address." << std::endl << def;
		return false;
	}

	return true;
}

bool ObjectsStore::ReadUObjectArray(const uintptr_t address, JsonStruct& objectArray)
{
	JsonStruct fUObjectItem, uObject;
	const size_t sSub = mem->Is64Bit && Utils::ProgramIs64() ? 0x0 : 0x4;
	
	if (!objectArray.ReadData(address, "FUObjectArray")) return false;
	auto objObjects = *objectArray["ObjObjects"].ReadAsStruct();
	int num = objObjects["NumElements"].ReadAs<int>();

	GObjObjects = new SdkUObject[num];
	auto currentFUObjAddress = objObjects["Objects"].ReadAs<uintptr_t>();
	for (int i = 0; i < num; ++i)
	{
		// Read the address as struct
		if (!fUObjectItem.ReadData(currentFUObjAddress, "FUObjectItem")) return false;

		// Convert class pointer to address
		auto dwUObject = fUObjectItem["Object"].ReadAs<uintptr_t>();

		// Skip null pointer in GObjects array
		if (dwUObject == 0)
		{
			currentFUObjAddress += fUObjectItem.StructSize - (sSub * 2);
			continue;
		}

		ReadUObject(dwUObject, uObject, GObjObjects[i]);
		currentFUObjAddress += fUObjectItem.StructSize - (sSub * 2);
		GObjectsCount++;
	}
	return true;
}

bool ObjectsStore::ReadUObject(const uintptr_t uObjectAddress, JsonStruct& uObject, SdkUObject& retObj)
{
	// Alloc and Read the address as class
	if (!uObject.ReadData(uObjectAddress, "UObject")) return false;

	retObj.ObjAddress = uObjectAddress;
	retObj = uObject;
	return true;
}

void* ObjectsStore::GetAddress()
{
	return reinterpret_cast<void*>(gObjAddress);
}

size_t ObjectsStore::GetObjectsNum() const 
{
	return GObjectsCount;
}

UEObject ObjectsStore::GetById(const size_t id) const
{
	return UEObject(GObjObjects[id]);
}

UEClass ObjectsStore::FindClass(const std::string& name) const
{
	for (const auto& obj : *this)
	{
		if (obj.GetFullName() == name)
		{
			return obj.Cast<UEClass>();
		}
	}
	return UEClass();
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

ObjectsIterator::ObjectsIterator(const ObjectsStore& store)
	: store(store),
	  index(store.GetObjectsNum())
{
}

ObjectsIterator::ObjectsIterator(const ObjectsStore& store, const size_t index)
	: store(store),
	index(index)
{
	current = store.GetById(index);
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
	for (++index; index < ObjectsStore(store).GetObjectsNum(); ++index)
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