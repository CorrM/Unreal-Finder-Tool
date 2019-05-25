#include "pch.h"
#include "NamesStore.h"
#include "Utils.h"
#include "Memory.h"

GName* NamesStore::gNames;
int NamesStore::gNamesChunkCount;
int NamesStore::gNamesChunks;
uintptr_t NamesStore::gNamesAddress;

#pragma region NamesStore
bool NamesStore::Initialize(const uintptr_t gNamesAddress)
{
	NamesStore::gNamesAddress = gNamesAddress;
	NamesStore::gNamesChunkCount = 16384;
	return FetchData();
}

bool NamesStore::FetchData()
{
	// GNames
	if (!ReadGNameArray(gNamesAddress))
	{
		std::cout << red << "[*] " << def << "Invalid GNames Address." << std::endl << def;
		return false;
	}

	return true;
}

bool NamesStore::ReadGNameArray(uintptr_t address)
{
	int ptrSize = Utils::PointerSize();
	size_t nameOffset = 0;
	JsonStruct fName;

	if (!Utils::MemoryObj->Is64Bit)
		address = Utils::MemoryObj->ReadInt(address);
	else
		address = Utils::MemoryObj->ReadInt64(address);

	// Get GNames Chunks
	std::vector<uintptr_t> gChunks;
	for (int i = 0; i < ptrSize * 15; ++i)
	{
		uintptr_t addr;
		const int offset = ptrSize * i;

		if (!Utils::MemoryObj->Is64Bit)
			addr = Utils::MemoryObj->ReadInt(address + offset); // 4byte
		else
			addr = Utils::MemoryObj->ReadInt64(address + offset); // 8byte

		if (!IsValidAddress(addr)) break;
		gChunks.push_back(addr);
		gNamesChunks++;
	}

	// Alloc Size
	gNames = new GName[gNamesChunkCount * gNamesChunks];

	// Calc AnsiName offset
	{
		uintptr_t noneNameAddress;
		if (!Utils::MemoryObj->Is64Bit)
			noneNameAddress = Utils::MemoryObj->ReadInt(gChunks[0]); // 4byte
		else
			noneNameAddress = Utils::MemoryObj->ReadInt64(gChunks[0]); // 8byte

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
		uintptr_t fNameAddress;
		for (int j = 0; j < gNamesChunkCount; ++j)
		{
			const int offset = ptrSize * j;

			if (!Utils::MemoryObj->Is64Bit)
				fNameAddress = Utils::MemoryObj->ReadInt(chunkAddress + offset); // 4byte
			else
				fNameAddress = Utils::MemoryObj->ReadInt64(chunkAddress + offset); // 8byte

			if (!IsValidAddress(fNameAddress))
			{
				gNames[i].Index = i;
				gNames[i].AnsiName = "";
				i++;
				continue;
			}

			// Read FName
			if (!fName.ReadData(fNameAddress, "FNameEntity")) return false;

			// Set The Name
			gNames[i].Index = i;
			gNames[i].AnsiName = Utils::MemoryObj->ReadText(fNameAddress + nameOffset);
			i++;
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