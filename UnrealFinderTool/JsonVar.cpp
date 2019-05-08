#include "pch.h"
#include "JsonVar.h"

JsonVar::JsonVar() : parent(nullptr), Size(0), Offset(0), IsStruct(false)
{
	// Solve unresolved problem
	ReadAs<int>();
	ReadAs<uintptr_t>();
	ReadAs<char*>();
}

JsonVar::JsonVar(const std::string& name, const std::string& type, const size_t offset, const bool isStruct): parent(nullptr)
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