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
	G.plugin_path = path;
//	fprintf(stderr, "NetRocks::WINPORT_DllStartup\n");
}

SHAREDSYMBOL int WINAPI _export GetMinFarVersion(void)
{
	#define MAKEFARVERSION(major,minor) ( ((major)<<16) | (minor))
	return MAKEFARVERSION(2, 1);
}


SHAREDSYMBOL void WINAPI _export SetStartupInfo(const struct PluginStartupInfo *Info)
{
	G.Startup(Info);
//	fprintf(stderr, "FSF=%p ExecuteLibrary=%p\n", G.info.FSF, G.info.FSF ? G.info.FSF->ExecuteLibrary : nullptr);

}
/*
SHAREDSYMBOL HANDLE WINAPI _export OpenFilePlugin(const char *Name, const unsigned char *Data, int DataSize, int OpMode)
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

SHAREDSYMBOL HANDLE WINAPI _export OpenPlugin(int OpenFrom, INT_PTR Item)
{
	fprintf(stderr, "NetRocks: OpenPlugin(%d, '%s')\n", OpenFrom, (const char *)Item);

	if (!G.IsStarted())// || OpenFrom != OPEN_COMMANDLINE)
		return INVALID_HANDLE_VALUE;


	const char *path = nullptr;

	if (OpenFrom == OPEN_COMMANDLINE) {
		if (strncmp((const char *)Item, G.command_prefix.c_str(), G.command_prefix.size()) != 0
		||  ((const char *)Item)[G.command_prefix.size()] != ':') {
			return INVALID_HANDLE_VALUE;
		}
		path = (const char *)Item + G.command_prefix.size() + 1;
	}


	return new PluginImpl(path);
}

SHAREDSYMBOL void WINAPI _export ClosePlugin(HANDLE hPlugin)
{
	delete (PluginImpl *)hPlugin;
}


SHAREDSYMBOL int WINAPI _export GetFindData(HANDLE hPlugin,struct PluginPanelItem **pPanelItem,int *pItemsNumber,int OpMode)
{
	return ((PluginImpl *)hPlugin)->GetFindData(pPanelItem, pItemsNumber, OpMode);
}


SHAREDSYMBOL void WINAPI _export FreeFindData(HANDLE hPlugin,struct PluginPanelItem *PanelItem,int ItemsNumber)
{
	((PluginImpl *)hPlugin)->FreeFindData(PanelItem, ItemsNumber);
}


SHAREDSYMBOL int WINAPI _export SetDirectory(HANDLE hPlugin,const char *Dir,int OpMode)
{
	return ((PluginImpl *)hPlugin)->SetDirectory(Dir, OpMode);
}


SHAREDSYMBOL int WINAPI _export DeleteFiles(HANDLE hPlugin,struct PluginPanelItem *PanelItem,int ItemsNumber,int OpMode)
{
	return ((PluginImpl *)hPlugin)->DeleteFiles(PanelItem, ItemsNumber, OpMode);
}


SHAREDSYMBOL int WINAPI _export GetFiles(HANDLE hPlugin,struct PluginPanelItem *PanelItem,
                   int ItemsNumber,int Move,char *DestPath,int OpMode)
{
	return ((PluginImpl *)hPlugin)->GetFiles(PanelItem, ItemsNumber, Move, DestPath, OpMode);
}


SHAREDSYMBOL int WINAPI _export PutFiles(HANDLE hPlugin,struct PluginPanelItem *PanelItem,
                   int ItemsNumber,int Move,int OpMode)
{
	return ((PluginImpl *)hPlugin)->PutFiles(PanelItem, ItemsNumber, Move, OpMode);
}

SHAREDSYMBOL int WINAPI _export MakeDirectory(HANDLE hPlugin, const char *Name, int OpMode)
{
	return ((PluginImpl *)hPlugin)->MakeDirectory(Name, OpMode);
}

SHAREDSYMBOL void WINAPI _export ExitFAR()
{
}


SHAREDSYMBOL void WINAPI _export GetPluginInfo(struct PluginInfo *Info)
{
	Info->StructSize = sizeof(*Info);

	Info->Flags = 0;//PF_FULLCMDLINE;
	static const char *PluginCfgStrings[1];
	PluginCfgStrings[0] = (char*)G.GetMsg(MTitle);
	Info->PluginConfigStrings = PluginCfgStrings;
	Info->PluginConfigStringsNumber = ARRAYSIZE(PluginCfgStrings);

	static char s_command_prefix[G.MAX_COMMAND_PREFIX + 1] = {}; // WHY?
	strncpy(s_command_prefix, G.command_prefix.c_str(), sizeof(s_command_prefix));
	Info->CommandPrefix = s_command_prefix;


//	static LPCSTR PluginMenuStrings[1];
//	static LPCSTR PluginCfgStrings[1];
//	static char     MenuString[MAX_PATH];
//	static char     CfgString[MAX_PATH];
//	snprintf(MenuString,     ARRAYSIZE(MenuString),     "%s", "MNetRocksMenu");//FP_GetMsg(MFtpMenu));
//	snprintf(DiskStrings[0], ARRAYSIZE(DiskStrings[0]), "%s", "MNetRocksDiskMenu");//(MFtpDiskMenu));
//	snprintf(CfgString,      ARRAYSIZE(CfgString),      "%s", "MNetRocksMenu");//FP_GetMsg(MFtpMenu));
	static char *DiskMenuStrings[2] = {"NetRocks\0", 0};
	Info->DiskMenuStrings           = DiskMenuStrings;
//	Info->DiskMenuNumbers           = 0;
	Info->DiskMenuStringsNumber     = 1;
//	Info->PluginMenuStrings         = PluginMenuStrings;
//	Info->PluginMenuStringsNumber   = Opt.AddToPluginsMenu ? ARRAYSIZE(PluginMenuStrings):0;
//	Info->PluginConfigStrings       = PluginCfgStrings;
//	Info->PluginConfigStringsNumber = ARRAYSIZE(PluginCfgStrings);
//	Info->CommandPrefix             = FTP_CMDPREFIX;
}


SHAREDSYMBOL void WINAPI _export GetOpenPluginInfo(HANDLE hPlugin,struct OpenPluginInfo *Info)
{
	((PluginImpl *)hPlugin)->GetOpenPluginInfo(Info);
}

SHAREDSYMBOL int WINAPI _export ProcessKey(HANDLE hPlugin,int Key,unsigned int ControlState)
{
	return ((PluginImpl *)hPlugin)->ProcessKey(Key, ControlState);
}

SHAREDSYMBOL int WINAPI _export Configure(int ItemNumber)
{
	if (!G.IsStarted())
		return 0;

	struct FarDialogItem fdi[] = {
            {DI_DOUBLEBOX,  3,  1,  70, 5,  0,{},0,0, {}},
            {DI_TEXT,      -1,  2,  0,  2,  0,{},0,0, {}},
            {DI_BUTTON,     34, 4,  0,  4,  0,{},0,0, {}}
	};

	strncpy(fdi[0].Data, G.GetMsg(MTitle), sizeof(fdi[0].Data));
	strncpy(fdi[1].Data, G.GetMsg(MDescription), sizeof(fdi[1].Data));
	strncpy(fdi[2].Data, G.GetMsg(MOK), sizeof(fdi[2].Data));

	G.info.Dialog(G.info.ModuleNumber, -1, -1, 74, 7, NULL, fdi, ARRAYSIZE(fdi));
	return 1;
}
