#ifndef JSON_STRUCT
#define JSON_STRUCT
#pragma once

#include "json.hpp"
#include "JsonVar.h"

class JsonVar;
class JsonStruct;

using JsonVariables = std::map<std::string, JsonVar>;
using JsonStructs = std::map<std::string, JsonStruct>;

class JsonStruct
{
public:
	static JsonStructs StructsList;
	static nlohmann::json* JsonObj;
	std::string Name;
	JsonVariables Vars;
	size_t VarCount = 0;
	// Pointer to alloc memory based on `StructSize`
	void* StructPointer = nullptr;
	int StructSize = 0;

	JsonStruct();
	JsonVar& operator[](const std::string& varName);
	JsonVar& GetVar(const std::string& varName);
	static bool Read(const std::string& structName, JsonStruct& destStruct);
	static bool Load(void* jsonObj);
	static int VarSizeFromJson(const std::string& typeName);
	static bool IsStructType(const std::string& typeName);
private:
	static bool Load(const std::string& structName);
};
#endif