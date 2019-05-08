#include "pch.h"
#include "JsonVar.h"

JsonVar::JsonVar() : parent(nullptr), Size(0), Offset(0), IsStruct(false)
{
	// Solve unresolved problem
	ReadAs<int>();
	ReadAs<uintptr_t>();
	ReadAs<char*>();
}

JsonVar::JsonVar(JsonStruct* parentStruct, const std::string name, const std::string type, const size_t offset, const bool isStruct)
{
	parent = parentStruct;
	Name = name;
	Type = type;
	Size = JsonStruct::VarSizeFromJson(*parent, Type);
	Offset = offset;
	IsStruct = isStruct;
}

template <class T>
T JsonVar::ReadAs()
{
	uintptr_t calcAddress = reinterpret_cast<uintptr_t>(parent->StructPointer) + Offset;
	return *reinterpret_cast<T*>(calcAddress);
}

JsonStruct& JsonVar::ReadAsStruct()
{
	if (!IsStruct)
		throw std::exception((Name + " Not a struct.").c_str());

	if (varStruct.StructPointer != nullptr)
		return varStruct;

	auto sStructIt = JsonStruct::StructsList.find(Name);
	if (sStructIt == JsonStruct::StructsList.end())
		throw std::exception(("Can't find this struct, When try read as " + Name).c_str());

	uintptr_t calcAddress = reinterpret_cast<uintptr_t>(parent->StructPointer) + Offset;
	auto sStruct = sStructIt->second;
	sStruct.StructPointer = reinterpret_cast<void*>(calcAddress);

	if (JsonStruct::Read(static_cast<void*>(parent->JsonObj), Name, sStruct))
		throw std::exception(("Can't read the struct `" + Name + "`.").c_str());

	varStruct = sStruct;
	return varStruct;
}