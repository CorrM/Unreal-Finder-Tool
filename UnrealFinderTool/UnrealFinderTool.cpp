#include "pch.h"
#include "GnamesFinder.h"
#include "GObjectsFinder.h"
#include "ClassFinder.h"
#include "InstanceLogger.h"
#include "SdkGenerator.h"
#include "UiWindow.h"
#include "ImGUI/imgui_internal.h"
#include "ImControl.h"

#include "Debug.h"

#include <sstream>

#define UNREAL_WINDOW_CLASS "UnrealWindow"

UiWindow* UiMainWindow = nullptr;
bool memory_init = false;

int DetectUe4Game(HWND* windowHandle)
{
	HWND childControl = FindWindow(UNREAL_WINDOW_CLASS, nullptr);
	if (childControl != nullptr)
	{
		DWORD pId;
		GetWindowThreadProcessId(childControl, &pId);

		if (Memory::GetProcessNameById(pId) == "EpicGamesLauncher.exe")
			return 0;

		if (windowHandle != nullptr)
			*windowHandle = childControl;

		return pId;
	}

	return 0;
}

int DetectUe4Game()
{
	return DetectUe4Game(nullptr);
}

bool IsValidProcess(const int p_id, HANDLE& pHandle)
{
	DWORD exitCode;
	pHandle = OpenProcess(PROCESS_ALL_ACCESS, false, p_id);
	return p_id != 0 && GetExitCodeProcess(pHandle, &exitCode) != FALSE && exitCode == STILL_ACTIVE;
}

void SetupMemoryStuff(const HANDLE pHandle)
{
	// Setup Memory Stuff
	if (!memory_init)
	{
		memory_init = true;
		Utils::MemoryObj = new Memory(pHandle, use_kernal);
		if (!use_kernal) Utils::MemoryObj->GetDebugPrivileges();

		// Grab engine version information
		Utils::UnrealEngineVersion(ue_version);

		// Override UE4 Engine Structs
		Utils::OverrideLoadedEngineCore(ue_version);
	}
}

bool IsReadyToGo()
{
	HANDLE pHandle;
	if (IsValidProcess(process_id, pHandle))
	{
		SetupMemoryStuff(pHandle);
		return true;
	}
	return false;
}

void StartGObjFinder(const bool easyMethod)
{
	g_obj_listbox_items.clear();
	std::thread t([=]()
	{
		DisabledAll();
		g_objects_find_disabled = true;
		g_objects_disabled = false;
		g_names_disabled = false;

		GObjectsFinder taf(easyMethod);
		std::vector<uintptr_t> ret = taf.Find();
		g_obj_listbox_items.clear();

		for (auto v : ret)
		{
			std::stringstream ss;
			ss << std::hex << v;

			std::string tmpUpper = ss.str();
			std::transform(tmpUpper.begin(), tmpUpper.end(), tmpUpper.begin(), toupper);

			g_obj_listbox_items.push_back(tmpUpper);
		}

		if (ret.size() == 1)
			strcpy_s(g_objects_buf, sizeof g_objects_buf, g_obj_listbox_items[0].data());

		g_objects_find_disabled = false;
		EnabledAll();
	});
	auto ht = static_cast<HANDLE>(t.native_handle());
	SetThreadPriority(ht, THREAD_PRIORITY_IDLE);
	t.detach();
}

/*
*	TODO: BUG
*
*	Definitely some thread safety issues happening here
*	will need to look into more.
*
*	Turns out g_names_find_disabled causes imgui to crash.
*/
void StartGNamesFinder()
{
	std::string s = "Searching...";
	g_names_listbox_items.push_back(s);

	DisabledAll();
	//	g_names_find_disabled = true;
	g_objects_disabled = false;
	g_names_disabled = false;

	std::thread t([&]()
	{
		GNamesFinder gf;
		std::vector<uintptr_t> ret = gf.Find();

		auto found = false;

		uintptr_t end = ret[0];
		uintptr_t start2 = ret[0];
		uintptr_t start = (end - 0x17FFFF0); //0xA0000
		uintptr_t end2 = (start2 + 0x17FFFF0);

		auto step = Utils::PointerSize();

		while (!found)
		{
			if (!Utils::IsValidGNamesAddress(start))
				start += step;
			else
			{
				ret[0] = start;
				found = true;
			}
			if (!Utils::IsValidGNamesAddress(start2))
				start2 += step;
			else
			{
				ret[0] = start2;
				found = true;
			}
			if (!Utils::IsValidGNamesAddress(start2))
				start2 += step;
			else
			{
				ret[0] = start2;
				found = true;
			}

			if (start == end || start2 == end2 || found)
				break;
		}

		g_names_listbox_items.clear();

		for (auto v : ret)
		{
			std::stringstream ss;
			ss << std::hex << v;

			std::string tmpUpper = ss.str();
			std::transform(tmpUpper.begin(), tmpUpper.end(), tmpUpper.begin(), toupper);

			g_names_listbox_items.push_back(tmpUpper);
		}

		if (ret.size() == 1)
		{
			strcpy_s(g_names_buf, sizeof g_names_buf, g_names_listbox_items[0].data());
		}

		EnabledAll();
	});
	auto ht = static_cast<HANDLE>(t.native_handle());
	SetThreadPriority(ht, THREAD_PRIORITY_IDLE);
	t.detach();
}

void StartClassFinder()
{
	bool contin = false;
	// Check Address
	if (!Utils::IsValidGNamesAddress(g_names_address))
		popup_not_valid_gnames = true;
	else if (!Utils::IsValidGObjectsAddress(g_objects_address))
		popup_not_valid_gobjects = true;
	else
		contin = true;

	if (!contin || std::string(class_find_buf).empty())
		return;

	class_listbox_items.clear();
	std::thread t([&]()
	{
		DisabledAll();
		class_find_disabled = true;

		ClassFinder cf;
		class_listbox_items = cf.Find(g_objects_address, g_names_address, class_find_buf);

		class_find_disabled = false;
		EnabledAll();
	});
	auto ht = static_cast<HANDLE>(t.native_handle());
	SetThreadPriority(ht, THREAD_PRIORITY_IDLE);
	t.detach();
}

void StartInstanceLogger()
{
	std::thread t([&]()
	{
		DisabledAll();
		il_objects_count = 0;
		il_names_count = 0;
		il_state = "Running . . .";
		InstanceLogger il(g_objects_address, g_names_address);
		auto retState = il.Start();

		switch (retState.State)
		{
		case LoggerState::Good:
			il_state = "Finished.!!";
			break;
		case LoggerState::BadGObject:
		case LoggerState::BadGObjectAddress:
			il_state = "Wrong (GObjects) Address.!!";
			break;
		case LoggerState::BadGName:
		case LoggerState::BadGNameAddress:
			il_state = "Wrong (GNames) Address.!!";
			break;
		}

		il_objects_count = retState.GObjectsCount;
		il_names_count = retState.GNamesCount;
		EnabledAll();
	});
	auto ht = static_cast<HANDLE>(t.native_handle());
	SetThreadPriority(ht, THREAD_PRIORITY_IDLE);
	t.detach();
}

void StartSdkGenerator()
{
	std::thread t([&]()
	{
		DisabledAll();
		g_objects_find_disabled = true;
		g_names_find_disabled = true;

		sg_objects_count = 0;
		sg_names_count = 0;
		sg_state = "Running . . .";
		SdkGenerator sg(g_objects_address, g_names_address);
		GeneratorState ret = sg.Start(&sg_objects_count,
		                              &sg_names_count,
		                              &sg_packages_count,
		                              &sg_packages_item_current,
		                              sg_game_name_buf,
		                              std::to_string(sg_game_version[0]) + "." + std::to_string(sg_game_version[1]) +
		                              "." + std::to_string(sg_game_version[2]),
		                              static_cast<SdkType>(sg_type_item_current),
		                              sg_state, sg_packages_items);

		if (ret == GeneratorState::Good)
		{
			sg_finished = true;
			sg_state = "Finished.!!";
			UiMainWindow->FlashWindow();
		}
		else if (ret == GeneratorState::BadGObject)
			sg_state = "Wrong (GObjects) Address.!!";
		else if (ret == GeneratorState::BadGName)
			sg_state = "Wrong (GNames) Address.!!";

		g_objects_find_disabled = false;
		g_names_find_disabled = false;
		EnabledAll();
	});
	auto ht = static_cast<HANDLE>(t.native_handle());
	SetThreadPriority(ht, THREAD_PRIORITY_IDLE);
	t.detach();
}

void MainUi(UiWindow& thiz)
{
	// ui::ShowDemoWindow();

	// Settings Button
	{
		if (ui::Button(ICON_FA_COG))
			ui::OpenPopup("SettingsMenu");

		if (ui::BeginPopup("SettingsMenu"))
		{
			if (ImGui::BeginMenu("Process##menu"))
			{
				if (ui::MenuItem("Pause Process", "", &process_controller_toggles[0]))
				{
					if (!IsReadyToGo())
					{
						process_controller_toggles[0] = false;
						popup_not_valid_process = true;
					}
					else
					{
						if (process_controller_toggles[0])
							Utils::MemoryObj->SuspendProcess();
						else
							Utils::MemoryObj->ResumeProcess();
					}
				}
				ui::EndMenu();
			}
			ui::EndPopup();
		}
	}

	// Title
	{
		ui::SameLine();
		ui::SetCursorPosX(abs(ui::CalcTextSize("Unreal Finder Tool By CorrM").x - ui::GetWindowSize().x) / 2);
		ui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "Unreal Finder Tool By CorrM");
	}

	ui::Separator();

	// Process ID
	{
		ui::AlignTextToFramePadding();
		ui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "Process ID : ");
		ui::SameLine();
		ENABLE_DISABLE_WIDGET(ui::InputInt("##ProcessID", &process_id), process_id_disabled);
		ui::SameLine();

		if (ui::IsItemHovered())
		{
			if (process_id != NULL)
			{
				ui::BeginTooltip();
				ui::AlignTextToFramePadding();
				ui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "Window Title : ");
				ui::SameLine();

				// Get Window Title
				HWND window = FindWindow(UNREAL_WINDOW_CLASS, nullptr);
				char windowTitle[30] = {0};
				GetWindowText(window, windowTitle, 30);

				ui::TextUnformatted(windowTitle);
				ui::EndTooltip();
			}
		}

		ENABLE_DISABLE_WIDGET_IF(ui::Button(ICON_FA_SEARCH "##ProcessAutoDetector"), process_detector_disabled,
		                         {
			                         process_id = DetectUe4Game();
		                         });
	}

	// Unreal version
	{
		ui::AlignTextToFramePadding();
		ui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "UE Version : ");
		ui::SameLine();
		ui::LabelText("##UnrealVer", "%s", ue_version.c_str());
	}

	// Use Kernel
	{
		ui::AlignTextToFramePadding();
		ui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "Use Kernel : ");
		ui::SameLine();
		ENABLE_DISABLE_WIDGET(ui::Checkbox("##UseKernal", &use_kernal), use_kernal_disabled);
	}

	// GObjects Address
	{
		ui::AlignTextToFramePadding();
		ui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "GObjects   : ");
		ui::SameLine();
		ENABLE_DISABLE_WIDGET(
			ui::InputText("##GObjects", g_objects_buf, IM_ARRAYSIZE(g_objects_buf), ImGuiInputTextFlags_CharsHexadecimal
			), g_objects_disabled);
		ui::SameLine();
		HelpMarker(
			"What you can put here .?\n- First UObject address.\n- First GObjects chunk address.\n\n* Not GObjects pointer.\n* It's the address you get from this tool.");
		g_objects_address = Utils::CharArrayToUintptr(g_objects_buf);
	}

	// GNames Address
	{
		ui::AlignTextToFramePadding();
		ui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "GNames     : ");
		ui::SameLine();
		ENABLE_DISABLE_WIDGET(
			ui::InputText("##GNames", g_names_buf, IM_ARRAYSIZE(g_names_buf), ImGuiInputTextFlags_CharsHexadecimal),
			g_names_disabled);
		ui::SameLine();
		HelpMarker(
			"What you can put here .?\n- GNames chunk array address.\n\n* Not GNames pointer.\n* It's NOT the address you get from this tool.");
		g_names_address = Utils::CharArrayToUintptr(g_names_buf);
	}

	ui::Separator();

	// Tabs
	{
		if (ui::BeginTabBar("ToolsTabBar", ImGuiTabBarFlags_NoTooltip))
		{
			if (ui::BeginTabItem("Finder"))
			{
				if (cur_tap_id != 1)
				{
					thiz.SetSize(380, 620);
					cur_tap_id = 1;
				}

				// ## GObjects
				{
					ui::BeginGroup();

					// Label
					ui::AlignTextToFramePadding();
					ui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "GObjects :");
					ui::SameLine();

					// Open Popup / Start Finder
					ENABLE_DISABLE_WIDGET_IF(ui::Button("Find##GObjects"), g_objects_find_disabled,
					                         {
						                         if (IsReadyToGo())
						                         ui::OpenPopup("Easy?");
						                         else
						                         popup_not_valid_process = true;
					                         });

					// Popup
					if (ui::BeginPopupModal("Easy?", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
					{
						ui::Text(
							"First try EASY method. not work.?\nUse HARD method and wait some time.!\nUse Easy Method .?\n\n");
						ui::Separator();

						if (ui::Button("Yes", ImVec2(75, 0)))
						{
							ui::CloseCurrentPopup();
							StartGObjFinder(true);
						}

						ui::SetItemDefaultFocus();
						ui::SameLine();
						if (ui::Button("No", ImVec2(75, 0)))
						{
							ui::CloseCurrentPopup();
							StartGObjFinder(false);
						}

						ui::SameLine();
						if (ui::Button("Cancel", ImVec2(75, 0)))
							ui::CloseCurrentPopup();

						ui::EndPopup();
					}

					ui::SameLine();

					if (ui::Button("Use##Objects", {38.0f, 0.0f}))
					{
						if (size_t(g_obj_listbox_item_current) < g_obj_listbox_items.size())
							strcpy_s(g_objects_buf, sizeof g_objects_buf,
							         g_obj_listbox_items[g_obj_listbox_item_current].data());
					}

					ui::PushItemWidth(ui::GetWindowSize().x / 2 - 10);
					ui::ListBox("##Obj_listbox", &g_obj_listbox_item_current, VectorGetter,
					            static_cast<void*>(&g_obj_listbox_items), static_cast<int>(g_obj_listbox_items.size()),
					            3);
					ui::PopItemWidth();
					ui::EndGroup();
				}

				ui::SameLine();

				// ## GNames
				{
					ui::BeginGroup();
					ui::AlignTextToFramePadding();
					ui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "GNames :");
					ui::SameLine();

					// Start Finder
					ENABLE_DISABLE_WIDGET_IF(ui::Button("Find##GNames"), g_names_find_disabled,
					{
						if (IsReadyToGo())
							StartGNamesFinder();
						else
							popup_not_valid_process = true;
					});

					ui::SameLine();

					// Set to input box
					if (ui::Button("Use##Names", {47.0f, 0.0f}))
					{
						if (size_t(g_names_listbox_item_current) < g_names_listbox_items.
							size())
							strcpy_s(g_names_buf, sizeof g_names_buf,
							         g_names_listbox_items[g_names_listbox_item_current].
							         data());
					}

					ui::PushItemWidth(ui::GetWindowSize().x / 2 - 15);
					ui::ListBox("##Names_listbox", &g_names_listbox_item_current,
					            VectorGetter,
					            static_cast<void*>(&g_names_listbox_items),
					            static_cast<int>(g_names_listbox_items.size()), 3);
					ui::PopItemWidth();
					ui::EndGroup();
				}

				ui::Separator();
				ui::SetCursorPosX(
					abs(ui::CalcTextSize("This section need GObjects and GNames").x -
						ui::GetWindowSize().x) / 2);
				ui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f),
				                "This section need GObjects and GNames");
				ui::Separator();

				// ## Class
				{
					ui::BeginGroup();
					ui::AlignTextToFramePadding();
					ui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "Class   :");
					ui::SameLine();
					ui::PushItemWidth(ui::GetWindowWidth() / 1.52f);
					ENABLE_DISABLE_WIDGET(
						ui::InputTextWithHint("##FindClass",
							"LocalPlayer, 0x0000000000", class_find_buf, IM_ARRAYSIZE(
								class_find_buf)), class_find_input_disabled);
					ui::PopItemWidth();
					ui::SameLine();
					HelpMarker(
						"What you can put here.?\n- Class Name:\n  - LocalPlayer or ULocalPlayer.\n  - MyGameInstance_C or UMyGameInstance_C.\n  - PlayerController or APlayerController.\n\n- Instance address:\n  - 0x0000000000.\n  - 0000000000.");

					ui::AlignTextToFramePadding();
					ui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "Buttons :");
					ui::SameLine();

					// Start Finder
					ENABLE_DISABLE_WIDGET_IF(
						ui::Button(" Find Class "), class_find_disabled,
						{
							if (IsReadyToGo())
							StartClassFinder();
							else
							popup_not_valid_process = true;
						});

					ui::SameLine();

					// Copy to clipboard
					if (ui::Button(" Copy Selected "))
					{
						if (size_t(class_listbox_item_current) < class_listbox_items.
							size())
							ui::SetClipboardText(
								class_listbox_items[class_listbox_item_current].
								c_str());
					}

					ui::PushItemWidth(ui::GetWindowSize().x - 15);
					ui::ListBox("##Class_listbox", &class_listbox_item_current,
					            VectorGetter,
					            static_cast<void*>(&class_listbox_items),
					            static_cast<int>(class_listbox_items.size()),
					            5);
					ui::PopItemWidth();
					ui::EndGroup();
				}

				ui::EndTabItem();
			}
			if (ui::BeginTabItem("Instance Logger"))
			{
				if (cur_tap_id != 2)
				{
					thiz.SetSize(380, 407);
					cur_tap_id = 2;
				}

				ui::AlignTextToFramePadding();
				ui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f),
				                "Objects Count : ");
				ui::SameLine();
				ui::Text("%d", il_objects_count);

				ui::AlignTextToFramePadding();
				ui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f),
				                "Names Count   : ");
				ui::SameLine();
				ui::Text("%d", il_names_count);

				ui::AlignTextToFramePadding();
				ui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f),
				                "State         : ");
				ui::SameLine();
				ui::Text("%s", il_state.c_str());

				// Start Logger
				ENABLE_DISABLE_WIDGET_IF(
					ui::Button("Start##InstanceLogger", { ui::GetWindowSize().x -
						14.0f, 0.0f }), il_start_disabled,
					{
						if (IsReadyToGo())
						StartInstanceLogger();
						else
						popup_not_valid_process = true;
					});

				ui::EndTabItem();
			}
			if (ui::BeginTabItem("Sdk Generator"))
			{
				if (cur_tap_id != 3)
				{
					thiz.SetSize(380, 622);
					cur_tap_id = 3;
				}

				ui::AlignTextToFramePadding();
				ui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f),
				                "Objects/Names : ");
				ui::SameLine();
				ui::Text("%d / %d", sg_objects_count, sg_names_count);

				ui::AlignTextToFramePadding();
				ui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f),
				                "Packages      : ");
				ui::SameLine();
				ui::Text("%d / %d", sg_packages_done_count, sg_packages_count);

				ui::AlignTextToFramePadding();
				ui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f),
				                "Sdk Type      : ");
				ui::SameLine();
				ui::PushItemWidth(100);
				ENABLE_DISABLE_WIDGET(
					ui::Combo("##SdkType", &sg_type_item_current, VectorGetter,
						static_cast<void*>(&sg_type_items),
						static_cast<int>(sg_type_items.size()), 4),
					sg_type_disabled);
				ui::PopItemWidth();
				ui::SameLine();
				HelpMarker(
					"- Internal: Generate functions for class/struct.\n- External: Don't gen functions for class/struct,\n    But generate ReadAsMe for every class/struct.");

				ui::AlignTextToFramePadding();
				ui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f),
				                "Game Name     : ");
				ui::SameLine();
				ENABLE_DISABLE_WIDGET(
					ui::InputTextWithHint("##GameName", "PUBG, Fortnite",
						sg_game_name_buf, IM_ARRAYSIZE(
							sg_game_name_buf)), sg_game_name_disabled);

				ui::AlignTextToFramePadding();
				ui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f),
				                "Game Version  : ");
				ui::SameLine();
				ENABLE_DISABLE_WIDGET(
					ui::InputInt3("##GameVersion", sg_game_version),
					sg_game_version_disabled);

				ui::AlignTextToFramePadding();
				ui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f),
				                "State         : ");
				ui::SameLine();
				ui::Text("%s", sg_state.c_str());

				// Packages Box
				ui::PushItemWidth(ui::GetWindowSize().x - 14.0f);
				ui::ListBox("##Packages_listbox", &sg_packages_item_current,
				            VectorGetter,
				            static_cast<void*>(&sg_packages_items),
				            static_cast<int>(sg_packages_items.size()), 5);
				ui::PopItemWidth();

				// Start Generator
				ENABLE_DISABLE_WIDGET_IF(
					ui::Button("Start##SdkGenerator", { ui::GetWindowSize().x -
						14.0f, 0.0f }), sg_start_disabled,
					{
						if (IsReadyToGo())
						StartSdkGenerator();
						else
						popup_not_valid_process = true;
					});

				ui::EndTabItem();
			}

			ui::EndTabBar();
		}
	}

	// Popups
	{
		WarningPopup("Note", "Sdk Generator finished. !!", sg_finished);
		WarningPopup("Warning", "Not Valid Process ID. !!",
		             popup_not_valid_process);
		WarningPopup("Warning", "Not Valid GNames Address. !!",
		             popup_not_valid_gnames);
		WarningPopup("Warning", "Not Valid GObjects Address. !!",
		             popup_not_valid_gobjects);
	}
}

#pragma warning(disable:4996)
// int APIENTRY WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow);
// Fix vs2019 Problem [wWinMain instead of WinMain]
// ReSharper disable once CppInconsistentNaming
int APIENTRY wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
                      LPWSTR lpCmdLine, int nShowCmd)
// NOLINT(readability-non-const-parameter)
{
	// Remove unneeded variables
	UNREFERENCED_PARAMETER(hPrevInstance);
	UNREFERENCED_PARAMETER(lpCmdLine);

	// Load Settings / Json Core
	if (!Utils::LoadSettings()) return 0;
	if (!Utils::LoadEngineCore()) return 0;

	Memory::GetProcessIdByName("EpicGamesLauncher.exe");
	Memory::GetProcessNameById(95128);

	// Autodetect in case game already open
	process_id = DetectUe4Game();

	// Run the new debugging tools
	auto d = new Debugging();
	d->EnterDebugMode();

	// Launch the main window
	UiMainWindow = new UiWindow(
		"Unreal Finder Tool. Version: 3.0.0", "CorrMFinder", 380, 578);
	UiMainWindow->Show(MainUi);

	while (!UiMainWindow->Closed())
		Sleep(1);

	// Cleanup
	if (Utils::MemoryObj != nullptr)
	{
		Utils::MemoryObj->ResumeProcess();
		CloseHandle(Utils::MemoryObj->ProcessHandle);
		delete Utils::MemoryObj;
		delete d;
	}

	return ERROR_SUCCESS;
}
