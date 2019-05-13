#include <utils.h>
#include "Globals.h"
#include "PluginImpl.h"
#include "BackgroundTasks.h"
#include "UI/BackgroundTasksUI.h"
#include <fcntl.h>
#include <set>
#ifndef __APPLE__
# include <elf.h>
#endif
#include <sudo.h>

//LPCSTR      DiskMenuStrings[ 1+FTP_MAXBACKUPS ];

SHAREDSYMBOL void WINPORT_DllStartup(const char *path)
{
	MB2Wide(path, G.plugin_path);
	//G.plugin_path = path;
//	fprintf(stderr, "NetRocks::WINPORT_DllStartup\n");
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
/*
SHAREDSYMBOL HANDLE WINAPI _export OpenFilePluginw(const wchar_t *Name, const unsigned wchar_t *Data, int DataSize, int OpMode)
{
	if (!G.IsStarted())
		return INVALID_HANDLE_VALUE;


	PluginImpl *out = nullptr;
#ifndef __APPLE__
	if (elf) {
		out = new PluginImplELF(Name, Data[4], Data[5]);

	} else 
#endif
	if (plain) {
		out = new PluginImplPlain(Name, plain);

	} else
		abort();

	return out ? (HANDLE)out : INVALID_HANDLE_VALUE;
}
*/

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
	return ((PluginImpl *)hPlugin)->MakeDirectory(Name ? *Name : nullptr, OpMode);
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
		if (G.info.FSF->DispatchInterthreadCalls() > 0) {
			usleep(1000);
		} else {
			usleep(100000);
		}
	}
}

SHAREDSYMBOL int WINAPI _export MayExitFARW()
{
	const size_t background_tasks_count = CountOfPendingBackgroundTasks();
	if ( background_tasks_count != 0 && !ConfirmExitFAR(background_tasks_count).Ask()) {
		return 0;
	}

	return 1;
}


SHAREDSYMBOL void WINAPI _export GetPluginInfoW(struct PluginInfo *Info)
{
	Info->StructSize = sizeof(*Info);

	fprintf(stderr, "NetRocks: GetPluginInfoW\n");

	Info->Flags = PF_FULLCMDLINE;
	static const wchar_t *PluginCfgStrings[1] = {(wchar_t *)G.GetMsgWide(MTitle)};
	static const wchar_t *PluginMenuStrings[2] = {(wchar_t *)G.GetMsgWide(MTitle), (wchar_t *)G.GetMsgWide(MBackgroundTasksTitle)};

	Info->PluginConfigStrings = PluginCfgStrings;
	Info->PluginConfigStringsNumber = ARRAYSIZE(PluginCfgStrings);

	Info->PluginMenuStrings = PluginMenuStrings;
	Info->PluginMenuStringsNumber = (CountOfAllBackgroundTasks() != 0) ? ARRAYSIZE(PluginMenuStrings) : 1;

	static wchar_t s_command_prefix[G.MAX_COMMAND_PREFIX + 1] = {}; // WHY?
	wcsncpy(s_command_prefix, G.command_prefix.c_str(), ARRAYSIZE(s_command_prefix));
	Info->CommandPrefix = s_command_prefix;


//	static LPCSTR PluginMenuStrings[1];
//	static LPCSTR PluginCfgStrings[1];
//	static char     MenuString[MAX_PATH];
//	static char     CfgString[MAX_PATH];
//	snprintf(MenuString,     ARRAYSIZE(MenuString),     "%ls", "MNetRocksMenu");//FP_GetMsg(MFtpMenu));
//	snprintf(DiskStrings[0], ARRAYSIZE(DiskStrings[0]), "%ls", "MNetRocksDiskMenu");//(MFtpDiskMenu));
//	snprintf(CfgString,      ARRAYSIZE(CfgString),      "%ls", "MNetRocksMenu");//FP_GetMsg(MFtpMenu));
	static wchar_t *DiskMenuStrings[2] = {L"NetRocks\0", 0};
	Info->DiskMenuStrings           = DiskMenuStrings;
//	Info->DiskMenuNumbers           = 0;
	Info->DiskMenuStringsNumber     = 1;
//	Info->PluginMenuStrings         = PluginMenuStrings;
//	Info->PluginMenuStringsNumber   = Opt.AddToPluginsMenu ? ARRAYSIZE(PluginMenuStrings):0;
//	Info->PluginConfigStrings       = PluginCfgStrings;
//	Info->PluginConfigStringsNumber = ARRAYSIZE(PluginCfgStrings);
//	Info->CommandPrefix             = FTP_CMDPREFIX;
}


SHAREDSYMBOL void WINAPI _export GetOpenPluginInfoW(HANDLE hPlugin,struct OpenPluginInfo *Info)
{
	((PluginImpl *)hPlugin)->GetOpenPluginInfo(Info);
}

SHAREDSYMBOL int WINAPI _export ProcessKeyW(HANDLE hPlugin,int Key,unsigned int ControlState)
{
	return ((PluginImpl *)hPlugin)->ProcessKey(Key, ControlState);
}

SHAREDSYMBOL int WINAPI _export ConfigureW(int ItemNumber)
{
	if (!G.IsStarted())
		return 0;

	return 1;
/*
	struct FarDialogItem fdi[] = {
            {DI_DOUBLEBOX,  3,  1,  70, 5,  0,{},0,0, {}},
            {DI_TEXT,      -1,  2,  0,  2,  0,{},0,0, {}},
            {DI_BUTTON,     34, 4,  0,  4,  0,{},0,0, {}}
	};

	wcsncpy(fdi[0].Data, G.GetMsgWide(MTitle), ARRAYSIZE(fdi[0].Data));
	wcsncpy(fdi[1].Data, G.GetMsgWide(MDescription), ARRAYSIZE(fdi[1].Data));
	wcsncpy(fdi[2].Data, G.GetMsgWide(MOK), ARRAYSIZE(fdi[2].Data));

	G.info.Dialog(G.info.ModuleNumber, -1, -1, 74, 7, NULL, fdi, ARRAYSIZE(fdi));
	return 1;
*/
}
