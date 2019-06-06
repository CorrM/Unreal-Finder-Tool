#pragma once
#pragma warning(disable: 4302)
#pragma warning(disable: 4311)

#include <string>
#include <windows.h>
#include "../Utils.h"
#include "../JsonReflector.h"

// #define OFFSET(s, m) ((size_t)&reinterpret_cast<char const volatile&>((((s*)0)->m)))
#define OFFSET(m) ((int)&reinterpret_cast<char const volatile&>((((decltype(this))0)->m)))
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
		m->ReadBytes(objAddress, jStruct["ObjectPointer"], &ObjectPointer);
		m->ReadBytes(objAddress, jStruct["InterfacePointer"], &InterfacePointer);

		// Fix pointer for x32 tool version
		FixPointers(jStruct.GetSize());

		// It's Initialized
		init = true;

		// Return next var offset
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
		// 
	}

	bool ReadData(const uintptr_t objAddress)
	{
		ObjAddress = objAddress;

		if (ObjAddress == NULL)
			return false;

		// Read this struct
		static JsonStruct jStruct = JsonReflector::GetStruct(TypeName());
		m->ReadBytes(objAddress, jStruct["Object"], &Object);
		m->ReadBytes(objAddress, jStruct["Flags"], &Flags);
		m->ReadBytes(objAddress, jStruct["ClusterIndex"], &ClusterIndex);
		m->ReadBytes(objAddress, jStruct["SerialNumber"], &SerialNumber);

		// Fix pointer for x32 tool version
		FixPointers(jStruct.GetSize());

		// It's Initialized
		init = true;

		// Return next var offset
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

class FNameEntity
{
	Memory* m = Utils::MemoryObj;

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
		m->ReadBytes(objAddress, jStruct["Index"], &Index);
		AnsiName = m->ReadText(objAddress + ansiNameOffset);

		// Fix pointer for x32 tool version
		FixPointers(jStruct.GetSize());

		return true;
	}

	bool ReadData(const int ansiNameOffset)
	{
		return ReadData(ObjAddress, ansiNameOffset);
	}
};

class UObject
{
	bool init = false;
protected:
	Memory* m = Utils::MemoryObj;

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

		// Read this struct
		m->ReadBytes(ObjAddress, jStruct["VfTable"], &VfTable);
		m->ReadBytes(ObjAddress, jStruct["Flags"], &Flags);
		m->ReadBytes(ObjAddress, jStruct["InternalIndex"], &InternalIndex);
		m->ReadBytes(ObjAddress, jStruct["Class"], &Class);
		m->ReadBytes(ObjAddress, jStruct["Name"], &Name);
		m->ReadBytes(ObjAddress, jStruct["Outer"], &Outer);

		// Fix pointer for x32 tool version
		FixPointers(jStruct.GetSize());

		// It's initialized
		init = true;

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
		// Read super first
		UObject::ReadData(objAddress);

		// Read this struct
		static JsonStruct jStruct = JsonReflector::GetStruct(TypeName());
		m->ReadBytes(ObjAddress, jStruct["Next"], &Next);

		// Fix pointer for x32 tool version
		FixPointers(jStruct.GetSize());

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
		// Read super first
		UField::ReadData(objAddress);

		// Read this struct
		static JsonStruct jStruct = JsonReflector::GetStruct(TypeName());
		m->ReadBytes(ObjAddress, jStruct["CppType"], &CppType);
		m->ReadBytes(ObjAddress, jStruct["Names"], &Names);
		m->ReadBytes(ObjAddress, jStruct["CppForm"], &CppForm);

		// Fix pointer for x32 tool version
		FixPointers(jStruct.GetSize());

		// Return next var offset
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
		// Read super first
		UField::ReadData(objAddress);

		// Read this struct
		static JsonStruct jStruct = JsonReflector::GetStruct(TypeName());
		m->ReadBytes(ObjAddress, jStruct["SuperField"], &SuperField);
		m->ReadBytes(ObjAddress, jStruct["Children"], &Children);
		m->ReadBytes(ObjAddress, jStruct["PropertySize"], &PropertySize);
		m->ReadBytes(ObjAddress, jStruct["MinAlignment"], &MinAlignment);

		// Fix pointer for x32 tool version
		FixPointers(jStruct.GetSize());

		// Return next var offset
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
		// Read super first
		UStruct::ReadData(objAddress);

		// Read this struct
		static JsonStruct jStruct = JsonReflector::GetStruct(TypeName());

		// Fix pointer for x32 tool version
		FixPointers(jStruct.GetSize());

		// Return next var offset
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
		// Read super first
		UStruct::ReadData(objAddress);

		// Read this struct
		static JsonStruct jStruct = JsonReflector::GetStruct(TypeName());
		m->ReadBytes(ObjAddress, jStruct["FunctionFlags"], &FunctionFlags);
		m->ReadBytes(ObjAddress, jStruct["NumParms"], &NumParms);
		m->ReadBytes(ObjAddress, jStruct["ParmsSize"], &ParmsSize);
		m->ReadBytes(ObjAddress, jStruct["ReturnValueOffset"], &ReturnValueOffset);
		m->ReadBytes(ObjAddress, jStruct["RPCId"], &RPCId);
		m->ReadBytes(ObjAddress, jStruct["RPCResponseId"], &RPCResponseId);
		m->ReadBytes(ObjAddress, jStruct["FirstPropertyToInit"], &FirstPropertyToInit);
		m->ReadBytes(ObjAddress, jStruct["EventGraphFunction"], &EventGraphFunction);
		m->ReadBytes(ObjAddress, jStruct["EventGraphCallOffset"], &EventGraphCallOffset);
		m->ReadBytes(ObjAddress, jStruct["Func"], &Func);

		// Fix pointer for x32 tool version
		FixPointers(jStruct.GetSize());

		// Return next var offset
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
		// Read super first
		UStruct::ReadData(objAddress);

		// Read this struct
		static JsonStruct jStruct = JsonReflector::GetStruct(TypeName());

		// Fix pointer for x32 tool version
		FixPointers(jStruct.GetSize());

		// Return next var offset
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
		// Read super first
		UField::ReadData(objAddress);

		// Read this struct
		static JsonStruct jStruct = JsonReflector::GetStruct(TypeName());
		m->ReadBytes(ObjAddress, jStruct["ArrayDim"], &ArrayDim);
		m->ReadBytes(ObjAddress, jStruct["ElementSize"], &ElementSize);
		m->ReadBytes(ObjAddress, jStruct["PropertyFlags"], &PropertyFlags);
		m->ReadBytes(ObjAddress, jStruct["Offset"], &Offset);
		m->ReadBytes(ObjAddress, jStruct["PropertyLinkNext"], &PropertyLinkNext);
		m->ReadBytes(ObjAddress, jStruct["NextRef"], &NextRef);
		m->ReadBytes(ObjAddress, jStruct["DestructorLinkNext"], &DestructorLinkNext);
		m->ReadBytes(ObjAddress, jStruct["PostConstructLinkNext"], &PostConstructLinkNext);

		// Fix pointer for x32 tool version
		FixPointers(jStruct.GetSize());

		// Return next var offset
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
		// Read super first
		UProperty::ReadData(objAddress);

		// Read this struct
		static JsonStruct jStruct = JsonReflector::GetStruct(TypeName());

		// Fix pointer for x32 tool version
		FixPointers(jStruct.GetSize());

		// Return next var offset
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
		// Read super first
		UNumericProperty::ReadData(objAddress);

		// Read this struct
		static JsonStruct jStruct = JsonReflector::GetStruct(TypeName());
		m->ReadBytes(ObjAddress, jStruct["Enum"], &Enum);

		// Fix pointer for x32 tool version
		FixPointers(jStruct.GetSize());

		// Return next var offset
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
		// Read super first
		UNumericProperty::ReadData(objAddress);

		// Read this struct
		static JsonStruct jStruct = JsonReflector::GetStruct(TypeName());

		// Fix pointer for x32 tool version
		FixPointers(jStruct.GetSize());

		// Return next var offset
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
		// Read super first
		UNumericProperty::ReadData(objAddress);

		// Read this struct
		static JsonStruct jStruct = JsonReflector::GetStruct(TypeName());

		// Fix pointer for x32 tool version
		FixPointers(jStruct.GetSize());

		// Return next var offset
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
		// Read super first
		UNumericProperty::ReadData(objAddress);

		// Read this struct
		static JsonStruct jStruct = JsonReflector::GetStruct(TypeName());

		// Fix pointer for x32 tool version
		FixPointers(jStruct.GetSize());

		// Return next var offset
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
		// Read super first
		UNumericProperty::ReadData(objAddress);

		// Read this struct
		static JsonStruct jStruct = JsonReflector::GetStruct(TypeName());

		// Fix pointer for x32 tool version
		FixPointers(jStruct.GetSize());

		// Return next var offset
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
		// Read super first
		UNumericProperty::ReadData(objAddress);

		// Read this struct
		static JsonStruct jStruct = JsonReflector::GetStruct(TypeName());

		// Fix pointer for x32 tool version
		FixPointers(jStruct.GetSize());

		// Return next var offset
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
		// Read super first
		UNumericProperty::ReadData(objAddress);

		// Read this struct
		static JsonStruct jStruct = JsonReflector::GetStruct(TypeName());

		// Fix pointer for x32 tool version
		FixPointers(jStruct.GetSize());

		// Return next var offset
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
		// Read super first
		UNumericProperty::ReadData(objAddress);

		// Read this struct
		static JsonStruct jStruct = JsonReflector::GetStruct(TypeName());

		// Fix pointer for x32 tool version
		FixPointers(jStruct.GetSize());

		// Return next var offset
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
		// Read super first
		UNumericProperty::ReadData(objAddress);

		// Read this struct
		static JsonStruct jStruct = JsonReflector::GetStruct(TypeName());

		// Fix pointer for x32 tool version
		FixPointers(jStruct.GetSize());

		// Return next var offset
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
		// Read super first
		UNumericProperty::ReadData(objAddress);

		// Read this struct
		static JsonStruct jStruct = JsonReflector::GetStruct(TypeName());

		// Fix pointer for x32 tool version
		FixPointers(jStruct.GetSize());

		// Return next var offset
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
		// Read super first
		UProperty::ReadData(objAddress);

		// Read this struct
		static JsonStruct jStruct = JsonReflector::GetStruct(TypeName());
		m->ReadBytes(ObjAddress, jStruct["FieldSize"], &FieldSize);
		m->ReadBytes(ObjAddress, jStruct["ByteOffset"], &ByteOffset);
		m->ReadBytes(ObjAddress, jStruct["ByteMask"], &ByteMask);
		m->ReadBytes(ObjAddress, jStruct["FieldMask"], &FieldMask);

		// Fix pointer for x32 tool version
		FixPointers(jStruct.GetSize());

		// Return next var offset
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
		// Read super first
		UProperty::ReadData(objAddress);

		// Read this struct
		static JsonStruct jStruct = JsonReflector::GetStruct(TypeName());
		m->ReadBytes(ObjAddress, jStruct["PropertyClass"], &PropertyClass);

		// Fix pointer for x32 tool version
		FixPointers(jStruct.GetSize());

		// Return next var offset
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
		// Read super first
		UObjectPropertyBase::ReadData(objAddress);

		// Read this struct
		static JsonStruct jStruct = JsonReflector::GetStruct(TypeName());

		// Fix pointer for x32 tool version
		FixPointers(jStruct.GetSize());

		// Return next var offset
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
		// Read super first
		UObjectProperty::ReadData(objAddress);

		// Read this struct
		static JsonStruct jStruct = JsonReflector::GetStruct(TypeName());
		m->ReadBytes(ObjAddress, jStruct["MetaClass"], &MetaClass);

		// Fix pointer for x32 tool version
		FixPointers(jStruct.GetSize());

		// Return next var offset
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
		// Read super first
		UProperty::ReadData(objAddress);

		// Read this struct
		static JsonStruct jStruct = JsonReflector::GetStruct(TypeName());
		m->ReadBytes(ObjAddress, jStruct["InterfaceClass"], &InterfaceClass);

		// Fix pointer for x32 tool version
		FixPointers(jStruct.GetSize());

		// Return next var offset
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
		// Read super first
		UObjectPropertyBase::ReadData(objAddress);

		// Read this struct
		static JsonStruct jStruct = JsonReflector::GetStruct(TypeName());

		// Fix pointer for x32 tool version
		FixPointers(jStruct.GetSize());

		// Return next var offset
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
		// Read super first
		UObjectPropertyBase::ReadData(objAddress);

		// Read this struct
		static JsonStruct jStruct = JsonReflector::GetStruct(TypeName());

		// Fix pointer for x32 tool version
		FixPointers(jStruct.GetSize());

		// Return next var offset
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
		// Read super first
		UObjectPropertyBase::ReadData(objAddress);

		// Read this struct
		static JsonStruct jStruct = JsonReflector::GetStruct(TypeName());

		// Fix pointer for x32 tool version
		FixPointers(jStruct.GetSize());

		// Return next var offset
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
		// Read super first
		UAssetObjectProperty::ReadData(objAddress);

		// Read this struct
		static JsonStruct jStruct = JsonReflector::GetStruct(TypeName());
		m->ReadBytes(ObjAddress, jStruct["MetaClass"], &MetaClass);

		// Fix pointer for x32 tool version
		FixPointers(jStruct.GetSize());

		// Return next var offset
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
		// Read super first
		UProperty::ReadData(objAddress);

		// Read this struct
		static JsonStruct jStruct = JsonReflector::GetStruct(TypeName());

		// Fix pointer for x32 tool version
		FixPointers(jStruct.GetSize());

		// Return next var offset
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
		// Read super first
		UProperty::ReadData(objAddress);

		// Read this struct
		static JsonStruct jStruct = JsonReflector::GetStruct(TypeName());
		m->ReadBytes(ObjAddress, jStruct["Struct"], &Struct);

		// Fix pointer for x32 tool version
		FixPointers(jStruct.GetSize());

		// Return next var offset
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
		// Read super first
		UProperty::ReadData(objAddress);

		// Read this struct
		static JsonStruct jStruct = JsonReflector::GetStruct(TypeName());

		// Fix pointer for x32 tool version
		FixPointers(jStruct.GetSize());

		// Return next var offset
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
		// Read super first
		UProperty::ReadData(objAddress);

		// Read this struct
		static JsonStruct jStruct = JsonReflector::GetStruct(TypeName());

		// Fix pointer for x32 tool version
		FixPointers(jStruct.GetSize());

		// Return next var offset
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
		// Read super first
		UProperty::ReadData(objAddress);

		// Read this struct
		static JsonStruct jStruct = JsonReflector::GetStruct(TypeName());
		m->ReadBytes(ObjAddress, jStruct["Inner"], &Inner);

		// Fix pointer for x32 tool version
		FixPointers(jStruct.GetSize());

		// Return next var offset
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
		// Read super first
		UProperty::ReadData(objAddress);

		// Read this struct
		static JsonStruct jStruct = JsonReflector::GetStruct(TypeName());
		m->ReadBytes(ObjAddress, jStruct["KeyProp"], &KeyProp);
		m->ReadBytes(ObjAddress, jStruct["ValueProp"], &ValueProp);

		// Fix pointer for x32 tool version
		FixPointers(jStruct.GetSize());

		// Return next var offset
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
		// Read super first
		UProperty::ReadData(objAddress);

		// Read this struct
		static JsonStruct jStruct = JsonReflector::GetStruct(TypeName());
		m->ReadBytes(ObjAddress, jStruct["SignatureFunction"], &SignatureFunction);

		// Fix pointer for x32 tool version
		FixPointers(jStruct.GetSize());

		// Return next var offset
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
		// Read super first
		UProperty::ReadData(objAddress);

		// Read this struct
		static JsonStruct jStruct = JsonReflector::GetStruct(TypeName());
		m->ReadBytes(ObjAddress, jStruct["SignatureFunction"], &SignatureFunction);

		// Fix pointer for x32 tool version
		FixPointers(jStruct.GetSize());

		// Return next var offset
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
		// Read super first
		UProperty::ReadData(objAddress);

		// Read this struct
		static JsonStruct jStruct = JsonReflector::GetStruct(TypeName());
		m->ReadBytes(ObjAddress, jStruct["UnderlyingProp"], &UnderlyingProp);
		m->ReadBytes(ObjAddress, jStruct["Enum"], &Enum);

		// Fix pointer for x32 tool version
		FixPointers(jStruct.GetSize());

		// Return next var offset
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