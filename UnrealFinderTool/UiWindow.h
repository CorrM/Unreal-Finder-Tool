#pragma once
#include <d3d11.h>
#include <thread>
#include <functional>

#include "ImGUI/imgui.h"
#include "ImGUI/imgui_internal.h"

#define DIRECTINPUT_VERSION 0x0800

class UiWindow;
namespace ui = ImGui;
using UiFunc = std::function<void(UiWindow*)>;

struct WindowSettings
{
	// Window
	std::string Title;
	std::string ClassName;
	int Width;
	int Height;

	// Ui
	ImVec4 ClearColor;
	bool IsOpen;
};

class UiWindow
{
	HWND hWindow;
	WNDCLASSEX wc;
	bool closed, render;
	UiFunc uiFunc;
	HANDLE loopThreadHandle;
	std::thread loopThread;
	WindowSettings settings;
	ImGuiStyle* uiStyle;

	static ID3D11Device* gPd3dDevice;
	static ID3D11DeviceContext* gPd3dDeviceContext;
	static IDXGISwapChain* gPSwapChain;
	static ID3D11RenderTargetView* gMainRenderTargetView;

	bool CreateUiWindow(std::string& title, std::string& className, int width, int height);
	bool CreateDeviceD3D(HWND hWnd);
	void CreateRenderTarget();
	void SetupImGui();
	void CleanupDeviceD3D();
	void CleanupRenderTarget();
	void WinLoop();
	void RenderFrame();
	void SetStyle();


	static LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
public:
	UiWindow(const char* title, const char* className, int width, int height);
	~UiWindow();
	void Show(UiFunc uiForm);
	bool Closed();
	void CenterPos();
	void SetSize(int newWidth, int newHeight);
	ImVec2 GetSize();
	ImGuiStyle& GetUiStyle();

	HWND GetWindowHandle();
	void FlashWindow();
};

