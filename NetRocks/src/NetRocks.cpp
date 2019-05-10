#include <utils.h>
#include "Globals.h"
#include "PluginImpl.h"
#include <fcntl.h>
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
	fprintf(stderr, "NetRocks: OpenPlugin(%d, '%ls')\n", OpenFrom, (const wchar_t *)Item);

	if (!G.IsStarted())// || OpenFrom != OPEN_COMMANDLINE)
		return INVALID_HANDLE_VALUE;


	const wchar_t *path = nullptr;

	if (OpenFrom == OPEN_COMMANDLINE) {
		if (wcsncmp((const wchar_t *)Item, G.command_prefix.c_str(), G.command_prefix.size()) != 0
		||  ((const wchar_t *)Item)[G.command_prefix.size()] != ':') {
			return INVALID_HANDLE_VALUE;
		}
		path = (const wchar_t *)Item + G.command_prefix.size() + 1;
	}


	return new PluginImpl(path);
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
//	abort();
}

SHAREDSYMBOL int WINAPI _export MayExitFARW()
{
//	abort();
	return 1;
}


SHAREDSYMBOL void WINAPI _export GetPluginInfoW(struct PluginInfo *Info)
{
	Info->StructSize = sizeof(*Info);

	Info->Flags = 0;//PF_FULLCMDLINE;
	static const wchar_t *PluginCfgStrings[1];
	PluginCfgStrings[0] = (wchar_t *)G.GetMsgWide(MTitle);
	Info->PluginConfigStrings = PluginCfgStrings;
	Info->PluginConfigStringsNumber = ARRAYSIZE(PluginCfgStrings);

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
