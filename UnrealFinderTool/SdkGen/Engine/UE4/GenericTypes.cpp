#include "pch.h"
#include "GenericTypes.h"

void* UEObject::GetAddress() const
{
	return object;
}

UEObject UEObject::GetPackageObject() const
{
	// Package Is The Last Outer
	UEObject package(nullptr, false);

	for (auto outer = GetOuter(); outer.IsValid(); outer = outer.GetOuter())
	{
		package = outer.object;
	}

	return package;
}

std::string UEObject::GetFullName() const
{
	auto cClass = GetClass();
	if (cClass.IsValid())
	{
		std::string temp;

		UEObject outer = GetOuter();
		while (outer.IsValid())
		{
			temp = outer.GetName() + "." + temp;
			outer = outer.GetOuter().object;
		}

		std::string name = cClass.GetName();
		name += " ";
		name += temp;
		name += GetName();

		return name;
	}

	return std::string("(null)");
}

std::string UEObject::GetNameCPP() const
{
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

	return name;
}

UEProperty::Info UEProperty::GetInfo() const
{
	if (IsValid())
	{
		if (IsA<UEByteProperty>())
		{
			return Cast<UEByteProperty>().GetInfo();
		}
		if (IsA<UEUInt16Property>())
		{
			return Cast<UEUInt16Property>().GetInfo();
		}
		if (IsA<UEUInt32Property>())
		{
			return Cast<UEUInt32Property>().GetInfo();
		}
		if (IsA<UEUInt64Property>())
		{
			return Cast<UEUInt64Property>().GetInfo();
		}
		if (IsA<UEInt8Property>())
		{
			return Cast<UEInt8Property>().GetInfo();
		}
		if (IsA<UEInt16Property>())
		{
			return Cast<UEInt16Property>().GetInfo();
		}
		if (IsA<UEIntProperty>())
		{
			return Cast<UEIntProperty>().GetInfo();
		}
		if (IsA<UEInt64Property>())
		{
			return Cast<UEInt64Property>().GetInfo();
		}
		if (IsA<UEFloatProperty>())
		{
			return Cast<UEFloatProperty>().GetInfo();
		}
		if (IsA<UEDoubleProperty>())
		{
			return Cast<UEDoubleProperty>().GetInfo();
		}
		if (IsA<UEBoolProperty>())
		{
			return Cast<UEBoolProperty>().GetInfo();
		}
		if (IsA<UEObjectProperty>())
		{
			return Cast<UEObjectProperty>().GetInfo();
		}
		if (IsA<UEClassProperty>())
		{
			return Cast<UEClassProperty>().GetInfo();
		}
		if (IsA<UEInterfaceProperty>())
		{
			return Cast<UEInterfaceProperty>().GetInfo();
		}
		if (IsA<UEWeakObjectProperty>())
		{
			return Cast<UEWeakObjectProperty>().GetInfo();
		}
		if (IsA<UELazyObjectProperty>())
		{
			return Cast<UELazyObjectProperty>().GetInfo();
		}
		if (IsA<UEAssetObjectProperty>())
		{
			return Cast<UEAssetObjectProperty>().GetInfo();
		}
		if (IsA<UEAssetClassProperty>())
		{
			return Cast<UEAssetClassProperty>().GetInfo();
		}
		if (IsA<UENameProperty>())
		{
			return Cast<UENameProperty>().GetInfo();
		}
		if (IsA<UEStructProperty>())
		{
			return Cast<UEStructProperty>().GetInfo();
		}
		if (IsA<UEStrProperty>())
		{
			return Cast<UEStrProperty>().GetInfo();
		}
		if (IsA<UETextProperty>())
		{
			return Cast<UETextProperty>().GetInfo();
		}
		if (IsA<UEArrayProperty>())
		{
			return Cast<UEArrayProperty>().GetInfo();
		}
		if (IsA<UEMapProperty>())
		{
			return Cast<UEMapProperty>().GetInfo();
		}
		if (IsA<UEDelegateProperty>())
		{
			return Cast<UEDelegateProperty>().GetInfo();
		}
		if (IsA<UEMulticastDelegateProperty>())
		{
			return Cast<UEMulticastDelegateProperty>().GetInfo();
		}
		if (IsA<UEEnumProperty>())
		{
			return Cast<UEEnumProperty>().GetInfo();
		}
	}
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