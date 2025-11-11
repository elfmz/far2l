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

	static void SetStartupInfo(const PluginStartupInfo *info);
	static void GetPluginInfo(PluginInfo *info);
	static HANDLE OpenPlugin(int openFrom, INT_PTR item);
	static ConfigureResult ConfigureImpl();
	static int Configure(int itemNumber);
	static void Exit();
	static const wchar_t* GetMsg(int MsgId);

private:
	static PluginStartupInfo s_Info;
	static FarStandardFunctions s_FSF;
	static bool s_UseExternalTerminal;
	static bool s_NoWaitForCommandCompletion;
	static bool s_ClearSelection;
	static bool s_ConfirmLaunch;
	static int s_ConfirmLaunchThreshold;

	static bool ShowDetailsDialogImpl(const std::vector<Field>& file_info, const std::vector<Field>& application_info, const Field& launch_command);
	static bool ShowDetailsDialog(AppProvider* provider, const CandidateInfo& app, const std::vector<std::wstring>& pathnames,  const std::vector<std::wstring>& cmds, const std::vector<std::wstring>& unique_mimes);
	static bool AskForLaunchConfirmation(const CandidateInfo& app, const std::vector<std::wstring>& pathnames);
	static void LaunchApplication(const CandidateInfo& app, const std::vector<std::wstring>& cmds);
	static void ProcessFiles(const std::vector<std::wstring>& pathnames);
	static void LoadOptions();
	static void SaveOptions();
	static void ShowError(const wchar_t *title, const std::vector<std::wstring>& text);
	static std::wstring JoinStrings(const std::vector<std::wstring>& vec, const std::wstring& delimiter);

	static int GetScreenWidth();
};


} // namespace OpenWith
