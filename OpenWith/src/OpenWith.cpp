#include "AppProvider.hpp"
#include "farplug-wide.h"
#include "KeyFileHelper.h"
#include "WinCompat.h"
#include "lng.hpp"
#include "common.hpp"
#include "utils.h"
#include <algorithm>
#include <cstdio>
#include <memory>
#include <string>
#include <vector>

#define INI_LOCATION InMyConfig("plugins/openwith/config.ini")
#define INI_SECTION  "Settings"

namespace OpenWith {


class OpenWithPlugin
{
private:
	static PluginStartupInfo s_Info;
	static FarStandardFunctions s_FSF;
	static bool s_UseExternalTerminal;
	static bool s_NoWaitForCommandCompletion;

	// return true if exit by button "Launch", false otherwise
	static bool ShowDetailsDialogImpl(const std::vector<Field>& file_info,
										const std::vector<Field>& application_info,
										const Field& launch_command)
	{
		constexpr int DIALOG_WIDTH = 80;
		int dialog_height = file_info.size() + application_info.size() + 9;

		// Helper lambda to find the maximum label length in a vector of Fields for alignment.
		auto max_in = [](const std::vector<Field>& v) -> size_t {
			if (v.empty()) return 0;
			return std::max_element(v.begin(), v.end(),
				[](const Field& x, const Field& y){ return x.label.size() < y.label.size(); })->label.size();
		};

		auto max_di_text_length = static_cast<int>(std::max({
			launch_command.label.size(),
			max_in(file_info),
			max_in(application_info)
		}));

		// Calculate coordinates for dialog items to right-align all text labels.
		int di_text_X2 = max_di_text_length + 4;
		int di_edit_X1 = max_di_text_length + 6;
		int di_edit_X2 = DIALOG_WIDTH - 6;

		std::vector<FarDialogItem> di;

		di.push_back({ DI_DOUBLEBOX, 3,  1, DIALOG_WIDTH - 4,  dialog_height - 2, FALSE, {}, 0, 0, GetMsg(MDetails), 0 });

		int cur_line = 2;

		for (auto &field : file_info) {
			int di_text_X1 = di_text_X2 - field.label.size() + 1;
			di.push_back({ DI_TEXT, di_text_X1, cur_line,  di_text_X2, cur_line, FALSE, {}, 0, 0, field.label.c_str(), 0 });
			di.push_back({ DI_EDIT, di_edit_X1, cur_line,  di_edit_X2, cur_line, FALSE, {}, DIF_READONLY | DIF_SELECTONENTRY, 0,  field.content.c_str(), 0});
			++cur_line;
		}

		di.push_back({ DI_TEXT, 5,  cur_line,  0,  cur_line, FALSE, {}, DIF_SEPARATOR, 0, L"", 0 });
		++cur_line;

		for (auto &field : application_info) {
			int di_text_X1 = di_text_X2 - field.label.size() + 1;
			di.push_back({ DI_TEXT, di_text_X1, cur_line,  di_text_X2, cur_line, FALSE, {}, 0, 0, field.label.c_str(), 0 });
			di.push_back({ DI_EDIT, di_edit_X1, cur_line,  di_edit_X2, cur_line, FALSE, {}, DIF_READONLY | DIF_SELECTONENTRY, 0,  field.content.c_str(), 0});
			++cur_line;
		}

		di.push_back({ DI_TEXT, 5,  cur_line,  0,  cur_line, FALSE, {}, DIF_SEPARATOR, 0, L"", 0 });
		++cur_line;

		int di_text_X1 = di_text_X2 - launch_command.label.size() + 1;
		di.push_back({ DI_TEXT, di_text_X1, cur_line,  di_text_X2, cur_line, FALSE, {}, 0, 0, launch_command.label.c_str(), 0 });
		di.push_back({ DI_EDIT, di_edit_X1, cur_line,  di_edit_X2, cur_line, FALSE, {}, DIF_READONLY | DIF_SELECTONENTRY, 0,  launch_command.content.c_str(), 0});
		++cur_line;

		di.push_back({ DI_TEXT, 5,  cur_line,  0,  cur_line, FALSE, {}, DIF_SEPARATOR, 0, L"", 0 });
		++cur_line;

		di.push_back({ DI_BUTTON, 0,  cur_line,  0,  cur_line, TRUE, {}, DIF_CENTERGROUP, 0, GetMsg(MClose), 0 });

		di.back().DefaultButton = TRUE;

		di.push_back({ DI_BUTTON, 0,  cur_line,  0,  cur_line, TRUE, {}, DIF_CENTERGROUP, 0, GetMsg(MLaunch), 0 });

		HANDLE dlg = s_Info.DialogInit(s_Info.ModuleNumber, -1, -1, DIALOG_WIDTH, dialog_height, L"InformationDialog",
										di.data(), static_cast<int>(di.size()), 0, 0, nullptr, 0);
		if (dlg != INVALID_HANDLE_VALUE) {
			int exitCode = s_Info.DialogRun(dlg);
			s_Info.DialogFree(dlg);
			return (exitCode == (int)di.size() - 1); // last element is button "Launch"
		}
		return false;
	}

	// A wrapper function that gathers all necessary details and calls the dialog implementation.
	// return true if exit by button "Launch", false otherwise
	static bool ShowDetailsDialog(AppProvider* provider, const CandidateInfo& app,
									const std::wstring& pathname, const std::wstring& cmd)
	{
		std::vector<Field> file_info = {
			{ GetMsg(MPathname), pathname },
			{ GetMsg(MMimeType), provider->GetMimeType(pathname) }
		};

		std::vector<Field> application_info = provider->GetCandidateDetails(app);

		Field launch_command { GetMsg(MLaunchCommand), cmd.c_str() };

		return ShowDetailsDialogImpl(file_info, application_info, launch_command);
	}


	static void LaunchApplication(const CandidateInfo& app, const std::wstring& cmd)
	{
		unsigned int flags = 0;
		// Determine execution flags based on the app's terminal requirement and global plugin settings.
		if (app.terminal) {
			// If the app requires a terminal, decide whether to use far2l's external terminal feature.
			flags = s_UseExternalTerminal ? EF_EXTERNALTERM : 0;
		} else {
			// For GUI apps, decide whether to wait for completion or run in the background.
			flags = s_NoWaitForCommandCompletion ? EF_NOWAIT : 0;
		}
		if (s_FSF.Execute(cmd.c_str(), flags) == -1) {
			ShowError(GetMsg(MError), { GetMsg(MCannotExecute) });
		}
	}


	static void ProcessFile(const std::wstring &pathname)
	{
		// The factory method creates an appropriate provider for the current OS (Linux, macOS, etc.).
		auto provider = AppProvider::CreateAppProvider(&OpenWithPlugin::GetMsg);
		auto candidates = provider->GetAppCandidates(pathname);

		if (candidates.empty()) {
			ShowError(GetMsg(MError), { GetMsg(MNoAppsFound) , provider->GetMimeType(pathname) });
			return;
		}

		std::vector<FarMenuItem> menu_items(candidates.size());
		for (size_t i = 0; i < candidates.size(); ++i) {
			menu_items[i].Text = candidates[i].name.c_str();
		}

		int BreakCode = -1;
		const int BreakKeys[] = {VK_F3, 0};
		int active_idx = 0;

		// This loop re-displays the menu, allowing the user to press F3 for details 
		// and then return to the same menu without starting over.
		while(true) {
			menu_items[active_idx].Selected = true;

			int selected_idx = s_Info.Menu(s_Info.ModuleNumber, -1, -1, 0, FMENU_WRAPMODE | FMENU_SHOWAMPERSAND | FMENU_CHANGECONSOLETITLE,
							GetMsg(MChooseApplication), L"F3 Ctrl+Alt+F", L"Contents", BreakKeys, &BreakCode, menu_items.data(), menu_items.size());

			if (selected_idx == -1) {
				break; // User cancelled the menu (e.g., with Esc).
			}

			menu_items[active_idx].Selected = false;
			active_idx = selected_idx;

			const auto& selected_app = candidates[selected_idx];
			std::wstring cmd = provider->ConstructCommandLine(selected_app, pathname);

			// BreakCode corresponds to the index in the BreakKeys array. 0 means the first key (VK_F3) was pressed.
			if (BreakCode == 0) { // F3
				if (ShowDetailsDialog(provider.get(), selected_app, pathname, cmd)) {
					// if dialog closed by Launch button do launch and close
					LaunchApplication(selected_app, cmd);
					break;
				}
			} else { // Enter
				LaunchApplication(selected_app, cmd);
				break;
			}
		}
	}


	static void LoadOptions()
	{
		KeyFileReadSection kfh(INI_LOCATION, INI_SECTION);
		s_UseExternalTerminal = kfh.GetInt("UseExternalTerminal", 0) != 0;
		s_NoWaitForCommandCompletion = kfh.GetInt("NoWaitForCommandCompletion", 1) != 0;
	}


	static void SaveOptions()
	{
		KeyFileHelper kfh(INI_LOCATION);
		kfh.SetInt(INI_SECTION, "UseExternalTerminal", s_UseExternalTerminal);
		kfh.SetInt(INI_SECTION, "NoWaitForCommandCompletion", s_NoWaitForCommandCompletion);
		if (!kfh.Save()) {
			ShowError(GetMsg(MError), { GetMsg(MSaveConfigError) });
		}
	}


	static void ShowError(const wchar_t *title, const std::vector<std::wstring>& text)
	{
		std::vector<const wchar_t*> items;
		items.reserve(text.size() + 2);
		items.push_back(title);
		for (const auto &line : text) items.push_back(line.c_str());
		items.push_back(GetMsg(MOk));
		s_Info.Message(s_Info.ModuleNumber, FMSG_WARNING, nullptr, items.data(), items.size(), 1);
	}


public:
	// Standard far2l plugin entry point for initialization.
	static void SetStartupInfo(const PluginStartupInfo *info)
	{
		s_Info = *info;
		s_FSF = *info->FSF;
		s_Info.FSF = &s_FSF;
		LoadOptions();
	}


	// Standard far2l plugin entry point to provide information about the plugin.
	static void GetPluginInfo(PluginInfo *info)
	{
		info->StructSize = sizeof(*info);
		info->Flags = 0;
		static const wchar_t *menuStr[1];
		menuStr[0] = GetMsg(MPluginTitle);
		info->PluginMenuStrings = menuStr;
		info->PluginMenuStringsNumber = ARRAYSIZE(menuStr);
		static const wchar_t *configStr[1];
		configStr[0] = GetMsg(MPluginTitle);
		info->PluginConfigStrings = configStr;
		info->PluginConfigStringsNumber = ARRAYSIZE(configStr);
		info->CommandPrefix = nullptr;
	}


	// Standard far2l plugin entry point called when the user invokes the plugin.
	static HANDLE OpenPlugin(int openFrom, INT_PTR item)
	{
		if (openFrom != OPEN_PLUGINSMENU) {
			return INVALID_HANDLE_VALUE;
		}

		PanelInfo pi = {};

		if (!s_Info.Control(PANEL_ACTIVE, FCTL_GETPANELINFO, 0, (LONG_PTR)&pi)) {
			return INVALID_HANDLE_VALUE;
		}

		if (pi.PanelType != PTYPE_FILEPANEL || pi.ItemsNumber <= 0 || pi.CurrentItem < 0 || pi.CurrentItem >= pi.ItemsNumber) {
			return INVALID_HANDLE_VALUE;
		}
		
		// To get a panel item, we must first query its required size.
		int itemSize = s_Info.Control(PANEL_ACTIVE, FCTL_GETPANELITEM, pi.CurrentItem, 0);
		if (itemSize <= 0) {
			return INVALID_HANDLE_VALUE;
		}

		auto item_buf = std::make_unique<unsigned char[]>(itemSize);
		PluginPanelItem *pi_item = reinterpret_cast<PluginPanelItem *>(item_buf.get());

		// Then, we retrieve the actual item data into our allocated buffer.
		if (!s_Info.Control(PANEL_ACTIVE, FCTL_GETPANELITEM, pi.CurrentItem, (LONG_PTR)pi_item)) {
			return INVALID_HANDLE_VALUE;
		}

		if (!pi_item->FindData.lpwszFileName) {
			return INVALID_HANDLE_VALUE;
		}

		// Similar to panel items, we first query the size needed for the panel's directory path.
		int dir_size = s_Info.Control(PANEL_ACTIVE, FCTL_GETPANELDIR, 0, 0);
		if (dir_size <= 0) {
			return INVALID_HANDLE_VALUE;
		}

		auto dir_buf = std::make_unique<wchar_t[]>(dir_size);
		// And then retrieve the path string.
		if (!s_Info.Control(PANEL_ACTIVE, FCTL_GETPANELDIR, dir_size, (LONG_PTR)dir_buf.get())) {
			return INVALID_HANDLE_VALUE;
		}

		// Concatenate directory and filename to form a full, unambiguous path.
		std::wstring pathname(dir_buf.get());
		if (!pathname.empty() && pathname.back() != L'/') {
			pathname += L'/';
		}
		pathname += pi_item->FindData.lpwszFileName;
		ProcessFile(pathname);
		return INVALID_HANDLE_VALUE; // We don't create a plugin panel, so return INVALID_HANDLE_VALUE.
	}


	static int Configure(int itemNumber)
	{
		LoadOptions();

		// Create a provider instance to fetch platform-specific settings for the dialog.
		auto provider = AppProvider::CreateAppProvider(&OpenWithPlugin::GetMsg);
		provider->LoadPlatformSettings();
		std::vector<ProviderSetting> platform_settings = provider->GetPlatformSettings();

		std::vector<FarDialogItem> di;
		int y = 1;

		di.push_back({ DI_CHECKBOX, 5, ++y, 0, 0, TRUE, { s_UseExternalTerminal }, 0, 0, GetMsg(MUseExternalTerminal), 0 });
		di.push_back({ DI_CHECKBOX, 5, ++y, 0, 0, 0, { s_NoWaitForCommandCompletion },  0, 0, GetMsg(MNoWaitForCommandCompletion), 0});

		if (!platform_settings.empty()) {
			di.push_back({ DI_TEXT, 5, ++y, 0, 0, FALSE, {}, DIF_SEPARATOR, 0, L"", 0 });
			for (const auto& setting : platform_settings) {
				di.push_back({ DI_CHECKBOX, 5, ++y, 0, 0, FALSE, { setting.value }, 0, 0, setting.display_name.c_str(), 0 });
			}
		}

		di.push_back({ DI_TEXT, 5, ++y, 0, 0, FALSE, {}, DIF_SEPARATOR, 0, L"", 0 });
		y++;
		di.push_back({ DI_BUTTON, 0, y, 0, 0, FALSE, {}, DIF_CENTERGROUP, 0, GetMsg(MOk), 0 });
		di.back().DefaultButton = TRUE;
		di.push_back({ DI_BUTTON, 0, y, 0, 0, FALSE, {}, DIF_CENTERGROUP, 0, GetMsg(MCancel), 0 });

		int dialog_height = y + 3;
		int dialog_width = 70;
		di.insert(di.begin(), { DI_DOUBLEBOX, 3, 1, dialog_width - 4, dialog_height - 2, FALSE, {}, 0, 0, GetMsg(MConfigTitle), 0 });

		HANDLE dlg = s_Info.DialogInit(s_Info.ModuleNumber, -1, -1, dialog_width, dialog_height, L"ConfigurationDialog", di.data(), di.size(), 0, 0, nullptr, 0);
		if (dlg == INVALID_HANDLE_VALUE) return FALSE;

		int exitCode = s_Info.DialogRun(dlg);

		// The index of the 'OK' button is determined by its position in the 'di' vector.
		if (exitCode == (int)di.size() - 2) { // OK

			// Retrieve new settings values from the dialog controls.
			s_UseExternalTerminal = (s_Info.SendDlgMessage(dlg, DM_GETCHECK, 1, 0) == BSTATE_CHECKED);
			s_NoWaitForCommandCompletion = (s_Info.SendDlgMessage(dlg, DM_GETCHECK, 2, 0) == BSTATE_CHECKED);
			SaveOptions();

			if (!platform_settings.empty()) {
				std::vector<ProviderSetting> new_settings;
				// The first platform-specific checkbox is at index 4 (0:box, 1:check, 2:check, 3:sep).
				int first_platform_item_idx = 4;
				for (size_t i = 0; i < platform_settings.size(); ++i) {
					bool new_value = (s_Info.SendDlgMessage(dlg, DM_GETCHECK, first_platform_item_idx + i, 0) == BSTATE_CHECKED);
					new_settings.push_back({ platform_settings[i].internal_key, platform_settings[i].display_name, new_value });
				}
				provider->SetPlatformSettings(new_settings);
				provider->SavePlatformSettings();
			}
		}
		s_Info.DialogFree(dlg);
		return TRUE;
	}


	static void Exit() {}


	static const wchar_t* GetMsg(int MsgId)
	{
		return s_Info.GetMsg(s_Info.ModuleNumber, MsgId);
	}
};

// Static member initialization.
PluginStartupInfo OpenWithPlugin::s_Info = {};
FarStandardFunctions OpenWithPlugin::s_FSF = {};
bool OpenWithPlugin::s_UseExternalTerminal = false;
bool OpenWithPlugin::s_NoWaitForCommandCompletion = true;

// Plugin entry points

SHAREDSYMBOL void WINAPI SetStartupInfoW(const PluginStartupInfo *info)
{
	OpenWith::OpenWithPlugin::SetStartupInfo(info);
}

SHAREDSYMBOL void WINAPI GetPluginInfoW(PluginInfo *info)
{
	OpenWith::OpenWithPlugin::GetPluginInfo(info);
}

SHAREDSYMBOL HANDLE WINAPI OpenPluginW(int openFrom, INT_PTR item)
{
	return OpenWith::OpenWithPlugin::OpenPlugin(openFrom, item);
}

SHAREDSYMBOL int WINAPI ConfigureW(int itemNumber)
{
	return OpenWith::OpenWithPlugin::Configure(itemNumber);
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
