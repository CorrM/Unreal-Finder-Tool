#include "pch.h"
#include "JsonTypes.h"
#include <fstream>
#include <Memory.h>
#include "Utils.h"

JsonStructs JsonReflector::StructsList;
nlohmann::json JsonReflector::jsonObj;
Memory* JsonReflector::MemoryObj;

// -------------------------------------------------
// JsonReflector
// -------------------------------------------------
bool JsonReflector::ReadJsonFile(const std::string& fileName, void* jsonObj)
{
	// read a JSON file
	std::ifstream i(fileName.c_str());
	i >> *reinterpret_cast<nlohmann::json*>(jsonObj);
	return true;
}

bool JsonReflector::ReadJsonFile(const std::string& fileName)
{
	return ReadJsonFile(fileName, &jsonObj);
}

bool JsonReflector::ReadStruct(const std::string& structName, JsonStruct& destStruct, const bool alloc)
{
	auto s = StructsList.find(structName);
	if (s != StructsList.end())
	{
		// Old info to keep
		void* oldAddress = destStruct.GetAllocPointer();

		// Get Copy From StructsList and Fill Struct
		destStruct = s->second;
		destStruct.SetAllocPointer(oldAddress);

		// Init Struct memory
		if (alloc)
		{
			void* structPointer = malloc(destStruct.StructSize);
			if (structPointer == nullptr) return false;
			ZeroMemory(structPointer, destStruct.StructSize);
			destStruct.SetAllocPointer(structPointer);
		}
		destStruct.Vars = JsonVariables(s->second.Vars.begin(), s->second.Vars.end());
		for (auto& kv : destStruct.Vars)
			kv.second.SetParent(&destStruct);

		return true;
	}
	return false;
}

bool JsonReflector::LoadStruct(const std::string& structName)
{
	// Check is already here
	if (StructsList.find(structName) != StructsList.end()) return true;
	auto j = jsonObj;

	for (const auto& parent : j.at("structs").items())
	{
		const std::string eName = parent.value().at("name");
		if (eName == structName)
		{
			JsonStruct tempToSave;
			JsonVariables vars;
			int structSize = 0;
			size_t offset = 0;

			// Get Super
			const std::string eSuper = parent.value().at("super");
			if (!eSuper.empty())
			{
				tempToSave.StructSuper = eSuper;

				// Add Variables to struct first
				if (LoadStruct(eSuper))
				{
					auto s = StructsList.find(eSuper);
					if (s != StructsList.end())
					{
						for (const auto& var : s->second.Vars)
						{
							JsonVar variable = var.second;

							const std::string& name = variable.Name;
							std::string type = variable.Type;

							auto jVar = JsonVar(name, type, offset, IsStructType(type));
							vars[name] = jVar;
							offset += jVar.Size;
						}

						structSize += s->second.StructSize;
					}
				}
				else
				{
					throw std::exception(("Can't find `" + eSuper + "` Struct.").c_str());
				}
			}

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
			tempToSave.StructName = eName;
			tempToSave.Vars = vars;
			tempToSave.StructSize = structSize;

			// [Copy] the struct to structs list
			if (StructsList.find(eName) == StructsList.end())
				StructsList[eName] = tempToSave;

			return true;
		}
	}

	return false;
}

bool JsonReflector::Load(void* jsonObj)
{
	auto jObj = reinterpret_cast<nlohmann::json*>(jsonObj);
	auto j = *jObj;

	for (const auto& parent : j.at("structs").items())
	{
		const std::string eName = parent.value().at("name");
		// Check is already here
		if (StructsList.find(eName) != StructsList.end()) continue;

		JsonStruct tempToSave;
		JsonVariables vars;
		int structSize = 0;
		size_t offset = 0;

		// Get Super
		const std::string eSuper = parent.value().at("super");
		if (!eSuper.empty())
		{
			tempToSave.StructSuper = eSuper;

			// Add Variables to struct first
			if (LoadStruct(eSuper))
			{
				auto s = StructsList.find(eSuper);
				if (s != StructsList.end())
				{
					for (const auto& var : s->second.Vars)
					{
						JsonVar variable = var.second;

						const std::string& name = variable.Name;
						std::string type = variable.Type;

						auto jVar = JsonVar(name, type, offset, IsStructType(type));
						vars[name] = jVar;
						offset += jVar.Size;
					}

					structSize += s->second.StructSize;
				}
			}
			else
			{
				throw std::exception(("Can't find `" + eSuper + "` Struct.").c_str());
			}
		}

		// Init vars
		auto element = parent.value().at("vars");
		for (const auto& var : element.items())
		{
			nlohmann::json::iterator it = var.value().begin();

			if (it.value().is_number_unsigned())
				structSize += static_cast<int>(var.value());
			else
				structSize += VarSizeFromJson(it.value()); // VarSizeFromJson Load other struct if not loaded

			const std::string& name = it.key();
			std::string type = it.value();

			auto jVar = JsonVar(name, type, offset, IsStructType(type));
			vars[name] = jVar;
			offset += jVar.Size;
		}

		// Init struct
		tempToSave.StructName = eName;
		tempToSave.Vars = vars;
		tempToSave.StructSize = structSize;

		// [Copy] the struct to structs list
		if (StructsList.find(eName) == StructsList.end())
			StructsList[eName] = tempToSave;
	}
	return true;
}

bool JsonReflector::Load()
{
	return Load(&jsonObj);
}

bool JsonReflector::ReadAndLoadFile(const std::string& fileName, void* jsonObj)
{
	return ReadJsonFile(fileName, jsonObj) && Load(jsonObj);
}

bool JsonReflector::ReadAndLoadFile(const std::string& fileName)
{
	return ReadAndLoadFile(fileName, reinterpret_cast<void*>(&jsonObj));
}

int JsonReflector::VarSizeFromJson(const std::string & typeName)
{
	if (typeName == "int8")
		return sizeof(int8_t);
	if (typeName == "int16")
		return sizeof(int16_t);
	if (typeName == "int" || typeName == "int32")
		return sizeof(int32_t);
	if (typeName == "int64")
		return sizeof(int64_t);

	if (typeName == "uint8")
		return sizeof(uint8_t);
	if (typeName == "uint16")
		return sizeof(uint16_t);
	if (typeName == "uint" || typeName == "uint32")
		return sizeof(uint32_t);
	if (typeName == "uint64")
		return sizeof(uint64_t);

	if (typeName == "pointer")
		return sizeof(uintptr_t);
	if (typeName == "DWORD")
		return sizeof(DWORD);
	if (typeName == "DWORD64")
		return sizeof(DWORD64);
	if (typeName == "string")
		return sizeof(uintptr_t);

	// Other type (usually) structs
	if (Utils::IsNumber(typeName))
		return std::stoi(typeName);

	if (IsStructType(typeName))
	{
		if (LoadStruct(typeName))
		{
			auto s = StructsList.find(typeName);
			if (s != StructsList.end())
				return s->second.StructSize;
		}
		throw std::exception(("Cant find struct `" + typeName + "`.").c_str());
	}
	throw std::exception(("Cant detect size of `" + typeName + "`.").c_str());
}

bool JsonReflector::IsStructType(const std::string & typeName)
{
	const bool isStruct =
		typeName == "int8"		||
		typeName == "int16"		||
		typeName == "int"		||
		typeName == "int32"		||
		typeName == "int64"		||

		typeName == "uint8"		||
		typeName == "uint16"	||
		typeName == "uint"		||
		typeName == "uint32"	||
		typeName == "uint64"	||

		typeName == "pointer"	||
		typeName == "DWORD"		||
		typeName == "DWORD64"	||
		typeName == "string";
	return !isStruct;
}

// -------------------------------------------------
// JsonStruct
// -------------------------------------------------
JsonStruct::JsonStruct() = default;

JsonStruct::~JsonStruct()
{
	if (_init && !AllocChanged && IsPointerStruct)
		free(allocPointer);
}

void JsonStruct::SetAllocPointer(void* newAddress)
{
	allocPointer = newAddress;
}

void* JsonStruct::GetAllocPointer()
{
	return allocPointer;
}

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

void JsonStruct::Init(const std::string& structName, const bool alloc)
{
	if (_init || structName.empty()) return;

	if (!JsonReflector::ReadStruct(structName, *this, alloc))
		throw std::exception(("Can't find struct `" + structName + "`.").c_str());
	_init = true;
}

// -------------------------------------------------
// JsonVar
// -------------------------------------------------
JsonVar::JsonVar() : parent(nullptr), Size(0), Offset(0), IsStruct(false), StructVars(nullptr)
{
	// Solve unresolved problem
	ReadAs<char>();
	ReadAs<bool>();
	ReadAs<short>();
	ReadAs<int>();

	ReadAs<__int8>();
	ReadAs<__int16>();
	ReadAs<__int32>();
	ReadAs<__int64>();

	ReadAs<int8_t>();
	ReadAs<int16_t>();
	ReadAs<int32_t>();
	ReadAs<int64_t>();

	ReadAs<uint8_t>();
	ReadAs<uint16_t>();
	ReadAs<uint32_t>();
	ReadAs<uint64_t>();

	ReadAs<DWORD>();
	ReadAs<DWORD64>();
	ReadAs<uintptr_t>();
}

JsonVar::JsonVar(const std::string& name, const std::string& type, const size_t offset, const bool isStruct) : parent(nullptr), StructVars(nullptr)
{
	Name = name;
	Type = type;
	Size = JsonReflector::VarSizeFromJson(Type);
	Offset = offset;
	IsStruct = isStruct;
}

JsonVar::~JsonVar()
{
	if (IsStruct && StructVars != nullptr)
		delete StructVars;
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

	if (StructVars == nullptr)
		auto readonly = ReadAsStruct();

	if (StructVars != nullptr)
	{
		if (StructVars->Vars.find(varName) != StructVars->Vars.end())
			return StructVars->Vars.find(varName)->second;
	}

	throw std::exception(("Not found " + varName + " in JsonVariables").c_str());
}

void JsonVar::SetParent(JsonStruct* parentStruct)
{
	parent = parentStruct;
}

template <typename T> T JsonVar::ReadAs()
{
	if (parent == nullptr)
		return 0;

	uintptr_t calcAddress = reinterpret_cast<uintptr_t>(parent->GetAllocPointer()) + Offset;
	return *reinterpret_cast<T*>(calcAddress);
}

JsonStruct* JsonVar::ReadAsStruct()
{
	if (!IsStruct)
		throw std::exception((Name + " Not a struct.").c_str());

	if (StructVars != nullptr)
		return StructVars;

	auto sStructIt = JsonReflector::StructsList.find(Type);
	if (sStructIt == JsonReflector::StructsList.end())
		throw std::exception(("Can't find struct When try read as " + Type).c_str());

	uintptr_t calcAddress = reinterpret_cast<uintptr_t>(parent->GetAllocPointer()) + Offset;

	StructVars = new JsonStruct(sStructIt->second);
	StructVars->AllocChanged = true;
	StructVars->Init(Type, false);
	StructVars->SetAllocPointer(reinterpret_cast<void*>(calcAddress));
	StructVars->IsPointerStruct = false;

	return StructVars;
}

JsonStruct* JsonVar::ReadAsPStruct(const string& ptrType)
{
	if (Type != "pointer")
		throw std::exception((Type + " Not a pointer.!").c_str());

	if (StructVars != nullptr)
		return StructVars;

	auto sStructIt = JsonReflector::StructsList.find(ptrType);
	if (sStructIt == JsonReflector::StructsList.end())
		throw std::exception(("Can't find struct When try read as " + ptrType).c_str());

	uintptr_t calcAddress = reinterpret_cast<uintptr_t>(parent->GetAllocPointer()) + Offset;
	calcAddress = *reinterpret_cast<uintptr_t*>(calcAddress);

	if (calcAddress == NULL)
		return nullptr;

	StructVars = new JsonStruct(sStructIt->second);
	StructVars->Init(ptrType);
	StructVars->IsPointerStruct = true;

	if (JsonReflector::MemoryObj->ReadBytes(calcAddress, StructVars->GetAllocPointer(), StructVars->StructSize) == 0)
		throw std::exception(("Can't read game memory `" + std::to_string(calcAddress) + "`.").c_str());

	Utils::FixPointersInStruct(StructVars, JsonReflector::MemoryObj->Is64Bit);

	return StructVars;
}