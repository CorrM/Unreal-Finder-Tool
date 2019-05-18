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
	return Object != nullptr && Object->ObjAddress != NULL && Object->VfTable != NULL;
}
size_t UEObject::GetIndex() const
{
	return Object->InternalIndex;
}
UEClass UEObject::GetClass() const
{
	auto cls = new UClass();
	cls->ObjAddress = Object->Class;

	if (!Utils::IsValidAddress(Utils::MemoryObj, cls->ObjAddress))
		return UEClass(nullptr, false);

	Utils::MemoryObj->Read<UClass>(cls->ObjAddress, *cls, sizeof(uintptr_t)); // Skip ObjAddress in SdkUObject
	return UEClass(cls, true);
}
UEObject UEObject::GetOuter() const
{
	// Init
	auto obj = new SdkUObject();
	JsonStruct uObject;

	// Read as UObject
	uObject.ReadData(Object->Outer, "UObject");

	// Fill Data into obj
	obj->ObjAddress = Object->Outer;
	*obj = uObject;

	return UEObject(obj);
}
std::string UEObject::GetName() const
{
	auto name = NamesStore().GetById(Object->Name.ComparisonIndex);
	if (Object->Name.Number > 0)
	{
		name += '_' + std::to_string(Object->Name.Number);
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
UEField UEField::GetNext() const
{
	// Read object AS UStruct, it's like cast
	auto castType = Object->Cast<UField>();

	auto cls = new UField();
	cls->ObjAddress = castType.Next;

	if (!Utils::IsValidAddress(Utils::MemoryObj, cls->ObjAddress))
		return UEField(nullptr, false);

	Utils::MemoryObj->Read<UField>(cls->ObjAddress, *cls, sizeof(uintptr_t)); // Skip ObjAddress in SdkUObject
	return UEField(cls);
}
UEClass UEField::StaticClass()
{
	static auto c = ObjectsStore().FindClass("Class CoreUObject.Field");
	return c;
}

//---------------------------------------------------------------------------
//UEEnum
//---------------------------------------------------------------------------
std::vector<std::string> UEEnum::GetNames() const
{
	std::vector<std::string> buffer;
	auto& names = static_cast<UEnum*>(Object)->Names;

	for (auto i = 0; i < names.Num; ++i)
	{
		// buffer.push_back(NamesStore().GetById(names[i].Key.ComparisonIndex));
	}

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
	return nullptr;
}

//---------------------------------------------------------------------------
//UEStruct
//---------------------------------------------------------------------------
UEStruct UEStruct::GetSuper() const
{
	// Read object AS UStruct, it's like cast
	auto castType = Object->Cast<UStruct>();

	// Init UClass
	auto cls = new UClass();
	cls->ObjAddress = castType.SuperField;

	if (!Utils::IsValidAddress(Utils::MemoryObj, cls->ObjAddress))
		return UEStruct(nullptr, false);

	Utils::MemoryObj->Read<UClass>(cls->ObjAddress, *cls, sizeof(uintptr_t)); // Skip ObjAddress in SdkUObject
	return UEStruct(cls, true);
}
UEField UEStruct::GetChildren() const
{
	// Read object AS UStruct, it's like cast
	auto castType = Object->Cast<UStruct>();

	auto cls = new UField();
	cls->ObjAddress = castType.Children;

	if (!Utils::IsValidAddress(Utils::MemoryObj, cls->ObjAddress))
		return UEField(nullptr, false);

	Utils::MemoryObj->Read<UField>(cls->ObjAddress, *cls, sizeof(uintptr_t)); // Skip ObjAddress in SdkUObject
	return UEField(cls, true);
}
size_t UEStruct::GetPropertySize() const
{
	auto castType = Object->Cast<UStruct>();
	return castType.PropertySize;
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
UEFunctionFlags UEFunction::GetFunctionFlags() const
{
	auto castType = Object->Cast<UFunction>();
	return static_cast<UEFunctionFlags>(castType.FunctionFlags);
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
size_t UEProperty::GetArrayDim() const
{
	auto castType = Object->Cast<UProperty>();
	return castType.ArrayDim;
}
size_t UEProperty::GetElementSize() const
{
	auto castType = Object->Cast<UProperty>();
	return castType.ElementSize;
}
UEPropertyFlags UEProperty::GetPropertyFlags() const
{
	auto castType = Object->Cast<UProperty>();
	return static_cast<UEPropertyFlags>(castType.PropertyFlags.A);
}
size_t UEProperty::GetOffset() const
{
	auto castType = Object->Cast<UProperty>();
	return castType.Offset;
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
UEEnum UEByteProperty::GetEnum() const
{
	// Read object AS UByteProperty, it's like cast
	auto castType = Object->Cast<UByteProperty>();

	auto cls = new UEnum();
	cls->ObjAddress = castType.Enum;

	if (!Utils::IsValidAddress(Utils::MemoryObj, cls->ObjAddress))
		return UEEnum(nullptr, false);

	Utils::MemoryObj->Read<UEnum>(cls->ObjAddress, *cls, sizeof(uintptr_t)); // Skip ObjAddress in SdkUObject
	return UEEnum(cls, true);
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
uint8_t UEBoolProperty::GetFieldSize() const
{
	auto castType = Object->Cast<UBoolProperty>();
	return castType.FieldSize;
}
uint8_t UEBoolProperty::GetByteOffset() const
{
	auto castType = Object->Cast<UBoolProperty>();
	return castType.ByteOffset;
}
uint8_t UEBoolProperty::GetByteMask() const
{
	auto castType = Object->Cast<UBoolProperty>();
	return castType.ByteMask;
}
uint8_t UEBoolProperty::GetFieldMask() const
{
	auto castType = Object->Cast<UBoolProperty>();
	return castType.FieldMask;
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
	static auto c = ObjectsStore().FindClass("Class CoreUObject.BoolProperty");
	return c;
}

//---------------------------------------------------------------------------
//UEObjectPropertyBase
//---------------------------------------------------------------------------
UEClass UEObjectPropertyBase::GetPropertyClass() const
{
	auto castType = Object->Cast<UObjectPropertyBase>();
	return UEClass(castType.PropertyClass);
}
UEClass UEObjectPropertyBase::StaticClass()
{
	static auto c = ObjectsStore().FindClass("Class CoreUObject.ObjectPropertyBase");
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
	static auto c = ObjectsStore().FindClass("Class CoreUObject.ObjectProperty");
	return c;
}

//---------------------------------------------------------------------------
//UEClassProperty
//---------------------------------------------------------------------------
UEClass UEClassProperty::GetMetaClass() const
{
	auto castType = Object->Cast<UClassProperty>();
	return UEClass(castType.MetaClass);
}
UEProperty::Info UEClassProperty::GetInfo() const
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
UEClass UEInterfaceProperty::GetInterfaceClass() const
{
	auto castType = Object->Cast<UInterfaceProperty>();
	return UEClass(castType.InterfaceClass);
}
UEProperty::Info UEInterfaceProperty::GetInfo() const
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
UEProperty::Info UEWeakObjectProperty::GetInfo() const
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
UEProperty::Info UELazyObjectProperty::GetInfo() const
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
UEProperty::Info UEAssetObjectProperty::GetInfo() const
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
UEClass UEAssetClassProperty::GetMetaClass() const
{
	auto castType = Object->Cast<UAssetClassProperty>();
	return UEClass(castType.MetaClass);
}
UEProperty::Info UEAssetClassProperty::GetInfo() const
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
UEScriptStruct UEStructProperty::GetStruct() const
{
	auto castType = Object->Cast<UStructProperty>();
	return UEScriptStruct(castType.Struct);
}
UEProperty::Info UEStructProperty::GetInfo() const
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
UEProperty UEArrayProperty::GetInner() const
{
	auto castType = Object->Cast<UArrayProperty>();
	return UEProperty(castType.Inner);
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
	static auto c = ObjectsStore().FindClass("Class CoreUObject.ArrayProperty");
	return c;
}

//---------------------------------------------------------------------------
//UEMapProperty
//---------------------------------------------------------------------------
UEProperty UEMapProperty::GetKeyProperty() const
{
	auto castType = Object->Cast<UMapProperty>();
	return UEProperty(castType.KeyProp);
}
UEProperty UEMapProperty::GetValueProperty() const
{
	auto castType = Object->Cast<UMapProperty>();
	return UEProperty(castType.ValueProp);
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
	static auto c = ObjectsStore().FindClass("Class CoreUObject.MapProperty");
	return c;
}

//---------------------------------------------------------------------------
//UEDelegateProperty
//---------------------------------------------------------------------------
UEFunction UEDelegateProperty::GetSignatureFunction() const
{
	auto castType = Object->Cast<UDelegateProperty>();
	return UEFunction(castType.SignatureFunction);
}
UEProperty::Info UEDelegateProperty::GetInfo() const
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
UEFunction UEMulticastDelegateProperty::GetSignatureFunction() const
{
	auto castType = Object->Cast<UDelegateProperty>();
	return UEFunction(castType.SignatureFunction);
}
UEProperty::Info UEMulticastDelegateProperty::GetInfo() const
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
UENumericProperty UEEnumProperty::GetUnderlyingProperty() const
{
	auto castType = Object->Cast<UEnumProperty>();
	return UENumericProperty(castType.UnderlyingProp);
}
UEEnum UEEnumProperty::GetEnum() const
{
	auto castType = Object->Cast<UEnumProperty>();
	return UEEnum(castType.Enum);
}
UEProperty::Info UEEnumProperty::GetInfo() const
{
	return Info::Create(PropertyType::Primitive, sizeof(uint8_t), false, MakeUniqueCppName(GetEnum()));
}
UEClass UEEnumProperty::StaticClass()
{
	static auto c = ObjectsStore().FindClass("Class CoreUObject.EnumProperty");
	return c;
}
