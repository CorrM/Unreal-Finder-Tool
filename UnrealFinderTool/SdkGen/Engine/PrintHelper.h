#pragma once

#include <vector>
#include <string>

std::string GetFileHeader(const std::vector<std::string>& pragmas, const std::vector<std::string>& includes, bool isHeaderFile);

void PrintFileHeader(std::ostream& os, const std::vector<std::string>& pragmas, const std::vector<std::string>& includes, bool isHeaderFile);

void PrintFileHeader(std::ostream& os, const std::vector<std::string>& includes, bool isHeaderFile);

void PrintFileHeader(std::ostream& os, bool isHeaderFile);

void PrintFileFooter(std::ostream& os);

void PrintSectionHeader(std::ostream& os, const char* name);

void PrintExistsFile(
	const std::string& fileName,
	const fs::path& sdkPath,
	const std::vector<std::string>& pragmas,
	const std::vector<std::string>& includes,
	bool isHeaderFile,
	const std::function<void(std::string&)>& execBeforeWrite);

enum class FileContentType
{
	Structs,
	Classes,
	Functions,
	FunctionParameters
};

/// <summary>
/// Generates a file name composed by the game name and the package object.
/// </summary>
/// <param name="type">The type of the file.</param>
/// <returns>
/// The generated file name.
/// </returns>
std::string GenerateFileName(FileContentType type, const class Package& package);
