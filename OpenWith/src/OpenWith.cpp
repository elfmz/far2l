#include "OpenWith.hpp"
#include "AppProvider.hpp"
#include "common.hpp"
#include "lng.hpp"
#include "farplug-wide.h"
#include "KeyFileHelper.h"
#include "utils.h"
#include "WideMB.h"
#include "WinCompat.h"
#include <algorithm>
#include <cstdio>
#include <iterator>
#include <memory>
#include <optional>
#include <string>
#include <vector>


namespace
{
	constexpr const char* INI_SECTION_GENERAL = "Settings";
}

namespace openwith
{
	PluginStartupInfo g_info;
	FarStandardFunctions g_fsf;

	// ****************************** PUBLIC API ******************************


	void Plugin::ProcessFiles(const std::vector<std::wstring>& filepaths)
	{
		auto* provider = AppProvider::GetInstance();
		if (!provider) {
			ShowErrorDlg({ GetMsg(MsgID::UnsupportedPlatform) });
			return;
		}

		std::optional<std::vector<CandidateInfo>> app_candidates;
		std::vector<FarMenuItem> menu_items;
		int active_menu_idx {};

		constexpr int BREAK_KEYS[] = { VK_F3, VK_F9, MAKELONG(VK_RETURN, PKF_SHIFT), 0 };
		enum class MenuAction : int { DETAILS, SETTINGS, FORCED_LAUNCH, LAUNCH = -1 };

		// Main application selection menu loop.
		while (true) {
			if (!app_candidates.has_value()) {
				app_candidates = provider->GetAppCandidates(filepaths);
				FilterOutTerminalCandidates(*app_candidates, filepaths.size());

				if ((*app_candidates).empty()) {
					ShowErrorDlg({ GetMsg(MsgID::NoAppsFound), JoinStrings(provider->GetMimeTypes(), L"; ") });
					return; // No application candidates; exit the plugin.
				}

				// NOTE: FarMenuItem stores a raw const wchar_t* — no copy is made.
				// The pointers here point directly into the name strings inside "app_candidates".
				// Do not modify, reset, or reallocate "app_candidates" while "menu_items" is alive.

				menu_items.clear();
				menu_items.reserve((*app_candidates).size());
				for (const auto& app_candidate : *app_candidates) {
					menu_items.push_back(FarMenuItem{app_candidate.name.c_str(), 0, 0, 0});
				}
				active_menu_idx = 0;
			}

			int menu_break_code = -1;

			// Display the menu and get the user's selection.
			menu_items[active_menu_idx].Selected = true;
			const int selected_menu_idx = g_info.Menu(g_info.ModuleNumber, -1, -1, 0, FMENU_WRAPMODE | FMENU_SHOWAMPERSAND | FMENU_CHANGECONSOLETITLE,
												FormatMenuTitle(filepaths).c_str(), L"  Enter Shift+Enter F3 F9 Ctrl+Alt+F  ", L"Contents", BREAK_KEYS,
												&menu_break_code, menu_items.data(), static_cast<int>(menu_items.size()));
			menu_items[active_menu_idx].Selected = false;

			if (selected_menu_idx == -1) {
				return; // User cancelled the menu (Esc/F10); exit the plugin.
			}

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



	Plugin::ConfigDlgResult Plugin::ShowConfigDlg()
	{
		auto* provider = AppProvider::GetInstance();
		if (!provider) {
			ShowErrorDlg({ GetMsg(MsgID::UnsupportedPlatform) });
			return {};
		}

		constexpr int CONFIG_DIALOG_WIDTH = 70;

		const bool old_use_external_terminal = s_use_external_terminal;
		auto platform_settings = provider->GetPlatformSettings();

		std::vector<FarDialogItem> config_dialog_items;
		int current_y = 1;

		auto add_item = [&config_dialog_items](const FarDialogItem& item) -> int {
			config_dialog_items.push_back(item);
			auto item_idx = static_cast<int>(config_dialog_items.size() - 1);
			return item_idx;
		};

		auto add_checkbox = [&add_item, &current_y](const wchar_t* text, bool is_checked, bool is_disabled = false) -> int {
			FarDialogItem chkbox = { DI_CHECKBOX, DlgLayout::H_SIDE_OVERHEAD, current_y, 0, current_y, FALSE, {},
									 is_disabled ? DIF_DISABLE : DIF_NONE, FALSE, text, 0 };
			chkbox.Param.Selected = is_checked;
			auto item_idx = add_item(chkbox);
			current_y++;
			return item_idx;
		};

		auto add_separator = [&add_item, &current_y]() -> int {
			auto item_idx = add_item({ DI_TEXT, 0, current_y, 0, current_y, FALSE, {}, DIF_SEPARATOR, FALSE, L"", 0 });
			current_y++;
			return item_idx;
		};

		add_item({ DI_DOUBLEBOX, DlgLayout::H_OUTER_MARGIN, current_y++, CONFIG_DIALOG_WIDTH - DlgLayout::H_OUTER_MARGIN - 1,
				   0, FALSE, {}, DIF_NONE, FALSE, GetMsg(MsgID::ConfigTitle), 0 });

		// ----- Add general (platform-independent) settings. -----
		auto use_external_terminal_idx          = add_checkbox(GetMsg(MsgID::UseExternalTerminal), s_use_external_terminal);
		auto no_wait_for_command_completion_idx = add_checkbox(GetMsg(MsgID::NoWaitForCommandCompletion), s_no_wait_for_command_completion);
		auto clear_selection_idx                = add_checkbox(GetMsg(MsgID::ClearSelection), s_clear_selection);

		const auto threshold_current = std::to_wstring(s_confirm_launch_threshold);
		const wchar_t* confirm_launch_label = GetMsg(MsgID::ConfirmLaunchOption);
		const int confirm_launch_label_width = CalculateVisibleCellWidth(confirm_launch_label);

		FarDialogItem confirm_launch_chkbx = { DI_CHECKBOX, DlgLayout::H_SIDE_OVERHEAD, current_y, 0, current_y, FALSE, {}, DIF_NONE,
											   FALSE, confirm_launch_label, 0 };
		confirm_launch_chkbx.Param.Selected  = s_confirm_launch;
		auto confirm_launch_chkbx_idx           = add_item(confirm_launch_chkbx);
		auto confirm_launch_edit_idx            = add_item({ DI_FIXEDIT, DlgLayout::H_SIDE_OVERHEAD + confirm_launch_label_width + 5, current_y,
															 DlgLayout::H_SIDE_OVERHEAD + confirm_launch_label_width + 8, current_y, FALSE,
															 {reinterpret_cast<DWORD_PTR>(L"9999")}, DIF_MASKEDIT, FALSE, threshold_current.c_str(), 0 });
		current_y++;

		auto display_filename_idx               = add_checkbox(GetMsg(MsgID::DisplayFilename), s_display_filename);

		// ----- Add platform-specific settings. -----
		std::vector<std::pair<int, size_t>> setting_control_bindings;
		setting_control_bindings.reserve(platform_settings.size());

		if (!platform_settings.empty()) {
			add_separator();
			for (size_t setting_idx = 0; setting_idx < platform_settings.size(); ++setting_idx) {
				auto& setting = platform_settings[setting_idx];
				int dlg_item_idx = add_checkbox(setting.display_name.c_str(), setting.value, setting.disabled);
				setting_control_bindings.emplace_back(dlg_item_idx, setting_idx);
			}
		}

		add_separator();
		auto ok_btn_idx = add_item({ DI_BUTTON, 0, current_y, 0, current_y, FALSE, {}, DIF_CENTERGROUP, TRUE, GetMsg(MsgID::Ok), 0 });
		add_item({ DI_BUTTON, 0, current_y, 0, current_y, FALSE, {}, DIF_CENTERGROUP, FALSE, GetMsg(MsgID::Cancel), 0 });

		int config_dialog_height = current_y + 3;
		config_dialog_items[0].Y2 = config_dialog_height - DlgLayout::V_OUTER_MARGIN - 1;

		bool is_platform_settings_changed = false;
		bool is_platform_settings_requiring_candidate_list_refresh_changed = false;

		HANDLE config_dlg = g_info.DialogInit(g_info.ModuleNumber, -1, -1, CONFIG_DIALOG_WIDTH, config_dialog_height, L"ConfigurationDialog",
									   config_dialog_items.data(), static_cast<unsigned int>(config_dialog_items.size()), 0, 0, nullptr, 0);

		if (config_dlg != INVALID_HANDLE_VALUE) {

			const int exit_code = g_info.DialogRun(config_dlg);

			// ----- Process results if "OK" was pressed. -----
			if (exit_code == static_cast<int>(ok_btn_idx)) {

				auto is_checked = [config_dlg](int i) -> bool {
					return g_info.SendDlgMessage(config_dlg, DM_GETCHECK, i, 0) == BSTATE_CHECKED;
				};

				s_use_external_terminal          = is_checked(use_external_terminal_idx);
				s_no_wait_for_command_completion = is_checked(no_wait_for_command_completion_idx);
				s_clear_selection                = is_checked(clear_selection_idx);
				s_confirm_launch                 = is_checked(confirm_launch_chkbx_idx);

				const auto threshold_new = (const wchar_t*)g_info.SendDlgMessage(config_dlg, DM_GETCONSTTEXTPTR, confirm_launch_edit_idx, 0);
				const long raw = wcstol(threshold_new, nullptr, 10);
				s_confirm_launch_threshold = static_cast<int>(std::clamp(raw, 0L, 9999L));

				s_display_filename               = is_checked(display_filename_idx);

				KeyFileHelper key_writer(InMyConfig(INI_FILEPATH));
				SaveGeneralSettings(key_writer);

				// Propagate changes to dynamic platform-specific settings back to the provider.
				if (!setting_control_bindings.empty()) {
					for (auto [dlg_item_idx, setting_idx] : setting_control_bindings) {
						auto& setting = platform_settings[setting_idx];
						const bool new_value = is_checked(dlg_item_idx);
						if (setting.value != new_value) {
							if (setting.affects_candidates) {
								is_platform_settings_requiring_candidate_list_refresh_changed = true;
							}
							is_platform_settings_changed = true;
							setting.value = new_value;
						}
					}

					if (is_platform_settings_changed) {
						provider->SetPlatformSettings(platform_settings);
					}
					provider->SavePlatformSettings(key_writer);
				}

				if (!key_writer.Save(true)) {
					ShowErrorDlg({GetMsg(MsgID::SaveConfigError), StrMB2Wide(InMyConfig(INI_FILEPATH))});
				}
			}
			g_info.DialogFree(config_dlg);
		}

		return { is_platform_settings_requiring_candidate_list_refresh_changed || (old_use_external_terminal != s_use_external_terminal) };
	}



	void Plugin::ShowErrorDlg(const std::vector<std::wstring>& error_lines)
	{
		std::vector<const wchar_t*> items;
		items.reserve(error_lines.size() + 1);
		items.push_back(GetMsg(MsgID::Error));
		for (const auto &line : error_lines) {
			items.push_back(line.c_str());
		}
		g_info.Message(g_info.ModuleNumber, FMSG_WARNING | FMSG_MB_OK, L"Troubleshooting", items.data(), items.size(), 0);
	}



	// ****************************** PRIVATE IMPLEMENTATION ******************************

	void Plugin::FilterOutTerminalCandidates(std::vector<CandidateInfo> &candidates, size_t file_count)
	{
		if (file_count <= 1 || s_use_external_terminal) return;

		auto should_remove_candidate = [](const CandidateInfo& c) {
			return c.terminal && !c.multi_file_aware;
		};

		candidates.erase(std::remove_if(candidates.begin(), candidates.end(), should_remove_candidate),
						 candidates.end());
	}



	bool Plugin::AskForLaunchConfirmation(const CandidateInfo& app, size_t file_count)
	{
		if (!s_confirm_launch || file_count <= static_cast<size_t>(s_confirm_launch_threshold)) {
			return true;
		}
		wchar_t message[512] = {};
		g_fsf.snprintf(message, std::size(message), GetMsg(MsgID::ConfirmLaunchMessage), file_count, app.name.c_str());
		const wchar_t* items[] = { GetMsg(MsgID::ConfirmLaunchTitle), message };
		int res = g_info.Message(g_info.ModuleNumber, FMSG_MB_YESNO, nullptr, items, std::size(items), 0);
		return (res == 0);
	}



	void Plugin::LaunchApplication(const CandidateInfo& app, const std::vector<std::wstring>& cmds, LaunchMode launch_mode)
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

		bool launch_succeeded = true;
		for (const auto& cmd : cmds) {
			if (g_fsf.Execute(cmd.c_str(), execute_flags) == -1) {
				ShowErrorDlg({GetMsg(MsgID::CannotExecute), cmd });
				launch_succeeded = false;
				break; // Stop on the first error.
			}
		}

		if (launch_succeeded && s_clear_selection) {
			g_info.Control(PANEL_ACTIVE, FCTL_UPDATEPANEL, 0, 0);
		}
	}



	Plugin::DetailsDlgResult Plugin::ShowDetailsDlg(const std::vector<std::wstring>& filepaths,
											   const std::vector<std::wstring>& unique_mime_profiles,
											   const std::vector<Field>& application_info,
											   const std::vector<std::wstring>& cmds,
											   const std::vector<CandidateContextLocation>& locations)
	{
		// ----- Assemble unified list of dialog fields and separators -----
		std::vector<std::optional<Field>> details;
		if (auto file_count = filepaths.size(); file_count != 1) {
			details.push_back(Field{GetMsg(MsgID::FilesSelected), std::to_wstring(file_count)});
		}
		details.push_back(Field{GetMsg(MsgID::Filepaths), JoinStrings(filepaths, L"; ")});
		details.push_back(Field{GetMsg(MsgID::MimeProfiles), JoinStrings(unique_mime_profiles, L"; ")});
		details.push_back(std::nullopt); // separator
		for (const auto& field : application_info) {
			details.push_back(field);
		}
		details.push_back(std::nullopt); // separator
		details.push_back(Field{GetMsg(MsgID::LaunchCommand), JoinStrings(cmds, L"; ")});
		details.push_back(std::nullopt); // separator

		// ----- Calculate dynamic dialog dimensions based on content -----

		constexpr int DETAILS_DLG_DESIRED_WIDTH = 100;
		constexpr int MIN_FIELD_EDIT_WIDTH = 10;

		const int screen_width = GetConsoleWidth();

		const int standard_buttons_span = GetButtonWidth(GetMsg(MsgID::Close)) + DlgLayout::H_BUTTON_GAP + GetButtonWidth(GetMsg(MsgID::Launch));
		int locations_span = 0;
		for (const auto& location : locations) {
			if (locations_span > 0) {
				locations_span += DlgLayout::H_BUTTON_GAP;
			}
			locations_span += GetButtonWidth(location.title);
		}

		const int max_label_width = GetMaxLabelCellWidth(details);
		const int details_dlg_min_width = std::max({standard_buttons_span, locations_span, max_label_width + 1 + MIN_FIELD_EDIT_WIDTH}) + DlgLayout::H_TOTAL_OVERHEAD;
		const int details_dlg_max_width = std::max(details_dlg_min_width, screen_width - 4);
		const int details_dlg_width = std::clamp(DETAILS_DLG_DESIRED_WIDTH, details_dlg_min_width, details_dlg_max_width);
		const int total_buttons_span = standard_buttons_span + (locations.empty() ? 0 : 1 + locations_span);
		const bool extra_row_for_locations = total_buttons_span > (details_dlg_width - DlgLayout::H_TOTAL_OVERHEAD);
		const int label_end_x = DlgLayout::H_SIDE_OVERHEAD + max_label_width - 1;
		const int edit_start_x = DlgLayout::H_SIDE_OVERHEAD + max_label_width + 1;
		const int edit_end_x = details_dlg_width - DlgLayout::H_SIDE_OVERHEAD - 1;
		const int details_dlg_height = DlgLayout::V_SIDE_OVERHEAD * 2 + static_cast<int>(details.size()) + 1 /* standard buttons */
														+ (extra_row_for_locations ? 1 : 0);

		// ----- Build dialog UI elements layout -----
		std::vector<FarDialogItem> details_dlg_items;
		details_dlg_items.reserve(1 + (details.size() * 2) + 2 + locations.size());
		int current_y = 1;

		auto add_field_row = [&details_dlg_items, &current_y, label_end_x, edit_start_x, edit_end_x](const Field& field) {
			int label_start_x = label_end_x - GetLabelCellWidth(field) + 1;
			details_dlg_items.push_back({ DI_TEXT, label_start_x, current_y, label_end_x, current_y, FALSE, {}, 0, 0, field.label.c_str(), 0 });
			details_dlg_items.push_back({ DI_EDIT, edit_start_x, current_y, edit_end_x, current_y, FALSE, {}, DIF_READONLY | DIF_SELECTONENTRY, 0,
										  field.content.c_str(), 0 });
			current_y++;
		};

		auto add_separator = [&details_dlg_items, &current_y]() {
			details_dlg_items.push_back({ DI_TEXT, 0, current_y, 0, current_y, FALSE, {}, DIF_SEPARATOR, 0, L"", 0 });
			current_y++;
		};

		auto add_button = [&details_dlg_items, &current_y](const wchar_t* caption, bool is_default = false, bool is_focused = false) {
			details_dlg_items.push_back({ DI_BUTTON, 0, current_y, 0, current_y, is_focused ? TRUE : FALSE, {}, DIF_CENTERGROUP, 0, caption, 0 });
			if (is_default) {
				details_dlg_items.back().DefaultButton = TRUE;
			}
		};

		auto add_doublebox = [&details_dlg_items, &current_y, &details_dlg_width, &details_dlg_height]() {
			details_dlg_items.push_back({ DI_DOUBLEBOX, DlgLayout::H_OUTER_MARGIN, current_y++, details_dlg_width - DlgLayout::H_OUTER_MARGIN - 1,
										  details_dlg_height - DlgLayout::V_OUTER_MARGIN - 1, FALSE, {}, 0, 0, GetMsg(MsgID::Details), 0 });
		};


		add_doublebox();
		for (const auto& opt_field : details) {
			if (opt_field.has_value()) {
				add_field_row(opt_field.value());
			} else {
				add_separator();
			}
		}
		add_button(GetMsg(MsgID::Close), true, true);
		const int launch_btn_idx = static_cast<int>(details_dlg_items.size());
		add_button(GetMsg(MsgID::Launch));
		if (extra_row_for_locations) {
			++current_y;
		}
		const int first_location_btn_idx = static_cast<int>(details_dlg_items.size());
		for (const auto &location : locations) {
			add_button(location.title.c_str());
		}


		// ----- Execute dialog and handle user interactions -----
		const HANDLE details_dlg = g_info.DialogInit(g_info.ModuleNumber, -1, -1, details_dlg_width, details_dlg_height, L"DetailsDialog",
									   details_dlg_items.data(), static_cast<unsigned int>(details_dlg_items.size()), 0, 0, nullptr, 0);

		if (details_dlg != INVALID_HANDLE_VALUE) {
			const int exit_code = g_info.DialogRun(details_dlg);
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



	bool Plugin::GoToFile(const std::wstring &filepath)
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
		for (int i = 0; i < panel_info.ItemsNumber; ++i) {
			int sz = g_info.Control(PANEL_ACTIVE, FCTL_GETPANELITEM, i, 0);
			if (sz <= 0) continue;
			if (static_cast<size_t>(sz) > buf.size()) {
				buf.resize(static_cast<size_t>(sz));
			}
			if (!g_info.Control(PANEL_ACTIVE, FCTL_GETPANELITEM, i, (LONG_PTR)buf.data())) {
				continue;
			}
			const auto *item = reinterpret_cast<const PluginPanelItem *>(buf.data());
			if (item->FindData.lpwszFileName && name == item->FindData.lpwszFileName) {
				PanelRedrawInfo panel_ri {};
				panel_ri.CurrentItem = i;
				return g_info.Control(PANEL_ACTIVE, FCTL_REDRAWPANEL, 0, (LONG_PTR)&panel_ri) != 0;
			}
		}
		return false;
	}



	void Plugin::LoadGeneralSettings(const KeyFileReadHelper& key_reader)
	{
		s_use_external_terminal          = key_reader.GetInt(INI_SECTION_GENERAL, "UseExternalTerminal",        0) != 0;
		s_no_wait_for_command_completion = key_reader.GetInt(INI_SECTION_GENERAL, "NoWaitForCommandCompletion", 1) != 0;
		s_clear_selection                = key_reader.GetInt(INI_SECTION_GENERAL, "ClearSelection",             0) != 0;
		s_confirm_launch                 = key_reader.GetInt(INI_SECTION_GENERAL, "ConfirmLaunch",              1) != 0;
		s_confirm_launch_threshold       = key_reader.GetInt(INI_SECTION_GENERAL, "ConfirmLaunchThreshold",    10);
		s_confirm_launch_threshold = std::clamp(s_confirm_launch_threshold, 0, 9999);
		s_display_filename               = key_reader.GetInt(INI_SECTION_GENERAL, "DisplayFilename",            0) != 0;
	}



	void Plugin::SaveGeneralSettings(KeyFileHelper& key_writer)
	{
		key_writer.SetInt(INI_SECTION_GENERAL, "UseExternalTerminal",        s_use_external_terminal);
		key_writer.SetInt(INI_SECTION_GENERAL, "NoWaitForCommandCompletion", s_no_wait_for_command_completion);
		key_writer.SetInt(INI_SECTION_GENERAL, "ClearSelection",             s_clear_selection);
		key_writer.SetInt(INI_SECTION_GENERAL, "ConfirmLaunch",              s_confirm_launch);
		key_writer.SetInt(INI_SECTION_GENERAL, "ConfirmLaunchThreshold",     s_confirm_launch_threshold);
		key_writer.SetInt(INI_SECTION_GENERAL, "DisplayFilename",            s_display_filename);
	}



	std::wstring Plugin::JoinStrings(const std::vector<std::wstring>& strings, const std::wstring& delimiter)
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



	int Plugin::CalculateVisibleCellWidth(std::wstring_view text, bool literal_ampersands)
	{
		const size_t raw_cells = g_fsf.StrCellsCount(text.data(), text.size());
		if (literal_ampersands) {
			return static_cast<int>(raw_cells);
		}

		size_t ampersand_discount = 0;
		size_t pos = 0;
		while ((pos = text.find(L'&', pos)) != std::wstring_view::npos) {
			ampersand_discount++;
			pos += 2;
		}
		auto result = (raw_cells > ampersand_discount) ? (raw_cells - ampersand_discount) : 0;
		return static_cast<int>(result);
	}



	int Plugin::GetLabelCellWidth(const Field& field)
	{
		return CalculateVisibleCellWidth(field.label);
	}



	int Plugin::GetMaxLabelCellWidth(const std::vector<std::optional<Field>>& fields)
	{
		int max_width = 0;
		for (const auto& field : fields) {
			if (field.has_value()) {
				max_width = std::max(max_width, GetLabelCellWidth(field.value()));
			}
		}
		return max_width;
	}



	int Plugin::GetButtonWidth(std::wstring_view caption)
	{
		constexpr int BRACKETS_OVERHEAD = 2;
		constexpr int SPACES_OVERHEAD = 2;
		return CalculateVisibleCellWidth(caption) + BRACKETS_OVERHEAD + SPACES_OVERHEAD;
	}



	int Plugin::GetConsoleWidth()
	{
		SMALL_RECT rect{};
		if (g_info.AdvControl(g_info.ModuleNumber, ACTL_GETFARRECT, &rect, nullptr)) {
			return rect.Right - rect.Left + 1;
		}
		return 80;
	}



	std::wstring Plugin::FormatMenuTitle(const std::vector<std::wstring>& filepaths)
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

		constexpr int MENU_UI_OVERHEAD_CELLS = 12;
		const int title_width = CalculateVisibleCellWidth(title, true);
		const int max_filename_width = std::max(1, GetConsoleWidth() - title_width - MENU_UI_OVERHEAD_CELLS);
		g_fsf.TruncStr(filename.data(), max_filename_width);
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


} // namespace openwith


// ****************************** PLUGIN ENTRY POINTS ******************************


SHAREDSYMBOL void WINAPI SetStartupInfoW(const PluginStartupInfo *Info)
{
	openwith::g_info = *Info;
	openwith::g_fsf = *Info->FSF;
	openwith::g_info.FSF = &openwith::g_fsf;

	KeyFileReadHelper key_reader(InMyConfig(openwith::INI_FILEPATH));
	openwith::Plugin::LoadGeneralSettings(key_reader);
	if (auto* provider = openwith::AppProvider::GetInstance()) {
		provider->LoadPlatformSettings(key_reader);
	}
}


SHAREDSYMBOL void WINAPI GetPluginInfoW(PluginInfo *Info)
{
	Info->StructSize = sizeof(struct PluginInfo);
	Info->SysID = 0x93CDEF19;
	Info->Flags = 0;
	static const wchar_t *s_menu_strings[1];
	s_menu_strings[0] = GetMsg(openwith::MsgID::PluginTitle);
	Info->PluginMenuStrings = s_menu_strings;
	Info->PluginMenuStringsNumber = std::size(s_menu_strings);
	static const wchar_t *s_config_strings[1];
	s_config_strings[0] = GetMsg(openwith::MsgID::PluginTitle);
	Info->PluginConfigStrings = s_config_strings;
	Info->PluginConfigStringsNumber = std::size(s_config_strings);
	Info->CommandPrefix = nullptr;
}


SHAREDSYMBOL HANDLE WINAPI OpenPluginW(int OpenFrom, INT_PTR Item)
{
	if (OpenFrom != OPEN_PLUGINSMENU && OpenFrom != (OPEN_FROMMACRO | MACROAREA_SHELL)) {
		return INVALID_HANDLE_VALUE;
	}

	PanelInfo pi {};

	if (!openwith::g_info.Control(PANEL_ACTIVE, FCTL_GETPANELINFO, 0, (LONG_PTR)&pi)) {
		return INVALID_HANDLE_VALUE;
	}

	if (pi.PanelType != PTYPE_FILEPANEL || pi.ItemsNumber <= 0) {
		return INVALID_HANDLE_VALUE;
	}

	if (pi.Plugin && !(pi.Flags & PFLAGS_REALNAMES)) {
		openwith::Plugin::ShowErrorDlg({GetMsg(openwith::MsgID::NotRealNames)});
		return INVALID_HANDLE_VALUE;
	}

	int dir_size = openwith::g_info.Control(PANEL_ACTIVE, FCTL_GETPANELDIR, 0, 0);
	if (dir_size <= 0) {
		return INVALID_HANDLE_VALUE;
	}

	auto dir_buf = std::make_unique<wchar_t[]>(dir_size);
	if (!openwith::g_info.Control(PANEL_ACTIVE, FCTL_GETPANELDIR, dir_size, (LONG_PTR)dir_buf.get())) {
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
			int item_size = openwith::g_info.Control(PANEL_ACTIVE, FCTL_GETSELECTEDPANELITEM, i, 0);
			if (item_size <= 0) {
				continue;
			}
			auto item_buf = std::make_unique<unsigned char[]>(item_size);
			PluginPanelItem* pi_item = reinterpret_cast<PluginPanelItem*>(item_buf.get());
			if (openwith::g_info.Control(PANEL_ACTIVE, FCTL_GETSELECTEDPANELITEM, i, (LONG_PTR)pi_item) && pi_item->FindData.lpwszFileName) {
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
		openwith::Plugin::ProcessFiles(selected_filepaths);
	}

	// Plugin performs an action and exits, rather than creating a new panel instance (like a VFS plugin).
	return INVALID_HANDLE_VALUE;
}


SHAREDSYMBOL int WINAPI ConfigureW(int ItemNumber)
{
	openwith::Plugin::ShowConfigDlg();
	return FALSE;
}


SHAREDSYMBOL void WINAPI ExitFARW()
{
}


SHAREDSYMBOL int WINAPI GetMinFarVersionW()
{
	return FARMANAGERVERSION;
}
