#pragma once
#pragma warning(disable: 4302)
#pragma warning(disable: 4311)

#include <string>
#include <windows.h>
#include "../Utils.h"
#include "../JsonReflector.h"

// #define OFFSET(s, m) ((size_t)&reinterpret_cast<char const volatile&>((((s*)0)->m)))
#define OFFSET(m) ((int)&reinterpret_cast<char const volatile&>((((decltype(this))0)->m)))
#define FILL_DATA(pData, var, offset) var = *reinterpret_cast<decltype(var)*>(&pData[offset])
#define SIZE_OF_ME sizeof(decltype(this))

class UClass;
class UObject;

#pragma region Same Size As Engine
struct FPointer
{
	uintptr_t Dummy;

	void FixPointers(const int fullStructSize)
	{
		Utils::FixPointers(this, fullStructSize, { OFFSET(Dummy) });
	}
};

struct FQWord
{
	int32_t A;
	int32_t B;

	void FixPointers(const int fullStructSize)
	{
		// 
	}
};

struct FName
{
	int32_t ComparisonIndex;
	int32_t Number;
};

struct TArray
{
	uintptr_t Data = NULL;
	int32_t Count = 0;
	int32_t Max = 0;

	friend struct FString;

	bool IsValidIndex(const int i) const
	{
		return i < Count;
	}

	void FixPointers(const int fullStructSize)
	{
		Utils::FixPointers(this, fullStructSize, { OFFSET(Data) });
	}
};

struct FString : TArray // <wchar_t>
{
	/*std::string ToString() const
	{
		int size = WideCharToMultiByte(CP_UTF8, 0, Data, Count, nullptr, 0, nullptr, nullptr);
		std::string str(size, 0);
		WideCharToMultiByte(CP_UTF8, 0, Data, Count, &str[0], size, nullptr, nullptr);
		return str;
	}*/

	void FixPointers(const int fullStructSize)
	{
		TArray::FixPointers(fullStructSize);
	}
};

struct FText
{
	char UnknownData[0x18];
};

struct FWeakObjectPtr
{
	int32_t ObjectIndex;
	int32_t ObjectSerialNumber;
};

struct FStringAssetReference
{
	FString AssetLongPathname;
};

struct FGuid
{
	uint32_t A;
	uint32_t B;
	uint32_t C;
	uint32_t D;
};

struct FUniqueObjectGuid
{
	FGuid Guid;
};

struct FScriptDelegate
{
	unsigned char UnknownData[20];
};

struct FScriptMulticastDelegate
{
	unsigned char UnknownData[16];
};

struct FUEnumItem // Same As TPair
{
	FName Key;
	uint64_t Value = 0;
};

#pragma endregion

class FScriptInterface
{
	bool init = false;
	Memory* m = Utils::MemoryObj;
	PBYTE pData = nullptr;

public:
	// Object remote address
	uintptr_t ObjAddress = NULL;

	uintptr_t ObjectPointer = NULL;
	uintptr_t InterfacePointer = NULL;

	uintptr_t GetObj() const
	{
		return ObjectPointer;
	}

	uintptr_t GetInterface() const
	{
		return ObjectPointer != NULL ? InterfacePointer : NULL;
	}

	std::string TypeName() { return "FScriptInterface"; }

	void FixPointers(const int fullStructSize)
	{
		Utils::FixPointers(this, fullStructSize, {
			OFFSET(ObjectPointer),
			OFFSET(InterfacePointer)
		});
	}

	bool ReadData(const uintptr_t objAddress)
	{
		ObjAddress = objAddress;

		if (ObjAddress == NULL)
			return false;

		// Read this struct
		static JsonStruct jStruct = JsonReflector::GetStruct(TypeName());
		static int jSize = jStruct.GetSize();

		// Read Remote Memory
		bool dataAllocer = false;
		if (pData == nullptr)
		{
			dataAllocer = true;
			pData = new BYTE[jSize];

			// Read this struct
			if (m->ReadBytes(ObjAddress, pData, jSize) != jSize) return false;

			// Fix pointers for x32 games
			FixPointers(jSize);
		}

		FILL_DATA(pData, ObjectPointer, jStruct["ObjectPointer"].Offset);
		FILL_DATA(pData, InterfacePointer, jStruct["InterfacePointer"].Offset);

		// It's Initialized
		init = true;

		if (dataAllocer)
		{
			delete pData;
			pData = nullptr;
		}

		return jStruct.GetSize();
	}

	bool ReadData()
	{
		return ReadData(ObjAddress);
	}

	int StructSize()
	{
		static JsonStruct jStruct = JsonReflector::GetStruct(TypeName());
		return jStruct.GetSize();
	}

	bool IsInit()
	{
		return init;
	}
};

template<typename TObjectId>
class TPersistentObjectPtr
{
public:
	FWeakObjectPtr WeakPtr;
	int32_t TagAtLastTest;
	TObjectId ObjectId;

	void FixPointers(const int fullStructSize)
	{
		// FScriptInterface::FixPointers();
		// Utils::FixPointers(this, fullStructSize, { OFFSET(Data) });
	}
};

class FAssetPtr : public TPersistentObjectPtr<FStringAssetReference>
{
public:
	void FixPointers(const int fullStructSize)
	{
		TPersistentObjectPtr::FixPointers(fullStructSize);
	}
};

class FLazyObjectPtr : public TPersistentObjectPtr<FUniqueObjectGuid>
{
public:
	void FixPointers(const int fullStructSize)
	{
		TPersistentObjectPtr::FixPointers(fullStructSize);
	}
};

class FUObjectItem
{
	bool init = false;
	Memory* m = Utils::MemoryObj;
	PBYTE pData = nullptr;

public:
	// Object remote address
	uintptr_t ObjAddress = NULL;

	uintptr_t Object = NULL;
	int32_t Flags = 0;
	int32_t ClusterIndex = 0;
	int32_t SerialNumber = 0;

	std::string TypeName() { return "FUObjectItem"; }

	void FixPointers(const int fullStructSize)
	{
		Utils::FixPointers(this, fullStructSize, { OFFSET(Object) });
	}

	bool ReadData(const uintptr_t objAddress)
	{
		ObjAddress = objAddress;

		if (ObjAddress == NULL)
			return false;

		// Read this struct
		static JsonStruct jStruct = JsonReflector::GetStruct(TypeName());
		static int jSize = jStruct.GetSize();

		// Read Remote Memory
		bool dataAllocer = false;
		if (pData == nullptr)
		{
			dataAllocer = true;
			pData = new BYTE[jSize];

			// Read this struct
			if (m->ReadBytes(ObjAddress, pData, jSize) != jSize) return false;

			// Fix pointers for x32 games
			FixPointers(jSize);
		}

		// Read this struct
		FILL_DATA(pData, Object, jStruct["Object"].Offset);
		FILL_DATA(pData, Flags, jStruct["Flags"].Offset);
		FILL_DATA(pData, ClusterIndex, jStruct["ClusterIndex"].Offset);
		FILL_DATA(pData, SerialNumber, jStruct["SerialNumber"].Offset);

		// It's Initialized
		init = true;

		if (dataAllocer)
		{
			delete pData;
			pData = nullptr;
		}
		
		return true;
	}

	bool ReadData()
	{
		return ReadData(ObjAddress);
	}

	int StructSize()
	{
		static JsonStruct jStruct = JsonReflector::GetStruct(TypeName());
		return jStruct.GetSize();
	}

	bool IsInit()
	{
		return init;
	}
};

class FNameEntity
{
	bool init = false;
	Memory* m = Utils::MemoryObj;
	PBYTE pData = nullptr;

public:
	// Object remote address
	uintptr_t ObjAddress = NULL;

	size_t Index;
	std::string AnsiName;

	FNameEntity() = default;
	explicit FNameEntity(const size_t index, const std::string ansiName) : Index(index), AnsiName(ansiName) { }

	std::string TypeName() { return "FNameEntity"; }

	void FixPointers(const int fullStructSize)
	{
		// 
	}

	bool ReadData(const uintptr_t objAddress, const int ansiNameOffset)
	{
		ObjAddress = objAddress;

		if (ObjAddress == NULL)
			return false;

		// Read this struct
		static JsonStruct jStruct = JsonReflector::GetStruct(TypeName());
		static int jSize = jStruct.GetSize();

		// Read Remote Memory
		bool dataAllocer = false;
		if (pData == nullptr)
		{
			dataAllocer = true;
			pData = new BYTE[jSize];

			// Read this struct
			if (m->ReadBytes(ObjAddress, pData, jSize) != jSize) return false;

			// Fix pointers for x32 games
			FixPointers(jSize);
		}

		FILL_DATA(pData, Index, jStruct["Index"].Offset);
		AnsiName = m->ReadText(objAddress + ansiNameOffset);

		if (dataAllocer)
		{
			delete pData;
			pData = nullptr;
		}
		return true;
	}

	bool ReadData(const int ansiNameOffset)
	{
		return ReadData(ObjAddress, ansiNameOffset);
	}

	bool IsInit()
	{
		return init;
	}
};

class UObject
{
	bool init = false;
protected:
	Memory* m = Utils::MemoryObj;
	PBYTE pData = nullptr;

public:
	// Object remote address
	uintptr_t ObjAddress = 0;

	uintptr_t VfTable = 0;
	int Flags = 0;
	int InternalIndex = 0;
	uintptr_t Class = 0;
	FName Name;
	uintptr_t Outer = 0;

	std::string TypeName() { return "UObject"; }

	void FixPointers(const int fullStructSize)
	{
		Utils::FixPointers(this, fullStructSize, {
			OFFSET(VfTable),
			OFFSET(Class),
			OFFSET(Outer),
		});
	}

	bool ReadData(const uintptr_t objAddress)
	{
		// Set object remote address
		ObjAddress = objAddress;

		if (ObjAddress == NULL)
			return false;

		static JsonStruct jStruct = JsonReflector::GetStruct(TypeName());
		static int jSize = jStruct.GetSize();

		bool dataAllocer = false;
		if (pData == nullptr)
		{
			dataAllocer = true;
			pData = new BYTE[jSize];

			// Read this struct
			if (m->ReadBytes(ObjAddress, pData, jSize) != jSize) return false;

			// Fix pointers for x32 games
			FixPointers(jSize);
		}

		FILL_DATA(pData, VfTable, jStruct["VfTable"].Offset);
		FILL_DATA(pData, Flags, jStruct["Flags"].Offset);
		FILL_DATA(pData, InternalIndex, jStruct["InternalIndex"].Offset);
		FILL_DATA(pData, Class, jStruct["Class"].Offset);
		FILL_DATA(pData, Name, jStruct["Name"].Offset);
		FILL_DATA(pData, Outer, jStruct["Outer"].Offset);

		// It's initialized
		init = true;

		if (dataAllocer)
		{
			delete pData;
			pData = nullptr;
		}
		return true;
	}

	bool ReadData()
	{
		return ReadData(ObjAddress);
	}

	int StructSize()
	{
		static JsonStruct jStruct = JsonReflector::GetStruct(TypeName());
		return jStruct.GetSize();
	}

	bool IsEqual(const UObject& toCheck) const
	{
		return toCheck.InternalIndex == this->InternalIndex && toCheck.Name.ComparisonIndex == this->Name.ComparisonIndex;
	}

	bool IsInit()
	{
		return init;
	}

	bool Empty() const
	{
		return ObjAddress == NULL && VfTable == NULL;
	}

	template <typename USdkStruct>
	USdkStruct Cast() const
	{
		// it's like internal cast, but for remote process
		USdkStruct castType;

		// it's just to solve start up problem, for really Utils::MemoryObj will never == nullptr
		if (Utils::MemoryObj != nullptr)
			castType.ReadData(ObjAddress);

		return castType;
	}
};

class UField : public UObject
{
public:
	uintptr_t Next = 0;

	std::string TypeName() { return "UField"; }

	void FixPointers(const int fullStructSize)
	{
		UObject::FixPointers(fullStructSize);
		Utils::FixPointers(this, fullStructSize, { OFFSET(Next) });
	}

	bool ReadData(const uintptr_t objAddress)
	{
		// Set object remote address
		ObjAddress = objAddress;

		// Read this struct
		static JsonStruct jStruct = JsonReflector::GetStruct(TypeName());
		static int jSize = jStruct.GetSize();

		// Read Remote Memory
		bool dataAllocer = false;
		if (pData == nullptr)
		{
			dataAllocer = true;
			pData = new BYTE[jSize];

			// Read this struct
			if (m->ReadBytes(ObjAddress, pData, jSize) != jSize) return false;

			// Fix pointers for x32 games
			FixPointers(jSize);
		}

		// Init super variables first
		UObject::ReadData(objAddress);

		// Init `this` variables
		FILL_DATA(pData, Next, jStruct["Next"].Offset);

		if (dataAllocer)
		{
			delete pData;
			pData = nullptr;
		}
		return true;
	}

	bool ReadData()
	{
		return ReadData(ObjAddress);
	}

	int StructSize()
	{
		static JsonStruct jStruct = JsonReflector::GetStruct(TypeName());
		return jStruct.GetSize();
	}
};

// TODO: Check here
class UEnum : public UField
{
public:
	FString CppType;
	TArray Names; /*TArray<TPair<FName, uint64_t>> */
	int64_t CppForm = 0;

	std::string TypeName() { return "UEnum"; }

	void FixPointers(const int fullStructSize)
	{
		UField::FixPointers(fullStructSize);
		Utils::FixPointers(this, fullStructSize, {
			OFFSET(CppType.Data),
			OFFSET(Names.Data)
		});
	}

	bool ReadData(const uintptr_t objAddress)
	{
		// Set object remote address
		ObjAddress = objAddress;

		// Read this struct
		static JsonStruct jStruct = JsonReflector::GetStruct(TypeName());
		static int jSize = jStruct.GetSize();

		// Read Remote Memory
		bool dataAllocer = false;
		if (pData == nullptr)
		{
			dataAllocer = true;
			pData = new BYTE[jSize];

			// Read this struct
			if (m->ReadBytes(ObjAddress, pData, jSize) != jSize) return false;

			// Fix pointers for x32 games
			FixPointers(jSize);
		}

		// Init super variables first
		UField::ReadData(objAddress);

		// Init `this` variables
		FILL_DATA(pData, CppType, jStruct["CppType"].Offset);
		FILL_DATA(pData, Names, jStruct["Names"].Offset);
		FILL_DATA(pData, CppForm, jStruct["CppForm"].Offset);

		if (dataAllocer)
		{
			delete pData;
			pData = nullptr;
		}
		return true;
	}

	bool ReadData()
	{
		return ReadData(ObjAddress);
	}

	int StructSize()
	{
		static JsonStruct jStruct = JsonReflector::GetStruct(TypeName());
		return jStruct.GetSize();
	}
};

class UStruct : public UField
{
public:
	uintptr_t SuperField = 0;
	uintptr_t Children = 0;
	int32_t PropertySize = 0;
	int32_t MinAlignment = 0;

	std::string TypeName() { return "UStruct"; }

	void FixPointers(const int fullStructSize)
	{
		UField::FixPointers(fullStructSize);
		Utils::FixPointers(this, fullStructSize, {
			OFFSET(SuperField),
			OFFSET(Children)
		});
	}

	bool ReadData(const uintptr_t objAddress)
	{
		// Set object remote address
		ObjAddress = objAddress;

		// Read this struct
		static JsonStruct jStruct = JsonReflector::GetStruct(TypeName());
		static int jSize = jStruct.GetSize();

		// Read Remote Memory
		bool dataAllocer = false;
		if (pData == nullptr)
		{
			dataAllocer = true;
			pData = new BYTE[jSize];

			// Read this struct
			if (m->ReadBytes(ObjAddress, pData, jSize) != jSize) return false;

			// Fix pointers for x32 games
			FixPointers(jSize);
		}

		// Init super variables first
		UField::ReadData(objAddress);

		// Init `this` variables
		FILL_DATA(pData, SuperField, jStruct["SuperField"].Offset);
		FILL_DATA(pData, Children, jStruct["Children"].Offset);
		FILL_DATA(pData, PropertySize, jStruct["PropertySize"].Offset);
		FILL_DATA(pData, MinAlignment, jStruct["MinAlignment"].Offset);

		if (dataAllocer)
		{
			delete pData;
			pData = nullptr;
		}
		return true;
	}

	bool ReadData()
	{
		return ReadData(ObjAddress);
	}

	int StructSize()
	{
		static JsonStruct jStruct = JsonReflector::GetStruct(TypeName());
		return jStruct.GetSize();
	}
};

class UScriptStruct : public UStruct
{
public:
	std::string TypeName() { return "UScriptStruct"; }

	void FixPointers(const int fullStructSize)
	{
		UStruct::FixPointers(fullStructSize);
	}

	bool ReadData(const uintptr_t objAddress)
	{
		// Set object remote address
		ObjAddress = objAddress;

		// Read this struct
		static JsonStruct jStruct = JsonReflector::GetStruct(TypeName());
		static int jSize = jStruct.GetSize();

		// Read Remote Memory
		bool dataAllocer = false;
		if (pData == nullptr)
		{
			dataAllocer = true;
			pData = new BYTE[jSize];

			// Read this struct
			if (m->ReadBytes(ObjAddress, pData, jSize) != jSize) return false;

			// Fix pointers for x32 games
			FixPointers(jSize);
		}

		// Init super variables first
		UStruct::ReadData(objAddress);

		if (dataAllocer)
		{
			delete pData;
			pData = nullptr;
		}
		return true;
	}

	bool ReadData()
	{
		return ReadData(ObjAddress);
	}

	int StructSize()
	{
		static JsonStruct jStruct = JsonReflector::GetStruct(TypeName());
		return jStruct.GetSize();
	}
};

// ToDo: Check what UFunction really need!!, then don't read anything u don't need
class UFunction : public UStruct
{
public:
	int32_t FunctionFlags = 0;
	int8_t NumParms = 0;
	int16_t ParmsSize = 0;
	uint16_t ReturnValueOffset = 0;
	uint16_t RPCId = 0;
	uint16_t RPCResponseId = 0;
	uintptr_t FirstPropertyToInit = 0;
	uintptr_t EventGraphFunction = 0;
	int32_t EventGraphCallOffset = 0;
	uintptr_t Func = 0;

	std::string TypeName() { return "UFunction"; }

	void FixPointers(const int fullStructSize)
	{
		UStruct::FixPointers(fullStructSize);
		Utils::FixPointers(this, fullStructSize, {
			OFFSET(FirstPropertyToInit),
			OFFSET(EventGraphFunction),
			OFFSET(Func)
		});
	}

	bool ReadData(const uintptr_t objAddress)
	{
		// Set object remote address
		ObjAddress = objAddress;

		// Read this struct
		static JsonStruct jStruct = JsonReflector::GetStruct(TypeName());
		static int jSize = jStruct.GetSize();

		// Read Remote Memory
		bool dataAllocer = false;
		if (pData == nullptr)
		{
			dataAllocer = true;
			pData = new BYTE[jSize];

			// Read this struct
			if (m->ReadBytes(ObjAddress, pData, jSize) != jSize) return false;
			FILL_DATA(pData, Func, jStruct["Func"].Offset);

			// Fix pointers for x32 games
			FixPointers(jSize);
		}

		// Init super variables first
		UStruct::ReadData(objAddress);

		// Init `this` variables
		FILL_DATA(pData, FunctionFlags, jStruct["FunctionFlags"].Offset);
		FILL_DATA(pData, NumParms, jStruct["NumParms"].Offset);
		FILL_DATA(pData, ParmsSize, jStruct["ParmsSize"].Offset);
		FILL_DATA(pData, ReturnValueOffset, jStruct["ReturnValueOffset"].Offset);
		FILL_DATA(pData, RPCId, jStruct["RPCId"].Offset);
		FILL_DATA(pData, RPCResponseId, jStruct["RPCResponseId"].Offset);
		FILL_DATA(pData, FirstPropertyToInit, jStruct["FirstPropertyToInit"].Offset);
		FILL_DATA(pData, EventGraphFunction, jStruct["EventGraphFunction"].Offset);
		FILL_DATA(pData, EventGraphCallOffset, jStruct["EventGraphCallOffset"].Offset);

		if (dataAllocer)
		{
			delete pData;
			pData = nullptr;
		}
		return true;
	}

	bool ReadData()
	{
		return ReadData(ObjAddress);
	}

	int StructSize()
	{
		static JsonStruct jStruct = JsonReflector::GetStruct(TypeName());
		return jStruct.GetSize();
	}
};

class UClass : public UStruct
{
public:
	std::string TypeName() { return "UClass"; }

	void FixPointers(const int fullStructSize)
	{
		UStruct::FixPointers(fullStructSize);
	}

	bool ReadData(const uintptr_t objAddress)
	{
		// Set object remote address
		ObjAddress = objAddress;

		// Read this struct
		static JsonStruct jStruct = JsonReflector::GetStruct(TypeName());
		static int jSize = jStruct.GetSize();

		// Read Remote Memory
		bool dataAllocer = false;
		if (pData == nullptr)
		{
			dataAllocer = true;
			pData = new BYTE[jSize];

			// Read this struct
			if (m->ReadBytes(ObjAddress, pData, jSize) != jSize) return false;

			// Fix pointers for x32 games
			FixPointers(jSize);
		}

		// Init super variables first
		UStruct::ReadData(objAddress);

		if (dataAllocer)
		{
			delete pData;
			pData = nullptr;
		}
		return true;
	}

	bool ReadData()
	{
		return ReadData(ObjAddress);
	}

	int StructSize()
	{
		static JsonStruct jStruct = JsonReflector::GetStruct(TypeName());
		return jStruct.GetSize();
	}
};

// ToDo: Check what UProperty really need!!, then don't read anything u don't need
class UProperty : public UField
{
public:
	int32_t ArrayDim = 0;
	int32_t ElementSize = 0;
	FQWord PropertyFlags;
	int32_t Offset = 0;
	uintptr_t PropertyLinkNext = 0;
	uintptr_t NextRef = 0;
	uintptr_t DestructorLinkNext = 0;
	uintptr_t PostConstructLinkNext = 0;

	std::string TypeName() { return "UProperty"; }

	void FixPointers(const int fullStructSize)
	{
		UField::FixPointers(fullStructSize);
		Utils::FixPointers(this, fullStructSize, {
			OFFSET(PropertyLinkNext),
			OFFSET(NextRef),
			OFFSET(DestructorLinkNext),
			OFFSET(PostConstructLinkNext)
		});
	}

	bool ReadData(const uintptr_t objAddress)
	{
		// Set object remote address
		ObjAddress = objAddress;

		// Read this struct
		static JsonStruct jStruct = JsonReflector::GetStruct(TypeName());
		static int jSize = jStruct.GetSize();

		// Read Remote Memory
		bool dataAllocer = false;
		if (pData == nullptr)
		{
			dataAllocer = true;
			pData = new BYTE[jSize];

			// Read this struct
			if (m->ReadBytes(ObjAddress, pData, jSize) != jSize) return false;

			// Fix pointers for x32 games
			FixPointers(jSize);
		}

		// Init super variables first
		UField::ReadData(objAddress);

		// Init `this` variables
		FILL_DATA(pData, ArrayDim, jStruct["ArrayDim"].Offset);
		FILL_DATA(pData, ElementSize, jStruct["ElementSize"].Offset);
		FILL_DATA(pData, PropertyFlags, jStruct["PropertyFlags"].Offset);
		FILL_DATA(pData, Offset, jStruct["Offset"].Offset);
		FILL_DATA(pData, PropertyLinkNext, jStruct["PropertyLinkNext"].Offset);
		FILL_DATA(pData, NextRef, jStruct["NextRef"].Offset);
		FILL_DATA(pData, DestructorLinkNext, jStruct["DestructorLinkNext"].Offset);
		FILL_DATA(pData, PostConstructLinkNext, jStruct["PostConstructLinkNext"].Offset);

		if (dataAllocer)
		{
			delete pData;
			pData = nullptr;
		}
		return true;
	}

	bool ReadData()
	{
		return ReadData(ObjAddress);
	}

	int StructSize()
	{
		static JsonStruct jStruct = JsonReflector::GetStruct(TypeName());
		return jStruct.GetSize();
	}
};

class UNumericProperty : public UProperty
{
public:
	std::string TypeName() { return "UNumericProperty"; }

	void FixPointers(const int fullStructSize)
	{
		UProperty::FixPointers(fullStructSize);
	}

	bool ReadData(const uintptr_t objAddress)
	{
		// Set object remote address
		ObjAddress = objAddress;

		// Read this struct
		static JsonStruct jStruct = JsonReflector::GetStruct(TypeName());
		static int jSize = jStruct.GetSize();

		// Read Remote Memory
		bool dataAllocer = false;
		if (pData == nullptr)
		{
			dataAllocer = true;
			pData = new BYTE[jSize];

			// Read this struct
			if (m->ReadBytes(ObjAddress, pData, jSize) != jSize) return false;

			// Fix pointers for x32 games
			FixPointers(jSize);
		}

		// Init super variables first
		UProperty::ReadData(objAddress);

		if (dataAllocer)
		{
			delete pData;
			pData = nullptr;
		}
		return true;
	}

	bool ReadData()
	{
		return ReadData(ObjAddress);
	}

	int StructSize()
	{
		static JsonStruct jStruct = JsonReflector::GetStruct(TypeName());
		return jStruct.GetSize();
	}
};

class UByteProperty : public UNumericProperty
{
public:
	uintptr_t Enum = 0;

	std::string TypeName() { return "UByteProperty"; }

	void FixPointers(const int fullStructSize)
	{
		UNumericProperty::FixPointers(fullStructSize);
		Utils::FixPointers(this, fullStructSize, { OFFSET(Enum) });
	}

	bool ReadData(const uintptr_t objAddress)
	{
		// Set object remote address
		ObjAddress = objAddress;

		// Read this struct
		static JsonStruct jStruct = JsonReflector::GetStruct(TypeName());
		static int jSize = jStruct.GetSize();

		// Read Remote Memory
		bool dataAllocer = false;
		if (pData == nullptr)
		{
			dataAllocer = true;
			pData = new BYTE[jSize];

			// Read this struct
			if (m->ReadBytes(ObjAddress, pData, jSize) != jSize) return false;

			// Fix pointers for x32 games
			FixPointers(jSize);
		}

		// Init super variables first
		UNumericProperty::ReadData(objAddress);

		// Init `this` variables
		FILL_DATA(pData, Enum, jStruct["Enum"].Offset);

		if (dataAllocer)
		{
			delete pData;
			pData = nullptr;
		}
		return true;
	}

	bool ReadData()
	{
		return ReadData(ObjAddress);
	}

	int StructSize()
	{
		static JsonStruct jStruct = JsonReflector::GetStruct(TypeName());
		return jStruct.GetSize();
	}
};

class UUInt16Property : public UNumericProperty
{
public:
	std::string TypeName() { return "UUInt16Property"; }

	void FixPointers(const int fullStructSize)
	{
		UNumericProperty::FixPointers(fullStructSize);
	}

	bool ReadData(const uintptr_t objAddress)
	{
		// Set object remote address
		ObjAddress = objAddress;

		// Read this struct
		static JsonStruct jStruct = JsonReflector::GetStruct(TypeName());
		static int jSize = jStruct.GetSize();

		// Read Remote Memory
		bool dataAllocer = false;
		if (pData == nullptr)
		{
			dataAllocer = true;
			pData = new BYTE[jSize];

			// Read this struct
			if (m->ReadBytes(ObjAddress, pData, jSize) != jSize) return false;

			// Fix pointers for x32 games
			FixPointers(jSize);
		}

		// Init super variables first
		UNumericProperty::ReadData(objAddress);

		if (dataAllocer)
		{
			delete pData;
			pData = nullptr;
		}
		return true;
	}

	bool ReadData()
	{
		return ReadData(ObjAddress);
	}

	int StructSize()
	{
		static JsonStruct jStruct = JsonReflector::GetStruct(TypeName());
		return jStruct.GetSize();
	}
};

class UUInt32Property : public UNumericProperty
{
public:
	std::string TypeName() { return "UUInt32Property"; }

	void FixPointers(const int fullStructSize)
	{
		UNumericProperty::FixPointers(fullStructSize);
	}

	bool ReadData(const uintptr_t objAddress)
	{
		// Set object remote address
		ObjAddress = objAddress;

		// Read this struct
		static JsonStruct jStruct = JsonReflector::GetStruct(TypeName());
		static int jSize = jStruct.GetSize();

		// Read Remote Memory
		bool dataAllocer = false;
		if (pData == nullptr)
		{
			dataAllocer = true;
			pData = new BYTE[jSize];

			// Read this struct
			if (m->ReadBytes(ObjAddress, pData, jSize) != jSize) return false;

			// Fix pointers for x32 games
			FixPointers(jSize);
		}

		// Init super variables first
		UNumericProperty::ReadData(objAddress);

		if (dataAllocer)
		{
			delete pData;
			pData = nullptr;
		}
		return true;
	}

	bool ReadData()
	{
		return ReadData(ObjAddress);
	}

	int StructSize()
	{
		static JsonStruct jStruct = JsonReflector::GetStruct(TypeName());
		return jStruct.GetSize();
	}
};

class UUInt64Property : public UNumericProperty
{
public:
	std::string TypeName() { return "UUInt64Property"; }

	void FixPointers(const int fullStructSize)
	{
		UNumericProperty::FixPointers(fullStructSize);
	}

	bool ReadData(const uintptr_t objAddress)
	{
		// Set object remote address
		ObjAddress = objAddress;

		// Read this struct
		static JsonStruct jStruct = JsonReflector::GetStruct(TypeName());
		static int jSize = jStruct.GetSize();

		// Read Remote Memory
		bool dataAllocer = false;
		if (pData == nullptr)
		{
			dataAllocer = true;
			pData = new BYTE[jSize];

			// Read this struct
			if (m->ReadBytes(ObjAddress, pData, jSize) != jSize) return false;

			// Fix pointers for x32 games
			FixPointers(jSize);
		}

		// Init super variables first
		UNumericProperty::ReadData(objAddress);

		if (dataAllocer)
		{
			delete pData;
			pData = nullptr;
		}
		return true;
	}

	bool ReadData()
	{
		return ReadData(ObjAddress);
	}

	int StructSize()
	{
		static JsonStruct jStruct = JsonReflector::GetStruct(TypeName());
		return jStruct.GetSize();
	}
};

class UInt8Property : public UNumericProperty
{
public:
	std::string TypeName() { return "UInt8Property"; }

	void FixPointers(const int fullStructSize)
	{
		UNumericProperty::FixPointers(fullStructSize);
	}

	bool ReadData(const uintptr_t objAddress)
	{
		// Set object remote address
		ObjAddress = objAddress;

		// Read this struct
		static JsonStruct jStruct = JsonReflector::GetStruct(TypeName());
		static int jSize = jStruct.GetSize();

		// Read Remote Memory
		bool dataAllocer = false;
		if (pData == nullptr)
		{
			dataAllocer = true;
			pData = new BYTE[jSize];

			// Read this struct
			if (m->ReadBytes(ObjAddress, pData, jSize) != jSize) return false;

			// Fix pointers for x32 games
			FixPointers(jSize);
		}

		// Init super variables first
		UNumericProperty::ReadData(objAddress);

		if (dataAllocer)
		{
			delete pData;
			pData = nullptr;
		}
		return true;
	}

	bool ReadData()
	{
		return ReadData(ObjAddress);
	}

	int StructSize()
	{
		static JsonStruct jStruct = JsonReflector::GetStruct(TypeName());
		return jStruct.GetSize();
	}
};

class UInt16Property : public UNumericProperty
{
public:
	std::string TypeName() { return "UInt16Property"; }

	void FixPointers(const int fullStructSize)
	{
		UNumericProperty::FixPointers(fullStructSize);
	}

	bool ReadData(const uintptr_t objAddress)
	{
		// Set object remote address
		ObjAddress = objAddress;

		// Read this struct
		static JsonStruct jStruct = JsonReflector::GetStruct(TypeName());
		static int jSize = jStruct.GetSize();

		// Read Remote Memory
		bool dataAllocer = false;
		if (pData == nullptr)
		{
			dataAllocer = true;
			pData = new BYTE[jSize];

			// Read this struct
			if (m->ReadBytes(ObjAddress, pData, jSize) != jSize) return false;

			// Fix pointers for x32 games
			FixPointers(jSize);
		}

		// Init super variables first
		UNumericProperty::ReadData(objAddress);

		if (dataAllocer)
		{
			delete pData;
			pData = nullptr;
		}
		return true;
	}

	bool ReadData()
	{
		return ReadData(ObjAddress);
	}

	int StructSize()
	{
		static JsonStruct jStruct = JsonReflector::GetStruct(TypeName());
		return jStruct.GetSize();
	}
};

class UIntProperty : public UNumericProperty
{
public:
	std::string TypeName() { return "UIntProperty"; }

	void FixPointers(const int fullStructSize)
	{
		UNumericProperty::FixPointers(fullStructSize);
	}

	bool ReadData(const uintptr_t objAddress)
	{
		// Set object remote address
		ObjAddress = objAddress;

		// Read this struct
		static JsonStruct jStruct = JsonReflector::GetStruct(TypeName());
		static int jSize = jStruct.GetSize();

		// Read Remote Memory
		bool dataAllocer = false;
		if (pData == nullptr)
		{
			dataAllocer = true;
			pData = new BYTE[jSize];

			// Read this struct
			if (m->ReadBytes(ObjAddress, pData, jSize) != jSize) return false;

			// Fix pointers for x32 games
			FixPointers(jSize);
		// Read Remote Memory
		}

		// Init super variables first
		UNumericProperty::ReadData(objAddress);

		if (dataAllocer)
		{
			delete pData;
			pData = nullptr;
		}
		return true;
	}

	bool ReadData()
	{
		return ReadData(ObjAddress);
	}

	int StructSize()
	{
		static JsonStruct jStruct = JsonReflector::GetStruct(TypeName());
		return jStruct.GetSize();
	}
};

class UInt64Property : public UNumericProperty
{
public:
	std::string TypeName() { return "UInt64Property"; }

	void FixPointers(const int fullStructSize)
	{
		UNumericProperty::FixPointers(fullStructSize);
	}

	bool ReadData(const uintptr_t objAddress)
	{
		// Set object remote address
		ObjAddress = objAddress;

		// Read this struct
		static JsonStruct jStruct = JsonReflector::GetStruct(TypeName());
		static int jSize = jStruct.GetSize();

		// Read Remote Memory
		bool dataAllocer = false;
		if (pData == nullptr)
		{
			dataAllocer = true;
			pData = new BYTE[jSize];

			// Read this struct
			if (m->ReadBytes(ObjAddress, pData, jSize) != jSize) return false;

			// Fix pointers for x32 games
			FixPointers(jSize);
		}

		// Init super variables first
		UNumericProperty::ReadData(objAddress);

		if (dataAllocer)
		{
			delete pData;
			pData = nullptr;
		}
		return true;
	}

	bool ReadData()
	{
		return ReadData(ObjAddress);
	}

	int StructSize()
	{
		static JsonStruct jStruct = JsonReflector::GetStruct(TypeName());
		return jStruct.GetSize();
	}
};

class UFloatProperty : public UNumericProperty
{
public:
	std::string TypeName() { return "UFloatProperty"; }

	void FixPointers(const int fullStructSize)
	{
		UNumericProperty::FixPointers(fullStructSize);
	}

	bool ReadData(const uintptr_t objAddress)
	{
		// Set object remote address
		ObjAddress = objAddress;

		// Read this struct
		static JsonStruct jStruct = JsonReflector::GetStruct(TypeName());
		static int jSize = jStruct.GetSize();

		// Read Remote Memory
		bool dataAllocer = false;
		if (pData == nullptr)
		{
			dataAllocer = true;
			pData = new BYTE[jSize];

			// Read this struct
			if (m->ReadBytes(ObjAddress, pData, jSize) != jSize) return false;

			// Fix pointers for x32 games
			FixPointers(jSize);
		}

		// Init super variables first
		UNumericProperty::ReadData(objAddress);

		if (dataAllocer)
		{
			delete pData;
			pData = nullptr;
		}
		return true;
	}

	bool ReadData()
	{
		return ReadData(ObjAddress);
	}

	int StructSize()
	{
		static JsonStruct jStruct = JsonReflector::GetStruct(TypeName());
		return jStruct.GetSize();
	}
};

class UDoubleProperty : public UNumericProperty
{
public:
	std::string TypeName() { return "UDoubleProperty"; }

	void FixPointers(const int fullStructSize)
	{
		UNumericProperty::FixPointers(fullStructSize);
	}

	bool ReadData(const uintptr_t objAddress)
	{
		// Set object remote address
		ObjAddress = objAddress;

		// Read this struct
		static JsonStruct jStruct = JsonReflector::GetStruct(TypeName());
		static int jSize = jStruct.GetSize();

		// Read Remote Memory
		bool dataAllocer = false;
		if (pData == nullptr)
		{
			dataAllocer = true;
			pData = new BYTE[jSize];

			// Read this struct
			if (m->ReadBytes(ObjAddress, pData, jSize) != jSize) return false;

			// Fix pointers for x32 games
			FixPointers(jSize);
		}

		// Init super variables first
		UNumericProperty::ReadData(objAddress);

		if (dataAllocer)
		{
			delete pData;
			pData = nullptr;
		}
		return true;
	}

	bool ReadData()
	{
		return ReadData(ObjAddress);
	}

	int StructSize()
	{
		static JsonStruct jStruct = JsonReflector::GetStruct(TypeName());
		return jStruct.GetSize();
	}
};

class UBoolProperty : public UProperty
{
public:
	uint8_t FieldSize = 0;
	uint8_t ByteOffset = 0;
	uint8_t ByteMask = 0;
	uint8_t FieldMask = 0;

	std::string TypeName() { return "UBoolProperty"; }

	void FixPointers(const int fullStructSize)
	{
		UProperty::FixPointers(fullStructSize);
	}

	bool ReadData(const uintptr_t objAddress)
	{
		// Set object remote address
		ObjAddress = objAddress;

		// Read this struct
		static JsonStruct jStruct = JsonReflector::GetStruct(TypeName());
		static int jSize = jStruct.GetSize();

		// Read Remote Memory
		bool dataAllocer = false;
		if (pData == nullptr)
		{
			dataAllocer = true;
			pData = new BYTE[jSize];

			// Read this struct
			if (m->ReadBytes(ObjAddress, pData, jSize) != jSize) return false;

			// Fix pointers for x32 games
			FixPointers(jSize);
		}

		// Init super variables first
		UProperty::ReadData(objAddress);

		// Init `this` variables
		FILL_DATA(pData, FieldSize, jStruct["FieldSize"].Offset);
		FILL_DATA(pData, ByteOffset, jStruct["ByteOffset"].Offset);
		FILL_DATA(pData, ByteMask, jStruct["ByteMask"].Offset);
		FILL_DATA(pData, FieldMask, jStruct["FieldMask"].Offset);

		if (dataAllocer)
		{
			delete pData;
			pData = nullptr;
		}
		return true;
	}

	bool ReadData()
	{
		return ReadData(ObjAddress);
	}

	int StructSize()
	{
		static JsonStruct jStruct = JsonReflector::GetStruct(TypeName());
		return jStruct.GetSize();
	}
};

class UObjectPropertyBase : public UProperty
{
public:
	uintptr_t PropertyClass = 0;

	std::string TypeName() { return "UObjectPropertyBase"; }

	void FixPointers(const int fullStructSize)
	{
		UProperty::FixPointers(fullStructSize);
		Utils::FixPointers(this, fullStructSize, { OFFSET(PropertyClass) });
	}

	bool ReadData(const uintptr_t objAddress)
	{
		// Set object remote address
		ObjAddress = objAddress;

		// Read this struct
		static JsonStruct jStruct = JsonReflector::GetStruct(TypeName());
		static int jSize = jStruct.GetSize();

		// Read Remote Memory
		bool dataAllocer = false;
		if (pData == nullptr)
		{
			dataAllocer = true;
			pData = new BYTE[jSize];

			// Read this struct
			if (m->ReadBytes(ObjAddress, pData, jSize) != jSize) return false;

			// Fix pointers for x32 games
			FixPointers(jSize);
		}

		// Init super variables first
		UProperty::ReadData(objAddress);

		// Init `this` variables
		FILL_DATA(pData, PropertyClass, jStruct["PropertyClass"].Offset);

		if (dataAllocer)
		{
			delete pData;
			pData = nullptr;
		}
		return true;
	}

	bool ReadData()
	{
		return ReadData(ObjAddress);
	}

	int StructSize()
	{
		static JsonStruct jStruct = JsonReflector::GetStruct(TypeName());
		return jStruct.GetSize();
	}
};

class UObjectProperty : public UObjectPropertyBase
{
public:
	std::string TypeName() { return "UObjectProperty"; }

	void FixPointers(const int fullStructSize)
	{
		UObjectPropertyBase::FixPointers(fullStructSize);
	}

	bool ReadData(const uintptr_t objAddress)
	{
		// Set object remote address
		ObjAddress = objAddress;

		// Read this struct
		static JsonStruct jStruct = JsonReflector::GetStruct(TypeName());
		static int jSize = jStruct.GetSize();

		// Read Remote Memory
		bool dataAllocer = false;
		if (pData == nullptr)
		{
			dataAllocer = true;
			pData = new BYTE[jSize];

			// Read this struct
			if (m->ReadBytes(ObjAddress, pData, jSize) != jSize) return false;

			// Fix pointers for x32 games
			FixPointers(jSize);
		}

		// Init super variables first
		UObjectPropertyBase::ReadData(objAddress);

		if (dataAllocer)
		{
			delete pData;
			pData = nullptr;
		}
		return true;
	}

	bool ReadData()
	{
		return ReadData(ObjAddress);
	}

	int StructSize()
	{
		static JsonStruct jStruct = JsonReflector::GetStruct(TypeName());
		return jStruct.GetSize();
	}
};

class UClassProperty : public UObjectProperty
{
public:
	uintptr_t MetaClass = 0;

	std::string TypeName() { return "UClassProperty"; }

	void FixPointers(const int fullStructSize)
	{
		UObjectProperty::FixPointers(fullStructSize);
		Utils::FixPointers(this, fullStructSize, { OFFSET(MetaClass) });
	}

	bool ReadData(const uintptr_t objAddress)
	{
		// Set object remote address
		ObjAddress = objAddress;

		// Read this struct
		static JsonStruct jStruct = JsonReflector::GetStruct(TypeName());
		static int jSize = jStruct.GetSize();

		// Read Remote Memory
		bool dataAllocer = false;
		if (pData == nullptr)
		{
			dataAllocer = true;
			pData = new BYTE[jSize];

			// Read this struct
			if (m->ReadBytes(ObjAddress, pData, jSize) != jSize) return false;

			// Fix pointers for x32 games
			FixPointers(jSize);
		}

		// Init super variables first
		UObjectProperty::ReadData(objAddress);

		// Init `this` variables
		FILL_DATA(pData, MetaClass, jStruct["MetaClass"].Offset);

		if (dataAllocer)
		{
			delete pData;
			pData = nullptr;
		}
		return true;
	}

	bool ReadData()
	{
		return ReadData(ObjAddress);
	}

	int StructSize()
	{
		static JsonStruct jStruct = JsonReflector::GetStruct(TypeName());
		return jStruct.GetSize();
	}
};

class UInterfaceProperty : public UProperty
{
public:
	uintptr_t InterfaceClass = 0;

	std::string TypeName() { return "UInterfaceProperty"; }

	void FixPointers(const int fullStructSize)
	{
		UProperty::FixPointers(fullStructSize);
		Utils::FixPointers(this, fullStructSize, { OFFSET(InterfaceClass) });
	}

	bool ReadData(const uintptr_t objAddress)
	{
		// Set object remote address
		ObjAddress = objAddress;

		// Read this struct
		static JsonStruct jStruct = JsonReflector::GetStruct(TypeName());
		static int jSize = jStruct.GetSize();

		// Read Remote Memory
		bool dataAllocer = false;
		if (pData == nullptr)
		{
			dataAllocer = true;
			pData = new BYTE[jSize];

			// Read this struct
			if (m->ReadBytes(ObjAddress, pData, jSize) != jSize) return false;

			// Fix pointers for x32 games
			FixPointers(jSize);
		}

		// Init super variables first
		UProperty::ReadData(objAddress);

		// Init `this` variables
		FILL_DATA(pData, InterfaceClass, jStruct["InterfaceClass"].Offset);

		if (dataAllocer)
		{
			delete pData;
			pData = nullptr;
		}
		return true;
	}

	bool ReadData()
	{
		return ReadData(ObjAddress);
	}

	int StructSize()
	{
		static JsonStruct jStruct = JsonReflector::GetStruct(TypeName());
		return jStruct.GetSize();
	}
};

class UWeakObjectProperty : public UObjectPropertyBase
{
public:
	std::string TypeName() { return "UWeakObjectProperty"; }

	void FixPointers(const int fullStructSize)
	{
		UObjectPropertyBase::FixPointers(fullStructSize);
	}

	bool ReadData(const uintptr_t objAddress)
	{
		// Set object remote address
		ObjAddress = objAddress;

		// Read this struct
		static JsonStruct jStruct = JsonReflector::GetStruct(TypeName());
		static int jSize = jStruct.GetSize();

		// Read Remote Memory
		bool dataAllocer = false;
		if (pData == nullptr)
		{
			dataAllocer = true;
			pData = new BYTE[jSize];

			// Read this struct
			if (m->ReadBytes(ObjAddress, pData, jSize) != jSize) return false;

			// Fix pointers for x32 games
			FixPointers(jSize);
		}

		// Init super variables first
		UObjectPropertyBase::ReadData(objAddress);

		if (dataAllocer)
		{
			delete pData;
			pData = nullptr;
		}
		return true;
	}

	bool ReadData()
	{
		return ReadData(ObjAddress);
	}

	int StructSize()
	{
		static JsonStruct jStruct = JsonReflector::GetStruct(TypeName());
		return jStruct.GetSize();
	}
};

class ULazyObjectProperty : public UObjectPropertyBase
{
public:
	std::string TypeName() { return "ULazyObjectProperty"; }

	void FixPointers(const int fullStructSize)
	{
		UObjectPropertyBase::FixPointers(fullStructSize);
	}

	bool ReadData(const uintptr_t objAddress)
	{
		// Set object remote address
		ObjAddress = objAddress;

		// Read this struct
		static JsonStruct jStruct = JsonReflector::GetStruct(TypeName());
		static int jSize = jStruct.GetSize();

		// Read Remote Memory
		bool dataAllocer = false;
		if (pData == nullptr)
		{
			dataAllocer = true;
			pData = new BYTE[jSize];

			// Read this struct
			if (m->ReadBytes(ObjAddress, pData, jSize) != jSize) return false;

			// Fix pointers for x32 games
			FixPointers(jSize);
		}

		// Init super variables first
		UObjectPropertyBase::ReadData(objAddress);

		if (dataAllocer)
		{
			delete pData;
			pData = nullptr;
		}
		return true;
	}

	bool ReadData()
	{
		return ReadData(ObjAddress);
	}

	int StructSize()
	{
		static JsonStruct jStruct = JsonReflector::GetStruct(TypeName());
		return jStruct.GetSize();
	}
};

class UAssetObjectProperty : public UObjectPropertyBase
{
public:
	std::string TypeName() { return "UAssetObjectProperty"; }

	void FixPointers(const int fullStructSize)
	{
		UObjectPropertyBase::FixPointers(fullStructSize);
	}

	bool ReadData(const uintptr_t objAddress)
	{
		// Set object remote address
		ObjAddress = objAddress;

		// Read this struct
		static JsonStruct jStruct = JsonReflector::GetStruct(TypeName());
		static int jSize = jStruct.GetSize();

		// Read Remote Memory
		bool dataAllocer = false;
		if (pData == nullptr)
		{
			dataAllocer = true;
			pData = new BYTE[jSize];

			// Read this struct
			if (m->ReadBytes(ObjAddress, pData, jSize) != jSize) return false;

			// Fix pointers for x32 games
			FixPointers(jSize);
		}

		// Init super variables first
		UObjectPropertyBase::ReadData(objAddress);

		if (dataAllocer)
		{
			delete pData;
			pData = nullptr;
		}
		return true;
	}

	bool ReadData()
	{
		return ReadData(ObjAddress);
	}

	int StructSize()
	{
		static JsonStruct jStruct = JsonReflector::GetStruct(TypeName());
		return jStruct.GetSize();
	}
};

class UAssetClassProperty : public UAssetObjectProperty
{
public:
	uintptr_t MetaClass = 0;

	std::string TypeName() { return "UAssetClassProperty"; }

	void FixPointers(const int fullStructSize)
	{
		UAssetObjectProperty::FixPointers(fullStructSize);
		Utils::FixPointers(this, fullStructSize, { OFFSET(MetaClass) });
	}

	bool ReadData(const uintptr_t objAddress)
	{
		// Set object remote address
		ObjAddress = objAddress;

		// Read this struct
		static JsonStruct jStruct = JsonReflector::GetStruct(TypeName());
		static int jSize = jStruct.GetSize();

		// Read Remote Memory
		bool dataAllocer = false;
		if (pData == nullptr)
		{
			dataAllocer = true;
			pData = new BYTE[jSize];

			// Read this struct
			if (m->ReadBytes(ObjAddress, pData, jSize) != jSize) return false;

			// Fix pointers for x32 games
			FixPointers(jSize);
		}

		// Init super variables first
		UAssetObjectProperty::ReadData(objAddress);

		// Init `this` variables
		FILL_DATA(pData, MetaClass, jStruct["MetaClass"].Offset);

		if (dataAllocer)
		{
			delete pData;
			pData = nullptr;
		}
		return true;
	}

	bool ReadData()
	{
		return ReadData(ObjAddress);
	}

	int StructSize()
	{
		static JsonStruct jStruct = JsonReflector::GetStruct(TypeName());
		return jStruct.GetSize();
	}
};

class UNameProperty : public UProperty
{
public:
	std::string TypeName() { return "UNameProperty"; }

	void FixPointers(const int fullStructSize)
	{
		UProperty::FixPointers(fullStructSize);
	}

	bool ReadData(const uintptr_t objAddress)
	{
		// Set object remote address
		ObjAddress = objAddress;

		// Read this struct
		static JsonStruct jStruct = JsonReflector::GetStruct(TypeName());
		static int jSize = jStruct.GetSize();

		// Read Remote Memory
		bool dataAllocer = false;
		if (pData == nullptr)
		{
			dataAllocer = true;
			pData = new BYTE[jSize];

			// Read this struct
			if (m->ReadBytes(ObjAddress, pData, jSize) != jSize) return false;

			// Fix pointers for x32 games
			FixPointers(jSize);
		}

		// Init super variables first
		UProperty::ReadData(objAddress);

		if (dataAllocer)
		{
			delete pData;
			pData = nullptr;
		}
		return true;
	}

	bool ReadData()
	{
		return ReadData(ObjAddress);
	}

	int StructSize()
	{
		static JsonStruct jStruct = JsonReflector::GetStruct(TypeName());
		return jStruct.GetSize();
	}
};

class UStructProperty : public UProperty
{
public:
	uintptr_t Struct = 0;

	std::string TypeName() { return "UStructProperty"; }

	void FixPointers(const int fullStructSize)
	{
		UProperty::FixPointers(fullStructSize);
		Utils::FixPointers(this, fullStructSize, { OFFSET(Struct) });
	}

	bool ReadData(const uintptr_t objAddress)
	{
		// Set object remote address
		ObjAddress = objAddress;

		// Read this struct
		static JsonStruct jStruct = JsonReflector::GetStruct(TypeName());
		static int jSize = jStruct.GetSize();

		// Read Remote Memory
		bool dataAllocer = false;
		if (pData == nullptr)
		{
			dataAllocer = true;
			pData = new BYTE[jSize];

			// Read this struct
			if (m->ReadBytes(ObjAddress, pData, jSize) != jSize) return false;

			// Fix pointers for x32 games
			FixPointers(jSize);
		}

		// Init super variables first
		UProperty::ReadData(objAddress);

		// Init `this` variables
		FILL_DATA(pData, Struct, jStruct["Struct"].Offset);

		if (dataAllocer)
		{
			delete pData;
			pData = nullptr;
		}
		return true;
	}

	bool ReadData()
	{
		return ReadData(ObjAddress);
	}

	int StructSize()
	{
		static JsonStruct jStruct = JsonReflector::GetStruct(TypeName());
		return jStruct.GetSize();
	}
};

class UStrProperty : public UProperty
{
public:
	std::string TypeName() { return "UStrProperty"; }

	void FixPointers(const int fullStructSize)
	{
		UProperty::FixPointers(fullStructSize);
	}

	bool ReadData(const uintptr_t objAddress)
	{
		// Set object remote address
		ObjAddress = objAddress;

		// Read this struct
		static JsonStruct jStruct = JsonReflector::GetStruct(TypeName());
		static int jSize = jStruct.GetSize();

		// Read Remote Memory
		bool dataAllocer = false;
		if (pData == nullptr)
		{
			dataAllocer = true;
			pData = new BYTE[jSize];

			// Read this struct
			if (m->ReadBytes(ObjAddress, pData, jSize) != jSize) return false;

			// Fix pointers for x32 games
			FixPointers(jSize);
		}

		// Init super variables first
		UProperty::ReadData(objAddress);

		if (dataAllocer)
		{
			delete pData;
			pData = nullptr;
		}
		return true;
	}

	bool ReadData()
	{
		return ReadData(ObjAddress);
	}

	int StructSize()
	{
		static JsonStruct jStruct = JsonReflector::GetStruct(TypeName());
		return jStruct.GetSize();
	}
};

class UTextProperty : public UProperty
{
public:
	std::string TypeName() { return "UTextProperty"; }

	void FixPointers(const int fullStructSize)
	{
		UProperty::FixPointers(fullStructSize);
	}

	bool ReadData(const uintptr_t objAddress)
	{
		// Set object remote address
		ObjAddress = objAddress;

		// Read this struct
		static JsonStruct jStruct = JsonReflector::GetStruct(TypeName());
		static int jSize = jStruct.GetSize();

		// Read Remote Memory
		bool dataAllocer = false;
		if (pData == nullptr)
		{
			dataAllocer = true;
			pData = new BYTE[jSize];

			// Read this struct
			if (m->ReadBytes(ObjAddress, pData, jSize) != jSize) return false;

			// Fix pointers for x32 games
			FixPointers(jSize);
		}

		// Init super variables first
		UProperty::ReadData(objAddress);

		if (dataAllocer)
		{
			delete pData;
			pData = nullptr;
		}
		return true;
	}

	bool ReadData()
	{
		return ReadData(ObjAddress);
	}

	int StructSize()
	{
		static JsonStruct jStruct = JsonReflector::GetStruct(TypeName());
		return jStruct.GetSize();
	}
};

class UArrayProperty : public UProperty
{
public:
	uintptr_t Inner = 0;

	std::string TypeName() { return "UArrayProperty"; }

	void FixPointers(const int fullStructSize)
	{
		UProperty::FixPointers(fullStructSize);
		Utils::FixPointers(this, fullStructSize, { OFFSET(Inner) });
	}

	bool ReadData(const uintptr_t objAddress)
	{
		// Set object remote address
		ObjAddress = objAddress;

		// Read this struct
		static JsonStruct jStruct = JsonReflector::GetStruct(TypeName());
		static int jSize = jStruct.GetSize();

		// Read Remote Memory
		bool dataAllocer = false;
		if (pData == nullptr)
		{
			dataAllocer = true;
			pData = new BYTE[jSize];

			// Read this struct
			if (m->ReadBytes(ObjAddress, pData, jSize) != jSize) return false;

			// Fix pointers for x32 games
			FixPointers(jSize);
		}

		// Init super variables first
		UProperty::ReadData(objAddress);

		// Init `this` variables
		FILL_DATA(pData, Inner, jStruct["Inner"].Offset);

		if (dataAllocer)
		{
			delete pData;
			pData = nullptr;
		}
		return true;
	}

	bool ReadData()
	{
		return ReadData(ObjAddress);
	}

	int StructSize()
	{
		static JsonStruct jStruct = JsonReflector::GetStruct(TypeName());
		return jStruct.GetSize();
	}
};

class UMapProperty : public UProperty
{
public:
	uintptr_t KeyProp = 0;
	uintptr_t ValueProp = 0;

	std::string TypeName() { return "UMapProperty"; }

	void FixPointers(const int fullStructSize)
	{
		UProperty::FixPointers(fullStructSize);
		Utils::FixPointers(this, fullStructSize, {
			OFFSET(KeyProp),
			OFFSET(ValueProp)
		});
	}

	bool ReadData(const uintptr_t objAddress)
	{
		// Set object remote address
		ObjAddress = objAddress;

		// Read this struct
		static JsonStruct jStruct = JsonReflector::GetStruct(TypeName());
		static int jSize = jStruct.GetSize();

		// Read Remote Memory
		bool dataAllocer = false;
		if (pData == nullptr)
		{
			dataAllocer = true;
			pData = new BYTE[jSize];

			// Read this struct
			if (m->ReadBytes(ObjAddress, pData, jSize) != jSize) return false;

			// Fix pointers for x32 games
			FixPointers(jSize);
		}

		// Init super variables first
		UProperty::ReadData(objAddress);

		// Init `this` variables
		FILL_DATA(pData, KeyProp, jStruct["KeyProp"].Offset);
		FILL_DATA(pData, ValueProp, jStruct["ValueProp"].Offset);

		if (dataAllocer)
		{
			delete pData;
			pData = nullptr;
		}
		return true;
	}

	bool ReadData()
	{
		return ReadData(ObjAddress);
	}

	int StructSize()
	{
		static JsonStruct jStruct = JsonReflector::GetStruct(TypeName());
		return jStruct.GetSize();
	}
};

class UDelegateProperty : public UProperty
{
public:
	uintptr_t SignatureFunction = 0;

	std::string TypeName() { return "UDelegateProperty"; }

	void FixPointers(const int fullStructSize)
	{
		UProperty::FixPointers(fullStructSize);
		Utils::FixPointers(this, fullStructSize, { OFFSET(SignatureFunction) });
	}

	bool ReadData(const uintptr_t objAddress)
	{
		// Set object remote address
		ObjAddress = objAddress;

		// Read this struct
		static JsonStruct jStruct = JsonReflector::GetStruct(TypeName());
		static int jSize = jStruct.GetSize();

		// Read Remote Memory
		bool dataAllocer = false;
		if (pData == nullptr)
		{
			dataAllocer = true;
			pData = new BYTE[jSize];

			// Read this struct
			if (m->ReadBytes(ObjAddress, pData, jSize) != jSize) return false;

			// Fix pointers for x32 games
			FixPointers(jSize);
		}

		// Init super variables first
		UProperty::ReadData(objAddress);

		// Init `this` variables
		FILL_DATA(pData, SignatureFunction, jStruct["SignatureFunction"].Offset);

		if (dataAllocer)
		{
			delete pData;
			pData = nullptr;
		}
		return true;
	}

	bool ReadData()
	{
		return ReadData(ObjAddress);
	}

	int StructSize()
	{
		static JsonStruct jStruct = JsonReflector::GetStruct(TypeName());
		return jStruct.GetSize();
	}
};

class UMulticastDelegateProperty : public UProperty
{
public:
	uintptr_t SignatureFunction = 0;

	std::string TypeName() { return "UMulticastDelegateProperty"; }

	void FixPointers(const int fullStructSize)
	{
		UProperty::FixPointers(fullStructSize);
		Utils::FixPointers(this, fullStructSize, { OFFSET(SignatureFunction) });
	}

	bool ReadData(const uintptr_t objAddress)
	{
		// Set object remote address
		ObjAddress = objAddress;

		// Read this struct
		static JsonStruct jStruct = JsonReflector::GetStruct(TypeName());
		static int jSize = jStruct.GetSize();

		// Read Remote Memory
		bool dataAllocer = false;
		if (pData == nullptr)
		{
			dataAllocer = true;
			pData = new BYTE[jSize];

			// Read this struct
			if (m->ReadBytes(ObjAddress, pData, jSize) != jSize) return false;

			// Fix pointers for x32 games
			FixPointers(jSize);
		}

		// Init super variables first
		UProperty::ReadData(objAddress);

		// Init `this` variables
		FILL_DATA(pData, SignatureFunction, jStruct["SignatureFunction"].Offset);

		if (dataAllocer)
		{
			delete pData;
			pData = nullptr;
		}
		return true;
	}

	bool ReadData()
	{
		return ReadData(ObjAddress);
	}

	int StructSize()
	{
		static JsonStruct jStruct = JsonReflector::GetStruct(TypeName());
		return jStruct.GetSize();
	}
};

class UEnumProperty : public UProperty
{
public:
	uintptr_t UnderlyingProp = 0;
	uintptr_t Enum = 0;

	std::string TypeName() { return "UEnumProperty"; }

	void FixPointers(const int fullStructSize)
	{
		UProperty::FixPointers(fullStructSize);
		Utils::FixPointers(this, fullStructSize, {
			OFFSET(UnderlyingProp),
			OFFSET(Enum)
		});
	}

	bool ReadData(const uintptr_t objAddress)
	{
		// Set object remote address
		ObjAddress = objAddress;

		// Read this struct
		static JsonStruct jStruct = JsonReflector::GetStruct(TypeName());
		static int jSize = jStruct.GetSize();

		// Read Remote Memory
		bool dataAllocer = false;
		if (pData == nullptr)
		{
			dataAllocer = true;
			pData = new BYTE[jSize];

			// Read this struct
			if (m->ReadBytes(ObjAddress, pData, jSize) != jSize) return false;

			// Fix pointers for x32 games
			FixPointers(jSize);
		}

		// Init super variables first
		UProperty::ReadData(objAddress);

		// Init `this` variables
		FILL_DATA(pData, UnderlyingProp, jStruct["UnderlyingProp"].Offset);
		FILL_DATA(pData, Enum, jStruct["Enum"].Offset);

		if (dataAllocer)
		{
			delete pData;
			pData = nullptr;
		}

		return true;
	}

	bool ReadData()
	{
		return ReadData(ObjAddress);
	}

	int StructSize()
	{
		static JsonStruct jStruct = JsonReflector::GetStruct(TypeName());
		return jStruct.GetSize();
	}
};