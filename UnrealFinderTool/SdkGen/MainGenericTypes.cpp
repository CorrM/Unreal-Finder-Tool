#include "pch.h"
#include "../IGenerator.h"
#include "GenericTypes.h"
#include "ObjectsStore.h"
#include "NamesStore.h"
#include "NameValidator.h"
#include "MainEngineClasses.h"
#include "Utils.h"

//---------------------------------------------------------------------------
//UEObject
//---------------------------------------------------------------------------
bool UEObject::IsValid() const
{
	return Object.ObjAddress != NULL && Object.VfTable != NULL;
}
size_t UEObject::GetIndex() const
{
	return Object.InternalIndex;
}
UEClass UEObject::GetClass()
{
	if (objClass.Empty())
	{
		if (!Utils::IsValidAddress(Utils::MemoryObj, Object.Class))
			return UEClass();

		objClass.ObjAddress = Object.Class;
		Utils::MemoryObj->Read<UClass>(objClass.ObjAddress, objClass, sizeof(uintptr_t)); // Skip ObjAddress in SdkUObject
	}
	
	return UEClass(objClass);
}
UEObject UEObject::GetOuter()
{
	if (objObject.Empty())
	{
		// Init
		JsonStruct uObject;

		// Read as UObject
		uObject.ReadData(Object.Outer, "UObject");

		// Fill Data into obj
		objObject.ObjAddress = Object.Outer;
		objObject = uObject;
	}

	return UEObject(objObject);
}
std::string UEObject::GetName() const
{
	auto name = NamesStore().GetById(Object.Name.ComparisonIndex);
	if (Object.Name.Number > 0)
	{
		name += '_' + std::to_string(Object.Name.Number);
	}

	auto pos = name.rfind('/');
	if (pos == std::string::npos)
	{
		return name;
	}

	return name.substr(pos + 1);
}
UEClass UEObject::StaticClass()
{
	static auto c = ObjectsStore().FindClass("Class CoreUObject.Object");
	return c;
}

//---------------------------------------------------------------------------
//UEField
//---------------------------------------------------------------------------
UEField UEField::GetNext()
{
	if (objField.Empty())
		objField = Object.Cast<UField>();

	if (next.Empty())
	{
		if (!Utils::IsValidAddress(Utils::MemoryObj, objField.Next))
			return UEField();

		next.ObjAddress = objField.Next;
		Utils::MemoryObj->Read<UField>(next.ObjAddress, next, sizeof(uintptr_t)); // Skip ObjAddress in SdkUObject
	}
	
	return UEField(next);
}
UEClass UEField::StaticClass()
{
	static auto c = ObjectsStore().FindClass("Class CoreUObject.Field");
	return c;
}

//---------------------------------------------------------------------------
//UEEnum
//---------------------------------------------------------------------------
std::vector<std::string> UEEnum::GetNames()
{
	std::vector<std::string> buffer;
	if (objEnum.Empty())
		objEnum = Object.Cast<UEnum>();

	// Get Names
	uintptr_t dataAddress = objEnum.Names.Data;
	auto cls = new FUEnumItem[objEnum.Names.Num];
	Utils::MemoryObj->ReadBytes(dataAddress, cls, sizeof(FUEnumItem) * objEnum.Names.Num);

	buffer.reserve(objEnum.Names.Num);
	for (auto i = 0; i < objEnum.Names.Num; ++i)
	{
		size_t index = cls[i].Key.ComparisonIndex;
		if (index > NamesStore().GetNamesNum() || index == 0)
			continue;

		buffer.push_back(NamesStore().GetById(index));
	}

	delete[] cls;
	return buffer;
}
UEClass UEEnum::StaticClass()
{
	static auto c = ObjectsStore().FindClass("Class CoreUObject.Enum");
	return c;
}

//---------------------------------------------------------------------------
//UEConst
//---------------------------------------------------------------------------
std::string UEConst::GetValue() const
{
	throw;
}
UEClass UEConst::StaticClass()
{
	//not supported by UE4
	return UEClass();
}

//---------------------------------------------------------------------------
//UEStruct
//---------------------------------------------------------------------------
UEStruct UEStruct::GetSuper()
{
	if (objStruct.Empty())
		objStruct = Object.Cast<UStruct>();

	if (superField.Empty())
	{
		if (!Utils::IsValidAddress(Utils::MemoryObj, objStruct.SuperField))
			return UEStruct();

		superField.ObjAddress = objStruct.SuperField;
		Utils::MemoryObj->Read<UStruct>(superField.ObjAddress, superField, sizeof(uintptr_t)); // Skip ObjAddress in SdkUObject
	}

	return UEStruct(superField);
}
UEField UEStruct::GetChildren()
{
	if (objStruct.Empty())
		objStruct = Object.Cast<UStruct>();

	if (children.Empty())
	{
		if (!Utils::IsValidAddress(Utils::MemoryObj, objStruct.Children))
			return UEField();

		children.ObjAddress = objStruct.Children;
		Utils::MemoryObj->Read<UField>(children.ObjAddress, children, sizeof(uintptr_t)); // Skip ObjAddress in SdkUObject
	}

	return UEField(children);
}
size_t UEStruct::GetPropertySize()
{
	if (objStruct.Empty())
		objStruct = Object.Cast<UStruct>();

	return objStruct.PropertySize;
}
UEClass UEStruct::StaticClass()
{
	static auto c = ObjectsStore().FindClass("Class CoreUObject.Struct");
	return c;
}

//---------------------------------------------------------------------------
//UEScriptStruct
//---------------------------------------------------------------------------
UEClass UEScriptStruct::StaticClass()
{
	static auto c = ObjectsStore().FindClass("Class CoreUObject.ScriptStruct");
	return c;
}

//---------------------------------------------------------------------------
//UEFunction
//---------------------------------------------------------------------------
UEFunctionFlags UEFunction::GetFunctionFlags()
{
	if (objFunction.Empty())
		objFunction = Object.Cast<UFunction>();

	return static_cast<UEFunctionFlags>(objFunction.FunctionFlags);
}
UEClass UEFunction::StaticClass()
{
	static auto c = ObjectsStore().FindClass("Class CoreUObject.Function");
	return c;
}

//---------------------------------------------------------------------------
//UEClass
//---------------------------------------------------------------------------
UEClass UEClass::StaticClass()
{
	static auto c = ObjectsStore().FindClass("Class CoreUObject.Class");
	return c;
}

//---------------------------------------------------------------------------
//UEProperty
//---------------------------------------------------------------------------
size_t UEProperty::GetArrayDim()
{
	if (objProperty.Empty())
		objProperty = Object.Cast<UProperty>();
	
	return objProperty.ArrayDim;
}
size_t UEProperty::GetElementSize()
{
	if (objProperty.Empty())
		objProperty = Object.Cast<UProperty>();

	return objProperty.ElementSize;
}
UEPropertyFlags UEProperty::GetPropertyFlags()
{
	if (objProperty.Empty())
		objProperty = Object.Cast<UProperty>();

	return static_cast<UEPropertyFlags>(objProperty.PropertyFlags.A);
}
size_t UEProperty::GetOffset()
{
	if (objProperty.Empty())
		objProperty = Object.Cast<UProperty>();

	return objProperty.Offset;
}
UEClass UEProperty::StaticClass()
{
	static auto c = ObjectsStore().FindClass("Class CoreUObject.Property");
	return c;
}

//---------------------------------------------------------------------------
//UENumericProperty
//---------------------------------------------------------------------------
UEClass UENumericProperty::StaticClass()
{
	static auto c = ObjectsStore().FindClass("Class CoreUObject.NumericProperty");
	return c;
}

//---------------------------------------------------------------------------
//UEByteProperty
//---------------------------------------------------------------------------
UEEnum UEByteProperty::GetEnum()
{
	if (objByteProperty.Empty())
		objByteProperty = Object.Cast<UByteProperty>();

	if (enumProperty.Empty())
	{
		if (!Utils::IsValidAddress(Utils::MemoryObj, objByteProperty.Enum))
			return UEEnum();

		enumProperty.ObjAddress = objByteProperty.Enum;
		Utils::MemoryObj->Read<UEnum>(enumProperty.ObjAddress, enumProperty, sizeof(uintptr_t)); // Skip ObjAddress in SdkUObject
	}
	
	return UEEnum(enumProperty);
}
UEProperty::Info UEByteProperty::GetInfo()
{
	if (IsEnum())
	{
		return Info::Create(PropertyType::Primitive, sizeof(uint8_t), false, "TEnumAsByte<" + MakeUniqueCppName(GetEnum()) + ">");
	}
	return Info::Create(PropertyType::Primitive, sizeof(uint8_t), false, "unsigned char");
}
UEClass UEByteProperty::StaticClass()
{
	static auto c = ObjectsStore().FindClass("Class CoreUObject.ByteProperty");
	return c;
}

//---------------------------------------------------------------------------
//UEUInt16Property
//---------------------------------------------------------------------------
UEProperty::Info UEUInt16Property::GetInfo() const
{
	return Info::Create(PropertyType::Primitive, sizeof(uint16_t), false, "uint16_t");
}
UEClass UEUInt16Property::StaticClass()
{
	static auto c = ObjectsStore().FindClass("Class CoreUObject.UInt16Property");
	return c;
}

//---------------------------------------------------------------------------
//UEUInt32Property
//---------------------------------------------------------------------------
UEProperty::Info UEUInt32Property::GetInfo() const
{
	return Info::Create(PropertyType::Primitive, sizeof(uint32_t), false, "uint32_t");
}
UEClass UEUInt32Property::StaticClass()
{
	static auto c = ObjectsStore().FindClass("Class CoreUObject.UInt32Property");
	return c;
}

//---------------------------------------------------------------------------
//UEUInt64Property
//---------------------------------------------------------------------------
UEProperty::Info UEUInt64Property::GetInfo() const
{
	return Info::Create(PropertyType::Primitive, sizeof(uint64_t), false, "uint64_t");
}
UEClass UEUInt64Property::StaticClass()
{
	static auto c = ObjectsStore().FindClass("Class CoreUObject.UInt64Property");
	return c;
}

//---------------------------------------------------------------------------
//UEInt8Property
//---------------------------------------------------------------------------
UEProperty::Info UEInt8Property::GetInfo() const
{
	return Info::Create(PropertyType::Primitive, sizeof(int8_t), false, "int8_t");
}
UEClass UEInt8Property::StaticClass()
{
	static auto c = ObjectsStore().FindClass("Class CoreUObject.Int8Property");
	return c;
}

//---------------------------------------------------------------------------
//UEInt16Property
//---------------------------------------------------------------------------
UEProperty::Info UEInt16Property::GetInfo() const
{
	return Info::Create(PropertyType::Primitive, sizeof(int16_t), false, "int16_t");
}
UEClass UEInt16Property::StaticClass()
{
	static auto c = ObjectsStore().FindClass("Class CoreUObject.Int16Property");
	return c;
}

//---------------------------------------------------------------------------
//UEIntProperty
//---------------------------------------------------------------------------
UEProperty::Info UEIntProperty::GetInfo() const
{
	return Info::Create(PropertyType::Primitive, sizeof(int), false, "int");
}
UEClass UEIntProperty::StaticClass()
{
	static auto c = ObjectsStore().FindClass("Class CoreUObject.IntProperty");
	return c;
}

//---------------------------------------------------------------------------
//UEInt64Property
//---------------------------------------------------------------------------
UEProperty::Info UEInt64Property::GetInfo() const
{
	return Info::Create(PropertyType::Primitive, sizeof(int64_t), false, "int64_t");
}
UEClass UEInt64Property::StaticClass()
{
	static auto c = ObjectsStore().FindClass("Class CoreUObject.Int64Property");
	return c;
}

//---------------------------------------------------------------------------
//UEFloatProperty
//---------------------------------------------------------------------------
UEProperty::Info UEFloatProperty::GetInfo() const
{
	return Info::Create(PropertyType::Primitive, sizeof(float), false, "float");
}
UEClass UEFloatProperty::StaticClass()
{
	static auto c = ObjectsStore().FindClass("Class CoreUObject.FloatProperty");
	return c;
}

//---------------------------------------------------------------------------
//UEDoubleProperty
//---------------------------------------------------------------------------
UEProperty::Info UEDoubleProperty::GetInfo() const
{
	return Info::Create(PropertyType::Primitive, sizeof(double), false, "double");
}
UEClass UEDoubleProperty::StaticClass()
{
	static auto c = ObjectsStore().FindClass("Class CoreUObject.DoubleProperty");
	return c;
}

//---------------------------------------------------------------------------
//UEBoolProperty
//---------------------------------------------------------------------------
uint8_t UEBoolProperty::GetFieldSize()
{
	if (objBoolProperty.Empty())
		objBoolProperty = Object.Cast<UBoolProperty>();

	return objBoolProperty.FieldSize;
}
uint8_t UEBoolProperty::GetByteOffset()
{
	if (objBoolProperty.Empty())
		objBoolProperty = Object.Cast<UBoolProperty>();

	return objBoolProperty.ByteOffset;
}
uint8_t UEBoolProperty::GetByteMask()
{
	if (objBoolProperty.Empty())
		objBoolProperty = Object.Cast<UBoolProperty>();

	return objBoolProperty.ByteMask;
}
uint8_t UEBoolProperty::GetFieldMask()
{
	if (objBoolProperty.Empty())
		objBoolProperty = Object.Cast<UBoolProperty>();

	return objBoolProperty.FieldMask;
}
UEProperty::Info UEBoolProperty::GetInfo()
{
	if (IsNativeBool())
	{
		return Info::Create(PropertyType::Primitive, sizeof(bool), false, "bool");
	}
	return Info::Create(PropertyType::Primitive, sizeof(unsigned char), false, "unsigned char");
}
UEClass UEBoolProperty::StaticClass()
{
	static auto c = ObjectsStore().FindClass("Class CoreUObject.BoolProperty");
	return c;
}

//---------------------------------------------------------------------------
//UEObjectPropertyBase
//---------------------------------------------------------------------------
UEClass UEObjectPropertyBase::GetPropertyClass()
{
	if (objObjectPropertyBase.Empty())
		objObjectPropertyBase = Object.Cast<UObjectPropertyBase>();

	if (propertyClass.Empty())
	{
		if (!Utils::IsValidAddress(Utils::MemoryObj, objObjectPropertyBase.PropertyClass))
			return UEClass();

		propertyClass.ObjAddress = objObjectPropertyBase.PropertyClass;
		Utils::MemoryObj->Read<UObjectPropertyBase>(propertyClass.ObjAddress, propertyClass, sizeof(uintptr_t)); // Skip ObjAddress in SdkUObject
	}

	return UEClass(propertyClass);
}
UEClass UEObjectPropertyBase::StaticClass()
{
	static auto c = ObjectsStore().FindClass("Class CoreUObject.ObjectPropertyBase");
	return c;
}

//---------------------------------------------------------------------------
//UEObjectProperty
//---------------------------------------------------------------------------
UEProperty::Info UEObjectProperty::GetInfo()
{
	return Info::Create(PropertyType::Primitive, sizeof(void*), false, "class " + MakeValidName(GetPropertyClass().GetNameCPP()) + "*");
}
UEClass UEObjectProperty::StaticClass()
{
	static auto c = ObjectsStore().FindClass("Class CoreUObject.ObjectProperty");
	return c;
}

//---------------------------------------------------------------------------
//UEClassProperty
//---------------------------------------------------------------------------
UEClass UEClassProperty::GetMetaClass()
{
	if (objClassProperty.Empty())
		objClassProperty = Object.Cast<UClassProperty>();

	if (metaClass.Empty())
	{
		if (!Utils::IsValidAddress(Utils::MemoryObj, objClassProperty.MetaClass))
			return UEClass();

		metaClass.ObjAddress = objClassProperty.MetaClass;
		Utils::MemoryObj->Read<UClassProperty>(metaClass.ObjAddress, metaClass, sizeof(uintptr_t)); // Skip ObjAddress in SdkUObject
	}
	
	return UEClass(metaClass);
}
UEProperty::Info UEClassProperty::GetInfo()
{
	return Info::Create(PropertyType::Primitive, sizeof(void*), false, "class " + MakeValidName(GetMetaClass().GetNameCPP()) + "*");
}
UEClass UEClassProperty::StaticClass()
{
	static auto c = ObjectsStore().FindClass("Class CoreUObject.ClassProperty");
	return c;
}

//---------------------------------------------------------------------------
//UEInterfaceProperty
//---------------------------------------------------------------------------
UEClass UEInterfaceProperty::GetInterfaceClass()
{
	if (objInterfaceProperty.Empty())
		objInterfaceProperty = Object.Cast<UInterfaceProperty>();

	if (interfaceClass.Empty())
	{
		if (!Utils::IsValidAddress(Utils::MemoryObj, objInterfaceProperty.InterfaceClass))
			return UEClass();

		interfaceClass.ObjAddress = objInterfaceProperty.InterfaceClass;
		Utils::MemoryObj->Read<UInterfaceProperty>(interfaceClass.ObjAddress, interfaceClass, sizeof(uintptr_t)); // Skip ObjAddress in SdkUObject
	}
	
	return UEClass(interfaceClass);
}
UEProperty::Info UEInterfaceProperty::GetInfo()
{
	return Info::Create(PropertyType::PredefinedStruct, sizeof(FScriptInterface), true, "TScriptInterface<class " + MakeValidName(GetInterfaceClass().GetNameCPP()) + ">");
}
UEClass UEInterfaceProperty::StaticClass()
{
	static auto c = ObjectsStore().FindClass("Class CoreUObject.InterfaceProperty");
	return c;
}

//---------------------------------------------------------------------------
//UEWeakObjectProperty
//---------------------------------------------------------------------------
UEProperty::Info UEWeakObjectProperty::GetInfo()
{
	return Info::Create(PropertyType::Container, sizeof(FWeakObjectPtr), false, "TWeakObjectPtr<class " + MakeValidName(GetPropertyClass().GetNameCPP()) + ">");
}
UEClass UEWeakObjectProperty::StaticClass()
{
	static auto c = ObjectsStore().FindClass("Class CoreUObject.WeakObjectProperty");
	return c;
}

//---------------------------------------------------------------------------
//UELazyObjectProperty
//---------------------------------------------------------------------------
UEProperty::Info UELazyObjectProperty::GetInfo()
{
	return Info::Create(PropertyType::Container, sizeof(FLazyObjectPtr), false, "TLazyObjectPtr<class " + MakeValidName(GetPropertyClass().GetNameCPP()) + ">");
}
UEClass UELazyObjectProperty::StaticClass()
{
	static auto c = ObjectsStore().FindClass("Class CoreUObject.LazyObjectProperty");
	return c;
}

//---------------------------------------------------------------------------
//UEAssetObjectProperty
//---------------------------------------------------------------------------
UEProperty::Info UEAssetObjectProperty::GetInfo()
{
	return Info::Create(PropertyType::Container, sizeof(FAssetPtr), false, "TAssetPtr<class " + MakeValidName(GetPropertyClass().GetNameCPP()) + ">");
}
UEClass UEAssetObjectProperty::StaticClass()
{
	static auto c = ObjectsStore().FindClass("Class CoreUObject.AssetObjectProperty");
	return c;
}

//---------------------------------------------------------------------------
//UEAssetClassProperty
//---------------------------------------------------------------------------
UEClass UEAssetClassProperty::GetMetaClass()
{
	if (objAssetClassProperty.Empty())
		objAssetClassProperty = Object.Cast<UAssetClassProperty>();

	if (metaClass.Empty())
	{
		if (!Utils::IsValidAddress(Utils::MemoryObj, objAssetClassProperty.MetaClass))
			return UEClass();

		metaClass.ObjAddress = objAssetClassProperty.MetaClass;
		Utils::MemoryObj->Read<UClass>(metaClass.ObjAddress, metaClass, sizeof(uintptr_t)); // Skip ObjAddress in SdkUObject
	}
	
	return UEClass(metaClass);
}
UEProperty::Info UEAssetClassProperty::GetInfo()
{
	return Info::Create(PropertyType::Primitive, sizeof(uint8_t), false, "");
}
UEClass UEAssetClassProperty::StaticClass()
{
	static auto c = ObjectsStore().FindClass("Class CoreUObject.AssetClassProperty");
	return c;
}

//---------------------------------------------------------------------------
//UENameProperty
//---------------------------------------------------------------------------
UEProperty::Info UENameProperty::GetInfo() const
{
	return Info::Create(PropertyType::PredefinedStruct, sizeof(FName), true, "struct FName");
}
UEClass UENameProperty::StaticClass()
{
	static auto c = ObjectsStore().FindClass("Class CoreUObject.NameProperty");
	return c;
}

//---------------------------------------------------------------------------
//UEStructProperty
//---------------------------------------------------------------------------
UEScriptStruct UEStructProperty::GetStruct()
{
	if (objStructProperty.Empty())
		objStructProperty = Object.Cast<UStructProperty>();

	if (objStruct.Empty())
	{
		if (!Utils::IsValidAddress(Utils::MemoryObj, objStructProperty.Struct))
			return UEScriptStruct();

		objStruct.ObjAddress = objStructProperty.Struct;
		Utils::MemoryObj->Read<UStructProperty>(objStruct.ObjAddress, objStruct, sizeof(uintptr_t)); // Skip ObjAddress in SdkUObject
	}
	
	return UEScriptStruct(objStruct);
}
UEProperty::Info UEStructProperty::GetInfo()
{
	return Info::Create(PropertyType::CustomStruct, GetElementSize(), true, "struct " + MakeUniqueCppName(GetStruct()));
}
UEClass UEStructProperty::StaticClass()
{
	static auto c = ObjectsStore().FindClass("Class CoreUObject.StructProperty");
	return c;
}

//---------------------------------------------------------------------------
//UEStrProperty
//---------------------------------------------------------------------------
UEProperty::Info UEStrProperty::GetInfo() const
{
	return Info::Create(PropertyType::PredefinedStruct, sizeof(FString), true, "struct FString");
}
UEClass UEStrProperty::StaticClass()
{
	static auto c = ObjectsStore().FindClass("Class CoreUObject.StrProperty");
	return c;
}

//---------------------------------------------------------------------------
//UETextProperty
//---------------------------------------------------------------------------
UEProperty::Info UETextProperty::GetInfo() const
{
	return Info::Create(PropertyType::PredefinedStruct, sizeof(FText), true, "struct FText");
}
UEClass UETextProperty::StaticClass()
{
	static auto c = ObjectsStore().FindClass("Class CoreUObject.TextProperty");
	return c;
}

//---------------------------------------------------------------------------
//UEArrayProperty
//---------------------------------------------------------------------------
UEProperty UEArrayProperty::GetInner()
{
	if (objArrayProperty.Empty())
		objArrayProperty = Object.Cast<UArrayProperty>();

	if (inner.Empty())
	{
		if (!Utils::IsValidAddress(Utils::MemoryObj, objArrayProperty.Inner))
			return UEProperty();

		inner.ObjAddress = objArrayProperty.Inner;
		Utils::MemoryObj->Read<UArrayProperty>(inner.ObjAddress, inner, sizeof(uintptr_t)); // Skip ObjAddress in SdkUObject
	}
	
	return UEProperty(inner);
}
UEProperty::Info UEArrayProperty::GetInfo()
{
	auto inner = GetInner().GetInfo();
	if (inner.Type != PropertyType::Unknown)
	{
		extern IGenerator* generator;

		return Info::Create(PropertyType::Container, sizeof(TArray<void*>), false, "TArray<" + generator->GetOverrideType(inner.CppType) + ">");
	}

	return { PropertyType::Unknown };
}
UEClass UEArrayProperty::StaticClass()
{
	static auto c = ObjectsStore().FindClass("Class CoreUObject.ArrayProperty");
	return c;
}

//---------------------------------------------------------------------------
//UEMapProperty
//---------------------------------------------------------------------------
UEProperty UEMapProperty::GetKeyProperty()
{
	if (objMapProperty.Empty())
		objMapProperty = Object.Cast<UMapProperty>();

	if (keyProp.Empty())
	{
		if (!Utils::IsValidAddress(Utils::MemoryObj, objMapProperty.KeyProp))
			return UEProperty();

		keyProp.ObjAddress = objMapProperty.KeyProp;
		Utils::MemoryObj->Read<UMapProperty>(keyProp.ObjAddress, keyProp, sizeof(uintptr_t)); // Skip ObjAddress in SdkUObject
	}
	
	return UEProperty(keyProp);
}
UEProperty UEMapProperty::GetValueProperty()
{
	if (objMapProperty.Empty())
		objMapProperty = Object.Cast<UMapProperty>();

	if (valueProp.Empty())
	{
		if (!Utils::IsValidAddress(Utils::MemoryObj, objMapProperty.ValueProp))
			return UEProperty();

		valueProp.ObjAddress = objMapProperty.ValueProp;
		Utils::MemoryObj->Read<UMapProperty>(valueProp.ObjAddress, valueProp, sizeof(uintptr_t)); // Skip ObjAddress in SdkUObject
	}
	
	return UEProperty(valueProp);
}
UEProperty::Info UEMapProperty::GetInfo()
{
	auto key = GetKeyProperty().GetInfo();
	auto value = GetValueProperty().GetInfo();
	if (key.Type != PropertyType::Unknown && value.Type != PropertyType::Unknown)
	{
		extern IGenerator* generator;

		return Info::Create(PropertyType::Container, 0x50, false, "TMap<" + generator->GetOverrideType(key.CppType) + ", " + generator->GetOverrideType(value.CppType) + ">");
	}

	return { PropertyType::Unknown };
}
UEClass UEMapProperty::StaticClass()
{
	static auto c = ObjectsStore().FindClass("Class CoreUObject.MapProperty");
	return c;
}

//---------------------------------------------------------------------------
//UEDelegateProperty
//---------------------------------------------------------------------------
UEFunction UEDelegateProperty::GetSignatureFunction()
{
	if (objDelegateProperty.Empty())
		objDelegateProperty = Object.Cast<UDelegateProperty>();

	if (signatureFunction.Empty())
	{
		if (!Utils::IsValidAddress(Utils::MemoryObj, objDelegateProperty.SignatureFunction))
			return UEFunction();

		signatureFunction.ObjAddress = objDelegateProperty.SignatureFunction;
		Utils::MemoryObj->Read<UDelegateProperty>(signatureFunction.ObjAddress, signatureFunction, sizeof(uintptr_t)); // Skip ObjAddress in SdkUObject
	}
	
	return UEFunction(signatureFunction);
}
UEProperty::Info UEDelegateProperty::GetInfo()
{
	return Info::Create(PropertyType::PredefinedStruct, sizeof(FScriptDelegate), true, "struct FScriptDelegate");
}
UEClass UEDelegateProperty::StaticClass()
{
	static auto c = ObjectsStore().FindClass("Class CoreUObject.DelegateProperty");
	return c;
}

//---------------------------------------------------------------------------
//UEMulticastDelegateProperty
//---------------------------------------------------------------------------
UEFunction UEMulticastDelegateProperty::GetSignatureFunction()
{
	if (objDelegateProperty.Empty())
		objDelegateProperty = Object.Cast<UDelegateProperty>();

	if (signatureFunction.Empty())
	{
		if (!Utils::IsValidAddress(Utils::MemoryObj, objDelegateProperty.SignatureFunction))
			return UEFunction();

		signatureFunction.ObjAddress = objDelegateProperty.SignatureFunction;
		Utils::MemoryObj->Read<UDelegateProperty>(signatureFunction.ObjAddress, signatureFunction, sizeof(uintptr_t)); // Skip ObjAddress in SdkUObject
	}
	
	return UEFunction(signatureFunction);
}
UEProperty::Info UEMulticastDelegateProperty::GetInfo()
{
	return Info::Create(PropertyType::PredefinedStruct, sizeof(FScriptMulticastDelegate), true, "struct FScriptMulticastDelegate");
}
UEClass UEMulticastDelegateProperty::StaticClass()
{
	static auto c = ObjectsStore().FindClass("Class CoreUObject.MulticastDelegateProperty");
	return c;
}

//---------------------------------------------------------------------------
//UEEnumProperty
//---------------------------------------------------------------------------
UENumericProperty UEEnumProperty::GetUnderlyingProperty()
{
	if (objEnumProperty.Empty())
		objEnumProperty = Object.Cast<UEnumProperty>();

	if (underlyingProp.Empty())
	{
		if (!Utils::IsValidAddress(Utils::MemoryObj, objEnumProperty.UnderlyingProp))
			return UENumericProperty();

		underlyingProp.ObjAddress = objEnumProperty.UnderlyingProp;
		Utils::MemoryObj->Read<UEnumProperty>(underlyingProp.ObjAddress, underlyingProp, sizeof(uintptr_t)); // Skip ObjAddress in SdkUObject
	}
	
	return UENumericProperty(underlyingProp);
}
UEEnum UEEnumProperty::GetEnum()
{
	if (objEnumProperty.Empty())
		objEnumProperty = Object.Cast<UEnumProperty>();

	if (Enum.Empty())
	{
		if (!Utils::IsValidAddress(Utils::MemoryObj, objEnumProperty.Enum))
			return UEEnum();

		Enum.ObjAddress = objEnumProperty.Enum;
		Utils::MemoryObj->Read<UEnumProperty>(Enum.ObjAddress, Enum, sizeof(uintptr_t)); // Skip ObjAddress in SdkUObject
	}
	
	return UEEnum(Enum);
}
UEProperty::Info UEEnumProperty::GetInfo()
{
	return Info::Create(PropertyType::Primitive, sizeof(uint8_t), false, MakeUniqueCppName(GetEnum()));
}
UEClass UEEnumProperty::StaticClass()
{
	static auto c = ObjectsStore().FindClass("Class CoreUObject.EnumProperty");
	return c;
}
