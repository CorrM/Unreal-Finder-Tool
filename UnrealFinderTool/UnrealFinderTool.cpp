#include "pch.h"
#include "GnamesFinder.h"
#include "GObjectsFinder.h"
#include "ClassFinder.h"
#include "InstanceLogger.h"
#include "SdkGenerator.h"

#include "UiWindow.h"
#include "ImControl.h"
#include "IconsFontAwesome.h"
#include "MemoryEditor.h"

#include "Memory.h"
#include "Debug.h"
#include "Scanner.h"
#include "Generator.h"

#include "Midi/MIDI.h"

#include <shellapi.h>

MemoryEditor mem_edit;
bool memory_init = false;
bool override_engine = false;
bool donate_show = true;
float LeftWidth, MidWidth, RightWidth;

#ifdef MIDI_h
CMIDI* MidiPlayer = nullptr;
#endif

void BeforeWork()
{
	DisabledAll();
}

void AfterWork()
{
	EnabledAll();
}

std::string GetTookTime(const std::tm take_time)
{
	return std::to_string(take_time.tm_hour) + "h " + std::to_string(take_time.tm_min) + "m " + std::to_string(take_time.tm_sec) + "s";
}

void LoadOverrideEngine()
{
	if (override_engine)
		return;

	override_engine = true;
	game_ue_disabled = true;

	// Override UE4 Engine Structs
	Utils::OverrideLoadedEngineCore(unreal_versions[ue_selected_version]);
}

void CheckLastVer()
{
	std::wstring lastVer;

	const auto requestTask = HttpWorker::Get(L"https://github.com/CorrM/Unreal-Finder-Tool/releases/latest", false)
	.then([&](http_response response)
	{
		std::wstring locationHeader = response.headers()[L"Location"];
		size_t pos;
		if ((pos = locationHeader.rfind(L'/')) != std::wstring::npos)
			lastVer.assign(locationHeader.begin() + pos + 1, locationHeader.end());
	});

	try
	{
		requestTask.wait();
		if (!lastVer.empty() && lastVer != std::wstring(U(TOOL_VERSION)))
		{
			MessageBoxW(Utils::UiMainWindow->GetWindowHandle(),
				(
					L"There is a new version of this tool.\n"s
					L"Your Version : " TOOL_VERSION + ".\n"
					L"New Version : " + lastVer + ".\n\n"
					L"Recommended to use last version.\nTo download :\nMenuButton->Help->Last version."
					).c_str(),
				L"New Version",
				MB_OK | MB_ICONINFORMATION);
		}
	}
	catch (const std::exception& e)
	{
		MessageBox(nullptr, ("Error exception: "s + e.what()).c_str(), "", MB_OK | MB_ICONERROR);
	}
}

#pragma region Patreon
pplx::task<http_response> GetGoals()
{
	auto jsonReq = HttpWorker::Get(L"https://www.patreon.com/api/campaigns/2879379");
	jsonReq.then([&](http_response& res)
	{
		// to solve json problem
		res.headers()[L"Content-Type"] = L"application/json";
		auto json = res.extract_json().get();

		auto includes = json.at(L"included").as_array();
		for (auto& item : includes)
		{
			if (item.at(L"type").as_string() != L"goal") continue;

			const int completed_percentage = item.at(L"attributes").at(L"completed_percentage").as_integer();
			std::wstring description = item.at(L"attributes").at(L"description").as_string();
			description = Utils::RemoveStringBetween(description, L"<", L">");

			Goals.push_back({ completed_percentage, description });
		}
	});

	return jsonReq;
}

pplx::task<http_response> GetLastNews()
{
	auto jsonReq = HttpWorker::Get(L"https://www.patreon.com/api/campaigns/2879379/posts");
	jsonReq.then([&](http_response& res)
	{
		// to solve json problem
		res.headers()[L"Content-Type"] = L"application/json";
		auto json = res.extract_json().get();

		auto data = json.at(L"data").as_array();
		for (auto& item : data)
		{
			if (item.at(L"id").as_string() != L"27973158") continue;

			std::wstring title = item.at(L"attributes").at(L"title").as_string();
			std::wstring content = item.at(L"attributes").at(L"content").as_string();
			content = Utils::ReplaceString(content, L"</p>", L"\n"); 
			content = Utils::ReplaceString(content, L"</li>", L"\n");
			content = Utils::ReplaceString(content, L"</ul>", L"\n");
			content = Utils::ReplaceString(content, L"\xa0", L""); // &nbsp; for html
			content = Utils::RemoveStringBetween(content, L"<", L">");

			LastNews = { title, content };
		}
	});

	return jsonReq;
}

void InitPatreon()
{
	try { GetGoals().wait(); }
	catch (const std::exception&) {}

	try { GetLastNews().wait(); }
	catch (const std::exception&) {}
}
#pragma endregion

#pragma region Address Viewer
PBYTE PCurrentAddressData = nullptr;
int BufSize = 0x200;
uintptr_t CurrentViewerAddress = uintptr_t(0x0);

MemoryEditor::u8 AddressViewerReadFn(const MemoryEditor::u8* data, const size_t off)
{
	if (!PCurrentAddressData)
		return 0;

	return PCurrentAddressData[off];
}

void GoToAddress(const uintptr_t address)
{
	if (Utils::MemoryObj)
	{
		// Only alloc once
		if (!PCurrentAddressData)
			PCurrentAddressData = new BYTE[BufSize];

		Utils::MemoryObj->ReadBytes(address, PCurrentAddressData, BufSize);
		CurrentViewerAddress = address;
	}
}
#pragma endregion

#pragma region Memory
void SetupMemoryStuff(HANDLE pHandle)
{
	// Setup Memory Stuff
	if (!memory_init)
	{
		memory_init = true;
		Utils::MemoryObj = new Memory(pHandle, use_kernel);
		if (!use_kernel) Utils::MemoryObj->GetDebugPrivileges();

		// Grab engine version information
		Utils::UnrealEngineVersion(game_ue_version);

		// Setup memory editor
		mem_edit.OptMidColsCount = static_cast<int>(Utils::PointerSize());
		mem_edit.PreviewDataType = Utils::MemoryObj->Is64 ? MemoryEditor::DataType_S64 : MemoryEditor::DataType_S32;
	}
}

bool IsReadyToGo()
{
	HANDLE pHandle;
	if (Memory::IsValidProcess(process_id, &pHandle))
	{
		// Setup Memory and lock process
		SetupMemoryStuff(pHandle);

		return true;
	}
	return false;
}
#pragma endregion

#pragma region Work Functions
void StartGObjFinder(const bool easyMethod)
{
	g_obj_listbox_items.clear();
	BeforeWork();
	g_objects_find_disabled = true;
	g_objects_disabled = false;
	g_names_disabled = false;
	g_obj_listbox_items.emplace_back("Searching...");
	g_obj_listbox_item_current = 0;
	Utils::WorkingNow.GObjectsFinder = true;

	std::thread t([=]()
	{
		std::vector<uintptr_t> ret;

		GObjectsFinder taf(easyMethod);
		taf.Find(ret);

		g_obj_listbox_items.clear();

		for (auto v : ret)
		{
			std::stringstream ss;
			ss << std::hex << v;

			std::string tmpUpper = ss.str();
			std::transform(tmpUpper.begin(), tmpUpper.end(), tmpUpper.begin(), ::toupper);

			g_obj_listbox_items.push_back(tmpUpper);
		}

		if (ret.size() == 1)
			strcpy_s(g_objects_buf, sizeof g_objects_buf, g_obj_listbox_items[0].data());

		Utils::WorkingNow.GObjectsFinder = false;
		g_objects_find_disabled = false;
		AfterWork();
	});
	t.detach();
}

void StartGNamesFinder()
{
	g_names_listbox_items.clear();
	g_names_listbox_items.emplace_back("Searching...");

	BeforeWork();
	g_names_find_disabled = true;
	g_objects_disabled = false;
	g_names_disabled = false;
	Utils::WorkingNow.GNamesFinder = true;

	std::thread t([&]()
	{
		GNamesFinder gf;
		const uintptr_t gname_address = gf.Find()[0]; // always return one address

		g_names_listbox_items.clear();

		if (gname_address != NULL)
		{
			// Convert to hex string
			std::stringstream ss; ss << std::hex << gname_address;

			// Make hex char is Upper
			std::string tmpUpper = ss.str();
			std::transform(tmpUpper.begin(), tmpUpper.end(), tmpUpper.begin(), ::toupper);

			// Set value for UI
			g_names_listbox_items.push_back(tmpUpper);
			strcpy_s(g_names_buf, sizeof g_names_buf, g_names_listbox_items[0].data());
		}

		Utils::WorkingNow.GNamesFinder = false;
		g_names_find_disabled = false;
		AfterWork();
	});
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

	LoadOverrideEngine();
	Utils::WorkingNow.ClassesFinder = true;
	class_listbox_items.clear();

	std::thread t([&]()
	{
		BeforeWork();
		class_find_disabled = true;

		ClassFinder cf;
		class_listbox_items = cf.Find(g_objects_address, g_names_address, class_find_buf);

		Utils::WorkingNow.ClassesFinder = false;
		class_find_disabled = false;
		AfterWork();
	});
	t.detach();
}

void StartInstanceLogger()
{
	BeforeWork();
	il_objects_count = 0;
	il_names_count = 0;
	il_state = "Running . . .";
	Utils::WorkingNow.InstanceLogger = true;
	LoadOverrideEngine();

	std::thread t([&]()
	{
		InstanceLogger il(g_objects_address, g_names_address);
		const auto retState = il.Start();

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

		Utils::WorkingNow.InstanceLogger = false;
		il_objects_count = retState.GObjectsCount;
		il_names_count = retState.GNamesCount;
		AfterWork();
	});
	t.detach();
}

void StartSdkGenerator()
{
	BeforeWork();
	g_objects_find_disabled = true;
	g_names_find_disabled = true;

	sg_objects_count = 0;
	sg_names_count = 0;
	sg_packages_count = 0;
	sg_packages_done_count = 0;
	sg_state = "Running . . .";
	Utils::WorkingNow.SdkGenerator = true;

	std::thread t([&]()
	{
		LoadOverrideEngine();

		StartInfo startInfo;
		startInfo.PObjCount = &sg_objects_count;
		startInfo.PNamesCount = &sg_names_count;
		startInfo.PPackagesCount = &sg_packages_count;
		startInfo.PPackagesDone = &sg_packages_done_count;
		startInfo.GameName = sg_game_name_buf;
		startInfo.GameVersion = std::to_string(sg_game_version[0]) + "." + std::to_string(sg_game_version[1]) + "." + std::to_string(sg_game_version[2]);
		startInfo.TargetSdkType = static_cast<SdkType>(sg_type_item_current);
		startInfo.State = &sg_state;
		startInfo.PackagesDone = &sg_packages_items;
		startInfo.SdkLang = sg_lang_items[sg_lang_item_current];

		SdkGenerator sg(g_objects_address, g_names_address);
		const SdkInfo ret = sg.Start(startInfo);

		if (ret.State == GeneratorState::Good)
		{
			sg_finished = true;
			sg_state = "Finished.!!";
			sg_finished_time = ret.TookTime;
			Utils::UiMainWindow->FlashWindow();
		}
		else if (ret.State == GeneratorState::BadGObject)
		{
			sg_state = "Wrong (GObjects) Address.!!";
		}
		else if (ret.State == GeneratorState::BadGName)
		{
			sg_state = "Wrong (GNames) Address.!!";
		}

		Utils::WorkingNow.SdkGenerator = false;
		AfterWork();
	});
	t.detach();
}
#pragma endregion

#pragma region User Interface
void DonationUi(UiWindow* thiz)
{
#ifndef _DEBUG
	if (donate_show)
		ui::OpenPopup("Donate?");

	// Popup
	if (ui::BeginPopupModal("Donate?", nullptr, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoMove))
	{
		ui::TextColored(ImVec4(0.92f, 0.30f, 0.29f, 1.0f), "Welcome on Unreal Finder Tool");
		ui::Text(R"(
To code this tool it take a BIG time.
With your support i can give it more time.
Any help, even small, make a difference.

)");
		ui::TextColored(IM_COL4(230, 126, 34, 255), "On Patreon:\nYou will open future Exclusive articles and tutorial");
		ui::Separator();

		ui::PushStyleColor(ImGuiCol_Text, ImVec4(0.92f, 0.30f, 0.29f, 1.0f));
		if (ui::Button("Patreon", ImVec2(120, 0)))
		{
			ShellExecute(nullptr,
				"open",
				"https://www.patreon.com/bePatron?u=16013498",
				nullptr,
				nullptr,
				SW_SHOWDEFAULT);
			donate_show = false;
			ui::CloseCurrentPopup();
		}
		ui::PopStyleColor();

		ui::SetItemDefaultFocus();
		ui::SameLine();
		ui::PushStyleColor(ImGuiCol_Text, ImVec4(0.28f, 0.20f, 0.83f, 1.0f));
		if (ui::Button("PayPal", ImVec2(120, 0)))
		{
			ShellExecute(nullptr,
				"open",
				"http://paypal.me/IslamNofl",
				nullptr,
				nullptr,
				SW_SHOWDEFAULT);
			donate_show = false;
			ui::CloseCurrentPopup();
		}
		ui::PopStyleColor();

		ui::SameLine();
		ui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.0f, 0.0f, 1.0f));
		if (ui::Button("Cancel", ImVec2(120, 0)))
		{
			donate_show = false;
			ui::CloseCurrentPopup();
		}
		ui::PopStyleColor();

		ui::EndPopup();
	}
#endif
}

void TitleBarUi(UiWindow* thiz)
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

			ui::Separator();

			if (ImGui::BeginMenu("Locator##menu"))
			{
				if (ui::MenuItem("Tool Folder"))
				{
					ShellExecute(nullptr,
						"open",
						Utils::GetWorkingDirectoryA().c_str(),
						nullptr,
						nullptr,
						SW_SHOWDEFAULT);
				}
				ui::Separator();

				if (ui::MenuItem("SDK Folder"))
				{
					ShellExecute(nullptr,
						"open",
						(Utils::GetWorkingDirectoryA() + "\\Results").c_str(), 
						nullptr,
						nullptr,
						SW_SHOWDEFAULT);
				}
				ui::Separator();

				if (ui::MenuItem("Config Folder"))
				{
					ShellExecute(nullptr,
						"open",
						(Utils::GetWorkingDirectoryA() + "\\Config").c_str(),
						nullptr,
						nullptr,
						SW_SHOWDEFAULT);
				}

				ui::EndMenu();
			}

			ui::Separator();

			if (ImGui::BeginMenu("Help##menu"))
			{
				if (ui::MenuItem("GitHub"))
				{
					ShellExecute(nullptr,
						"open",
						"https://github.com/CorrM/Unreal-Finder-Tool",
						nullptr,
						nullptr,
						SW_SHOWDEFAULT);
				}
				ui::Separator();

				if (ui::MenuItem("Wiki"))
				{
					ShellExecute(nullptr,
						"open",
						"https://github.com/CorrM/Unreal-Finder-Tool/wiki",
						nullptr,
						nullptr,
						SW_SHOWDEFAULT);
				}
				ui::Separator();

				if (ui::MenuItem("Report issue"))
				{
					ShellExecute(nullptr,
						"open",
						"https://github.com/CorrM/Unreal-Finder-Tool/issues/new",
						nullptr,
						nullptr,
						SW_SHOWDEFAULT);
				}
				ui::Separator();

				if (ui::MenuItem("Last version"))
				{
					ShellExecute(nullptr,
						"open",
						"https://github.com/CorrM/Unreal-Finder-Tool/releases/latest",
						nullptr,
						nullptr,
						SW_SHOWDEFAULT);
				}
				ui::Separator();

				if (ui::MenuItem("Version note"))
				{
					ShellExecute(nullptr,
						"open",
						"https://github.com/CorrM/Unreal-Finder-Tool/releases/tag/" TOOL_VERSION,
						nullptr,
						nullptr,
						SW_SHOWDEFAULT);
				}

				ui::EndMenu();
			}

#ifndef _DEBUG
			ui::Separator();

			ui::PushStyleColor(ImGuiCol_Text, ImVec4(0.0f, 1.0f, 0.0f, 1.0f));
			if (ui::MenuItem("DONATE")) donate_show = true;
			ui::PopStyleColor();
#endif
			ui::EndPopup();
		}
	}

	// Title
	{
		ui::SameLine();
		ui::SetCursorPosX(abs(ui::CalcTextSize("Unreal Finder Tool By CorrM").x - ui::GetWindowWidth()) / 2);
		ui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "Unreal Finder Tool By CorrM");

#ifdef MIDI_h
		ui::SameLine();
		ui::SetCursorPosX(abs(ui::GetWindowWidth() - 65));
		
		if (ui::Button(!MidiPlayer || (MidiPlayer->IsPaused() || !MidiPlayer->IsPlaying()) ? ICON_FA_PLAY : ICON_FA_PAUSE))
		{
			if (!MidiPlayer)
			{
				MidiPlayer = new CMIDI();
				MidiPlayer->Create(const_cast<LPBYTE>(midi_track1), sizeof midi_track1);
			}
			if (MidiPlayer->IsPaused())
				MidiPlayer->Continue();
			else if (MidiPlayer->IsPlaying())
				MidiPlayer->Pause();
			else
				MidiPlayer->Play(true);
		}
		ui::SameLine();
		if (ui::Button(ICON_FA_STOP))
		{
			if (MidiPlayer && MidiPlayer->IsPlaying())
				MidiPlayer->Stop();
		}
#endif
	}
}

void InformationSectionUi(UiWindow* thiz)
{
	// Process ID
	{
		ui::AlignTextToFramePadding();
		ui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "Process ID : ");
		ui::SameLine();
		ui::SetNextItemWidth(LeftWidth / 2.55f);
		ENABLE_DISABLE_WIDGET(ui::InputInt("##ProcessID", &process_id), process_id_disabled);
		ui::SameLine();

		ENABLE_DISABLE_WIDGET_IF(ui::Button(ICON_FA_LOCK "##ProcessSubmit"), process_lock_disabled,
		{
			if (!IsReadyToGo())
			{
				popup_not_valid_process = true;
				return;
			}

			process_id_disabled = true;
			process_detector_disabled = true;
			process_lock_disabled = true;
			EnabledAll();
			use_kernel_disabled = false;
			g_objects_disabled = false;
			g_names_disabled = false;
			game_ue_disabled = false;
			process_module_disabled = false;

			// Get Game Window Title
			if (process_id != NULL && Memory::IsValidProcess(process_id))
			{
				Utils::DetectUnrealGame(window_title);
				if (!window_title.empty() && sg_game_name_buf[0] == '\0')
				{
					sg_game_name_buf.insert(0, window_title);
				}
			}

			// Get Game Modules
			if (process_module_items.empty())
			{
				auto modList = Utils::MemoryObj->GetModuleList();
				for (auto& mod : modList)
				{
					if (!Utils::EndsWith(mod.szModule, ".dll") && !Utils::EndsWith(mod.szModule, ".DLL"))
						process_module_items.emplace_back(mod.szModule);
				}
			}

			// Get JsonEngine to override
			for (size_t i = 0; i < unreal_versions.size(); ++i)
			{
				std::string toLowerTitle = window_title;
				std::string toLowerJson = unreal_versions[i];

				std::transform(toLowerTitle.begin(), toLowerTitle.end(), toLowerTitle.begin(), ::tolower);
				std::transform(toLowerJson.begin(), toLowerJson.end(), toLowerJson.begin(), ::tolower);

				if (Utils::ContainsString(unreal_versions[i], game_ue_version) ||
					Utils::ContainsString(toLowerTitle, toLowerJson))
				{
					ue_selected_version = i;
					break;
				}
			}
		});

		ui::SameLine();

		ENABLE_DISABLE_WIDGET_IF(ui::Button(ICON_FA_SEARCH "##ProcessAutoDetector"), process_detector_disabled,
		{
			DWORD pid = Utils::DetectUnrealGame();
			process_id = pid ? pid : process_id;
		});
	}

	// Use Kernel
	{
		ui::AlignTextToFramePadding();
		ui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "Use Kernel : ");
		ui::SameLine();
		ENABLE_DISABLE_WIDGET(ui::Checkbox("##UseKernal", &use_kernel), use_kernel_disabled);
	}

	// Game Module
	{
		ui::AlignTextToFramePadding();
		ui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "Module     : ");
		ui::SameLine();
		ui::SetNextItemWidth(MidWidth / 1.8f);
		ENABLE_DISABLE_WIDGET(ui::Combo("##GameModule", &process_module_item_current, VectorGetter, static_cast<void*>(&process_module_items), static_cast<int>(process_module_items.size()), 4), process_module_disabled);
	}

	// GObjects Address
	{
		ui::AlignTextToFramePadding();
		ui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "GObjects   : ");
		ui::SameLine();
		ui::SetNextItemWidth(LeftWidth / 2.4f);

		bool style_pushed = false;
		if (!g_objects_disabled && g_objects_address != 0)
		{
			style_pushed = true;
			const bool isValid = Utils::IsTUobjectArray(g_objects_address);
			if (isValid)
				ui::PushStyleColor(ImGuiCol_Text, ImVec4(0.0f, 1.0f, 0.0f, 1.0f));
			else
				ui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.0f, 0.0f, 1.0f));
		}

		ENABLE_DISABLE_WIDGET(ui::InputText("##GObjects", g_objects_buf, IM_ARRAYSIZE(g_objects_buf), ImGuiInputTextFlags_CharsHexadecimal), g_objects_disabled);
		
		if (style_pushed)
			ui::PopStyleColor();

		ui::SameLine();
		HelpMarker("What you can put here .?\n- First UObject address.\n- First GObjects chunk address.\n\n* Not GObjects pointer.\n* It's the address you get from this tool.");
		g_objects_address = Utils::CharArrayToUintptr(g_objects_buf);
		ui::SameLine();
		if (ui::Button(ICON_FA_EYE"##view address gobjects") && g_objects_address != NULL)
			GoToAddress(g_objects_address);
	}

	// GNames Address
	{
		ui::AlignTextToFramePadding();
		ui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "GNames     : ");
		ui::SameLine();


		bool style_pushed = false;
		if (!g_names_disabled && g_names_address != 0)
		{
			style_pushed = true;
			if (Utils::IsValidGNamesAddress(g_names_address))
				ui::PushStyleColor(ImGuiCol_Text, ImVec4(0.0f, 1.0f, 0.0f, 1.0f));
			else
				ui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.0f, 0.0f, 1.0f));
		}

		ui::SetNextItemWidth(LeftWidth / 2.4f);
		ENABLE_DISABLE_WIDGET(ui::InputText("##GNames", g_names_buf, IM_ARRAYSIZE(g_names_buf), ImGuiInputTextFlags_CharsHexadecimal), g_names_disabled);
		g_names_address = Utils::CharArrayToUintptr(g_names_buf);

		if (style_pushed)
			ui::PopStyleColor();

		ui::SameLine();
		HelpMarker("What you can put here .?\n- GNames chunk array address.\n\n* Not GNames pointer.\n* It's the address you get from this tool.");
		ui::SameLine();
		if (ui::Button(ICON_FA_EYE"##view address gnames") && g_names_address != NULL)
			GoToAddress(g_names_address);
	}

	// Unreal version
	{
		ui::AlignTextToFramePadding();
		ui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "UE Version : ");
		if (ui::IsItemHovered())
		{
			ui::BeginTooltip();
			ui::PushTextWrapPos(ui::GetFontSize() * 35.0f);
			ui::Text("%s", game_ue_version.c_str());
			ui::PopTextWrapPos();
			ui::EndTooltip();
		}
		ui::SameLine();
		ui::SetNextItemWidth(LeftWidth / 1.57f);
		ENABLE_DISABLE_WIDGET_IF(ui::BeginCombo("##UnrealVersion", unreal_versions[ue_selected_version].c_str()), game_ue_disabled,
		{
			for (size_t i = 0; i < unreal_versions.size(); ++i)
				if (ui::Selectable(unreal_versions[i].c_str())) ue_selected_version = i;

			ui::EndCombo();
		});
	}

	// Window Title
	{
		ui::AlignTextToFramePadding();
		ui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "Win Title  : ");
		ui::SameLine();

		ui::TextUnformatted(window_title.c_str());
	}
}

void MemoryInterfaceUi(UiWindow* thiz)
{
	if (ui::BeginChild("AddressViewer", { 0, 180 }, false, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse))
	{
		mem_edit.DrawContents(nullptr, BufSize, CurrentViewerAddress);
		ui::EndChild();
	}
}

void FinderUi(UiWindow* thiz)
{
	if (ui::BeginTabItem("Finder"))
	{
		if (cur_tap_id != 1)
		{
			//thiz.SetSize(380, 620);
			cur_tap_id = 1;
		}

		// ## GObjects
		{
			ui::BeginGroup();

			// Label
			ui::AlignTextToFramePadding();
			static float gobj_label_pos = ui::GetCursorPosX() + abs(ui::CalcTextSize("!~[ GObjects ]~!").x - (MidWidth / 2.f)) / 2 - 3.f;
			ui::SetCursorPosX(gobj_label_pos);
			ui::TextColored(ImVec4(0.16f, 0.50f, 72.0f, 1.0f), "!~[ GObjects ]~!");

			// Finder
			ENABLE_DISABLE_WIDGET_IF(ui::Button("Find##GObjects", { MidWidth / 4.7f, 0.0f }), g_objects_find_disabled,
			{
				if (IsReadyToGo())
					ui::OpenPopup("Easy?");
				else
					popup_not_valid_process = true;
			});

			ui::SameLine();
			if (ui::Button("Use##Objects", { MidWidth / 4.7f, 0.0f }))
			{
				if (size_t(g_obj_listbox_item_current) < g_obj_listbox_items.size())
				{
					if (!g_objects_disabled)
						strcpy_s(g_objects_buf, sizeof g_objects_buf, g_obj_listbox_items[g_obj_listbox_item_current].data());

					if (Utils::MemoryObj)
					{
						const uintptr_t address = Utils::CharArrayToUintptr(g_obj_listbox_items[g_obj_listbox_item_current]);

						// Only alloc once
						if (!PCurrentAddressData)
							PCurrentAddressData = new BYTE[BufSize];

						Utils::MemoryObj->ReadBytes(address, PCurrentAddressData, BufSize);
						GoToAddress(address);
						
					}
				}
			}

			ui::SetNextItemWidth(MidWidth / 2.15f);
			ui::ListBox("##Obj_listbox",
				&g_obj_listbox_item_current,
				VectorGetter,
				static_cast<void*>(&g_obj_listbox_items), static_cast<int>(g_obj_listbox_items.size()),
				4);

			// Popup
			if (ui::BeginPopupModal("Easy?", nullptr, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoMove))
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

			ui::EndGroup();
		}

		ui::SameLine();
		ui::VerticalSeparator();
		ui::SameLine();

		// ## GNames
		{
			ui::BeginGroup();
			ui::AlignTextToFramePadding();
			static float gnames_label_pos = MidWidth / 2 + abs(ui::CalcTextSize("!~[ GNames ]~!").x - MidWidth / 2.f) / 2.f;
			ui::SetCursorPosX(gnames_label_pos);
			ui::TextColored(ImVec4(0.16f, 0.50f, 72.0f, 1.0f), "!~[ GNames ]~!");

			// Start Finder
			ENABLE_DISABLE_WIDGET_IF(ui::Button("Find##GNames", { MidWidth / 4.7f, 0.0f }), g_names_find_disabled,
			{
				if (IsReadyToGo())
					StartGNamesFinder();
				else
					popup_not_valid_process = true;
			});

			ui::SameLine();

			// Set to input box
			if (ui::Button("Use##Names", { MidWidth / 4.7f, 0.0f }))
			{
				if (size_t(g_names_listbox_item_current) < g_names_listbox_items.size())
				{
					if (!g_names_disabled)
						strcpy_s(g_names_buf, sizeof g_names_buf, g_names_listbox_items[g_names_listbox_item_current].data());

					if (Utils::MemoryObj)
					{
						const uintptr_t address = Utils::CharArrayToUintptr(g_names_listbox_items[g_names_listbox_item_current]);

						// Only alloc once
						if (!PCurrentAddressData)
							PCurrentAddressData = new BYTE[BufSize];

						Utils::MemoryObj->ReadBytes(address, PCurrentAddressData, BufSize);
						GoToAddress(address);
					}
				}
			}

			ui::SetNextItemWidth(MidWidth / 2.15f);
			ui::ListBox("##Names_listbox",
				&g_names_listbox_item_current,
				VectorGetter,
				static_cast<void*>(&g_names_listbox_items),
				static_cast<int>(g_names_listbox_items.size()),
				4);

			ui::EndGroup();
		}

		ui::Separator();

		// ## Class
		{
			ui::BeginGroup();

			// Label
			ui::AlignTextToFramePadding();
			static float class_label_pos = ui::GetCursorPosX() + abs(ui::CalcTextSize("!~[ Classes ]~!").x - MidWidth) / 2.f;
			ui::SetCursorPosX(class_label_pos);
			ui::TextColored(ImVec4(0.16f, 0.50f, 72.0f, 1.0f), "!~[ Classes ]~!");

			ui::AlignTextToFramePadding();
			ui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "Class   :");
			ui::SameLine();

			// Class Input
			ui::SetNextItemWidth(MidWidth / 1.45f);
			ENABLE_DISABLE_WIDGET(ui::InputTextWithHint("##FindClass", "LocalPlayer, 0x0000000000", class_find_buf, IM_ARRAYSIZE(class_find_buf)), class_find_input_disabled);
			ui::SameLine();
			HelpMarker("What you can put here.?\n- Class Name:\n  - LocalPlayer or ULocalPlayer.\n  - MyGameInstance_C or UMyGameInstance_C.\n  - PlayerController or APlayerController.\n\n- Instance address:\n  - 0x0000000000.\n  - 0000000000.");

			ui::AlignTextToFramePadding();
			ui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "Buttons :");
			ui::SameLine();

			// Start Finder
			ENABLE_DISABLE_WIDGET_IF(ui::Button(" Find Class "), class_find_disabled,
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
				if (size_t(class_listbox_item_current) < class_listbox_items.size())
				{
					const std::string curStr = class_listbox_items[class_listbox_item_current];
					size_t position;
					if ((position = curStr.find(' ')) != std::string::npos)
						ui::SetClipboardText(curStr.substr(0, position).c_str());
				}
			}

			ui::SetNextItemWidth(MidWidth);
			ui::ListBox("##Class_listbox",
				&class_listbox_item_current,
				VectorGetter,
				static_cast<void*>(&class_listbox_items),
				static_cast<int>(class_listbox_items.size()),
				6);

			ui::EndGroup();
		}

		ui::EndTabItem();
	}
}

void InstanceLoggerUi(UiWindow* thiz)
{
	if (ui::BeginTabItem("Instance"))
	{
		if (cur_tap_id != 2)
		{
			//thiz.SetSize(380, 407);
			cur_tap_id = 2;
		}

		ui::AlignTextToFramePadding();
		ui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "Objects Count : ");
		ui::SameLine();
		ui::Text("%d", il_objects_count);

		ui::AlignTextToFramePadding();
		ui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "Names Count   : ");
		ui::SameLine();
		ui::Text("%d", il_names_count);

		ui::AlignTextToFramePadding();
		ui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "State         : ");
		ui::SameLine();
		ui::Text("%s", il_state.c_str());

		// Start Logger
		ENABLE_DISABLE_WIDGET_IF(ui::Button("Start##InstanceLogger", { MidWidth, 0.0f }), il_start_disabled,
		{
			if (IsReadyToGo())
				StartInstanceLogger();
			else
				popup_not_valid_process = true;
		});

		ui::EndTabItem();
	}
}

void SdkGeneratorUi(UiWindow* thiz)
{
	if (ui::BeginTabItem("S-D-K"))
	{
		if (cur_tap_id != 3)
		{
			//thiz.SetSize(380, 622);
			cur_tap_id = 3;
		}

		// Objects/Names
		ui::AlignTextToFramePadding();
		ui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "Objects/Names : ");
		ui::SameLine();
		ui::Text("%d / %d", sg_objects_count, sg_names_count);

		// Packages
		ui::AlignTextToFramePadding();
		ui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "Packages      : ");
		ui::SameLine();
		ui::Text("%d / %d", sg_packages_done_count, sg_packages_count);

		// Sdk Lang
		ui::AlignTextToFramePadding();
		ui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "Sdk Lang      : ");
		ui::SameLine();
		ui::SetNextItemWidth(MidWidth / 1.8f);
		ENABLE_DISABLE_WIDGET(ui::Combo("##SdkLang", &sg_lang_item_current, VectorGetter, static_cast<void*>(&sg_lang_items), static_cast<int>(sg_lang_items.size()), 4), sg_lang_disabled);
		ui::SameLine();
		HelpMarker("Pick programming language for generated sdk.");

		// Sdk Type
		ui::AlignTextToFramePadding();
		ui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "Sdk Type      : ");
		ui::SameLine();
		ui::SetNextItemWidth(MidWidth / 1.8f);
		ENABLE_DISABLE_WIDGET(ui::Combo("##SdkType", &sg_type_item_current, VectorGetter, static_cast<void*>(&sg_type_items), static_cast<int>(sg_type_items.size()), 4), sg_type_disabled);
		ui::SameLine();
		HelpMarker("- Internal: Generate functions for class/struct.\n- External: Don't gen functions for class/struct,\n    But generate ReadAsMe for every class/struct.");

		// Game Name
		ui::AlignTextToFramePadding();
		ui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "Game Name     : ");
		ui::SameLine();
		ui::SetNextItemWidth(MidWidth / 1.8f);
		ENABLE_DISABLE_WIDGET(ui::InputTextWithHint("##GameName", "PUBG, Fortnite", sg_game_name_buf.data(), sg_game_name_buf.length()), sg_game_name_disabled);

		// Game Version
		ui::AlignTextToFramePadding();
		ui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "Game Version  : ");
		ui::SameLine();
		ui::SetNextItemWidth(MidWidth / 1.8f);
		ENABLE_DISABLE_WIDGET(ui::InputInt3("##GameVersion", sg_game_version), sg_game_version_disabled);
		
		// State
		ui::AlignTextToFramePadding();
		ui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "State         : ");
		ui::SameLine();
		ui::Text("%s", sg_state.c_str());

		// Packages Box
		ui::SetNextItemWidth(MidWidth);
		ui::ListBoxA("##Packages_listbox",
			&sg_packages_item_current,
			VectorGetter,
			static_cast<void*>(&sg_packages_items),
			static_cast<int>(sg_packages_items.size()), 6, true);

		// Start Generator
		ENABLE_DISABLE_WIDGET_IF(ui::Button("Start##SdkGenerator", { MidWidth, 0.0f }), sg_start_disabled,
		{
			sg_suspend_popup = true;
		});

		// Popup
		if (sg_suspend_popup)
			ui::OpenPopup("Suspend?");
		else if (sg_delete_old_popup)
			ui::OpenPopup("Delete Old SDK?");

		if (ui::BeginPopupModal("Delete Old SDK?", nullptr, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoMove))
		{
			ui::Text("There is an old sdk !, Delete it .?\n\n");
			ui::Separator();

			if (ui::Button("Yes", ImVec2(75, 0)))
			{
				ui::CloseCurrentPopup();
				Utils::DirectoryDelete(Utils::GetWorkingDirectoryA() + "\\Results");
				if (IsReadyToGo())
					StartSdkGenerator();
				else
					popup_not_valid_process = true;

				sg_delete_old_popup = false;
			}

			ui::SetItemDefaultFocus();
			ui::SameLine();
			if (ui::Button("No", ImVec2(75, 0)))
			{
				ui::CloseCurrentPopup();
				if (IsReadyToGo())
					StartSdkGenerator();
				else
					popup_not_valid_process = true;

				sg_delete_old_popup = false;
			}

			ui::SameLine();
			if (ui::Button("Cancel", ImVec2(75, 0)))
			{
				ui::CloseCurrentPopup();
				sg_delete_old_popup = false;
			}

			ui::EndPopup();
		}
		if (ui::BeginPopupModal("Suspend?", nullptr, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoMove))
		{
			ui::Text("For some reasons UFT love to Suspend target process.\nIf you want to suspend click yes !!\n\n");
			ui::Separator();

			if (ui::Button("Yes", ImVec2(115, 0)))
			{
				ui::CloseCurrentPopup();
				Utils::MemoryObj->SuspendProcess();
				if (Utils::FileExists(Utils::GetWorkingDirectoryA() + "\\Results"))
				{
					sg_delete_old_popup = true;
				}
				else
				{
					if (IsReadyToGo())
						StartSdkGenerator();
					else
						popup_not_valid_process = true;
				}

				sg_suspend_popup = false;
			}

			ui::SetItemDefaultFocus();
			ui::SameLine();
			if (ui::Button("No", ImVec2(115, 0)))
			{
				ui::CloseCurrentPopup();
				if (Utils::FileExists(Utils::GetWorkingDirectoryA() + "\\Results"))
				{
					sg_delete_old_popup = true;
				}
				else
				{
					if (IsReadyToGo())
						StartSdkGenerator();
					else
						popup_not_valid_process = true;
				}

				sg_suspend_popup = false;
			}

			ui::SameLine();
			if (ui::Button("Cancel", ImVec2(115, 0)))
			{
				ui::CloseCurrentPopup();
				sg_suspend_popup = false;
			}

			ui::EndPopup();
		}
		ui::EndTabItem();
	}
}

void PatreonSection(UiWindow* thiz)
{
	if (ui::BeginChild("last-news", ImVec2(0, thiz->GetSize().y * 0.35f)))
	{
		ui::TextColored(IM_COL4(22, 160, 133, 255), ICON_FA_GRIN_HEARTS " Patreon Support");
		ui::Separator();
		if (!LastNews.Title.empty())
		{
			ui::TextColored(IM_COL4(142, 68, 173, 255), "Go to patreon by press `MenuButton->DONATE`.");
			ui::TextWrapped("%ls", LastNews.Content.c_str());
		}

		ui::EndChild();
	}
	ui::Separator();

	ui::TextColored(IM_COL4(231, 76, 60, 255), "Patreon Goals [%d]", Goals.size());
	if (ui::BeginChild("goals"))
	{
		for (size_t goal = 0; goal < Goals.size(); ++goal)
		{
			ui::TextColored(IM_COL4(22, 160, 133, 255), ICON_FA_FLAG " %d%% Complete", Goals[goal].CompletedPercentage);
			ui::ProgressBar(static_cast<float>(Goals[goal].CompletedPercentage) * .01f);
			ui::TextWrapped("%ls", Goals[goal].Description.c_str());
			if (goal != Goals.size() - 1)
				ui::Separator();
		}

		ui::EndChild();
	}
}

void MainUi(UiWindow* thiz)
{
	DonationUi(thiz);
	TitleBarUi(thiz);

	ui::Separator();

	// left-group
	{
		if (ui::BeginChild("##left-group", { 320.f - thiz->GetUiStyle().ItemSpacing.x, 0 }, false))
		{
			LeftWidth = ui::GetWindowWidth();

			InformationSectionUi(thiz);
			ui::Separator();

			// Tabs
			{
				if (ui::BeginTabBar("ToolsTabBar", ImGuiTabBarFlags_NoTooltip))
				{
					if (ui::BeginTabItem("Address Viewer"))
					{
						MemoryInterfaceUi(thiz);
						ui::EndTabItem();
					}

					ui::EndTabBar();
				}
			}

			ui::EndChild();
		}
	}

	ui::SameLine();
	ui::VerticalSeparator();
	ui::SameLine();
	
	// mid-group
	{
		if (ui::BeginChild("##mid-group", { 350, 0 }, false))
		{
			MidWidth = ui::GetWindowWidth();

			// Tabs
			{
				if (ui::BeginTabBar("Tools", ImGuiTabBarFlags_NoTooltip))
				{
					FinderUi(thiz);
					InstanceLoggerUi(thiz);
					SdkGeneratorUi(thiz);

					ui::EndTabBar();
				}
			}

			ui::EndChild();
		}
	}

	ui::SameLine();
	ui::VerticalSeparator();
	ui::SameLine();

	// right-group
	{
		if (ui::BeginChild("##right-group"))
		{
			RightWidth = ui::GetWindowWidth();
			PatreonSection(thiz);

			ui::EndChild();
		}
	}

	// Popups
	{
		WarningPopup("Note", "SDK Generation complete. !!"s + "\n" + "Took: " + GetTookTime(sg_finished_time), sg_finished);
		WarningPopup("Warning", "Not Valid Process ID. !!", popup_not_valid_process);
		WarningPopup("Warning", "Not Valid GNames Address. !!", popup_not_valid_gnames);
		WarningPopup("Warning", "Not Valid GObjects Address. !!", popup_not_valid_gobjects);
	}
}
#pragma endregion

// Fix vs2019 Problem [wWinMain instead of WinMain]
// ReSharper disable once CppInconsistentNaming
int APIENTRY wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPWSTR lpCmdLine, int nShowCmd) // NOLINT(readability-non-const-parameter)
{
	// Remove unneeded variables
	UNREFERENCED_PARAMETER(hInstance);
	UNREFERENCED_PARAMETER(hPrevInstance);
	UNREFERENCED_PARAMETER(lpCmdLine);
	UNREFERENCED_PARAMETER(nShowCmd);

	// Load Settings / Json Core
	if (!Utils::LoadSettings()) return 0;
	if (!Utils::LoadEngineCore(unreal_versions)) return 0;

	// Autodetect in case game already open
	int pid = static_cast<int>(Utils::DetectUnrealGame());
	process_id = pid ? pid : process_id;

	// Setup Address Viewer
	mem_edit.Cols = 8;
	mem_edit.OptMidColsCount = 4;
	mem_edit.OptAddrDigitsCount = 12;
	mem_edit.OptUpperCaseHex = true;
	mem_edit.OptShowAscii = false;
	mem_edit.OptShowHexIi = false;
	mem_edit.OptShowOptions = false;
	mem_edit.OptShowDataPreview = true;
	mem_edit.OptShowDataPreviewAs = false;
	mem_edit.OptShowDataPreviewDec = false;
	mem_edit.OptShowDataPreviewBin = false;
	mem_edit.OptShowDataPreviewHex = true;
	mem_edit.OptGreyOutZeroes = true;
	mem_edit.HighlightColor = IM_COL32(0, 0, 200, 200);
	mem_edit.ReadOnly = true;
	mem_edit.ReadFn = &AddressViewerReadFn;

	// Run the new debugging tools
	Debugging d;
	d.EnterDebugMode();

	// Patreon
	InitPatreon();

	// Setup ui
	DisabledAll();
	process_id_disabled = false;
	process_lock_disabled = false;
	process_detector_disabled = false;

	// Launch the main window
	Utils::UiMainWindow = new UiWindow("Unreal Finder Tool. Version: " TOOL_VERSION " - " TOOL_VERSION_TITLE, "CorrMFinder", 1050, 530);
	Utils::UiMainWindow->Show(MainUi);

// If this is called before the window is created: kaboom!
#if defined(MIDI_h) && !defined(_DEBUG)
	// Auto play MIDI
	MidiPlayer = new CMIDI();
	MidiPlayer->Create(const_cast<LPBYTE>(midi_track1), sizeof midi_track1);
	MidiPlayer->Play();
#endif // MIDI_h

#ifndef _DEBUG
	// Check New Version
	CheckLastVer();
#endif // _DEBUG

	// Wait Window to close
	while (!Utils::UiMainWindow->Closed())
		Sleep(1);

	// Cleanup
	Utils::CleanUp();

	return ERROR_SUCCESS;
}
