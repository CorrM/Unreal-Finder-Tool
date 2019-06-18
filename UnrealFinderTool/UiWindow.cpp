#include "pch.h"
#include "UiWindow.h"
#include "ImGUI/imgui_internal.h"
#include "ImGUI/imgui_impl_win32.h"
#include "ImGUI/imgui_impl_dx11.h"
#include "IconsFontAwesome.h"

#include <tchar.h>
#include "resource.h"
#include <functional>
#include <utility>

ID3D11Device* UiWindow::gPd3dDevice = nullptr;
ID3D11DeviceContext* UiWindow::gPd3dDeviceContext = nullptr;
IDXGISwapChain* UiWindow::gPSwapChain = nullptr;
ID3D11RenderTargetView* UiWindow::gMainRenderTargetView = nullptr;

// Win32 message handler
extern LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

LRESULT WINAPI UiWindow::WndProc(const HWND hWnd, const UINT msg, const WPARAM wParam, const LPARAM lParam)
{
	if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam))
		return true;

	// Get UiWindow Pointer
	RECT rect;
	auto pUiWindow = reinterpret_cast<UiWindow*>(GetWindowLongPtr(hWnd, GWLP_USERDATA));
	if (pUiWindow == nullptr)
		return DefWindowProc(hWnd, msg, wParam, lParam);

	switch (msg)
	{
	case WM_SIZE:
		if (wParam == SIZE_MINIMIZED)
			pUiWindow->render = false;
		if (gPd3dDevice != nullptr && wParam != SIZE_MINIMIZED)
		{
			pUiWindow->render = true;
			pUiWindow->CleanupRenderTarget();
			gPSwapChain->ResizeBuffers(0, static_cast<UINT>(LOWORD(lParam)), static_cast<UINT>(HIWORD(lParam)), DXGI_FORMAT_UNKNOWN, 0);
			pUiWindow->CreateRenderTarget();
		}
		if (GetWindowRect(pUiWindow->hWindow, &rect))
		{
			pUiWindow->settings.Width = rect.right - rect.left;
			pUiWindow->settings.Height = rect.bottom - rect.top;
		}
		return 0;

	case WM_SYSCOMMAND:
		if ((wParam & 0xfff0) == SC_KEYMENU) // Disable ALT application menu
			return 0;
		break;

	case WM_DESTROY:
		PostQuitMessage(0);
		return 0;
	}
	return DefWindowProc(hWnd, msg, wParam, lParam);
}

UiWindow::UiWindow(const char* title, const char* className, const int width, const int height) :
	hWindow(nullptr),
	wc(), closed(false), render(true),
	loopThreadHandle(nullptr),
	uiStyle(nullptr)
{
	settings.Title = title;
	settings.ClassName = className;
	settings.Width = width;
	settings.Height = height;
}

UiWindow::~UiWindow()
{
	WaitForSingleObject(loopThreadHandle, 0);
}

void UiWindow::Show(UiFunc uiForm)
{
	// Start thread, create window and show it
	if (hWindow == nullptr)
	{
		uiFunc = std::move(uiForm);
		loopThread = std::thread(&UiWindow::WinLoop, this);
		auto ht = static_cast<HANDLE>(loopThread.native_handle());
		SetThreadPriority(ht, THREAD_PRIORITY_ABOVE_NORMAL);
		loopThreadHandle = ht;
		loopThread.detach();
	}
}

bool UiWindow::Closed()
{
	return closed;
}

void UiWindow::CenterPos()
{
	RECT rectScreen;
	HWND hwndScreen = GetDesktopWindow();
	GetWindowRect(hwndScreen, &rectScreen);

	int wPosX = ((rectScreen.right - rectScreen.left) / 2 - settings.Width / 2);
	int wPosY = ((rectScreen.bottom - rectScreen.top) / 2 - settings.Height / 2);
	SetWindowPos(hWindow, nullptr, wPosX, wPosY, settings.Width, settings.Height, SWP_SHOWWINDOW | SWP_NOSIZE);
}

void UiWindow::SetSize(const int newWidth, const int newHeight)
{
	settings.Width = newWidth;
	settings.Height = newHeight;
	SetWindowPos(hWindow, nullptr, 0, 0, settings.Width, settings.Height, SWP_SHOWWINDOW | SWP_NOMOVE | SWP_NOREDRAW);
}

ImVec2 UiWindow::GetSize()
{
	return { static_cast<float>(settings.Width), static_cast<float>(settings.Height) };
}

bool UiWindow::CreateUiWindow(std::string& title, std::string& className, const int width, const int height)
{
	// Create application window
	wc = { sizeof(WNDCLASSEX), CS_CLASSDC, &UiWindow::WndProc, 0L, 0L, GetModuleHandle(nullptr), nullptr, nullptr, nullptr, nullptr, className.c_str(), nullptr };
	RegisterClassEx(&wc);
	hWindow = CreateWindow(wc.lpszClassName, title.c_str(), WS_OVERLAPPED | WS_SYSMENU  | WS_MINIMIZEBOX , 100, 100, width, height, NULL, NULL, wc.hInstance, NULL);
	if (!hWindow) {
		UnregisterClass(wc.lpszClassName, wc.hInstance);
		MessageBox(HWND_DESKTOP, "Unable to create main window!", "CorrM Finder", MB_ICONERROR | MB_OK);
		return false;
	}

	HINSTANCE hInstance = wc.hInstance;
	HICON hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_ICON1));
	_ASSERTE(hIcon != nullptr);
	SendMessage(hWindow, WM_SETICON, ICON_BIG, reinterpret_cast<LPARAM>(hIcon));

	// Initialize Direct3D
	if (!CreateDeviceD3D(hWindow))
	{
		CleanupDeviceD3D();
		UnregisterClass(wc.lpszClassName, wc.hInstance);
		return false;
	}

	// Trick to get the class inside static func
	SetWindowLongPtr(hWindow, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(this));

	// Show the window
	ShowWindow(hWindow, SW_SHOWDEFAULT);
	UpdateWindow(hWindow);

	CenterPos();
	return true;
}

bool UiWindow::CreateDeviceD3D(const HWND hWnd)
{
	// Setup swap chain
	DXGI_SWAP_CHAIN_DESC sd;
	ZeroMemory(&sd, sizeof(sd));
	sd.BufferCount = 2;
	sd.BufferDesc.Width = 0;
	sd.BufferDesc.Height = 0;
	sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	sd.BufferDesc.RefreshRate.Numerator = 60;
	sd.BufferDesc.RefreshRate.Denominator = 1;
	sd.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
	sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	sd.OutputWindow = hWnd;
	sd.SampleDesc.Count = 1;
	sd.SampleDesc.Quality = 0;
	sd.Windowed = TRUE;
	sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

	UINT createDeviceFlags = 0;
	//createDeviceFlags |= D3D11_CREATE_DEVICE_DEBUG;
	D3D_FEATURE_LEVEL featureLevel;
	const D3D_FEATURE_LEVEL featureLevelArray[2] = { D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_0, };
	if (D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, createDeviceFlags, featureLevelArray, 2, D3D11_SDK_VERSION, &sd, &gPSwapChain, &gPd3dDevice, &featureLevel, &gPd3dDeviceContext) != S_OK)
		return false;

	CreateRenderTarget();
	return true;
}

void UiWindow::CreateRenderTarget()
{
	ID3D11Texture2D* pBackBuffer;
	gPSwapChain->GetBuffer(0, IID_PPV_ARGS(&pBackBuffer));
	gPd3dDevice->CreateRenderTargetView(pBackBuffer, nullptr, &gMainRenderTargetView);
	pBackBuffer->Release();
}

void UiWindow::SetupImGui()
{
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ui::GetIO();
	uiStyle = &ui::GetStyle();
	SetStyle();

	// Setup Platform/Renderer bindings
	ImGui_ImplWin32_Init(hWindow);
	ImGui_ImplDX11_Init(gPd3dDevice, gPd3dDeviceContext);

	// merge in icons from Font Awesome
	io.Fonts->AddFontDefault();
	static const ImWchar icons_ranges[] = { ICON_MIN_FA, ICON_MAX_FA, 0 };
	ImFontConfig icons_config;
	icons_config.MergeMode = true; icons_config.PixelSnapH = true;
	auto fontAwesome = io.Fonts->AddFontFromFileTTF(FONT_ICON_FILE_NAME_FAS, 11.0f, &icons_config, icons_ranges);

	if (fontAwesome == nullptr)
	{
		std::string msg = "Unable to load " + std::string(FONT_ICON_FILE_NAME_FAS) + ".";
		ShowWindow(hWindow, SW_HIDE);
		MessageBox(HWND_DESKTOP, msg.c_str(), "Error!", MB_OK | MB_ICONERROR);
		PostQuitMessage(-1);
	}
	settings.ClearColor = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);
}

void UiWindow::CleanupDeviceD3D()
{
	CleanupRenderTarget();
	if (gPSwapChain) { gPSwapChain->Release(); gPSwapChain = nullptr; }
	if (gPd3dDeviceContext) { gPd3dDeviceContext->Release(); gPd3dDeviceContext = nullptr; }
	if (gPd3dDevice) { gPd3dDevice->Release(); gPd3dDevice = nullptr; }
}

void UiWindow::CleanupRenderTarget()
{
	if (gMainRenderTargetView) { gMainRenderTargetView->Release(); gMainRenderTargetView = nullptr; }
}

void UiWindow::WinLoop()
{
	CreateUiWindow(settings.Title, settings.ClassName, settings.Width, settings.Height);
	SetupImGui();

	// Main loop
	MSG msg;
	ZeroMemory(&msg, sizeof(msg));
	while (msg.message != WM_QUIT)
	{
		if (PeekMessage(&msg, nullptr, 0U, 0U, PM_REMOVE))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
			continue;
		}

		if (render)
			RenderFrame();
	}

	ImGui_ImplDX11_Shutdown();
	ImGui_ImplWin32_Shutdown();
	ImGui::DestroyContext();

	CleanupDeviceD3D();
	DestroyWindow(hWindow);
	UnregisterClass(wc.lpszClassName, wc.hInstance);
	closed = true;
}

void UiWindow::RenderFrame()
{
	// Start the Dear ImGui frame
	ImGui_ImplDX11_NewFrame();
	ImGui_ImplWin32_NewFrame();
	ImGui::NewFrame();

	ImGui::SetNextWindowPos({ 0, 0 });
	ImGui::SetNextWindowSize({ static_cast<float>(settings.Width - 15), static_cast<float>(settings.Height - 38) });
	ImGui::SetNextWindowContentSize({ static_cast<float>(settings.Width - 15), static_cast<float>(settings.Height - 38) });
	ImGui::Begin("Main", &settings.IsOpen, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
	uiFunc(this);
	ImGui::End();

	// Rendering
	ImGui::Render();
	gPd3dDeviceContext->OMSetRenderTargets(1, &gMainRenderTargetView, nullptr);
	gPd3dDeviceContext->ClearRenderTargetView(gMainRenderTargetView, reinterpret_cast<float*>(&settings.ClearColor));
	ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

	gPSwapChain->Present(1, 0); // Present with vsync
	//g_pSwapChain->Present(0, 0); // Present without vsync
}

void UiWindow::SetStyle()
{
	ImGuiStyle* style = &ImGui::GetStyle();

	style->WindowPadding = ImVec2(7, 7);
	style->WindowRounding = 0.0f;
	style->FramePadding = ImVec2(5, 5);
	style->FrameRounding = 4.0f;
	style->ItemSpacing = ImVec2(12, 8);
	style->ItemInnerSpacing = ImVec2(8, 6);
	style->IndentSpacing = 25.0f;
	style->ScrollbarSize = 15.0f;
	style->ScrollbarRounding = 9.0f;
	style->GrabMinSize = 5.0f;
	style->GrabRounding = 3.0f;
	style->WindowBorderSize = 0.0f;

	style->Colors[ImGuiCol_Text] = ImVec4(0.80f, 0.80f, 0.83f, 1.00f);
	style->Colors[ImGuiCol_TextDisabled] = ImVec4(0.24f, 0.23f, 0.29f, 1.00f);
	style->Colors[ImGuiCol_WindowBg] = ImVec4(0.06f, 0.05f, 0.07f, 1.00f);
	style->Colors[ImGuiCol_ChildWindowBg] = ImVec4(0.07f, 0.07f, 0.09f, 1.00f);
	style->Colors[ImGuiCol_PopupBg] = ImVec4(0.07f, 0.07f, 0.09f, 1.00f);
	style->Colors[ImGuiCol_Border] = ImVec4(0.80f, 0.80f, 0.83f, 0.88f);
	style->Colors[ImGuiCol_BorderShadow] = ImVec4(0.92f, 0.91f, 0.88f, 0.00f);
	style->Colors[ImGuiCol_FrameBg] = ImVec4(0.10f, 0.09f, 0.12f, 1.00f);
	style->Colors[ImGuiCol_FrameBgHovered] = ImVec4(0.24f, 0.23f, 0.29f, 1.00f);
	style->Colors[ImGuiCol_FrameBgActive] = ImVec4(0.56f, 0.56f, 0.58f, 1.00f);
	style->Colors[ImGuiCol_TitleBg] = ImVec4(0.10f, 0.09f, 0.12f, 1.00f);
	style->Colors[ImGuiCol_TitleBgCollapsed] = ImVec4(1.00f, 0.98f, 0.95f, 0.75f);
	style->Colors[ImGuiCol_TitleBgActive] = ImVec4(0.07f, 0.07f, 0.09f, 1.00f);
	style->Colors[ImGuiCol_MenuBarBg] = ImVec4(0.10f, 0.09f, 0.12f, 1.00f);
	style->Colors[ImGuiCol_ScrollbarBg] = ImVec4(0.10f, 0.09f, 0.12f, 1.00f);
	style->Colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.80f, 0.80f, 0.83f, 0.31f);
	style->Colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.56f, 0.56f, 0.58f, 1.00f);
	style->Colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.06f, 0.05f, 0.07f, 1.00f);
	style->Colors[ImGuiCol_PopupBg] = ImVec4(0.19f, 0.18f, 0.21f, 1.00f);
	style->Colors[ImGuiCol_CheckMark] = ImVec4(0.80f, 0.80f, 0.83f, 0.31f);
	style->Colors[ImGuiCol_SliderGrab] = ImVec4(0.80f, 0.80f, 0.83f, 0.31f);
	style->Colors[ImGuiCol_SliderGrabActive] = ImVec4(0.06f, 0.05f, 0.07f, 1.00f);
	style->Colors[ImGuiCol_Button] = ImVec4(0.10f, 0.09f, 0.12f, 1.00f);
	style->Colors[ImGuiCol_ButtonHovered] = ImVec4(0.24f, 0.23f, 0.29f, 1.00f);
	style->Colors[ImGuiCol_ButtonActive] = ImVec4(0.56f, 0.56f, 0.58f, 1.00f);

	style->Colors[ImGuiCol_Header] = ImVec4(0.10f, 0.14f, 0.12f, 1.00f);
	style->Colors[ImGuiCol_HeaderHovered] = ImVec4(0.44f, 0.50f, 0.57f, 1.00f);
	style->Colors[ImGuiCol_HeaderActive] = ImVec4(0.06f, 0.05f, 0.07f, 1.00f);

	style->Colors[ImGuiCol_Column] = ImVec4(0.56f, 0.56f, 0.58f, 1.00f);
	style->Colors[ImGuiCol_ColumnHovered] = ImVec4(0.24f, 0.23f, 0.29f, 1.00f);
	style->Colors[ImGuiCol_ColumnActive] = ImVec4(0.56f, 0.56f, 0.58f, 1.00f);
	style->Colors[ImGuiCol_ResizeGrip] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
	style->Colors[ImGuiCol_ResizeGripHovered] = ImVec4(0.56f, 0.56f, 0.58f, 1.00f);
	style->Colors[ImGuiCol_ResizeGripActive] = ImVec4(0.06f, 0.05f, 0.07f, 1.00f);
	style->Colors[ImGuiCol_PlotLines] = ImVec4(0.40f, 0.39f, 0.38f, 0.63f);
	style->Colors[ImGuiCol_PlotLinesHovered] = ImVec4(0.25f, 1.00f, 0.00f, 1.00f);
	style->Colors[ImGuiCol_PlotHistogram] = ImVec4(0.40f, 0.39f, 0.38f, 0.63f);
	style->Colors[ImGuiCol_PlotHistogramHovered] = ImVec4(0.25f, 1.00f, 0.00f, 1.00f);
	style->Colors[ImGuiCol_TextSelectedBg] = ImVec4(0.25f, 1.00f, 0.00f, 0.43f);
	style->Colors[ImGuiCol_ModalWindowDarkening] = ImVec4(1.00f, 0.98f, 0.95f, 0.73f);
	style->Colors[ImGuiCol_Tab] = ImVec4(0.17f, 0.24f, 0.31f, 1.00f);
	style->Colors[ImGuiCol_TabActive] = ImVec4(0.90f, 0.29f, 0.23f, 1.00f);
	style->Colors[ImGuiCol_TabHovered] = ImVec4(0.16f, 0.50f, 0.72f, 1.00f);

	/*
	io.Fonts->AddFontFromFileTTF("C:\\Windows\\Fonts\\Ruda-Bold.ttf", 12);
	io.Fonts->AddFontFromFileTTF("C:\\Windows\\Fonts\\Ruda-Bold.ttf", 10);
	io.Fonts->AddFontFromFileTTF("C:\\Windows\\Fonts\\Ruda-Bold.ttf", 14);
	io.Fonts->AddFontFromFileTTF("C:\\Windows\\Fonts\\Ruda-Bold.ttf", 18);
	*/
}

ImGuiStyle& UiWindow::GetUiStyle()
{
	return *uiStyle;
}

HWND UiWindow::GetWindowHandle()
{
	return hWindow;
}

void UiWindow::FlashWindow()
{
	::FlashWindow(hWindow, TRUE);
}