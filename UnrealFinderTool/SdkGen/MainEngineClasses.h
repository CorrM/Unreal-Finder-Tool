#pragma once
#include <string>
#include <windows.h>
#include "../Utils.h"
#include "../JsonReflector.h"

class UClass;
class SdkUObject;

struct FPointer
{
	uintptr_t Dummy;
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
};

template<typename KeyType, typename ValueType>
class TPair
{
public:
	KeyType   Key;
	ValueType Value;
};

struct FString : public TArray<wchar_t>
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
	SdkUObject* ObjectPointer = nullptr;
	void* InterfacePointer = nullptr;

public:
	SdkUObject* GetObj() const
	{
		return ObjectPointer;
	}

	SdkUObject*& GetObjectRef()
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
	uintptr_t Data;
	int Max = 0;
	int Num = 0;
};

class SdkUObject
{
public:
	uintptr_t ObjAddress = 0;

	uintptr_t VfTable = 0;
	int Flags = 0;
	int InternalIndex = 0;
	uintptr_t Class;
	FName Name;
	uintptr_t Outer;

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

	bool IsEqual(const SdkUObject& toCheck) const
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

		return castType;
	}

};

class UField : public SdkUObject
{
public:
	uintptr_t Next;
};

class UEnum : public UField
{
public:
	FString CppType; //0x0030 
	/*TArray<TPair<FName, uint64_t>> */TUEnumArray Names; //0x0040 
	__int64 CppForm; //0x0050 
};

class UStruct : public UField
{
public:
	uintptr_t SuperField;
	uintptr_t Children;
	int32_t PropertySize;
	int32_t MinAlignment;
	char pad_0x0048[0x40];
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
	int32_t FunctionFlags;
	int8_t NumParms;
	char pad1[0x1];
	int16_t ParmsSize;
	uint16_t ReturnValueOffset;
	uint16_t RPCId;
	uint16_t RPCResponseId;
	uintptr_t FirstPropertyToInit;
	uintptr_t EventGraphFunction;
	int32_t EventGraphCallOffset;
	uintptr_t Func;
};

class UClass : public UStruct
{
public:
	char pad_0x0088[0x198]; //0x0088
};

class UProperty : public UField
{
public:
	__int32 ArrayDim; //0x0030 
	__int32 ElementSize; //0x0034 
	FQWord PropertyFlags; //0x0038
	__int32 PropertySize; //0x0040 
	char pad_0x0044[0xC]; //0x0044
	__int32 Offset; //0x0050 
	char pad_0x0054[0x24]; //0x0054
};

class UNumericProperty : public UProperty
{
public:
	
};

class UByteProperty : public UNumericProperty
{
public:
	uintptr_t		Enum;	// 0x0088 (0x04)
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
	uint8_t FieldSize;
	uint8_t ByteOffset;
	uint8_t ByteMask;
	uint8_t FieldMask;
};

class UObjectPropertyBase : public UProperty
{
public:
	uintptr_t PropertyClass;
};

class UObjectProperty : public UObjectPropertyBase
{
public:

};

class UClassProperty : public UObjectProperty
{
public:
	uintptr_t MetaClass;
};

class UInterfaceProperty : public UProperty
{
public:
	uintptr_t InterfaceClass;
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
	uintptr_t MetaClass;
};

class UNameProperty : public UProperty
{
public:

};

class UStructProperty : public UProperty
{
public:
	uintptr_t Struct;
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
	uintptr_t Inner;
};

class UMapProperty : public UProperty
{
public:
	uintptr_t KeyProp;
	uintptr_t ValueProp;
};

class UDelegateProperty : public UProperty
{
public:
	uintptr_t SignatureFunction;
};

class UMulticastDelegateProperty : public UProperty
{
public:
	uintptr_t SignatureFunction;
};

class UEnumProperty : public UProperty
{
public:
	uintptr_t UnderlyingProp; //0x0070
	uintptr_t Enum; //0x0078
}; //Size: 0x0080