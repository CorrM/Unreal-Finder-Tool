#include "pch.h"
#include "FunctionFlags.hpp"

#include <vector>
#include <sstream>
#include <iterator>

std::string StringifyFlags(const UEFunctionFlags flags)
{
	std::vector<const char*> buffer;

	if (flags & UEFunctionFlags::Final) { buffer.push_back("Final"); }
	if (flags & UEFunctionFlags::RequiredAPI) { buffer.push_back("RequiredAPI"); }
	if (flags & UEFunctionFlags::BlueprintAuthorityOnly) { buffer.push_back("BlueprintAuthorityOnly"); }
	if (flags & UEFunctionFlags::BlueprintCosmetic) { buffer.push_back("BlueprintCosmetic"); }
	if (flags & UEFunctionFlags::Net) { buffer.push_back("Net"); }
	if (flags & UEFunctionFlags::NetReliable) { buffer.push_back("NetReliable"); }
	if (flags & UEFunctionFlags::NetRequest) { buffer.push_back("NetRequest"); }
	if (flags & UEFunctionFlags::Exec) { buffer.push_back("Exec"); }
	if (flags & UEFunctionFlags::Native) { buffer.push_back("Native"); }
	if (flags & UEFunctionFlags::Event) { buffer.push_back("Event"); }
	if (flags & UEFunctionFlags::NetResponse) { buffer.push_back("NetResponse"); }
	if (flags & UEFunctionFlags::Static) { buffer.push_back("Static"); }
	if (flags & UEFunctionFlags::NetMulticast) { buffer.push_back("NetMulticast"); }
	if (flags & UEFunctionFlags::MulticastDelegate) { buffer.push_back("MulticastDelegate"); }
	if (flags & UEFunctionFlags::Public) { buffer.push_back("Public"); }
	if (flags & UEFunctionFlags::Private) { buffer.push_back("Private"); }
	if (flags & UEFunctionFlags::Protected) { buffer.push_back("Protected"); }
	if (flags & UEFunctionFlags::Delegate) { buffer.push_back("Delegate"); }
	if (flags & UEFunctionFlags::NetServer) { buffer.push_back("NetServer"); }
	if (flags & UEFunctionFlags::HasOutParms) { buffer.push_back("HasOutParms"); }
	if (flags & UEFunctionFlags::HasDefaults) { buffer.push_back("HasDefaults"); }
	if (flags & UEFunctionFlags::NetClient) { buffer.push_back("NetClient"); }
	if (flags & UEFunctionFlags::DLLImport) { buffer.push_back("DLLImport"); }
	if (flags & UEFunctionFlags::BlueprintCallable) { buffer.push_back("BlueprintCallable"); }
	if (flags & UEFunctionFlags::BlueprintEvent) { buffer.push_back("BlueprintEvent"); }
	if (flags & UEFunctionFlags::BlueprintPure) { buffer.push_back("BlueprintPure"); }
	if (flags & UEFunctionFlags::Const) { buffer.push_back("Const"); }
	if (flags & UEFunctionFlags::NetValidate) { buffer.push_back("NetValidate"); }

	switch (buffer.size())
	{
	case 0:
		return std::string();
	case 1:
		return std::string(buffer[0]);
	default:
		std::ostringstream os;
		std::copy(buffer.begin(), buffer.end() - 1, std::ostream_iterator<const char*>(os, ", "));
		os << *buffer.rbegin();
		return os.str();
	}
}