#pragma once
#include <string>
#include <vector>

class Utils
{
public:
	static std::vector<std::string> SplitString(const std::string& str, const std::string& delimiter);
	static std::string ReplaceString(std::string str, const std::string& to_find, const std::string& to_replace);
};

