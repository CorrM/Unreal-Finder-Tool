#include "pch.h"
#include "../Package.h"

bool Package::Method::Parameter::MakeType(UEPropertyFlags flags, Type& type)
{
	if (flags & UEPropertyFlags::ReturnParm)
	{
		type = Type::Return;
	}
	else if (flags & UEPropertyFlags::OutParm)
	{
		//if it is a const parameter make it a default parameter
		if (flags & UEPropertyFlags::ConstParm)
		{
			type = Type::Default;
		}
		else
		{
			type = Type::Out;
		}
	}
	else if (flags & UEPropertyFlags::Parm)
	{
		type = Type::Default;
	}
	else
	{
		return false;
	}

	return true;
}