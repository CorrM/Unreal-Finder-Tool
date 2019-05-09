#pragma once

#include <string>
#include <vector>
#include <array>

#include "PropertyFlags.hpp"
#include "FunctionFlags.hpp"
#include "../IGenerator.hpp"

class UObject;
class UEClass;

class UEObject
{
public:
	UEObject()
		: object(nullptr)
	{
	}
	UEObject(UObject *_object)
		: object(_object)
	{
	}

	bool IsValid() const
	{
		return object != nullptr;
	}

	size_t GetIndex() const;

	UEClass GetClass() const;

	UEObject GetOuter() const;

	std::string GetName() const;

	std::string GetFullName() const;

	std::string GetNameCPP() const;

	UEObject GetPackageObject() const;

	void* GetAddress() const;

	template<typename Base>
	Base Cast() const
	{
		return Base(object);
	}

	template<typename T>
	bool IsA() const;

	static UEClass StaticClass();

protected:
	UObject* object;
};

namespace std
{
	template<>
	struct hash<UEObject>
	{
		size_t operator()(const UEObject& obj) const
		{
			return std::hash<void*>()(obj.GetAddress());
		}
	};
}

inline bool operator==(const UEObject& lhs, const UEObject& rhs) { return rhs.GetAddress() == lhs.GetAddress(); }
inline bool operator!=(const UEObject& lhs, const UEObject& rhs) { return !(lhs == rhs); }

class UEField : public UEObject
{
public:
	using UEObject::UEObject;

	UEField GetNext() const;

	static UEClass StaticClass();
};

class UEEnum : public UEField
{
public:
	using UEField::UEField;

	std::vector<std::string> GetNames() const;

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
public:
	using UEField::UEField;

	UEStruct GetSuper() const;
	
	UEField GetChildren() const;

	size_t GetPropertySize() const;

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
public:
	using UEStruct::UEStruct;

	UEFunctionFlags GetFunctionFlags() const;

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
public:
	using UEField::UEField;

	size_t GetArrayDim() const;

	size_t GetElementSize() const;

	UEPropertyFlags GetPropertyFlags() const;

	size_t GetOffset() const;

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
public:
	using UENumericProperty::UENumericProperty;

	bool IsEnum() const;

	UEEnum GetEnum() const;

	UEProperty::Info GetInfo() const;

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
public:
	using UEProperty::UEProperty;

	bool IsNativeBool() const { return GetFieldMask() == 0xFF; }

	bool IsBitfield() const { return !IsNativeBool(); }

	uint8_t GetFieldSize() const;

	uint8_t GetByteOffset() const;

	uint8_t GetByteMask() const;

	uint8_t GetFieldMask() const;

	std::array<int, 2> GetMissingBitsCount(const UEBoolProperty& other) const;

	UEProperty::Info GetInfo() const;

	static UEClass StaticClass();
};

inline bool operator<(const UEBoolProperty& lhs, const UEBoolProperty& rhs)
{
	if (lhs.GetByteOffset() == rhs.GetByteOffset())
	{
		return lhs.GetByteMask() < rhs.GetByteMask();
	}
	return lhs.GetByteOffset() < rhs.GetByteOffset();
}

class UEObjectPropertyBase : public UEProperty
{
public:
	using UEProperty::UEProperty;

	UEClass GetPropertyClass() const;

	static UEClass StaticClass();
};

class UEObjectProperty : public UEObjectPropertyBase
{
public:
	using UEObjectPropertyBase::UEObjectPropertyBase;

	UEProperty::Info GetInfo() const;

	static UEClass StaticClass();
};

class UEClassProperty : public UEObjectProperty
{
public:
	using UEObjectProperty::UEObjectProperty;

	UEClass GetMetaClass() const;

	UEProperty::Info GetInfo() const;

	static UEClass StaticClass();
};

class UEInterfaceProperty : public UEProperty
{
public:
	using UEProperty::UEProperty;

	UEClass GetInterfaceClass() const;

	UEProperty::Info GetInfo() const;

	static UEClass StaticClass();
};

class UEWeakObjectProperty : public UEObjectPropertyBase
{
public:
	using UEObjectPropertyBase::UEObjectPropertyBase;

	UEProperty::Info GetInfo() const;

	static UEClass StaticClass();
};

class UELazyObjectProperty : public UEObjectPropertyBase
{
public:
	using UEObjectPropertyBase::UEObjectPropertyBase;

	UEProperty::Info GetInfo() const;

	static UEClass StaticClass();
};

class UEAssetObjectProperty : public UEObjectPropertyBase
{
public:
	using UEObjectPropertyBase::UEObjectPropertyBase;

	UEProperty::Info GetInfo() const;

	static UEClass StaticClass();
};

class UEAssetClassProperty : public UEAssetObjectProperty
{
public:
	using UEAssetObjectProperty::UEAssetObjectProperty;

	UEClass GetMetaClass() const;

	UEProperty::Info GetInfo() const;

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
public:
	using UEProperty::UEProperty;

	UEScriptStruct GetStruct() const;

	UEProperty::Info GetInfo() const;

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
public:
	using UEProperty::UEProperty;

	UEProperty GetInner() const;

	UEProperty::Info GetInfo() const;

	static UEClass StaticClass();
};

class UEMapProperty : public UEProperty
{
public:
	using UEProperty::UEProperty;

	UEProperty GetKeyProperty() const;
	UEProperty GetValueProperty() const;

	UEProperty::Info GetInfo() const;

	static UEClass StaticClass();
};

class UEDelegateProperty : public UEProperty
{
public:
	using UEProperty::UEProperty;

	UEFunction GetSignatureFunction() const;

	UEProperty::Info GetInfo() const;

	static UEClass StaticClass();
};

class UEMulticastDelegateProperty : public UEProperty
{
public:
	using UEProperty::UEProperty;

	UEFunction GetSignatureFunction() const;

	UEProperty::Info GetInfo() const;

	static UEClass StaticClass();
};

class UEEnumProperty : public UEProperty
{
public:
	using UEProperty::UEProperty;

	UENumericProperty GetUnderlyingProperty() const;
	UEEnum GetEnum() const;

	UEProperty::Info GetInfo() const;

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

	for (auto super = GetClass(); super.IsValid(); super = super.GetSuper().Cast<UEClass>())
	{
		if (super.object == cmp.object)
		{
			return true;
		}
	}

	return false;
}