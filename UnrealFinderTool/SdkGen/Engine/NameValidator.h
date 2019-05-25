#pragma once

#include <string>

class UEConst;
class UEEnum;
class UEStruct;

/// <summary>
/// Makes valid C++ name from the given name.
/// </summary>
/// <param name="name">The name to process.</param>
/// <returns>A valid C++ name.</returns>
std::string MakeValidName(std::string&& name);

std::string SimplifyEnumName(std::string&& name);

std::string MakeUniqueCppName(const UEConst& c);
std::string MakeUniqueCppName(const UEEnum& e);
std::string MakeUniqueCppName(const UEStruct& ss);
