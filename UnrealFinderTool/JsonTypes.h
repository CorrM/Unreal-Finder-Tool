// This header just to help fast coding
#pragma once
#include "JsonReflector.h"

class JsonFQWord : public JsonStruct
{
public:
	auto A() { return GetVar("A").ReadAs<int>(); }
	auto B() { return GetVar("B").ReadAs<int>(); }
};

class JsonFNameEntity : public JsonStruct
{
public:
	auto Index() { return GetVar("Index").ReadAs<int>(); }
	auto Number() { return GetVar("Number").ReadAs<int>(); }
	auto HashNext() { return GetVar("HashNext").ReadAs<uintptr_t>(); }
	auto AnsiName() { return GetVar("AnsiName").ReadAs<uintptr_t>(); }
};

class JsonFName : public JsonStruct
{
public:
	auto Index() { return GetVar("Index").ReadAs<int>(); }
	auto Number() { return GetVar("Number").ReadAs<int>(); }
};

class JsonFUObjectItem : public JsonStruct
{
public:
	auto Object() { return GetVar("Object").ReadAs<uintptr_t>(); }
	auto Flags() { return GetVar("Flags").ReadAs<int>(); }
	auto ClusterIndex() { return GetVar("ClusterIndex").ReadAs<int>(); }
	auto SerialNumber() { return GetVar("SerialNumber").ReadAs<int>(); }
};

class JsonTUObjectArray : public JsonStruct
{
public:
	auto Objects() { return GetVar("Objects").ReadAs<uintptr_t>(); }
	auto MaxElements() { return GetVar("MaxElements").ReadAs<int>(); }
	auto NumElements() { return GetVar("NumElements").ReadAs<int>(); }
};

class JsonFUObjectArray : public JsonStruct
{
public:
	auto ObjFirstGcIndex() { return GetVar("ObjFirstGcIndex").ReadAs<int>(); }
	auto ObjLastNonGcIndex() { return GetVar("ObjLastNonGcIndex").ReadAs<int>(); }
	auto MaxObjectsNotConsideredByGc() { return GetVar("MaxObjectsNotConsideredByGc").ReadAs<int>(); }
	auto OpenForDisregardForGc() { return GetVar("OpenForDisregardForGc").ReadAs<int>(); }
	auto ObjObjects() { return *static_cast<JsonTUObjectArray*>(GetVar("ObjObjects").ReadAsStruct()); }
};

class JsonUObject : public JsonStruct
{
public:
	auto VfTable() { return GetVar("VfTable").ReadAs<uintptr_t>(); }
	auto Flags() { return GetVar("Flags").ReadAs<int>(); }
	auto InternalIndex() { return GetVar("InternalIndex").ReadAs<int>(); }
	auto Class() { return GetVar("Class").ReadAs<uintptr_t>(); }
	auto Name() { return *static_cast<JsonFName*>(GetVar("Name").ReadAsStruct()); }
	auto Outer() { return static_cast<JsonUObject*>(GetVar("Outer").ReadAsPStruct()); }
};

class JsonUField : public JsonUObject
{
public:
 auto Next() { return static_cast<JsonUField*>(GetVar("Next").ReadAsPStruct()); }
};

class JsonUStruct : public JsonUField
{
public:
	auto SuperField() { return static_cast<JsonUStruct*>(GetVar("SuperField").ReadAsPStruct()); }
	auto Children() { return static_cast<JsonUField*>(GetVar("Children").ReadAsPStruct()); }
	auto PropertySize() { return GetVar("PropertySize").ReadAs<int>(); }
	auto MinAlignment() { return GetVar("MinAlignment").ReadAs<int>(); }
};

class JsonUScriptStruct : public JsonUStruct
{
public:

};

class JsonUProperty : public JsonUField
{
public:
	auto ArrayDim() { return GetVar("ArrayDim").ReadAs<int>(); }
	auto ElementSize() { return GetVar("ElementSize").ReadAs<int>(); }
	auto PropertyFlags() { return *static_cast<JsonFQWord*>(GetVar("PropertyFlags").ReadAsStruct()); }
	auto PropertySize() { return GetVar("PropertySize").ReadAs<int>(); }
	auto Offset() { return GetVar("Offset").ReadAs<int>(); }
};

class JsonUFunction : public JsonUStruct
{
public:
	auto FunctionFlags() { return GetVar("FunctionFlags").ReadAs<int>(); }
	auto RepOffset() { return GetVar("RepOffset").ReadAs<int16_t>(); }
	auto NumParms() { return GetVar("NumParms").ReadAs<int8_t>(); }
	auto ParmsSize() { return GetVar("ParmsSize").ReadAs<int16_t>(); }
	auto ReturnValueOffset() { return GetVar("ReturnValueOffset").ReadAs<int16_t>(); }
	auto RPCId() { return GetVar("RPCId").ReadAs<int16_t>(); }
	auto RPCResponseId() { return GetVar("RPCResponseId").ReadAs<int16_t>(); }
	auto FirstPropertyToInit() { return static_cast<JsonUProperty*>(GetVar("FirstPropertyToInit").ReadAsPStruct()); }
	auto EventGraphFunction() { return static_cast<JsonUFunction*>(GetVar("EventGraphFunction").ReadAsPStruct()); }
	auto EventGraphCallOffset() { return GetVar("EventGraphCallOffset").ReadAs<int>(); }
	auto Func() { return GetVar("Func").ReadAs<uintptr_t>(); }
};

class JsonUClass : public JsonUStruct
{
public:

};

class Json : public JsonStruct
{
public:
	auto VfTable() { return GetVar("VfTable").ReadAs<int>(); }
	auto Children() { return static_cast<JsonUField*>(GetVar("Children").ReadAsPStruct()); }
	auto PropertyFlags() { return *static_cast<JsonFQWord*>(GetVar("PropertyFlags").ReadAsStruct()); }
};

class JsonUNumericProperty : public JsonUProperty
{
public:

};

class JsonUByteProperty : public JsonUNumericProperty
{
public:
	auto Enum() { return static_cast<JsonUEnum*>(GetVar("Enum").ReadAsPStruct()); }
};

class JsonUUInt16Property : public JsonUNumericProperty
{
public:

};

class JsonUUInt32Property : public JsonUNumericProperty
{
public:

};

class JsonUUInt64Property : public JsonUNumericProperty
{
public:

};

class JsonUInt8Property : public JsonUNumericProperty
{
public:

};

class JsonUInt16Property : public JsonUNumericProperty
{
public:

};

class JsonUIntProperty : public JsonUNumericProperty
{
public:

};

class JsonUInt64Property : public JsonUNumericProperty
{
public:

};

class JsonUFloatProperty : public JsonUNumericProperty
{
public:

};

class JsonUDoubleProperty : public JsonUNumericProperty
{
public:

};

class JsonUBoolProperty : public JsonUProperty
{
public:
	auto FieldSize() { return GetVar("FieldSize").ReadAs<uint8_t>(); }
	auto ByteOffset() { return GetVar("ByteOffset").ReadAs<uint8_t>(); }
	auto ByteMask() { return GetVar("ByteMask").ReadAs<uint8_t>(); }
	auto FieldMask() { return GetVar("FieldMask").ReadAs<uint8_t>(); }
};

class JsonUObjectPropertyBase : public JsonUProperty
{
public:
 auto PropertyClass() { return static_cast<JsonUClass*>(GetVar("PropertyClass").ReadAsPStruct()); }
};

class JsonUObjectProperty : public JsonUObjectPropertyBase
{
public:

};

class JsonUClassProperty : public JsonUObjectProperty
{
public:
	auto MetaClass() { return static_cast<JsonUClass*>(GetVar("MetaClass").ReadAsPStruct()); }
};

class JsonUInterfaceProperty : public JsonUProperty
{
public:
	auto InterfaceClass() { return static_cast<JsonUClass*>(GetVar("InterfaceClass").ReadAsPStruct()); }
};

class JsonUWeakObjectProperty : public JsonUObjectPropertyBase
{
public:

};

class JsonULazyObjectProperty : public JsonUObjectPropertyBase
{
public:

};

class JsonUAssetObjectProperty : public JsonUObjectPropertyBase
{
public:

};

class JsonUAssetClassProperty : public JsonUAssetObjectProperty
{
public:
	auto MetaClass() { return static_cast<JsonUClass*>(GetVar("MetaClass").ReadAsPStruct()); }
};

class JsonUNameProperty : public JsonUProperty
{
public:

};

class JsonUStructProperty : public JsonUProperty
{
public:
	auto Struct() { return static_cast<JsonUScriptStruct*>(GetVar("Struct").ReadAsPStruct()); }
};

class JsonUStrProperty : public JsonUProperty
{
public:

};

class JsonUTextProperty : public JsonUProperty
{
public:

};

class JsonUArrayProperty : public JsonUProperty
{
public:
	auto Inner() { return static_cast<JsonUProperty*>(GetVar("Inner").ReadAsPStruct()); }
};

class JsonUMapProperty : public JsonUProperty
{
public:
	auto KeyProp() { return static_cast<JsonUProperty*>(GetVar("KeyProp").ReadAsPStruct()); }
	auto ValueProp() { return static_cast<JsonUProperty*>(GetVar("ValueProp").ReadAsPStruct()); }
};

class JsonUDelegateProperty : public JsonUProperty
{
public:
	auto SignatureFunction() { return static_cast<JsonUFunction*>(GetVar("SignatureFunction").ReadAsPStruct()); }
};

class JsonUMulticastDelegateProperty : public JsonUProperty
{
public:
	auto SignatureFunction() { return static_cast<JsonUFunction*>(GetVar("SignatureFunction").ReadAsPStruct()); }
};

class JsonUEnumProperty : public JsonUProperty
{
public:
	auto UnderlyingProp() { return static_cast<JsonUNumericProperty*>(GetVar("UnderlyingProp").ReadAsPStruct()); }
	auto Enum() { return static_cast<JsonUEnum*>(GetVar("Enum").ReadAsPStruct()); }
};


// class Json : public JsonStruct
// {
// public:
// 	auto VfTable() { return GetVar("VfTable").ReadAs<int>(); }
//	auto Children() { return static_cast<JsonUField*>(GetVar("Children").ReadAsPStruct()); }
//  auto PropertyFlags() { return *static_cast<JsonFQWord*>(GetVar("PropertyFlags").ReadAsStruct()); }
// };