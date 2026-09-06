#include "MultiArc.hpp"
#include "marclng.hpp"
#include <farkeys.h>

bool ConfigGeneral()
{
	struct InitDialogItem InitItems[] = {
			/*  0 */ {DI_DOUBLEBOX, 3, 1, 72, 17, 0, 0, 0, 0, (char *)MGConfigTitle},
			/*  1 */ {DI_RADIOBUTTON, 5, 2, 0, 0, 1, 0, DIF_GROUP, 0, (char *)MGConfigHideNone},
			/*  2 */ {DI_RADIOBUTTON, 5, 3, 0, 0, 0, 0, 0, 0, (char *)MGConfigHideView},
			/*  3 */ {DI_RADIOBUTTON, 5, 4, 0, 0, 0, 0, 0, 0, (char *)MGConfigHideAlways},
			/*  4 */ {DI_TEXT, 5, 5, 0, 0, 0, 0, DIF_BOXCOLOR | DIF_SEPARATOR, 0, ""},
			/*  5 */ {DI_CHECKBOX, 5, 6, 0, 0, 0, 0, 0, 0, (char *)MGConfigProcessShiftF1},

			/*  5 */ {DI_CHECKBOX, 5, 7, 0, 0, 0, 0, 0, 0, (char *)MGConfigAllowChangeDir},
			/*  6 */ {DI_CHECKBOX, 5, 8, 0, 0, 0, 0, 0, 0, (char *)MGConfigUseLastHistory},

			//*  7 */{DI_CHECKBOX,5,8,0,0,0,0,0,0,(char *)MGConfigDeleteExtFile},
			//*  8 */{DI_CHECKBOX,5,9,0,0,0,0,0,0,(char *)MGConfigAddExtArchive},
			/*  7 */ {DI_CHECKBOX, 5, 9, 0, 0, 0, 0, 0, 0, (char *)MGConfigAutoResetExactArcName},
			/*  8 */
			{DI_CHECKBOX, 5, 10, 0, 0, 0, 1, DIF_DISABLE | DIF_HIDDEN, 0, (char *)MGConfigOldDialogStyle},

			/*  9 */ {DI_TEXT, 5, 10, 0, 0, 0, 0, DIF_BOXCOLOR | DIF_SEPARATOR, 0, ""},
			/* 10 */ {DI_TEXT, 5, 11, 0, 0, 0, 0, 0, 0, (char *)MGConfigDizNames},
			/* 11 */ {DI_EDIT, 5, 12, 70, 10, 0, 0, 0, 0, ""},
			/* 12 */ {DI_CHECKBOX, 5, 13, 0, 0, 0, 0, 0, 0, (char *)MGConfigReadDiz},
			/* 13 */ {DI_CHECKBOX, 5, 14, 0, 0, 0, 0, 0, 0, (char *)MGConfigUpdateDiz},
			/* 14 */ {DI_TEXT, 5, 15, 0, 0, 0, 0, DIF_BOXCOLOR | DIF_SEPARATOR, 0, ""},
			/* 15 */ {DI_BUTTON, 0, 16, 0, 0, 0, 0, DIF_CENTERGROUP, 1, (char *)MOk},
			/* 16 */ {DI_BUTTON, 0, 16, 0, 0, 0, 0, DIF_CENTERGROUP, 0, (char *)MCancel},
	};

	struct FarDialogItem DialogItems[ARRAYSIZE(InitItems)];
	InitDialogItems(InitItems, DialogItems, ARRAYSIZE(InitItems));
	DialogItems[1].Selected = (Opt.HideOutput == 0);
	DialogItems[2].Selected = (Opt.HideOutput == 1);
	DialogItems[3].Selected = (Opt.HideOutput == 2);
	DialogItems[5].Selected = Opt.ProcessShiftF1;
	DialogItems[6].Selected = Opt.AllowChangeDir;
	DialogItems[7].Selected = Opt.UseLastHistory;
	//  DialogItems[7].Param.Selected=Opt.DeleteExtFile;
	//  DialogItems[8].Param.Selected=Opt.AddExtArchive;
	DialogItems[8].Selected = Opt.AdvFlags.AutoResetExactArcName;
	CharArrayCpyZ(DialogItems[12].Data, Opt.DescriptionNames.c_str());
	DialogItems[13].Selected = Opt.ReadDescriptions;
	DialogItems[14].Selected = Opt.UpdateDescriptions;
	int ExitCode = Info.Dialog(Info.ModuleNumber, -1, -1, 76, 19, "ArcSettings1", DialogItems,
			ARRAYSIZE(DialogItems));
	if (ExitCode == 17 || ExitCode < 0)
		return false;

	if (DialogItems[1].Selected)
		Opt.HideOutput = 0;
	else
		Opt.HideOutput = (DialogItems[2].Selected) ? 1 : 2;
	Opt.ProcessShiftF1 = DialogItems[5].Selected;
	Opt.AllowChangeDir = DialogItems[6].Selected;
	Opt.UseLastHistory = DialogItems[7].Selected;

	// Opt.DeleteExtFile=DialogItems[7].Param.Selected;
	// Opt.AddExtArchive=DialogItems[8].Param.Selected;
	Opt.AdvFlags.AutoResetExactArcName = DialogItems[8].Selected;

	Opt.DescriptionNames = DialogItems[12].Data;
	Opt.ReadDescriptions = DialogItems[13].Selected;
	Opt.UpdateDescriptions = DialogItems[14].Selected;

	KeyFileHelper kfh(INI_LOCATION);
	kfh.SetInt(INI_SECTION, "HideOutput", Opt.HideOutput);
	kfh.SetInt(INI_SECTION, "UseLastHistory", Opt.UseLastHistory);
	kfh.SetInt(INI_SECTION, "ProcessShiftF1", Opt.ProcessShiftF1);
	kfh.SetString(INI_SECTION, "DescriptionNames", Opt.DescriptionNames);
	kfh.SetInt(INI_SECTION, "ReadDescriptions", Opt.ReadDescriptions);
	kfh.SetInt(INI_SECTION, "UpdateDescriptions", Opt.UpdateDescriptions);
	kfh.SetInt(INI_SECTION, "AllowChangeDir", Opt.AllowChangeDir);

	// kfh.SetInt(INI_SECTION,"DeleteExtFile",Opt.DeleteExtFile);
	// kfh.SetInt(INI_SECTION,"AddExtArchive",Opt.AddExtArchive);
	// kfh.SetInt(INI_SECTION,"AutoResetExactArcName",Opt.AutoResetExactArcName);
	kfh.SetInt(INI_SECTION, "AdvFlags", (int)Opt.AdvFlags);

	return true;
}

#define DM_INITCONFIG DM_USER + 1
typedef struct FORMATINFO_
{
	std::string ArcFormat;
	int PluginNumber{}, PluginType{};
	bool FastAccess{};
} FORMATINFO;

LONG_PTR WINAPI CfgCmdProc(HANDLE hDlg, int Msg, int Param1, LONG_PTR Param2)
{
	switch (Msg) {
		case DN_INITDIALOG:
			Info.SendDlgMessage(hDlg, DM_INITCONFIG, 0, 0);
			break;

		case DM_INITCONFIG: {
			int I, J;
			std::string Command;
			FORMATINFO *FormatInfo = (FORMATINFO *)Info.SendDlgMessage(hDlg, DM_GETDLGDATA, 0, 0);
			for (I = 2, J = 0; I <= 32; I+= 2, J++) {
				Command.clear();
				int PluginNumber = FormatInfo->PluginNumber, PluginType = FormatInfo->PluginType;
				if (FormatInfo->FastAccess
						|| PluginClass::FormatToPlugin(FormatInfo->ArcFormat, PluginNumber, PluginType)) {
					if (I == 32) {
						std::string PluginFormat;
						ArcPlugin->GetFormatName(PluginNumber, PluginType, PluginFormat, Command);
					} else
						ArcPlugin->GetDefaultCommands(PluginNumber, PluginType, J, Command);
				}
				if (!Param1)	// if not Reset
				{
					Command = KeyFileReadSection(INI_LOCATION, FormatInfo->ArcFormat).GetString(CmdNames[J], Command.c_str());
				}
				SetDialogControlText(hDlg, I, Command);
			}
			return TRUE;
		}

		case DN_BTNCLICK:
			if (Param1 == 36) {
				Info.SendDlgMessage(hDlg, DM_INITCONFIG, 1, 0);
				return TRUE;
			}
			break;
	}

	return Info.DefDlgProc(hDlg, Msg, Param1, Param2);
}

bool ConfigCommands(const std::string &ArcFormat, int IDFocus, bool FastAccess, int PluginNumber, int PluginType)
{
	struct InitDialogItem InitItems[] = {
			/* 00 */ {DI_DOUBLEBOX, 3, 1, 72, 20, 0, 0, 0, 0, ArcFormat.c_str()},
			/* 01 */ {DI_TEXT, 5, 2, 0, 0, 0, 0, 0, 0, (char *)MArcSettingsExtract},
			/* 02 */ {DI_EDIT, 25, 2, 70, 3, 0, 0, 0, 0, ""},
			/* 03 */ {DI_TEXT, 5, 3, 0, 0, 0, 0, 0, 0, (char *)MArcSettingsExtractWithoutPath},
			/* 04 */ {DI_EDIT, 25, 3, 70, 3, 0, 0, 0, 0, ""},
			/* 05 */ {DI_TEXT, 5, 4, 0, 0, 0, 0, 0, 0, (char *)MArcSettingsTest},
			/* 06 */ {DI_EDIT, 25, 4, 70, 3, 0, 0, 0, 0, ""},
			/* 07 */ {DI_TEXT, 5, 5, 0, 0, 0, 0, 0, 0, (char *)MArcSettingsDelete},
			/* 08 */ {DI_EDIT, 25, 5, 70, 3, 0, 0, 0, 0, ""},
			/* 09 */ {DI_TEXT, 5, 6, 0, 0, 0, 0, 0, 0, (char *)MArcSettingsComment},
			/* 10 */ {DI_EDIT, 25, 6, 70, 3, 0, 0, 0, 0, ""},
			/* 11 */ {DI_TEXT, 5, 7, 0, 0, 0, 0, 0, 0, (char *)MArcSettingsCommentFiles},
			/* 12 */ {DI_EDIT, 25, 7, 70, 3, 0, 0, 0, 0, ""},
			/* 13 */ {DI_TEXT, 5, 8, 0, 0, 0, 0, 0, 0, (char *)MArcSettingsSFX},
			/* 14 */ {DI_EDIT, 25, 8, 70, 3, 0, 0, 0, 0, ""},
			/* 15 */ {DI_TEXT, 5, 9, 0, 0, 0, 0, 0, 0, (char *)MArcSettingsLock},
			/* 16 */ {DI_EDIT, 25, 9, 70, 3, 0, 0, 0, 0, ""},
			/* 17 */ {DI_TEXT, 5, 10, 0, 0, 0, 0, 0, 0, (char *)MArcSettingsProtect},
			/* 18 */ {DI_EDIT, 25, 10, 70, 3, 0, 0, 0, 0, ""},
			/* 19 */ {DI_TEXT, 5, 11, 0, 0, 0, 0, 0, 0, (char *)MArcSettingsRecover},
			/* 20 */ {DI_EDIT, 25, 11, 70, 3, 0, 0, 0, 0, ""},
			/* 21 */ {DI_TEXT, 5, 12, 0, 0, 0, 0, 0, 0, (char *)MArcSettingsAdd},
			/* 22 */ {DI_EDIT, 25, 12, 70, 3, 0, 0, 0, 0, ""},
			/* 23 */ {DI_TEXT, 5, 13, 0, 0, 0, 0, 0, 0, (char *)MArcSettingsMove},
			/* 24 */ {DI_EDIT, 25, 13, 70, 3, 0, 0, 0, 0, ""},
			/* 25 */ {DI_TEXT, 5, 14, 0, 0, 0, 0, 0, 0, (char *)MArcSettingsAddRecurse},
			/* 26 */ {DI_EDIT, 25, 14, 70, 3, 0, 0, 0, 0, ""},
			/* 27 */ {DI_TEXT, 5, 15, 0, 0, 0, 0, 0, 0, (char *)MArcSettingsMoveRecurse},
			/* 28 */ {DI_EDIT, 25, 15, 70, 3, 0, 0, 0, 0, ""},
			/* 29 */ {DI_TEXT, 5, 16, 0, 0, 0, 0, 0, 0, (char *)MArcSettingsAllFilesMask},
			/* 30 */ {DI_EDIT, 25, 16, 70, 3, 0, 0, 0, 0, ""},
			/* 31 */ {DI_TEXT, 5, 17, 0, 0, 0, 0, 0, 0, (char *)MArcSettingsDefaultExt},
			/* 32 */ {DI_EDIT, 25, 17, 70, 3, 0, 0, 0, 0, ""},
			/* 33 */ {DI_TEXT, 3, 18, 0, 0, 0, 0, DIF_BOXCOLOR | DIF_SEPARATOR, 0, ""},
			/* 34 */ {DI_BUTTON, 0, 19, 0, 0, 0, 0, DIF_CENTERGROUP, 1, (char *)MOk},
			/* 35 */ {DI_BUTTON, 0, 19, 0, 0, 0, 0, DIF_CENTERGROUP, 0, (char *)MCancel},
			/* 36 */ {DI_BUTTON, 0, 19, 0, 0, 0, 0, DIF_CENTERGROUP, 0, (char *)MReset},
	};

	struct FarDialogItem DialogItems[ARRAYSIZE(InitItems)];
	int Length, MaxLength = 0, I, J;
	InitDialogItems(InitItems, DialogItems, ARRAYSIZE(InitItems));

	DialogItems[IDFocus].Focus = 1;

	for (I = 1; I <= 31; I+= 2)
		if ((Length = FSF.StrCellsCount(DialogItems[I].Data, strlen(DialogItems[I].Data))) > MaxLength)
			MaxLength = Length;

	for (I = 2, J = 0; I <= 32; I+= 2, J++)
		DialogItems[I].X1 = 6 + MaxLength;

	FORMATINFO FormatInfo;
	FormatInfo.ArcFormat = ArcFormat;
	FormatInfo.FastAccess = FastAccess;
	FormatInfo.PluginNumber = PluginNumber;
	FormatInfo.PluginType = PluginType;

	int ExitCode = Info.DialogEx(Info.ModuleNumber, -1, -1, 76, 22, "ArcSettings2", DialogItems,
			ARRAYSIZE(DialogItems), 0, 0, CfgCmdProc, (LONG_PTR)&FormatInfo);

	if (ExitCode == 35 || ExitCode < 0)
		return false;

	KeyFileHelper kfh(INI_LOCATION);
	for (I = 2, J = 0; I <= 32; I+= 2, J++)
		kfh.SetString(ArcFormat, CmdNames[J], DialogItems[I].Data);

	return true;
}
