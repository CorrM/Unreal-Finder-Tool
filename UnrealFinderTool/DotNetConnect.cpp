#include "pch.h"
#include "DotNetConnect.h"
#include "Utils.h"

DotNetConnect::DotNetConnect(): lib(nullptr), freed(false)
{
}

DotNetConnect::~DotNetConnect()
{
	if (!freed)
		FreeLibrary(lib);
}

bool DotNetConnect::Load(const std::wstring& dllPath)
{
	if (lib || !Utils::FileExists(dllPath)) return false;
	this->dllPath = dllPath;

	lib = LoadLibraryW(dllPath.c_str());
	return lib != nullptr;
}

void DotNetConnect::Free()
{
	if (!freed)
		FreeLibrary(lib);
	lib = nullptr;
}
