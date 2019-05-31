#pragma once
#include <iterator>
#include "GenericTypes.h"

class Memory;
class NamesIterator;

class GName
{
public:
	size_t Index;
	std::string AnsiName;
};

class NamesStore
{
	friend NamesIterator;
	static GName* gNames;
	static int gNamesChunkCount, gNamesChunks;
	static uintptr_t gNamesAddress;
	static bool FetchData();
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
	size_t GetNamesNum() const;

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
	std::string GetById(size_t id);
};

class NamesIterator : public std::iterator<std::forward_iterator_tag, GName>
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

	GName operator*() const;

	GName operator->() const;
};
