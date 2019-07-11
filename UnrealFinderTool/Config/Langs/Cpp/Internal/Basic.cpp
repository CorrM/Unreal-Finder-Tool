/*!!INCLUDE!!*/
/*!!DEFINE!!*/
TNameEntryArray* FName::GNames = nullptr;
FUObjectArray* UObject::GObjects = nullptr;

//---------------------------------------------------------------------------
void InitSdk(const std::string& moduleName, const size_t gObjectsOffset, const size_t gNamesOffset)
{
	auto mBaseAddress = reinterpret_cast<uintptr_t>(GetModuleHandleA(moduleName.c_str()));

	UObject::GObjects = reinterpret_cast<SDK::FUObjectArray*>(mBaseAddress + gObjectsOffset - 0x10);
	FName::GNames = reinterpret_cast<SDK::TNameEntryArray*>(mBaseAddress + gNamesOffset);
}
void InitSdk()
{
	/*!!AUTO_INIT_SDK!!*/
}
//---------------------------------------------------------------------------
bool FWeakObjectPtr::IsValid() const
{
	if (ObjectSerialNumber == 0)
	{
		return false;
	}
	if (ObjectIndex < 0)
	{
		return false;
	}
	auto ObjectItem = UObject::GetGlobalObjects().GetItemByIndex(ObjectIndex);
	if (!ObjectItem)
	{
		return false;
	}
	if (!SerialNumbersMatch(ObjectItem))
	{
		return false;
	}
	return !(ObjectItem->IsUnreachable() || ObjectItem->IsPendingKill());
}
//---------------------------------------------------------------------------
UObject* FWeakObjectPtr::Get() const
{
	if (IsValid())
	{
		auto ObjectItem = UObject::GetGlobalObjects().GetItemByIndex(ObjectIndex);
		if (ObjectItem)
		{
			return ObjectItem->Object;
		}
	}
	return nullptr;
}
//---------------------------------------------------------------------------

/*!!FOOTER!!*/
