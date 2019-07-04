#include "pch.h"
#include "Utils.h"
#include "DotNetConnect.h"

DotNetConnect::DotNetConnect(): lib(nullptr), freed(true)
{
}

DotNetConnect::~DotNetConnect()
{
	if (!freed)
		FreeLibrary(lib);
}

bool DotNetConnect::Loaded() const
{
	return !freed;
}

bool DotNetConnect::Load(const std::wstring& dllPath)
{
	if (lib || !Utils::FileExists(dllPath)) return false;

	// Init vars
	freed = false;
	this->dllPath = dllPath;
	lib = LoadLibraryW(dllPath.c_str());

	return lib != nullptr;
}

void DotNetConnect::Free()
{
	if (!freed)
		FreeLibrary(lib);

	freed = true;
	lib = nullptr;
}
