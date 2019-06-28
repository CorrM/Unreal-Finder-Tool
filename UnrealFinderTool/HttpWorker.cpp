#include "pch.h"
#include <winhttp.h>
#include <map>
#include "HttpWorker.h"

pplx::task<http_response> HttpWorker::Get(const std::wstring& url, const bool autoRedirect, std::wstring cookies)
{
	http_client_config config;
	if (!autoRedirect)
	{
		config.set_nativehandle_options([](const native_handle handle)
		{
			// Disable auto redirects
			DWORD dwOptionValue = WINHTTP_DISABLE_REDIRECTS;
			WinHttpSetOption(handle, WINHTTP_OPTION_DISABLE_FEATURE, &dwOptionValue, sizeof dwOptionValue);
		});
	}

	http_request httpRequest;
	httpRequest.set_method(methods::GET);

	httpRequest.headers().add(L"User-Agent", L"Mozilla/5.0 (Windows NT 10.0; Win64; x64; rv:67.0) Gecko/20100101 Firefox/67.0");
	httpRequest.headers().add(L"Accept", L"*/*");
	if (!cookies.empty())
		httpRequest.headers().add(L"Cookie", cookies);

	http_client client(url, config);
	pplx::task<http_response> resp = client.request(httpRequest);

	return resp;
}

pplx::task<http_response> HttpWorker::Post(const std::wstring& url, const std::map<std::wstring, std::wstring>& params)
{
	std::wstring postParametersWString;
	for (auto& param : params)
		postParametersWString += param.first + L"=" + param.second + L"&";

	http_request postTestRequest;
	postTestRequest.set_method(methods::POST);
	postTestRequest.set_body(postParametersWString, L"application/x-www-form-urlencoded");

	http_client client(url);
	pplx::task<http_response> resp = client.request(postTestRequest);

	return resp;
}
