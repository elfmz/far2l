#include "OpenWith.hpp"
#include "AppProvider.hpp"
#include "lng.hpp"
#include "common.hpp"
#include "WideMB.h"
#include "utils.h"
#include "farplug-wide.h"
#include "KeyFileHelper.h"
#include "WinCompat.h"
#include <algorithm>
#include <cstdio>
#include <memory>
#include <string>
#include <vector>
#include <optional>

constexpr const char* INI_FILEPATH = "plugins/openwith/config.ini";
constexpr const char* INI_SECTION_GENERAL = "Settings";


// ****************************** Public API ******************************


void OpenWithPlugin::ProcessFiles(const std::vector<std::wstring>& filepaths)
{
	auto* provider = AppProvider::GetInstance();
	if (!provider) {
		ShowError({ GetMsg(MsgID::UnsupportedPlatform) });
		return;
	}

	std::optional<std::vector<CandidateInfo>> app_candidates;
	std::vector<FarMenuItem> menu_items;
	int active_menu_idx {};

	// Main application selection menu loop.
	while(true) {
		if (!app_candidates.has_value()) {
			app_candidates = provider->GetAppCandidates(filepaths);
			FilterOutTerminalCandidates(*app_candidates, filepaths.size());

			if ((*app_candidates).empty()) {
				ShowError({ GetMsg(MsgID::NoAppsFound), JoinStrings(provider->GetMimeTypes(), L"; ") });
				return; // No application candidates; exit the plugin.
			}

			menu_items.clear();
			menu_items.reserve((*app_candidates).size());
			for (const auto& app_candidate : *app_candidates) {
				menu_items.push_back(FarMenuItem{app_candidate.name.c_str(), 0, 0, 0});
			}
			active_menu_idx = 0;
		}

		constexpr int BREAK_KEYS[] = { VK_F3, VK_F9, MAKELONG(VK_RETURN, PKF_SHIFT), 0 };
		enum class MenuAction : int { DETAILS, SETTINGS, FORCED_LAUNCH, LAUNCH = -1 };
		int menu_break_code = -1;

		// Display the menu and get the user's selection.
		menu_items[active_menu_idx].Selected = true;
		const int selected_menu_idx = g_info.Menu(g_info.ModuleNumber, -1, -1, 0, FMENU_WRAPMODE | FMENU_SHOWAMPERSAND | FMENU_CHANGECONSOLETITLE,
											FormatMenuTitle(filepaths).c_str(), L"  Enter Shift+Enter F3 F9 Ctrl+Alt+F  ", L"Contents", BREAK_KEYS, &menu_break_code,
											menu_items.data(), static_cast<int>(menu_items.size()));
		if (selected_menu_idx == -1) {
			return; // User cancelled the menu (Esc/F10); exit the plugin.
		}
		menu_items[active_menu_idx].Selected = false;
		active_menu_idx = selected_menu_idx;
		const auto& selected_app = (*app_candidates)[selected_menu_idx];
		const auto menu_action = static_cast<MenuAction>(menu_break_code);

		switch (menu_action) {
			case MenuAction::DETAILS: {
				const auto mime_profiles = provider->GetMimeTypes();
				const auto app_info = provider->GetCandidateDetails(selected_app);
				const auto cmds = provider->ConstructLaunchCommands(selected_app, filepaths);
				const auto locations = provider->GetCandidateContextLocations(selected_app);

				bool keep_showing = true;
				while (keep_showing) {
					const auto details_dlg_result = ShowDetailsDlg(filepaths, mime_profiles, app_info, cmds, locations);
					switch (details_dlg_result.action) {
						case DetailsDlgResult::Action::Launch: {
							if (AskForLaunchConfirmation(selected_app, filepaths.size())) {
								LaunchApplication(selected_app, cmds);
								return; // Exit the plugin.
							}
							break; // Back to details dialog.
						}
						case DetailsDlgResult::Action::GoTo: {
							GoToFile(details_dlg_result.goto_target);
							return; // Exit the plugin.
						}
						case DetailsDlgResult::Action::Close: {
							keep_showing = false; // Return to the main menu.
							break;
						}
					}
				}
				break;
			}

			case MenuAction::SETTINGS: {
				auto config_result = ShowConfigDlg();
				if (config_result.should_refresh_candidates) {
					app_candidates.reset();
				}
				break;
			}

			case MenuAction::LAUNCH:
			case MenuAction::FORCED_LAUNCH: {
				if (AskForLaunchConfirmation(selected_app, filepaths.size())) {
					const auto cmds = provider->ConstructLaunchCommands(selected_app, filepaths);
					LaunchApplication(selected_app, cmds, (menu_action == MenuAction::FORCED_LAUNCH) ? LaunchMode::Forced : LaunchMode::Standard);
					return; // Exit the plugin.
				}
				break;
			}
		}
	}
}



OpenWithPlugin::ConfigDlgResult OpenWithPlugin::ShowConfigDlg()
{
	auto* provider = AppProvider::GetInstance();
	if (!provider) {
		ShowError({ GetMsg(MsgID::UnsupportedPlatform) });
		return {};
	}

	constexpr int CONFIG_DIALOG_WIDTH = 70;

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

	add_item({ DI_DOUBLEBOX, 3, current_y++, CONFIG_DIALOG_WIDTH - 4, 0, FALSE, {}, DIF_NONE, FALSE, GetMsg(MsgID::ConfigTitle), 0 });

	// ----- Add general (platform-independent) settings. -----
	auto use_external_terminal_idx          = add_checkbox(GetMsg(MsgID::UseExternalTerminal), s_use_external_terminal);
	auto no_wait_for_command_completion_idx = add_checkbox(GetMsg(MsgID::NoWaitForCommandCompletion), s_no_wait_for_command_completion);
	auto clear_selection_idx                = add_checkbox(GetMsg(MsgID::ClearSelection), s_clear_selection);

	const auto threshold_current = std::to_wstring(s_confirm_launch_threshold);
	const wchar_t* confirm_launch_label = GetMsg(MsgID::ConfirmLaunchOption);
	int confirm_launch_label_width = static_cast<int>(g_fsf.StrCellsCount(confirm_launch_label, wcslen(confirm_launch_label)));

	FarDialogItem confirm_launch_chkbx = { DI_CHECKBOX, 5, current_y, 0, current_y, FALSE, {}, DIF_NONE, FALSE, confirm_launch_label, 0 };
	confirm_launch_chkbx.Param.Selected  = s_confirm_launch;
	auto confirm_launch_chkbx_idx           = add_item(confirm_launch_chkbx);
	auto confirm_launch_edit_idx            = add_item({ DI_FIXEDIT, confirm_launch_label_width + 10, current_y, confirm_launch_label_width + 13, current_y, FALSE, {(DWORD_PTR)L"9999"}, DIF_MASKEDIT, FALSE, threshold_current.c_str(), 0 });
	current_y++;

	auto display_filename_idx               = add_checkbox(GetMsg(MsgID::DisplayFilename), s_display_filename);

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
	auto ok_btn_idx = add_item({ DI_BUTTON, 0, current_y, 0, current_y, FALSE, {}, DIF_CENTERGROUP, TRUE, GetMsg(MsgID::Ok), 0 });
	add_item({ DI_BUTTON, 0, current_y, 0, current_y, FALSE, {}, DIF_CENTERGROUP, FALSE, GetMsg(MsgID::Cancel), 0 });

	int config_dialog_height = current_y + 3;
	config_dialog_items[0].Y2 = config_dialog_height - 2;

	bool is_platform_settings_changed = false;
	bool is_platform_settings_requiring_candidate_list_refresh_changed = false;

	HANDLE config_dlg = g_info.DialogInit(g_info.ModuleNumber, -1, -1, CONFIG_DIALOG_WIDTH, config_dialog_height, L"ConfigurationDialog",
								   config_dialog_items.data(), static_cast<unsigned int>(config_dialog_items.size()), 0, 0, nullptr, 0);

	if (config_dlg != INVALID_HANDLE_VALUE) {

		int exit_code = g_info.DialogRun(config_dlg);

		// ----- Process results if "OK" was pressed. -----
		if (exit_code == static_cast<int>(ok_btn_idx)) {

			auto is_checked = [&config_dlg](size_t i) -> bool {
				return g_info.SendDlgMessage(config_dlg, DM_GETCHECK, i, 0) == BSTATE_CHECKED;
			};

			s_use_external_terminal          = is_checked(use_external_terminal_idx);
			s_no_wait_for_command_completion = is_checked(no_wait_for_command_completion_idx);
			s_clear_selection                = is_checked(clear_selection_idx);
			s_confirm_launch                 = is_checked(confirm_launch_chkbx_idx);

			const auto threshold_new = (const wchar_t*)g_info.SendDlgMessage(config_dlg, DM_GETCONSTTEXTPTR, confirm_launch_edit_idx, 0);
			s_confirm_launch_threshold = wcstol(threshold_new, nullptr, 10);
			s_confirm_launch_threshold = std::clamp(s_confirm_launch_threshold, 0, 9999);

			s_display_filename               = is_checked(display_filename_idx);

			KeyFileHelper key_writer(InMyConfig(INI_FILEPATH));
			SaveGeneralSettings(key_writer);

			// Propagate changes to dynamic platform-specific settings back to the provider.
			if (!dynamic_settings.empty()) {
				std::vector<ProviderSetting> new_platform_settings;
				new_platform_settings.reserve(dynamic_settings.size());

				for (auto& [idx, setting] : dynamic_settings) {
					bool new_value = is_checked(idx);
					if (setting.value != new_value) {
						if (setting.affects_candidates) {
							is_platform_settings_requiring_candidate_list_refresh_changed = true;
						}
						is_platform_settings_changed = true;
					}
					new_platform_settings.push_back(std::move(setting));
					new_platform_settings.back().value = new_value;
				}

				if (is_platform_settings_changed) {
					provider->SetPlatformSettings(new_platform_settings);
				}
				provider->SavePlatformSettings(key_writer);
			}

			if (!key_writer.Save(true)) {
				ShowError({GetMsg(MsgID::SaveConfigError), StrMB2Wide(InMyConfig(INI_FILEPATH))});
			}
		}
		g_info.DialogFree(config_dlg);
	}

	return { is_platform_settings_requiring_candidate_list_refresh_changed || (old_use_external_terminal != s_use_external_terminal) };
}



void OpenWithPlugin::ShowError(const std::vector<std::wstring>& error_lines)
{
	std::vector<const wchar_t*> items;
	items.reserve(error_lines.size() + 1);
	items.push_back(GetMsg(MsgID::Error));
	for (const auto &line : error_lines) {
		items.push_back(line.c_str());
	}
	g_info.Message(g_info.ModuleNumber, FMSG_WARNING | FMSG_MB_OK, L"Troubleshooting", items.data(), items.size(), 0);
}



// ****************************** Private implementation ******************************

void OpenWithPlugin::FilterOutTerminalCandidates(std::vector<CandidateInfo> &candidates, size_t file_count)
{
	if (file_count <= 1 || s_use_external_terminal) return;

	auto should_remove_candidate = [](const CandidateInfo& c) {
		return c.terminal && !c.multi_file_aware;
	};

	candidates.erase(std::remove_if(candidates.begin(), candidates.end(), should_remove_candidate),
					 candidates.end());
}



bool OpenWithPlugin::AskForLaunchConfirmation(const CandidateInfo& app, const size_t file_count)
{
	if (!s_confirm_launch || file_count <= static_cast<size_t>(s_confirm_launch_threshold)) {
		return true;
	}
	wchar_t message[255] = {};
	g_fsf.snprintf(message, ARRAYSIZE(message) - 1, GetMsg(MsgID::ConfirmLaunchMessage), file_count, app.name.c_str());
	const wchar_t* items[] = { GetMsg(MsgID::ConfirmLaunchTitle), message };
	int res = g_info.Message(g_info.ModuleNumber, FMSG_MB_YESNO, nullptr, items, ARRAYSIZE(items), 0);
	return (res == 0);
}



void OpenWithPlugin::LaunchApplication(const CandidateInfo& app, const std::vector<std::wstring>& cmds, LaunchMode launch_mode)
{
	if (cmds.empty()) {
		return;
	}

	unsigned int execute_flags = 0;
	if (app.terminal) {
		if (s_use_external_terminal || (launch_mode == LaunchMode::Forced)) {
			execute_flags |= EF_EXTERNALTERM;
		}
	} else {
		// If we have multiple commands to run, force asynchronous execution to avoid UI blocking.
		if ((cmds.size() > 1) || s_no_wait_for_command_completion || (launch_mode == LaunchMode::Forced)) {
			execute_flags |= (EF_NOWAIT | EF_HIDEOUT);
		}
	}

	for (const auto& cmd : cmds) {
		if (g_fsf.Execute(cmd.c_str(), execute_flags) == -1) {
			ShowError({GetMsg(MsgID::CannotExecute), cmd });
			break; // Stop on the first error.
		}
	}

	if (s_clear_selection) {
		g_info.Control(PANEL_ACTIVE, FCTL_UPDATEPANEL, 0, 0);
	}
}



OpenWithPlugin::DetailsDlgResult OpenWithPlugin::ShowDetailsDlg(const std::vector<std::wstring>& filepaths,
									   const std::vector<std::wstring>& unique_mime_profiles,
									   const std::vector<Field>& application_info,
									   const std::vector<std::wstring>& cmds,
									   const std::vector<CandidateContextLocation>& locations)
{
	std::vector<Field> file_info;
	if (auto file_count = filepaths.size(); file_count != 1) {
		file_info.push_back({ GetMsg(MsgID::FilesSelected), std::to_wstring(file_count)});
	}
	file_info.push_back({ GetMsg(MsgID::Filepaths), JoinStrings(filepaths, L"; ") });
	file_info.push_back({ GetMsg(MsgID::MimeProfile), JoinStrings(unique_mime_profiles, L"; ") });

	Field launch_command { GetMsg(MsgID::LaunchCommand), JoinStrings(cmds, L"; ") };

	constexpr int DETAILS_DIALOG_MIN_WIDTH = 40;
	constexpr int DETAILS_DIALOG_DESIRED_WIDTH = 90;

	const int screen_width = GetConsoleWidth();
	const int details_dialog_max_width = std::max(DETAILS_DIALOG_MIN_WIDTH, screen_width - 4);
	const int details_dialog_width = std::clamp(DETAILS_DIALOG_DESIRED_WIDTH, DETAILS_DIALOG_MIN_WIDTH, details_dialog_max_width);
	const int details_dialog_height = static_cast<int>(file_info.size() + application_info.size() + 9);

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

	details_dialog_items.push_back({ DI_DOUBLEBOX, 3, current_y++, details_dialog_width - 4, details_dialog_height - 2, FALSE, {}, 0, 0, GetMsg(MsgID::Details), 0 });
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

	details_dialog_items.push_back({ DI_BUTTON, 0, current_y, 0, current_y, TRUE, {}, DIF_CENTERGROUP, 0, GetMsg(MsgID::Close), 0 });
	details_dialog_items.back().DefaultButton = TRUE;

	const int launch_btn_idx = static_cast<int>(details_dialog_items.size());
	details_dialog_items.push_back({ DI_BUTTON, 0, current_y, 0, current_y, FALSE, {}, DIF_CENTERGROUP, 0, GetMsg(MsgID::Launch), 0 });

	const int first_location_btn_idx = static_cast<int>(details_dialog_items.size());
	for (const auto &location : locations) {
		details_dialog_items.push_back({ DI_BUTTON, 0, current_y, 0, current_y, FALSE, {}, DIF_CENTERGROUP, 0, location.title.c_str(), 0 });
	}


	HANDLE details_dlg = g_info.DialogInit(g_info.ModuleNumber, -1, -1, details_dialog_width, details_dialog_height, L"DetailsDialog",
								   details_dialog_items.data(), static_cast<unsigned int>(details_dialog_items.size()), 0, 0, nullptr, 0);

	if (details_dlg != INVALID_HANDLE_VALUE) {
		int exit_code = g_info.DialogRun(details_dlg);
		g_info.DialogFree(details_dlg);
		if (exit_code == launch_btn_idx) {
			return { DetailsDlgResult::Action::Launch };
		} else if (exit_code >= first_location_btn_idx && exit_code < first_location_btn_idx + static_cast<int>(locations.size())) {
			const int location_idx = exit_code - first_location_btn_idx;
			return { DetailsDlgResult::Action::GoTo, locations[location_idx].target_filepath };
		}
	}
	return { DetailsDlgResult::Action::Close };
}



bool OpenWithPlugin::GoToFile(const std::wstring &filepath)
{
	auto dir  = ExtractFilePath(filepath);
	if (dir.empty()) dir = L"/";
	auto name = ExtractFileName(filepath);
	if (!g_info.Control(PANEL_ACTIVE, FCTL_SETPANELDIR, 0, (LONG_PTR)dir.c_str())) {
		return false;
	}
	PanelInfo panel_info {};
	if (!g_info.Control(PANEL_ACTIVE, FCTL_GETPANELINFO, 0, (LONG_PTR)&panel_info)) {
		return false;
	}
	std::vector<unsigned char> buf;
	for (int i = 0; i < panel_info.ItemsNumber; i++) {
		int sz = g_info.Control(PANEL_ACTIVE, FCTL_GETPANELITEM, i, 0);
		if (sz <= 0) continue;
		if (static_cast<size_t>(sz) > buf.size()) {
			buf.resize(static_cast<size_t>(sz));
		}
		g_info.Control(PANEL_ACTIVE, FCTL_GETPANELITEM, i, (LONG_PTR)buf.data());
		const auto *item = reinterpret_cast<const PluginPanelItem *>(buf.data());
		if (item->FindData.lpwszFileName && name == item->FindData.lpwszFileName) {
			PanelRedrawInfo panel_ri {};
			panel_ri.CurrentItem = i;
			return g_info.Control(PANEL_ACTIVE, FCTL_REDRAWPANEL, 0, (LONG_PTR)&panel_ri) != 0;
		}
	}
	return false;
}



void OpenWithPlugin::LoadGeneralSettings(const KeyFileReadHelper& key_reader)
{
	s_use_external_terminal          = key_reader.GetInt(INI_SECTION_GENERAL, "UseExternalTerminal",        0) != 0;
	s_no_wait_for_command_completion = key_reader.GetInt(INI_SECTION_GENERAL, "NoWaitForCommandCompletion", 1) != 0;
	s_clear_selection                = key_reader.GetInt(INI_SECTION_GENERAL, "ClearSelection",             0) != 0;
	s_confirm_launch                 = key_reader.GetInt(INI_SECTION_GENERAL, "ConfirmLaunch",              1) != 0;
	s_confirm_launch_threshold       = key_reader.GetInt(INI_SECTION_GENERAL, "ConfirmLaunchThreshold",    10);
	s_confirm_launch_threshold = std::clamp(s_confirm_launch_threshold, 0, 9999);
	s_display_filename               = key_reader.GetInt(INI_SECTION_GENERAL, "DisplayFilename",            0) != 0;
}



void OpenWithPlugin::SaveGeneralSettings(KeyFileHelper& key_writer)
{
	key_writer.SetInt(INI_SECTION_GENERAL, "UseExternalTerminal",        s_use_external_terminal);
	key_writer.SetInt(INI_SECTION_GENERAL, "NoWaitForCommandCompletion", s_no_wait_for_command_completion);
	key_writer.SetInt(INI_SECTION_GENERAL, "ClearSelection",             s_clear_selection);
	key_writer.SetInt(INI_SECTION_GENERAL, "ConfirmLaunch",              s_confirm_launch);
	key_writer.SetInt(INI_SECTION_GENERAL, "ConfirmLaunchThreshold",     s_confirm_launch_threshold);
	key_writer.SetInt(INI_SECTION_GENERAL, "DisplayFilename",            s_display_filename);
}



std::wstring OpenWithPlugin::JoinStrings(const std::vector<std::wstring>& strings, const std::wstring& delimiter)
{
	if (strings.empty()) {
		return L"";
	}
	std::wstring joined = strings[0];
	for (size_t i = 1; i < strings.size(); ++i) {
		joined += delimiter;
		joined += strings[i];
	}
	return joined;
}



size_t OpenWithPlugin::GetLabelCellWidth(const Field& field)
{
	return g_fsf.StrCellsCount(field.label.c_str(), field.label.size());
}



size_t OpenWithPlugin::GetMaxLabelCellWidth(const std::vector<Field>& fields)
{
	size_t max_width = 0;
	for (const auto& field : fields) {
		max_width = std::max(max_width, GetLabelCellWidth(field));
	}
	return max_width;
}



int OpenWithPlugin::GetConsoleWidth()
{
	SMALL_RECT rect;
	if (g_info.AdvControl(g_info.ModuleNumber, ACTL_GETFARRECT, &rect, nullptr)) {
		return rect.Right - rect.Left + 1;
	}
	return 80;
}



std::wstring OpenWithPlugin::FormatMenuTitle(const std::vector<std::wstring>& filepaths)
{
	if (!s_display_filename) {
		return GetMsg(MsgID::ChooseApplication);
	}

	std::wstring title = GetMsg(MsgID::OpenWithFor);

	if (filepaths.size() != 1) {
		title += std::to_wstring(filepaths.size()) + GetMsg(MsgID::File_s);
		return title;
	}

	auto filename = ExtractFileName(filepaths.front());

	constexpr int menu_ui_overhead_cells = 1 + 4 + 1 + 1 + 4 + 1;

	const int title_cells = static_cast<int>(g_fsf.StrCellsCount(title.c_str(), title.size()));
	const int max_filename_cells = std::max(1, GetConsoleWidth() - title_cells - menu_ui_overhead_cells);

	g_fsf.TruncStr(filename.data(), max_filename_cells);
	filename.resize(wcslen(filename.c_str()));

	title += L'"';
	title += filename;
	title += L'"';

	return title;
}



const wchar_t* GetMsg(MsgID msg_id)
{
	return g_info.GetMsg(g_info.ModuleNumber, static_cast<int>(msg_id));
}


// ****************************** Plugin entry points ******************************

PluginStartupInfo g_info;
FarStandardFunctions g_fsf;

SHAREDSYMBOL void WINAPI SetStartupInfoW(const struct PluginStartupInfo *Info)
{
	g_info = *Info;
	g_fsf = *Info->FSF;
	g_info.FSF = &g_fsf;

	KeyFileReadHelper key_reader(InMyConfig(INI_FILEPATH));
	OpenWithPlugin::LoadGeneralSettings(key_reader);
	if (auto* provider = AppProvider::GetInstance()) {
		provider->LoadPlatformSettings(key_reader);
	}
}


SHAREDSYMBOL void WINAPI GetPluginInfoW(struct PluginInfo *Info)
{
	Info->StructSize = sizeof(struct PluginInfo);
	Info->SysID = 0x93CDEF19;
	Info->Flags = 0;
	static const wchar_t *s_menu_strings[1];
	s_menu_strings[0] = GetMsg(MsgID::PluginTitle);
	Info->PluginMenuStrings = s_menu_strings;
	Info->PluginMenuStringsNumber = ARRAYSIZE(s_menu_strings);
	static const wchar_t *s_config_strings[1];
	s_config_strings[0] = GetMsg(MsgID::PluginTitle);
	Info->PluginConfigStrings = s_config_strings;
	Info->PluginConfigStringsNumber = ARRAYSIZE(s_config_strings);
	Info->CommandPrefix = nullptr;
}


SHAREDSYMBOL HANDLE WINAPI OpenPluginW(int OpenFrom, INT_PTR Item)
{
	if (OpenFrom != OPEN_PLUGINSMENU && OpenFrom != (OPEN_FROMMACRO | MACROAREA_SHELL)) {
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
		OpenWithPlugin::ShowError({GetMsg(MsgID::NotRealNames)});
		return INVALID_HANDLE_VALUE;
	}

	int dir_size = g_info.Control(PANEL_ACTIVE, FCTL_GETPANELDIR, 0, 0);
	if (dir_size <= 0) {
		return INVALID_HANDLE_VALUE;
	}

	auto dir_buf = std::make_unique<wchar_t[]>(dir_size);
	if (!g_info.Control(PANEL_ACTIVE, FCTL_GETPANELDIR, dir_size, (LONG_PTR)dir_buf.get())) {
		return INVALID_HANDLE_VALUE;
	}

	std::wstring base_path(dir_buf.get());

	auto path_prefix = base_path;
	if (!base_path.empty() && base_path.back() != L'/') {
		path_prefix += L'/';
	}

	std::vector<std::wstring> selected_filepaths;

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
				selected_filepaths.push_back(path_prefix + pi_item->FindData.lpwszFileName);
			}
		}
	} else {
		// Special case: cursor on ".." with no items selected.
		if (!base_path.empty()) {
			selected_filepaths.push_back(base_path);
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
