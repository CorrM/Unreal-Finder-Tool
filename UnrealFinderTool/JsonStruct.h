#pragma once
#include <map>
#include "json.hpp"

class JsonVar;

class JsonStruct
{
	using JsonVariables = std::map<std::string, JsonVar*>;
	using JsonStructs = std::map<std::string, JsonStruct>;

public:
	static JsonStructs StructsList;
	nlohmann::json* JsonObj;
	std::string Name;
	JsonVariables Vars;
	int VarCount = 0;
	// Pointer to alloc memory based on `StructSize`
	void* StructPointer = nullptr;
	int StructSize = 0;

	JsonStruct();
	~JsonStruct();
	JsonVar& operator[](const std::string& varName);
	JsonVar& GetVar(const std::string& varName);
	static bool Read(void* jsonObj, const std::string& structName, JsonStruct& destStruct);
	static int VarSizeFromJson(JsonStruct& jsonStruct, const std::string& typeName);
	static bool IsStructType(const std::string& typeName);
	static JsonVar& GetVar(const JsonVariables& vars, const std::string& varName);
};