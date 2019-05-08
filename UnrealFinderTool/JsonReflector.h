#pragma once
#include "json.hpp"

class JsonStruct;

class JsonVar
{
	JsonStruct* parent;
	JsonStruct structVars;

public:
	std::string Name, Type;
	size_t Size;
	size_t Offset;
	bool IsStruct;

	JsonVar();
	JsonVar(const std::string& name, const std::string& type, size_t offset, bool isStruct);
	void SetParent(JsonStruct* parentStruct);
	template <class T> T ReadAs();
	JsonStruct& ReadAsStruct();
};

class JsonStruct
{
	using JsonVariables = std::map<std::string, JsonVar>;
	using JsonStructs = std::map<std::string, JsonStruct>;
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