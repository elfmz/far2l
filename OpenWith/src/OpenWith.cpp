#include "OpenWith.hpp"
#include "AppProvider.hpp"
#include "farplug-wide.h"
#include "KeyFileHelper.h"
#include "WinCompat.h"
#include "WinPort.h"
#include "lng.hpp"
#include "common.hpp"
#include "utils.h"
#include <algorithm>
#include <cstdio>
#include <memory>
#include <string>
#include <vector>
#include <optional>

#define INI_LOCATION InMyConfig("plugins/openwith/config.ini")
#define INI_SECTION  "Settings"

namespace OpenWith {


// ****************************** Public API ******************************

// Main workflow orchestrator: resolves application candidates via AppProvider,
// displays the selection menu, and handles user actions: F3 (details), F9 (settings), Enter (launch).
void OpenWithPlugin::ProcessFiles(const std::vector<std::wstring>& filepaths)
{
	if (filepaths.empty()) {
		return;
	}

	auto provider = AppProvider::CreateAppProvider(&GetMsg);

	int active_menu_idx {};

	std::optional<std::vector<CandidateInfo>> app_candidates;
	std::vector<FarMenuItem> menu_items;

	// Main application selection menu loop.
	while(true) {
		if (!app_candidates.has_value()) {
			app_candidates = provider->GetAppCandidates(filepaths);
			FilterOutTerminalCandidates(*app_candidates, filepaths.size());
			menu_items.clear();
			menu_items.reserve((*app_candidates).size());
			for (const auto& app_candidate : *app_candidates) {
				FarMenuItem menu_item = {};
				menu_item.Text = app_candidate.name.c_str();
				menu_items.push_back(menu_item);
			}
			active_menu_idx = 0;
		}

		if ((*app_candidates).empty()) {
			ShowError({ GetMsg(MNoAppsFound), JoinStrings(provider->GetMimeTypes(), L"; ") });
			return; // No application candidates; exit the plugin.
		}

		constexpr int BREAK_KEYS[] = {VK_F3, VK_F9, 0};
		constexpr int KEY_F3_DETAILS = 0;
		constexpr int KEY_F9_SETTINGS = 1;
		int menu_break_code = -1;

		// Display the menu and get the user's selection.
		menu_items[active_menu_idx].Selected = true;
		int selected_menu_idx = g_info.Menu(g_info.ModuleNumber, -1, -1, 0, FMENU_WRAPMODE | FMENU_SHOWAMPERSAND | FMENU_CHANGECONSOLETITLE,
											GetMsg(MChooseApplication), L"F3 F9 Ctrl+Alt+F", L"Contents", BREAK_KEYS, &menu_break_code,
											menu_items.data(), static_cast<int>(menu_items.size()));
		menu_items[active_menu_idx].Selected = false;

		if (selected_menu_idx == -1) {
			return; // User cancelled the menu; exit the plugin.
		}

		active_menu_idx = selected_menu_idx;
		const auto& selected_app = (*app_candidates)[selected_menu_idx];

		if (menu_break_code == KEY_F3_DETAILS) {
			const auto mime_profiles = provider->GetMimeTypes();
			const auto app_info = provider->GetCandidateDetails(selected_app);
			const auto cmds = provider->ConstructLaunchCommands(selected_app, filepaths);
			// Repeat until user either launches the application or closes the dialog to go back.
			while (true) {
				if (ShowDetailsDlg(filepaths, mime_profiles, app_info, cmds) == DetailsDlgResult::Launch) {
					if (AskForLaunchConfirmation(selected_app, filepaths.size())) {
						LaunchApplication(selected_app, cmds);
						return; // Application launched; exit the plugin.
					}
				} else {
					break; // User clicked "Close"; return to the main menu.
				}
			}

		} else if (menu_break_code == KEY_F9_SETTINGS) {
			auto config_result = ShowConfigDlg();
			if (config_result.is_platform_settings_changed) {
				provider->LoadPlatformSettings();
			}
			if (config_result.should_refresh_candidates) {
				app_candidates.reset();
			}

		} else { // Enter to launch.
			if (AskForLaunchConfirmation(selected_app, filepaths.size())) {
				const auto cmds = provider->ConstructLaunchCommands(selected_app, filepaths);
				LaunchApplication(selected_app, cmds);
				return; // Application launched; exit the plugin.
			}
		}
	}
}



// Displays the configuration dialog containing both platform-independent (plugin-wide)
// and platform-specific settings fetched dynamically from the current AppProvider.
OpenWithPlugin::ConfigDlgResult OpenWithPlugin::ShowConfigDlg()
{
	constexpr int CONFIG_DIALOG_WIDTH = 70;

	LoadGeneralSettings();

	// Create a temporary provider to access platform-specific settings (they are loaded automatically).
	auto provider = AppProvider::CreateAppProvider(GetMsg);

	const bool old_use_external_terminal = s_use_external_terminal;
	const auto old_platform_settings = provider->GetPlatformSettings();

	std::vector<FarDialogItem> config_dialog_items;
	int current_y = 1;

	auto add_item = [&config_dialog_items](const FarDialogItem& item) -> size_t {
		config_dialog_items.push_back(item);
		auto item_idx = config_dialog_items.size() - 1;
		return item_idx;
	};

	auto add_checkbox = [&add_item, &current_y](const wchar_t* text, bool is_checked, bool is_disabled = false) -> size_t {
		FarDialogItem chkbox = { DI_CHECKBOX, 5, current_y, 0, current_y, FALSE, {}, is_disabled ? DIF_DISABLE : DIF_NONE, FALSE, text, 0 };
		chkbox.Param.Selected = is_checked;
		auto item_idx = add_item(chkbox);
		current_y++;
		return item_idx;
	};

	auto add_separator = [&add_item, &current_y]() -> size_t {
		auto item_idx = add_item({ DI_TEXT, 5, current_y, 0, current_y, FALSE, {}, DIF_SEPARATOR, FALSE, L"", 0 });
		current_y++;
		return item_idx;
	};

	add_item({ DI_DOUBLEBOX, 3, current_y++, CONFIG_DIALOG_WIDTH - 4, 0, FALSE, {}, DIF_NONE, FALSE, GetMsg(MConfigTitle), 0 });

	// ----- Add platform-independent settings. -----
	auto use_external_terminal_idx          = add_checkbox(GetMsg(MUseExternalTerminal), s_use_external_terminal);
	auto no_wait_for_command_completion_idx = add_checkbox(GetMsg(MNoWaitForCommandCompletion), s_no_wait_for_command_completion);
	auto clear_selection_idx                = add_checkbox(GetMsg(MClearSelection), s_clear_selection);

	auto threshold_str = std::to_wstring(s_confirm_launch_threshold);
	const wchar_t* confirm_launch_label = GetMsg(MConfirmLaunchOption);
	int confirm_launch_label_width = static_cast<int>(g_fsf.StrCellsCount(confirm_launch_label, wcslen(confirm_launch_label)));

	FarDialogItem confirm_launch_chkbx = { DI_CHECKBOX, 5, current_y, 0, current_y, FALSE, {}, DIF_NONE, FALSE, confirm_launch_label, 0 };
	confirm_launch_chkbx.Param.Selected  = s_confirm_launch;
	auto confirm_launch_chkbx_idx = add_item(confirm_launch_chkbx);
	auto confirm_launch_edit_idx  = add_item({ DI_FIXEDIT, confirm_launch_label_width + 10, current_y, confirm_launch_label_width + 13, current_y, FALSE, {(DWORD_PTR)L"9999"}, DIF_MASKEDIT, FALSE, threshold_str.c_str(), 0 });
	current_y++;

	// ----- Add platform-specific settings. -----
	std::vector<std::pair<size_t, ProviderSetting>> dynamic_settings;
	dynamic_settings.reserve(old_platform_settings.size());

	if (!old_platform_settings.empty()) {
		add_separator();
		for (const auto& s : old_platform_settings) {
			dynamic_settings.emplace_back(add_checkbox(s.display_name.c_str(), s.value, s.disabled), s);
		}
	}

	add_separator();
	auto ok_btn_idx = add_item({ DI_BUTTON, 0, current_y, 0, current_y, FALSE, {}, DIF_CENTERGROUP, TRUE, GetMsg(MOk), 0 });
	add_item({ DI_BUTTON, 0, current_y, 0, current_y, FALSE, {}, DIF_CENTERGROUP, FALSE, GetMsg(MCancel), 0 });

	int config_dialog_height = current_y + 3;
	config_dialog_items[0].Y2 = config_dialog_height - 2;

	HANDLE dlg = g_info.DialogInit(g_info.ModuleNumber, -1, -1, CONFIG_DIALOG_WIDTH, config_dialog_height, L"ConfigurationDialog",
								   config_dialog_items.data(), static_cast<unsigned int>(config_dialog_items.size()), 0, 0, nullptr, 0);

	bool is_platform_settings_changed = false;
	bool is_platform_settings_requiring_candidate_list_refresh_changed = false;

	if (dlg != INVALID_HANDLE_VALUE) {

		int exit_code = g_info.DialogRun(dlg);

		// ----- Process results if "OK" was pressed. -----
		if (exit_code == static_cast<int>(ok_btn_idx)) {

			auto is_checked = [&dlg](size_t i) -> bool {
				return g_info.SendDlgMessage(dlg, DM_GETCHECK, i, 0) == BSTATE_CHECKED;
			};

			s_use_external_terminal          = is_checked(use_external_terminal_idx);
			s_no_wait_for_command_completion = is_checked(no_wait_for_command_completion_idx);
			s_clear_selection                = is_checked(clear_selection_idx);
			s_confirm_launch                 = is_checked(confirm_launch_chkbx_idx);

			auto threshold_str = (const wchar_t*)g_info.SendDlgMessage(dlg, DM_GETCONSTTEXTPTR, confirm_launch_edit_idx, 0);
			s_confirm_launch_threshold = wcstol(threshold_str, nullptr, 10);

			SaveGeneralSettings();

			// Propagate changes to dynamic platform-specific settings back to the provider.
			if (!dynamic_settings.empty()) {
				std::vector<ProviderSetting> new_platform_settings;
				new_platform_settings.reserve(dynamic_settings.size());

				for (const auto& [idx, setting] : dynamic_settings) {
					bool new_value = is_checked(idx);
					if (setting.value != new_value) {
						if (setting.affects_candidates) {
							is_platform_settings_requiring_candidate_list_refresh_changed = true;
						}
						is_platform_settings_changed = true;
					}
					new_platform_settings.push_back({ setting.internal_key, setting.display_name, new_value });
				}

				if (is_platform_settings_changed) {
					provider->SetPlatformSettings(new_platform_settings);
					provider->SavePlatformSettings();
				}
			}
		}
		g_info.DialogFree(dlg);
	}

	return { is_platform_settings_changed,
			(is_platform_settings_requiring_candidate_list_refresh_changed || (old_use_external_terminal != s_use_external_terminal)) };
}



void OpenWithPlugin::ShowError(const std::vector<std::wstring>& error_lines)
{
	std::vector<const wchar_t*> items;
	items.reserve(error_lines.size() + 2);
	items.push_back(GetMsg(MError));
	for (const auto &line : error_lines) {
		items.push_back(line.c_str());
	}
	items.push_back(GetMsg(MOk));
	g_info.Message(g_info.ModuleNumber, FMSG_WARNING, nullptr, items.data(), items.size(), 1);
}



// ****************************** Private implementation ******************************

// Excludes terminal‑based applications that are not multi‑file aware when multiple files are selected
// and the built‑in far2l terminal is used, since multiple concurrent instances cannot be managed.
void OpenWithPlugin::FilterOutTerminalCandidates(std::vector<CandidateInfo> &candidates, size_t file_count)
{
	if (file_count > 1 && !s_use_external_terminal) {
		candidates.erase(
			std::remove_if(candidates.begin(), candidates.end(),
						   [](const CandidateInfo& c) {
							   return c.terminal && !c.multi_file_aware;
						   }),
			candidates.end());
	}
}



// Prompts the user for confirmation if the number of files exceeds the configured threshold.
bool OpenWithPlugin::AskForLaunchConfirmation(const CandidateInfo& app, const size_t file_count)
{
	if (!s_confirm_launch || file_count <= static_cast<size_t>(s_confirm_launch_threshold)) {
		return true;
	}
	wchar_t message[255] = {};
	g_fsf.snprintf(message, ARRAYSIZE(message) - 1, GetMsg(MConfirmLaunchMessage), file_count, app.name.c_str());
	const wchar_t* items[] = { GetMsg(MConfirmLaunchTitle), message };
	int res = g_info.Message(g_info.ModuleNumber, FMSG_MB_YESNO, nullptr, items, ARRAYSIZE(items), 2);
	return (res == 0);
}



// Executes one or more command lines to launch the selected application to open the provided files.
void OpenWithPlugin::LaunchApplication(const CandidateInfo& app, const std::vector<std::wstring>& cmds)
{
	if (cmds.empty()) {
		return;
	}

	// If we have multiple commands to run, force asynchronous execution to avoid UI blocking.
	bool force_no_wait = cmds.size() > 1;

	unsigned int execute_flags = 0;
	if (app.terminal) {
		if (s_use_external_terminal) {
			execute_flags |= EF_EXTERNALTERM;
		}
	} else {
		if (s_no_wait_for_command_completion || force_no_wait) {
			execute_flags |= (EF_NOWAIT | EF_HIDEOUT);
		}
	}

	for (const auto& cmd : cmds) {
		if (g_fsf.Execute(cmd.c_str(), execute_flags) == -1) {
			ShowError({GetMsg(MCannotExecute), cmd });
			break; // Stop on the first error.
		}
	}

	if (s_clear_selection) {
		g_info.Control(PANEL_ACTIVE, FCTL_UPDATEPANEL, 0, 0);
	}
}



// Displays detailed information about selected file(s), the candidate application,
// and the resolved launch command that will be used to launch it.
OpenWithPlugin::DetailsDlgResult OpenWithPlugin::ShowDetailsDlg(const std::vector<std::wstring>& filepaths,
									   const std::vector<std::wstring>& unique_mime_profiles,
									   const std::vector<Field>& application_info,
									   const std::vector<std::wstring>& cmds)
{
	std::vector<Field> file_info;
	// For a single file, show its full path. For multiple files, show a summary count.
	if (filepaths.size() == 1) {
		file_info.push_back({ GetMsg(MPathname), filepaths[0] });
	} else {
		auto count_msg = std::wstring(GetMsg(MFilesSelected)) + std::to_wstring(filepaths.size());
		file_info.push_back({ GetMsg(MPathname), count_msg });
	}
	file_info.push_back({ GetMsg(MMimeProfile), JoinStrings(unique_mime_profiles, L"; ") });
	Field launch_command { GetMsg(MLaunchCommand), JoinStrings(cmds, L"; ") };

	constexpr int DETAILS_DIALOG_MIN_WIDTH = 40;
	constexpr int DETAILS_DIALOG_DESIRED_WIDTH = 90;

	const int screen_width = GetScreenWidth();
	const int details_dialog_max_width = std::max(DETAILS_DIALOG_MIN_WIDTH, screen_width - 4);
	const int details_dialog_width = std::clamp(DETAILS_DIALOG_DESIRED_WIDTH, DETAILS_DIALOG_MIN_WIDTH, details_dialog_max_width);
	const int details_dialog_height = static_cast<int>(file_info.size() + application_info.size() + 9);

	// Calculate the column width for labels to align fields nicely.
	const auto max_label_cell_width = static_cast<int>(std::max({
		GetMaxLabelCellWidth(file_info),
		GetMaxLabelCellWidth(application_info),
		GetLabelCellWidth(launch_command)
	}));

	const int label_end_x = max_label_cell_width + 4;
	const int edit_start_x = max_label_cell_width + 6;
	const int edit_end_x = details_dialog_width - 6;

	std::vector<FarDialogItem> details_dialog_items;
	details_dialog_items.reserve(file_info.size() * 2 + application_info.size() * 2 + 8);

	int current_y = 1;

	// Lambda to add a label/value pair.
	auto add_field_row = [&details_dialog_items, &current_y, label_end_x, edit_start_x, edit_end_x](const Field& field) {
		int label_start_x = label_end_x - static_cast<int>(GetLabelCellWidth(field)) + 1;
		details_dialog_items.push_back({ DI_TEXT, label_start_x, current_y, label_end_x, current_y, FALSE, {}, 0, 0, field.label.c_str(), 0 });
		details_dialog_items.push_back({ DI_EDIT, edit_start_x, current_y, edit_end_x, current_y, FALSE, {}, DIF_READONLY | DIF_SELECTONENTRY, 0, field.content.c_str(), 0 });
		current_y++;
	};

	auto add_separator = [&details_dialog_items, &current_y]() {
		details_dialog_items.push_back({ DI_TEXT, 5, current_y, 0, current_y, FALSE, {}, DIF_SEPARATOR, 0, L"", 0 });
		current_y++;
	};

	details_dialog_items.push_back({ DI_DOUBLEBOX, 3, current_y++, details_dialog_width - 4, details_dialog_height - 2, FALSE, {}, 0, 0, GetMsg(MDetails), 0 });
	for (const auto& field : file_info) {
		add_field_row(field);
	}
	add_separator();
	for (const auto& field : application_info) {
		add_field_row(field);
	}
	add_separator();
	add_field_row(launch_command);
	add_separator();
	details_dialog_items.push_back({ DI_BUTTON, 0, current_y, 0, current_y, TRUE, {}, DIF_CENTERGROUP, 0, GetMsg(MClose), 0 });
	details_dialog_items.back().DefaultButton = TRUE;
	details_dialog_items.push_back({ DI_BUTTON, 0, current_y, 0, current_y, FALSE, {}, DIF_CENTERGROUP, 0, GetMsg(MLaunch), 0 });
	const int launch_btn_idx = static_cast<int>(details_dialog_items.size()) - 1;

	HANDLE dlg = g_info.DialogInit(g_info.ModuleNumber, -1, -1, details_dialog_width, details_dialog_height, L"DetailsDialog",
								   details_dialog_items.data(), static_cast<unsigned int>(details_dialog_items.size()), 0, 0, nullptr, 0);

	if (dlg != INVALID_HANDLE_VALUE) {
		int exit_code = g_info.DialogRun(dlg);
		g_info.DialogFree(dlg);
		if (exit_code == launch_btn_idx) {
			return DetailsDlgResult::Launch;
		}
	}
	return DetailsDlgResult::Close;
}



// Loads platform-independent configuration from the INI file.
void OpenWithPlugin::LoadGeneralSettings()
{
	KeyFileReadSection kfh(INI_LOCATION, INI_SECTION);
	s_use_external_terminal = kfh.GetInt("UseExternalTerminal", 0) != 0;
	s_no_wait_for_command_completion = kfh.GetInt("NoWaitForCommandCompletion", 1) != 0;
	s_clear_selection = kfh.GetInt("ClearSelection", 0) != 0;
	s_confirm_launch = kfh.GetInt("ConfirmLaunch", 1) != 0;
	s_confirm_launch_threshold = kfh.GetInt("ConfirmLaunchThreshold", 10);
	s_confirm_launch_threshold = std::clamp(s_confirm_launch_threshold, 0, 9999);
}



// Saves current platform-independent configuration to the INI file.
void OpenWithPlugin::SaveGeneralSettings()
{
	KeyFileHelper kfh(INI_LOCATION);
	kfh.SetInt(INI_SECTION, "UseExternalTerminal", s_use_external_terminal);
	kfh.SetInt(INI_SECTION, "NoWaitForCommandCompletion", s_no_wait_for_command_completion);
	kfh.SetInt(INI_SECTION, "ClearSelection", s_clear_selection);
	kfh.SetInt(INI_SECTION, "ConfirmLaunch", s_confirm_launch);
	s_confirm_launch_threshold = std::clamp(s_confirm_launch_threshold, 0, 9999);
	kfh.SetInt(INI_SECTION, "ConfirmLaunchThreshold", s_confirm_launch_threshold);
	if (!kfh.Save()) {
		ShowError({ GetMsg(MSaveConfigError) });
	}
}



// Joins a vector of strings with a specified delimiter.
std::wstring OpenWithPlugin::JoinStrings(const std::vector<std::wstring>& vec, const std::wstring& delimiter)
{
	if (vec.empty()) {
		return L"";
	}
	std::wstring result = vec[0];
	for (size_t i = 1; i < vec.size(); ++i) {
		result += delimiter;
		result += vec[i];
	}
	return result;
}



// Calculates the width of a Field label in console cells.
// Used in Details dialog to align "Edit" controls.
size_t OpenWithPlugin::GetLabelCellWidth(const Field& field)
{
	return g_fsf.StrCellsCount(field.label.c_str(), field.label.size());
}



// Determines the maximum label width (in cells) in a vector of Fields.
// Used in Details dialog to align "Edit" controls.
size_t OpenWithPlugin::GetMaxLabelCellWidth(const std::vector<Field>& fields)
{
	size_t max_width = 0;
	for (const auto& field : fields) {
		max_width = std::max(max_width, GetLabelCellWidth(field));
	}
	return max_width;
}



// Retrieves the current console width.
int OpenWithPlugin::GetScreenWidth()
{
	SMALL_RECT rect;
	if (g_info.AdvControl(g_info.ModuleNumber, ACTL_GETFARRECT, &rect, 0)) {
		return rect.Right - rect.Left + 1;
	}
	return 80;
}



// Retrieves a localized message string from the language file by its ID.
const wchar_t* GetMsg(int msg_id)
{
	return g_info.GetMsg(g_info.ModuleNumber, msg_id);
}


// ****************************** Static member initialization. ******************************

bool OpenWithPlugin::s_use_external_terminal = false;
bool OpenWithPlugin::s_no_wait_for_command_completion = true;
bool OpenWithPlugin::s_clear_selection = false;
bool OpenWithPlugin::s_confirm_launch = true;
int OpenWithPlugin::s_confirm_launch_threshold = 10;

// ****************************** Plugin entry points ******************************

PluginStartupInfo g_info;
FarStandardFunctions g_fsf;

SHAREDSYMBOL void WINAPI SetStartupInfoW(const struct PluginStartupInfo *Info)
{
	g_info = *Info;
	g_fsf = *Info->FSF;
	g_info.FSF = &g_fsf;
	OpenWithPlugin::LoadGeneralSettings();
}


SHAREDSYMBOL void WINAPI GetPluginInfoW(struct PluginInfo *Info)
{
	Info->StructSize = sizeof(struct PluginInfo);
	Info->Flags = 0;
	static const wchar_t *s_menu_strings[1];
	s_menu_strings[0] = GetMsg(MPluginTitle);
	Info->PluginMenuStrings = s_menu_strings;
	Info->PluginMenuStringsNumber = ARRAYSIZE(s_menu_strings);
	static const wchar_t *s_config_strings[1];
	s_config_strings[0] = GetMsg(MPluginTitle);
	Info->PluginConfigStrings = s_config_strings;
	Info->PluginConfigStringsNumber = ARRAYSIZE(s_config_strings);
	Info->CommandPrefix = nullptr;
}


// Main entry point when the plugin is invoked from the plugins menu (F11).
// Validates the active panel state, collects selected items, and initiates the processing workflow.
SHAREDSYMBOL HANDLE WINAPI OpenPluginW(int OpenFrom, INT_PTR Item)
{
	if (OpenFrom != OPEN_PLUGINSMENU) {
		return INVALID_HANDLE_VALUE;
	}

	PanelInfo pi {};

	if (!g_info.Control(PANEL_ACTIVE, FCTL_GETPANELINFO, 0, (LONG_PTR)&pi)) {
		return INVALID_HANDLE_VALUE;
	}

	if (pi.PanelType != PTYPE_FILEPANEL || pi.ItemsNumber <= 0) {
		return INVALID_HANDLE_VALUE;
	}

	if (pi.Plugin && !(pi.Flags & PFLAGS_REALNAMES)) {
		OpenWithPlugin::ShowError({GetMsg(MNotRealNames)});
		return INVALID_HANDLE_VALUE;
	}

	std::vector<std::wstring> selected_filepaths;
	int dir_size = g_info.Control(PANEL_ACTIVE, FCTL_GETPANELDIR, 0, 0);
	if (dir_size <= 0) {
		return INVALID_HANDLE_VALUE;
	}

	auto dir_buf = std::make_unique<wchar_t[]>(dir_size);
	if (!g_info.Control(PANEL_ACTIVE, FCTL_GETPANELDIR, dir_size, (LONG_PTR)dir_buf.get())) {
		return INVALID_HANDLE_VALUE;
	}

	std::wstring base_path(dir_buf.get());
	if (!base_path.empty() && base_path.back() != L'/') {
		base_path += L'/';
	}

	// If no specific selection exists, 'SelectedItemsNumber' is 1, and the item is the one under the cursor.
	if (pi.SelectedItemsNumber > 0) {
		selected_filepaths.reserve(pi.SelectedItemsNumber);
		for (int i = 0; i < pi.SelectedItemsNumber; ++i) {
			int item_size = g_info.Control(PANEL_ACTIVE, FCTL_GETSELECTEDPANELITEM, i, 0);
			if (item_size <= 0) {
				continue;
			}
			auto item_buf = std::make_unique<unsigned char[]>(item_size);
			PluginPanelItem* pi_item = reinterpret_cast<PluginPanelItem*>(item_buf.get());
			if (g_info.Control(PANEL_ACTIVE, FCTL_GETSELECTEDPANELITEM, i, (LONG_PTR)pi_item) && pi_item->FindData.lpwszFileName) {
				selected_filepaths.push_back(base_path + pi_item->FindData.lpwszFileName);
			}
		}
	}

	if (!selected_filepaths.empty()) {
		OpenWithPlugin::ProcessFiles(selected_filepaths);
	}

	// Plugin performs an action and exits, rather than creating a new panel instance (like a VFS plugin).
	return INVALID_HANDLE_VALUE;
}


SHAREDSYMBOL int WINAPI ConfigureW(int ItemNumber)
{
	OpenWithPlugin::ShowConfigDlg();
	return FALSE;
}


SHAREDSYMBOL void WINAPI ExitFARW()
{
}


SHAREDSYMBOL int WINAPI GetMinFarVersionW()
{
	return FARMANAGERVERSION;
}

} // namespace OpenWith
