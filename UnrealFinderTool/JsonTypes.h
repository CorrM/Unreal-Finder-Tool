// This header just to help fast coding
#pragma once
#include "JsonReflector.h"

class JsonDefault : public JsonStruct
{

	std::string JsonName() override
	{
		return "";
	}
	void InitData() override
	{
	}
};

class JsonFQWord : public JsonStruct
{
public:
	JsonFQWord() { Init("FQWord"); }
	int A = 0;
	int B = 0;

	std::string JsonName() override
	{
		return "FQWord";
	}
	void InitData() override
	{
		A = GetVar("A").ReadAs<int>();
		B = GetVar("B").ReadAs<int>();
	}
};

class JsonFNameEntity : public JsonStruct
{
public:
	JsonFNameEntity() { Init("FNameEntity"); }
	int Index = 0;
	int Number = 0;
	uintptr_t HashNext = 0;
	uintptr_t AnsiName = 0;

	std::string JsonName() override
	{
		return "FNameEntity";
	}
	void InitData() override
	{
		Index = GetVar("Index").ReadAs<int>();
		Number = GetVar("Number").ReadAs<int>();
		HashNext = GetVar("HashNext").ReadAs<uintptr_t>();
		AnsiName = GetVar("AnsiName").ReadAs<uintptr_t>();
	}
};

class JsonFName : public JsonStruct
{
public:
	JsonFName() { Init("FName"); }
	int Index = 0;
	int Number = 0;

	std::string JsonName() override
	{
		return "FName";
	}
	void InitData() override
	{
		Index = GetVar("Index").ReadAs<int>();
		Number = GetVar("Number").ReadAs<int>();
	}
};

class JsonFUObjectItem : public JsonStruct
{
public:
	JsonFUObjectItem() { Init("FUObjectItem"); }
	uintptr_t Object = 0;
	int Flags = 0;
	int ClusterIndex = 0;
	int SerialNumber = 0;

	std::string JsonName() override
	{
		return "FUObjectItem";
	}
	void InitData() override
	{
		Object = GetVar("Object").ReadAs<uintptr_t>();
		Flags = GetVar("Flags").ReadAs<int>();
		ClusterIndex = GetVar("ClusterIndex").ReadAs<int>();
		SerialNumber = GetVar("SerialNumber").ReadAs<int>();
	}
};

class JsonTUObjectArray : public JsonStruct
{
public:
	JsonTUObjectArray() { Init("TUObjectArray"); }
	uintptr_t Objects = 0;
	int MaxElements = 0;
	int NumElements = 0;

	std::string JsonName() override
	{
		return "TUObjectArray";
	}
	void InitData() override
	{
		Objects = GetVar("Objects").ReadAs<uintptr_t>();
		MaxElements = GetVar("MaxElements").ReadAs<int>();
		NumElements = GetVar("NumElements").ReadAs<int>();
	}
};

class JsonFUObjectArray : public JsonStruct
{
public:
	JsonFUObjectArray() { Init("FUObjectArray"); }
	int ObjFirstGcIndex = 0;
	int ObjLastNonGcIndex = 0;
	int MaxObjectsNotConsideredByGc = 0;
	int OpenForDisregardForGc = 0;
	JsonTUObjectArray ObjObjects;

	std::string JsonName() override
	{
		return "FUObjectArray";
	}
	void InitData() override
	{
		ObjFirstGcIndex = GetVar("ObjFirstGcIndex").ReadAs<int>();
		ObjLastNonGcIndex = GetVar("ObjLastNonGcIndex").ReadAs<int>();
		MaxObjectsNotConsideredByGc = GetVar("MaxObjectsNotConsideredByGc").ReadAs<int>();
		OpenForDisregardForGc = GetVar("OpenForDisregardForGc").ReadAs<int>();
		ObjObjects = GetVar("ObjObjects").ReadAsStruct<JsonTUObjectArray>();
	}
};

class JsonUClass;

class JsonUObject : public JsonStruct
{
public:
	JsonUObject() { Init("UObject"); }
	uintptr_t VfTable = 0;
	int Flags = 0;
	int InternalIndex = 0;
	JsonUClass* Class = nullptr;
	JsonFName Name;
	JsonUObject* Outer = nullptr;

	std::string JsonName() override
	{
		return "UObject";
	}
	void InitData() override
	{
		VfTable = GetVar("VfTable").ReadAs<uintptr_t>();
		Flags = GetVar("Flags").ReadAs<int>();
		InternalIndex = GetVar("InternalIndex").ReadAs<int>();
		Class = GetVar("Class").ReadAsPStruct<JsonUClass>();
		Name = GetVar("Name").ReadAsStruct<JsonFName>();
		Outer = GetVar("Outer").ReadAsPStruct<JsonUObject>();
	}
};

class JsonUField : public JsonUObject
{
public:
	JsonUField() { Init("UField"); }
	JsonUField* Next = nullptr;

	std::string JsonName() override
	{
		return "UField";
	}
	void InitData() override
	{
		Next = GetVar("Next").ReadAsPStruct<JsonUField>();
	}
};

class JsonUStruct : public JsonUField
{
public:
	JsonUStruct() { Init("UStruct"); }
	JsonUStruct* SuperField = nullptr;
	JsonUField* Children = nullptr;
	int PropertySize = 0;
	int MinAlignment = 0;

	std::string JsonName() override
	{
		return "UStruct";
	}
	void InitData() override
	{
		SuperField = GetVar("SuperField").ReadAsPStruct<JsonUStruct>();
		Children = GetVar("Children").ReadAsPStruct<JsonUField>();
		PropertySize = GetVar("PropertySize").ReadAs<int>();
		MinAlignment = GetVar("MinAlignment").ReadAs<int>();
	}
};

class JsonUClass : public JsonUStruct
{
public:
	JsonUClass() { Init("UClass"); }

	std::string JsonName() override
	{
		return "UClass";
	}
	void InitData() override
	{

	}
};

class JsonFUEnumItem : public JsonStruct
{
public:
	JsonFUEnumItem() { Init("FUEnumItem"); }
	JsonFQWord Key;
	int Value = 0;

	std::string JsonName() override
	{
		return "FUEnumItem";
	}
	void InitData() override
	{
		Key = GetVar("Key").ReadAsStruct<JsonFQWord>();
		Value = GetVar("Value").ReadAs<int>();
	}
};

class JsonTUEnumArray : public JsonStruct
{
public:
	JsonTUEnumArray() { Init("TUEnumArray"); }
	JsonFUEnumItem* Data = nullptr;
	int Count = 0;
	int Max = 0;

	std::string JsonName() override
	{
		return "TUEnumArray";
	}
	void InitData() override
	{
		Data = GetVar("Data").ReadAsPStruct<JsonFUEnumItem>();
		Count = GetVar("Count").ReadAs<int>();
		Max = GetVar("Max").ReadAs<int>();
	}
};

class JsonUEnum : public JsonUField
{
public:
	JsonUEnum() { Init("UEnum"); }
	JsonUField* Children = nullptr;
	JsonTUEnumArray Names;
	int64_t CppForm = 0;

	std::string JsonName() override
	{
		return "UEnum";
	}
	void InitData() override
	{
		Children = GetVar("Children").ReadAsPStruct<JsonUField>();
		Names = GetVar("Names").ReadAsStruct<JsonTUEnumArray>();
		CppForm = GetVar("CppForm").ReadAs<int64_t>();
	}
};

class JsonUScriptStruct : public JsonUStruct
{
public:
	JsonUScriptStruct() { Init("UScriptStruct"); }

	std::string JsonName() override
	{
		return "UScriptStruct";
	}
	void InitData() override
	{

	}
};

class JsonUProperty : public JsonUField
{
public:
	JsonUProperty() { Init("UProperty"); }
	int ArrayDim = 0;
	int ElementSize = 0;
	JsonFQWord PropertyFlags;
	int PropertySize = 0;
	int Offset = 0;

	std::string JsonName() override
	{
		return "UProperty";
	}
	void InitData() override
	{
		ArrayDim = GetVar("ArrayDim").ReadAs<int>();
		ElementSize = GetVar("ElementSize").ReadAs<int>();
		PropertyFlags = GetVar("PropertyFlags").ReadAsStruct<JsonFQWord>();
		PropertySize = GetVar("PropertySize").ReadAs<int>();
		Offset = GetVar("Offset").ReadAs<int>();
	}
};

class JsonUFunction : public JsonUStruct
{
public:
	JsonUFunction() { Init("UFunction"); }
	int FunctionFlags = 0;
	int16_t RepOffset = 0;
	int8_t NumParms = 0;
	int16_t ParmsSize = 0;
	int16_t ReturnValueOffset = 0;
	int16_t RPCId = 0;
	int16_t RPCResponseId = 0;
	JsonUProperty* FirstPropertyToInit = nullptr;
	JsonUFunction* EventGraphFunction = nullptr;
	int EventGraphCallOffset = 0;
	uintptr_t Func = 0;

	std::string JsonName() override
	{
		return "UFunction";
	}
	void InitData() override
	{
		FunctionFlags = GetVar("FunctionFlags").ReadAs<int>();
		RepOffset = GetVar("RepOffset").ReadAs<int16_t>();
		NumParms = GetVar("NumParms").ReadAs<int8_t>();
		ParmsSize = GetVar("ParmsSize").ReadAs<int16_t>();
		ReturnValueOffset = GetVar("ReturnValueOffset").ReadAs<int16_t>();
		RPCId = GetVar("RPCId").ReadAs<int16_t>();
		RPCResponseId = GetVar("RPCResponseId").ReadAs<int16_t>();
		FirstPropertyToInit = GetVar("FirstPropertyToInit").ReadAsPStruct<JsonUProperty>();
		EventGraphFunction = GetVar("EventGraphFunction").ReadAsPStruct<JsonUFunction>();
		EventGraphCallOffset = GetVar("EventGraphCallOffset").ReadAs<int>();
		Func = GetVar("Func").ReadAs<uintptr_t>();
	}
};

class JsonUNumericProperty : public JsonUProperty
{
public:
	JsonUNumericProperty() { Init("UNumericProperty"); }

	std::string JsonName() override
	{
		return "UNumericProperty";
	}
	void InitData() override
	{

	}
};

class JsonUByteProperty : public JsonUNumericProperty
{
public:
	JsonUByteProperty() { Init("UByteProperty"); }
	JsonUEnum* Enum = nullptr;

	std::string JsonName() override
	{
		return "UByteProperty";
	}
	void InitData() override
	{
		Enum = GetVar("Enum").ReadAsPStruct<JsonUEnum>();
	}
};

class JsonUUInt16Property : public JsonUNumericProperty
{
public:
	JsonUUInt16Property() { Init("UUInt16Property"); }

	std::string JsonName() override
	{
		return "UUInt16Property";
	}
	void InitData() override
	{

	}
};

class JsonUUInt32Property : public JsonUNumericProperty
{
public:
	JsonUUInt32Property() { Init("UUInt32Property"); }

	std::string JsonName() override
	{
		return "UUInt32Property";
	}
	void InitData() override
	{

	}
};

class JsonUUInt64Property : public JsonUNumericProperty
{
public:
	JsonUUInt64Property() { Init("UUInt64Property"); }

	std::string JsonName() override
	{
		return "UUInt64Property";
	}
	void InitData() override
	{

	}
};

class JsonUInt8Property : public JsonUNumericProperty
{
public:
	JsonUInt8Property() { Init("UInt8Property"); }

	std::string JsonName() override
	{
		return "UInt8Property";
	}
	void InitData() override
	{

	}
};

class JsonUInt16Property : public JsonUNumericProperty
{
public:
	JsonUInt16Property() { Init("UInt16Property"); }

	std::string JsonName() override
	{
		return "UInt16Property";
	}
	void InitData() override
	{

	}
};

class JsonUIntProperty : public JsonUNumericProperty
{
public:
	JsonUIntProperty() { Init("UIntProperty"); }

	std::string JsonName() override
	{
		return "UIntProperty";
	}
	void InitData() override
	{

	}
};

class JsonUInt64Property : public JsonUNumericProperty
{
public:
	JsonUInt64Property() { Init("UInt64Property"); }

	std::string JsonName() override
	{
		return "UInt64Property";
	}
	void InitData() override
	{

	}
};

class JsonUFloatProperty : public JsonUNumericProperty
{
public:
	JsonUFloatProperty() { Init("UFloatProperty"); }

	std::string JsonName() override
	{
		return "UFloatProperty";
	}
	void InitData() override
	{

	}
};

class JsonUDoubleProperty : public JsonUNumericProperty
{
public:
	JsonUDoubleProperty() { Init("UDoubleProperty"); }

	std::string JsonName() override
	{
		return "UDoubleProperty";
	}
	void InitData() override
	{

	}
};

class JsonUBoolProperty : public JsonUProperty
{
public:
	JsonUBoolProperty() { Init("UBoolProperty"); }
	uint8_t FieldSize = 0;
	uint8_t ByteOffset = 0;
	uint8_t ByteMask = 0;
	uint8_t FieldMask = 0;

	std::string JsonName() override
	{
		return "UBoolProperty";
	}
	void InitData() override
	{
		FieldSize = GetVar("FieldSize").ReadAs<uint8_t>();
		ByteOffset = GetVar("ByteOffset").ReadAs<uint8_t>();
		ByteMask = GetVar("ByteMask").ReadAs<uint8_t>();
		FieldMask = GetVar("FieldMask").ReadAs<uint8_t>();
	}
};

class JsonUObjectPropertyBase : public JsonUProperty
{
public:
	JsonUObjectPropertyBase() { Init("UObjectPropertyBase"); }
	JsonUClass* PropertyClass = nullptr;

	std::string JsonName() override
	{
		return "UObjectPropertyBase";
	}
	void InitData() override
	{
		PropertyClass = GetVar("PropertyClass").ReadAsPStruct<JsonUClass>();
	}
};

class JsonUObjectProperty : public JsonUObjectPropertyBase
{
public:
	JsonUObjectProperty() { Init("UObjectProperty"); }

	std::string JsonName() override
	{
		return "UObjectProperty";
	}
	void InitData() override
	{

	}
};

class JsonUClassProperty : public JsonUObjectProperty
{
public:
	JsonUClassProperty() { Init("UClassProperty"); }
	JsonUClass* MetaClass = nullptr;

	std::string JsonName() override
	{
		return "UClassProperty";
	}
	void InitData() override
	{
		MetaClass = GetVar("MetaClass").ReadAsPStruct<JsonUClass>();
	}
};

class JsonUInterfaceProperty : public JsonUProperty
{
public:
	JsonUInterfaceProperty() { Init("UInterfaceProperty"); }
	JsonUClass* InterfaceClass = nullptr;

	std::string JsonName() override
	{
		return "UInterfaceProperty";
	}
	void InitData() override
	{
		InterfaceClass = GetVar("InterfaceClass").ReadAsPStruct<JsonUClass>();
	}
};

class JsonUWeakObjectProperty : public JsonUObjectPropertyBase
{
public:
	JsonUWeakObjectProperty() { Init("UWeakObjectProperty"); }

	std::string JsonName() override
	{
		return "UWeakObjectProperty";
	}
	void InitData() override
	{

	}
};

class JsonULazyObjectProperty : public JsonUObjectPropertyBase
{
public:
	JsonULazyObjectProperty() { Init("ULazyObjectProperty"); }

	std::string JsonName() override
	{
		return "ULazyObjectProperty";
	}
	void InitData() override
	{

	}
};

class JsonUAssetObjectProperty : public JsonUObjectPropertyBase
{
public:
	JsonUAssetObjectProperty() { Init("UAssetObjectProperty"); }

	std::string JsonName() override
	{
		return "UAssetObjectProperty";
	}
	void InitData() override
	{

	}
};

class JsonUAssetClassProperty : public JsonUAssetObjectProperty
{
public:
	JsonUAssetClassProperty() { Init("UAssetClassProperty"); }
	JsonUClass* MetaClass = nullptr;

	std::string JsonName() override
	{
		return "UAssetClassProperty";
	}
	void InitData() override
	{
		MetaClass = GetVar("MetaClass").ReadAsPStruct<JsonUClass>();
	}
};

class JsonUNameProperty : public JsonUProperty
{
public:
	JsonUNameProperty() { Init("UNameProperty"); }

	std::string JsonName() override
	{
		return "UNameProperty";
	}
	void InitData() override
	{

	}
};

class JsonUStructProperty : public JsonUProperty
{
public:
	JsonUStructProperty() { Init("UStructProperty"); }
	JsonUScriptStruct* Struct = nullptr;

	std::string JsonName() override
	{
		return "UStructProperty";
	}
	void InitData() override
	{
		Struct = GetVar("Struct").ReadAsPStruct<JsonUScriptStruct>();
	}
};

class JsonUStrProperty : public JsonUProperty
{
public:
	JsonUStrProperty() { Init("UStrProperty"); }

	std::string JsonName() override
	{
		return "UStrProperty";
	}
	void InitData() override
	{

	}
};

class JsonUTextProperty : public JsonUProperty
{
public:
	JsonUTextProperty() { Init("UTextProperty"); }

	std::string JsonName() override
	{
		return "UTextProperty";
	}
	void InitData() override
	{

	}
};

class JsonUArrayProperty : public JsonUProperty
{
public:
	JsonUArrayProperty() { Init("UArrayProperty"); }
	JsonUProperty* Inner = nullptr;

	std::string JsonName() override
	{
		return "UArrayProperty";
	}
	void InitData() override
	{
		Inner = GetVar("Inner").ReadAsPStruct<JsonUProperty>();
	}
};

class JsonUMapProperty : public JsonUProperty
{
public:
	JsonUMapProperty() { Init("UMapProperty"); }
	JsonUProperty* KeyProp = nullptr;
	JsonUProperty* ValueProp = nullptr;

	std::string JsonName() override
	{
		return "UMapProperty";
	}
	void InitData() override
	{
		KeyProp = GetVar("KeyProp").ReadAsPStruct<JsonUProperty>();
		ValueProp = GetVar("ValueProp").ReadAsPStruct<JsonUProperty>();
	}
};

class JsonUDelegateProperty : public JsonUProperty
{
public:
	JsonUDelegateProperty() { Init("UDelegateProperty"); }
	JsonUFunction* SignatureFunction = nullptr;

	std::string JsonName() override
	{
		return "UDelegateProperty";
	}
	void InitData() override
	{
		SignatureFunction = GetVar("SignatureFunction").ReadAsPStruct<JsonUFunction>();
	}
};

class JsonUMulticastDelegateProperty : public JsonUProperty
{
public:
	JsonUMulticastDelegateProperty() { Init("UMulticastDelegateProperty"); }
	JsonUFunction* SignatureFunction = nullptr;

	std::string JsonName() override
	{
		return "UMulticastDelegateProperty";
	}
	void InitData() override
	{
		SignatureFunction = GetVar("SignatureFunction").ReadAsPStruct<JsonUFunction>();
	}
};

class JsonUEnumProperty : public JsonUProperty
{
public:
	JsonUEnumProperty() { Init("UEnumProperty"); }
	JsonUNumericProperty* UnderlyingProp = nullptr;
	JsonUEnum* Enum = nullptr;

	std::string JsonName() override
	{
		return "UEnumProperty";
	}
	void InitData() override
	{
		UnderlyingProp = GetVar("UnderlyingProp").ReadAsPStruct<JsonUNumericProperty>();
		Enum = GetVar("Enum").ReadAsPStruct<JsonUEnum>();
	}
};