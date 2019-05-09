#include "pch.h"
#include "JsonReflector.h"
#include <cassert>
#include <fstream>

JsonStructs JsonStruct::StructsList;
nlohmann::json* JsonStruct::JsonObj;


bool JsonReflector::ReadJsonFile(const std::string& fileName, void* jsonObj)
{
	// read a JSON file
	std::ifstream i(fileName.c_str());
	i >> *reinterpret_cast<nlohmann::json*>(jsonObj);
	return true;
}

// ##############################################

JsonStruct::JsonStruct() = default;

JsonVar& JsonStruct::operator[](const std::string& varName)
{
	return GetVar(varName);
}

void JsonStruct::SetAllocAddress(void* newAddress)
{
	// Free old memory
	if (AllocPointer != nullptr && !AllocAddressChanged)
		free(AllocPointer);

	AllocPointer = newAddress;

	// ReWrite StructAllocPointer for inside vars
	if (!Read(Name, *this, false))
		throw std::exception(("Can't read `"+ Name + "`.").c_str());

	AllocAddressChanged = true;
}

void JsonStruct::SetAllocAddress(const uintptr_t newAddress)
{
	SetAllocAddress(reinterpret_cast<void*>(newAddress));
}

void JsonStruct::MoveAllocNext()
{
	uintptr_t nextAddress = reinterpret_cast<uintptr_t>(AllocPointer) + StructSize;
	SetAllocAddress(nextAddress);
}

void JsonStruct::MoveAllocPrev()
{
	uintptr_t prevAddress = reinterpret_cast<uintptr_t>(AllocPointer) - StructSize;
	SetAllocAddress(prevAddress);
}

JsonVar& JsonStruct::GetVar(const std::string& varName)
{
	if (Vars.find(varName) != Vars.end())
		return Vars.find(varName)->second;

	throw std::exception(("Not found " + varName + " in JsonVariables").c_str());
}

bool JsonStruct::Read(const std::string& structName, JsonStruct& destStruct, const bool alloc)
{
	if (Load(structName))
	{
		auto s = StructsList.find(structName);
		if (s != StructsList.end())
		{
			void* oldAddress = destStruct.AllocPointer;
			destStruct = s->second;
			destStruct.AllocPointer = oldAddress;

			// Init Struct memory
			if (destStruct.StructSize != 0)
			{
				if (alloc)
				{
					void* structPointer = malloc(destStruct.StructSize);
					if (structPointer == nullptr) return false;
					ZeroMemory(structPointer, destStruct.StructSize);
					destStruct.AllocPointer = structPointer;
				}
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
	return true;
}

bool JsonStruct::Load(const std::string& structName)
{
	// Check is already here
	if (StructsList.find(structName) != StructsList.end()) return true;

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

int JsonStruct::VarSizeFromJson(const std::string& typeName)
{
	if (typeName == "int")
		return sizeof(int);
	if (typeName == "pointer")
		return sizeof(uintptr_t);
	if (typeName == "DWORD")
		return sizeof(DWORD);
	if (typeName == "DWORD64")
		return sizeof(DWORD64);
	if (typeName == "string")
		return sizeof(uintptr_t);

	// Other type (usually) structs
	if (IsNumber(typeName))
		return std::stoi(typeName);

	if (IsStructType(typeName))
	{
		if (Load(typeName))
		{
			auto s = StructsList.find(typeName);
			if (s != StructsList.end())
				return s->second.StructSize;
		}
		throw std::exception(("Cant find struct `" + typeName + "`.").c_str());
	}

	throw std::exception(("Cant detect size of `" + typeName + "`.").c_str());
}

bool JsonStruct::IsStructType(const std::string& typeName)
{
	const bool isStruct = 
		typeName == "int" ||
		typeName == "pointer" ||
		typeName == "DWORD" ||
		typeName == "DWORD64" ||
		typeName == "string"
	;
	return !isStruct;
}

bool JsonStruct::IsNumber(const std::string& s)
{
	return !s.empty() && std::find_if(s.begin(),
		s.end(), [](const char c) { return !std::isdigit(c); }) == s.end();
}


// ##############################################

JsonVar::JsonVar() : parent(nullptr), structVars(nullptr), Size(0), Offset(0), IsStruct(false)
{
	// Solve unresolved problem
	ReadAs<int>();
	ReadAs<DWORD>();
	ReadAs<DWORD64>();
	ReadAs<uintptr_t>();
	ReadAs<char*>();
}

JsonVar::JsonVar(const std::string& name, const std::string& type, const size_t offset, const bool isStruct) : parent(nullptr), structVars(nullptr)
{
	Name = name;
	Type = type;
	Size = JsonStruct::VarSizeFromJson(Type);
	Offset = offset;
	IsStruct = isStruct;
}

JsonVar::~JsonVar()
{
	if (IsStruct && structVars != nullptr)
		delete structVars;
}

JsonVar& JsonVar::operator[](const std::string& varName)
{
	if (!IsStruct)
		throw std::exception((Name + " not a struct.").c_str());

	return GetVar(varName);
}

JsonVar& JsonVar::GetVar(const std::string& varName)
{
	if (!IsStruct)
		throw std::exception((Name + " not a struct.").c_str());

	if (structVars == nullptr)
		auto readonly = ReadAsStruct();

	if (structVars != nullptr)
	{
		if (structVars->Vars.find(varName) != structVars->Vars.end())
			return structVars->Vars.find(varName)->second;
	}

	throw std::exception(("Not found " + varName + " in JsonVariables").c_str());
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

	uintptr_t calcAddress = reinterpret_cast<uintptr_t>(parent->AllocPointer) + Offset;
	return *reinterpret_cast<T*>(calcAddress);
}

JsonStruct JsonVar::ReadAsStruct()
{
	if (!IsStruct)
		throw std::exception((Name + " Not a struct.").c_str());

	if (structVars != nullptr)
		return *structVars;

	auto sStructIt = JsonStruct::StructsList.find(Type);
	if (sStructIt == JsonStruct::StructsList.end())
		throw std::exception(("Can't find this struct, When try read as " + Name).c_str());

	uintptr_t calcAddress = reinterpret_cast<uintptr_t>(parent->AllocPointer) + Offset;

	structVars = new JsonStruct();
	*structVars = sStructIt->second;
	structVars->AllocPointer = reinterpret_cast<void*>(calcAddress);

	if (!JsonStruct::Read(Type, *structVars, false))
		throw std::exception(("Can't read the struct `" + Name + "`.").c_str());

	return *structVars;
}