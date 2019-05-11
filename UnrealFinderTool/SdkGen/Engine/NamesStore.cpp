#include "pch.h"
#include "NamesStore.h"
#include "Utils.h"
#include "Memory.h"

GName* NamesStore::gNames;
Memory* NamesStore::mem;
int NamesStore::gNamesChunkCount;
int NamesStore::gNamesChunks;
uintptr_t NamesStore::gNamesAddress;

#pragma region NamesStore
bool NamesStore::Initialize(Memory* mem, const uintptr_t gNamesAddress)
{
	NamesStore::mem = mem;
	NamesStore::gNamesAddress = gNamesAddress;

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
	int ptrSize = Utils::PointerSize(mem);
	int sSub = mem->Is64Bit && Utils::ProgramIs64() ? 0x0 : 0x4;
	JsonStruct fName;
	if (!JsonReflector::ReadStruct("FNameEntity", fName))
	{
		std::cout << red << "[*] " << def << "Can't find struct `FNameEntity`." << std::endl << def;
		return false;
	}

	if (!mem->Is64Bit)
		address = mem->ReadInt(address);
	else
		address = mem->ReadInt64(address);

	// Get GNames Chunks
	std::vector<uintptr_t> gChunks;
	for (int i = 0; i < ptrSize * 15; ++i)
	{
		uintptr_t addr;
		const int offset = ptrSize * i;

		if (!mem->Is64Bit)
			addr = mem->ReadInt(address + offset); // 4byte
		else
			addr = mem->ReadInt64(address + offset); // 8byte

		if (!IsValidAddress(addr)) break;
		gChunks.push_back(addr);
		gNamesChunks++;
	}

	// Alloc Size
	gNames = new GName[gNamesChunkCount * gNamesChunks];

	// Dump GNames
	int i = 0;
	for (uintptr_t chunkAddress : gChunks)
	{
		uintptr_t fNameAddress;
		for (int j = 0; j < gNamesChunkCount; ++j)
		{
			const int offset = ptrSize * j;

			if (!mem->Is64Bit)
				fNameAddress = mem->ReadInt(chunkAddress + offset); // 4byte
			else
				fNameAddress = mem->ReadInt64(chunkAddress + offset); // 8byte

			if (!IsValidAddress(fNameAddress))
			{
				gNames[i].Index = i;
				gNames[i].AnsiName = "";
				i++;
				continue;
			}

			// Read FName
			if (mem->ReadBytes(fNameAddress, fName.AllocPointer, fName.StructSize) == 0)
				return false;

			// Set The Name
			gNames[i].Index = i;
			gNames[i].AnsiName = mem->ReadText(fNameAddress + fName["AnsiName"].Offset - sSub);
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
	if (VirtualQueryEx(mem->ProcessHandle, LPVOID(address), &info, sizeof info) == sizeof info)
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
	return gNamesChunkCount * gNamesChunks;
}

bool NamesStore::IsValid(const size_t id)
{
	return id >= 0 && id <= GetNamesNum() && !GetById(id).empty();
}

std::string NamesStore::GetById(const size_t id)
{
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
		if (store.IsValid(index))
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