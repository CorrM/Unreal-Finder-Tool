#include "pch.h"
#include "Utils.h"
#include <vector>
#include <fstream>

std::vector<std::string> Utils::SplitString(const std::string& str, const std::string& delimiter)
{
	std::vector<std::string> strings;

	std::string::size_type pos = 0;
	std::string::size_type prev = 0;
	while ((pos = str.find(delimiter, prev)) != std::string::npos)
	{
		strings.push_back(str.substr(prev, pos - prev));
		prev = pos + 1;
	}

	// To get the last substring (or only, if delimiter is not found)
	strings.push_back(str.substr(prev));

	return strings;
}

std::string Utils::ReplaceString(std::string str, const std::string& to_find, const std::string& to_replace)
{
	size_t position = 0;
	for (position = str.find(to_find); position != std::string::npos; position = str.find(to_find, position))
		str.replace(position, 1, to_replace);
	return str;
}

bool Utils::ProgramIs64()
{
#if _WIN64
	return true;
#else
	return false;
#endif
}

bool Utils::ReadJsonFile(const std::string& fileName, void* jsonObj)
{
	// read a JSON file
	std::ifstream i(fileName.c_str());
	i >> *reinterpret_cast<nlohmann::json*>(jsonObj);
	return true;
}

JsonStruct Utils::ReadJsonStruct(void* jsonObj, const std::string& structName)
{
	auto j = *reinterpret_cast<nlohmann::json*>(jsonObj);
	for (const auto& parent : j.at("structs").items())
	{
		const std::string eName = parent.value().at("name");
		if (eName == structName)
		{
			JsonVariables vars;
			int structSize = 0;

			auto element = parent.value().at("vars");

			// Init vars
			for (const auto& var : element.items())
			{
				if (var.value().is_number_unsigned())
					structSize += int(var.value());
				else
					structSize += VarSizeFromJson(jsonObj, var.value());

				// Add Var
				auto pointer = malloc(structSize);
				//							   Type		    Pointer to local data
				JsonVar jVar = std::make_tuple(var.value(), pointer);
				vars[var.key()] = jVar;
			}

			// Init Struct
			JsonStruct ret;
			ret.Name = eName;
			ret.Vars = vars;
			ret.VarCount = vars.size();
			ret.StructSize = structSize;

			return ret;
		}
	}
	return JsonStruct();
}

int Utils::VarSizeFromJson(void* jsonObj, const std::string& typeName)
{
	nlohmann::json j = *reinterpret_cast<nlohmann::json*>(jsonObj);

	if (typeName == "int")
		return sizeof(int);
	if (typeName == "pointer")
		return sizeof(uintptr_t);
	
	// Other type (usually) structs
	return ReadJsonStruct(jsonObj, typeName).StructSize;
}

bool Utils::IsNumber(const std::string& s)
{
	return !s.empty() && std::find_if(s.begin(),
		s.end(), [](const char c) { return !std::isdigit(c); }) == s.end();
}