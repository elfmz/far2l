#pragma once

#include "AppProvider.hpp"
#include "farplug-wide.h"
#include "WinCompat.h"
#include "WinPort.h"
#include "common.hpp"
#include "utils.h"
#include <string>
#include <vector>

namespace OpenWith {

class OpenWithPlugin
{
public:

	struct ConfigureResult
	{
		bool settings_saved = false;  // true if the user clicked "Ok" and settings were saved.
		bool refresh_needed = false; // true if a setting affecting the candidate list was changed.
	};

	static void SetStartupInfo(const PluginStartupInfo *plugin_startup_info);
	static void GetPluginInfo(PluginInfo *plugin_info);
	static HANDLE OpenPlugin(int open_from, INT_PTR item);
	static ConfigureResult ConfigureImpl();
	static int Configure(int item_number);
	static void Exit();
	static const wchar_t* GetMsg(int msg_id);

private:
	static PluginStartupInfo s_info;
	static FarStandardFunctions s_fsf;
	static bool s_use_external_terminal;
	static bool s_no_wait_for_command_completion;
	static bool s_clear_selection;
	static bool s_confirm_launch;
	static int s_confirm_launch_threshold;

	static bool ShowDetailsDialogImpl(const std::vector<Field>& file_info, const std::vector<Field>& application_info, const Field& launch_command);
	static bool ShowDetailsDialog(AppProvider* provider, const CandidateInfo& app, const std::vector<std::wstring>& filepaths,  const std::vector<std::wstring>& cmds, const std::vector<std::wstring>& unique_mime_profiles);
	static bool AskForLaunchConfirmation(const CandidateInfo& app, const std::vector<std::wstring>& filepaths);
	static void LaunchApplication(const CandidateInfo& app, const std::vector<std::wstring>& cmds);
	static void ProcessFiles(const std::vector<std::wstring>& filepaths);
	static void LoadOptions();
	static void SaveOptions();
	static void ShowError(const wchar_t *title, const std::vector<std::wstring>& text);
	static std::wstring JoinStrings(const std::vector<std::wstring>& vec, const std::wstring& delimiter);

	static int GetScreenWidth();
};


} // namespace OpenWith
