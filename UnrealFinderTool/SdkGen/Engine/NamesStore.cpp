#include "pch.h"
#include "Memory.h"
#include "NamesStore.h"

std::vector<FNameEntity> NamesStore::gNames;
int NamesStore::chunkCount = 16384;
int NamesStore::gNamesChunks;
uintptr_t NamesStore::gNamesAddress;

#pragma region NamesStore
bool NamesStore::Initialize(const uintptr_t gNamesAddress, const bool forceReInit)
{
	if (!forceReInit && NamesStore::gNamesAddress != NULL)
		return true;

	gNames.clear();
	gNamesChunks = 0;

	NamesStore::gNamesAddress = gNamesAddress;
	return ReadGNameArray(gNamesAddress);
}

bool NamesStore::ReadGNameArray(const uintptr_t address)
{
	int ptrSize = Utils::PointerSize();
	size_t nameOffset = 0;

	// Get GNames Chunks
	std::vector<uintptr_t> gChunks;
	for (int i = 0; i < 30; ++i)
	{
		uintptr_t addr;
		const int offset = ptrSize * i;

		addr = Utils::MemoryObj->ReadAddress(address + offset);

		if (!IsValidAddress(addr)) break;
		gChunks.push_back(addr);
		gNamesChunks++;
	}

	// Calc AnsiName offset
	{
		uintptr_t noneNameAddress = Utils::MemoryObj->ReadAddress(gChunks[0]);

		Pattern none_sig = PatternScan::Parse("None", 0, "4E 6F 6E 65 00", 0xFF);
		auto result = PatternScan::FindPattern(Utils::MemoryObj, noneNameAddress, noneNameAddress + 0x100, { none_sig }, true);
		auto it = result.find("None");
		if (it != result.end() && !it->second.empty())
			nameOffset = it->second[0] - noneNameAddress;
	}

	// Dump GNames
	int i = 0;
	for (uintptr_t chunkAddress : gChunks)
	{
		for (int j = 0; j < chunkCount; ++j)
		{
			FNameEntity tmp;
			const int offset = ptrSize * j;
			uintptr_t fNameAddress = Utils::MemoryObj->ReadAddress(chunkAddress + offset);

			if (!IsValidAddress(fNameAddress))
			{
				// Push Empty, if i just skip will case a problems, so just add empty item
				tmp.Index = i + 1; // FNameEntity Index look like that 0 .. 2 .. 4 .. 6
				tmp.AnsiName = "";

				gNames.push_back(std::move(tmp));
				++i;
				continue;
			}

			// Read FName
			if (!tmp.ReadData(fNameAddress, nameOffset)) return false;

			gNames.push_back(std::move(tmp));
			++i;
		}
	}

	return true;
}

bool NamesStore::IsValidAddress(const uintptr_t address)
{
	if (INVALID_POINTER_VALUE(address) /*|| pointer <= dwStart || pointer > dwEnd*/)
		return false;

	// Check memory state, type and permission
	MEMORY_BASIC_INFORMATION info;

	//const uintptr_t pointerVal = _memory->ReadInt(pointer);
	if (VirtualQueryEx(Utils::MemoryObj->ProcessHandle, LPVOID(address), &info, sizeof info) == sizeof info)
	{
		// Bad Memory
		return info.Protect != PAGE_NOACCESS;
	}

	return false;
}

uintptr_t NamesStore::GetAddress()
{
	return gNamesAddress;
}

size_t NamesStore::GetNamesNum() const
{
	return chunkCount * gNamesChunks;
}

bool NamesStore::IsValid(const size_t id)
{
	return id >= 0 && id <= GetNamesNum() && !GetByIndex(id).empty();
}

std::string NamesStore::GetByIndex(const size_t id)
{
	// TODO: i think here is a problem, set BP at (return "") and check call stack. (why wrong id)
	if (id > GetNamesNum())
		return "";

	return gNames[id].AnsiName;
}

int NamesStore::GetByName(const std::string& name)
{
	auto retIt = std::find_if(gNames.begin(), gNames.end(), [&](FNameEntity& fName) -> bool
	{
		return fName.AnsiName == name;
	});

	if (retIt == gNames.end())
		return -1;

	return std::distance(gNames.begin(), retIt);
}
#pragma endregion

#pragma region NamesIterator
NamesIterator NamesStore::begin()
{
	return NamesIterator(*this, 0);
}

NamesIterator NamesStore::begin() const
{
	return NamesIterator(*this, 0);
}

NamesIterator NamesStore::end()
{
	return NamesIterator(*this);
}

NamesIterator NamesStore::end() const
{
	return NamesIterator(*this);
}

NamesIterator::NamesIterator(const NamesStore& _store)
	: store(_store),
	  index(_store.GetNamesNum())
{
}

NamesIterator::NamesIterator(const NamesStore& _store, size_t _index)
	: store(_store),
	  index(_index)
{
}

void NamesIterator::swap(NamesIterator& other) noexcept
{
	std::swap(index, other.index);
}

NamesIterator& NamesIterator::operator++()
{
	for (++index; index < store.GetNamesNum(); ++index)
	{
		if (NamesStore(store).IsValid(index))
		{
			break;
		}
	}
	return *this;
}

NamesIterator NamesIterator::operator++ (int)
{
	auto tmp(*this);
	++(*this);
	return tmp;
}

bool NamesIterator::operator==(const NamesIterator& rhs) const
{
	return index == rhs.index;
}

bool NamesIterator::operator!=(const NamesIterator& rhs) const
{
	return index != rhs.index;
}

FNameEntity NamesIterator::operator*() const
{
	return NamesStore::gNames[index];
}

FNameEntity NamesIterator::operator->() const
{
	return NamesStore::gNames[index];
}
#pragma endregion