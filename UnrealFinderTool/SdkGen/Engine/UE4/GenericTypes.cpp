#include "pch.h"
#include "../MainEngineClasses.h"
#include "GenericTypes.h"

void* UEObject::GetAddress() const
{
	return reinterpret_cast<PVOID>(Object.ObjAddress);
}
/*
bool UEObject::Update(const UEObject& newObject)
{
	if (Object.IsEqual(newObject.Object))
		return false;

	Object = newObject.Object;
	return true;
}

bool UEObject::Update(const UObject& newObject)
{
	if (!Utils::IsValidAddress(Utils::MemoryObj, newObject.ObjAddress))
		return false;

	if (Object.IsEqual(newObject))
		return false;

	Object = newObject;
	return true;
}
*/

UEObject UEObject::GetPackageObject() const
{
	// Package Is The Last Outer
	if (!package.Empty())
		return UEObject(package);

	for (UEObject outer = GetOuter(); outer.IsValid(); outer = outer.GetOuter())
	{
		package = outer.Object;
	}

	return UEObject(package);
}

std::string UEObject::GetFullName() const
{
	if (!fullName.empty())
		return fullName;

	auto cClass = GetClass();
	if (cClass.IsValid())
	{
		std::string temp;

		for (auto outer = GetOuter(); outer.IsValid(); outer = outer.GetOuter())
		{
			temp.insert(0, outer.GetName() + ".");
		}

		std::string name = cClass.GetName();
		name += " ";
		name += temp;
		name += GetName();

		fullName = name;
		return fullName;
	}

	return std::string("(null)");
}

std::string UEObject::GetNameCPP() const
{
	if (!nameCpp.empty())
		return nameCpp;

	std::string name;
	if (IsA<UEClass>())
	{
		auto c = Cast<UEClass>();
		while (c.IsValid())
		{
			const auto className = c.GetName();
			if (className == "Actor")
			{
				name += "A";
				break;
			}
			if (className == "Object")
			{
				name += "U";
				break;
			}

			c = c.GetSuper().Cast<UEClass>();
		}
	}
	else
	{
		name += "F";
	}

	name += GetName();
	nameCpp = name;

	return name;
}

UEProperty::Info UEProperty::GetInfo() const
{
	if (infoChanged)
		return curInfo;

	if (IsValid())
	{
		if (IsA<UEByteProperty>())
		{
			infoChanged = true;
			curInfo = Cast<UEByteProperty>().GetInfo();
		}
		else if (IsA<UEUInt16Property>())
		{
			infoChanged = true;
			curInfo = Cast<UEUInt16Property>().GetInfo();
		}
		else if (IsA<UEUInt32Property>())
		{
			infoChanged = true;
			curInfo = Cast<UEUInt32Property>().GetInfo();
		}
		else if (IsA<UEUInt64Property>())
		{
			infoChanged = true;
			curInfo = Cast<UEUInt64Property>().GetInfo();
		}
		else if (IsA<UEInt8Property>())
		{
			infoChanged = true;
			curInfo = Cast<UEInt8Property>().GetInfo();
		}
		else if (IsA<UEInt16Property>())
		{
			infoChanged = true;
			curInfo = Cast<UEInt16Property>().GetInfo();
		}
		else if (IsA<UEIntProperty>())
		{
			infoChanged = true;
			curInfo = Cast<UEIntProperty>().GetInfo();
		}
		else if (IsA<UEInt64Property>())
		{
			infoChanged = true;
			curInfo = Cast<UEInt64Property>().GetInfo();
		}
		else if (IsA<UEFloatProperty>())
		{
			infoChanged = true;
			curInfo = Cast<UEFloatProperty>().GetInfo();
		}
		else if (IsA<UEDoubleProperty>())
		{
			infoChanged = true;
			curInfo = Cast<UEDoubleProperty>().GetInfo();
		}
		else if (IsA<UEBoolProperty>())
		{
			infoChanged = true;
			curInfo = Cast<UEBoolProperty>().GetInfo();
		}
		else if (IsA<UEObjectProperty>())
		{
			infoChanged = true;
			curInfo = Cast<UEObjectProperty>().GetInfo();
		}
		else if (IsA<UEClassProperty>())
		{
			infoChanged = true;
			curInfo = Cast<UEClassProperty>().GetInfo();
		}
		else if (IsA<UEInterfaceProperty>())
		{
			infoChanged = true;
			curInfo = Cast<UEInterfaceProperty>().GetInfo();
		}
		else if (IsA<UEWeakObjectProperty>())
		{
			infoChanged = true;
			curInfo = Cast<UEWeakObjectProperty>().GetInfo();
		}
		else if (IsA<UELazyObjectProperty>())
		{
			infoChanged = true;
			curInfo = Cast<UELazyObjectProperty>().GetInfo();
		}
		else if (IsA<UEAssetObjectProperty>())
		{
			infoChanged = true;
			curInfo = Cast<UEAssetObjectProperty>().GetInfo();
		}
		else if (IsA<UEAssetClassProperty>())
		{
			infoChanged = true;
			curInfo = Cast<UEAssetClassProperty>().GetInfo();
		}
		else if (IsA<UENameProperty>())
		{
			infoChanged = true;
			curInfo = Cast<UENameProperty>().GetInfo();
		}
		else if (IsA<UEStructProperty>())
		{
			infoChanged = true;
			curInfo = Cast<UEStructProperty>().GetInfo();
		}
		else if (IsA<UEStrProperty>())
		{
			infoChanged = true;
			curInfo = Cast<UEStrProperty>().GetInfo();
		}
		else if (IsA<UETextProperty>())
		{
			infoChanged = true;
			curInfo = Cast<UETextProperty>().GetInfo();
		}
		else if (IsA<UEArrayProperty>())
		{
			infoChanged = true;
			curInfo = Cast<UEArrayProperty>().GetInfo();
		}
		else if (IsA<UEMapProperty>())
		{
			infoChanged = true;
			curInfo = Cast<UEMapProperty>().GetInfo();
		}
		else if (IsA<UEDelegateProperty>())
		{
			infoChanged = true;
			curInfo = Cast<UEDelegateProperty>().GetInfo();
		}
		else if (IsA<UEMulticastDelegateProperty>())
		{
			infoChanged = true;
			curInfo = Cast<UEMulticastDelegateProperty>().GetInfo();
		}
		else if (IsA<UEEnumProperty>())
		{
			infoChanged = true;
			curInfo = Cast<UEEnumProperty>().GetInfo();
		}
	}

	if (infoChanged)
		return curInfo;

	return { PropertyType::Unknown };
}

//---------------------------------------------------------------------------
//UEByteProperty
//---------------------------------------------------------------------------
bool UEByteProperty::IsEnum() const
{
	return GetEnum().IsValid();
}

//---------------------------------------------------------------------------
//UEBoolProperty
//---------------------------------------------------------------------------
int GetBitPosition(uint8_t value)
{
	int i4 = !(value & 0xf) << 2;
	value >>= i4;

	int i2 = !(value & 0x3) << 1;
	value >>= i2;

	int i1 = !(value & 0x1);

	int i0 = (value >> i1) & 1 ? 0 : -8;

	return i4 + i2 + i1 + i0;
}
std::array<int, 2> UEBoolProperty::GetMissingBitsCount(const UEBoolProperty & other) const
{
	// If there is no previous bitfield member, just calculate the missing bits.
	if (!other.IsValid())
	{
		return { GetBitPosition(GetByteMask()), -1 };
	}

	// If both bitfield member belong to the same byte, calculate the bit position difference.
	if (GetOffset() == other.GetOffset())
	{
		return { GetBitPosition(GetByteMask()) - GetBitPosition(other.GetByteMask()) - 1, -1 };
	}

	// If they have different offsets, we need two distances
	// |00001000|00100000|
	// 1.   ^---^
	// 2.       ^--^

	return { std::numeric_limits<uint8_t>::digits - GetBitPosition(other.GetByteMask()) - 1, GetBitPosition(GetByteMask()) };
}