#pragma once
#include <string>
#include <vector>
#include <tuple>
#include "json.hpp"

typedef std::tuple<std::string, void*> JsonVar;
typedef std::map<std::string, JsonVar> JsonVariables;

struct JsonStruct
{
	std::string Name;
	JsonVariables Vars;
	int VarCount, StructSize;
};

class Utils
{
public:
	static std::vector<std::string> SplitString(const std::string& str, const std::string& delimiter);
	static std::string ReplaceString(std::string str, const std::string& to_find, const std::string& to_replace);
	static bool ProgramIs64();
	static bool ReadJsonFile(const std::string& fileName, void* jsonObj);
	static JsonStruct ReadJsonStruct(void* jsonObj, const std::string& structName);
	static int VarSizeFromJson(void* jsonObj, const std::string& typeName);
	static bool IsNumber(const std::string& s);
};

