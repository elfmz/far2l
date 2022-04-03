#include <utils.h>
#include "Globals.h"
#include "PluginImpl.h"
#include "BackgroundTasks.h"
#include "UI/Activities/BackgroundTasksUI.h"
#include "Protocol/Protocol.h"
#include <fcntl.h>
#include <set>
#include <sudo.h>

//LPCSTR      DiskMenuStrings[ 1+FTP_MAXBACKUPS ];

SHAREDSYMBOL void PluginModuleOpen(const char *path)
{
	MB2Wide(path, G.plugin_path);
	//G.plugin_path = path;
//	fprintf(stderr, "NetRocks::PluginModuleOpen\n");
}

SHAREDSYMBOL int WINAPI _export GetMinFarVersionW(void)
{
	#define MAKEFARVERSION(major,minor) ( ((major)<<16) | (minor))
	return MAKEFARVERSION(2, 2);
}


SHAREDSYMBOL void WINAPI _export SetStartupInfoW(const struct PluginStartupInfo *Info)
{
	G.Startup(Info);
//	fprintf(stderr, "FSF=%p ExecuteLibrary=%p\n", G.info.FSF, G.info.FSF ? G.info.FSF->ExecuteLibrary : nullptr);

}

SHAREDSYMBOL HANDLE WINAPI _export OpenFilePluginW(const wchar_t *Name, const unsigned char *Data, int DataSize, int OpMode)
{
	if (!G.IsStarted() || !Name) {
		return INVALID_HANDLE_VALUE;
	}

	// Only user can start config files browsing, avoid doing that during file search etc
	if ( (OpMode & (~(OPM_PGDN))) != 0) {
		return INVALID_HANDLE_VALUE;
	}


	// Filter out files that has name not suffixed by NETROCKS_EXPORT_SITE_EXTENSION
	size_t l = wcslen(Name);
	if (l <= ARRAYSIZE(NETROCKS_EXPORT_SITE_EXTENSION)) {
		return INVALID_HANDLE_VALUE;
	}

	if (wmemcmp(Name + (l + 1 - ARRAYSIZE(NETROCKS_EXPORT_SITE_EXTENSION)),
			L"" NETROCKS_EXPORT_SITE_EXTENSION, ARRAYSIZE(NETROCKS_EXPORT_SITE_EXTENSION) - 1) != 0) {
		return INVALID_HANDLE_VALUE;
	}

	// Bit deeper check - expecting first line to be section name,
	// so expecting first non-empty character to be '['.
	// This BTW throws away any comments.. But NetRocks doesn't write comments.
	for (int i = 0; i < DataSize; ++i) {
		if (Data[i] != ' ' && Data[i] != '\t' && Data[i] != '\r' && Data[i] != '\n') {
			if (Data[i] == '[') {
				break;
			}
			return INVALID_HANDLE_VALUE;
		}
	}

	return new PluginImpl(Name, true, OpMode);
}


SHAREDSYMBOL HANDLE WINAPI _export OpenPluginW(int OpenFrom, INT_PTR Item)
{
	fprintf(stderr, "NetRocks: OpenPlugin(%d, '0x%lx')\n", OpenFrom, (unsigned long)Item);

	if (!G.IsStarted())// || OpenFrom != OPEN_COMMANDLINE)
		return INVALID_HANDLE_VALUE;

	if (OpenFrom == OPEN_PLUGINSMENU) {
		if (Item == 1) {
			BackgroundTasksList();
			return INVALID_HANDLE_VALUE;
		}

		Item = 0;
//		static const wchar_t s_curdir_cmd[] = L"file:.";
//		Item = (INT_PTR)&s_curdir_cmd[0];
	}


	const wchar_t *path = (Item > 0xfff) ? (const wchar_t *)Item : nullptr;

	try {
		return new PluginImpl(path);

	} catch (std::exception &ex) {
		const std::wstring &tmp_what = MB2Wide(ex.what());
		const wchar_t *msg[] = { G.GetMsgWide(MError), tmp_what.c_str(), G.GetMsgWide(MOK)};
		G.info.Message(G.info.ModuleNumber, FMSG_WARNING, nullptr, msg, ARRAYSIZE(msg), 1);
		return INVALID_HANDLE_VALUE;
	}
}

SHAREDSYMBOL void WINAPI _export ClosePluginW(HANDLE hPlugin)
{
	delete (PluginImpl *)hPlugin;
}


SHAREDSYMBOL int WINAPI _export GetFindDataW(HANDLE hPlugin,struct PluginPanelItem **pPanelItem,int *pItemsNumber,int OpMode)
{
	return ((PluginImpl *)hPlugin)->GetFindData(pPanelItem, pItemsNumber, OpMode);
}


SHAREDSYMBOL void WINAPI _export FreeFindDataW(HANDLE hPlugin,struct PluginPanelItem *PanelItem,int ItemsNumber)
{
	((PluginImpl *)hPlugin)->FreeFindData(PanelItem, ItemsNumber);
}


SHAREDSYMBOL int WINAPI _export SetDirectoryW(HANDLE hPlugin,const wchar_t *Dir,int OpMode)
{
	return ((PluginImpl *)hPlugin)->SetDirectory(Dir, OpMode);
}


SHAREDSYMBOL int WINAPI _export DeleteFilesW(HANDLE hPlugin,struct PluginPanelItem *PanelItem,int ItemsNumber,int OpMode)
{
	return ((PluginImpl *)hPlugin)->DeleteFiles(PanelItem, ItemsNumber, OpMode);
}


SHAREDSYMBOL int WINAPI _export GetFilesW(HANDLE hPlugin,struct PluginPanelItem *PanelItem,
                   int ItemsNumber,int Move, const wchar_t **DestPath,int OpMode)
{
	return ((PluginImpl *)hPlugin)->GetFiles(PanelItem, ItemsNumber, Move, DestPath ? *DestPath : nullptr, OpMode);
}


SHAREDSYMBOL int WINAPI _export PutFilesW(HANDLE hPlugin,struct PluginPanelItem *PanelItem,
                   int ItemsNumber,int Move,const wchar_t *SrcPath, int OpMode)
{
	return ((PluginImpl *)hPlugin)->PutFiles(PanelItem, ItemsNumber, Move, SrcPath, OpMode);
}

SHAREDSYMBOL int WINAPI _export MakeDirectoryW(HANDLE hPlugin, const wchar_t **Name, int OpMode)
{
	return ((PluginImpl *)hPlugin)->MakeDirectory(Name, OpMode);
}

SHAREDSYMBOL int WINAPI ProcessEventW(HANDLE hPlugin, int Event, void * Param)
{
	switch (Event) {
		case FE_COMMAND:
			if (Param)
				return ((PluginImpl *)hPlugin)->ProcessEventCommand((const wchar_t *)Param);
		break;

		default:
			;
	}

	return 0;
}

SHAREDSYMBOL void WINAPI _export ExitFARW()
{
	BackgroundTasksInfo info;
	GetBackgroundTasksInfo(info);
	for (const auto &task_info : info) {
		if (task_info.status == BTS_ACTIVE || task_info.status == BTS_PAUSED) {
			fprintf(stderr, "NetRocks::ExitFARW: aborting task id=%lu info='%s'\n", task_info.id, task_info.information.c_str());
			AbortBackgroundTask(task_info.id);
		}
	}

	while (CountOfPendingBackgroundTasks() != 0) {
		if (G.info.FSF->DispatchInterThreadCalls() > 0) {
			usleep(1000);
		} else {
			usleep(100000);
		}
	}

	PluginImpl::sOnExiting();
}

SHAREDSYMBOL int WINAPI _export MayExitFARW()
{
	const size_t background_tasks_count = CountOfPendingBackgroundTasks();
	if ( background_tasks_count != 0 && !ConfirmExitFAR(background_tasks_count).Ask()) {
		return 0;
	}

	return 1;
}

static std::wstring CombineAllProtocolPrefixes()
{
	std::wstring out = L"net:";
	for (auto pi = ProtocolInfoHead(); pi->name; ++pi) {
		out+= MB2Wide(pi->name);
		out+= L':';
	}
	return out;
}

SHAREDSYMBOL void WINAPI _export GetPluginInfoW(struct PluginInfo *Info)
{
//	fprintf(stderr, "NetRocks: GetPluginInfoW\n");
	static const wchar_t *s_cfg_strings[2], *s_menu_strings[2];
	s_cfg_strings[0] = G.GetMsgWide(MPluginOptionsTitle);
	s_cfg_strings[1] = G.GetMsgWide(MBackgroundTasksTitle);
	s_menu_strings[0] = G.GetMsgWide(MTitle);
	s_menu_strings[1] = G.GetMsgWide(MBackgroundTasksTitle);
	static const wchar_t *s_disk_menu_strings[] = {L"NetRocks\0"};
	static std::wstring s_command_prefixes = CombineAllProtocolPrefixes();

	Info->StructSize = sizeof(*Info);
	Info->Flags = PF_FULLCMDLINE;
	Info->PluginConfigStrings = s_cfg_strings;
	Info->PluginConfigStringsNumber = (CountOfAllBackgroundTasks() != 0) ? ARRAYSIZE(s_cfg_strings) : 1;
	Info->PluginMenuStrings = s_menu_strings;
	Info->PluginMenuStringsNumber = (CountOfAllBackgroundTasks() != 0) ? ARRAYSIZE(s_menu_strings) : 1;
	Info->CommandPrefix = s_command_prefixes.c_str();
	Info->DiskMenuStrings           = s_disk_menu_strings;
	Info->DiskMenuStringsNumber     = ARRAYSIZE(s_disk_menu_strings);
}


SHAREDSYMBOL void WINAPI _export GetOpenPluginInfoW(HANDLE hPlugin,struct OpenPluginInfo *Info)
{
	((PluginImpl *)hPlugin)->GetOpenPluginInfo(Info);
}

SHAREDSYMBOL int WINAPI _export ProcessKeyW(HANDLE hPlugin,int Key,unsigned int ControlState)
{
	return ((PluginImpl *)hPlugin)->ProcessKey(Key, ControlState);
}

void ConfigurePluginGlobalOptions();

SHAREDSYMBOL int WINAPI _export ConfigureW(int ItemNumber)
{
	if (!G.IsStarted())
		return 0;

	switch (ItemNumber) {
		case 0: {
			ConfigurePluginGlobalOptions();
			PluginImpl::sOnGlobalSettingsChanged();
		} break;

		case 1: {
			BackgroundTasksList();
		} break;

		default: {
			fprintf(stderr, "NetRocks::ConfigureW(%d) - bad item\n", ItemNumber);
		}
	}
	return 1;
}
