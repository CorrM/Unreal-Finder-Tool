#pragma once
#include "json.hpp"
#include "UnsortedMap.h"

class JsonVar;
class JsonStruct;
class Memory;


using JsonStructs = UnsortedMap<std::string, JsonStruct>;
using JsonVariables = UnsortedMap<std::string, JsonVar>;

class JsonReflector
{
public:
	// Main json reader for structs
	static nlohmann::json JsonObj;
	// Contains all loaded structs
	static JsonStructs StructsList;
	// Read Json file into memory to be ready to load structs inside them
	static bool ReadJsonFile(const std::string& fileName, void* jsonObj);
	// Read Json file into memory to be ready to load structs inside them, [Using main `JsonObj`]
	static bool ReadJsonFile(const std::string& fileName);
	// Check variable type is struct
	static bool IsStructType(const std::string& typeName);
	// Read struct form loaded json structs
	static bool ReadStruct(const std::string& structName, JsonStruct& destStruct, bool alloc = true);
	// Load all json structs inside `StructsList`
	static bool Load(void* jsonObj);
	// Load all json structs inside `StructsList`, [Using main `JsonObj`]
	static bool Load();
	// Read Json file into memory AND read all structs inside json file, then store structs inside `StructsList`
	static bool ReadAndLoadFile(const std::string& fileName, void* jsonObj);
	// Read Json file into memory AND read all structs inside json file, then store structs inside `StructsList`, [Using main `JsonObj`]
	static bool ReadAndLoadFile(const std::string& fileName);
	// Load json struct by name
	static bool LoadStruct(const std::string& structName);
	// Get json struct variable size
	static int VarSizeFromJson(const std::string& typeName);
};

class JsonVar
{
	// Parent of this variable
	JsonStruct* parent;
public:
	// Variable Name
	std::string Name;
	// Variable Type
	std::string Type;
	// Variable Size
	size_t Size;
	// Variable offset of his parent
	size_t Offset;
	// Variable is struct
	bool IsStruct;
	// If this variable is struct this is pointer to struct contains variables
	JsonStruct* Struct;

	JsonVar();
	JsonVar(const std::string& name, const std::string& type, size_t offset, bool isStruct);
	~JsonVar();
	// Access variable inside this variable, ONLY work if this variable is struct
	JsonVar& operator[](const std::string& varName);
	// Change parent of this variable, useful for init this variable if is struct
	void SetParent(JsonStruct* parentStruct);
	// Read variable as type [int, DWORD, ..etc]
	template <typename T> T ReadAs();
	// Read variable as struct, ONLY work if this variable is struct [NOT POINTER TO STRUCT]
	JsonStruct* ReadAsStruct();
	// Read variable as [derived] struct, ONLY work if this variable is struct [NOT POINTER TO STRUCT]
	template <typename T> T ReadAsStruct()
	{
		T tmp = *reinterpret_cast<T*>(ReadAsStruct());
		tmp.FixStructData();
		tmp.InitData();
		return tmp;
	}
	// Read variable as struct, ONLY work if this variable is [POINTER TO STRUCT]
	JsonStruct* ReadAsPStruct(const std::string& ptrType);
	// Read variable as [derived] pointer json struct, ONLY work if this variable is [POINTER TO STRUCT]
	template <typename T> T* ReadAsPStruct()
	{
		T obj;
		T* ret = reinterpret_cast<T*>(ReadAsPStruct(obj.JsonName()));
		ret->InitData();
		return ret;
	}
private:
	// Access variable in this variable if this variable is struct
	JsonVar& GetVar(const std::string& varName);
};

class JsonStruct
{
	bool _init = false;
	// local data Pointer based on `StructSize`
	void* allocPointer = nullptr;
public:
	// Struct Name
	std::string StructName = "";
	// Super Name
	std::string StructSuper = "";
	// Variable inside this struct
	JsonVariables Vars;
	// Size of this struct
	size_t StructSize = 0;
	// Check if new alloc address putted
	bool AllocChanged = false;
	// If this variable is struct, this var check if it's pointer or local struct
	bool IsPointerStruct = false;

	JsonStruct();
	virtual ~JsonStruct();

	// Set local data Pointer
	void SetAllocPointer(void* newAddress);
	// Get right size need to read, useful for 32bit games in 64bit version of this tool
	int SubUnNeededSize(JsonVariables& variables);
	// Read game data, set date to local memory and fix pointer if needed
	bool ReadData(uintptr_t address, const std::string& structType);
	// Get local data Pointer
	void* GetAllocPointer();
	// Access to variable inside this struct
	JsonVar& operator[](const std::string& varName);
	// Access to variable inside this struct
	JsonVar& GetVar(const std::string& varName);
	// Check is valid or not
	bool IsValid();
	// Initialize the struct (alloc and init variables), Don't use for temp struct or not allocated struct
	void Init(const std::string& structName, bool alloc = true);
	// 
	void FixStructData();
	// This function for init derived variables data, (set data to derived variables)
	void InitData();
};