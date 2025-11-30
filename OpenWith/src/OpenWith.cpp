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


	// Standard far2l plugin entry point for initialization.
	void OpenWithPlugin::SetStartupInfo(const PluginStartupInfo *plugin_startup_info)
	{
		s_info = *plugin_startup_info;
		s_fsf = *plugin_startup_info->FSF;
		s_info.FSF = &s_fsf;
		LoadOptions();
	}


	// Standard far2l plugin entry point to provide information about the plugin.
	void OpenWithPlugin::GetPluginInfo(PluginInfo *plugin_info)
	{
		plugin_info->StructSize = sizeof(*plugin_info);
		plugin_info->Flags = 0;
		static const wchar_t *s_menu_strings[1];
		s_menu_strings[0] = GetMsg(MPluginTitle);
		plugin_info->PluginMenuStrings = s_menu_strings;
		plugin_info->PluginMenuStringsNumber = ARRAYSIZE(s_menu_strings);
		static const wchar_t *s_config_strings[1];
		s_config_strings[0] = GetMsg(MPluginTitle);
		plugin_info->PluginConfigStrings = s_config_strings;
		plugin_info->PluginConfigStringsNumber = ARRAYSIZE(s_config_strings);
		plugin_info->CommandPrefix = nullptr;
	}


	// The public Configure function called by far2l.
	int OpenWithPlugin::Configure(int item_number)
	{
		return ConfigureImpl().settings_saved;
	}


	// Main plugin entry point, called when the user activates the plugin from the menu.
	// It collects selected file paths from the active panel and initiates processing.
	HANDLE OpenWithPlugin::OpenPlugin(int open_from, INT_PTR item)
	{
		if (open_from != OPEN_PLUGINSMENU) {
			return INVALID_HANDLE_VALUE;
		}

		PanelInfo pi = {};

		if (!s_info.Control(PANEL_ACTIVE, FCTL_GETPANELINFO, 0, (LONG_PTR)&pi)) {
			return INVALID_HANDLE_VALUE;
		}

		if (pi.PanelType != PTYPE_FILEPANEL || pi.ItemsNumber <= 0) {
			return INVALID_HANDLE_VALUE;
		}

		if (pi.Plugin && !(pi.Flags & PFLAGS_REALNAMES)) {
			ShowError(GetMsg(MError), {GetMsg(MNotRealNames)});
			return INVALID_HANDLE_VALUE;
		}

		std::vector<std::wstring> selected_filepaths;

		// Query the required buffer size for the panel's directory path.
		int dir_size = s_info.Control(PANEL_ACTIVE, FCTL_GETPANELDIR, 0, 0);
		if (dir_size <= 0) {
			return INVALID_HANDLE_VALUE;
		}

		// Then, retrieve the path itself.
		auto dir_buf = std::make_unique<wchar_t[]>(dir_size);
		if (!s_info.Control(PANEL_ACTIVE, FCTL_GETPANELDIR, dir_size, (LONG_PTR)dir_buf.get())) {
			return INVALID_HANDLE_VALUE;
		}

		std::wstring base_path(dir_buf.get());
		// Ensure the base path ends with a separator.
		if (!base_path.empty() && base_path.back() != L'/') {
			base_path += L'/';
		}

		// This single block handles both selected files and the file under the cursor.
		// If no items are selected, pi.SelectedItemsNumber will be 1,
		// and FCTL_GETSELECTEDPANELITEM will return the item under the cursor.
		if (pi.SelectedItemsNumber > 0) {
			selected_filepaths.reserve(pi.SelectedItemsNumber);
			for (int i = 0; i < pi.SelectedItemsNumber; ++i) {
				// Query the buffer size for the selected panel item.
				int item_size = s_info.Control(PANEL_ACTIVE, FCTL_GETSELECTEDPANELITEM, i, 0);
				if (item_size <= 0) continue;

				auto item_buf = std::make_unique<unsigned char[]>(item_size);
				PluginPanelItem* pi_item = reinterpret_cast<PluginPanelItem*>(item_buf.get());

				// Retrieve the panel item data.
				if (s_info.Control(PANEL_ACTIVE, FCTL_GETSELECTEDPANELITEM, i, (LONG_PTR)pi_item) && pi_item->FindData.lpwszFileName) {
					// Construct the full path and add it to the list.
					selected_filepaths.push_back(base_path + pi_item->FindData.lpwszFileName);
				}
			}
		}

		if (!selected_filepaths.empty()) {
			ProcessFiles(selected_filepaths);
		}

		// The plugin doesn't create its own panel, so it returns INVALID_HANDLE_VALUE.
		return INVALID_HANDLE_VALUE;
	}


	void OpenWithPlugin::Exit()
	{
	}


	const wchar_t* OpenWithPlugin::GetMsg(int msg_id)
	{
		return s_info.GetMsg(s_info.ModuleNumber, msg_id);
	}


	// ****************************** Private implementation ******************************


	// The core implementation of the configuration logic.
	// It returns a detailed result, allowing the caller to know if the application list needs to be refreshed.
	OpenWithPlugin::ConfigureResult OpenWithPlugin::ConfigureImpl()
	{
		LoadOptions();

		auto provider = AppProvider::CreateAppProvider(&OpenWithPlugin::GetMsg);
		provider->LoadPlatformSettings();

		const bool old_use_external_terminal = s_use_external_terminal;

		// Store the state of platform-specific settings *before* showing the dialog.
		// This is crucial for detecting changes later.
		std::vector<ProviderSetting> old_platform_settings = provider->GetPlatformSettings();

		std::vector<FarDialogItem> di;
		int y = 0;
		di.push_back({ DI_DOUBLEBOX, 3, ++y, 0, 0, FALSE, {}, 0, 0, GetMsg(MConfigTitle), 0 });
		di.push_back({ DI_CHECKBOX, 5, ++y, 0, 0, TRUE, { s_use_external_terminal }, 0, 0, GetMsg(MUseExternalTerminal), 0 });
		di.push_back({ DI_CHECKBOX, 5, ++y, 0, 0, 0, { s_no_wait_for_command_completion },  0, 0, GetMsg(MNoWaitForCommandCompletion), 0});
		di.push_back({ DI_CHECKBOX, 5, ++y, 0, 0, 0, { s_clear_selection },  0, 0, GetMsg(MClearSelection), 0});

		auto threshold_wstr = std::to_wstring(s_confirm_launch_threshold);
		const wchar_t* confirm_label = GetMsg(MConfirmLaunchOption);
		size_t confirm_label_width = s_fsf.StrCellsCount(confirm_label, wcslen(confirm_label));

		y++;
		di.push_back({ DI_CHECKBOX, 5, y, 0, 0, 0,  { s_confirm_launch }, 0, 0, confirm_label, 0 });
		di.push_back({ DI_FIXEDIT, (int)(confirm_label_width + 11), y, (int)(confirm_label_width + 14), 0, FALSE, {(DWORD_PTR)L"9999"}, DIF_MASKEDIT, 0, threshold_wstr.c_str(), 0});

		int first_platform_item_idx {};

		if (!old_platform_settings.empty()) {
			di.push_back({ DI_TEXT, 5, ++y, 0, 0, FALSE, {}, DIF_SEPARATOR, 0, L"", 0 });
			first_platform_item_idx = (int)di.size();
			for (const auto& setting : old_platform_settings) {
				di.push_back({ DI_CHECKBOX, 5, ++y, 0, 0, FALSE, { setting.value }, setting.disabled ? DIF_DISABLE : DIF_NONE, 0, setting.display_name.c_str(), 0 });
			}
		}

		di.push_back({ DI_TEXT, 5, ++y, 0, 0, FALSE, {}, DIF_SEPARATOR, 0, L"", 0 });
		y++;
		di.push_back({ DI_BUTTON, 0, y, 0, 0, FALSE, {}, DIF_CENTERGROUP, 0, GetMsg(MOk), 0 });
		di.back().DefaultButton = TRUE;
		di.push_back({ DI_BUTTON, 0, y, 0, 0, FALSE, {}, DIF_CENTERGROUP, 0, GetMsg(MCancel), 0 });

		int config_dialog_height = y + 3;
		constexpr int CONFIG_DIALOG_WIDTH = 70;

		// Update dynamically calculated DI_DOUBLEBOX coordinates
		di[0].X2 = CONFIG_DIALOG_WIDTH - 4;
		di[0].Y2 = config_dialog_height - 2;

		HANDLE dlg = s_info.DialogInit(s_info.ModuleNumber, -1, -1, CONFIG_DIALOG_WIDTH, config_dialog_height, L"ConfigurationDialog", di.data(), di.size(), 0, 0, nullptr, 0);
		if (dlg == INVALID_HANDLE_VALUE) {
			return {};
		}

		int exit_code = s_info.DialogRun(dlg);
		ConfigureResult result;
		auto ok_button_index = (int)di.size() - 2;

		if (exit_code == ok_button_index) {
			result.settings_saved = true;
			// Save platform-independent settings
			s_use_external_terminal = (s_info.SendDlgMessage(dlg, DM_GETCHECK, 1, 0) == BSTATE_CHECKED);
			s_no_wait_for_command_completion = (s_info.SendDlgMessage(dlg, DM_GETCHECK, 2, 0) == BSTATE_CHECKED);
			s_clear_selection = (s_info.SendDlgMessage(dlg, DM_GETCHECK, 3, 0) == BSTATE_CHECKED);
			s_confirm_launch = (s_info.SendDlgMessage(dlg, DM_GETCHECK, 4, 0) == BSTATE_CHECKED);
			const wchar_t* threshold_val_str = (const wchar_t*)s_info.SendDlgMessage(dlg, DM_GETCONSTTEXTPTR, 5, 0);
			s_confirm_launch_threshold = wcstol(threshold_val_str, NULL, 10);

			SaveOptions();

			bool platform_settings_changed = false;
			if (!old_platform_settings.empty()) {
				std::vector<ProviderSetting> new_settings;

				for (size_t i = 0; i < old_platform_settings.size(); ++i) {
					bool new_value = (s_info.SendDlgMessage(dlg, DM_GETCHECK, first_platform_item_idx + i, 0) == BSTATE_CHECKED);
					// If any platform-specific setting has changed, the candidate list must be regenerated.
					if (old_platform_settings[i].value != new_value) {
						platform_settings_changed = true;
					}
					new_settings.push_back({ old_platform_settings[i].internal_key, old_platform_settings[i].display_name, new_value });
				}
				provider->SetPlatformSettings(new_settings);
				provider->SavePlatformSettings();
			}

			if (platform_settings_changed || (old_use_external_terminal != s_use_external_terminal)) {
				result.refresh_needed = true;
			}
		}

		s_info.DialogFree(dlg);
		return result;
	}


	// return true if exit by button "Launch", false otherwise
	bool OpenWithPlugin::ShowDetailsDialogImpl(const std::vector<Field>& file_info,
											   const std::vector<Field>& application_info,
											   const Field& launch_command)
	{
		constexpr int DETAILS_DIALOG_MIN_WIDTH = 40;
		constexpr int DETAILS_DIALOG_DESIRED_WIDTH = 90;

		int details_dialog_max_width = std::max(DETAILS_DIALOG_MIN_WIDTH, GetScreenWidth() - 4);
		int details_dialog_width = std::clamp(DETAILS_DIALOG_DESIRED_WIDTH, DETAILS_DIALOG_MIN_WIDTH, details_dialog_max_width);

		int details_dialog_height = file_info.size() + application_info.size() + 9;

		// Helper lambda to get the console cell width of a field's label string.
		// This is crucial for correct UI alignment with non-ASCII characters.
		auto get_label_cell_width = [](const Field& f) -> size_t {
			return s_fsf.StrCellsCount(f.label.c_str(), f.label.size());
		};

		// Helper lambda to find the maximum label length (in cells) in a vector of Fields for alignment.
		auto max_in = [&](const std::vector<Field>& v) -> size_t {
			if (v.empty()) return 0;
			// Use a custom comparator that measures string width in console cells.
			// The inner lambda captures '[&]' to access the get_label_cell_width helper.
			return get_label_cell_width(*std::max_element(v.begin(), v.end(),
														  [&](const Field& x, const Field& y){
															  return get_label_cell_width(x) < get_label_cell_width(y);
														  }));
		};

		// Calculate the maximum label width (in cells) across all sections.
		auto max_di_text_length = static_cast<int>(std::max({
			get_label_cell_width(launch_command),
			max_in(file_info),
			max_in(application_info)
		}));

		// Calculate coordinates for dialog items to right-align all text labels.
		int di_text_x2 = max_di_text_length + 4;
		int di_edit_x1 = max_di_text_length + 6;
		int di_edit_x2 = details_dialog_width - 6;

		std::vector<FarDialogItem> di;

		di.push_back({ DI_DOUBLEBOX, 3,  1, details_dialog_width - 4,  details_dialog_height - 2, FALSE, {}, 0, 0, GetMsg(MDetails), 0 });

		int cur_line = 2;

		for (auto &field : file_info) {
			int di_text_x1 = di_text_x2 - static_cast<int>(get_label_cell_width(field)) + 1;
			di.push_back({ DI_TEXT, di_text_x1, cur_line,  di_text_x2, cur_line, FALSE, {}, 0, 0, field.label.c_str(), 0 });
			di.push_back({ DI_EDIT, di_edit_x1, cur_line,  di_edit_x2, cur_line, FALSE, {}, DIF_READONLY | DIF_SELECTONENTRY, 0,  field.content.c_str(), 0});
			++cur_line;
		}

		di.push_back({ DI_TEXT, 5,  cur_line,  0,  cur_line, FALSE, {}, DIF_SEPARATOR, 0, L"", 0 });
		++cur_line;

		for (auto &field : application_info) {
			int di_text_x1 = di_text_x2 - static_cast<int>(get_label_cell_width(field)) + 1;
			di.push_back({ DI_TEXT, di_text_x1, cur_line,  di_text_x2, cur_line, FALSE, {}, 0, 0, field.label.c_str(), 0 });
			di.push_back({ DI_EDIT, di_edit_x1, cur_line,  di_edit_x2, cur_line, FALSE, {}, DIF_READONLY | DIF_SELECTONENTRY, 0,  field.content.c_str(), 0});
			++cur_line;
		}

		di.push_back({ DI_TEXT, 5,  cur_line,  0,  cur_line, FALSE, {}, DIF_SEPARATOR, 0, L"", 0 });
		++cur_line;

		int di_text_x1 = di_text_x2 - static_cast<int>(get_label_cell_width(launch_command)) + 1;
		di.push_back({ DI_TEXT, di_text_x1, cur_line,  di_text_x2, cur_line, FALSE, {}, 0, 0, launch_command.label.c_str(), 0 });
		di.push_back({ DI_EDIT, di_edit_x1, cur_line,  di_edit_x2, cur_line, FALSE, {}, DIF_READONLY | DIF_SELECTONENTRY, 0,  launch_command.content.c_str(), 0});
		++cur_line;

		di.push_back({ DI_TEXT, 5,  cur_line,  0,  cur_line, FALSE, {}, DIF_SEPARATOR, 0, L"", 0 });
		++cur_line;

		di.push_back({ DI_BUTTON, 0,  cur_line,  0,  cur_line, TRUE, {}, DIF_CENTERGROUP, 0, GetMsg(MClose), 0 });

		di.back().DefaultButton = TRUE;

		di.push_back({ DI_BUTTON, 0,  cur_line,  0,  cur_line, TRUE, {}, DIF_CENTERGROUP, 0, GetMsg(MLaunch), 0 });

		HANDLE dlg = s_info.DialogInit(s_info.ModuleNumber, -1, -1, details_dialog_width, details_dialog_height, L"InformationDialog",
									   di.data(), static_cast<int>(di.size()), 0, 0, nullptr, 0);
		if (dlg != INVALID_HANDLE_VALUE) {
			int exit_code = s_info.DialogRun(dlg);
			s_info.DialogFree(dlg);
			return (exit_code == (int)di.size() - 1); // last element is button "Launch"
		}
		return false;
	}


	// Shows the details dialog with file and application information.
	// For a single file, it shows the full path. For multiple files, it shows a count.
	bool OpenWithPlugin::ShowDetailsDialog(AppProvider* provider, const CandidateInfo& app,
										   const std::vector<std::wstring>& filepaths,
										   const std::vector<std::wstring>& cmds,
										   const std::vector<std::wstring>& unique_mime_profiles)
	{
		std::vector<Field> file_info;
		if (filepaths.size() == 1) {
			// For a single file, show its full path.
			file_info.push_back({ GetMsg(MPathname), filepaths[0] });
		} else {
			// For multiple files, show a summary count.
			std::wstring count_msg = std::wstring(GetMsg(MFilesSelected)) + std::to_wstring(filepaths.size());
			file_info.push_back({ GetMsg(MPathname), count_msg });
		}

		// Use the pre-fetched mime profiles instead of re-calculating them.
		file_info.push_back({ GetMsg(MMimeType), JoinStrings(unique_mime_profiles, L"; ") });

		std::wstring all_cmds = JoinStrings(cmds, L"; ");
		std::vector<Field> application_info = provider->GetCandidateDetails(app);
		Field launch_command { GetMsg(MLaunchCommand), all_cmds.c_str() };

		return ShowDetailsDialogImpl(file_info, application_info, launch_command);
	}


	// Returns true if the user confirms or no confirmation is needed, false otherwise.
	bool OpenWithPlugin::AskForLaunchConfirmation(const CandidateInfo& app, const std::vector<std::wstring>& filepaths)
	{
		if (!s_confirm_launch || filepaths.size() <= static_cast<size_t>(s_confirm_launch_threshold)) {
			return true;
		}
		wchar_t message[255] = {};
		s_fsf.snprintf(message, ARRAYSIZE(message) - 1, GetMsg(MConfirmLaunchMessage), filepaths.size(), app.name.c_str());
		const wchar_t* items[] = { GetMsg(MConfirmLaunchTitle), message };
		int res = s_info.Message(s_info.ModuleNumber, FMSG_MB_YESNO, nullptr, items, ARRAYSIZE(items), 2);
		return (res == 0);
	}


	// Executes one or more command lines to launch the application.
	// If multiple commands are provided, it forces asynchronous execution to avoid blocking the UI.
	void OpenWithPlugin::LaunchApplication(const CandidateInfo& app, const std::vector<std::wstring>& cmds)
	{
		if (cmds.empty()) return;

		// If we have multiple commands to run, force asynchronous execution to avoid blocking.
		bool force_no_wait = cmds.size() > 1;

		unsigned int flags = 0;
		if (app.terminal) {
			flags = s_use_external_terminal ? EF_EXTERNALTERM : 0;
		} else {
			flags = (s_no_wait_for_command_completion || force_no_wait) ? EF_NOWAIT : 0;
		}

		for (const auto& cmd : cmds) {
			if (s_fsf.Execute(cmd.c_str(), flags) == -1) {
				ShowError(GetMsg(MError), { GetMsg(MCannotExecute), cmd.c_str() });
				break; // Stop on the first error.
			}
		}

		if (s_clear_selection) {
			s_info.Control(PANEL_ACTIVE, FCTL_UPDATEPANEL, 0, 0);
		}
	}


	// The main logic handler for both single and multiple files.
	// It gets candidate applications, displays a menu, and handles user actions.
	void OpenWithPlugin::ProcessFiles(const std::vector<std::wstring>& filepaths)
	{
		if (filepaths.empty()) {
			return;
		}

		// Get a platform-specific application provider.
		auto provider = AppProvider::CreateAppProvider(&OpenWithPlugin::GetMsg);

		// A cache for MIME profiles. It will be populated (lazily) only if the user presses F3 or if no apps are found.
		std::optional<std::vector<std::wstring>> unique_mime_profiles_cache;

		// Helper lambda to lazily get or populate the MIME profiles cache.
		// It's called only when the MIME info is actually needed.
		auto get_unique_mime_profiles = [&]() -> const std::vector<std::wstring>& {
			if (!unique_mime_profiles_cache.has_value()) {
				unique_mime_profiles_cache = provider->GetMimeTypes();
			}
			return *unique_mime_profiles_cache;
		};

		std::vector<CandidateInfo> candidates;

		// Lambda to fetch and filter application candidates.
		auto update_candidates = [&]() {
			// Fetch the raw list of candidates from the platform-specific provider.
			candidates = provider->GetAppCandidates(filepaths);

			// When multiple files are selected and the internal far2l console is used, we must filter out terminal-based applications
			// because the internal console cannot handle multiple concurrent instances.
			if (filepaths.size() > 1 && !s_use_external_terminal) {
				candidates.erase(
					std::remove_if(candidates.begin(), candidates.end(),
								   [](const CandidateInfo& c) { return c.terminal && !c.multi_file_aware; }),
					candidates.end());
			}
		};

		// Perform the initial fetch and filtering of application candidates.
		update_candidates();

		constexpr int BREAK_KEYS[] = {VK_F3, VK_F9, 0};
		constexpr int KEY_F3_DETAILS = 0;
		constexpr int KEY_F9_OPTIONS = 1;

		int break_code = -1;
		int active_idx = 0;

		// Main application selection menu loop.
		while(true) {
			if (candidates.empty()) {
				std::vector<std::wstring> error_lines = { GetMsg(MNoAppsFound) };

				// Get the MIME types (lazily) only now that we need them for the error message.
				const auto& unique_mimes = get_unique_mime_profiles();

				error_lines.push_back(JoinStrings(unique_mimes, L"; "));

				ShowError(GetMsg(MError), error_lines);
				return;	// No application candidates; exit the plugin entirely
			}

			std::vector<FarMenuItem> menu_items(candidates.size());
			for (size_t i = 0; i < candidates.size(); ++i) {
				menu_items[i].Text = candidates[i].name.c_str();
			}

			menu_items[active_idx].Selected = true;

			// Display the menu and get the user's selection.
			int selected_idx = s_info.Menu(s_info.ModuleNumber, -1, -1, 0, FMENU_WRAPMODE | FMENU_SHOWAMPERSAND | FMENU_CHANGECONSOLETITLE,
										   GetMsg(MChooseApplication), L"F3 F9 Ctrl+Alt+F", L"Contents", BREAK_KEYS, &break_code, menu_items.data(), menu_items.size());

			if (selected_idx == -1) {
				return; // User cancelled the menu (e.g., with Esc); exit the plugin entirely
			}

			active_idx = selected_idx;
			const auto& selected_app = candidates[selected_idx];

			if (break_code == KEY_F3_DETAILS) {
				std::vector<std::wstring> cmds = provider->ConstructLaunchCommands(selected_app, filepaths);
				// Repeat until user either launches the application or closes the dialog to go back.
				while (true) {
					// Get MIME profiles (lazily) and pass them to the details dialog.
					bool wants_to_launch = ShowDetailsDialog(provider.get(), selected_app, filepaths, cmds, get_unique_mime_profiles());
					if (!wants_to_launch) {
						// User clicked "Close", break the inner loop to return to the main menu.
						break;
					}

					// User clicked "Launch", so ask for confirmation if needed.
					if (AskForLaunchConfirmation(selected_app, filepaths)) {
						// Confirmation was given. Launch the application and exit the plugin entirely.
						LaunchApplication(selected_app, cmds);
						return;
					}
				}

			} else if (break_code == KEY_F9_OPTIONS) {
				const auto configure_result = ConfigureImpl();

				// Check if settings were saved AND if a refresh is required. A refresh is needed if any setting that affects
				// the candidate list (e.g., s_UseExternalTerminal or any platform-specific option) has been changed.
				if (configure_result.settings_saved && configure_result.refresh_needed) {
					// The provider needs to reload its own settings from the config file.
					provider->LoadPlatformSettings();

					// Re-run the candidate fetch and filter logic to update the menu.
					update_candidates();

					// Reset the active menu item to the first one, as the list may have changed.
					active_idx = 0;

					// Invalidate the mime cache, as settings affecting it might have changed.
					unique_mime_profiles_cache.reset();
				}

			} else { // Enter to launch.
				if (AskForLaunchConfirmation(selected_app, filepaths)) {
					std::vector<std::wstring> cmds = provider->ConstructLaunchCommands(selected_app, filepaths);
					LaunchApplication(selected_app, cmds);
					return; // Exit the plugin after a successful launch.
				}
			}
		}
	}


	void OpenWithPlugin::LoadOptions()
	{
		KeyFileReadSection kfh(INI_LOCATION, INI_SECTION);
		s_use_external_terminal = kfh.GetInt("UseExternalTerminal", 0) != 0;
		s_no_wait_for_command_completion = kfh.GetInt("NoWaitForCommandCompletion", 1) != 0;
		s_clear_selection = kfh.GetInt("ClearSelection", 0) != 0;
		s_confirm_launch = kfh.GetInt("ConfirmLaunch", 1) != 0;

		s_confirm_launch_threshold = kfh.GetInt("ConfirmLaunchThreshold", 10);
		s_confirm_launch_threshold = std::clamp(s_confirm_launch_threshold, 1, 9999);
	}


	void OpenWithPlugin::SaveOptions()
	{
		KeyFileHelper kfh(INI_LOCATION);
		kfh.SetInt(INI_SECTION, "UseExternalTerminal", s_use_external_terminal);
		kfh.SetInt(INI_SECTION, "NoWaitForCommandCompletion", s_no_wait_for_command_completion);
		kfh.SetInt(INI_SECTION, "ClearSelection", s_clear_selection);
		kfh.SetInt(INI_SECTION, "ConfirmLaunch", s_confirm_launch);

		s_confirm_launch_threshold = std::clamp(s_confirm_launch_threshold, 1, 9999);
		kfh.SetInt(INI_SECTION, "ConfirmLaunchThreshold", s_confirm_launch_threshold);

		if (!kfh.Save()) {
			ShowError(GetMsg(MError), { GetMsg(MSaveConfigError) });
		}
	}


	void OpenWithPlugin::ShowError(const wchar_t *title, const std::vector<std::wstring>& text)
	{
		std::vector<const wchar_t*> items;
		items.reserve(text.size() + 2);
		items.push_back(title);
		for (const auto &line : text) items.push_back(line.c_str());
		items.push_back(GetMsg(MOk));
		s_info.Message(s_info.ModuleNumber, FMSG_WARNING, nullptr, items.data(), items.size(), 1);
	}


	// Helper function to join a vector of wstrings with a delimiter.
	std::wstring OpenWithPlugin::JoinStrings(const std::vector<std::wstring>& vec, const std::wstring& delimiter)
	{
		if (vec.empty()) return L"";
		std::wstring result = vec[0];
		for (size_t i = 1; i < vec.size(); ++i) {
			result += delimiter;
			result += vec[i];
		}
		return result;
	}


	int OpenWithPlugin::GetScreenWidth()
	{
		SMALL_RECT rect;
		if (s_info.AdvControl(s_info.ModuleNumber, ACTL_GETFARRECT, &rect, 0)) {
			return rect.Right - rect.Left + 1;
		}
		return 0;
	}


	// Static member initialization.
	PluginStartupInfo OpenWithPlugin::s_info = {};
	FarStandardFunctions OpenWithPlugin::s_fsf = {};
	bool OpenWithPlugin::s_use_external_terminal = false;
	bool OpenWithPlugin::s_no_wait_for_command_completion = true;
	bool OpenWithPlugin::s_clear_selection = false;
	bool OpenWithPlugin::s_confirm_launch = true;
	int OpenWithPlugin::s_confirm_launch_threshold = 10;


	// Plugin entry points

	SHAREDSYMBOL void WINAPI SetStartupInfoW(const PluginStartupInfo *Info)
	{
		OpenWith::OpenWithPlugin::SetStartupInfo(Info);
	}

	SHAREDSYMBOL void WINAPI GetPluginInfoW(PluginInfo *Info)
	{
		OpenWith::OpenWithPlugin::GetPluginInfo(Info);
	}

	SHAREDSYMBOL HANDLE WINAPI OpenPluginW(int OpenFrom, INT_PTR Item)
	{
		return OpenWith::OpenWithPlugin::OpenPlugin(OpenFrom, Item);
	}

	SHAREDSYMBOL int WINAPI ConfigureW(int ItemNumber)
	{
		return OpenWith::OpenWithPlugin::Configure(ItemNumber);
	}

	SHAREDSYMBOL void WINAPI ExitFARW()
	{
		OpenWith::OpenWithPlugin::Exit();
	}

	SHAREDSYMBOL int WINAPI GetMinFarVersionW()
	{
		return FARMANAGERVERSION;
	}

} // namespace OpenWith
