#include "pch.h"
#include "Color.h"
#include "GnamesFinder.h"
#include "GObjectsFinder.h"
#include "InstanceLogger.h"
#include "SdkGenerator.h"
#include "UiWindow.h"
#include "Utils.h"
#include "ImGUI/imgui_internal.h"
#include "ImControl.h"

#include <sstream>

bool memory_init = false;

bool IsValidProcess(const int p_id, HANDLE& pHandle)
{
	DWORD exitCode;
	pHandle = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, false, p_id);
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
	std::thread t([=]()
	{
		DisabledAll();
		g_objects_find_disabled = true;

		GObjectsFinder taf(easyMethod);
		std::vector<uintptr_t> ret = taf.Find();
		obj_listbox_items.clear();

		for (auto v : ret)
		{
			std::stringstream ss; ss << std::hex << v;

			std::string tmpUpper = ss.str();
			std::transform(tmpUpper.begin(), tmpUpper.end(), tmpUpper.begin(), toupper);

			obj_listbox_items.push_back(tmpUpper);
		}

		if (ret.size() == 1)
			strcpy_s(g_objects_buf, sizeof g_objects_buf, obj_listbox_items[0].data());

		g_objects_find_disabled = false;
		EnabledAll();
	});
	t.detach();
}

void StartGNamesFinder()
{
	std::thread t([&]()
	{
		DisabledAll();
		g_names_find_disabled = true;

		GNamesFinder gf;
		std::vector<uintptr_t> ret = gf.Find();

		for (auto v : ret)
		{
			std::stringstream ss; ss << std::hex << v;

			std::string tmpUpper = ss.str();
			std::transform(tmpUpper.begin(), tmpUpper.end(), tmpUpper.begin(), toupper);

			names_listbox_items.push_back(tmpUpper);
		}

		if (ret.size() == 1)
			strcpy_s(g_names_buf, sizeof g_names_buf, names_listbox_items[0].data());

		g_names_find_disabled = false;
		EnabledAll();
	});
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

		if (retState.State == LoggerState::Good)
			il_state = "Finished.!!";
		else if (retState.State == LoggerState::BadGObject)
			il_state = "Wrong (GObjects) Address.!!";
		else if (retState.State == LoggerState::BadGName)
			il_state = "Wrong (GNames) Address.!!";

		il_objects_count = retState.GObjectsCount;
		il_names_count = retState.GNamesCount;
		EnabledAll();
	});
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
		GeneratorState ret = sg.Start(&sg_objects_count, &sg_names_count, &sg_packages_count, &sg_packages_done_count, sg_state, sg_packages_items);

		if (ret == GeneratorState::Good)
			sg_state = "Finished.!!";
		else if (ret == GeneratorState::BadGObject)
			sg_state = "Wrong (GObjects) Address.!!";
		else if (ret == GeneratorState::BadGName)
			sg_state = "Wrong (GNames) Address.!!";

		g_objects_find_disabled = false;
		g_names_find_disabled = false;
		EnabledAll();
	});
	t.detach();
}

void MainUi(UiWindow& thiz)
{
	// ui::ShowDemoWindow();
	ui::SetCursorPosX(abs(ui::CalcTextSize("Unreal Finder Tool By CorrM").x - ui::GetWindowSize().x) / 2);
	ui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "Unreal Finder Tool By CorrM");
	
	ui::Separator();

	// Process ID
	{
		ui::AlignTextToFramePadding();
		ui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "Process ID : "); ui::SameLine();
		ENABLE_DISABLE_WIDGET(ui::InputInt("##ProcessID", &process_id), process_id_disabled);
	}
	
	// Use Kernal
	{
		ui::AlignTextToFramePadding();
		ui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "Use Kernal : "); ui::SameLine();
		ENABLE_DISABLE_WIDGET(ui::Checkbox("##UseKernal", &use_kernal), use_kernal_disabled);
	}
	
	// GObjects Address
	{
		ui::AlignTextToFramePadding();
		ui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "GObjects   : "); ui::SameLine();
		ENABLE_DISABLE_WIDGET(ui::InputText("##GObjects", g_objects_buf, IM_ARRAYSIZE(g_objects_buf), ImGuiInputTextFlags_CharsHexadecimal), g_objects_disabled);
		ui::SameLine(); HelpMarker("First uObject address.\nNot gObjects pointer.\nIt's the address you get from this tool.");
		g_objects_address = Utils::CharArrayToUintptr(g_objects_buf);
	}
	
	// GNames Address
	{
		ui::AlignTextToFramePadding();
		ui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "GNames     : "); ui::SameLine();
		ENABLE_DISABLE_WIDGET(ui::InputText("##GNames", g_names_buf, IM_ARRAYSIZE(g_names_buf), ImGuiInputTextFlags_CharsHexadecimal), g_names_disabled);
		ui::SameLine(); HelpMarker("First GNames chunk address.\nNot GNames pointer.\nIt's NOT the address you get from this tool.");
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
					thiz.SetSize(400, 350);
					cur_tap_id = 1;
				}

				// ## GObjects
				{
					ui::BeginGroup();

					// Label
					ui::AlignTextToFramePadding();
					ui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "GObjects :"); ui::SameLine();

					// Open Popup / Start Finder
					ENABLE_DISABLE_WIDGET_IF(ui::Button("Find##GObjects"), g_objects_find_disabled,
					{
						if (IsReadyToGo())
							ui::OpenPopup("Easy?");
						else
							ui::OpenPopup("Warning##NotValidProcess");
					});

					// Popup
					if (ui::BeginPopupModal("Easy?", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
					{
						ui::Text("First try EASY method. not work.?\nUse HARD method and wait some time.!\nUse Easy Method .?\n\n");
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

					if (ui::Button("Use##Objects"))
					{
						if (size_t(obj_listbox_item_current) < obj_listbox_items.size())
							strcpy_s(g_objects_buf, sizeof g_objects_buf, obj_listbox_items[obj_listbox_item_current].data());
					}

					ui::PushItemWidth(ui::GetWindowSize().x / 2 - 20);
					ui::ListBox("##Obj_listbox", &obj_listbox_item_current, VectorGetter, static_cast<void*>(&obj_listbox_items), static_cast<int>(obj_listbox_items.size()), 3);
					ui::PopItemWidth();
					ui::EndGroup();
				}

				ui::SameLine();

				// ## GNames
				{
					ui::BeginGroup();
					ui::AlignTextToFramePadding();
					ui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "GNames :"); ui::SameLine();

					// Start Finder
					ENABLE_DISABLE_WIDGET_IF(ui::Button("Find##GNames"), g_names_find_disabled,
					{
						if (IsReadyToGo())
							StartGNamesFinder();
						else
							ui::OpenPopup("Warning##NotValidProcess");
					});

					ui::SameLine();

					// Set to input box
					if (ui::Button("Use##Names"))
					{
						if (size_t(names_listbox_item_current) < names_listbox_items.size())
							strcpy_s(g_names_buf, sizeof g_names_buf, names_listbox_items[names_listbox_item_current].data());
					}

					ui::PushItemWidth(ui::GetWindowSize().x / 2 - 26);
					ui::ListBox("##Names_listbox", &names_listbox_item_current, VectorGetter, static_cast<void*>(&names_listbox_items), static_cast<int>(names_listbox_items.size()), 3);
					ui::PopItemWidth();
					ui::EndGroup();
				}

				NotValidProcessPopup();
				ui::EndTabItem();
			}
			if (ui::BeginTabItem("Instance Logger"))
			{
				if (cur_tap_id != 2)
				{
					thiz.SetSize(400, 365);
					cur_tap_id = 2;
				}

				ui::AlignTextToFramePadding();
				ui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "Objects Count : "); ui::SameLine();
				ui::Text("%d", il_objects_count);

				ui::AlignTextToFramePadding();
				ui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "Names Count   : "); ui::SameLine();
				ui::Text("%d", il_names_count);

				ui::AlignTextToFramePadding();
				ui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "State         : "); ui::SameLine();
				ui::Text("%s", il_state.c_str());

				// Start Logger
				ENABLE_DISABLE_WIDGET_IF(ui::Button("Start##InstanceLogger", { 370.0f, 0.0f }), il_start_disabled,
				{
					if (IsReadyToGo())
						StartInstanceLogger();
					else
						ui::OpenPopup("Warning##NotValidProcess");
				});

				NotValidProcessPopup();
				ui::EndTabItem();
			}
			if (ui::BeginTabItem("Sdk Generator"))
			{
				if (cur_tap_id != 3)
				{
					thiz.SetSize(400, 550);
					cur_tap_id = 3;
				}

				ui::AlignTextToFramePadding();
				ui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "Objects Count  : "); ui::SameLine();
				ui::Text("%d", sg_objects_count);

				ui::AlignTextToFramePadding();
				ui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "Names Count    : "); ui::SameLine();
				ui::Text("%d", sg_names_count);

				ui::AlignTextToFramePadding();
				ui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "Packages Count : "); ui::SameLine();
				ui::Text("%d", sg_packages_count);

				ui::AlignTextToFramePadding();
				ui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "Done Packages  : "); ui::SameLine();
				ui::Text("%d", sg_packages_done_count);

				ui::AlignTextToFramePadding();
				ui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "State          : "); ui::SameLine();
				ui::Text("%s", sg_state.c_str());

				// Packages Box
				ui::PushItemWidth(ui::GetWindowSize().x - 30);
				ui::ListBox("##Packages_listbox", &sg_packages_item_current, VectorGetter, static_cast<void*>(&sg_packages_items), static_cast<int>(sg_packages_items.size()), 5);
				ui::PopItemWidth();

				// Start Logger
				ENABLE_DISABLE_WIDGET_IF(ui::Button("Start##SdkGenerator", { 370.0f, 0.0f }), sg_start_disabled,
				{
					if (IsReadyToGo())
						StartSdkGenerator();
					else
						ui::OpenPopup("Warning##NotValidProcess");
				});

				NotValidProcessPopup();
				ui::EndTabItem();
			}

			ui::EndTabBar();
		}
		
	}
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
	// Remove unneeded variables
	UNREFERENCED_PARAMETER(hPrevInstance);
	UNREFERENCED_PARAMETER(lpCmdLine);

	// Load Settings / Json Core
	if (!Utils::LoadSettings()) return 0;
	if (!Utils::LoadJsonCore()) return 0;

	UiWindow ui("Unreal Finder Tool. ver: 2.0.0", "CorrMFinder", 400, 350);
	ui.Show(MainUi);

	while (!ui.Closed())
		Sleep(1);

	if (Utils::MemoryObj != nullptr)
	{
		CloseHandle(Utils::MemoryObj->ProcessHandle);
		delete Utils::MemoryObj;
	}

	return ERROR_SUCCESS;
}
