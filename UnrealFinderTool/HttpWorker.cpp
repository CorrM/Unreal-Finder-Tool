#include "pch.h"
#include <winhttp.h>
#include "HttpWorker.h"

pplx::task<http_response> HttpWorker::Get(const std::wstring& url)
{
	http_client_config config;
	config.set_nativehandle_options([](const native_handle handle)
	{
		// Disable auto redirects
		DWORD dwOptionValue = WINHTTP_DISABLE_REDIRECTS;
		WinHttpSetOption(handle, WINHTTP_OPTION_DISABLE_FEATURE, &dwOptionValue, sizeof dwOptionValue);
	});

	http_client client(url, config);
	pplx::task<http_response> resp = client.request(methods::GET);

	return resp;
}
