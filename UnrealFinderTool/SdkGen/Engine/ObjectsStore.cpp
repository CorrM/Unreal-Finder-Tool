#include "pch.h"
#include "Memory.h"
#include "ObjectsStore.h"
#include "EngineClasses.h"
#include "NamesStore.h"
#include <cassert>

GObjectInfo ObjectsStore::GInfo;
UnsortedMap<uintptr_t, std::unique_ptr<UEObject>> ObjectsStore::GObjObjects;

#pragma region ObjectsStore
bool ObjectsStore::Initialize(const uintptr_t gObjAddress, const bool forceReInit)
{
	ObjectsStore tmp;
	if (!forceReInit && GInfo.GObjAddress != NULL)
		return true;

	GObjObjects.clear();
	GInfo.Count = 0;
	GInfo.GObjAddress = gObjAddress;
	return tmp.FetchData();
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
			uintptr_t curAddress = GInfo.GObjAddress + uIndex * Utils::PointerSize();
			uintptr_t chunk = Utils::MemoryObj->ReadAddress(curAddress);

			// Skip null address
			if (chunk == 0)
			{
				++skipCount;
				continue;
			}

			skipCount = 0;
			GInfo.GChunks.push_back(chunk);
			++GInfo.ChunksCount;
		}

		if (skipCount >= 5)
		{
			GInfo.IsChunksAddress = true;
		}
		else
		{
			GInfo.IsChunksAddress = false;
			GInfo.GChunks.clear();
			GInfo.ChunksCount = 1;
		}
	}

	// Get Work Method [Pointer Next Pointer or FUObjectItem(Flags, ClusterIndex, etc)]
	{
		uintptr_t firstUObj = GInfo.IsChunksAddress ? Utils::MemoryObj->ReadAddress(GInfo.GObjAddress) : GInfo.GObjAddress;
		uintptr_t obj1 = Utils::MemoryObj->ReadAddress(firstUObj);
		uintptr_t obj2 = Utils::MemoryObj->ReadAddress(firstUObj + Utils::PointerSize());

		if (!Utils::IsValidAddress(Utils::MemoryObj, obj1)) return false;

		// Not Valid mean it's not Pointer Next To Pointer, or GObject address is wrong.
		GInfo.IsPointerNextToPointer = Utils::IsValidAddress(Utils::MemoryObj, obj2);
	}

	return true;
}

bool ObjectsStore::ReadUObjectArray()
{
	for (int i = 0; i < GInfo.ChunksCount; ++i)
	{
		int skipCount = 0;
		int offset = i * Utils::PointerSize();
		uintptr_t chunkAddress = GInfo.IsChunksAddress ? Utils::MemoryObj->ReadAddress(GInfo.GObjAddress + size_t(offset)) : GInfo.GObjAddress;

		for (size_t uIndex = 0; skipCount <= maxZeroAddress; ++uIndex)
		{
			uintptr_t dwUObject = NULL;

			// Get object address
			if (GInfo.IsPointerNextToPointer)
			{
				dwUObject = Utils::MemoryObj->ReadAddress(chunkAddress + uIndex * Utils::PointerSize());
			}
			else
			{
				FUObjectItem fUObjectItem;
				dwUObject = chunkAddress + uIndex * fUObjectItem.StructSize();
				fUObjectItem.ReadData(dwUObject);
				dwUObject = fUObjectItem.Object;
			}

			// Skip null pointer in GObjects array
			if (dwUObject == 0)
			{
				++skipCount;
				continue;
			}
			skipCount = 0;

			// Object container
			auto curObject = std::make_unique<UEObject>();

			// Skip bad object in GObjects array
			if (!ReadUObject(dwUObject, *curObject) || !IsValidUObject(curObject->Object))
			{
				++skipCount;
				continue;
			}
			skipCount = 0;

			GObjObjects.push_back(std::make_pair(dwUObject, std::move(curObject)));
			++GInfo.Count;
		}
	}

	return true;
}

bool ObjectsStore::ReadUObject(const uintptr_t uObjectAddress, UEObject& retUObj)
{
	UObject tmp;
	if (!tmp.ReadData(uObjectAddress)) return false;

	retUObj.Object = tmp;
	return true;
}

bool ObjectsStore::IsValidUObject(const UObject& uObject, const bool outerCheck) const
{
	if (NamesStore().GetNamesNum() == 0)
		throw std::exception("Init NamesStore first.");

	// Check if FName Index is bigger than current names count
	if (size_t(uObject.Name.ComparisonIndex) >= NamesStore().GetNamesNum()) return false;

	// Check internal index, it's must be bigger than the last one [ By one ]
	if (!outerCheck && !GObjObjects.empty())
	{
		UEObject& lastObj = *GObjObjects.back().second;
		if (uObject.InternalIndex < lastObj.Object.InternalIndex ||
			uObject.InternalIndex > lastObj.Object.InternalIndex + 5)
			return false;
	}

	// Check Outer is == NULL or Valid
	if (uObject.Outer != NULL)
	{
		bool found;
		UEObject& outer2 = GetByAddress(uObject.Outer, found);

		// if not found
		if (!found) return false;

		// if found, check is valid or not
		if (!IsValidUObject(outer2.Object, true)) return false;
	}

	// Check class

	return true;
}

uintptr_t ObjectsStore::GetAddress()
{
	return GInfo.GObjAddress;
}

size_t ObjectsStore::GetObjectsNum() const 
{
	return GInfo.Count;
}

UEObject& ObjectsStore::GetByIndex(const size_t index) const
{
	return *GObjObjects[index].second;
}

UEObject& ObjectsStore::GetByAddress(const uintptr_t objAddress) const
{
	return *GObjObjects.Find(objAddress);
}

UEObject& ObjectsStore::GetByAddress(const uintptr_t objAddress, bool& success) const
{
	auto& uniquePtr = GObjObjects.Find(objAddress, success);
	return success ? *uniquePtr : UEObjectEmpty;
}

UEClass ObjectsStore::FindClass(const std::string& name) const
{
	auto it = std::find_if(this->begin(), this->end(), [&](const UEObject& obj) -> bool { return obj.GetFullName() == name; });

	if (it != this->end())
		return (*it).Cast<UEClass>();

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
	current = store.GetByIndex(index);
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
		current = store.GetByIndex(index);
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
	++*this;
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