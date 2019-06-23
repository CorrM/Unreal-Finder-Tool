#pragma once

#include <string>
#include <unordered_map>
#include <vector>
#include <iterator>
#include "PatternScan.h"

enum class SdkType
{
	Internal = 0,
	External = 1
};

class IGenerator
{
public:
	virtual ~IGenerator() = default;

	/// <summary>
	/// Initializes this object.
	/// Add predefined types, ...
	/// </summary>
	/// <returns>true if it succeeds, false if it fails.</returns>
	virtual bool Initialize() = 0;

	/// <summary>
	/// Gets output directory where the files are getting stored.
	/// The name of the game gets appended to this directory.
	/// </summary>
	/// <returns>The output directory.</returns>
	virtual std::string GetOutputDirectory() const
	{
		return "C:/SDK_GEN";
	}

	/// <summary>
	/// Gets the name of the game.
	/// </summary>
	/// <returns>The game name. </returns>
	virtual std::string GetGameName() const = 0;

	/// <summary>
	/// Gets the name of the game.
	/// </summary>
	/// <returns>The game name. </returns>
	virtual void SetGameName(const std::string& gameName) const = 0;

	/// <summary>
	/// Sets the version of the game.
	/// </summary>
	/// <returns>The version of the game.</returns>
	virtual void SetGameVersion(const std::string& gameVersion) const = 0;

	/// <summary>
	/// Gets the version of the game.
	/// </summary>
	/// <returns>The version of the game.</returns>
	virtual std::string GetGameVersion() const = 0;

	/// <summary>
	/// Gets generator type.
	/// </summary>
	virtual SdkType GetSdkType() const = 0;

	/// <summary>
	/// Sets generator type.
	/// </summary>
	/// <returns>The version of the game.</returns>
	virtual void SetSdkType(SdkType sdkType) const = 0;

	virtual void SetIsGObjectsChunks(bool isChunks) const = 0;

	/// <summary>
	/// Check if the generator should dump the object and name arrays.
	/// </summary>
	/// <returns>true if the arrays should get dumped.</returns>
	virtual bool ShouldDumpArrays() const
	{
		return false;
	}

	/// <summary>
	/// Check if the generator should generate empty files (no classes, structs, ...).
	/// </summary>
	/// <returns>true if empty files should get generated.</returns>
	virtual bool ShouldGenerateEmptyFiles() const
	{
		return false;
	}

	/// <summary>
	/// Check if the generated classes should use strings to identify objects.
	/// If false the generated classes use the object index.
	/// Warning: The object index may change for non default classes.
	/// </summary>
	/// <returns>true if strings should be used.</returns>
	virtual bool ShouldUseStrings() const
	{
		return true;
	}

	/// <summary>
	/// Check if strings (<see cref="ShouldUseStrings()" />) should be xor encoded.
	/// </summary>
	/// <returns>true if string should be xor encoded.</returns>
	virtual bool ShouldXorStrings() const
	{
		return false;
	}

	/// <summary>
	/// Check if static methods should get converted to normal methods.
	/// Static methods require a CreateDefaultObject() method in the UObject class.
	/// </summary>
	/// <returns>true if static methods should get converted to normal methods.</returns>
	virtual bool ShouldConvertStaticMethods() const
	{
		return true;
	}

	/// <summary>
	/// Check if we should generate a function parameters file.
	/// Otherwise the parameters are declared inside the function body.
	/// If hooks with access to the parameters are need, this method should return true.
	/// </summary>
	/// <returns>True if a function parameters file should be generated.</returns>
	virtual bool ShouldGenerateFunctionParametersFile() const = 0;

	/// <summary>
	/// Gets namespace name for the classes. If the name is empty no namespace gets generated.
	/// </summary>
	/// <returns>The namespace name.</returns>
	virtual std::string GetNamespaceName() const
	{
		return std::string();
	}

	/// <summary>
	/// Gets a list of custom include files which gets inserted in the SDK.
	/// </summary>
	/// <returns>The list of include files.</returns>
	virtual std::vector<std::string> GetIncludes() const
	{
		return {};
	}

	/// <summary>
	/// Gets the member alignment.
	/// https://msdn.microsoft.com/en-us/library/2e70t5y1.aspx
	/// </summary>
	/// <returns>The member alignment.</returns>
	virtual size_t GetGlobalMemberAlignment() const
	{
		return sizeof(size_t);
	}

	/// <summary>
	/// Gets alignas size for the specific class.
	/// http://cppreference.com/w/cpp/language/alignas
	/// </summary>
	/// <param name="name">The name.</param>
	/// <returns>If the class is not found the return value is 0, else the alignas size.</returns>
	virtual size_t GetClassAlignas(const std::string& name) const
	{
		auto it = alignasClasses.find(name);
		if (it != std::end(alignasClasses))
		{
			return it->second;
		}
		return 0;
	}

	/// <summary>
	/// Gets the declarations of some basic classes and methods.
	/// </summary>
	/// <returns>The basic declarations.</returns>
	virtual std::string GetBasicDeclarations() const
	{
		return std::string();
	}

	/// <summary>
	/// Gets the definitions of the <see cref="GetBasicDeclarations()" /> declarations.
	/// </summary>
	/// <returns>The basic definitions.</returns>
	virtual std::string GetBasicDefinitions() const
	{
		return std::string();
	}

	/// <summary>
	/// Checks if an override is defined for the given type.
	/// </summary>
	/// <param name="type">The parameter type.</param>
	/// <returns>If no override is found the original name is returned.</returns>
	virtual std::string GetOverrideType(const std::string& type) const
	{
		auto it = overrideTypes.find(type);
		if (it == std::end(overrideTypes))
		{
			return type;
		}
		return it->second;
	}

	/// <summary>
	/// Checks if a name is Cpp Keywords.
	/// </summary>
	/// <param name="name">The parameter name.</param>
	/// <returns>If no override is found the original name is returned.</returns>
	virtual std::string GetSafeKeywordsName(const std::string& name) const
	{
		std::string ret = name;
		auto it = keywordsName.find(ret);
		if (it == std::end(keywordsName))
		{
			for (const auto& badChar : badChars)
				ret = Utils::ReplaceString(ret, badChar.first, badChar.second);
			return ret;
		}

		ret = it->second;
		for (const auto& badChar : badChars)
			ret = Utils::ReplaceString(ret, badChar.first, badChar.second);

		return ret;
	}

	struct PredefinedMember
	{
		std::string Type;
		std::string Name;
	};

	/// <summary>
	/// Gets the predefined members of the specific class.
	/// </summary>
	/// <param name="name">The name of the class.</param>
	/// <param name="members">[out] The predefined members.</param>
	/// <returns>true if predefined members are found.</returns>
	virtual bool GetPredefinedClassMembers(const std::string& name, std::vector<PredefinedMember>& members) const
	{
		auto it = predefinedMembers.find(name);
		if (it != std::end(predefinedMembers))
		{
			std::copy(std::begin(it->second), std::end(it->second), std::back_inserter(members));

			return true;
		}

		return false;
	}

	/// <summary>
	/// Gets the static predefined members of the specific class.
	/// </summary>
	/// <param name="name">The name of the class.</param>
	/// <param name="members">[out] The predefined members.</param>
	/// <returns>true if predefined members are found.</returns>
	virtual bool GetPredefinedClassStaticMembers(const std::string& name, std::vector<PredefinedMember>& members) const
	{
		auto it = predefinedStaticMembers.find(name);
		if (it != std::end(predefinedStaticMembers))
		{
			std::copy(std::begin(it->second), std::end(it->second), std::back_inserter(members));

			return true;
		}

		return false;
	}

	using VirtualFunctionPatterns = std::vector<std::tuple<Pattern, const char*>>;

	/// <summary>
	/// Gets the patterns of virtual functions of the specific class.
	/// The generator loops the virtual functions of the class and adds a class method if the pattern matches.
	/// </summary>
	/// <param name="name">The name of the class.</param>
	/// <param name="patterns">[out] The patterns.</param>
	/// <returns>true if patterns are found.</returns>
	virtual bool GetVirtualFunctionPatterns(const std::string& name, VirtualFunctionPatterns& patterns) const
	{
		auto it = virtualFunctionPattern.find(name);
		if (it != std::end(virtualFunctionPattern))
		{
			std::copy(std::begin(it->second), std::end(it->second), std::back_inserter(patterns));

			return true;
		}

		return false;
	}

	struct PredefinedMethod
	{
		enum class Type
		{
			Default,
			Inline
		};

		std::string Signature;
		std::string Body;
		Type MethodType;

		/// <summary>Adds a predefined method which gets splittet in declaration and definition.</summary>
		/// <param name="signature">The method signature.</param>
		/// <param name="body">The method body.</param>
		/// <returns>The method.</returns>
		static PredefinedMethod Default(std::string&& signature, std::string&& body)
		{
			return { signature, body, Type::Default };
		}

		/// <summary>Adds a predefined method which gets included as an inline method.</summary>
		/// <param name="body">The body.</param>
		/// <returns>The method.</returns>
		static PredefinedMethod Inline(std::string&& body)
		{
			return { std::string(), body, Type::Inline };
		}
	};

	/// <summary>Gets the predefined methods of the specific class.</summary>
	/// <param name="name">The name of the class.</param>
	/// <param name="methods">[out] The predefined methods.</param>
	/// <returns>true if predefined methods are found.</returns>
	virtual bool GetPredefinedClassMethods(const std::string& name, std::vector<PredefinedMethod>& methods) const
	{
		auto it = predefinedMethods.find(name);
		if (it != std::end(predefinedMethods))
		{
			std::copy(std::begin(it->second), std::end(it->second), std::back_inserter(methods));

			return true;
		}

		return false;
	}

protected:
	std::unordered_map<std::string, size_t> alignasClasses;
	std::unordered_map<std::string, std::string> badChars;
	std::unordered_map<std::string, std::string> keywordsName;
	std::unordered_map<std::string, std::string> overrideTypes;
	std::unordered_map<std::string, std::vector<PredefinedMember>> predefinedMembers;
	std::unordered_map<std::string, std::vector<PredefinedMember>> predefinedStaticMembers;
	std::unordered_map<std::string, std::vector<PredefinedMethod>> predefinedMethods;
	std::unordered_map<std::string, VirtualFunctionPatterns> virtualFunctionPattern;
};
