#pragma once

#include "common.hpp"
#include "farplug-wide.h"
#include "KeyFileHelper.h"
#include <optional>
#include <string>
#include <vector>

namespace openwith
{
	extern PluginStartupInfo g_info;
	extern FarStandardFunctions g_fsf;

	class Plugin
	{
	public:

		struct ConfigDlgResult
		{
			bool should_refresh_candidates = false;
		};

		static void ProcessFiles(const std::vector<std::wstring>& filepaths);
		static ConfigDlgResult ShowConfigDlg();
		static void ShowErrorDlg(const std::vector<std::wstring>& error_lines);
		static void LoadGeneralSettings(const KeyFileReadHelper& key_reader);

	private:

		struct DlgLayout
		{
			static constexpr int BOX_BORDER_WIDTH  = 1;
			static constexpr int H_OUTER_MARGIN  = 3;
			static constexpr int H_INNER_PADDING = 1;
			static constexpr int H_SIDE_OVERHEAD = H_OUTER_MARGIN + BOX_BORDER_WIDTH + H_INNER_PADDING;
			static constexpr int H_TOTAL_OVERHEAD = H_SIDE_OVERHEAD * 2;
			static constexpr int V_OUTER_MARGIN = 1;
			static constexpr int V_SIDE_OVERHEAD = V_OUTER_MARGIN + BOX_BORDER_WIDTH;
			static constexpr int SEPARATOR_HEIGHT = 1;
			static constexpr int H_BUTTON_GAP = 1;
		};

		struct DetailsDlgResult
		{
			enum class Action
			{
				Close,
				Launch,
				GoTo
			};
			Action action = Action::Close;
			std::wstring goto_target;
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
		static int CalculateVisibleCellWidth(std::wstring_view text, bool literal_ampersands = false);
		static int GetLabelCellWidth(const Field& field);
		static int GetMaxLabelCellWidth(const std::vector<std::optional<Field>>& fields);
		static int GetButtonWidth(std::wstring_view caption);
		static int GetConsoleWidth();
		static std::wstring FormatMenuTitle(const std::vector<std::wstring>& filepaths);
	};

} // namespace openwith
