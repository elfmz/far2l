#pragma once

#include "AppProvider.hpp"
#include "farplug-wide.h"
#include "WinCompat.h"
#include "WinPort.h"
#include "common.hpp"
#include "utils.h"
#include <string>
#include <vector>
#include <optional>

namespace OpenWith {

extern PluginStartupInfo g_info;
extern FarStandardFunctions g_fsf;

const wchar_t* GetMsg(int msg_id);

class OpenWithPlugin
{
public:

	struct ConfigDlgResult
	{
		bool is_platform_settings_changed = false;
		bool should_refresh_candidates = false;
	};

	static void ProcessFiles(const std::vector<std::wstring>& filepaths);
	static ConfigDlgResult ShowConfigDlg();
	static void ShowError(const std::vector<std::wstring>& error_lines);
	static void LoadGeneralSettings();

private:

	enum class DetailsDlgResult { Close, Launch };

	static bool s_use_external_terminal;
	static bool s_no_wait_for_command_completion;
	static bool s_clear_selection;
	static bool s_confirm_launch;
	static int s_confirm_launch_threshold;

	static void FilterOutTerminalCandidates(std::vector<CandidateInfo> &candidates, size_t file_count);
	static bool AskForLaunchConfirmation(const CandidateInfo& app, size_t file_count);
	static void LaunchApplication(const CandidateInfo& app, const std::vector<std::wstring>& cmds);
	static DetailsDlgResult ShowDetailsDlg(const std::vector<std::wstring>& filepaths, const std::vector<std::wstring>& unique_mime_profiles, const std::vector<Field> &application_info, const std::vector<std::wstring>& cmds);
	static void SaveGeneralSettings();
	static std::wstring JoinStrings(const std::vector<std::wstring>& vec, const std::wstring& delimiter);
	static size_t GetLabelCellWidth(const Field& field);
	static size_t GetMaxLabelCellWidth(const std::vector<Field>& fields);
	static int GetScreenWidth();
};


} // namespace OpenWith
