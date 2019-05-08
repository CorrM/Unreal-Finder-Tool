#include "pch.h"
#include "JsonVar.h"
#include "JsonStruct.h"

JsonStructs JsonStruct::StructsList;
JsonStruct::JsonStruct() = default;

JsonStruct::~JsonStruct()
{
	// Delete pointers in the struct
	for (auto& element : Vars)
		delete element.second;

	// Delete struct them self
	free(StructPointer);
}

JsonVar& JsonStruct::operator[](const std::string& varName)
{
	return GetVar(varName);
}

JsonVar& JsonStruct::GetVar(const std::string& varName)
{
	return GetVar(Vars, varName);
}

bool JsonStruct::Read(void* jsonObj, const std::string& structName, JsonStruct& destStruct)
{
	// Check if there is struct with the same name and return it
	auto sStruct = StructsList.find(structName);
	if (sStruct != StructsList.end())
	{
		// setup new copy of the struct
		void* structPointer = malloc(sStruct->second.StructSize);
		ZeroMemory(structPointer, sStruct->second.StructSize);

		// Setup Memory stuff
		destStruct = sStruct->second;
		destStruct.StructPointer = structPointer;
		return true;
	}

	auto j = *reinterpret_cast<nlohmann::json*>(jsonObj);

	for (const auto& parent : j.at("structs").items())
	{
		const std::string eName = parent.value().at("name");
		if (eName == structName)
		{
			JsonVariables vars;
			int structSize = 0;
			size_t offset = 0;

			// Init vars
			auto element = parent.value().at("vars");
			for (const auto& var : element.items())
			{
				nlohmann::json::iterator it = var.value().begin();
				
				if (it.value().is_number_unsigned())
					structSize += static_cast<int>(var.value());
				else
					structSize += VarSizeFromJson(destStruct, it.value());

				const std::string& name = it.key();
				std::string type = it.value();

				JsonVar* jVar = new JsonVar(&destStruct, name, type, offset, IsStructType(type));
				vars[name] = jVar;
				offset += jVar->Size;
			}

			// Init Struct memory
			void* structPointer = malloc(structSize);
			ZeroMemory(structPointer, structSize);

			// Init struct
			destStruct.JsonObj = reinterpret_cast<nlohmann::json*>(jsonObj);
			destStruct.Name = eName;
			destStruct.Vars = vars;
			destStruct.VarCount = vars.size();
			destStruct.StructSize = structSize;

			// [Copy] the struct to structs list
			StructsList[eName] = destStruct;

			// Setup Memory stuff
			destStruct.StructPointer = structPointer;
			
			return true;
		}
	}
	return false;
}

int JsonStruct::VarSizeFromJson(JsonStruct& jsonStruct, const std::string& typeName)
{
	if (typeName == "int")
		return sizeof(int);
	if (typeName == "pointer")
		return sizeof(uintptr_t);

	// Other type (usually) structs
	if (!Read(jsonStruct.JsonObj, typeName, jsonStruct))
		throw std::exception(("Cant find struct `" + typeName + "`.").c_str());

	return jsonStruct.StructSize;
}

bool JsonStruct::IsStructType(const std::string& typeName)
{
	return !(typeName == "int" || typeName == "pointer");
}

JsonVar& JsonStruct::GetVar(const JsonVariables& vars, const std::string & varName)
{
	if (vars.find(varName) != vars.end())
		return *vars.find(varName)->second;

	throw std::exception(("Not found " + varName + " in JsonVariables").c_str());
}