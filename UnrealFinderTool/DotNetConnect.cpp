#include "pch.h"
#include "DotNetConnect.h"
#include "Utils.h"

DotNetConnect::DotNetConnect(const std::wstring& dllPath): lib(nullptr), freed(false)
{
	this->dllPath = dllPath;
}

DotNetConnect::~DotNetConnect()
{
	if (!freed)
		FreeLibrary(lib);
}

bool DotNetConnect::Load()
{
	if (lib || !Utils::FileExists(dllPath)) return false;
	lib = LoadLibraryW(dllPath.c_str());

	return lib != nullptr;
}

void DotNetConnect::Free()
{
	if (!freed)
		FreeLibrary(lib);
}
