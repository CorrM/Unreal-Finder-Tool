#pragma once
#include <iterator>
#include "GenericTypes.h"

class Memory;
class NamesIterator;

class NamesStore
{
	friend NamesIterator;
	static std::vector<FNameEntity> gNames;
	static int chunkCount, gNamesChunks;
	static uintptr_t gNamesAddress;
	static bool ReadGNameArray(uintptr_t address);
	static bool IsValidAddress(uintptr_t address);

public:
	/// <summary>
	/// Initializes this object.
	/// </summary>
	/// <returns>true if it succeeds, false if it fails.</returns>
	static bool Initialize(uintptr_t gNamesAddress, bool forceReInit = true);

	/// <summary>Gets the address of the global names store.</summary>
	/// <returns>The address of the global names store.</returns>
	static void* GetAddress();

	NamesIterator begin();
	NamesIterator begin() const;
	NamesIterator end();
	NamesIterator end() const;

	/// <summary>
	/// Gets the number of available names.
	/// </summary>
	/// <returns>The number of names.</returns>
	int GetNamesNum() const;

	/// <summary>
	/// Test if the given id is valid.
	/// </summary>
	/// <param name="id">The identifier.</param>
	/// <returns>true if valid, false if not.</returns>
	bool IsValid(size_t id);

	/// <summary>
	/// Gets a name by id.
	/// </summary>
	/// <param name="id">The identifier.</param>
	/// <returns>The name.</returns>
	std::string GetByIndex(size_t id);
};

class NamesIterator : public std::iterator<std::forward_iterator_tag, FNameEntity>
{
	const NamesStore& store;
	size_t index;

public:
	NamesIterator(const NamesStore& store);

	explicit NamesIterator(const NamesStore& store, size_t index);

	void swap(NamesIterator& other) noexcept;

	NamesIterator& operator++();

	NamesIterator operator++ (int);

	bool operator==(const NamesIterator& rhs) const;

	bool operator!=(const NamesIterator& rhs) const;

	FNameEntity operator*() const;

	FNameEntity operator->() const;
};
