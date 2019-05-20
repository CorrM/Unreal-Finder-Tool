#pragma once
#include <string>
#include <windows.h>
#include "../Utils.h"
#include "../JsonReflector.h"

// #define OFFSET(s, m) ((size_t)&reinterpret_cast<char const volatile&>((((s*)0)->m)))
#define OFFSET(m) ((int)&reinterpret_cast<char const volatile&>((((decltype(this))0)->m)))

class UClass;
class UObject;

struct FPointer
{
	uintptr_t Dummy;

	void FixPointers()
	{
		Utils::FixPointers(this, { OFFSET(Dummy) });
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

template<class T>
struct TArray
{
	friend struct FString;

public:
	TArray()
	{
		Data = nullptr;
		Count = Max = 0;
	};

	size_t Num() const
	{
		return Count;
	};

	T& operator[](size_t i)
	{
		return Data[i];
	};

	const T& operator[](size_t i) const
	{
		return Data[i];
	};

	bool IsValidIndex(size_t i) const
	{
		return i < Num();
	}

private:
	T* Data;
	int32_t Count;
	int32_t Max;

	void FixPointers()
	{
		// Utils::FixPointers(this, {  });
		// UStruct::FixPointers();
	}
};

template<typename KeyType, typename ValueType>
class TPair
{
public:
	KeyType   Key;
	ValueType Value;
};

struct FString : TArray<wchar_t>
{
	std::string ToString() const
	{
		int size = WideCharToMultiByte(CP_UTF8, 0, Data, Count, nullptr, 0, nullptr, nullptr);
		std::string str(size, 0);
		WideCharToMultiByte(CP_UTF8, 0, Data, Count, &str[0], size, nullptr, nullptr);
		return str;
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
};

class FAssetPtr : public TPersistentObjectPtr<FStringAssetReference>
{

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

class TUEnumArray
{
public:
	uintptr_t Data = 0;
	int Max = 0;
	int Num = 0;

	void FixPointers()
	{
		Utils::FixPointers(this, { OFFSET(Data) });
	}
};

class UObject
{
public:
	uintptr_t ObjAddress = 0;

	uintptr_t VfTable = 0;
	int Flags = 0;
	int InternalIndex = 0;
	uintptr_t Class = 0;
	FName Name;
	uintptr_t Outer = 0;

	void FixPointers()
	{
		Utils::FixPointers(this, {
			OFFSET(VfTable),
			OFFSET(Class),
			OFFSET(Outer),
		});
	}

	void operator=(JsonStruct& uObject)
	{
		VfTable = uObject["VfTable"].ReadAs<uintptr_t>();
		Flags = uObject["Flags"].ReadAs<int>();
		InternalIndex = uObject["InternalIndex"].ReadAs<int>();
		Class = uObject["Class"].ReadAs<uintptr_t>();
		Outer = uObject["Outer"].ReadAs<uintptr_t>();

		// Read FName
		JsonStruct tmpFName = *uObject["Name"].ReadAsStruct();
		Name = FName(tmpFName["Index"].ReadAs<int>(), tmpFName["Number"].ReadAs<int>());
	}

	bool IsEqual(const UObject& toCheck) const
	{
		return toCheck.InternalIndex == this->InternalIndex && toCheck.Name.ComparisonIndex == this->Name.ComparisonIndex;
	}

	bool Empty() const
	{
		return ObjAddress == NULL && VfTable == NULL;
	}

	template <typename SdkStruct>
	SdkStruct Cast() const
	{
		// it's like internal cast, but for remote process
		SdkStruct castType;

		castType.ObjAddress = ObjAddress;
		Utils::MemoryObj->Read<SdkStruct>(ObjAddress, castType, sizeof(uintptr_t)); // Skip ObjAddress in SdkUObject
		castType.FixPointers();

		return castType;
	}
};

class UField : public UObject
{
public:
	uintptr_t Next = 0;

	void FixPointers()
	{
		UObject::FixPointers();
		Utils::FixPointers(this, { OFFSET(Next) });
	}
};

class UEnum : public UField
{
public:
	FString CppType;
	TUEnumArray Names; /*TArray<TPair<FName, uint64_t>> */
	__int64 CppForm = 0;
};

class UStruct : public UField
{
public:
	uintptr_t SuperField = 0;
	uintptr_t Children = 0;
	int32_t PropertySize = 0;
	int32_t MinAlignment = 0;
	char pad_0x0048[0x40];

	void FixPointers()
	{
		UField::FixPointers();
		Utils::FixPointers(this, {
			OFFSET(SuperField),
			OFFSET(Children)
		});
	}
};

class UScriptStruct : public UStruct
{
public:
	char pad_0x0088[0x10];
	//char pad_0x0089[0x1];
};

class UFunction : public UStruct
{
public:
	int32_t FunctionFlags = 0;
	int8_t NumParms = 0;
	char pad1[0x1];
	int16_t ParmsSize = 0;
	uint16_t ReturnValueOffset = 0;
	uint16_t RPCId = 0;
	uint16_t RPCResponseId = 0;
	uintptr_t FirstPropertyToInit = 0;
	uintptr_t EventGraphFunction = 0;
	int32_t EventGraphCallOffset = 0;
	uintptr_t Func = 0;

	void FixPointers()
	{
		UStruct::FixPointers();
		Utils::FixPointers(this, {
			OFFSET(FirstPropertyToInit),
			OFFSET(EventGraphFunction),
			OFFSET(Func)
		});
	}
};

class UClass : public UStruct
{
public:
	char pad_0x0088[0x198]; //0x0088

	void FixPointers()
	{
		UStruct::FixPointers();
	}
};

class UProperty : public UField
{
public:
	int ArrayDim = 0;
	int ElementSize = 0;
	FQWord PropertyFlags;
	int PropertySize = 0;
	char pad_0x0044[0xC];
	int Offset = 0;
	char pad_0x0054[0x24];

	void FixPointers()
	{
		UField::FixPointers();
	}
};

class UNumericProperty : public UProperty
{
public:
	void FixPointers()
	{
		UProperty::FixPointers();
	}
};

class UByteProperty : public UNumericProperty
{
public:
	uintptr_t Enum = 0;

	void FixPointers()
	{
		UNumericProperty::FixPointers();
		Utils::FixPointers(this, { OFFSET(Enum) });
	}
};

class UUInt16Property : public UNumericProperty
{
public:

};

class UUInt32Property : public UNumericProperty
{
public:

};

class UUInt64Property : public UNumericProperty
{
public:

};

class UInt8Property : public UNumericProperty
{
public:

};

class UInt16Property : public UNumericProperty
{
public:

};

class UIntProperty : public UNumericProperty
{
public:

};

class UInt64Property : public UNumericProperty
{
public:

};

class UFloatProperty : public UNumericProperty
{
public:

};

class UDoubleProperty : public UNumericProperty
{
public:

};

class UBoolProperty : public UProperty
{
public:
	uint8_t FieldSize = 0;
	uint8_t ByteOffset = 0;
	uint8_t ByteMask = 0;
	uint8_t FieldMask = 0;
};

class UObjectPropertyBase : public UProperty
{
public:
	uintptr_t PropertyClass = 0;

	void FixPointers()
	{
		UProperty::FixPointers();
		Utils::FixPointers(this, { OFFSET(PropertyClass) });
	}
};

class UObjectProperty : public UObjectPropertyBase
{
public:

};

class UClassProperty : public UObjectProperty
{
public:
	uintptr_t MetaClass = 0;

	void FixPointers()
	{
		UObjectProperty::FixPointers();
		Utils::FixPointers(this, { OFFSET(MetaClass) });
	}
};

class UInterfaceProperty : public UProperty
{
public:
	uintptr_t InterfaceClass = 0;

	void FixPointers()
	{
		UProperty::FixPointers();
		Utils::FixPointers(this, { OFFSET(InterfaceClass) });
	}
};

class UWeakObjectProperty : public UObjectPropertyBase
{
public:

};

class ULazyObjectProperty : public UObjectPropertyBase
{
public:

};

class UAssetObjectProperty : public UObjectPropertyBase
{
public:

};

class UAssetClassProperty : public UAssetObjectProperty
{
public:
	uintptr_t MetaClass = 0;

	void FixPointers()
	{
		UAssetObjectProperty::FixPointers();
		Utils::FixPointers(this, { OFFSET(MetaClass) });
	}
};

class UNameProperty : public UProperty
{
public:

};

class UStructProperty : public UProperty
{
public:
	uintptr_t Struct = 0;

	void FixPointers()
	{
		UProperty::FixPointers();
		Utils::FixPointers(this, { OFFSET(Struct) });
	}
};

class UStrProperty : public UProperty
{
public:

};


class UTextProperty : public UProperty
{
public:

};

class UArrayProperty : public UProperty
{
public:
	uintptr_t Inner = 0;

	void FixPointers()
	{
		UProperty::FixPointers();
		Utils::FixPointers(this, { OFFSET(Inner) });
	}
};

class UMapProperty : public UProperty
{
public:
	uintptr_t KeyProp = 0;
	uintptr_t ValueProp = 0;

	void FixPointers()
	{
		UProperty::FixPointers();
		Utils::FixPointers(this, {
			OFFSET(KeyProp),
			OFFSET(ValueProp)
		});
	}
};

class UDelegateProperty : public UProperty
{
public:
	uintptr_t SignatureFunction = 0;

	void FixPointers()
	{
		UProperty::FixPointers();
		Utils::FixPointers(this, { OFFSET(SignatureFunction) });
	}
};

class UMulticastDelegateProperty : public UProperty
{
public:
	uintptr_t SignatureFunction = 0;

	void FixPointers()
	{
		UProperty::FixPointers();
		Utils::FixPointers(this, { OFFSET(SignatureFunction) });
	}
};

class UEnumProperty : public UProperty
{
public:
	uintptr_t UnderlyingProp = 0;
	uintptr_t Enum = 0;

	void FixPointers()
	{
		UProperty::FixPointers();
		Utils::FixPointers(this, {
			OFFSET(UnderlyingProp),
			OFFSET(Enum)
		});
	}
};