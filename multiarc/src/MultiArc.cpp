#include "MultiArc.hpp"
#include "marclng.hpp"
#include <string>
#include <unistd.h>

SHAREDSYMBOL int WINAPI _export GetMinFarVersion(void)
{
#define MAKEFARVERSION(major, minor) (((major) << 16) | (minor))
	return MAKEFARVERSION(2, 1);
}

SHAREDSYMBOL void WINAPI _export SetStartupInfo(const struct PluginStartupInfo *Info)
{
	::Info = *Info;
	FSF = *Info->FSF;
	::Info.FSF = &FSF;

	::Info.FSF->Reserved[0] = (DWORD_PTR)malloc;
	::Info.FSF->Reserved[1] = (DWORD_PTR)free;

	if (ArcPlugin == NULL)
		ArcPlugin = new ArcPlugins(Info->ModuleName);

	KeyFileReadSection kfh(INI_LOCATION, INI_SECTION);
	Opt.HideOutput = kfh.GetInt("HideOutput", 0);
	Opt.ProcessShiftF1 = kfh.GetInt("ProcessShiftF1", 1);
	Opt.UseLastHistory = kfh.GetInt("UseLastHistory", 0);

	// Opt.DeleteExtFile=GetRegKey(HKEY_CURRENT_USER,"","DeleteExtFile",1);
	// Opt.AddExtArchive=GetRegKey(HKEY_CURRENT_USER,"","AddExtArchive",0);

	// Opt.AutoResetExactArcName=GetRegKey(HKEY_CURRENT_USER,"","AutoResetExactArcName",1);
	// Opt.ExactArcName=GetRegKey(HKEY_CURRENT_USER, "", "ExactArcName", 0);
	Opt.AdvFlags = kfh.GetInt("AdvFlags", 2);

	Opt.DescriptionNames = kfh.GetString("DescriptionNames", "descript.ion,files.bbs");
	Opt.ReadDescriptions = kfh.GetInt("ReadDescriptions", 0);
	Opt.UpdateDescriptions = kfh.GetInt("UpdateDescriptions", 0);
	// Opt.UserBackground=GetRegKey(HKEY_CURRENT_USER,"","Background",0); // $ 06.02.2002 AA
	Opt.OldUserBackground = 0;	// $ 02.07.2002 AY
	Opt.AllowChangeDir = kfh.GetInt("AllowChangeDir", 0);

	kfh.GetChars(Opt.CommandPrefix1, sizeof(Opt.CommandPrefix1), "Prefix1", "ma");

	Opt.PriorityClass = 2;									// default: NORMAL
}

static const char *KnownDocumentTypes[] = {"docx", "docm", "dotx", "dotm",

		"xlsx", "xlsm", "xltx", "xltm", "xlsb", "xlam", "pptx", "pptm", "potx", "potm", "ppam", "ppsx",

		"ppsm", "sldx", "sldm", "thmx",

		"odt", "ods", "odp"};

static bool IsKnownDocumentType(const char *Name)
{
	const char *ext = strrchr(Name, '.');
	if (ext && ext[1]) {
		++ext;
		for (const char *known_type : KnownDocumentTypes) {
			if (strcasecmp(ext, known_type) == 0) {
				return true;
			}
		}
	}
	return false;
}

SHAREDSYMBOL HANDLE WINAPI _export
OpenFilePlugin(const char *Name, const unsigned char *Data, int DataSize, int OpMode)
{
	if (ArcPlugin == NULL)
		return INVALID_HANDLE_VALUE;

	// if its a docx&etc then Enter should open document by default instead of sinking into it
	// as archive (even while its really archive)
	if (OpMode == 0 && Name && IsKnownDocumentType(Name))
		return INVALID_HANDLE_VALUE;

	int ArcPluginNumber = -1;
	if (Name == NULL) {
		if (!Opt.ProcessShiftF1)
			return INVALID_HANDLE_VALUE;
	} else {
		ArcPluginNumber = ArcPlugin->IsArchive(Name, Data, DataSize);
		if (ArcPluginNumber == -1)
			return INVALID_HANDLE_VALUE;
	}

	try {
		PluginClass *pPlugin = new PluginClass(ArcPluginNumber);
		if (Name != NULL && !pPlugin->PreReadArchive(Name)) {
			delete pPlugin;
			return INVALID_HANDLE_VALUE;
		}
		return (HANDLE)pPlugin;

	} catch (std::exception &e) {
		fprintf(stderr, "OpenFilePlugin: %s\n", e.what());
		return INVALID_HANDLE_VALUE;
	}
}

std::string MakeFullName(const char *name)
{
	if (name[0] == '/')
		return name;

	std::string out;
	char cd[PATH_MAX];
	if (getcwd(cd, sizeof(cd))) {
		if (name[0] == '.' && name[1] == '/') {
			name+= 2;
		}
		out = cd;
		out+= '/';
	}

	out+= name;
	return out;
}

static HANDLE OpenPluginWithCmdLine(const char *CmdLine)
{
	const size_t Prefix1Len = strnlen(Opt.CommandPrefix1, ARRAYSIZE(Opt.CommandPrefix1));
	if (strncmp(CmdLine, Opt.CommandPrefix1, Prefix1Len) != 0)
		return INVALID_HANDLE_VALUE;

	if (CmdLine[Prefix1Len] != ':' || CmdLine[Prefix1Len + 1] == 0)
		return INVALID_HANDLE_VALUE;

	std::string filename = MakeFullName(&CmdLine[Prefix1Len + 1]);
	if (filename.empty())
		return INVALID_HANDLE_VALUE;

	HANDLE h = WINPORT(CreateFile)(StrMB2Wide(filename).c_str(), GENERIC_READ, FILE_SHARE_READ, NULL,
			OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if (h == INVALID_HANDLE_VALUE)
		return INVALID_HANDLE_VALUE;

	DWORD datasize = (DWORD)Info.AdvControl(Info.ModuleNumber, ACTL_GETPLUGINMAXREADDATA, (void *)0);
	if (!datasize)
		return INVALID_HANDLE_VALUE;

	unsigned char *data = (unsigned char *)malloc(datasize);
	if (!data)
		return INVALID_HANDLE_VALUE;

	if (!WINPORT(ReadFile)(h, data, datasize, &datasize, NULL))
		datasize = 0;

	WINPORT(CloseHandle)(h);

	h = (datasize == 0)
			? INVALID_HANDLE_VALUE
			: OpenFilePlugin(filename.c_str(), data, datasize, OPM_COMMANDS);

	free(data);

	return h;
}

SHAREDSYMBOL HANDLE WINAPI _export OpenPlugin(int OpenFrom, INT_PTR Item)
{
	if (ArcPlugin == NULL)
		return INVALID_HANDLE_VALUE;

	if (OpenFrom == OPEN_COMMANDLINE)
		return OpenPluginWithCmdLine((const char *)Item);

	return INVALID_HANDLE_VALUE;
}

SHAREDSYMBOL void WINAPI _export ClosePlugin(HANDLE hPlugin)
{
	delete (PluginClass *)hPlugin;
}

SHAREDSYMBOL int WINAPI _export
GetFindData(HANDLE hPlugin, struct PluginPanelItem **pPanelItem, int *pItemsNumber, int OpMode)
{
	PluginClass *Plugin = (PluginClass *)hPlugin;
	return Plugin->GetFindData(pPanelItem, pItemsNumber, OpMode);
}

SHAREDSYMBOL void WINAPI _export
FreeFindData(HANDLE hPlugin, struct PluginPanelItem *PanelItem, int ItemsNumber)
{
	PluginClass *Plugin = (PluginClass *)hPlugin;
	Plugin->FreeFindData(PanelItem, ItemsNumber);
}

SHAREDSYMBOL int WINAPI _export SetDirectory(HANDLE hPlugin, const char *Dir, int OpMode)
{
	PluginClass *Plugin = (PluginClass *)hPlugin;
	return Plugin->SetDirectory(Dir, OpMode);
}

SHAREDSYMBOL int WINAPI _export
DeleteFiles(HANDLE hPlugin, struct PluginPanelItem *PanelItem, int ItemsNumber, int OpMode)
{
	PluginClass *Plugin = (PluginClass *)hPlugin;
	return Plugin->DeleteFiles(PanelItem, ItemsNumber, OpMode);
}

SHAREDSYMBOL int WINAPI _export GetFiles(HANDLE hPlugin, struct PluginPanelItem *PanelItem, int ItemsNumber,
		int Move, char *DestPath, int OpMode)
{
	PluginClass *Plugin = (PluginClass *)hPlugin;
	return Plugin->GetFiles(PanelItem, ItemsNumber, Move, DestPath, OpMode);
}

SHAREDSYMBOL int WINAPI _export
PutFiles(HANDLE hPlugin, struct PluginPanelItem *PanelItem, int ItemsNumber, int Move, int OpMode)
{
	PluginClass *Plugin = (PluginClass *)hPlugin;
	return Plugin->PutFiles(PanelItem, ItemsNumber, Move, OpMode);
}

SHAREDSYMBOL void WINAPI _export ExitFAR()
{
	delete ArcPlugin;
	ArcPlugin = NULL;
}

SHAREDSYMBOL void WINAPI _export GetPluginInfo(struct PluginInfo *Info)
{
	Info->StructSize = sizeof(*Info);
	Info->Flags = PF_FULLCMDLINE;
	static const char *PluginCfgStrings[1];
	PluginCfgStrings[0] = (char *)GetMsg(MCfgLine0);
	Info->PluginConfigStrings = PluginCfgStrings;
	Info->PluginConfigStringsNumber = ARRAYSIZE(PluginCfgStrings);
	static char CommandPrefix[sizeof(Opt.CommandPrefix1)];
	CharArrayCpyZ(CommandPrefix, Opt.CommandPrefix1);
	Info->CommandPrefix = CommandPrefix;
}

SHAREDSYMBOL void WINAPI _export GetOpenPluginInfo(HANDLE hPlugin, struct OpenPluginInfo *Info)
{
	PluginClass *Plugin = (PluginClass *)hPlugin;
	Plugin->GetOpenPluginInfo(Info);
}

SHAREDSYMBOL int WINAPI _export
ProcessHostFile(HANDLE hPlugin, struct PluginPanelItem *PanelItem, int ItemsNumber, int OpMode)
{
	PluginClass *Plugin = (PluginClass *)hPlugin;
	return Plugin->ProcessHostFile(PanelItem, ItemsNumber, OpMode);
}

SHAREDSYMBOL int WINAPI _export ProcessKey(HANDLE hPlugin, int Key, unsigned int ControlState)
{
	PluginClass *Plugin = (PluginClass *)hPlugin;
	return Plugin->ProcessKey(Key, ControlState);
}

SHAREDSYMBOL int WINAPI _export Configure(int ItemNumber)
{
	struct FarMenuItem MenuItems[2];
	ZeroFill(MenuItems);
	CharArrayCpyZ(MenuItems[0].Text, GetMsg(MCfgLine1));
	CharArrayCpyZ(MenuItems[1].Text, GetMsg(MCfgLine2));
	MenuItems[0].Selected = TRUE;

	do {
		ItemNumber = Info.Menu(Info.ModuleNumber, -1, -1, 0, FMENU_WRAPMODE, GetMsg(MCfgLine0), NULL,
				"Config", NULL, NULL, MenuItems, ARRAYSIZE(MenuItems));
		switch (ItemNumber) {
			case -1:
				return FALSE;
			case 0:
				ConfigGeneral();
				break;
			case 1: {
				std::string ArcFormat;
				while (PluginClass::SelectFormat(ArcFormat))
					;	// ConfigCommands(ArcFormat);
				break;
			}
		}
		MenuItems[0].Selected = FALSE;
		MenuItems[1].Selected = FALSE;
		MenuItems[ItemNumber].Selected = TRUE;
	} while (1);

	// return FALSE;
}

std::string gMultiArcPluginPath;

SHAREDSYMBOL void PluginModuleOpen(const char *path)
{
	gMultiArcPluginPath = path;
}
