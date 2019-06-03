#include "pch.h"
#include "NameValidator.h"

#include <cctype>

#include "ObjectsStore.h"

std::string MakeValidName(std::string&& name)
{
	std::string valid(name);

	for (auto i = 0u; i < name.length(); ++i)
	{
		if (valid[i] == ' '
			|| valid[i] == '?'
			|| valid[i] == '+'
			|| valid[i] == '-'
			|| valid[i] == ':'
			|| valid[i] == '/'
			|| valid[i] == '^'
			|| valid[i] == '('
			|| valid[i] == ')'
			|| valid[i] == '['
			|| valid[i] == ']'
			|| valid[i] == '<'
			|| valid[i] == '>'
			|| valid[i] == '&'
			|| valid[i] == '.'
			|| valid[i] == '#'
			|| valid[i] == '\''
			|| valid[i] == '"'
			|| valid[i] == '%')
		{
			valid[i] = '_';
		}
	}

	if (!valid.empty())
	{
		if (std::isdigit(valid[0]))
		{
			valid = '_' + valid;
		}
	}

	return valid;
}

std::string SimplifyEnumName(std::string&& name)
{
	const auto index = name.find_last_of(':');
	if (index == std::string::npos)
		return "";

	return name.substr(index + 1);
}

template<typename T>
std::string MakeUniqueCppNameImpl(const T& t)
{
	std::string name;
	if (ObjectsStore().CountObjects<T>(t.GetName()) > 1)
	{
		name += MakeValidName(t.GetOuter().GetName()) + "_";
	}
	return name + MakeValidName(t.GetName());
}

std::string MakeUniqueCppName(const UEConst& c)
{
	return MakeUniqueCppNameImpl(c);
}

std::string MakeUniqueCppName(const UEEnum& e)
{
	auto name = MakeUniqueCppNameImpl(e);
	if (!name.empty() && name[0] != 'E')
	{
		name = 'E' + name;
	}
	return name;
}

std::string MakeUniqueCppName(const UEStruct& ss)
{
	std::string name;
	if (ss.IsValid())
	{
		if (ObjectsStore().CountObjects<UEStruct>(ss.GetName()) > 1)
		{
			name += MakeValidName(ss.GetOuter().GetNameCPP()) + "_";
		}
	}
	
	return name + MakeValidName(ss.GetNameCPP());
}
