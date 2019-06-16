#include "pch.h"
#include "EngineClasses.h"
#include "IGenerator.h"
#include "ObjectsStore.h"
#include "NamesStore.h"
#include "NameValidator.h"
#include "GenericTypes.h"

#pragma region UEObject
bool UEObject::IsA(const std::string& typeName) const
{
	if (!IsValid()) return false;

	for (UEClass super = GetClass(); super.IsValid(); super = super.GetSuper().Cast<UEClass>())
	{
		if (super.GetName() == typeName || super.GetNameCpp() == typeName)
			return true;
	}

	return false;
}

uintptr_t UEObject::GetAddress() const
{
	return Object->ObjAddress;
}

bool UEObject::IsValid() const
{
	return Object->ObjAddress != NULL && Object->VfTable != NULL && (Object->Name.ComparisonIndex > 0 && size_t(Object->Name.ComparisonIndex) <= NamesStore().GetNamesNum());
}

size_t UEObject::GetIndex() const
{
	return Object->InternalIndex;
}

std::string UEObject::GetName() const
{
	// Get Original Object, if not have a value update it
	auto& obj = GetObjByAddress(GetAddress());
	if (!obj.objName.empty())
		return obj.objName;

	auto name = NamesStore().GetByIndex(Object->Name.ComparisonIndex);
	if (!name.empty() && Object->Name.Number > 0)
		name += '_' + std::to_string(Object->Name.Number);

	auto pos = name.rfind('/');
	if (pos == std::string::npos)
	{
		// Update original and local
		obj.objName = name;
		this->objName = obj.objName;

		return name;
	}

	// Update original and local
	obj.objName = name.substr(pos + 1);
	this->objName = obj.objName;

	return objName;
}

std::string UEObject::GetInstanceClassName() const
{
	if (!IsValid()) return "";

	bool find;
	auto& obj = ObjectsStore().GetByAddress(GetAddress(), find);

	return find ? obj.GetClass().GetNameCpp() : "";
}

std::string UEObject::GetFullName() const
{
	// Get Original Object, if not have a value update it
	auto& obj = GetObjByAddress(GetAddress());
	if (!obj.fullName.empty())
		return obj.fullName;

	auto cClass = GetClass();
	if (cClass.IsValid())
	{
		std::string temp;

		for (auto outer = GetOuter(); outer.IsValid(); outer = outer.GetOuter())
			temp.insert(0, outer.GetName() + ".");

		std::string name = cClass.GetName();
		name += " ";
		name += temp;
		name += GetName();

		// Update original and local
		obj.fullName = name;
		this->fullName = name;

		return fullName;
	}

	return std::string("(null)");
}

std::string UEObject::GetNameCpp() const
{
	// Get Original Object, if not have a value update it
	auto& obj = GetObjByAddress(GetAddress());
	if (!obj.nameCpp.empty())
		return obj.nameCpp;

	std::string name;
	if (IsA<UEClass>())
	{
		auto c = Cast<UEClass>();
		while (c.IsValid())
		{
			const auto className = c.GetName();
			if (className == "Actor")
			{
				name += "A";
				break;
			}
			if (className == "Object")
			{
				name += "U";
				break;
			}

			c = c.GetSuper().Cast<UEClass>();
		}
	}
	else
	{
		name += "F";
	}

	name += GetName();

	// Update original and local
	obj.nameCpp = name;
	this->nameCpp = name;

	return name;
}

UEClass UEObject::GetClass() const
{
	// Must have a class
	/*if (INVALID_POINTER_VALUE(Object->Class))
		return UEClass();*/

	return GetObjByAddress(Object->Class).Cast<UEClass>();
	// return UEClass(objClass);
}

UEObject& UEObject::GetOuter() const
{
	if (INVALID_POINTER_VALUE(Object->Outer))
		return UEObjectEmpty;

	bool found;
	UEObject& outer = ObjectsStore().GetByAddress(Object->Outer, found);

	return found ? outer : UEObjectEmpty;
}

UEObject& UEObject::GetPackageObject() const
{
	// Package Is The Last Outer
	if (packageAddress == NULL)
	{
		UObject* package = nullptr;
		for (UEObject outer = GetOuter(); outer.IsValid(); outer = outer.GetOuter())
			package = outer.Object;

		// If outer == null then this object is Package
		packageAddress = !package || package->ObjAddress == NULL ? Object->ObjAddress : package->ObjAddress;
	}

	return ObjectsStore().GetByAddress(packageAddress);
}

std::string UEObject::TypeName()
{
	static std::string ret = "UObject";
	return ret;
}

UEObject& UEObject::GetObjByAddress(const uintptr_t address)
{
	return ObjectsStore().GetByAddress(address);
}

UEClass UEObject::StaticClass()
{
	static auto c = ObjectsStore().FindClass("Class " + Utils::Settings.SdkGen.CorePackageName + ".Object");
	return c;
}
#pragma endregion

#pragma region UEField
UEField UEField::GetNext() const
{
	if (objField.Empty())
		objField = Object->Cast<UField>();

	if (INVALID_POINTER_VALUE(objField.Next))
		return UEField();

	return GetObjByAddress(objField.Next).Cast<UEField>();
	// return UEField(next);
}

std::string UEField::TypeName()
{
	static std::string ret = "UField";
	return ret;
}

UEClass UEField::StaticClass()
{
	static auto c = ObjectsStore().FindClass("Class " + Utils::Settings.SdkGen.CorePackageName + ".Field");
	return c;
}
#pragma endregion

#pragma region UEEnum
std::vector<std::string> UEEnum::GetNames() const
{
	std::vector<std::string> buffer;
	if (objEnum.Empty())
		objEnum = Object->Cast<UEnum>();

	// Get Names
	uintptr_t dataAddress = objEnum.Names.Data;
	auto cls = new FUEnumItem[objEnum.Names.Count];
	Utils::MemoryObj->ReadBytes(dataAddress, cls, sizeof(FUEnumItem) * objEnum.Names.Count);

	buffer.reserve(objEnum.Names.Count);
	for (auto i = 0; i < objEnum.Names.Count; ++i)
	{
		size_t index = cls[i].Key.ComparisonIndex;
		if (index > NamesStore().GetNamesNum() || index == 0)
			continue;

		buffer.push_back(NamesStore().GetByIndex(index));
	}

	delete[] cls;
	return buffer;
}

std::string UEEnum::TypeName()
{
	static std::string ret = "Enum";
	return ret;
}

UEClass UEEnum::StaticClass()
{
	static auto c = ObjectsStore().FindClass("Class " + Utils::Settings.SdkGen.CorePackageName + ".Enum");
	return c;
}
#pragma endregion

#pragma region UEConst
std::string UEConst::GetValue() const
{
	throw;
}

std::string UEConst::TypeName()
{
	static std::string ret = "UConst";
	return ret;
}

UEClass UEConst::StaticClass()
{
	//not supported by UE4
	return UEClass();
}
#pragma endregion

#pragma region UEStruct
UEStruct UEStruct::GetSuper() const
{
	if (objStruct.Empty())
		objStruct = Object->Cast<UStruct>();

	if (INVALID_POINTER_VALUE(objStruct.SuperField))
		return UEStruct();

	return GetObjByAddress(objStruct.SuperField).Cast<UEStruct>();
	// return UEStruct(superField);
}

UEField UEStruct::GetChildren() const
{
	if (objStruct.Empty())
		objStruct = Object->Cast<UStruct>();

	if (INVALID_POINTER_VALUE(objStruct.Children))
		return UEField();

	return GetObjByAddress(objStruct.Children).Cast<UEField>();
	// return UEField(children);
}

size_t UEStruct::GetPropertySize() const
{
	if (objStruct.Empty())
		objStruct = Object->Cast<UStruct>();

	return objStruct.PropertySize;
}

std::string UEStruct::TypeName()
{
	static std::string ret = "Struct";
	return ret;
}

UEClass UEStruct::StaticClass()
{
	static auto c = ObjectsStore().FindClass("Class " + Utils::Settings.SdkGen.CorePackageName + ".Struct");
	return c;
}
#pragma endregion

#pragma region UEScriptStruct
std::string UEScriptStruct::TypeName()
{
	static std::string ret = "ScriptStruct";
	return ret;
}

UEClass UEScriptStruct::StaticClass()
{
	static auto c = ObjectsStore().FindClass("Class " + Utils::Settings.SdkGen.CorePackageName + ".ScriptStruct");
	return c;
}
#pragma endregion

#pragma region UEFunction
UEFunctionFlags UEFunction::GetFunctionFlags() const
{
	if (objFunction.Empty())
		objFunction = Object->Cast<UFunction>();

	return static_cast<UEFunctionFlags>(objFunction.FunctionFlags);
}

std::string UEFunction::TypeName()
{
	static std::string ret = "Function";
	return ret;
}

UEClass UEFunction::StaticClass()
{
	static auto c = ObjectsStore().FindClass("Class " + Utils::Settings.SdkGen.CorePackageName + ".Function");
	return c;
}
#pragma endregion

#pragma region UEClass
std::string UEClass::TypeName()
{
	static std::string ret = "Class";
	return ret;
}

UEClass UEClass::StaticClass()
{
	static auto c = ObjectsStore().FindClass("Class " + Utils::Settings.SdkGen.CorePackageName + ".Class");
	return c;
}
#pragma endregion

#pragma region UEProperty
size_t UEProperty::GetArrayDim() const
{
	if (objProperty.Empty())
		objProperty = Object->Cast<UProperty>();

	return objProperty.ArrayDim;
}

size_t UEProperty::GetElementSize() const
{
	if (objProperty.Empty())
		objProperty = Object->Cast<UProperty>();

	return objProperty.ElementSize;
}

UEPropertyFlags UEProperty::GetPropertyFlags() const
{
	if (objProperty.Empty())
		objProperty = Object->Cast<UProperty>();

	return static_cast<UEPropertyFlags>(objProperty.PropertyFlags.A);
}

size_t UEProperty::GetOffset() const
{
	if (objProperty.Empty())
		objProperty = Object->Cast<UProperty>();

	return objProperty.Offset;
}

UEProperty::Info UEProperty::GetInfo() const
{
	if (infoChanged)
		return curInfo;

	if (IsValid())
	{
		if (IsA<UEByteProperty>())
		{
			infoChanged = true;
			curInfo = Cast<UEByteProperty>().GetInfo();
		}
		else if (IsA<UEUInt16Property>())
		{
			infoChanged = true;
			curInfo = Cast<UEUInt16Property>().GetInfo();
		}
		else if (IsA<UEUInt32Property>())
		{
			infoChanged = true;
			curInfo = Cast<UEUInt32Property>().GetInfo();
		}
		else if (IsA<UEUInt64Property>())
		{
			infoChanged = true;
			curInfo = Cast<UEUInt64Property>().GetInfo();
		}
		else if (IsA<UEInt8Property>())
		{
			infoChanged = true;
			curInfo = Cast<UEInt8Property>().GetInfo();
		}
		else if (IsA<UEInt16Property>())
		{
			infoChanged = true;
			curInfo = Cast<UEInt16Property>().GetInfo();
		}
		else if (IsA<UEIntProperty>())
		{
			infoChanged = true;
			curInfo = Cast<UEIntProperty>().GetInfo();
		}
		else if (IsA<UEInt64Property>())
		{
			infoChanged = true;
			curInfo = Cast<UEInt64Property>().GetInfo();
		}
		else if (IsA<UEFloatProperty>())
		{
			infoChanged = true;
			curInfo = Cast<UEFloatProperty>().GetInfo();
		}
		else if (IsA<UEDoubleProperty>())
		{
			infoChanged = true;
			curInfo = Cast<UEDoubleProperty>().GetInfo();
		}
		else if (IsA<UEBoolProperty>())
		{
			infoChanged = true;
			curInfo = Cast<UEBoolProperty>().GetInfo();
		}
		else if (IsA<UEObjectProperty>())
		{
			infoChanged = true;
			curInfo = Cast<UEObjectProperty>().GetInfo();
		}
		else if (IsA<UEClassProperty>())
		{
			infoChanged = true;
			curInfo = Cast<UEClassProperty>().GetInfo();
		}
		else if (IsA<UEInterfaceProperty>())
		{
			infoChanged = true;
			curInfo = Cast<UEInterfaceProperty>().GetInfo();
		}
		else if (IsA<UEWeakObjectProperty>())
		{
			infoChanged = true;
			curInfo = Cast<UEWeakObjectProperty>().GetInfo();
		}
		else if (IsA<UELazyObjectProperty>())
		{
			infoChanged = true;
			curInfo = Cast<UELazyObjectProperty>().GetInfo();
		}
		else if (IsA<UEAssetObjectProperty>())
		{
			infoChanged = true;
			curInfo = Cast<UEAssetObjectProperty>().GetInfo();
		}
		else if (IsA<UEAssetClassProperty>())
		{
			infoChanged = true;
			curInfo = Cast<UEAssetClassProperty>().GetInfo();
		}
		else if (IsA<UENameProperty>())
		{
			infoChanged = true;
			curInfo = Cast<UENameProperty>().GetInfo();
		}
		else if (IsA<UEStructProperty>())
		{
			infoChanged = true;
			curInfo = Cast<UEStructProperty>().GetInfo();
		}
		else if (IsA<UEStrProperty>())
		{
			infoChanged = true;
			curInfo = Cast<UEStrProperty>().GetInfo();
		}
		else if (IsA<UETextProperty>())
		{
			infoChanged = true;
			curInfo = Cast<UETextProperty>().GetInfo();
		}
		else if (IsA<UEArrayProperty>())
		{
			infoChanged = true;
			curInfo = Cast<UEArrayProperty>().GetInfo();
		}
		else if (IsA<UEMapProperty>())
		{
			infoChanged = true;
			curInfo = Cast<UEMapProperty>().GetInfo();
		}
		else if (IsA<UEDelegateProperty>())
		{
			infoChanged = true;
			curInfo = Cast<UEDelegateProperty>().GetInfo();
		}
		else if (IsA<UEMulticastDelegateProperty>())
		{
			infoChanged = true;
			curInfo = Cast<UEMulticastDelegateProperty>().GetInfo();
		}
		else if (IsA<UEEnumProperty>())
		{
			infoChanged = true;
			curInfo = Cast<UEEnumProperty>().GetInfo();
		}
	}

	if (infoChanged)
		return curInfo;

	return { PropertyType::Unknown };
}

std::string UEProperty::TypeName()
{
	static std::string ret = "Property";
	return ret;
}

UEClass UEProperty::StaticClass()
{
	static auto c = ObjectsStore().FindClass("Class " + Utils::Settings.SdkGen.CorePackageName + ".Property");
	return c;
}
#pragma endregion

#pragma region UENumericProperty
std::string UENumericProperty::TypeName()
{
	static std::string ret = "NumericProperty";
	return ret;
}

UEClass UENumericProperty::StaticClass()
{
	static auto c = ObjectsStore().FindClass("Class " + Utils::Settings.SdkGen.CorePackageName + ".NumericProperty");
	return c;
}
#pragma endregion

#pragma region UEUInt16Property
UEProperty::Info UEUInt16Property::GetInfo() const
{
	return Info::Create(PropertyType::Primitive, sizeof(uint16_t), false, "uint16_t");
}

std::string UEUInt16Property::TypeName()
{
	static std::string ret = "UInt16Property";
	return ret;
}

UEClass UEUInt16Property::StaticClass()
{
	static auto c = ObjectsStore().FindClass("Class " + Utils::Settings.SdkGen.CorePackageName + ".UInt16Property");
	return c;
}
#pragma endregion

#pragma region UEUInt32Property
UEProperty::Info UEUInt32Property::GetInfo() const
{
	return Info::Create(PropertyType::Primitive, sizeof(uint32_t), false, "uint32_t");
}

std::string UEUInt32Property::TypeName()
{
	static std::string ret = "UInt32Property";
	return ret;
}

UEClass UEUInt32Property::StaticClass()
{
	static auto c = ObjectsStore().FindClass("Class " + Utils::Settings.SdkGen.CorePackageName + ".UInt32Property");
	return c;
}
#pragma endregion

#pragma region UEUInt64Property
UEProperty::Info UEUInt64Property::GetInfo() const
{
	return Info::Create(PropertyType::Primitive, sizeof(uint64_t), false, "uint64_t");
}

std::string UEUInt64Property::TypeName()
{
	static std::string ret = "UInt64Property";
	return ret;
}

UEClass UEUInt64Property::StaticClass()
{
	static auto c = ObjectsStore().FindClass("Class " + Utils::Settings.SdkGen.CorePackageName + ".UInt64Property");
	return c;
}
#pragma endregion

#pragma region UEInt8Property
UEProperty::Info UEInt8Property::GetInfo() const
{
	return Info::Create(PropertyType::Primitive, sizeof(int8_t), false, "int8_t");
}

std::string UEInt8Property::TypeName()
{
	static std::string ret = "Int8Property";
	return ret;
}

UEClass UEInt8Property::StaticClass()
{
	static auto c = ObjectsStore().FindClass("Class " + Utils::Settings.SdkGen.CorePackageName + ".Int8Property");
	return c;
}
#pragma endregion

#pragma region UEInt16Property
UEProperty::Info UEInt16Property::GetInfo() const
{
	return Info::Create(PropertyType::Primitive, sizeof(int16_t), false, "int16_t");
}

std::string UEInt16Property::TypeName()
{
	static std::string ret = "Int16Property";
	return ret;
}

UEClass UEInt16Property::StaticClass()
{
	static auto c = ObjectsStore().FindClass("Class " + Utils::Settings.SdkGen.CorePackageName + ".Int16Property");
	return c;
}
#pragma endregion

#pragma region UEIntProperty
UEProperty::Info UEIntProperty::GetInfo() const
{
	return Info::Create(PropertyType::Primitive, sizeof(int), false, "int");
}

std::string UEIntProperty::TypeName()
{
	static std::string ret = "IntProperty";
	return ret;
}

UEClass UEIntProperty::StaticClass()
{
	static auto c = ObjectsStore().FindClass("Class " + Utils::Settings.SdkGen.CorePackageName + ".IntProperty");
	return c;
}
#pragma endregion

#pragma region UEInt64Property
UEProperty::Info UEInt64Property::GetInfo() const
{
	return Info::Create(PropertyType::Primitive, sizeof(int64_t), false, "int64_t");
}

std::string UEInt64Property::TypeName()
{
	static std::string ret = "Int64Property";
	return ret;
}

UEClass UEInt64Property::StaticClass()
{
	static auto c = ObjectsStore().FindClass("Class " + Utils::Settings.SdkGen.CorePackageName + ".Int64Property");
	return c;
}
#pragma endregion

#pragma region UEFloatProperty
UEProperty::Info UEFloatProperty::GetInfo() const
{
	return Info::Create(PropertyType::Primitive, sizeof(float), false, "float");
}

std::string UEFloatProperty::TypeName()
{
	static std::string ret = "FloatProperty";
	return ret;
}

UEClass UEFloatProperty::StaticClass()
{
	static auto c = ObjectsStore().FindClass("Class " + Utils::Settings.SdkGen.CorePackageName + ".FloatProperty");
	return c;
}
#pragma endregion

#pragma region UEDoubleProperty
UEProperty::Info UEDoubleProperty::GetInfo() const
{
	return Info::Create(PropertyType::Primitive, sizeof(double), false, "double");
}

std::string UEDoubleProperty::TypeName()
{
	static std::string ret = "DoubleProperty";
	return ret;
}

UEClass UEDoubleProperty::StaticClass()
{
	static auto c = ObjectsStore().FindClass("Class " + Utils::Settings.SdkGen.CorePackageName + ".DoubleProperty");
	return c;
}
#pragma endregion

#pragma region UEObjectPropertyBase
UEClass UEObjectPropertyBase::GetPropertyClass() const
{
	if (objObjectPropertyBase.Empty())
		objObjectPropertyBase = Object->Cast<UObjectPropertyBase>();

	if (INVALID_POINTER_VALUE(objObjectPropertyBase.PropertyClass))
		return UEClass();

	return GetObjByAddress(objObjectPropertyBase.PropertyClass).Cast<UEClass>();
	// return UEClass(propertyClass);
}

std::string UEObjectPropertyBase::TypeName()
{
	static std::string ret = "ObjectPropertyBase";
	return ret;
}

UEClass UEObjectPropertyBase::StaticClass()
{
	static auto c = ObjectsStore().FindClass("Class " + Utils::Settings.SdkGen.CorePackageName + ".ObjectPropertyBase");
	return c;
}
#pragma endregion

#pragma region UEObjectProperty
UEProperty::Info UEObjectProperty::GetInfo() const
{
	return Info::Create(PropertyType::Primitive, sizeof(void*), false, "class " + MakeValidName(GetPropertyClass().GetNameCpp()) + "*");
}

std::string UEObjectProperty::TypeName()
{
	static std::string ret = "ObjectProperty";
	return ret;
}

UEClass UEObjectProperty::StaticClass()
{
	static auto c = ObjectsStore().FindClass("Class " + Utils::Settings.SdkGen.CorePackageName + ".ObjectProperty");
	return c;
}
#pragma endregion

#pragma region UEClassProperty
UEClass UEClassProperty::GetMetaClass() const
{
	if (objClassProperty.Empty())
		objClassProperty = Object->Cast<UClassProperty>();

	if (INVALID_POINTER_VALUE(objClassProperty.MetaClass))
		return UEClass();

	return GetObjByAddress(objClassProperty.MetaClass).Cast<UEClass>();
	// return UEClass(metaClass);
}

UEProperty::Info UEClassProperty::GetInfo() const
{
	return Info::Create(PropertyType::Primitive, sizeof(void*), false, "class " + MakeValidName(GetMetaClass().GetNameCpp()) + "*");
}

std::string UEClassProperty::TypeName()
{
	static std::string ret = "ClassProperty";
	return ret;
}

UEClass UEClassProperty::StaticClass()
{
	static auto c = ObjectsStore().FindClass("Class " + Utils::Settings.SdkGen.CorePackageName + ".ClassProperty");
	return c;
}
#pragma endregion

#pragma region UEInterfaceProperty
UEClass UEInterfaceProperty::GetInterfaceClass() const
{
	if (objInterfaceProperty.Empty())
		objInterfaceProperty = Object->Cast<UInterfaceProperty>();

	if (INVALID_POINTER_VALUE(objInterfaceProperty.InterfaceClass))
		return UEClass();

	return GetObjByAddress(objInterfaceProperty.InterfaceClass).Cast<UEClass>();
	// return UEClass(interfaceClass);
}

UEProperty::Info UEInterfaceProperty::GetInfo() const
{
	return Info::Create(PropertyType::PredefinedStruct, sizeof(FScriptInterface), true, "TScriptInterface<class " + MakeValidName(GetInterfaceClass().GetNameCpp()) + ">");
}

std::string UEInterfaceProperty::TypeName()
{
	static std::string ret = "InterfaceProperty";
	return ret;
}

UEClass UEInterfaceProperty::StaticClass()
{
	static auto c = ObjectsStore().FindClass("Class " + Utils::Settings.SdkGen.CorePackageName + ".InterfaceProperty");
	return c;
}
#pragma endregion

#pragma region UEWeakObjectProperty
UEProperty::Info UEWeakObjectProperty::GetInfo() const
{
	return Info::Create(PropertyType::Container, sizeof(FWeakObjectPtr), false, "TWeakObjectPtr<class " + MakeValidName(GetPropertyClass().GetNameCpp()) + ">");
}

std::string UEWeakObjectProperty::TypeName()
{
	static std::string ret = "WeakObjectProperty";
	return ret;
}

UEClass UEWeakObjectProperty::StaticClass()
{
	static auto c = ObjectsStore().FindClass("Class " + Utils::Settings.SdkGen.CorePackageName + ".WeakObjectProperty");
	return c;
}
#pragma endregion

#pragma region UELazyObjectProperty
UEProperty::Info UELazyObjectProperty::GetInfo() const
{
	return Info::Create(PropertyType::Container, sizeof(FLazyObjectPtr), false, "TLazyObjectPtr<class " + MakeValidName(GetPropertyClass().GetNameCpp()) + ">");
}

std::string UELazyObjectProperty::TypeName()
{
	static std::string ret = "LazyObjectProperty";
	return ret;
}

UEClass UELazyObjectProperty::StaticClass()
{
	static auto c = ObjectsStore().FindClass("Class " + Utils::Settings.SdkGen.CorePackageName + ".LazyObjectProperty");
	return c;
}
#pragma endregion

#pragma region UEAssetObjectProperty
UEProperty::Info UEAssetObjectProperty::GetInfo() const
{
	return Info::Create(PropertyType::Container, sizeof(FAssetPtr), false, "TAssetPtr<class " + MakeValidName(GetPropertyClass().GetNameCpp()) + ">");
}

std::string UEAssetObjectProperty::TypeName()
{
	static std::string ret = "AssetObjectProperty";
	return ret;
}

UEClass UEAssetObjectProperty::StaticClass()
{
	static auto c = ObjectsStore().FindClass("Class " + Utils::Settings.SdkGen.CorePackageName + ".AssetObjectProperty");
	return c;
}
#pragma endregion

#pragma region UEAssetClassProperty
UEClass UEAssetClassProperty::GetMetaClass() const
{
	if (objAssetClassProperty.Empty())
		objAssetClassProperty = Object->Cast<UAssetClassProperty>();

	if (INVALID_POINTER_VALUE(objAssetClassProperty.MetaClass))
		return UEClass();

	return GetObjByAddress(objAssetClassProperty.MetaClass).Cast<UEClass>();
	// return UEClass(metaClass);
}

UEProperty::Info UEAssetClassProperty::GetInfo() const
{
	return Info::Create(PropertyType::Primitive, sizeof(uint8_t), false, "");
}

std::string UEAssetClassProperty::TypeName()
{
	static std::string ret = "AssetClassProperty";
	return ret;
}

UEClass UEAssetClassProperty::StaticClass()
{
	static auto c = ObjectsStore().FindClass("Class " + Utils::Settings.SdkGen.CorePackageName + ".AssetClassProperty");
	return c;
}
#pragma endregion

#pragma region UENameProperty
UEProperty::Info UENameProperty::GetInfo() const
{
	return Info::Create(PropertyType::PredefinedStruct, sizeof(FName), true, "struct FName");
}

std::string UENameProperty::TypeName()
{
	static std::string ret = "NameProperty";
	return ret;
}

UEClass UENameProperty::StaticClass()
{
	static auto c = ObjectsStore().FindClass("Class " + Utils::Settings.SdkGen.CorePackageName + ".NameProperty");
	return c;
}
#pragma endregion

#pragma region UEStructProperty
UEScriptStruct UEStructProperty::GetStruct() const
{
	if (objStructProperty.Empty())
		objStructProperty = Object->Cast<UStructProperty>();

	if (INVALID_POINTER_VALUE(objStructProperty.Struct))
		return UEScriptStruct();

	return GetObjByAddress(objStructProperty.Struct).Cast<UEScriptStruct>();
	// return UEScriptStruct(objStruct);
}

UEProperty::Info UEStructProperty::GetInfo() const
{
	return Info::Create(PropertyType::CustomStruct, GetElementSize(), true, "struct " + MakeUniqueCppName(GetStruct()));
}

std::string UEStructProperty::TypeName()
{
	static std::string ret = "StructProperty";
	return ret;
}

UEClass UEStructProperty::StaticClass()
{
	static auto c = ObjectsStore().FindClass("Class " + Utils::Settings.SdkGen.CorePackageName + ".StructProperty");
	return c;
}
#pragma endregion

#pragma region UEStrProperty
UEProperty::Info UEStrProperty::GetInfo() const
{
	return Info::Create(PropertyType::PredefinedStruct, sizeof(FString), true, "struct FString");
}

std::string UEStrProperty::TypeName()
{
	static std::string ret = "StrProperty";
	return ret;
}

UEClass UEStrProperty::StaticClass()
{
	static auto c = ObjectsStore().FindClass("Class " + Utils::Settings.SdkGen.CorePackageName + ".StrProperty");
	return c;
}
#pragma endregion

#pragma region UETextProperty
UEProperty::Info UETextProperty::GetInfo() const
{
	return Info::Create(PropertyType::PredefinedStruct, sizeof(FText), true, "struct FText");
}

std::string UETextProperty::TypeName()
{
	static std::string ret = "TextProperty";
	return ret;
}

UEClass UETextProperty::StaticClass()
{
	static auto c = ObjectsStore().FindClass("Class " + Utils::Settings.SdkGen.CorePackageName + ".TextProperty");
	return c;
}
#pragma endregion

#pragma region UEArrayProperty
UEProperty UEArrayProperty::GetInner() const
{
	if (objArrayProperty.Empty())
		objArrayProperty = Object->Cast<UArrayProperty>();

	if (INVALID_POINTER_VALUE(objArrayProperty.Inner))
		return UEProperty();

	return GetObjByAddress(objArrayProperty.Inner).Cast<UEProperty>();
	// return UEProperty(inner);
}

UEProperty::Info UEArrayProperty::GetInfo() const
{
	auto inner = GetInner().GetInfo();
	if (inner.Type != PropertyType::Unknown)
	{
		extern IGenerator* generator;

		return Info::Create(PropertyType::Container, sizeof(TArray), false, "TArray<" + generator->GetOverrideType(inner.CppType) + ">");
	}

	return { PropertyType::Unknown };
}

std::string UEArrayProperty::TypeName()
{
	static std::string ret = "ArrayProperty";
	return ret;
}

UEClass UEArrayProperty::StaticClass()
{
	static auto c = ObjectsStore().FindClass("Class " + Utils::Settings.SdkGen.CorePackageName + ".ArrayProperty");
	return c;
}
#pragma endregion

#pragma region UEMapProperty
UEProperty UEMapProperty::GetKeyProperty() const
{
	if (objMapProperty.Empty())
		objMapProperty = Object->Cast<UMapProperty>();

	if (INVALID_POINTER_VALUE(objMapProperty.KeyProp))
		return UEProperty();

	return GetObjByAddress(objMapProperty.KeyProp).Cast<UEProperty>();
	// return UEProperty(keyProp);
}

UEProperty UEMapProperty::GetValueProperty() const
{
	if (objMapProperty.Empty())
		objMapProperty = Object->Cast<UMapProperty>();

	if (INVALID_POINTER_VALUE(objMapProperty.ValueProp))
		return UEProperty();

	return GetObjByAddress(objMapProperty.ValueProp).Cast<UEProperty>();
	// return UEProperty(valueProp);
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

std::string UEMapProperty::TypeName()
{
	static std::string ret = "MapProperty";
	return ret;
}

UEClass UEMapProperty::StaticClass()
{
	static auto c = ObjectsStore().FindClass("Class " + Utils::Settings.SdkGen.CorePackageName + ".MapProperty");
	return c;
}
#pragma endregion

#pragma region UEDelegateProperty
UEFunction UEDelegateProperty::GetSignatureFunction() const
{
	if (objDelegateProperty.Empty())
		objDelegateProperty = Object->Cast<UDelegateProperty>();

	if (INVALID_POINTER_VALUE(objDelegateProperty.SignatureFunction))
		return UEFunction();

	return GetObjByAddress(objDelegateProperty.SignatureFunction).Cast<UEFunction>();
	// return UEFunction(signatureFunction);
}

UEProperty::Info UEDelegateProperty::GetInfo() const
{
	return Info::Create(PropertyType::PredefinedStruct, sizeof(FScriptDelegate), true, "struct FScriptDelegate");
}

std::string UEDelegateProperty::TypeName()
{
	static std::string ret = "DelegateProperty";
	return ret;
}

UEClass UEDelegateProperty::StaticClass()
{
	static auto c = ObjectsStore().FindClass("Class " + Utils::Settings.SdkGen.CorePackageName + ".DelegateProperty");
	return c;
}
#pragma endregion

#pragma region UEMulticastDelegateProperty
UEFunction UEMulticastDelegateProperty::GetSignatureFunction() const
{
	if (objDelegateProperty.Empty())
		objDelegateProperty = Object->Cast<UDelegateProperty>();

	if (INVALID_POINTER_VALUE(objDelegateProperty.SignatureFunction))
		return UEFunction();

	return GetObjByAddress(objDelegateProperty.SignatureFunction).Cast<UEFunction>();
	// return UEFunction(signatureFunction);
}

UEProperty::Info UEMulticastDelegateProperty::GetInfo() const
{
	return Info::Create(PropertyType::PredefinedStruct, sizeof(FScriptMulticastDelegate), true, "struct FScriptMulticastDelegate");
}

std::string UEMulticastDelegateProperty::TypeName()
{
	static std::string ret = "MulticastDelegateProperty";
	return ret;
}

UEClass UEMulticastDelegateProperty::StaticClass()
{
	static auto c = ObjectsStore().FindClass("Class " + Utils::Settings.SdkGen.CorePackageName + ".MulticastDelegateProperty");
	return c;
}
#pragma endregion

#pragma region UEEnumProperty
UENumericProperty UEEnumProperty::GetUnderlyingProperty() const
{
	if (objEnumProperty.Empty())
		objEnumProperty = Object->Cast<UEnumProperty>();

	if (INVALID_POINTER_VALUE(objEnumProperty.UnderlyingProp))
		return UENumericProperty();

	return GetObjByAddress(objEnumProperty.UnderlyingProp).Cast<UENumericProperty>();
	// return UENumericProperty(underlyingProp);
}

UEEnum UEEnumProperty::GetEnum() const
{
	if (objEnumProperty.Empty())
		objEnumProperty = Object->Cast<UEnumProperty>();

	if (INVALID_POINTER_VALUE(objEnumProperty.Enum))
		return UEEnum();

	return GetObjByAddress(objEnumProperty.Enum).Cast<UEEnum>();
	// return UEEnum(Enum);
}

UEProperty::Info UEEnumProperty::GetInfo() const
{
	return Info::Create(PropertyType::Primitive, sizeof(uint8_t), false, MakeUniqueCppName(GetEnum()));
}

std::string UEEnumProperty::TypeName()
{
	static std::string ret = "EnumProperty";
	return ret;
}

UEClass UEEnumProperty::StaticClass()
{
	static auto c = ObjectsStore().FindClass("Class " + Utils::Settings.SdkGen.CorePackageName + ".EnumProperty");
	return c;
}
#pragma endregion

#pragma region UEByteProperty
bool UEByteProperty::IsEnum() const
{
	return GetEnum().IsValid();
}

UEEnum UEByteProperty::GetEnum() const
{
	if (objByteProperty.Empty())
		objByteProperty = Object->Cast<UByteProperty>();

	if (INVALID_POINTER_VALUE(objByteProperty.Enum))
		return UEEnum();

	return GetObjByAddress(objByteProperty.Enum).Cast<UEEnum>();
	// return UEEnum(enumProperty);
}

UEProperty::Info UEByteProperty::GetInfo() const
{
	if (IsEnum())
	{
		return Info::Create(PropertyType::Primitive, sizeof(uint8_t), false, "TEnumAsByte<" + MakeUniqueCppName(GetEnum()) + ">");
	}
	return Info::Create(PropertyType::Primitive, sizeof(uint8_t), false, "unsigned char");
}

std::string UEByteProperty::TypeName()
{
	static std::string ret = "ByteProperty";
	return ret;
}

UEClass UEByteProperty::StaticClass()
{
	static auto c = ObjectsStore().FindClass("Class " + Utils::Settings.SdkGen.CorePackageName + ".ByteProperty");
	return c;
}
#pragma endregion

#pragma region UEBoolProperty
int GetBitPosition(uint8_t value)
{
	int i4 = !(value & 0xf) << 2;
	value >>= i4;

	int i2 = !(value & 0x3) << 1;
	value >>= i2;

	int i1 = !(value & 0x1);

	int i0 = (value >> i1) & 1 ? 0 : -8;

	return i4 + i2 + i1 + i0;
}

std::array<int, 2> UEBoolProperty::GetMissingBitsCount(const UEBoolProperty & other) const
{
	// If there is no previous bitfield member, just calculate the missing bits.
	if (!other.IsValid())
	{
		return { GetBitPosition(GetByteMask()), -1 };
	}

	// If both bitfield member belong to the same byte, calculate the bit position difference.
	if (GetOffset() == other.GetOffset())
	{
		return { GetBitPosition(GetByteMask()) - GetBitPosition(other.GetByteMask()) - 1, -1 };
	}

	// If they have different offsets, we need two distances
	// |00001000|00100000|
	// 1.   ^---^
	// 2.       ^--^

	return { std::numeric_limits<uint8_t>::digits - GetBitPosition(other.GetByteMask()) - 1, GetBitPosition(GetByteMask()) };
}

uint8_t UEBoolProperty::GetFieldSize() const
{
	if (objBoolProperty.Empty())
		objBoolProperty = Object->Cast<UBoolProperty>();

	return objBoolProperty.FieldSize;
}

uint8_t UEBoolProperty::GetByteOffset() const
{
	if (objBoolProperty.Empty())
		objBoolProperty = Object->Cast<UBoolProperty>();

	return objBoolProperty.ByteOffset;
}

uint8_t UEBoolProperty::GetByteMask() const
{
	if (objBoolProperty.Empty())
		objBoolProperty = Object->Cast<UBoolProperty>();

	return objBoolProperty.ByteMask;
}

uint8_t UEBoolProperty::GetFieldMask() const
{
	if (objBoolProperty.Empty())
		objBoolProperty = Object->Cast<UBoolProperty>();

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

std::string UEBoolProperty::TypeName()
{
	static std::string ret = "BoolProperty";
	return ret;
}

UEClass UEBoolProperty::StaticClass()
{
	static auto c = ObjectsStore().FindClass("Class " + Utils::Settings.SdkGen.CorePackageName + ".BoolProperty");
	return c;
}
#pragma endregion