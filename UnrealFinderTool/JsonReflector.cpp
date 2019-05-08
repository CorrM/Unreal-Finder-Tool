#include "pch.h"
#include "JsonReflector.h"
#include <cassert>

JsonStructs JsonStruct::StructsList;
nlohmann::json* JsonStruct::JsonObj;

JsonStruct::JsonStruct() = default;

JsonVar& JsonStruct::operator[](const std::string& varName)
{
	return GetVar(varName);
}

JsonVar& JsonStruct::GetVar(const std::string& varName)
{
	if (Vars.find(varName) != Vars.end())
		return Vars.find(varName)->second;

	throw std::exception(("Not found " + varName + " in JsonVariables").c_str());
}

bool JsonStruct::Read(const std::string & structName, JsonStruct & destStruct)
{
	if (Load(structName))
	{
		auto s = StructsList.find(structName);
		if (s != StructsList.end())
		{
			destStruct = s->second;
			// Init Struct memory
			if (destStruct.StructSize != 0)
			{
				void* structPointer = malloc(destStruct.StructSize);
				if (structPointer == nullptr) return false;
				ZeroMemory(structPointer, destStruct.StructSize);
				destStruct.StructPointer = structPointer;
				destStruct.Vars = JsonVariables(s->second.Vars.begin(), s->second.Vars.end());

				for (auto& kv : destStruct.Vars)
					kv.second.SetParent(&destStruct);
				return true;
			}
		}
	}
	return false;
}

bool JsonStruct::Load(void* jsonObj)
{
	JsonObj = reinterpret_cast<nlohmann::json*>(jsonObj);
	auto j = *JsonObj;
	StructsList.clear();

	for (const auto& parent : j.at("structs").items())
	{
		const std::string eName = parent.value().at("name");
		// Check is already here
		if (StructsList.find(eName) != StructsList.end()) continue;

		JsonStruct tempToSave;
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
				structSize += VarSizeFromJson(it.value());

			const std::string& name = it.key();
			std::string type = it.value();

			auto jVar = JsonVar(name, type, offset, IsStructType(type));
			vars[name] = jVar;
			offset += jVar.Size;
		}

		// Init struct
		tempToSave.Name = eName;
		tempToSave.Vars = vars;
		tempToSave.VarCount = vars.size();
		tempToSave.StructSize = structSize;

		// [Copy] the struct to structs list
		StructsList[eName] = tempToSave;
	}
	return false;
}

bool JsonStruct::Load(const std::string & structName)
{
	assert(JsonObj != nullptr);

	auto j = *JsonObj;
	for (const auto& parent : j.at("structs").items())
	{
		const std::string eName = parent.value().at("name");
		if (eName == structName)
		{
			JsonStruct tempToSave;
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
					structSize += VarSizeFromJson(it.value());

				const std::string& name = it.key();
				std::string type = it.value();

				JsonVar jVar(name, type, offset, IsStructType(type));
				vars[name] = jVar;
				offset += jVar.Size;
			}

			// Init struct
			tempToSave.Name = eName;
			tempToSave.Vars = vars;
			tempToSave.VarCount = vars.size();
			tempToSave.StructSize = structSize;

			// [Copy] the struct to structs list
			StructsList[eName] = tempToSave;

			return true;
		}
	}

	return false;
}

int JsonStruct::VarSizeFromJson(const std::string & typeName)
{
	if (typeName == "int")
		return sizeof(int);
	if (typeName == "pointer")
		return sizeof(uintptr_t);

	// Other type (usually) structs

	if (Load(typeName))
	{
		auto s = StructsList.find(typeName);
		if (s != StructsList.end())
			return s->second.StructSize;
	}

	throw std::exception(("Cant find struct `" + typeName + "`.").c_str());
}

bool JsonStruct::IsStructType(const std::string & typeName)
{
	return !(typeName == "int" || typeName == "pointer");
}



JsonVar::JsonVar() : parent(nullptr), Size(0), Offset(0), IsStruct(false)
{
	// Solve unresolved problem
	ReadAs<int>();
	ReadAs<uintptr_t>();
	ReadAs<char*>();
}

JsonVar::JsonVar(const std::string& name, const std::string& type, const size_t offset, const bool isStruct) : parent(nullptr)
{
	Name = name;
	Type = type;
	Size = JsonStruct::VarSizeFromJson(Type);
	Offset = offset;
	IsStruct = isStruct;

	if (IsStruct)
		JsonStruct::Read(Type, structVars);
}

void JsonVar::SetParent(JsonStruct* parentStruct)
{
	parent = parentStruct;
}

template <class T>
T JsonVar::ReadAs()
{
	if (parent == nullptr)
		return 0;

	uintptr_t calcAddress = reinterpret_cast<uintptr_t>(parent->StructPointer) + Offset;
	return *reinterpret_cast<T*>(calcAddress);
}

JsonStruct& JsonVar::ReadAsStruct()
{
	if (!IsStruct)
		throw std::exception((Name + " Not a struct.").c_str());

	if (structVars.StructPointer != nullptr)
		return structVars;

	auto sStructIt = JsonStruct::StructsList.find(Name);
	if (sStructIt == JsonStruct::StructsList.end())
		throw std::exception(("Can't find this struct, When try read as " + Name).c_str());

	uintptr_t calcAddress = reinterpret_cast<uintptr_t>(parent->StructPointer) + Offset;
	auto sStruct = sStructIt->second;
	sStruct.StructPointer = reinterpret_cast<void*>(calcAddress);

	if (JsonStruct::Read(Name, sStruct))
		throw std::exception(("Can't read the struct `" + Name + "`.").c_str());

	structVars = sStruct;
	return structVars;
}