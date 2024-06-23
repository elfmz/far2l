#include "MultiArc.hpp"
#include "marclng.hpp"
#include <errno.h>

BOOL PluginClass::GetFormatName(std::string &FormatName, std::string &DefExt)
{
	FormatName.clear();
	DefExt.clear();
	return ArcPlugin->GetFormatName(ArcPluginNumber, ArcPluginType, FormatName, DefExt);
}

BOOL PluginClass::GetFormatName(std::string &FormatName)
{
	FormatName.clear();
	std::string TempDefExt;
	return ArcPlugin->GetFormatName(ArcPluginNumber, ArcPluginType, FormatName, TempDefExt);
}


std::string PluginClass::GetCommandFormat(int Command)
{
	std::string ArcFormat;
	if (!GetFormatName(ArcFormat))
		return std::string();
	std::string CommandFormat;
	ArcPlugin->GetDefaultCommands(ArcPluginNumber, ArcPluginType, Command, CommandFormat);
	return KeyFileReadSection(INI_LOCATION, ArcFormat).GetString(CmdNames[Command], CommandFormat.c_str());
}

int PluginClass::DeleteFiles(struct PluginPanelItem *PanelItem, int ItemsNumber, int OpMode)
{
	//char Command[MA_MAX_SIZE_COMMAND_NAME], AllFilesMask[MA_MAX_SIZE_COMMAND_NAME];
	std::string Command, AllFilesMask;
	if (ItemsNumber == 0)
		return FALSE;
	if ((OpMode & OPM_SILENT) == 0) {
		const char *MsgItems[] = {GetMsg(MDeleteTitle), GetMsg(MDeleteFiles), GetMsg(MDeleteDelete),
				GetMsg(MDeleteCancel)};
		std::string Msg;
		if (ItemsNumber == 1) {
			const auto &NameMsg = FormatMessagePath(PanelItem[0].FindData.cFileName, false);
			Msg = StrPrintf(GetMsg(MDeleteFile), NameMsg.c_str());
			MsgItems[1] = Msg.c_str();
		}
		if (Info.Message(Info.ModuleNumber, 0, NULL, MsgItems, ARRAYSIZE(MsgItems), 2) != 0)
			return FALSE;

		if (ItemsNumber > 1) {
			Msg = StrPrintf(GetMsg(MDeleteNumberOfFiles), ItemsNumber);
			MsgItems[1] = Msg.c_str();
			if (Info.Message(Info.ModuleNumber, FMSG_WARNING, NULL, MsgItems, ARRAYSIZE(MsgItems), 2) != 0)
				return FALSE;
		}
	}
	Command = GetCommandFormat(CMD_DELETE);
	AllFilesMask = GetCommandFormat(CMD_ALLFILESMASK);
	int IgnoreErrors = (CurArcInfo.Flags & AF_IGNOREERRORS);
	ArcCommand ArcCmd(PanelItem, ItemsNumber, Command, ArcName, CurDir, "", AllFilesMask, IgnoreErrors, CMD_DELETE, 0,
			CurDir, ItemsInfo.Codepage);
	if (!IgnoreErrors && ArcCmd.GetExecCode() != 0)
		return FALSE;
	if (Opt.UpdateDescriptions)
		for (int I = 0; I < ItemsNumber; I++)
			PanelItem[I].Flags|= PPIF_PROCESSDESCR;
	return TRUE;
}

int PluginClass::ProcessHostFile(struct PluginPanelItem *PanelItem, int ItemsNumber, int OpMode)
{
	struct ArcCmdMenuData
	{
		int Msg, Cmd;
	};
	static const ArcCmdMenuData MenuData[] = {
			{MArcCmdTest,         CMD_TEST        },
			{MArcCmdComment,      CMD_COMMENT     },
			{MArcCmdCommentFiles, CMD_COMMENTFILES},
			{MArcCmdSFX,          CMD_SFX         },
			{MArcCmdRecover,      CMD_RECOVER     },
			{MArcCmdProtect,      CMD_PROTECT     },
			{MArcCmdLock,         CMD_LOCK        },
	};

	std::string Command, AllFilesMask;
	int CommandType;
	int ExitCode = 0;

	while (1) {
		struct FarMenuItemEx MenuItems[ARRAYSIZE(MenuData)];

		ZeroFill(MenuItems);
		MenuItems[ExitCode].Flags = MIF_SELECTED;

		int Count = 0;
		for (size_t i = 0; i < ARRAYSIZE(MenuData); i++) {
			Command = GetCommandFormat(MenuData[i].Cmd);
			if (!Command.empty()) {
				MenuItems[Count].Text.TextPtr = GetMsg(MenuData[i].Msg);
				MenuItems[Count].Flags|= MIF_USETEXTPTR;
				MenuItems[Count++].UserData = MenuData[i].Cmd;
			}
		}

		if (!Count) {
			Count = 1;
			MenuItems[0].UserData = 0xFFFFFFFF;
		}

		int BreakCode;
		int BreakKeys[2] = {VK_F4, 0};
		ExitCode = Info.Menu(Info.ModuleNumber, -1, -1, 0, FMENU_USEEXT | FMENU_WRAPMODE,
				GetMsg(MArcCmdTitle), GetMsg(MSelectF4), "ArcCmd", BreakKeys, &BreakCode,
				(FarMenuItem *)MenuItems, Count);
		if (ExitCode >= 0) {
			if (BreakCode == 0)		// F4 pressed
			{
				MenuItems[0].Flags&= ~MIF_USETEXTPTR;
				std::string FormatName;
				GetFormatName(FormatName);
				CharArrayCpyZ(MenuItems[0].Text.Text, FormatName.c_str());
				ConfigCommands(FormatName, 2 + MenuData[ExitCode].Cmd * 2);
				continue;
			}
			CommandType = (int)MenuItems[ExitCode].UserData;
			if (MenuItems[ExitCode].UserData == 0xFFFFFFFF)
				return FALSE;
		} else
			return FALSE;
		break;
	}

	WINPORT(FlushConsoleInputBuffer)(NULL);		// GetStdHandle(STD_INPUT_HANDLE));

	Command = GetCommandFormat(CommandType);
	AllFilesMask = GetCommandFormat(CMD_ALLFILESMASK);
	int IgnoreErrors = (CurArcInfo.Flags & AF_IGNOREERRORS);
	std::string Password;

	int AskVolume = (OpMode & (OPM_FIND | OPM_VIEW | OPM_EDIT | OPM_QUICKVIEW)) == 0 && CurArcInfo.Volume
			&& *CurDir == 0 && ExitCode == 0;
	struct PluginPanelItem MaskPanelItem;

	if (AskVolume) {
		const auto &NameMsg = FormatMessagePath(ArcName.c_str(), true);
		const auto &VolMsg = StrPrintf(GetMsg(MExtrVolume), NameMsg.c_str());
		const char *MsgItems[] = {"", VolMsg.c_str(), GetMsg(MExtrVolumeAsk1), GetMsg(MExtrVolumeAsk2),
				GetMsg(MExtrVolumeSelFiles), GetMsg(MExtrAllVolumes)};
		int MsgCode = Info.Message(Info.ModuleNumber, 0, NULL, MsgItems, ARRAYSIZE(MsgItems), 2);
		if (MsgCode < 0)
			return -1;
		if (MsgCode == 1) {
			ZeroFill(MaskPanelItem);
			CharArrayCpyZ(MaskPanelItem.FindData.cFileName, AllFilesMask.c_str());
			if (ItemsInfo.Encrypted)
				MaskPanelItem.Flags = F_ENCRYPTED;
			PanelItem = &MaskPanelItem;
			ItemsNumber = 1;
		}
	}

	if (Command.find("%%P") != std::string::npos)
		for (int I = 0; I < ItemsNumber; I++)
			if ((PanelItem[I].Flags & F_ENCRYPTED)
					|| (ItemsInfo.Encrypted
							&& (PanelItem[I].FindData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))) {
				if (!GetPassword(Password, FSF.PointToName((char *)ArcName.c_str())))
					return FALSE;
				break;
			}

	ArcCommand ArcCmd(PanelItem, ItemsNumber, Command, ArcName, CurDir,
		Password, AllFilesMask, IgnoreErrors, CommandType, 0, CurDir, ItemsInfo.Codepage);
	return IgnoreErrors || ArcCmd.GetExecCode() == 0;
}

int __cdecl FormatSort(struct FarMenuItemEx *Item1, struct FarMenuItemEx *Item2)
{
	return strcasecmp(Item1->Text.Text, Item2->Text.Text);
}

bool PluginClass::SelectFormat(std::string &ArcFormat, int AddOnly)
{
	typedef int(__cdecl * FCmp)(const void *, const void *);
	struct FarMenuItemEx *MenuItems = NULL, *NewMenuItems;
	int MenuItemsNumber = 0;
	std::string Format, DefExt;
	int BreakCode;
	int BreakKeys[] = {VK_F4, VK_RETURN, 0};
	int ExitCode;

	BreakKeys[1] = (AddOnly) ? 0 : VK_RETURN;

	while (1) {
		for (int i = 0; i < ArcPlugin->FmtCount(); i++) {
			for (int j = 0;; j++) {
				if (!ArcPlugin->GetFormatName(i, j, Format, DefExt))
					break;

				if (AddOnly)	// Only add to archive?
				{
					std::string CmdAdd;
					ArcPlugin->GetDefaultCommands(i, j, CMD_ADD, CmdAdd);
					CmdAdd = KeyFileReadSection(INI_LOCATION, Format).GetString(CmdNames[CMD_ADD], CmdAdd.c_str());
					if (CmdAdd.empty())
						continue;
				}

				NewMenuItems = (struct FarMenuItemEx *)realloc(MenuItems,
						(MenuItemsNumber + 1) * sizeof(struct FarMenuItemEx));
				if (NewMenuItems == NULL) {
					free(MenuItems);
					return false;
				}
				MenuItems = NewMenuItems;
				ZeroFill(MenuItems[MenuItemsNumber]);
				MenuItems[MenuItemsNumber].UserData = MAKEWPARAM((WORD)i, (WORD)j);
				CharArrayCpyZ(MenuItems[MenuItemsNumber].Text.Text, Format.c_str());
				MenuItems[MenuItemsNumber].Flags =
					((MenuItemsNumber == 0 && ArcFormat.empty()) || !strcasecmp(ArcFormat.c_str(), Format.c_str()))
						? MIF_SELECTED : 0;
				MenuItemsNumber++;
			}
		}
		if (MenuItemsNumber == 0)
			return false;

		FSF.qsort(MenuItems, MenuItemsNumber, sizeof(struct FarMenuItemEx), (FCmp)FormatSort);

		DWORD Flags = FMENU_AUTOHIGHLIGHT | FMENU_USEEXT;
		if (!Opt.AdvFlags.MenuWrapMode)
			Flags|= FMENU_WRAPMODE;
		else if (Opt.AdvFlags.MenuWrapMode == 2) {
			CONSOLE_SCREEN_BUFFER_INFO csbi;
			WINPORT(GetConsoleScreenBufferInfo)(NULL, &csbi);	// GetStdHandle(STD_OUTPUT_HANDLE)
			if (csbi.dwSize.Y - 6 >= MenuItemsNumber)
				Flags|= FMENU_WRAPMODE;
		}
		ExitCode = Info.Menu(Info.ModuleNumber, -1, -1, 0, Flags, GetMsg(MSelectArchiver), GetMsg(MSelectF4),
				NULL, BreakKeys, &BreakCode, (struct FarMenuItem *)MenuItems, MenuItemsNumber);
		if (ExitCode >= 0) {
			CharArrayAssignToStr(ArcFormat, MenuItems[ExitCode].Text.Text);
			if ((BreakCode >= 0 && BreakCode <= 1) || !AddOnly)		// F4 or Enter pressed
				ConfigCommands(ArcFormat, 2, TRUE,
					LOWORD(MenuItems[ExitCode].UserData), HIWORD(MenuItems[ExitCode].UserData));
			else
				break;
		} else
			break;
		free(MenuItems);
		MenuItems = NULL;
		MenuItemsNumber = 0;
	}
	if (MenuItems)
		free(MenuItems);
	return ExitCode >= 0;
}

bool PluginClass::FormatToPlugin(const std::string &Format, int &PluginNumber, int &PluginType)
{
	std::string PluginFormat, DefExt;
	for (int i = 0; i < ArcPlugin->FmtCount(); i++) {
		for (int j = 0;; j++) {
			if (!ArcPlugin->GetFormatName(i, j, PluginFormat, DefExt))
				break;
			if (!strcasecmp(PluginFormat.c_str(), Format.c_str())) {
				PluginNumber = i;
				PluginType = j;
				return true;
			}
		}
	}
	return false;
}

SHAREDSYMBOL int WINAPI _export Configure(int ItemNumber);

int PluginClass::ProcessKey(int Key, unsigned int ControlState)
{
	if ((ControlState & PKF_ALT) && Key == VK_F6) {
		//    HANDLE hScreen=Info.SaveScreen(0,0,-1,-1);
		if (strstr(ArcName.c_str(), /*"FarTmp"*/ "FTMP") == NULL)		//$AA какая-то бяка баловалась
		{
			std::string CurDir = ArcName;
			const size_t Slash = CurDir.rfind(GOOD_SLASH);
			if (Slash != std::string::npos) {
				CurDir.resize(Slash);
				if (sdc_chdir(CurDir.c_str()))
					fprintf(stderr, "sdc_chdir('%s') - %u\n", CurDir.c_str(), errno);
			}
		}
		struct PanelInfo PInfo;
		Info.Control(this, FCTL_GETPANELINFO, &PInfo);
		GetFiles(PInfo.SelectedItems, PInfo.SelectedItemsNumber, FALSE, PInfo.CurDir, OPM_SILENT);
		//    Info.RestoreScreen(hScreen);
		Info.Control(this, FCTL_UPDATEPANEL, (void *)1);
		Info.Control(this, FCTL_REDRAWPANEL, NULL);
		Info.Control(this, FCTL_UPDATEANOTHERPANEL, (void *)1);
		Info.Control(this, FCTL_REDRAWANOTHERPANEL, NULL);
		return TRUE;
	} else if (ControlState == (PKF_ALT | PKF_SHIFT) && Key == VK_F9) {
		Configure(0);
		return TRUE;
	}
	return FALSE;
}
