#pragma once
#include "GenericTypes.h"

#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <mutex>

namespace fs = std::filesystem;


class Package
{
	friend struct std::hash<Package>;
	friend struct PackageDependencyComparer;
	friend bool operator==(const Package& lhs, const Package& rhs);

public:
	struct Constant
	{
		std::string Name;
		std::string Value;
	};
	struct Enum
	{
		std::string Name;
		std::string FullName;
		std::vector<std::string> Values;
	};
	struct Member
	{
		std::string Name;
		std::string Type;
		bool IsStatic = false;

		size_t Offset;
		size_t Size;

		size_t Flags;
		std::string FlagsString;

		std::string Comment;
	};
	struct ScriptStruct
	{
		std::string Name;
		std::string FullName;
		std::string NameCpp;
		std::string NameCppFull;

		size_t Size;
		size_t InheritedSize;

		std::vector<Member> Members;
		std::vector<PredefinedMethod> PredefinedMethods;
	};
	struct Method
	{
		struct Parameter
		{
			enum class Type
			{
				Default,
				Out,
				Return
			};

			Type ParamType;
			bool PassByReference;
			std::string CppType;
			std::string Name;
			std::string FlagsString;

			/// <summary>
			/// Generates a valid type of the property flags.
			/// </summary>
			/// <param name="flags">The property flags.</param>
			/// <param name="type">[out] The parameter type.</param>
			/// <returns>true if it is a valid type, else false.</returns>
			static bool MakeType(UEPropertyFlags flags, Type& type);
		};

		size_t Index;
		std::string Name;
		std::string FullName;
		std::vector<Parameter> Parameters;
		std::string FlagsString;
		bool IsNative;
		bool IsStatic;
	};
	struct Class : ScriptStruct
	{
		std::vector<std::string> VirtualFunctions;
		std::vector<Method> Methods;
	};

	/// <summary>
	/// Constructor.
	/// </summary>
	/// <param name="packageObj">The package object.</param>
	explicit Package(UEObject* packageObj);

	/// <summary>
	/// Get all objects in this package
	/// </summary>
	static void GetObjsInPack(UEObject* packageObj, std::vector<UEObject*>& out);

	std::string GetName() const { return packageObj->GetName(); }

	/// <summary>
	/// Process the classes the package contains.
	/// </summary>
	void Process(std::unordered_map<uintptr_t, bool>& processedObjects);

	/// <summary>
	/// Saves the package classes as C++ code.
	/// Files are only generated if there is code present or the generator forces the generation of empty files.
	/// </summary>
	/// <returns>true if files got saved, else false.</returns>
	bool Save() const;

private:
	bool AddDependency(UEObject* package) const;

	/// <summary>
	/// Checks and generates the prerequisites of the object.
	/// Should be a UEClass or UEScriptStruct.
	/// </summary>
	/// <param name="obj">The object.</param>
	/// <param name="processedObjects"></param>
	void GeneratePrerequisites(const UEObject& obj, std::unordered_map<uintptr_t, bool>& processedObjects);

	/// <summary>
	/// Checks and generates the prerequisites of the members.
	/// </summary>
	/// <param name="first">The first member in the chain.</param>
	/// <param name="processedObjects"></param>
	void GenerateMemberPrerequisites(const UEProperty& first, std::unordered_map<uintptr_t, bool>& processedObjects);

	/// <summary>
	/// Generates a script structure.
	/// </summary>
	/// <param name="scriptStructObj">The script structure object.</param>
	void GenerateScriptStruct(const UEScriptStruct& scriptStructObj);

	/// <summary>
	/// Generates a constant.
	/// </summary>
	/// <param name="constObj">The constant object.</param>
	void GenerateConst(const UEConst& constObj);

	/// <summary>
	/// Generates an enum.
	/// </summary>
	/// <param name="enumObj">The enum object.</param>
	void GenerateEnum(const UEEnum& enumObj);

	/// <summary>
	/// Generates the class.
	/// </summary>
	/// <param name="classObj">The class object.</param>
	void GenerateClass(const UEClass& classObj);

	/// <summary>
	/// Writes all structs into the appropriate file.
	/// </summary>
	void SaveStructs() const;

	/// <summary>
	/// Writes all classes into the appropriate file.
	/// </summary>
	void SaveClasses() const;

	/// <summary>
	/// Writes all functions into the appropriate file.
	/// </summary>
	void SaveFunctions() const;

	/// <summary>
	/// Writes all function parameters into the appropriate file.
	/// </summary>
	void SaveFunctionParameters() const;

	UEObject* packageObj;
	mutable std::unordered_set<UEObject> dependencies;

	/*
	 * MEMBER
	 */
	/// <summary>
	/// Generates a padding member.
	/// </summary>
	/// <param name="id">The unique name identifier.</param>
	/// <param name="offset">The offset.</param>
	/// <param name="size">The size.</param>
	/// <param name="reason">The reason.</param>
	/// <returns>A padding member.</returns>
	static Member CreatePadding(size_t id, size_t offset, size_t size, std::string reason);

	/// <summary>
	/// Generates a padding member.
	/// </summary>
	/// <param name="id">The unique name identifier.</param>
	/// <param name="offset">The offset.</param>
	/// <param name="type">The type.</param>
	/// <param name="bits">The bits.</param>
	/// <returns>A padding member.</returns>
	static Member CreateBitfieldPadding(size_t id, size_t offset, std::string type, size_t bits);

	/// <summary>
	/// Generates the members of a struct or class.
	/// </summary>
	/// <param name="structObj">The structure object.</param>
	/// <param name="offset">The start offset.</param>
	/// <param name="properties">The properties describing the members.</param>
	/// <param name="members">[out] The members of the struct or class.</param>
	void GenerateMembers(const UEStruct& structObj, size_t offset, const std::vector<UEProperty>& properties, std::vector<Member>& members) const;

	/*
	 * METHODS
	 */
	/// <summary>
	/// Generates the methods of a class.
	/// </summary>
	/// <param name="classObj">The class object.</param>
	/// <param name="methods">[out] The methods of the class.</param>
	void GenerateMethods(const UEClass& classObj, std::vector<Method>& methods) const;
public:
	static std::unordered_map<UEObject, const Package*> PackageMap;
	std::vector<Constant> Constants;
	std::vector<Class> Classes;
	std::vector<ScriptStruct> ScriptStructs;
	std::vector<Enum> Enums;
};

namespace std
{
	template<>
	struct hash<Package>
	{
		size_t operator()(const Package& package) const noexcept
		{
			return std::hash<uintptr_t>()(package.packageObj->GetAddress());
		}
	};
}

inline bool operator==(const Package& lhs, const Package& rhs) { return rhs.packageObj->GetAddress() == lhs.packageObj->GetAddress(); }
inline bool operator!=(const Package& lhs, const Package& rhs) { return !(lhs == rhs); }

struct PackageDependencyComparer
{
	bool operator()(const std::unique_ptr<Package>& lhs, const std::unique_ptr<Package>& rhs) const
	{
		return operator()(*lhs, *rhs);
	}

	bool operator()(const Package* lhs, const Package* rhs) const
	{
		return operator()(*lhs, *rhs);
	}

	bool operator()(const Package& lhs, const Package& rhs) const
	{
		if (rhs.dependencies.empty())
		{
			return false;
		}

		if (std::find(rhs.dependencies.begin(), rhs.dependencies.end(), *lhs.packageObj) != std::end(rhs.dependencies))
		{
			return true;
		}

		for (const auto& dep : rhs.dependencies)
		{
			const auto package = Package::PackageMap[dep];
			if (package == nullptr)
			{
				// Missing package, should not occur...
				continue;
			}

			if (operator()(lhs, *package))
			{
				return true;
			}
		}

		return false;
	}
};