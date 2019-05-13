// This header just to help fast coding
#pragma once
#include "JsonReflector.h"

class JsonFQWord : public JsonStruct
{
public:
	JsonFQWord() { Init("FQWord"); }
	auto A() { return GetVar("A").ReadAs<int>(); }
	auto B() { return GetVar("B").ReadAs<int>(); }
};

class JsonFNameEntity : public JsonStruct
{
public:
	JsonFNameEntity() { Init("FNameEntity"); }
	auto Index() { return GetVar("Index").ReadAs<int>(); }
	auto Number() { return GetVar("Number").ReadAs<int>(); }
	auto HashNext() { return GetVar("HashNext").ReadAs<uintptr_t>(); }
	auto AnsiName() { return GetVar("AnsiName").ReadAs<uintptr_t>(); }
};

class JsonFName : public JsonStruct
{
public:
	JsonFName() { Init("FName"); }
	auto Index() { return GetVar("Index").ReadAs<int>(); }
	auto Number() { return GetVar("Number").ReadAs<int>(); }
};

class JsonFUObjectItem : public JsonStruct
{
public:
	JsonFUObjectItem() { Init("FUObjectItem"); }
	auto Object() { return GetVar("Object").ReadAs<uintptr_t>(); }
	auto Flags() { return GetVar("Flags").ReadAs<int>(); }
	auto ClusterIndex() { return GetVar("ClusterIndex").ReadAs<int>(); }
	auto SerialNumber() { return GetVar("SerialNumber").ReadAs<int>(); }
};

class JsonTUObjectArray : public JsonStruct
{
public:
	JsonTUObjectArray() { Init("TUObjectArray"); }
	auto Objects() { return GetVar("Objects").ReadAs<uintptr_t>(); }
	auto MaxElements() { return GetVar("MaxElements").ReadAs<int>(); }
	auto NumElements() { return GetVar("NumElements").ReadAs<int>(); }
};

class JsonFUObjectArray : public JsonStruct
{
public:
	JsonFUObjectArray() { Init("FUObjectArray"); }
	auto ObjFirstGcIndex() { return GetVar("ObjFirstGcIndex").ReadAs<int>(); }
	auto ObjLastNonGcIndex() { return GetVar("ObjLastNonGcIndex").ReadAs<int>(); }
	auto MaxObjectsNotConsideredByGc() { return GetVar("MaxObjectsNotConsideredByGc").ReadAs<int>(); }
	auto OpenForDisregardForGc() { return GetVar("OpenForDisregardForGc").ReadAs<int>(); }
	auto ObjObjects() { return *static_cast<JsonTUObjectArray*>(GetVar("ObjObjects").ReadAsStruct()); }
};

class JsonUClass;

class JsonUObject : public JsonStruct
{
public:
	JsonUObject() { Init("UObject"); }
	uintptr_t GG = GetVar("VfTable").ReadAs<uintptr_t>();

	auto VfTable() { return GetVar("VfTable").ReadAs<uintptr_t>(); }
	auto Flags() { return GetVar("Flags").ReadAs<int>(); }
	auto InternalIndex() { return GetVar("InternalIndex").ReadAs<int>(); }
	auto Class() { return GetVar("Class").ReadAsPStruct<JsonUClass>(); }
	auto Name() { return *static_cast<JsonFName*>(GetVar("Name").ReadAsStruct()); }
	auto Outer() { return GetVar("Outer").ReadAsPStruct<JsonUObject>(); }
};

class JsonUField : public JsonUObject
{
public:
	JsonUField() { Init("UField"); }
	auto Next() { return GetVar("Next").ReadAsPStruct<JsonUField>(); }
};

class JsonUStruct : public JsonUField
{
public:
	JsonUStruct() { Init("UStruct"); }
	auto SuperField() { return GetVar("SuperField").ReadAsPStruct<JsonUStruct>(); }
	auto Children() { return GetVar("Children").ReadAsPStruct<JsonUField>(); }
	auto PropertySize() { return GetVar("PropertySize").ReadAs<int>(); }
	auto MinAlignment() { return GetVar("MinAlignment").ReadAs<int>(); }
};

class JsonUClass : public JsonUStruct
{
public:
	JsonUClass() { Init("UClass"); }
};

class JsonFUEnumItem : public JsonStruct
{
public:
	JsonFUEnumItem() { Init("FUEnumItem"); }
	auto Key() { return *static_cast<JsonFQWord*>(GetVar("Key").ReadAsStruct()); }
	auto Value() { return GetVar("Value").ReadAs<int>(); }
};

class JsonTUEnumArray : public JsonStruct
{
public:
	JsonTUEnumArray() { Init("TUEnumArray"); }
	auto Data() { return GetVar("Data").ReadAsPStruct<JsonFUEnumItem>(); }
	auto Count() { return GetVar("Count").ReadAs<int>(); }
	auto Max() { return GetVar("Max").ReadAs<int>(); }
};

class JsonUEnum : public JsonUField
{
public:
	JsonUEnum() { Init("UEnum"); }
	auto Children() { return GetVar("Children").ReadAsPStruct<JsonUField>(); }
	auto Names() { return *static_cast<JsonTUEnumArray*>(GetVar("Names").ReadAsStruct()); }
	auto CppForm() { return GetVar("CppForm").ReadAs<int64_t>(); }
};

class JsonUScriptStruct : public JsonUStruct
{
public:
	JsonUScriptStruct() { Init("UScriptStruct"); }
};

class JsonUProperty : public JsonUField
{
public:
	JsonUProperty() { Init("UProperty"); }
	auto ArrayDim() { return GetVar("ArrayDim").ReadAs<int>(); }
	auto ElementSize() { return GetVar("ElementSize").ReadAs<int>(); }
	auto PropertyFlags() { return *static_cast<JsonFQWord*>(GetVar("PropertyFlags").ReadAsStruct()); }
	auto PropertySize() { return GetVar("PropertySize").ReadAs<int>(); }
	auto Offset() { return GetVar("Offset").ReadAs<int>(); }
};

class JsonUFunction : public JsonUStruct
{
public:
	JsonUFunction() { Init("UFunction"); }
	auto FunctionFlags() { return GetVar("FunctionFlags").ReadAs<int>(); }
	auto RepOffset() { return GetVar("RepOffset").ReadAs<int16_t>(); }
	auto NumParms() { return GetVar("NumParms").ReadAs<int8_t>(); }
	auto ParmsSize() { return GetVar("ParmsSize").ReadAs<int16_t>(); }
	auto ReturnValueOffset() { return GetVar("ReturnValueOffset").ReadAs<int16_t>(); }
	auto RPCId() { return GetVar("RPCId").ReadAs<int16_t>(); }
	auto RPCResponseId() { return GetVar("RPCResponseId").ReadAs<int16_t>(); }
	auto FirstPropertyToInit() { return GetVar("FirstPropertyToInit").ReadAsPStruct<JsonUProperty>(); }
	auto EventGraphFunction() { return GetVar("EventGraphFunction").ReadAsPStruct<JsonUFunction>(); }
	auto EventGraphCallOffset() { return GetVar("EventGraphCallOffset").ReadAs<int>(); }
	auto Func() { return GetVar("Func").ReadAs<uintptr_t>(); }
};

class JsonUNumericProperty : public JsonUProperty
{
public:
	JsonUNumericProperty() { Init("UNumericProperty"); }
};

class JsonUByteProperty : public JsonUNumericProperty
{
public:
	JsonUByteProperty() { Init("UByteProperty"); }
	auto Enum() { return GetVar("Enum").ReadAsPStruct<JsonUEnum>(); }
};

class JsonUUInt16Property : public JsonUNumericProperty
{
public:
	JsonUUInt16Property() { Init("UUInt16Property"); }
};

class JsonUUInt32Property : public JsonUNumericProperty
{
public:
	JsonUUInt32Property() { Init("UUInt32Property"); }
};

class JsonUUInt64Property : public JsonUNumericProperty
{
public:
	JsonUUInt64Property() { Init("UUInt64Property"); }
};

class JsonUInt8Property : public JsonUNumericProperty
{
public:
	JsonUInt8Property() { Init("UInt8Property"); }
};

class JsonUInt16Property : public JsonUNumericProperty
{
public:
	JsonUInt16Property() { Init("UInt16Property"); }
};

class JsonUIntProperty : public JsonUNumericProperty
{
public:
	JsonUIntProperty() { Init("UIntProperty"); }
};

class JsonUInt64Property : public JsonUNumericProperty
{
public:
	JsonUInt64Property() { Init("UInt64Property"); }
};

class JsonUFloatProperty : public JsonUNumericProperty
{
public:
	JsonUFloatProperty() { Init("UFloatProperty"); }
};

class JsonUDoubleProperty : public JsonUNumericProperty
{
public:
	JsonUDoubleProperty() { Init("UDoubleProperty"); }
};

class JsonUBoolProperty : public JsonUProperty
{
public:
	JsonUBoolProperty() { Init("UBoolProperty"); }
	auto FieldSize() { return GetVar("FieldSize").ReadAs<uint8_t>(); }
	auto ByteOffset() { return GetVar("ByteOffset").ReadAs<uint8_t>(); }
	auto ByteMask() { return GetVar("ByteMask").ReadAs<uint8_t>(); }
	auto FieldMask() { return GetVar("FieldMask").ReadAs<uint8_t>(); }
};

class JsonUObjectPropertyBase : public JsonUProperty
{
public:
	JsonUObjectPropertyBase() { Init("UObjectPropertyBase"); }
	auto PropertyClass() { return GetVar("PropertyClass").ReadAsPStruct<JsonUClass>(); }
};

class JsonUObjectProperty : public JsonUObjectPropertyBase
{
public:
	JsonUObjectProperty() { Init("UObjectProperty"); }
};

class JsonUClassProperty : public JsonUObjectProperty
{
public:
	JsonUClassProperty() { Init("UClassProperty"); }
	auto MetaClass() { return GetVar("MetaClass").ReadAsPStruct<JsonUClass>(); }
};

class JsonUInterfaceProperty : public JsonUProperty
{
public:
	JsonUInterfaceProperty() { Init("UInterfaceProperty"); }
	auto InterfaceClass() { return GetVar("InterfaceClass").ReadAsPStruct<JsonUClass>(); }
};

class JsonUWeakObjectProperty : public JsonUObjectPropertyBase
{
public:
	JsonUWeakObjectProperty() { Init("UWeakObjectProperty"); }
};

class JsonULazyObjectProperty : public JsonUObjectPropertyBase
{
public:
	JsonULazyObjectProperty() { Init("ULazyObjectProperty"); }
};

class JsonUAssetObjectProperty : public JsonUObjectPropertyBase
{
public:
	JsonUAssetObjectProperty() { Init("UAssetObjectProperty"); }
};

class JsonUAssetClassProperty : public JsonUAssetObjectProperty
{
public:
	JsonUAssetClassProperty() { Init("UAssetClassProperty"); }
	auto MetaClass() { return GetVar("MetaClass").ReadAsPStruct<JsonUClass>(); }
};

class JsonUNameProperty : public JsonUProperty
{
public:
	JsonUNameProperty() { Init("UNameProperty"); }
};

class JsonUStructProperty : public JsonUProperty
{
public:
	JsonUStructProperty() { Init("UStructProperty"); }
	auto Struct() { return GetVar("Struct").ReadAsPStruct<JsonUScriptStruct>(); }
};

class JsonUStrProperty : public JsonUProperty
{
public:
	JsonUStrProperty() { Init("UStrProperty"); }
};

class JsonUTextProperty : public JsonUProperty
{
public:
	JsonUTextProperty() { Init("UTextProperty"); }
};

class JsonUArrayProperty : public JsonUProperty
{
public:
	JsonUArrayProperty() { Init("UArrayProperty"); }
	auto Inner() { return GetVar("Inner").ReadAsPStruct<JsonUProperty>(); }
};

class JsonUMapProperty : public JsonUProperty
{
public:
	JsonUMapProperty() { Init("UMapProperty"); }
	auto KeyProp() { return GetVar("KeyProp").ReadAsPStruct<JsonUProperty>(); }
	auto ValueProp() { return GetVar("ValueProp").ReadAsPStruct<JsonUProperty>(); }
};

class JsonUDelegateProperty : public JsonUProperty
{
public:
	JsonUDelegateProperty() { Init("UDelegateProperty"); }
	auto SignatureFunction() { return GetVar("SignatureFunction").ReadAsPStruct<JsonUFunction>(); }
};

class JsonUMulticastDelegateProperty : public JsonUProperty
{
public:
	JsonUMulticastDelegateProperty() { Init("UMulticastDelegateProperty"); }
	auto SignatureFunction() { return GetVar("SignatureFunction").ReadAsPStruct<JsonUFunction>(); }
};

class JsonUEnumProperty : public JsonUProperty
{
public:
	JsonUEnumProperty() { Init("UEnumProperty"); }
	auto UnderlyingProp() { return GetVar("UnderlyingProp").ReadAsPStruct<JsonUNumericProperty>(); }
	auto Enum() { return GetVar("Enum").ReadAsPStruct<JsonUEnum>(); }
};


// class Json : public JsonStruct
// {
// public:
// 	auto VfTable() { return GetVar("VfTable").ReadAs<int>(); }
//	auto Children() { return static_cast<JsonUField*>(GetVar("Children").ReadAsPStruct<>()); }
//  auto PropertyFlags() { return *static_cast<JsonFQWord*>(GetVar("PropertyFlags").ReadAsStruct()); }
// };