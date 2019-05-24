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
	return Object.ObjAddress != NULL && Object.VfTable != NULL && (Object.Name.ComparisonIndex > 0 && size_t(Object.Name.ComparisonIndex) <= NamesStore().GetNamesNum());
}
size_t UEObject::GetIndex() const
{
	return Object.InternalIndex;
}
UEClass UEObject::GetClass() const
{
	if (objClass.Empty())
	{
		if (INVALID_POINTER_VALUE(Object.Class))
			return UEClass();

		objClass.ObjAddress = Object.Class;
		Utils::MemoryObj->Read<UClass>(objClass.ObjAddress, objClass, sizeof(uintptr_t)); // Skip ObjAddress in SdkUObject
		objClass.FixPointers(sizeof UClass);
	}

	return UEClass(objClass);
}
UEObject UEObject::GetOuter() const
{
	/*
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
	*/
	if (objOuter.Empty())
	{
		if (INVALID_POINTER_VALUE(Object.Outer))
			return UEObject();

		objOuter.ObjAddress = Object.Outer;
		Utils::MemoryObj->Read<UObject>(objOuter.ObjAddress, objOuter, sizeof(uintptr_t)); // Skip ObjAddress in SdkUObject
		objOuter.FixPointers(sizeof UObject);
	}

	return UEObject(objOuter);
}
std::string UEObject::GetName() const
{
	if (!objName.empty())
		return objName;

	auto name = NamesStore().GetById(Object.Name.ComparisonIndex);
	if (!name.empty() && Object.Name.Number > 0)
	{
		name += '_' + std::to_string(Object.Name.Number);
	}

	auto pos = name.rfind('/');
	if (pos == std::string::npos)
	{
		objName = name;
		return name;
	}

	objName = name.substr(pos + 1);
	return objName;
}
UEClass UEObject::StaticClass()
{
	static auto c = ObjectsStore().FindClass("Class "+ Utils::Settings.SdkGen.CorePackageName + ".Object");
	return c;
}

//---------------------------------------------------------------------------
//UEField
//---------------------------------------------------------------------------
UEField UEField::GetNext() const
{
	if (objField.Empty())
		objField = Object.Cast<UField>();

	if (next.Empty())
	{
		if (INVALID_POINTER_VALUE(objField.Next))
			return UEField();

		next.ObjAddress = objField.Next;
		Utils::MemoryObj->Read<UField>(next.ObjAddress, next, sizeof(uintptr_t)); // Skip ObjAddress in SdkUObject
		next.FixPointers(sizeof UField);
	}

	return UEField(next);
}
UEClass UEField::StaticClass()
{
	static auto c = ObjectsStore().FindClass("Class " + Utils::Settings.SdkGen.CorePackageName + ".Field");
	return c;
}

//---------------------------------------------------------------------------
//UEEnum
//---------------------------------------------------------------------------
std::vector<std::string> UEEnum::GetNames() const
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
	static auto c = ObjectsStore().FindClass("Class " + Utils::Settings.SdkGen.CorePackageName + ".Enum");
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
UEStruct UEStruct::GetSuper() const
{
	if (objStruct.Empty())
		objStruct = Object.Cast<UStruct>();

	if (superField.Empty())
	{
		if (INVALID_POINTER_VALUE(objStruct.SuperField))
			return UEStruct();

		superField.ObjAddress = objStruct.SuperField;
		Utils::MemoryObj->Read<UStruct>(superField.ObjAddress, superField, sizeof(uintptr_t)); // Skip ObjAddress in SdkUObject
		superField.FixPointers(sizeof UStruct);
	}

	//Sleep(1);
	return UEStruct(superField);
}
UEField UEStruct::GetChildren() const
{
	if (objStruct.Empty())
		objStruct = Object.Cast<UStruct>();

	if (children.Empty())
	{
		if (INVALID_POINTER_VALUE(objStruct.Children))
			return UEField();

		children.ObjAddress = objStruct.Children;
		Utils::MemoryObj->Read<UField>(children.ObjAddress, children, sizeof(uintptr_t)); // Skip ObjAddress in SdkUObject
		children.FixPointers(sizeof UField);
	}

	return UEField(children);
}
size_t UEStruct::GetPropertySize() const
{
	if (objStruct.Empty())
		objStruct = Object.Cast<UStruct>();

	return objStruct.PropertySize;
}
UEClass UEStruct::StaticClass()
{
	static auto c = ObjectsStore().FindClass("Class " + Utils::Settings.SdkGen.CorePackageName + ".Struct");
	return c;
}

//---------------------------------------------------------------------------
//UEScriptStruct
//---------------------------------------------------------------------------
UEClass UEScriptStruct::StaticClass()
{
	static auto c = ObjectsStore().FindClass("Class " + Utils::Settings.SdkGen.CorePackageName + ".ScriptStruct");
	return c;
}

//---------------------------------------------------------------------------
//UEFunction
//---------------------------------------------------------------------------
UEFunctionFlags UEFunction::GetFunctionFlags() const
{
	if (objFunction.Empty())
		objFunction = Object.Cast<UFunction>();

	return static_cast<UEFunctionFlags>(objFunction.FunctionFlags);
}
UEClass UEFunction::StaticClass()
{
	static auto c = ObjectsStore().FindClass("Class " + Utils::Settings.SdkGen.CorePackageName + ".Function");
	return c;
}

//---------------------------------------------------------------------------
//UEClass
//---------------------------------------------------------------------------
UEClass UEClass::StaticClass()
{
	static auto c = ObjectsStore().FindClass("Class " + Utils::Settings.SdkGen.CorePackageName + ".Class");
	return c;
}

//---------------------------------------------------------------------------
//UEProperty
//---------------------------------------------------------------------------
size_t UEProperty::GetArrayDim() const
{
	if (objProperty.Empty())
		objProperty = Object.Cast<UProperty>();

	return objProperty.ArrayDim;
}
size_t UEProperty::GetElementSize() const
{
	if (objProperty.Empty())
		objProperty = Object.Cast<UProperty>();

	return objProperty.ElementSize;
}
UEPropertyFlags UEProperty::GetPropertyFlags() const
{
	if (objProperty.Empty())
		objProperty = Object.Cast<UProperty>();

	return static_cast<UEPropertyFlags>(objProperty.PropertyFlags.A);
}
size_t UEProperty::GetOffset() const
{
	if (objProperty.Empty())
		objProperty = Object.Cast<UProperty>();

	return objProperty.Offset;
}
UEClass UEProperty::StaticClass()
{
	static auto c = ObjectsStore().FindClass("Class " + Utils::Settings.SdkGen.CorePackageName + ".Property");
	return c;
}

//---------------------------------------------------------------------------
//UENumericProperty
//---------------------------------------------------------------------------
UEClass UENumericProperty::StaticClass()
{
	static auto c = ObjectsStore().FindClass("Class " + Utils::Settings.SdkGen.CorePackageName + ".NumericProperty");
	return c;
}

//---------------------------------------------------------------------------
//UEByteProperty
//---------------------------------------------------------------------------
UEEnum UEByteProperty::GetEnum() const
{
	if (objByteProperty.Empty())
		objByteProperty = Object.Cast<UByteProperty>();

	if (enumProperty.Empty())
	{
		if (INVALID_POINTER_VALUE(objByteProperty.Enum))
			return UEEnum();

		enumProperty.ObjAddress = objByteProperty.Enum;
		Utils::MemoryObj->Read<UEnum>(enumProperty.ObjAddress, enumProperty, sizeof(uintptr_t)); // Skip ObjAddress in SdkUObject
		enumProperty.FixPointers(sizeof UEnum);
	}

	return UEEnum(enumProperty);
}
UEProperty::Info UEByteProperty::GetInfo() const
{
	if (IsEnum())
	{
		return Info::Create(PropertyType::Primitive, sizeof(uint8_t), false, "TEnumAsByte<" + MakeUniqueCppName(GetEnum()) + ">");
	}
	return Info::Create(PropertyType::Primitive, sizeof(uint8_t), false, "unsigned char");
}
UEClass UEByteProperty::StaticClass()
{
	static auto c = ObjectsStore().FindClass("Class " + Utils::Settings.SdkGen.CorePackageName + ".ByteProperty");
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
	static auto c = ObjectsStore().FindClass("Class " + Utils::Settings.SdkGen.CorePackageName + ".UInt16Property");
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
	static auto c = ObjectsStore().FindClass("Class " + Utils::Settings.SdkGen.CorePackageName + ".UInt32Property");
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
	static auto c = ObjectsStore().FindClass("Class " + Utils::Settings.SdkGen.CorePackageName + ".UInt64Property");
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
	static auto c = ObjectsStore().FindClass("Class " + Utils::Settings.SdkGen.CorePackageName + ".Int8Property");
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
	static auto c = ObjectsStore().FindClass("Class " + Utils::Settings.SdkGen.CorePackageName + ".Int16Property");
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
	static auto c = ObjectsStore().FindClass("Class " + Utils::Settings.SdkGen.CorePackageName + ".IntProperty");
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
	static auto c = ObjectsStore().FindClass("Class " + Utils::Settings.SdkGen.CorePackageName + ".Int64Property");
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
	static auto c = ObjectsStore().FindClass("Class " + Utils::Settings.SdkGen.CorePackageName + ".FloatProperty");
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
	static auto c = ObjectsStore().FindClass("Class " + Utils::Settings.SdkGen.CorePackageName + ".DoubleProperty");
	return c;
}

//---------------------------------------------------------------------------
//UEBoolProperty
//---------------------------------------------------------------------------
uint8_t UEBoolProperty::GetFieldSize() const
{
	if (objBoolProperty.Empty())
		objBoolProperty = Object.Cast<UBoolProperty>();

	return objBoolProperty.FieldSize;
}
uint8_t UEBoolProperty::GetByteOffset() const
{
	if (objBoolProperty.Empty())
		objBoolProperty = Object.Cast<UBoolProperty>();

	return objBoolProperty.ByteOffset;
}
uint8_t UEBoolProperty::GetByteMask() const
{
	if (objBoolProperty.Empty())
		objBoolProperty = Object.Cast<UBoolProperty>();

	return objBoolProperty.ByteMask;
}
uint8_t UEBoolProperty::GetFieldMask() const
{
	if (objBoolProperty.Empty())
		objBoolProperty = Object.Cast<UBoolProperty>();

	return objBoolProperty.FieldMask;
}
UEProperty::Info UEBoolProperty::GetInfo() const
{
	if (IsNativeBool())
	{
		return Info::Create(PropertyType::Primitive, sizeof(bool), false, "bool");
	}
	return Info::Create(PropertyType::Primitive, sizeof(unsigned char), false, "unsigned char");
}
UEClass UEBoolProperty::StaticClass()
{
	static auto c = ObjectsStore().FindClass("Class " + Utils::Settings.SdkGen.CorePackageName + ".BoolProperty");
	return c;
}

//---------------------------------------------------------------------------
//UEObjectPropertyBase
//---------------------------------------------------------------------------
UEClass UEObjectPropertyBase::GetPropertyClass() const
{
	if (objObjectPropertyBase.Empty())
		objObjectPropertyBase = Object.Cast<UObjectPropertyBase>();

	if (propertyClass.Empty())
	{
		if (INVALID_POINTER_VALUE(objObjectPropertyBase.PropertyClass))
			return UEClass();

		propertyClass.ObjAddress = objObjectPropertyBase.PropertyClass;
		Utils::MemoryObj->Read<UObjectPropertyBase>(propertyClass.ObjAddress, propertyClass, sizeof(uintptr_t)); // Skip ObjAddress in SdkUObject
		propertyClass.FixPointers(sizeof UObjectPropertyBase);
	}

	return UEClass(propertyClass);
}
UEClass UEObjectPropertyBase::StaticClass()
{
	static auto c = ObjectsStore().FindClass("Class " + Utils::Settings.SdkGen.CorePackageName + ".ObjectPropertyBase");
	return c;
}

//---------------------------------------------------------------------------
//UEObjectProperty
//---------------------------------------------------------------------------
UEProperty::Info UEObjectProperty::GetInfo() const
{
	return Info::Create(PropertyType::Primitive, sizeof(void*), false, "class " + MakeValidName(GetPropertyClass().GetNameCPP()) + "*");
}
UEClass UEObjectProperty::StaticClass()
{
	static auto c = ObjectsStore().FindClass("Class " + Utils::Settings.SdkGen.CorePackageName + ".ObjectProperty");
	return c;
}

//---------------------------------------------------------------------------
//UEClassProperty
//---------------------------------------------------------------------------
UEClass UEClassProperty::GetMetaClass() const
{
	if (objClassProperty.Empty())
		objClassProperty = Object.Cast<UClassProperty>();

	if (metaClass.Empty())
	{
		if (INVALID_POINTER_VALUE(objClassProperty.MetaClass))
			return UEClass();

		metaClass.ObjAddress = objClassProperty.MetaClass;
		Utils::MemoryObj->Read<UClassProperty>(metaClass.ObjAddress, metaClass, sizeof(uintptr_t)); // Skip ObjAddress in SdkUObject
		metaClass.FixPointers(sizeof UClassProperty);
	}

	return UEClass(metaClass);
}
UEProperty::Info UEClassProperty::GetInfo() const
{
	return Info::Create(PropertyType::Primitive, sizeof(void*), false, "class " + MakeValidName(GetMetaClass().GetNameCPP()) + "*");
}
UEClass UEClassProperty::StaticClass()
{
	static auto c = ObjectsStore().FindClass("Class " + Utils::Settings.SdkGen.CorePackageName + ".ClassProperty");
	return c;
}

//---------------------------------------------------------------------------
//UEInterfaceProperty
//---------------------------------------------------------------------------
UEClass UEInterfaceProperty::GetInterfaceClass() const
{
	if (objInterfaceProperty.Empty())
		objInterfaceProperty = Object.Cast<UInterfaceProperty>();

	if (interfaceClass.Empty())
	{
		if (INVALID_POINTER_VALUE(objInterfaceProperty.InterfaceClass))
			return UEClass();

		interfaceClass.ObjAddress = objInterfaceProperty.InterfaceClass;
		Utils::MemoryObj->Read<UInterfaceProperty>(interfaceClass.ObjAddress, interfaceClass, sizeof(uintptr_t)); // Skip ObjAddress in SdkUObject
		interfaceClass.FixPointers(sizeof UInterfaceProperty);
	}

	return UEClass(interfaceClass);
}
UEProperty::Info UEInterfaceProperty::GetInfo() const
{
	return Info::Create(PropertyType::PredefinedStruct, sizeof(FScriptInterface), true, "TScriptInterface<class " + MakeValidName(GetInterfaceClass().GetNameCPP()) + ">");
}
UEClass UEInterfaceProperty::StaticClass()
{
	static auto c = ObjectsStore().FindClass("Class " + Utils::Settings.SdkGen.CorePackageName + ".InterfaceProperty");
	return c;
}

//---------------------------------------------------------------------------
//UEWeakObjectProperty
//---------------------------------------------------------------------------
UEProperty::Info UEWeakObjectProperty::GetInfo() const
{
	return Info::Create(PropertyType::Container, sizeof(FWeakObjectPtr), false, "TWeakObjectPtr<class " + MakeValidName(GetPropertyClass().GetNameCPP()) + ">");
}
UEClass UEWeakObjectProperty::StaticClass()
{
	static auto c = ObjectsStore().FindClass("Class " + Utils::Settings.SdkGen.CorePackageName + ".WeakObjectProperty");
	return c;
}

//---------------------------------------------------------------------------
//UELazyObjectProperty
//---------------------------------------------------------------------------
UEProperty::Info UELazyObjectProperty::GetInfo() const
{
	return Info::Create(PropertyType::Container, sizeof(FLazyObjectPtr), false, "TLazyObjectPtr<class " + MakeValidName(GetPropertyClass().GetNameCPP()) + ">");
}
UEClass UELazyObjectProperty::StaticClass()
{
	static auto c = ObjectsStore().FindClass("Class " + Utils::Settings.SdkGen.CorePackageName + ".LazyObjectProperty");
	return c;
}

//---------------------------------------------------------------------------
//UEAssetObjectProperty
//---------------------------------------------------------------------------
UEProperty::Info UEAssetObjectProperty::GetInfo() const
{
	return Info::Create(PropertyType::Container, sizeof(FAssetPtr), false, "TAssetPtr<class " + MakeValidName(GetPropertyClass().GetNameCPP()) + ">");
}
UEClass UEAssetObjectProperty::StaticClass()
{
	static auto c = ObjectsStore().FindClass("Class " + Utils::Settings.SdkGen.CorePackageName + ".AssetObjectProperty");
	return c;
}

//---------------------------------------------------------------------------
//UEAssetClassProperty
//---------------------------------------------------------------------------
UEClass UEAssetClassProperty::GetMetaClass() const
{
	if (objAssetClassProperty.Empty())
		objAssetClassProperty = Object.Cast<UAssetClassProperty>();

	if (metaClass.Empty())
	{
		if (INVALID_POINTER_VALUE(objAssetClassProperty.MetaClass))
			return UEClass();

		metaClass.ObjAddress = objAssetClassProperty.MetaClass;
		Utils::MemoryObj->Read<UClass>(metaClass.ObjAddress, metaClass, sizeof(uintptr_t)); // Skip ObjAddress in SdkUObject
		metaClass.FixPointers(sizeof UClass);
	}

	return UEClass(metaClass);
}
UEProperty::Info UEAssetClassProperty::GetInfo() const
{
	return Info::Create(PropertyType::Primitive, sizeof(uint8_t), false, "");
}
UEClass UEAssetClassProperty::StaticClass()
{
	static auto c = ObjectsStore().FindClass("Class " + Utils::Settings.SdkGen.CorePackageName + ".AssetClassProperty");
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
	static auto c = ObjectsStore().FindClass("Class " + Utils::Settings.SdkGen.CorePackageName + ".NameProperty");
	return c;
}

//---------------------------------------------------------------------------
//UEStructProperty
//---------------------------------------------------------------------------
UEScriptStruct UEStructProperty::GetStruct() const
{
	if (objStructProperty.Empty())
		objStructProperty = Object.Cast<UStructProperty>();

	if (objStruct.Empty())
	{
		if (INVALID_POINTER_VALUE(objStructProperty.Struct))
			return UEScriptStruct();

		objStruct.ObjAddress = objStructProperty.Struct;
		Utils::MemoryObj->Read<UStructProperty>(objStruct.ObjAddress, objStruct, sizeof(uintptr_t)); // Skip ObjAddress in SdkUObject
		objStruct.FixPointers(sizeof UStructProperty);
	}

	return UEScriptStruct(objStruct);
}
UEProperty::Info UEStructProperty::GetInfo() const
{
	return Info::Create(PropertyType::CustomStruct, GetElementSize(), true, "struct " + MakeUniqueCppName(GetStruct()));
}
UEClass UEStructProperty::StaticClass()
{
	static auto c = ObjectsStore().FindClass("Class " + Utils::Settings.SdkGen.CorePackageName + ".StructProperty");
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
	static auto c = ObjectsStore().FindClass("Class " + Utils::Settings.SdkGen.CorePackageName + ".StrProperty");
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
	static auto c = ObjectsStore().FindClass("Class " + Utils::Settings.SdkGen.CorePackageName + ".TextProperty");
	return c;
}

//---------------------------------------------------------------------------
//UEArrayProperty
//---------------------------------------------------------------------------
UEProperty UEArrayProperty::GetInner() const
{
	if (objArrayProperty.Empty())
		objArrayProperty = Object.Cast<UArrayProperty>();

	if (inner.Empty())
	{
		if (INVALID_POINTER_VALUE(objArrayProperty.Inner))
			return UEProperty();

		inner.ObjAddress = objArrayProperty.Inner;
		Utils::MemoryObj->Read<UArrayProperty>(inner.ObjAddress, inner, sizeof(uintptr_t)); // Skip ObjAddress in SdkUObject
		inner.FixPointers(sizeof UArrayProperty);
	}

	return UEProperty(inner);
}
UEProperty::Info UEArrayProperty::GetInfo() const
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
	static auto c = ObjectsStore().FindClass("Class " + Utils::Settings.SdkGen.CorePackageName + ".ArrayProperty");
	return c;
}

//---------------------------------------------------------------------------
//UEMapProperty
//---------------------------------------------------------------------------
UEProperty UEMapProperty::GetKeyProperty() const
{
	if (objMapProperty.Empty())
		objMapProperty = Object.Cast<UMapProperty>();

	if (keyProp.Empty())
	{
		if (INVALID_POINTER_VALUE(objMapProperty.KeyProp))
			return UEProperty();

		keyProp.ObjAddress = objMapProperty.KeyProp;
		Utils::MemoryObj->Read<UMapProperty>(keyProp.ObjAddress, keyProp, sizeof(uintptr_t)); // Skip ObjAddress in SdkUObject
		keyProp.FixPointers(sizeof UMapProperty);
	}

	return UEProperty(keyProp);
}
UEProperty UEMapProperty::GetValueProperty() const
{
	if (objMapProperty.Empty())
		objMapProperty = Object.Cast<UMapProperty>();

	if (valueProp.Empty())
	{
		if (INVALID_POINTER_VALUE(objMapProperty.ValueProp))
			return UEProperty();

		valueProp.ObjAddress = objMapProperty.ValueProp;
		Utils::MemoryObj->Read<UMapProperty>(valueProp.ObjAddress, valueProp, sizeof(uintptr_t)); // Skip ObjAddress in SdkUObject
		valueProp.FixPointers(sizeof UMapProperty);
	}

	return UEProperty(valueProp);
}
UEProperty::Info UEMapProperty::GetInfo() const
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
	static auto c = ObjectsStore().FindClass("Class " + Utils::Settings.SdkGen.CorePackageName + ".MapProperty");
	return c;
}

//---------------------------------------------------------------------------
//UEDelegateProperty
//---------------------------------------------------------------------------
UEFunction UEDelegateProperty::GetSignatureFunction() const
{
	if (objDelegateProperty.Empty())
		objDelegateProperty = Object.Cast<UDelegateProperty>();

	if (signatureFunction.Empty())
	{
		if (INVALID_POINTER_VALUE(objDelegateProperty.SignatureFunction))
			return UEFunction();

		signatureFunction.ObjAddress = objDelegateProperty.SignatureFunction;
		Utils::MemoryObj->Read<UDelegateProperty>(signatureFunction.ObjAddress, signatureFunction, sizeof(uintptr_t)); // Skip ObjAddress in SdkUObject
		signatureFunction.FixPointers(sizeof UDelegateProperty);
	}

	return UEFunction(signatureFunction);
}
UEProperty::Info UEDelegateProperty::GetInfo() const
{
	return Info::Create(PropertyType::PredefinedStruct, sizeof(FScriptDelegate), true, "struct FScriptDelegate");
}
UEClass UEDelegateProperty::StaticClass()
{
	static auto c = ObjectsStore().FindClass("Class " + Utils::Settings.SdkGen.CorePackageName + ".DelegateProperty");
	return c;
}

//---------------------------------------------------------------------------
//UEMulticastDelegateProperty
//---------------------------------------------------------------------------
UEFunction UEMulticastDelegateProperty::GetSignatureFunction() const
{
	if (objDelegateProperty.Empty())
		objDelegateProperty = Object.Cast<UDelegateProperty>();

	if (signatureFunction.Empty())
	{
		if (INVALID_POINTER_VALUE(objDelegateProperty.SignatureFunction))
			return UEFunction();

		signatureFunction.ObjAddress = objDelegateProperty.SignatureFunction;
		Utils::MemoryObj->Read<UDelegateProperty>(signatureFunction.ObjAddress, signatureFunction, sizeof(uintptr_t)); // Skip ObjAddress in SdkUObject
		signatureFunction.FixPointers(sizeof UDelegateProperty);
	}

	return UEFunction(signatureFunction);
}
UEProperty::Info UEMulticastDelegateProperty::GetInfo() const
{
	return Info::Create(PropertyType::PredefinedStruct, sizeof(FScriptMulticastDelegate), true, "struct FScriptMulticastDelegate");
}
UEClass UEMulticastDelegateProperty::StaticClass()
{
	static auto c = ObjectsStore().FindClass("Class " + Utils::Settings.SdkGen.CorePackageName + ".MulticastDelegateProperty");
	return c;
}

//---------------------------------------------------------------------------
//UEEnumProperty
//---------------------------------------------------------------------------
UENumericProperty UEEnumProperty::GetUnderlyingProperty() const
{
	if (objEnumProperty.Empty())
		objEnumProperty = Object.Cast<UEnumProperty>();

	if (underlyingProp.Empty())
	{
		if (INVALID_POINTER_VALUE(objEnumProperty.UnderlyingProp))
			return UENumericProperty();

		underlyingProp.ObjAddress = objEnumProperty.UnderlyingProp;
		Utils::MemoryObj->Read<UEnumProperty>(underlyingProp.ObjAddress, underlyingProp, sizeof(uintptr_t)); // Skip ObjAddress in SdkUObject
		underlyingProp.FixPointers(sizeof UEnumProperty);
	}

	return UENumericProperty(underlyingProp);
}
UEEnum UEEnumProperty::GetEnum() const
{
	if (objEnumProperty.Empty())
		objEnumProperty = Object.Cast<UEnumProperty>();

	if (Enum.Empty())
	{
		if (INVALID_POINTER_VALUE(objEnumProperty.Enum))
			return UEEnum();

		Enum.ObjAddress = objEnumProperty.Enum;
		Utils::MemoryObj->Read<UEnumProperty>(Enum.ObjAddress, Enum, sizeof(uintptr_t)); // Skip ObjAddress in SdkUObject
		Enum.FixPointers(sizeof UEnumProperty);
	}

	return UEEnum(Enum);
}
UEProperty::Info UEEnumProperty::GetInfo() const
{
	return Info::Create(PropertyType::Primitive, sizeof(uint8_t), false, MakeUniqueCppName(GetEnum()));
}
UEClass UEEnumProperty::StaticClass()
{
	static auto c = ObjectsStore().FindClass("Class " + Utils::Settings.SdkGen.CorePackageName + ".EnumProperty");
	return c;
}
