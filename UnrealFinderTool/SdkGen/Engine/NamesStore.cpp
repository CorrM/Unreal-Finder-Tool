#include "pch.h"
#include "Memory.h"
#include "NamesStore.h"

GName* NamesStore::gNames;
int NamesStore::gNamesChunkCount;
int NamesStore::gNamesChunks;
uintptr_t NamesStore::gNamesAddress;

#pragma region NamesStore
bool NamesStore::Initialize(const uintptr_t gNamesAddress, const bool forceReInit)
{
	if (!forceReInit && NamesStore::gNamesAddress != NULL)
		return true;

	delete[] gNames;
	gNamesChunks = 0;

	NamesStore::gNamesAddress = gNamesAddress;
	NamesStore::gNamesChunkCount = 16384;
	return FetchData();
}

bool NamesStore::FetchData()
{
	// GNames
	return ReadGNameArray(gNamesAddress);
}

bool NamesStore::ReadGNameArray(const uintptr_t address)
{
	int ptrSize = Utils::PointerSize();
	size_t nameOffset = 0;
	JsonStruct fName;

	// Get GNames Chunks
	std::vector<uintptr_t> gChunks;
	for (int i = 0; i < 15; ++i)
	{
		uintptr_t addr;
		const int offset = ptrSize * i;

		addr = Utils::MemoryObj->ReadAddress(address + offset);

		if (!IsValidAddress(addr)) break;
		gChunks.push_back(addr);
		gNamesChunks++;
	}

	// Alloc Size
	gNames = new GName[gNamesChunkCount * gNamesChunks];

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
		for (int j = 0; j < gNamesChunkCount; ++j)
		{
			const int offset = ptrSize * j;
			uintptr_t fNameAddress = Utils::MemoryObj->ReadAddress(chunkAddress + offset);

			if (!IsValidAddress(fNameAddress))
			{
				gNames[i].Index = i;
				gNames[i].AnsiName = "";
				++i;
				continue;
			}

			// Read FName
			if (!fName.ReadData(fNameAddress, "FNameEntity")) return false;

			// Set The Name
			gNames[i].Index = i;
			gNames[i].AnsiName = Utils::MemoryObj->ReadText(fNameAddress + nameOffset);
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

void* NamesStore::GetAddress()
{
	return reinterpret_cast<void*>(gNamesAddress);
}

size_t NamesStore::GetNamesNum() const
{
	int num = gNamesChunkCount * gNamesChunks;
	return size_t(num);
}

bool NamesStore::IsValid(const size_t id)
{
	return id >= 0 && id <= GetNamesNum() && !GetById(id).empty();
}

std::string NamesStore::GetById(const size_t id)
{
	// TODO: i think here is a problem, set BP at (return "") and check call stack. (why wrong id)
	if (id > GetNamesNum())
		return "";

	return gNames[id].AnsiName;
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

GName NamesIterator::operator*() const
{
	return { index, NamesStore(store).GetById(index) };
}

GName NamesIterator::operator->() const
{
	return { index, NamesStore(store).GetById(index) };
}
#pragma endregion