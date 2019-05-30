#include "pch.h"
#include "Memory.h"
#include "ObjectsStore.h"
#include "SdkGen/EngineClasses.h"
#include <cassert>

std::vector<std::unique_ptr<UEObject>> ObjectsStore::GObjObjects;
int ObjectsStore::gObjectsCount;
uintptr_t ObjectsStore::gObjAddress;
int ObjectsStore::maxZeroAddress = 150;

#pragma region ObjectsStore
bool ObjectsStore::Initialize(const uintptr_t gObjAddress)
{
	ObjectsStore::gObjAddress = gObjAddress;

	return FetchData();
}

bool ObjectsStore::FetchData()
{
	// GObjects
	if (!ReadUObjectArray(gObjAddress))
		return false;

	return true;
}

bool ObjectsStore::ReadUObjectArray(const uintptr_t address)
{
	// Get Work Method [Pointer Next Pointer or FUObjectItem(Flags, ClusterIndex, etc)]
	bool isPointerNextPointer = false;
	uintptr_t obj1 = Utils::MemoryObj->ReadAddress(address);
	uintptr_t obj2 = Utils::MemoryObj->ReadAddress(address + Utils::PointerSize());

	if (!Utils::IsValidAddress(Utils::MemoryObj, obj1)) return false;
	// Not Valid mean it's not Pointer Next To Pointer, or GObject address is wrong.
	isPointerNextPointer = Utils::IsValidAddress(Utils::MemoryObj, obj2);

	return isPointerNextPointer ? ReadUObjectArrayPnP(address) : ReadUObjectArrayNormal(address);
}

bool ObjectsStore::ReadUObjectArrayPnP(const uintptr_t address)
{
	JsonStruct uObject;
	int skipCount = 0;

	for (size_t uIndex = 0; skipCount <= maxZeroAddress; ++uIndex)
	{
		uintptr_t curAddress = address + uIndex * Utils::PointerSize();
		uintptr_t dwUObject = Utils::MemoryObj->ReadAddress(curAddress);

		// Skip null pointer in GObjects array
		if (dwUObject == 0)
		{
			++skipCount;
			continue;
		}

		auto curObject = std::make_unique<UEObject>();
		ReadUObject(dwUObject, uObject, *curObject);

		GObjObjects.push_back(std::move(curObject));
		++gObjectsCount;
		skipCount = 0;
	}
	return true;
}

bool ObjectsStore::ReadUObjectArrayNormal(const uintptr_t address)
{
	JsonStruct fUObjectItem, uObject;
	int skipCount = 0;
	uintptr_t curAddress = 0;

	for (size_t uIndex = 0; skipCount <= maxZeroAddress; ++uIndex)
	{
		curAddress = address + uIndex * (fUObjectItem.StructSize - fUObjectItem.SubUnNeededSize());

		// Read the address as struct
		if (!fUObjectItem.ReadData(curAddress, "FUObjectItem")) return false;
		auto dwUObject = fUObjectItem["Object"].ReadAs<uintptr_t>();

		// Skip null pointer in GObjects array
		if (dwUObject == 0)
		{
			++skipCount;
			continue;
		}

		auto curObject = std::make_unique<UEObject>();
		ReadUObject(dwUObject, uObject, *curObject);

		GObjObjects.push_back(std::move(curObject));
		++gObjectsCount;
		skipCount = 0;
	}
	return true;
}

bool ObjectsStore::ReadUObject(const uintptr_t uObjectAddress, JsonStruct& uObject, UEObject& retUObj)
{
	// Alloc and Read the address as class
	if (!uObject.ReadData(uObjectAddress, "UObject")) return false;

	UObject coreObj;
	coreObj.ObjAddress = uObjectAddress;
	coreObj = uObject;

	retUObj.Object = coreObj;
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

UEObject& ObjectsStore::GetById(const size_t id) const
{
	return *GObjObjects[id];
}

UEClass ObjectsStore::FindClass(const std::string& name) const
{
	for (const UEObject& obj : *this)
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