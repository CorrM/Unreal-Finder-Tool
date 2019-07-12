#pragma once
#include <vector>
#include "ImGUI/imgui.h"
#include "IconsFontAwesome.h"

#define TOOL_VERSION		"3.2.0"
#define TOOL_VERSION_TITLE	"Unreal Suspender"

#define IM_COL4(R, G, B, A) ImVec4((float)(R) / 255.f, (float)(G) / 255.f, (float)(B) / 255.f, (float)(A) / 255.f)
#define ENABLE_DISABLE_WIDGET(uiCode, disabledBool) { static bool disCheck = false; if (disabledBool) { disCheck = true; ui::PushItemFlag(ImGuiItemFlags_Disabled, true); ui::PushStyleVar(ImGuiStyleVar_Alpha, ImGui::GetStyle().Alpha * 0.5f); } uiCode; if (disCheck && disabledBool) { ImGui::PopItemFlag(); ImGui::PopStyleVar(); disCheck = false; } }
#define ENABLE_DISABLE_WIDGET_IF(uiCode, disabledBool, body) { static bool disCheck = false; if (disabledBool) { disCheck = true; ui::PushItemFlag(ImGuiItemFlags_Disabled, true); ui::PushStyleVar(ImGuiStyleVar_Alpha, ImGui::GetStyle().Alpha * 0.5f);} if(uiCode) body if (disCheck && disabledBool) { ImGui::PopItemFlag(); ImGui::PopStyleVar(); disCheck = false; } }

// => Patreon Section
struct PatreonGoal
{
	int CompletedPercentage;
	std::wstring Description;
};
struct PatreonPost
{
	std::wstring Title;
	std::wstring Content;
};

inline std::vector<PatreonGoal> Goals;
inline PatreonPost LastNews;
// => Patreon Section

// => Main Options Section
inline bool process_id_disabled = false;
inline bool process_lock_disabled = false;
inline bool process_detector_disabled = false;
inline int process_id = 19804;
inline bool process_controller_toggles[] = { false };

inline bool use_kernel_disabled = false;
inline bool use_kernel;

inline bool process_module_disabled = false;
inline std::vector<std::string> process_module_items;
inline int process_module_item_current = 0;

inline bool g_objects_disabled = false;
inline bool g_names_disabled = false;

inline bool game_ue_disabled = false;
inline std::string game_ue_version = "0.0.0";
inline size_t ue_selected_version;
inline std::vector<std::string> unreal_versions;

inline std::string window_title = "NONE";
// => Main Options Section

// => Popup
inline bool popup_not_valid_process = false;
inline bool popup_not_valid_gnames = false;
inline bool popup_not_valid_gobjects = false;
// => Popup

// => Tabs
inline int cur_tap_id = 0;
// => Tabs

// => GObjects, GNames, Class
inline bool g_objects_find_disabled = false;
inline uintptr_t g_objects_address;
inline char g_objects_buf[18] = "7FF6D3352B00";// { 0 };
inline std::vector<std::string> g_obj_listbox_items;
inline int g_obj_listbox_item_current = 0;

inline bool g_names_find_disabled = false;
inline uintptr_t g_names_address;
inline char g_names_buf[18] = "7FF6D346F1A8";// { 0 };
inline std::vector<std::string> g_names_listbox_items;
inline int g_names_listbox_item_current = 0;

inline bool class_find_disabled = false;
inline bool class_find_input_disabled = false;
inline char class_find_buf[90] = { 0 };
inline std::vector<std::string> class_listbox_items;
inline int class_listbox_item_current = 0;
// => GObjects, GNames, Class

// => Instance Logger
inline bool il_start_disabled = false;
inline size_t il_objects_count = 0;
inline size_t il_names_count = 0;
inline std::string il_state = "Ready ..!!";
// => Instance Logger

// => Sdk Generator
inline bool sg_delete_old_popup = false;
inline bool sg_suspend_popup = false;
inline bool sg_start_disabled = false;
inline bool sg_finished = false;
inline std::tm sg_finished_time;

inline size_t sg_objects_count = 0;
inline size_t sg_names_count = 0;
inline size_t sg_packages_count = 0;
inline size_t sg_packages_done_count = 0;

inline bool sg_type_disabled = false;
inline std::vector<std::string> sg_type_items = { "Internal", "External" };
inline int sg_type_item_current = 0;

inline bool sg_lang_disabled = false;
inline std::vector<std::string> sg_lang_items = { "Cpp" };
inline int sg_lang_item_current = 0;

inline bool sg_game_name_disabled = false;
inline std::string sg_game_name_buf(30, '\0');

inline bool sg_game_version_disabled = false;
inline int sg_game_version[3] = { 1, 0, 0 };

inline std::string sg_state = "Ready ..!!";

inline std::vector<std::string> sg_packages_items;
inline int sg_packages_item_current = 0;
// => Sdk Generator

// => Help Functions
static void DisabledAll()
{
	process_id_disabled = true;
	process_lock_disabled = true;
	process_detector_disabled = true;
	use_kernel_disabled = true;
	process_module_disabled = true;
	game_ue_disabled = true;
	g_objects_disabled = true;
	g_names_disabled = true;

	g_objects_find_disabled = true;
	g_names_find_disabled = true;
	class_find_disabled = true;
	class_find_input_disabled = true;

	il_start_disabled = true;

	sg_lang_disabled = true;
	sg_type_disabled = true;
	sg_game_name_disabled = true;
	sg_game_version_disabled = true;
	sg_start_disabled = true;
}

static void EnabledAll()
{
	g_objects_disabled = false;
	g_names_disabled = false;
	g_objects_find_disabled = false;
	g_names_find_disabled = false;
	class_find_disabled = false;
	class_find_input_disabled = false;

	il_start_disabled = false;

	sg_lang_disabled = false;
	sg_type_disabled = false;
	sg_game_name_disabled = false;
	sg_game_version_disabled = false;
	sg_start_disabled = false;
}

static bool VectorGetter(void* vec, const int idx, const char** out_text)
{
	auto& vector = *static_cast<std::vector<std::string>*>(vec);
	if (idx < 0 || idx >= static_cast<int>(vector.size())) { return false; }
	*out_text = vector.at(idx).c_str();
	return true;
};

static void HelpMarker(const char* desc)
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

static void WarningPopup(const std::string& title, const std::string& message, bool& opener, const std::function<void()>& okCallBack)
{
	std::string id = title + "##" + message;

	if (opener)
		ui::OpenPopup(id.c_str());

	if (ui::BeginPopupModal(id.c_str(), nullptr, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoMove))
	{
		ui::Text("%s", message.c_str());
		ui::Separator();

		if (ui::Button("Ok", ImVec2(200, 0)))
		{
			if (okCallBack)
				okCallBack();
			ui::CloseCurrentPopup();
			opener = false;
		}
		ui::SetItemDefaultFocus();
		ui::EndPopup();

	}
}

static void WarningPopup(const std::string& title, const std::string& message, bool& opener)
{
	WarningPopup(title, message, opener,  nullptr);
}

#pragma region Custoum Controls
namespace ImGui
{
	inline bool ListBoxA(const char* label, int* current_item, bool (*items_getter)(void*, int, const char**), void* data, const int items_count, const int height_in_items, const bool auto_scroll)
	{
		if (!ListBoxHeader(label, items_count, height_in_items))
			return false;

		if (auto_scroll)
		{
			SetScrollY(99999999.f);
		}

		// Assume all items have even height (= 1 line of text). If you need items of different or variable sizes you can create a custom version of ListBox() in your code without using the clipper.
		ImGuiContext& g = *GImGui;
		bool value_changed = false;
		ImGuiListClipper clipper(items_count, GetTextLineHeightWithSpacing()); // We know exactly our line height here so we pass it as a minor optimization, but generally you don't need to.
		while (clipper.Step())
			for (int i = clipper.DisplayStart; i < clipper.DisplayEnd; i++)
			{
				const bool item_selected = (i == *current_item);
				const char* item_text;
				if (!items_getter(data, i, &item_text))
					item_text = "*Unknown item*";

				PushID(i);
				if (Selectable(item_text, item_selected))
				{
					*current_item = i;
					value_changed = true;
				}
				if (item_selected)
					SetItemDefaultFocus();
				PopID();
			}
		ListBoxFooter();
		if (value_changed)
			MarkItemEdited(g.CurrentWindow->DC.LastItemId);

		return value_changed;
	}
}
#pragma endregion
