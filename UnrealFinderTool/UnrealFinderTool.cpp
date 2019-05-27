#include "pch.h"
#include "Color.h"
#include "GnamesFinder.h"
#include "GObjectsFinder.h"
#include "InstanceLogger.h"
#include "SdkGenerator.h"
#include "ParallelWorker.h"
#include "UiWindow.h"
#include "Utils.h"

#include <iostream>
#include "ImGUI/imgui_internal.h"

#define ENABLE_DISABLE_WIDGET(uiCode, disabledBool) if (disabledBool) { ui::PushItemFlag(ImGuiItemFlags_Disabled, true);ui::PushStyleVar(ImGuiStyleVar_Alpha, ImGui::GetStyle().Alpha * 0.5f); } (uiCode); if (disabledBool) { ImGui::PopItemFlag();ImGui::PopStyleVar(); }

bool process_id_disabled = false;
int process_id;

bool use_kernal_disabled = false;
bool use_kernal;

bool g_objects_disabled = false;
bool g_names_disabled = false;
uintptr_t g_objects, g_names;
char g_objects_buf[17] = { 0 };
char g_names_buf[17] = { 0 };

int cur_tap_id = 0;
void HelpMarker(const char* desc);

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
	// Remove unneeded variables
	UNREFERENCED_PARAMETER(hPrevInstance);
	UNREFERENCED_PARAMETER(lpCmdLine);

	UiWindow ui("Unreal Finder Tool", "CorrMFinder", 400, 350);
	ui.Show([](UiWindow& thiz)
	{
		ui::ShowDemoWindow();
		ui::SetCursorPosX(abs(ui::CalcTextSize("Unreal Finder Tool By CorrM").x - ui::GetWindowSize().x) / 2);
		ui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "Unreal Finder Tool By CorrM");

		ui::Separator();

		// Process ID
		{
			ImGui::AlignTextToFramePadding();
			ui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "Process ID : "); ui::SameLine();
			ENABLE_DISABLE_WIDGET(ui::InputInt("##ProcessID", &process_id), process_id_disabled);
		}

		// Use Kernal
		{
			ImGui::AlignTextToFramePadding();
			ui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "Use Kernal : "); ui::SameLine();
			ENABLE_DISABLE_WIDGET(ui::Checkbox("##UseKernal", &use_kernal), use_kernal_disabled);
		}

		// GObjects Address
		{
			ImGui::AlignTextToFramePadding();
			ui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "GObjects   : "); ui::SameLine();
			ENABLE_DISABLE_WIDGET(ui::InputText("##GObjects", g_objects_buf, IM_ARRAYSIZE(g_objects_buf), ImGuiInputTextFlags_CharsHexadecimal), g_objects_disabled);
			ui::SameLine(); HelpMarker("First uObject address.\nNot gObjects pointer.\nIt's the address you get from this tool.");
			g_objects = Utils::CharArrayToUintptr(g_objects_buf);
		}
		
		// GNames Address
		{
			ImGui::AlignTextToFramePadding();
			ui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "GNames     : "); ui::SameLine();
			ENABLE_DISABLE_WIDGET(ui::InputText("##GNames", g_names_buf, IM_ARRAYSIZE(g_names_buf), ImGuiInputTextFlags_CharsHexadecimal), g_names_disabled);
			ui::SameLine(); HelpMarker("First GNames chunk address.\nNot GNames pointer.\nIt's NOT the address you get from this tool.");
			g_names = Utils::CharArrayToUintptr(g_names_buf);
		}

		ui::Separator();

		if (ui::BeginTabBar("ToolsTabBar", ImGuiTabBarFlags_None))
		{
			if (ui::BeginTabItem("Finder"))
			{
				if (cur_tap_id != 1)
				{
					thiz.SetSize(400, 350);
					cur_tap_id = 1;
				}

				// GObjects
				ui::BeginGroup();
				ui::AlignTextToFramePadding();
				ui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "GObjects :"); ui::SameLine();
				ui::Button("Find"); ui::SameLine();
				ui::Button("Set");

				static std::vector<const char*> obj_listbox_items;
				static int obj_listbox_item_current = 1;

				ui::PushItemWidth(ui::GetWindowSize().x / 2 - 20);
				ui::ListBox("##Obj_listbox", &obj_listbox_item_current, obj_listbox_items.data(), static_cast<int>(obj_listbox_items.size()), 3);
				ui::PopItemWidth();
				ui::EndGroup();

				ui::SameLine();

				// GNames
				ui::BeginGroup();
				ui::AlignTextToFramePadding();
				ui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "GNames :"); ui::SameLine();
				ui::Button("Find");

				static std::vector<const char*> names_listbox_items;
				static int names_listbox_item_current = 1;
				ui::PushItemWidth(ui::GetWindowSize().x / 2 - 22);
				ui::ListBox("##Names_listbox", &names_listbox_item_current, names_listbox_items.data(), static_cast<int>(names_listbox_items.size()), 3);
				ui::PopItemWidth();
				ui::EndGroup();

				ui::EndTabItem();
			}
			if (ui::BeginTabItem("Broccoli"))
			{
				if (cur_tap_id != 2)
				{
					thiz.SetSize(400, 480);
					cur_tap_id = 2;
				}
				ui::Text("This is the Broccoli tab!\nblah blah blah blah blah");
				ui::EndTabItem();
			}
			if (ui::BeginTabItem("Cucumber"))
			{
				if (cur_tap_id != 3)
				{
					thiz.SetSize(400, 600);
					cur_tap_id = 3;
				}

				ui::Text("This is the Cucumber tab!\nblah blah blah blah blah");
				ui::EndTabItem();
			}
			ui::EndTabBar();
		}
	});

	while (!ui.Closed())
		Sleep(1);

	return ERROR_SUCCESS;
}

void HelpMarker(const char* desc)
{
	ui::TextDisabled("(?)");
	if (ui::IsItemHovered())
	{
		ui::BeginTooltip();
		ui::PushTextWrapPos(ui::GetFontSize() * 35.0f);
		ui::TextUnformatted(desc);
		ui::PopTextWrapPos();
		ui::EndTooltip();
	}
}

int main()
{
	CONSOLE_CURSOR_INFO     cursorInfo;
	const HWND console = GetConsoleWindow();
	const HANDLE out = GetStdHandle(STD_OUTPUT_HANDLE);

	// Set Console Pos/Size
	RECT r;
	GetWindowRect(console, &r);
	MoveWindow(console, r.left, r.top, 800, 400, TRUE);

	// Hide Cursor
	GetConsoleCursorInfo(out, &cursorInfo);
	cursorInfo.bVisible = false;
	SetConsoleCursorInfo(out, &cursorInfo);

	// Load Settings / Json Core
	if (!Utils::LoadSettings()) return 0;
	if (!Utils::LoadJsonCore()) return 0;

	int tool_id, p_id, old_pid = 0;

	/*
	// ##################
	HANDLE pHandle1 = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, false, 0x5ffc);
	auto memManager1 = new Memory(pHandle1, false);
	Utils::MemoryObj = memManager1;
	auto objObjectsAddress = uintptr_t(0x1DAC6520000); // 2B703210
	auto gNamesAddress = uintptr_t(0x1DAC3750080); // 2B419744

	SdkGenerator sg(objObjectsAddress, gNamesAddress);
	sg.Start();

	std::cin >> tool_id;
	return 0;
	// ##################
	*/

	RESTART:
	system("cls");
	SetConsoleTitle("Unreal Engine Finder Tool By CorrM");
	std::cout << red << "[*] " << green << "Unreal Engine Finder Tool By " << yellow << "CorrM" << std::endl << std::endl << def;

	Tools:
	std::cout << yellow << "[?] " << red << "1: " << def << "GNames Finder" << "   -  " << yellow << "Find GNamesArray in ue4 game." << std::endl << def;
	std::cout << yellow << "[?] " << red << "2: " << def << "GObject Finder" << "  -  " << yellow << "Find GObjectArray in ue4 game." << std::endl << def;
	std::cout << yellow << "[?] " << red << "3: " << def << "Instance Logger" << " -  " << yellow << "Dump all Instance in ue4 game." << std::endl << def;
	std::cout << yellow << "[?] " << red << "4: " << def << "SDK Generator" << "   -  " << yellow << "Dump class/struct/enums/method from ue4 game." << std::endl << def;

	std::cout << std::endl;
	std::cout << green << "[-] " << yellow << "Input tool ID: " << dgreen;
	std::cin >> tool_id;

	if (tool_id == 0 || tool_id > 4)
	{
		std::cout << red << "[*] " << def << "Input valid tool ID." << std::endl << def;
		std::cout << def << "===================================" << std::endl;
		goto Tools;
	}

	// Get pID
	pID:
	std::cout << green << "[-] " << yellow << "input process ID: " << dgreen;
	std::cin >> p_id;

	if (p_id == 0 && old_pid != 0)
	{
		p_id = old_pid;
	}

	HANDLE pHandle = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, false, p_id);
	DWORD exitCode;

	if (p_id == 0 && old_pid == 0 || GetExitCodeProcess(pHandle, &exitCode) == FALSE && exitCode != STILL_ACTIVE)
	{
		std::cout << red << "[*] " << def << "Input valid process ID." << std::endl << def;
		goto pID;
	}
	old_pid = p_id;

	// Use Kernal ???
	char cUseKernal;
	std::cout << green << "[-] " << yellow << "Use Kernal (Y/N): " << dgreen;
	std::cin >> cUseKernal;
	const bool bUseKernal = cUseKernal == 'Y' || cUseKernal == 'y';

	std::cout << def << "===================================" << std::endl;

	// Setup Memory Stuff
	auto memManager = new Memory(pHandle, bUseKernal);
	if (!bUseKernal) memManager->GetDebugPrivileges();

	Utils::MemoryObj = memManager;

	if (tool_id == 1) // GNames Finder
	{
		GNamesFinder gf;
		gf.Find();
	}
	else if (tool_id == 2) // GObjects Finder
	{
		std::cout << red << "[?] " << yellow << "First try EASY method. not work.? use HARD method and wait some time :D." << std::endl;
		char cUseEz;
		std::cout << green << "[-] " << yellow << "Use Easy Method (Y/N): " << dgreen;
		std::cin >> cUseEz;
		const bool bUseEz = cUseEz == 'Y' || cUseEz == 'y';

		GObjectsFinder taf(bUseEz);
		taf.Find();
	}
	else if (tool_id == 3) // Instance Logger
	{
		uintptr_t objObjectsAddress, gNamesAddress;
		std::cout << green << "[-] " << yellow << "Input (GObjects) Address: " << dgreen;
		std::cin >> std::hex >> objObjectsAddress;

		std::cout << green << "[-] " << yellow << "Input (GNames Chunk) Address: " << dgreen;
		std::cin >> std::hex >> gNamesAddress;

		std::cout << def << "===================================" << std::endl;

		InstanceLogger il(objObjectsAddress, gNamesAddress);
		il.Start();
	}
	else if (tool_id == 4) // SDK Generator
	{
		uintptr_t objObjectsAddress, gNamesAddress;
		std::cout << green << "[-] " << yellow << "Input (GObjects) Address: " << dgreen;
		std::cin >> std::hex >> objObjectsAddress;

		std::cout << green << "[-] " << yellow << "Input (GNamesArray) Address: " << dgreen;
		std::cin >> std::hex >> gNamesAddress;
		std::cout << def << "===================================" << std::endl;

		SdkGenerator sg(objObjectsAddress, gNamesAddress);
		sg.Start();
	}

	std::cout << def << "===================================" << std::endl;

	delete memManager;
	CloseHandle(pHandle);

	char cRestart;
	std::cout << yellow << "[?] " << yellow << "RESTART (Y/N): " << dgreen;
	std::cin >> cRestart;

	if (cRestart == 'Y' || cRestart == 'y')
		goto RESTART;

	return 0;
}
