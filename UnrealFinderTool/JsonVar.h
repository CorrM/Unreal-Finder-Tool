#ifndef JSON_VAR
#define JSON_VAR
#pragma once
#include "JsonStruct.h"

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
#endif