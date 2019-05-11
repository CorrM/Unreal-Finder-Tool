// This header just to help fast coding
#pragma once
#include "JsonReflector.h"

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
	auto Outer() { return GetVar("Outer").ReadAs<uintptr_t>(); }
};


// class Json : public JsonStruct
// {
// public:
// 	auto VfTable() { return GetVar("VfTable").ReadAs<int>(); }
// };