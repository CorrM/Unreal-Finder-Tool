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
	int A;
	int B;
};

struct FName
{
	int32_t ComparisonIndex;
	int32_t Number;

	FName(): ComparisonIndex(0), Number(0)
	{
	}

	FName(const int32_t index, const int32_t number): ComparisonIndex(index), Number(number)
	{
	}
};

struct TArray
{
	uintptr_t Data = NULL;
	int32_t Count = 0;
	int32_t Max = 0;

	friend struct FString;

	size_t Num() const
	{
		return Count;
	};

	bool IsValidIndex(const size_t i) const
	{
		return i < Num();
	}

	void FixPointers(const int fullStructSize)
	{
		Utils::FixPointers(this, fullStructSize, { OFFSET(Data) });
	}
};

template<typename KeyType, typename ValueType>
class TPair
{
public:
	KeyType   Key;
	ValueType Value;
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

class FScriptInterface
{
	UObject* ObjectPointer = nullptr;
	void* InterfacePointer = nullptr;

public:
	UObject* GetObj() const
	{
		return ObjectPointer;
	}

	UObject*& GetObjectRef()
	{
		return ObjectPointer;
	}

	void* GetInterface() const
	{
		return ObjectPointer != nullptr ? InterfacePointer : nullptr;
	}

	void FixPointers(const int fullStructSize)
	{
		// Utils::FixPointers(this, fullStructSize, { OFFSET(ObjectPointer), OFFSET(InterfacePointer) });
	}
};

template<class InterfaceType>
class TScriptInterface : public FScriptInterface
{
public:
	InterfaceType* operator->() const
	{
		return reinterpret_cast<InterfaceType*>(GetInterface());
	}

	InterfaceType& operator*() const
	{
		return *reinterpret_cast<InterfaceType*>(GetInterface());
	}

	operator bool() const
	{
		return GetInterface() != nullptr;
	}

	void FixPointers(const int fullStructSize)
	{
		FScriptInterface::FixPointers(fullStructSize);
		// Utils::FixPointers(this, fullStructSize, { OFFSET(Data) });
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

template<typename TObjectID>
class TPersistentObjectPtr
{
public:
	FWeakObjectPtr WeakPtr;
	int32_t TagAtLastTest;
	TObjectID ObjectID;

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
		// Utils::FixPointers(this, fullStructSize, { OFFSET(Data) });
	}
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

class FLazyObjectPtr : public TPersistentObjectPtr<FUniqueObjectGuid>
{
public:
	void FixPointers(const int fullStructSize)
	{
		TPersistentObjectPtr::FixPointers(fullStructSize);
		// Utils::FixPointers(this, fullStructSize, { OFFSET(Data) });
	}
};

struct FScriptDelegate
{
	unsigned char UnknownData[20];
};

struct FScriptMulticastDelegate
{
	unsigned char UnknownData[16];
};

class FUEnumItem
{
public:
	FName Key;
	uint64_t Value = 0;
};

class UObject
{
protected:
	Memory* m = Utils::MemoryObj;

public:
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

	int ReadData()
	{
		if (ObjAddress == NULL)
			throw std::exception("ObjAddress cant be zero !!");

		static JsonStruct jStruct = JsonReflector::GetStruct(TypeName());

		// Read this struct
		m->ReadBytes(ObjAddress, jStruct["VfTable"], &VfTable);
		m->ReadBytes(ObjAddress, jStruct["Flags"], &Flags);
		m->ReadBytes(ObjAddress, jStruct["InternalIndex"], &InternalIndex);
		m->ReadBytes(ObjAddress, jStruct["Class"], &Class);
		m->ReadBytes(ObjAddress, jStruct["Name"], &Name);
		m->ReadBytes(ObjAddress, jStruct["Outer"], &Outer);

		// Fix pointer for x32 tool version
		FixPointers(jStruct.StructSize);

		return jStruct.StructSize;
	}

	bool IsEqual(const UObject& toCheck) const
	{
		return toCheck.InternalIndex == this->InternalIndex && toCheck.Name.ComparisonIndex == this->Name.ComparisonIndex;
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
		{
			castType.ObjAddress = ObjAddress;
			castType.ReadData();
		}

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

	int ReadData()
	{
		// Read super first
		int lastOff = UObject::ReadData();

		// Read this struct
		static JsonStruct jStruct = JsonReflector::GetStruct(TypeName());
		m->ReadBytes(ObjAddress + lastOff, jStruct["Next"], &Next);

		// Fix pointer for x32 tool version
		FixPointers(jStruct.StructSize);

		return lastOff + jStruct.StructSize;
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

	int ReadData()
	{
		// Read super first
		int lastOff = UField::ReadData();

		// Read this struct
		static JsonStruct jStruct = JsonReflector::GetStruct(TypeName());
		m->ReadBytes(ObjAddress + lastOff, jStruct["CppType"], &CppType);
		m->ReadBytes(ObjAddress + lastOff, jStruct["Names"], &Names);
		m->ReadBytes(ObjAddress + lastOff , jStruct["CppForm"], &CppForm);

		// Fix pointer for x32 tool version
		FixPointers(jStruct.StructSize);

		// Return next var offset
		return lastOff + jStruct.StructSize;
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

	int ReadData()
	{
		// Read super first
		int lastOff = UField::ReadData();

		// Read this struct
		static JsonStruct jStruct = JsonReflector::GetStruct(TypeName());
		m->ReadBytes(ObjAddress + lastOff, jStruct["SuperField"], &SuperField);
		m->ReadBytes(ObjAddress + lastOff, jStruct["Children"], &Children);
		m->ReadBytes(ObjAddress + lastOff, jStruct["PropertySize"], &PropertySize);
		m->ReadBytes(ObjAddress + lastOff, jStruct["MinAlignment"], &MinAlignment);

		// Fix pointer for x32 tool version
		FixPointers(jStruct.StructSize);

		// Return next var offset
		return lastOff + jStruct.StructSize;
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

	int ReadData()
	{
		// Read super first
		int lastOff = UStruct::ReadData();

		// Read this struct
		static JsonStruct jStruct = JsonReflector::GetStruct(TypeName());

		// Fix pointer for x32 tool version
		FixPointers(jStruct.StructSize);

		// Return next var offset
		return lastOff + jStruct.StructSize;
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

	int ReadData()
	{
		// Read super first
		int lastOff = UStruct::ReadData();

		// Read this struct
		static JsonStruct jStruct = JsonReflector::GetStruct(TypeName());
		m->ReadBytes(ObjAddress + lastOff, jStruct["FunctionFlags"], &FunctionFlags);
		m->ReadBytes(ObjAddress + lastOff, jStruct["NumParms"], &NumParms);
		m->ReadBytes(ObjAddress + lastOff, jStruct["ParmsSize"], &ParmsSize);
		m->ReadBytes(ObjAddress + lastOff, jStruct["ReturnValueOffset"], &ReturnValueOffset);
		m->ReadBytes(ObjAddress + lastOff, jStruct["RPCId"], &RPCId);
		m->ReadBytes(ObjAddress + lastOff, jStruct["RPCResponseId"], &RPCResponseId);
		m->ReadBytes(ObjAddress + lastOff, jStruct["FirstPropertyToInit"], &FirstPropertyToInit);
		m->ReadBytes(ObjAddress + lastOff, jStruct["EventGraphFunction"], &EventGraphFunction);
		m->ReadBytes(ObjAddress + lastOff, jStruct["EventGraphCallOffset"], &EventGraphCallOffset);
		m->ReadBytes(ObjAddress + lastOff, jStruct["Func"], &Func);

		// Fix pointer for x32 tool version
		FixPointers(jStruct.StructSize);

		// Return next var offset
		return lastOff + jStruct.StructSize;
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

	int ReadData()
	{
		// Read super first
		int lastOff = UStruct::ReadData();

		// Read this struct
		static JsonStruct jStruct = JsonReflector::GetStruct(TypeName());

		// Fix pointer for x32 tool version
		FixPointers(jStruct.StructSize);

		// Return next var offset
		return lastOff + jStruct.StructSize;
	}
};

// ToDo: Check what UProperty really need!!, then don't read anything u don't need
class UProperty : public UField
{
public:
	int32_t ArrayDim = 0;
	int32_t ElementSize = 0;
	FQWord PropertyFlags;
	uint16_t RepIndex = 0;
	uint8_t BlueprintReplicationCondition = 0;
	int32_t Offset = 0;
	FName RepNotifyFunc;
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

	int ReadData()
	{
		// Read super first
		int lastOff = UField::ReadData();

		// Read this struct
		static JsonStruct jStruct = JsonReflector::GetStruct(TypeName());
		m->ReadBytes(ObjAddress + lastOff, jStruct["ArrayDim"], &ArrayDim);
		m->ReadBytes(ObjAddress + lastOff, jStruct["ElementSize"], &ElementSize);
		m->ReadBytes(ObjAddress + lastOff, jStruct["PropertyFlags"], &PropertyFlags);
		m->ReadBytes(ObjAddress + lastOff, jStruct["RepIndex"], &RepIndex);
		m->ReadBytes(ObjAddress + lastOff, jStruct["BlueprintReplicationCondition"], &BlueprintReplicationCondition);
		m->ReadBytes(ObjAddress + lastOff, jStruct["Offset"], &Offset);
		m->ReadBytes(ObjAddress + lastOff, jStruct["RepNotifyFunc"], &RepNotifyFunc);
		m->ReadBytes(ObjAddress + lastOff, jStruct["PropertyLinkNext"], &PropertyLinkNext);
		m->ReadBytes(ObjAddress + lastOff, jStruct["NextRef"], &NextRef);
		m->ReadBytes(ObjAddress + lastOff, jStruct["DestructorLinkNext"], &DestructorLinkNext);
		m->ReadBytes(ObjAddress + lastOff, jStruct["PostConstructLinkNext"], &PostConstructLinkNext);

		// Fix pointer for x32 tool version
		FixPointers(jStruct.StructSize);

		// Return next var offset
		return lastOff + jStruct.StructSize;
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

	int ReadData()
	{
		// Read super first
		int lastOff = UProperty::ReadData();

		// Read this struct
		static JsonStruct jStruct = JsonReflector::GetStruct(TypeName());

		// Fix pointer for x32 tool version
		FixPointers(jStruct.StructSize);

		// Return next var offset
		return lastOff + jStruct.StructSize;
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

	int ReadData()
	{
		// Read super first
		int lastOff = UNumericProperty::ReadData();

		// Read this struct
		static JsonStruct jStruct = JsonReflector::GetStruct(TypeName());
		m->ReadBytes(ObjAddress + lastOff, jStruct["Enum"], &Enum);

		// Fix pointer for x32 tool version
		FixPointers(jStruct.StructSize);

		// Return next var offset
		return lastOff + jStruct.StructSize;
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

	int ReadData()
	{
		// Read super first
		int lastOff = UNumericProperty::ReadData();

		// Read this struct
		static JsonStruct jStruct = JsonReflector::GetStruct(TypeName());

		// Fix pointer for x32 tool version
		FixPointers(jStruct.StructSize);

		// Return next var offset
		return lastOff + jStruct.StructSize;
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

	int ReadData()
	{
		// Read super first
		int lastOff = UNumericProperty::ReadData();

		// Read this struct
		static JsonStruct jStruct = JsonReflector::GetStruct(TypeName());

		// Fix pointer for x32 tool version
		FixPointers(jStruct.StructSize);

		// Return next var offset
		return lastOff + jStruct.StructSize;
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

	int ReadData()
	{
		// Read super first
		int lastOff = UNumericProperty::ReadData();

		// Read this struct
		static JsonStruct jStruct = JsonReflector::GetStruct(TypeName());

		// Fix pointer for x32 tool version
		FixPointers(jStruct.StructSize);

		// Return next var offset
		return lastOff + jStruct.StructSize;
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

	int ReadData()
	{
		// Read super first
		int lastOff = UNumericProperty::ReadData();

		// Read this struct
		static JsonStruct jStruct = JsonReflector::GetStruct(TypeName());

		// Fix pointer for x32 tool version
		FixPointers(jStruct.StructSize);

		// Return next var offset
		return lastOff + jStruct.StructSize;
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

	int ReadData()
	{
		// Read super first
		int lastOff = UNumericProperty::ReadData();

		// Read this struct
		static JsonStruct jStruct = JsonReflector::GetStruct(TypeName());

		// Fix pointer for x32 tool version
		FixPointers(jStruct.StructSize);

		// Return next var offset
		return lastOff + jStruct.StructSize;
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

	int ReadData()
	{
		// Read super first
		int lastOff = UNumericProperty::ReadData();

		// Read this struct
		static JsonStruct jStruct = JsonReflector::GetStruct(TypeName());

		// Fix pointer for x32 tool version
		FixPointers(jStruct.StructSize);

		// Return next var offset
		return lastOff + jStruct.StructSize;
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

	int ReadData()
	{
		// Read super first
		int lastOff = UNumericProperty::ReadData();

		// Read this struct
		static JsonStruct jStruct = JsonReflector::GetStruct(TypeName());

		// Fix pointer for x32 tool version
		FixPointers(jStruct.StructSize);

		// Return next var offset
		return lastOff + jStruct.StructSize;
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

	int ReadData()
	{
		// Read super first
		int lastOff = UNumericProperty::ReadData();

		// Read this struct
		static JsonStruct jStruct = JsonReflector::GetStruct(TypeName());

		// Fix pointer for x32 tool version
		FixPointers(jStruct.StructSize);

		// Return next var offset
		return lastOff + jStruct.StructSize;
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

	int ReadData()
	{
		// Read super first
		int lastOff = UNumericProperty::ReadData();

		// Read this struct
		static JsonStruct jStruct = JsonReflector::GetStruct(TypeName());

		// Fix pointer for x32 tool version
		FixPointers(jStruct.StructSize);

		// Return next var offset
		return lastOff + jStruct.StructSize;
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

	int ReadData()
	{
		// Read super first
		int lastOff = UProperty::ReadData();

		// Read this struct
		static JsonStruct jStruct = JsonReflector::GetStruct(TypeName());
		m->ReadBytes(ObjAddress + lastOff, jStruct["FieldSize"], &FieldSize);
		m->ReadBytes(ObjAddress + lastOff, jStruct["ByteOffset"], &ByteOffset);
		m->ReadBytes(ObjAddress + lastOff, jStruct["ByteMask"], &ByteMask);
		m->ReadBytes(ObjAddress + lastOff, jStruct["FieldMask"], &FieldMask);

		// Fix pointer for x32 tool version
		FixPointers(jStruct.StructSize);

		// Return next var offset
		return lastOff + jStruct.StructSize;
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

	int ReadData()
	{
		// Read super first
		int lastOff = UProperty::ReadData();

		// Read this struct
		static JsonStruct jStruct = JsonReflector::GetStruct(TypeName());
		m->ReadBytes(ObjAddress + lastOff, jStruct["PropertyClass"], &PropertyClass);

		// Fix pointer for x32 tool version
		FixPointers(jStruct.StructSize);

		// Return next var offset
		return lastOff + jStruct.StructSize;
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

	int ReadData()
	{
		// Read super first
		int lastOff = UObjectPropertyBase::ReadData();

		// Read this struct
		static JsonStruct jStruct = JsonReflector::GetStruct(TypeName());

		// Fix pointer for x32 tool version
		FixPointers(jStruct.StructSize);

		// Return next var offset
		return lastOff + jStruct.StructSize;
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

	int ReadData()
	{
		// Read super first
		int lastOff = UObjectProperty::ReadData();

		// Read this struct
		static JsonStruct jStruct = JsonReflector::GetStruct(TypeName());
		m->ReadBytes(ObjAddress + lastOff, jStruct["MetaClass"], &MetaClass);

		// Fix pointer for x32 tool version
		FixPointers(jStruct.StructSize);

		// Return next var offset
		return lastOff + jStruct.StructSize;
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

	int ReadData()
	{
		// Read super first
		int lastOff = UProperty::ReadData();

		// Read this struct
		static JsonStruct jStruct = JsonReflector::GetStruct(TypeName());
		m->ReadBytes(ObjAddress + lastOff, jStruct["InterfaceClass"], &InterfaceClass);

		// Fix pointer for x32 tool version
		FixPointers(jStruct.StructSize);

		// Return next var offset
		return lastOff + jStruct.StructSize;
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

	int ReadData()
	{
		// Read super first
		int lastOff = UObjectPropertyBase::ReadData();

		// Read this struct
		static JsonStruct jStruct = JsonReflector::GetStruct(TypeName());

		// Fix pointer for x32 tool version
		FixPointers(jStruct.StructSize);

		// Return next var offset
		return lastOff + jStruct.StructSize;
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

	int ReadData()
	{
		// Read super first
		int lastOff = UObjectPropertyBase::ReadData();

		// Read this struct
		static JsonStruct jStruct = JsonReflector::GetStruct(TypeName());

		// Fix pointer for x32 tool version
		FixPointers(jStruct.StructSize);

		// Return next var offset
		return lastOff + jStruct.StructSize;
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

	int ReadData()
	{
		// Read super first
		int lastOff = UObjectPropertyBase::ReadData();

		// Read this struct
		static JsonStruct jStruct = JsonReflector::GetStruct(TypeName());

		// Fix pointer for x32 tool version
		FixPointers(jStruct.StructSize);

		// Return next var offset
		return lastOff + jStruct.StructSize;
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

	int ReadData()
	{
		// Read super first
		int lastOff = UAssetObjectProperty::ReadData();

		// Read this struct
		static JsonStruct jStruct = JsonReflector::GetStruct(TypeName());
		m->ReadBytes(ObjAddress + lastOff, jStruct["MetaClass"], &MetaClass);

		// Fix pointer for x32 tool version
		FixPointers(jStruct.StructSize);

		// Return next var offset
		return lastOff + jStruct.StructSize;
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

	int ReadData()
	{
		// Read super first
		int lastOff = UProperty::ReadData();

		// Read this struct
		static JsonStruct jStruct = JsonReflector::GetStruct(TypeName());

		// Fix pointer for x32 tool version
		FixPointers(jStruct.StructSize);

		// Return next var offset
		return lastOff + jStruct.StructSize;
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

	int ReadData()
	{
		// Read super first
		int lastOff = UProperty::ReadData();

		// Read this struct
		static JsonStruct jStruct = JsonReflector::GetStruct(TypeName());
		m->ReadBytes(ObjAddress + lastOff, jStruct["Struct"], &Struct);

		// Fix pointer for x32 tool version
		FixPointers(jStruct.StructSize);

		// Return next var offset
		return lastOff + jStruct.StructSize;
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

	int ReadData()
	{
		// Read super first
		int lastOff = UProperty::ReadData();

		// Read this struct
		static JsonStruct jStruct = JsonReflector::GetStruct(TypeName());

		// Fix pointer for x32 tool version
		FixPointers(jStruct.StructSize);

		// Return next var offset
		return lastOff + jStruct.StructSize;
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

	int ReadData()
	{
		// Read super first
		int lastOff = UProperty::ReadData();

		// Read this struct
		static JsonStruct jStruct = JsonReflector::GetStruct(TypeName());

		// Fix pointer for x32 tool version
		FixPointers(jStruct.StructSize);

		// Return next var offset
		return lastOff + jStruct.StructSize;
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

	int ReadData()
	{
		// Read super first
		int lastOff = UProperty::ReadData();

		// Read this struct
		static JsonStruct jStruct = JsonReflector::GetStruct(TypeName());
		m->ReadBytes(ObjAddress + lastOff, jStruct["Inner"], &Inner);

		// Fix pointer for x32 tool version
		FixPointers(jStruct.StructSize);

		// Return next var offset
		return lastOff + jStruct.StructSize;
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

	int ReadData()
	{
		// Read super first
		int lastOff = UProperty::ReadData();

		// Read this struct
		static JsonStruct jStruct = JsonReflector::GetStruct(TypeName());
		m->ReadBytes(ObjAddress + lastOff, jStruct["KeyProp"], &KeyProp);
		m->ReadBytes(ObjAddress + lastOff, jStruct["ValueProp"], &ValueProp);

		// Fix pointer for x32 tool version
		FixPointers(jStruct.StructSize);

		// Return next var offset
		return lastOff + jStruct.StructSize;
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

	int ReadData()
	{
		// Read super first
		int lastOff = UProperty::ReadData();

		// Read this struct
		static JsonStruct jStruct = JsonReflector::GetStruct(TypeName());
		m->ReadBytes(ObjAddress + lastOff, jStruct["SignatureFunction"], &SignatureFunction);

		// Fix pointer for x32 tool version
		FixPointers(jStruct.StructSize);

		// Return next var offset
		return lastOff + jStruct.StructSize;
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

	int ReadData()
	{
		// Read super first
		int lastOff = UProperty::ReadData();

		// Read this struct
		static JsonStruct jStruct = JsonReflector::GetStruct(TypeName());
		m->ReadBytes(ObjAddress + lastOff, jStruct["SignatureFunction"], &SignatureFunction);

		// Fix pointer for x32 tool version
		FixPointers(jStruct.StructSize);

		// Return next var offset
		return lastOff + jStruct.StructSize;
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

	int ReadData()
	{
		// Read super first
		int lastOff = UProperty::ReadData();

		// Read this struct
		static JsonStruct jStruct = JsonReflector::GetStruct(TypeName());
		m->ReadBytes(ObjAddress + lastOff, jStruct["UnderlyingProp"], &UnderlyingProp);
		m->ReadBytes(ObjAddress + lastOff, jStruct["Enum"], &Enum);

		// Fix pointer for x32 tool version
		FixPointers(jStruct.StructSize);

		// Return next var offset
		return lastOff + jStruct.StructSize;
	}
};