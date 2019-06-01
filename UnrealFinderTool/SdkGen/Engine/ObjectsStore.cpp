#include "pch.h"
#include "Memory.h"
#include "ObjectsStore.h"
#include "SdkGen/EngineClasses.h"
#include <cassert>
#include "NamesStore.h"

GObjectInfo ObjectsStore::gInfo;
std::vector<std::unique_ptr<UEObject>> ObjectsStore::GObjObjects;
int ObjectsStore::maxZeroAddress = 150;

#pragma region ObjectsStore
bool ObjectsStore::Initialize(const uintptr_t gObjAddress, const bool forceReInit)
{
	if (!forceReInit && gInfo.GObjAddress != NULL)
		return true;

	GObjObjects.clear();
	gInfo.Count = 0;
	gInfo.GObjAddress = gObjAddress;
	return FetchData();
}

bool ObjectsStore::FetchData()
{
	if (!GetGObjectInfo()) return false;
	return ReadUObjectArray();
}

bool ObjectsStore::GetGObjectInfo()
{
	// Check if it's first `UObject` or first `Chunk` address
	{
		int skipCount = 0;
		for (size_t uIndex = 0; uIndex <= 20 && skipCount <= 5; ++uIndex)
		{
			uintptr_t curAddress = gInfo.GObjAddress + uIndex * Utils::PointerSize();
			uintptr_t chunk = Utils::MemoryObj->ReadAddress(curAddress);

			// Skip null address
			if (chunk == 0)
			{
				++skipCount;
				continue;
			}

			skipCount = 0;
			gInfo.GChunks.push_back(chunk);
			++gInfo.ChunksCount;
		}

		if (skipCount >= 5)
		{
			gInfo.IsChunksAddress = true;
		}
		else
		{
			gInfo.IsChunksAddress = false;
			gInfo.GChunks.clear();
			gInfo.ChunksCount = 1;
		}
	}

	// Get Work Method [Pointer Next Pointer or FUObjectItem(Flags, ClusterIndex, etc)]
	{
		uintptr_t firstUObj = gInfo.IsChunksAddress ? Utils::MemoryObj->ReadAddress(gInfo.GObjAddress) : gInfo.GObjAddress;
		uintptr_t obj1 = Utils::MemoryObj->ReadAddress(firstUObj);
		uintptr_t obj2 = Utils::MemoryObj->ReadAddress(firstUObj + Utils::PointerSize());

		if (!Utils::IsValidAddress(Utils::MemoryObj, obj1)) return false;

		// Not Valid mean it's not Pointer Next To Pointer, or GObject address is wrong.
		gInfo.IsPointerNextToPointer = Utils::IsValidAddress(Utils::MemoryObj, obj2);
	}

	return true;
}

bool ObjectsStore::ReadUObjectArray()
{
	return gInfo.IsPointerNextToPointer ? ReadUObjectArrayPnP() : ReadUObjectArrayNormal();
}

bool ObjectsStore::ReadUObjectArrayPnP()
{
	JsonStruct uObject;
	int skipCount = 0;

	for (int i = 0; i < gInfo.ChunksCount; ++i)
	{
		skipCount = 0;
		int offset = i * Utils::PointerSize();
		uintptr_t chunkAddress = gInfo.IsChunksAddress ? Utils::MemoryObj->ReadAddress(gInfo.GObjAddress + size_t(offset)) : gInfo.GObjAddress;

		for (size_t uIndex = 0; skipCount <= maxZeroAddress; ++uIndex)
		{
			uintptr_t curAddress = chunkAddress + uIndex * Utils::PointerSize();
			uintptr_t dwUObject = Utils::MemoryObj->ReadAddress(curAddress);

			// Skip null pointer in GObjects array
			if (dwUObject == 0)
			{
				++skipCount;
				continue;
			}
			skipCount = 0;

			auto curObject = std::make_unique<UEObject>();
			ReadUObject(dwUObject, uObject, *curObject);

			// Skip bad object in GObjects array
			if (!IsValidUObject(*curObject))
			{
				++skipCount;
				continue;
			}
			skipCount = 0;

			GObjObjects.push_back(std::move(curObject));
			++gInfo.Count;
		}
	}
	return true;
}

bool ObjectsStore::ReadUObjectArrayNormal()
{
	JsonStruct fUObjectItem, uObject;
	int skipCount = 0;

	for (int i = 0; i < gInfo.ChunksCount; ++i)
	{
		skipCount = 0;
		int offset = i * Utils::PointerSize();
		uintptr_t chunkAddress = gInfo.IsChunksAddress ? Utils::MemoryObj->ReadAddress(gInfo.GObjAddress + size_t(offset)) : gInfo.GObjAddress;

		for (size_t uIndex = 0; skipCount <= maxZeroAddress; ++uIndex)
		{
			uintptr_t curAddress = chunkAddress + uIndex * (fUObjectItem.StructSize - fUObjectItem.SubUnNeededSize());

			// Read the address as struct
			if (!fUObjectItem.ReadData(curAddress, "FUObjectItem")) return false;
			auto dwUObject = fUObjectItem["Object"].ReadAs<uintptr_t>();

			// Skip null pointer in GObjects array
			if (dwUObject == 0)
			{
				++skipCount;
				continue;
			}
			skipCount = 0;

			auto curObject = std::make_unique<UEObject>();
			ReadUObject(dwUObject, uObject, *curObject);

			// Skip bad object in GObjects array
			if (!IsValidUObject(*curObject))
			{
				++skipCount;
				continue;
			}
			skipCount = 0;

			GObjObjects.push_back(std::move(curObject));
			++gInfo.Count;
		}
	}
	return true;
}

bool ObjectsStore::ReadUObject(const uintptr_t uObjectAddress, JsonStruct& uObject, UEObject& retUObj)
{
	// Alloc and Read the address as class
	if (!uObject.ReadData(uObjectAddress, "UObject")) return false;

	retUObj.Object = uObject;
	retUObj.Object.ObjAddress = uObjectAddress;
	return true;
}

bool ObjectsStore::IsValidUObject(const UEObject& uObject)
{
	if (NamesStore().GetNamesNum() == 0)
		throw std::exception("Init NamesStore first.");

	return size_t(uObject.Object.Name.ComparisonIndex) <= NamesStore().GetNamesNum();
}

uintptr_t ObjectsStore::GetAddress()
{
	return gInfo.GObjAddress;
}

size_t ObjectsStore::GetObjectsNum() const 
{
	return gInfo.Count;
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