#pragma once

#include "KeyFileHelper.h"
#include "farplug-wide.h"
#include "common.hpp"
#include <string>
#include <vector>

namespace OpenWith {

extern PluginStartupInfo g_info;
extern FarStandardFunctions g_fsf;

const wchar_t* GetMsg(int msg_id);

class OpenWithPlugin
{
public:

	struct ConfigDlgResult
	{
		bool should_refresh_candidates = false;
	};

	static void ProcessFiles(const std::vector<std::wstring>& filepaths);
	static ConfigDlgResult ShowConfigDlg();
	static void ShowError(const std::vector<std::wstring>& error_lines);
	static void LoadGeneralSettings(const KeyFileReadHelper& key_reader);

private:

	struct DetailsDlgResult
	{
		enum class Action { Close, Launch, GoTo } action;
		std::wstring goto_target = L"";
	};

	inline static bool s_use_external_terminal;
	inline static bool s_no_wait_for_command_completion;
	inline static bool s_clear_selection;
	inline static bool s_confirm_launch;
	inline static int s_confirm_launch_threshold;
	inline static bool s_display_filename;

	static void FilterOutTerminalCandidates(std::vector<CandidateInfo> &candidates, size_t file_count);
	static bool AskForLaunchConfirmation(const CandidateInfo& app, size_t file_count);

	enum class LaunchMode
	{
		Standard, // Enter
		Forced    // Shift+Enter
	};

	static void LaunchApplication(const CandidateInfo& app, const std::vector<std::wstring>& cmds, LaunchMode launch_mode = LaunchMode::Standard);
	static DetailsDlgResult ShowDetailsDlg(const std::vector<std::wstring>& filepaths, const std::vector<std::wstring>& unique_mime_profiles, const std::vector<Field> &application_info, const std::vector<std::wstring>& cmds, const std::vector<CandidateContextLocation>& locations);
	static bool GoToFile(const std::wstring &filepath);
	static void SaveGeneralSettings(KeyFileHelper& key_writer);
	static std::wstring JoinStrings(const std::vector<std::wstring>& strings, const std::wstring& delimiter);
	static size_t GetLabelCellWidth(const Field& field);
	static size_t GetMaxLabelCellWidth(const std::vector<Field>& fields);
	static int GetConsoleWidth();
	static std::wstring FormatMenuTitle(const std::vector<std::wstring>& filepaths);
};


} // namespace OpenWith
