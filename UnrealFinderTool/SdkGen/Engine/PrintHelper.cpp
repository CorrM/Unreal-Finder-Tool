#include "pch.h"
#include <tinyformat.h>

#include "Package.h"
#include "Utils.h"
#include "PrintHelper.h"

std::string GetFileHeader(const std::vector<std::string>& pragmas, const std::vector<std::string>& includes, const bool isHeaderFile)
{
	std::string sstr;

	if (isHeaderFile)
	{
		sstr += "#pragma once\n";
		if (!pragmas.empty())
		{
			for (auto&& i : pragmas) { sstr += "#pragma " + i + "\n"; }
		}
		sstr += "\n";
	}

	if (Utils::GenObj->GetSdkType() == SdkType::External)
		sstr += "#include \"" + Utils::Settings.SdkGen.MemoryHeader + "\"\n";

	if (!includes.empty())
	{
		for (auto&& i : includes) { sstr += "#include " + i + "\n"; }
		sstr += "\n";
	}

	sstr += tfm::format("// Name: %s, Version: %s\n\n", Utils::GenObj->GetGameName(), Utils::GenObj->GetGameVersion())
		 + tfm::format("#ifdef _MSC_VER\n\t#pragma pack(push, 0x%X)\n#endif\n\n", Utils::GenObj->GetGlobalMemberAlignment());

	sstr += "namespace " + Utils::GenObj->GetNamespaceName() + "\n{\n";

	return sstr;
}

std::string GetFileFooter()
{
	std::string str;

	if (!Utils::GenObj->GetNamespaceName().empty())
		str += "}\n\n";

	str += "#ifdef _MSC_VER\n\t#pragma pack(pop)\n#endif\n";
	return str;
}

void PrintFileHeader(std::ostream& os, const std::vector<std::string>& pragmas, const std::vector<std::string>& includes, const bool isHeaderFile)
{
	os << GetFileHeader(pragmas, includes, isHeaderFile);
}

void PrintFileHeader(std::ostream& os, const std::vector<std::string>& includes, const bool isHeaderFile)
{
	PrintFileHeader(os, {}, includes, isHeaderFile);
}

void PrintFileHeader(std::ostream& os, const bool isHeaderFile)
{
	PrintFileHeader(os, {}, {}, isHeaderFile);
}

void PrintFileFooter(std::ostream& os)
{
	os << GetFileFooter();
}

void PrintExistsFile(
	const std::string& fileName,
	const fs::path& sdkPath,
	const std::vector<std::string>& pragmas,
	const std::vector<std::string>& includes,
	const bool isHeaderFile,
	const std::function<void(std::string&)>& execBeforeWrite)
{
	std::string fileText;
	if (!Utils::FileRead("Config\\Includes\\" + fileName, fileText)) return;

	// Replace Main Stuff
	fileText = Utils::ReplaceString(fileText, "/*!!INCLUDE_PLACEHOLDER!!*/", GetFileHeader(pragmas, includes, isHeaderFile));

	// Do replaces here
	execBeforeWrite(fileText);

	// Get footer
	fileText = Utils::ReplaceString(fileText, "/*!!FOOTER_PLACEHOLDER!!*/", GetFileFooter());

	// Write file
	std::string filePath = (sdkPath / fileName).string();
	Utils::FileCreate(filePath);
	Utils::FileWrite(filePath, fileText);
}

void PrintSectionHeader(std::ostream& os, const char* name)
{
	os  << "//---------------------------------------------------------------------------\n"
		<< "// " << name << "\n"
		<< "//---------------------------------------------------------------------------\n\n";
}

std::string GenerateFileName(const FileContentType type, const Package& package)
{
	const char* name = "";
	switch (type)
	{
	case FileContentType::Structs:
		name = "%s_structs.h";
		break;
	case FileContentType::Classes:
		name = "%s_classes.h";
		break;
	case FileContentType::Functions:
		name = "%s_functions.cpp";
		break;
	case FileContentType::FunctionParameters:
		name = "%s_parameters.h";
		break;
	default:
		assert(false);
	}

	return tfm::format(name, package.GetName());
}
