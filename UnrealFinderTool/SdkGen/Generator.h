#pragma once
#include <string>
#include <unordered_map>
#include <vector>
#include "PatternScan.h"

enum class SdkType
{
	Internal,
	External
};

struct PredefinedMember
{
	std::string Type;
	std::string Name;
};

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

	/// <summary>Adds a predefined method which gets splitter in declaration and definition.</summary>
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

using VirtualFunctionPatterns = std::vector<std::tuple<Pattern, const char*>>;

class Generator
{
	mutable std::string gameName;
	mutable std::string gameVersion;
	mutable bool isGObjectsChunks = false;
	mutable SdkType sdkType = SdkType::Internal;
	mutable std::string sdkLang;

	std::unordered_map<std::string, size_t> alignasClasses;
	std::unordered_map<std::string, std::string> badChars;
	std::unordered_map<std::string, std::string> keywordsName;
	std::unordered_map<std::string, std::string> overrideTypes;
	std::unordered_map<std::string, std::vector<PredefinedMember>> predefinedMembers;
	std::unordered_map<std::string, std::vector<PredefinedMember>> predefinedStaticMembers;
	std::unordered_map<std::string, std::vector<PredefinedMethod>> predefinedMethods;
	std::unordered_map<std::string, VirtualFunctionPatterns> virtualFunctionPattern;

public:
	/// <summary>
	/// Initializes this object.
	/// Add predefined types, ...
	/// </summary>
	/// <returns>true if it succeeds, false if it fails.</returns>
	bool Initialize();

	/// <summary>
	/// Gets output directory where the files are getting stored.
	/// The name of the game gets appended to this directory.
	/// </summary>
	/// <returns>The output directory.</returns>
	std::string GetOutputDirectory() const;

	/// <summary>
	/// Gets the name of the game.
	/// </summary>
	/// <returns>The game name. </returns>
	std::string GetGameName() const;

	/// <summary>
	/// Gets the name of the game.
	/// </summary>
	/// <returns>The game name. </returns>
	void SetGameName(const std::string& gameName) const;

	/// <summary>
	/// Sets the version of the game.
	/// </summary>
	/// <returns>The version of the game.</returns>
	void SetGameVersion(const std::string& gameVersion) const;

	/// <summary>
	/// Gets the version of the game.
	/// </summary>
	/// <returns>The version of the game.</returns>
	std::string GetGameVersion() const;

	/// <summary>
	/// Gets generator type.
	/// </summary>
	SdkType GetSdkType() const;

	/// <summary>
	/// Sets generator type.
	/// </summary>
	void SetSdkType(SdkType sdkType) const;

	/// <summary>
	/// Sets generator Lang (Cpp, C#, etc).
	/// </summary>
	void SetSdkLang(const std::string& sdkLang) const;

	/// <summary>
	/// Gets generator Lang (Cpp, C#, etc).
	/// </summary>
	std::string GetSdkLang() const;

	void SetIsGObjectsChunks(bool isChunks) const;
	bool GetIsGObjectsChunks() const;

	/// <summary>
	/// Check if the generator should dump the object and name arrays.
	/// </summary>
	/// <returns>true if the arrays should get dumped.</returns>
	bool ShouldDumpArrays() const;

	/// <summary>
	/// Check if the generator should generate empty files (no classes, structs, ...).
	/// </summary>
	/// <returns>true if empty files should get generated.</returns>
	bool ShouldGenerateEmptyFiles() const;

	/// <summary>
	/// Check if the generated classes should use strings to identify objects.
	/// If false the generated classes use the object index.
	/// Warning: The object index may change for non default classes.
	/// </summary>
	/// <returns>true if strings should be used.</returns>
	bool ShouldUseStrings() const;

	/// <summary>
	/// Check if strings (<see cref="ShouldUseStrings()" />) should be xor encoded.
	/// </summary>
	/// <returns>true if string should be xor encoded.</returns>
	bool ShouldXorStrings() const;

	/// <summary>
	/// Check if static methods should get converted to normal methods.
	/// Static methods require a CreateDefaultObject() method in the UObject class.
	/// </summary>
	/// <returns>true if static methods should get converted to normal methods.</returns>
	bool ShouldConvertStaticMethods() const;

	/// <summary>
	/// Check if we should generate a function parameters file.
	/// Otherwise the parameters are declared inside the function body.
	/// If hooks with access to the parameters are need, this method should return true.
	/// </summary>
	/// <returns>True if a function parameters file should be generated.</returns>
	bool ShouldGenerateFunctionParametersFile() const;

	/// <summary>
	/// Gets namespace name for the classes. If the name is empty no namespace gets generated.
	/// </summary>
	/// <returns>The namespace name.</returns>
	std::string GetNamespaceName() const;

	/// <summary>
	/// Gets the member alignment.
	/// https://msdn.microsoft.com/en-us/library/2e70t5y1.aspx
	/// </summary>
	/// <returns>The member alignment.</returns>
	size_t GetGlobalMemberAlignment() const;

	/// <summary>
	/// Gets alignas size for the specific class.
	/// http://cppreference.com/w/cpp/language/alignas
	/// </summary>
	/// <param name="name">The name.</param>
	/// <returns>If the class is not found the return value is 0, else the alignas size.</returns>
	size_t GetClassAlignas(const std::string& name) const;

	/// <summary>
	/// Checks if an override is defined for the given type.
	/// </summary>
	/// <param name="type">The parameter type.</param>
	/// <returns>If no override is found the original name is returned.</returns>
	std::string GetOverrideType(const std::string& type) const;

	/// <summary>
	/// Checks if a name is Cpp Keywords.
	/// </summary>
	/// <param name="name">The parameter name.</param>
	/// <returns>If no override is found the original name is returned.</returns>
	std::string GetSafeKeywordsName(const std::string& name) const;

	/// <summary>
	/// Gets the predefined members of the specific class.
	/// </summary>
	/// <param name="name">The name of the class.</param>
	/// <param name="members">[out] The predefined members.</param>
	/// <returns>true if predefined members are found.</returns>
	bool GetPredefinedClassMembers(const std::string& name, std::vector<PredefinedMember>& members) const;

	/// <summary>
	/// Gets the static predefined members of the specific class.
	/// </summary>
	/// <param name="name">The name of the class.</param>
	/// <param name="members">[out] The predefined members.</param>
	/// <returns>true if predefined members are found.</returns>
	bool GetPredefinedClassStaticMembers(const std::string& name, std::vector<PredefinedMember>& members) const;


	/// <summary>
	/// Gets the patterns of virtual functions of the specific class.
	/// The generator loops the virtual functions of the class and adds a class method if the pattern matches.
	/// </summary>
	/// <param name="name">The name of the class.</param>
	/// <param name="patterns">[out] The patterns.</param>
	/// <returns>true if patterns are found.</returns>
	bool GetVirtualFunctionPatterns(const std::string& name, VirtualFunctionPatterns& patterns) const;

	/// <summary>Gets the predefined methods of the specific class.</summary>
	/// <param name="name">The name of the class.</param>
	/// <param name="methods">[out] The predefined methods.</param>
	/// <returns>true if predefined methods are found.</returns>
	bool GetPredefinedClassMethods(const std::string& name, std::vector<PredefinedMethod>& methods) const;
};
