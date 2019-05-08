#pragma once
#include "JsonStruct.h"

class JsonVar
{
	JsonStruct* parent;
	JsonStruct varStruct;

public:
	std::string Name, Type;
	size_t Size;
	size_t Offset;
	bool IsStruct;

	JsonVar();
	JsonVar(JsonStruct* parentStruct, std::string name, std::string type, size_t offset, bool isStruct);
	template <class T> T ReadAs();
	JsonStruct& ReadAsStruct();
};
