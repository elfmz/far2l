/*
TMPPANEL.CPP

Temporary panel main plugin code

*/

#include "TmpPanel.hpp"
#include <string>
#include <sys/stat.h>
#include <sys/mman.h>
#include "utils.h"

wchar_t *PluginRootKey;
unsigned int CurrentCommonPanel;
struct PluginStartupInfo Info;
struct FarStandardFunctions FSF;

PluginPanels CommonPanels[COMMONPANELSNUMBER];

#if defined(__GNUC__)

#ifdef __cplusplus
extern "C" {
#endif
BOOL WINAPI DllMainCRTStartup(HANDLE hDll, DWORD dwReason, LPVOID lpReserved);
#ifdef __cplusplus
};
#endif

BOOL WINAPI DllMainCRTStartup(HANDLE hDll, DWORD dwReason, LPVOID lpReserved)
{
	(void)lpReserved;
	(void)dwReason;
	(void)hDll;
	return TRUE;
}
#endif

static void ProcessList(HANDLE hPlugin, wchar_t *Name, int Mode);
static void ShowMenuFromList(wchar_t *Name);
static HANDLE OpenPanelFromOutput(wchar_t *argv);

static wchar_t TmpPanelPath[] = WGOOD_SLASH L"TmpPanel";
static wchar_t *TmpPanelModule = nullptr;

const wchar_t *GetTmpPanelModule()
{
	return TmpPanelModule;
}

SHAREDSYMBOL void WINAPI EXP_NAME(SetStartupInfo)(const struct PluginStartupInfo *Info)
{
	::Info = *Info;
	::FSF = *Info->FSF;
	::Info.FSF = &::FSF;
	TmpPanelModule = wcsdup(Info->ModuleName);

	PluginRootKey = (wchar_t *)malloc((wcslen(Info->RootKey) + 1) * sizeof(wchar_t) + sizeof(TmpPanelPath));
	wcscpy(PluginRootKey, Info->RootKey);
	wcscat(PluginRootKey, TmpPanelPath);
	GetOptions();
	StartupOptFullScreenPanel = Opt.FullScreenPanel;
	StartupOptCommonPanel = Opt.CommonPanel;
	CurrentCommonPanel = 0;
	memset(CommonPanels, 0, sizeof(CommonPanels));
	CommonPanels[0].Items = (PluginPanelItem *)calloc(1, sizeof(PluginPanelItem));
	Opt.LastSearchResultsPanel = 0;
}

SHAREDSYMBOL HANDLE WINAPI EXP_NAME(OpenPlugin)(int OpenFrom, INT_PTR Item)
{
	HANDLE hPlugin = INVALID_HANDLE_VALUE;

	GetOptions();

	StartupOpenFrom = OpenFrom;
	if (OpenFrom == OPEN_COMMANDLINE) {
		wchar_t *argv = (wchar_t *)Item;

		constexpr size_t OPT_COUNT = 5;

		static const wchar_t ParamsStr[OPT_COUNT][8] = {L"safe", L"any", L"replace", L"menu", L"full"};
		const int *ParamsOpt[OPT_COUNT] = {&Opt.SafeModePanel, &Opt.AnyInPanel, &Opt.Mode, &Opt.MenuForFilelist, &Opt.FullScreenPanel};

		while (*argv == L' ')
			argv++;

		while (wcslen(argv) > 1 && (*argv == L'+' || *argv == L'-')) {
			size_t k = 0;
			while (*argv && *argv != L' ' && *argv != L'<') {
				k++;
				argv++;
			}

			std::wstring tmp(argv - k, k);

			for (size_t i = 0; i < OPT_COUNT; i++) {
				if (wcscasecmp(tmp.c_str() + 1, ParamsStr[i]) == 0) {
					*(int *)ParamsOpt[i] = (tmp[0] == L'+');
					break;
				}
			}

			if (tmp[1] >= L'0' && tmp[1] <= L'9')
				CurrentCommonPanel = tmp[1] - L'0';

			while (*argv == L' ')
				argv++;
		}

		FSF.Trim(argv);
		if (wcslen(argv)) {
			if (*argv == L'<') {
				argv++;
				hPlugin = OpenPanelFromOutput(argv);
				if (Opt.MenuForFilelist)
					return INVALID_HANDLE_VALUE;
			} else {
				FSF.Unquote(argv);
				StrBuf TmpIn;
				ExpandEnvStrs(argv, TmpIn);
				StrBuf TmpOut;
				if (FindListFile(TmpIn, TmpOut)) {
					if (Opt.MenuForFilelist) {
						ShowMenuFromList(TmpOut);
						return INVALID_HANDLE_VALUE;
					} else {
						hPlugin = new TmpPanel(TmpOut);
						if (hPlugin == NULL)
							return INVALID_HANDLE_VALUE;

						ProcessList(hPlugin, TmpOut, Opt.Mode);
					}
				} else {
					return INVALID_HANDLE_VALUE;
				}
			}
		}
	}

	if (hPlugin == INVALID_HANDLE_VALUE) {
		hPlugin = new TmpPanel();
		if (hPlugin == NULL)
			return INVALID_HANDLE_VALUE;
	}

	return hPlugin;
}

static HANDLE OpenPanelFromOutput(wchar_t *argv)
{
	StrBuf tempfilename(MAX_PATH);
	FSF.MkTemp(tempfilename, tempfilename.Size(), L"FAR");

	std::wstring fullcmd = L"echo Waiting command to complete...; "
						   L"echo You can use Ctrl+C to stop it, or Ctrl+Alt+C - to hardly terminate.; ";
	fullcmd +=  argv;
	fullcmd += L" >";
	fullcmd += tempfilename;

	DWORD flags = EF_NOCMDPRINT;

	HANDLE hPlugin = INVALID_HANDLE_VALUE;

	FSF.Execute(fullcmd.c_str(), flags);
	if (Opt.MenuForFilelist) {
		ShowMenuFromList(tempfilename);
	} else {
		hPlugin = new TmpPanel();
		if (hPlugin == NULL)
			return INVALID_HANDLE_VALUE;
		ProcessList(hPlugin, tempfilename, Opt.Mode);
	}

	DeleteFile(tempfilename);
	return hPlugin;
}

void ReadFileLines(int fd, DWORD FileSizeLow, wchar_t **argv, wchar_t *args, UINT *numargs,
		UINT *numchars)
{
	*numchars = 0;
	*numargs = 0;

	char *FileData = (char *)mmap(NULL, FileSizeLow, PROT_READ, MAP_PRIVATE, fd, 0);

	if (FileData == (char *)MAP_FAILED)
		return;

	StrBuf TMP(NT_MAX_PATH);	// BUGBUG
	DWORD Len, Pos = 0, Size = FileSizeLow;

	if (Size >= 3) {
		if (((LPBYTE)FileData)[0] == 0xEF && ((LPBYTE)FileData)[1] == 0xBB && ((LPBYTE)FileData)[2] == 0xBF) {
			Pos+= 3;
		}
	}

	while (Pos < Size) {

		char c;
		while (Pos < Size && ((c = FileData[Pos]) == '\r' || c == '\n'))
			Pos++;
		DWORD Off = Pos;
		while (Pos < Size && (c = FileData[Pos]) != '\r' && c != '\n')
			Pos++;
		Len = Pos - Off;

	    Len = MultiByteToWideChar(CP_UTF8, MB_PRECOMPOSED | MB_ERR_INVALID_CHARS, &FileData[Off], Len, TMP,
			TMP.Size() - 1);

		if (!Len)
			continue;

		TMP.Ptr()[Len] = 0;

		FSF.Trim(TMP);
		FSF.Unquote(TMP);

		Len = wcslen(TMP);
		if (!Len)
			continue;

		if (argv)
			*argv++ = args;

		if (args) {
			wcscpy(args, TMP);
			args+= Len + 1;
		}

		(*numchars)+= Len + 1;
		++*numargs;
	}

}

	static void ReadFileList(wchar_t * filename, int *argc, wchar_t ***argv)
	{
		*argc = 0;
		*argv = NULL;

		StrBuf FullPath;
		GetFullPath(filename, FullPath);

		HANDLE hFile = CreateFile(FullPath, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);

		if (hFile != INVALID_HANDLE_VALUE) {
			DWORD FileSizeLow = GetFileSize(hFile, NULL);
			int fd = GetFileDescriptor(hFile);
			if (fd != -1 && FileSizeLow != INVALID_FILE_SIZE) {
				UINT i;
				ReadFileLines(fd, FileSizeLow, NULL, NULL, (UINT *)argc, &i);
				*argv = (wchar_t **)malloc(*argc * sizeof(wchar_t *) + i * sizeof(wchar_t));
				ReadFileLines(fd, FileSizeLow, *argv, (wchar_t *)&(*argv)[*argc], (UINT *)argc,
						&i);
			}
			CloseHandle(hFile);
		}
	}

	static void ProcessList(HANDLE hPlugin, wchar_t * Name, int Mode)
	{
		if (Mode) {
			FreePanelItems(CommonPanels[CurrentCommonPanel].Items,
					CommonPanels[CurrentCommonPanel].ItemsNumber);
			CommonPanels[CurrentCommonPanel].Items = (PluginPanelItem *)calloc(1, sizeof(PluginPanelItem));
			CommonPanels[CurrentCommonPanel].ItemsNumber = 0;
		}
		TmpPanel *Panel = (TmpPanel *)hPlugin;

		int argc;
		wchar_t **argv;
		ReadFileList(Name, &argc, &argv);

		HANDLE hScreen = Panel->BeginPutFiles();

		for (UINT i = 0; (int)i < argc; i++)
			Panel->PutOneFile(argv[i]);

		Panel->CommitPutFiles(hScreen, TRUE);
		if (argv)
			free(argv);
	}

	void ShowMenuFromList(wchar_t * Name)
	{
		int argc;
		wchar_t **argv = 0;

		ReadFileList(Name, &argc, &argv);

		FarMenuItem *fmi = (FarMenuItem *)malloc(argc * sizeof(FarMenuItem));
		if (fmi) {
			StrBuf TMP(NT_MAX_PATH);	// BUGBUG
			for (int i = 0; i < argc; ++i) {
				wchar_t *param, *p = TMP;
				ExpandEnvStrs(argv[i], TMP);
				param =  ParseParam(p);
				if (!param) {
					param = p;
				}

				FSF.TruncStr(param, 67);
				fmi[i].Text = wcsdup(param);
				fmi[i].Separator = !wcscmp(param, L"-");
				fmi[i].Selected = FALSE;
				fmi[i].Checked = FALSE;

			}
			//    fmi[0].Selected=TRUE;

			wchar_t Title[128];	// BUGBUG
			FSF.ProcessName(FSF.PointToName(Name), wcscpy(Title, L"*."),
					ARRAYSIZE(Title),PN_GENERATENAME);
			FSF.TruncPathStr(Title, 64);

			int BreakCode;
			static const int BreakKeys[2] = {MAKELONG(VK_RETURN, PKF_SHIFT), 0};

			int ExitCode = Info.Menu(Info.ModuleNumber, -1, -1, 0, FMENU_WRAPMODE, Title, L"Enter Shift+Enter Esc Ctrl+Alt+F",
					L"Contents", &BreakKeys[0], &BreakCode, fmi, argc);

			for (int i = 0; i < argc; ++i)
				if (fmi[i].Text)
					free((void *)fmi[i].Text);

			free(fmi);

			if ((unsigned)ExitCode < (unsigned)argc) {
				wchar_t *p = argv[ExitCode];
				ParseParam(p);
				ExpandEnvStrs(p, TMP);
				p = TMP;

				bool bShellExecute = BreakCode != -1;

				enum { ACTION_NOP, ACTION_SETPANELDIR, ACTION_SETCMDLINE, ACTION_OPEN,
					   ACTION_EXECUTE} Action = ACTION_NOP;

				FAR_FIND_DATA FindData = {};

				if (TmpPanel::GetFileInfoAndValidate(p, &FindData, FALSE)) {
					if (FindData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
						Action = bShellExecute ? ACTION_OPEN : ACTION_SETPANELDIR;
					} else if (FindData.dwFileAttributes & FILE_ATTRIBUTE_EXECUTABLE) {
						Action = ACTION_EXECUTE;
					} else {
						Action = ACTION_OPEN;
					}
				} else {
					Action = bShellExecute ? ACTION_OPEN : ACTION_SETCMDLINE;
				}

				if (FindData.lpwszFileName) {
					free((void *)FindData.lpwszFileName);
				}

				switch (Action) {
					case ACTION_SETPANELDIR: {
						Info.Control(INVALID_HANDLE_VALUE, FCTL_SETPANELDIR, 0, (LONG_PTR)p);
						break;
					}

					case ACTION_SETCMDLINE: {
						Info.Control(PANEL_ACTIVE, FCTL_SETCMDLINE, 0, (LONG_PTR)p);
						break;
					}

					case ACTION_OPEN: {
						std::wstring cmd = p;
						QuoteCmdArgIfNeed(cmd);
						FSF.Execute(cmd.c_str(), EF_OPEN | EF_NOCMDPRINT | EF_NOWAIT);
						break;
					}

					case ACTION_EXECUTE: {
						std::wstring cmd = p;
						QuoteCmdArgIfNeed(cmd);
						FSF.Execute(cmd.c_str(), bShellExecute ? EF_EXTERNALTERM | EF_NOCMDPRINT | EF_HIDEOUT: EF_MAYBGND);
						break;
					}

					default:
						;
				}
			}
		}

		if (argv)
			free(argv);
	}

	SHAREDSYMBOL HANDLE WINAPI
	EXP_NAME(OpenFilePlugin)(const wchar_t * Name, const unsigned char *, int DataSize, int OpMode)
	{
		if (!Name)
			return INVALID_HANDLE_VALUE;
		GetOptions();
		StrBuf pName(NT_MAX_PATH);	// BUGBUG
		wcscpy(pName, Name);

		if (!DataSize || !FSF.ProcessName(Opt.Mask, pName, pName.Size(), PN_CMPNAMELIST))
			return INVALID_HANDLE_VALUE;

		if (!Opt.MenuForFilelist) {
			HANDLE hPlugin = new TmpPanel(pName);
			if (hPlugin == NULL)
				return INVALID_HANDLE_VALUE;

			ProcessList(hPlugin, pName, Opt.Mode);
			return hPlugin;
		} else {
			ShowMenuFromList(pName);
			return ((HANDLE)-2);
		}
	}

	SHAREDSYMBOL void WINAPI EXP_NAME(ClosePlugin)(HANDLE hPlugin)
	{
		delete (TmpPanel *)hPlugin;
	}

	SHAREDSYMBOL void WINAPI EXP_NAME(ExitFAR)()
	{
		free(TmpPanelModule);
		free(PluginRootKey);
		for (int i = 0; i < COMMONPANELSNUMBER; ++i)
			FreePanelItems(CommonPanels[i].Items, CommonPanels[i].ItemsNumber);
	}

	SHAREDSYMBOL int WINAPI
	EXP_NAME(GetFindData)(HANDLE hPlugin, struct PluginPanelItem * *pPanelItem, int *pItemsNumber, int OpMode)
	{
		TmpPanel *Panel = (TmpPanel *)hPlugin;
		return Panel->GetFindData(pPanelItem, pItemsNumber, OpMode);
	}

	SHAREDSYMBOL void WINAPI EXP_NAME(GetPluginInfo)(struct PluginInfo * Info)
	{
		Info->StructSize = sizeof(*Info);
		Info->Flags = 0;
		static const wchar_t *DiskMenuStrings[1];
		DiskMenuStrings[0] = GetMsg(MDiskMenuString);
		Info->DiskMenuStrings = DiskMenuStrings;

		Info->DiskMenuStringsNumber = Opt.AddToDisksMenu ? ARRAYSIZE(DiskMenuStrings) : 0;
		static const wchar_t *PluginMenuStrings[1];
		PluginMenuStrings[0] = GetMsg(MTempPanel);
		Info->PluginMenuStrings = Opt.AddToPluginsMenu ? PluginMenuStrings : NULL;
		Info->PluginMenuStringsNumber = Opt.AddToPluginsMenu ? ARRAYSIZE(PluginMenuStrings) : 0;
		static const wchar_t *PluginCfgStrings[1];
		PluginCfgStrings[0] = GetMsg(MTempPanel);
		Info->PluginConfigStrings = PluginCfgStrings;
		Info->PluginConfigStringsNumber = ARRAYSIZE(PluginCfgStrings);
		Info->CommandPrefix = Opt.Prefix;
	}

	SHAREDSYMBOL void WINAPI EXP_NAME(GetOpenPluginInfo)(HANDLE hPlugin, struct OpenPluginInfo * Info)
	{
		TmpPanel *Panel = (TmpPanel *)hPlugin;
		Panel->GetOpenPluginInfo(Info);
	}

	SHAREDSYMBOL int WINAPI EXP_NAME(SetDirectory)(HANDLE hPlugin, const wchar_t *Dir, int OpMode)
	{
		TmpPanel *Panel = (TmpPanel *)hPlugin;
		return (Panel->SetDirectory(Dir, OpMode));
	}

	SHAREDSYMBOL int WINAPI
	EXP_NAME(PutFiles)(HANDLE hPlugin, struct PluginPanelItem * PanelItem, int ItemsNumber, int Move,
			const wchar_t *SrcPath,	int OpMode)
	{

		TmpPanel *Panel = (TmpPanel *)hPlugin;
		return Panel->PutFiles(PanelItem, ItemsNumber, Move, SrcPath, OpMode);
	}

	SHAREDSYMBOL int WINAPI
	EXP_NAME(SetFindList)(HANDLE hPlugin, const struct PluginPanelItem *PanelItem, int ItemsNumber)
	{
		TmpPanel *Panel = (TmpPanel *)hPlugin;
		return (Panel->SetFindList(PanelItem, ItemsNumber));
	}

	SHAREDSYMBOL int WINAPI EXP_NAME(ProcessEvent)(HANDLE hPlugin, int Event, void *Param)
	{
		TmpPanel *Panel = (TmpPanel *)hPlugin;
		return (Panel->ProcessEvent(Event, Param));
	}

	SHAREDSYMBOL int WINAPI EXP_NAME(ProcessKey)(HANDLE hPlugin, int Key, unsigned int ControlState)
	{
		TmpPanel *Panel = (TmpPanel *)hPlugin;
		return (Panel->ProcessKey(Key, ControlState));
	}

	SHAREDSYMBOL int WINAPI EXP_NAME(Configure)(int ItemNumber)
	{
		return (!ItemNumber ? Config() : FALSE);
	}

	SHAREDSYMBOL int WINAPI EXP_NAME(GetMinFarVersion)()
	{
		return FARMANAGERVERSION;
	}
