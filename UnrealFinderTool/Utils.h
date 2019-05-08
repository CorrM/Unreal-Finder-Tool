#pragma once
#include <string>
#include <vector>
#include "json.hpp"


class Utils
{
public:
	static std::vector<std::string> SplitString(const std::string& str, const std::string& delimiter);
	static std::string ReplaceString(std::string str, const std::string& to_find, const std::string& to_replace);
	static bool ProgramIs64();
	static bool ReadJsonFile(const std::string& fileName, void* jsonObj);
	static bool IsNumber(const std::string& s);
};