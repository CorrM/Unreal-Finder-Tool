#pragma once

class DotNetConnect
{
	HMODULE lib;
	bool freed;
	std::wstring dllPath;

public:
	explicit DotNetConnect();
	~DotNetConnect();
	bool Load(const std::wstring& dllPath);
	void Free();
	template<typename Fn>
	Fn GetFunction(const std::string& funcName);
};

template <typename Fn>
Fn DotNetConnect::GetFunction(const std::string& funcName)
{
	if (!lib)
		return nullptr;

	return reinterpret_cast<Fn>(GetProcAddress(lib, funcName.c_str()));
}
