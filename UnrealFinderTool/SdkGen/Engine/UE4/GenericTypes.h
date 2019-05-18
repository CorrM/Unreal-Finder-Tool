#pragma once
#include <string>
#include <vector>
#include <array>

#include "PropertyFlags.h"
#include "FunctionFlags.h"
#include "../IGenerator.h"
#include "../MainEngineClasses.h"
#include "Utils.h"

class UEClass;

class UEObject
{
protected:
	UClass objClass;
	SdkUObject objObject;

public:
	SdkUObject Object;

	UEObject() = default;

	UEObject(const SdkUObject object) : Object(object)
	{

	}

	bool IsValid() const;

	size_t GetIndex() const;

	UEClass GetClass();

	UEObject GetOuter();

	std::string GetName() const;

	std::string GetFullName() const;

	std::string GetNameCPP() const;

	UEObject GetPackageObject() const;

	void* GetAddress() const;

	bool Update(const UEObject& newObject);

	bool Update(const SdkUObject& newObject);

	template<typename Base>
	Base Cast() const
	{
		// TODO: Solve Problem HERE
		// may case a delete problem it's use the same `object`
		// may delete from other UEObject

		return Base(Object);
	}

	template<typename T>
	bool IsA() const;

	static UEClass StaticClass();

};

namespace std
{
	template<>
	struct hash<UEObject>
	{
		size_t operator()(const UEObject& obj) const noexcept
		{
			return std::hash<void*>()(obj.GetAddress());
		}
	};
}

inline bool operator==(const UEObject& lhs, const UEObject& rhs) { return rhs.GetAddress() == lhs.GetAddress(); }
inline bool operator!=(const UEObject& lhs, const UEObject& rhs) { return !(lhs == rhs); }

class UEField : public UEObject
{
	UField objField;
	UField next;

public:
	using UEObject::UEObject;

	UEField GetNext();

	static UEClass StaticClass();
};

class UEEnum : public UEField
{
	UEnum objEnum;

public:
	using UEField::UEField;

	std::vector<std::string> GetNames();

	static UEClass StaticClass();
};

class UEConst : public UEField
{
public:
	using UEField::UEField;

	std::string GetValue() const;

	static UEClass StaticClass();
};

class UEStruct : public UEField
{
	UStruct objStruct;
	UStruct superField;
	UField children;

public:
	using UEField::UEField;

	UEStruct GetSuper();

	UEField GetChildren();

	size_t GetPropertySize();

	static UEClass StaticClass();
};

class UEScriptStruct : public UEStruct
{
public:
	using UEStruct::UEStruct;

	static UEClass StaticClass();
};

class UEFunction : public UEStruct
{
	UFunction objFunction;

public:
	using UEStruct::UEStruct;

	UEFunctionFlags GetFunctionFlags();

	static UEClass StaticClass();
};

class UEClass : public UEStruct
{
public:
	using UEStruct::UEStruct;

	static UEClass StaticClass();
};

class UEProperty : public UEField
{
	UProperty objProperty;

public:
	using UEField::UEField;

	size_t GetArrayDim();

	size_t GetElementSize();

	UEPropertyFlags GetPropertyFlags();

	size_t GetOffset();

	enum class PropertyType
	{
		Unknown,
		Primitive,
		PredefinedStruct,
		CustomStruct,
		Container
	};

	struct Info
	{
		PropertyType Type;
		size_t Size;
		bool CanBeReference;
		std::string CppType;

		static Info Create(PropertyType type, size_t size, bool reference, std::string&& cppType)
		{
			extern IGenerator* generator;

			return { type, size, reference, generator->GetOverrideType(cppType) };
		}
	};

	Info GetInfo() const;

	static UEClass StaticClass();
};

class UENumericProperty : public UEProperty
{
public:
	using UEProperty::UEProperty;

	static UEClass StaticClass();
};

class UEByteProperty : public UENumericProperty
{
	UByteProperty objByteProperty;
	UEnum enumProperty;

public:
	using UENumericProperty::UENumericProperty;

	bool IsEnum() const;

	UEEnum GetEnum();

	UEProperty::Info GetInfo();

	static UEClass StaticClass();
};

class UEUInt16Property : public UENumericProperty
{
public:
	using UENumericProperty::UENumericProperty;

	UEProperty::Info GetInfo() const;

	static UEClass StaticClass();
};

class UEUInt32Property : public UENumericProperty
{
public:
	using UENumericProperty::UENumericProperty;

	UEProperty::Info GetInfo() const;

	static UEClass StaticClass();
};

class UEUInt64Property : public UENumericProperty
{
public:
	using UENumericProperty::UENumericProperty;

	UEProperty::Info GetInfo() const;

	static UEClass StaticClass();
};

class UEInt8Property : public UENumericProperty
{
public:
	using UENumericProperty::UENumericProperty;

	UEProperty::Info GetInfo() const;

	static UEClass StaticClass();
};

class UEInt16Property : public UENumericProperty
{
public:
	using UENumericProperty::UENumericProperty;

	UEProperty::Info GetInfo() const;

	static UEClass StaticClass();
};

class UEIntProperty : public UENumericProperty
{
public:
	using UENumericProperty::UENumericProperty;

	UEProperty::Info GetInfo() const;

	static UEClass StaticClass();
};

class UEInt64Property : public UENumericProperty
{
public:
	using UENumericProperty::UENumericProperty;

	UEProperty::Info GetInfo() const;

	static UEClass StaticClass();
};

class UEFloatProperty : public UENumericProperty
{
public:
	using UENumericProperty::UENumericProperty;

	UEProperty::Info GetInfo() const;

	static UEClass StaticClass();
};

class UEDoubleProperty : public UENumericProperty
{
public:
	using UENumericProperty::UENumericProperty;

	UEProperty::Info GetInfo() const;

	static UEClass StaticClass();
};

class UEBoolProperty : public UEProperty
{
	UBoolProperty objBoolProperty;

public:
	using UEProperty::UEProperty;

	bool IsNativeBool() { return GetFieldMask() == 0xFF; }

	bool IsBitfield() { return !IsNativeBool(); }

	uint8_t GetFieldSize();

	uint8_t GetByteOffset();

	uint8_t GetByteMask();

	uint8_t GetFieldMask();

	std::array<int, 2> GetMissingBitsCount(const UEBoolProperty& other) const;

	UEProperty::Info GetInfo();

	static UEClass StaticClass();
};

inline bool operator<(UEBoolProperty & lhs, UEBoolProperty & rhs)
{
	if (lhs.GetByteOffset() == rhs.GetByteOffset())
	{
		return lhs.GetByteMask() < rhs.GetByteMask();
	}
	return lhs.GetByteOffset() < rhs.GetByteOffset();
}

class UEObjectPropertyBase : public UEProperty
{
	UObjectPropertyBase objObjectPropertyBase;
	UObjectPropertyBase propertyClass;

public:
	using UEProperty::UEProperty;

	UEClass GetPropertyClass();

	static UEClass StaticClass();
};

class UEObjectProperty : public UEObjectPropertyBase
{
public:
	using UEObjectPropertyBase::UEObjectPropertyBase;

	UEProperty::Info GetInfo();

	static UEClass StaticClass();
};

class UEClassProperty : public UEObjectProperty
{
	UClassProperty objClassProperty;
	UClassProperty metaClass;

public:
	using UEObjectProperty::UEObjectProperty;

	UEClass GetMetaClass();

	UEProperty::Info GetInfo();

	static UEClass StaticClass();
};

class UEInterfaceProperty : public UEProperty
{
	UInterfaceProperty objInterfaceProperty;
	UInterfaceProperty interfaceClass;

public:
	using UEProperty::UEProperty;

	UEClass GetInterfaceClass();

	UEProperty::Info GetInfo();

	static UEClass StaticClass();
};

class UEWeakObjectProperty : public UEObjectPropertyBase
{
public:
	using UEObjectPropertyBase::UEObjectPropertyBase;

	UEProperty::Info GetInfo();

	static UEClass StaticClass();
};

class UELazyObjectProperty : public UEObjectPropertyBase
{
public:
	using UEObjectPropertyBase::UEObjectPropertyBase;

	UEProperty::Info GetInfo();

	static UEClass StaticClass();
};

class UEAssetObjectProperty : public UEObjectPropertyBase
{
public:
	using UEObjectPropertyBase::UEObjectPropertyBase;

	UEProperty::Info GetInfo();

	static UEClass StaticClass();
};

class UEAssetClassProperty : public UEAssetObjectProperty
{
	UAssetClassProperty objAssetClassProperty;
	UClass metaClass;

public:
	using UEAssetObjectProperty::UEAssetObjectProperty;

	UEClass GetMetaClass();

	UEProperty::Info GetInfo();

	static UEClass StaticClass();
};

class UENameProperty : public UEProperty
{
public:
	using UEProperty::UEProperty;

	UEProperty::Info GetInfo() const;

	static UEClass StaticClass();
};

class UEStructProperty : public UEProperty
{
	UStructProperty objStructProperty;
	UStructProperty objStruct;

public:
	using UEProperty::UEProperty;

	UEScriptStruct GetStruct();

	UEProperty::Info GetInfo();

	static UEClass StaticClass();
};

class UEStrProperty : public UEProperty
{
public:
	using UEProperty::UEProperty;

	UEProperty::Info GetInfo() const;

	static UEClass StaticClass();
};

class UETextProperty : public UEProperty
{
public:
	using UEProperty::UEProperty;

	UEProperty::Info GetInfo() const;

	static UEClass StaticClass();
};

class UEArrayProperty : public UEProperty
{
	UArrayProperty objArrayProperty;
	UArrayProperty inner;

public:
	using UEProperty::UEProperty;

	UEProperty GetInner();

	UEProperty::Info GetInfo();

	static UEClass StaticClass();
};

class UEMapProperty : public UEProperty
{
	UMapProperty objMapProperty;
	UMapProperty keyProp, valueProp;

public:
	using UEProperty::UEProperty;

	UEProperty GetKeyProperty();
	UEProperty GetValueProperty();

	UEProperty::Info GetInfo();

	static UEClass StaticClass();
};

class UEDelegateProperty : public UEProperty
{
	UDelegateProperty objDelegateProperty;
	UDelegateProperty signatureFunction;

public:
	using UEProperty::UEProperty;

	UEFunction GetSignatureFunction();

	UEProperty::Info GetInfo();

	static UEClass StaticClass();
};

class UEMulticastDelegateProperty : public UEProperty
{
	UDelegateProperty objDelegateProperty;
	UDelegateProperty signatureFunction;

public:
	using UEProperty::UEProperty;

	UEFunction GetSignatureFunction();

	UEProperty::Info GetInfo();

	static UEClass StaticClass();
};

class UEEnumProperty : public UEProperty
{
	UEnumProperty objEnumProperty;
	UEnumProperty underlyingProp;
	UEnumProperty Enum;

public:
	using UEProperty::UEProperty;

	UENumericProperty GetUnderlyingProperty();
	UEEnum GetEnum();

	UEProperty::Info GetInfo();

	static UEClass StaticClass();
};

template<typename T>
bool UEObject::IsA() const
{
	auto cmp = T::StaticClass();
	if (!cmp.IsValid())
	{
		return false;
	}

	UEClass super = GetClass();
	while (super.IsValid())
	{
		if (super.Object.IsEqual(cmp.Object))
		{
			return true;
		}

		if (!super.Update(super.GetSuper().Cast<UEClass>())) break;
	}

	return false;
}