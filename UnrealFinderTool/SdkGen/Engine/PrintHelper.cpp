#include "pch.h"
#include "PrintHelper.h"

#include <tinyformat.h>

#include "IGenerator.h"
#include "Package.h"

void PrintFileHeader(std::ostream& os, const std::vector<std::string>& includes, bool isHeaderFile)
{
	extern IGenerator* generator;

	if (isHeaderFile)
	{
		os << "#pragma once\n\n";
	}

	os << tfm::format("// %s SDK\n\n", generator->GetGameName())
		<< tfm::format("#ifdef _MSC_VER\n\t#pragma pack(push, 0x%X)\n#endif\n\n", generator->GetGlobalMemberAlignment());

	if (!includes.empty())
	{
		for (auto&& i : includes) { os << "#include " << i << "\n"; }
		os << "\n";
	}

	if (!generator->GetNamespaceName().empty())
	{
		os << "namespace " << generator->GetNamespaceName() << "\n{\n";
	}
}

void PrintFileHeader(std::ostream& os, bool isHeaderFile)
{
	extern IGenerator* generator;

	PrintFileHeader(os, std::vector<std::string>(), isHeaderFile);
}

void PrintFileFooter(std::ostream& os)
{
	extern IGenerator* generator;

	if (!generator->GetNamespaceName().empty())
	{
		os << "}\n\n";
	}

	os << "#ifdef _MSC_VER\n\t#pragma pack(pop)\n#endif\n";
}

void PrintSectionHeader(std::ostream& os, const char* name)
{
	os << "//---------------------------------------------------------------------------\n"
		<< "//" << name << "\n"
		<< "//---------------------------------------------------------------------------\n\n";
}

std::string GenerateFileName(FileContentType type, const Package& package)
{
	extern IGenerator* generator;

	const char* name = "";
	switch (type)
	{
	case FileContentType::Structs:
		name = "%s_structs.hpp";
		break;
	case FileContentType::Classes:
		name = "%s_classes.hpp";
		break;
	case FileContentType::Functions:
		name = "%s_functions.cpp";
		break;
	case FileContentType::FunctionParameters:
		name = "%s_parameters.hpp";
		break;
	default:
		assert(false);
	}

	return tfm::format(name, package.GetName());
}
