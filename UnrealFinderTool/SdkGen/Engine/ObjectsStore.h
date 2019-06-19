#pragma once
#include "GenericTypes.h"
#include "ParallelWorker.h"
#include <unordered_map>

class ObjectsIterator;
class Memory;

using GObjects = UnsortedMap<uintptr_t, std::unique_ptr<UEObject>>;

struct GObjectInfo
{
	uintptr_t GObjAddress;
	std::vector<uintptr_t> GChunks;
	int Count, ChunksCount;
	bool IsPointerNextToPointer;
	bool IsChunksAddress;
};

class ObjectsStore
{
	size_t numElementsPerChunk = 0x103FF;

	bool FetchData();
	bool GetGObjectInfo();
	bool ReadUObjectArray();
	bool ReadUObject(uintptr_t uObjectAddress, UEObject& retUObj);

public:
	static GObjectInfo GInfo;
	static GObjects GObjObjects;

	/// <summary>
	/// Initializes this object.
	/// </summary>
	/// <returns>
	/// true if it succeeds, false if it fails.
	/// </returns>
	static bool Initialize(uintptr_t gObjAddress, bool forceReInit = true);
	/// <summary>Gets the address of the global objects store.</summary>
	/// <returns>The address of the global objects store.</returns>
	static uintptr_t GetAddress();

	/// <summary>
	/// Gets the number of available objects.
	/// </summary>
	/// <returns>The number of objects.</returns>
	size_t GetObjectsNum() const;

	/// <summary>
	/// Gets the object by id.
	/// </summary>
	/// <param name="index">The identifier.</param>
	/// <returns>The object.</returns>
	UEObject* GetByIndex(size_t index) const;

	/// <summary>
	/// Gets the object by id.
	/// </summary>
	/// <param name="objAddress">The address of object.</param>
	/// <returns>The object.</returns>
	static UEObject* GetByAddress(uintptr_t objAddress);
	static UEObject* GetByAddress(uintptr_t objAddress, bool& success);

	/// <summary>
	/// Searches for the first class with the given name.
	/// </summary>
	/// <param name="name">The name of the class.</param>
	/// <returns>The found class which is not valid if no class could be found.</returns>
	UEClass FindClass(const std::string& name) const;

	/// <summary>Count objects which have the same name and type.</summary>
	/// <typeparam name="T">Type of the object.</typeparam>
	/// <param name="name">The name to search for.</param>
	/// <returns>The number of objects which share a name.</returns>
	template<class T>
	size_t CountObjects(const std::string& name) const;

	ObjectsIterator begin();
	ObjectsIterator begin() const;
	ObjectsIterator end();
	ObjectsIterator end() const;
};

template <class T>
size_t ObjectsStore::CountObjects(const std::string& name) const
{
	static std::unordered_map<std::string, size_t> cache;

	auto it = cache.find(name);
	if (it != std::end(cache))
		return it->second;

	size_t index = 0;
	size_t count = 0;
	ParallelSingleShot worker(Utils::Settings.SdkGen.Threads, [&](ParallelOptions& options)
	{
		while (index > GObjObjects.size())
		{
			UEObject* curObj;

			// Get current object
			{
				std::lock_guard lock(options.Locker);
				curObj = GObjObjects[index].second.get();
				++index;
			}

			// Calc count of object
			if (curObj->IsA<T>() && curObj->GetName() == name)
			{
				std::lock_guard lock(options.Locker);
				++count;
			}
		}
	});
	worker.Start();
	worker.WaitAll();

	cache[name] = count;
	return count;
}

/// <summary>An iterator for objects.</summary>
class ObjectsIterator : public std::iterator<std::forward_iterator_tag, UEObject>
{
	const ObjectsStore& store;
	size_t index;
	UEObject* current;

public:
	/// <summary>Constructor.</summary>
	/// <param name="store">The store to iterate.</param>
	ObjectsIterator(const ObjectsStore& store);

	/// <summary>Constructor.</summary>
	/// <param name="store">The store to iterate.</param>
	/// <param name="index">Zero-based start index.</param>
	explicit ObjectsIterator(const ObjectsStore& store, size_t index);

	ObjectsIterator(const ObjectsIterator& other);
	ObjectsIterator(ObjectsIterator&& other) noexcept;

	ObjectsIterator& operator=(const ObjectsIterator& rhs);

	void swap(ObjectsIterator& other) noexcept;

	ObjectsIterator& operator++();

	ObjectsIterator operator++ (int);

	bool operator==(const ObjectsIterator& rhs) const;

	bool operator!=(const ObjectsIterator& rhs) const;

	UEObject operator*() const;

	UEObject operator->() const;
};
