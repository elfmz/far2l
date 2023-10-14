/*
TMPCFG.CPP

Temporary panel configuration

*/

#include "TmpPanel.hpp"
#include <KeyFileHelper.h>
#include <utils.h>

#ifndef UNICODE
#define GetCheck(i)   DialogItems[i].Selected
#define GetDataPtr(i) DialogItems[i].Data
#else
#define GetCheck(i)   (int)Info.SendDlgMessage(hDlg, DM_GETCHECK, i, 0)
#define GetDataPtr(i) ((const TCHAR *)Info.SendDlgMessage(hDlg, DM_GETCONSTTEXTPTR, i, 0))
#endif

#define INI_LOCATION InMyConfig("plugins/tmppanel/config.ini")
#define INI_SECTION  "Settings"

enum
{
	AddToDisksMenu,
	AddToPluginsMenu,
	CommonPanel,
	SafeModePanel,
	AnyInPanel,
	CopyContents,
	Mode,
	MenuForFilelist,
	NewPanelForSearchResults,
	FullScreenPanel,
	ColumnTypes,
	ColumnWidths,
	StatusColumnTypes,
	StatusColumnWidths,
#ifndef UNICODE
	DisksMenuDigit,
#endif
	Mask,
	Prefix
};

options_t Opt;

static const char REGStr[][9] = {("InDisks"), ("InPlug"), ("Common"), ("Safe"), ("Any"), ("Contents"),
		("Mode"), ("Menu"), ("NewP"), ("Full"), ("ColT"), ("ColW"), ("StatT"), ("StatW"),
#ifndef UNICODE
		("DigitV"),
#endif
		("Mask"), ("Prefix")};

struct COptionsList
{
	void *Option;
	const TCHAR *pStr;
	unsigned int DialogItem;
	size_t OptionSize;
};

static const struct COptionsList OptionsList[] = {
		{&Opt.AddToDisksMenu,           _T(""),          1,  sizeof(Opt.AddToDisksMenu)          },
		{&Opt.AddToPluginsMenu,         _T(""),          4,  sizeof(Opt.AddToPluginsMenu)        },

		{&Opt.CommonPanel,              _T(""),          6,  sizeof(Opt.CommonPanel)             },
		{&Opt.SafeModePanel,            NULL,            7,  sizeof(Opt.SafeModePanel)           },
		{&Opt.AnyInPanel,               NULL,            8,  sizeof(Opt.AnyInPanel)              },
		{&Opt.CopyContents,             NULL,            9,  sizeof(Opt.CopyContents)            },
		{&Opt.Mode,                     _T(""),          10, sizeof(Opt.Mode)                    },
		{&Opt.MenuForFilelist,          NULL,            11, sizeof(Opt.MenuForFilelist)         },
		{&Opt.NewPanelForSearchResults, NULL,            12, sizeof(Opt.NewPanelForSearchResults)},

		{&Opt.FullScreenPanel,          NULL,            22, sizeof(Opt.FullScreenPanel)         },

		{Opt.ColumnTypes,               _T("N,S"),       15, sizeof(Opt.ColumnTypes)             },
		{Opt.ColumnWidths,              _T("0,8"),       17, sizeof(Opt.ColumnWidths)            },
		{Opt.StatusColumnTypes,         _T("NR,SC,D,T"), 19, sizeof(Opt.StatusColumnTypes)       },
		{Opt.StatusColumnWidths,        _T("0,8,0,5"),   21, sizeof(Opt.StatusColumnWidths)      },

#ifndef UNICODE
		{Opt.DisksMenuDigit,            _T("1"),         2,  sizeof(Opt.DisksMenuDigit)          },
#endif
		{Opt.Mask,                      _T("*.temp"),    25, sizeof(Opt.Mask)                    },
		{Opt.Prefix,                    _T("tmp"),       27, sizeof(Opt.Mask)                    },
};

int StartupOptFullScreenPanel, StartupOptCommonPanel, StartupOpenFrom;

void GetOptions(void)
{
	KeyFileReadSection kfh(INI_LOCATION, INI_SECTION);
	std::wstring str;
	for (int i = AddToDisksMenu; i <= Prefix; i++) {
		if (i < ColumnTypes) {
			(*(int *)(OptionsList[i]).Option) = kfh.GetInt(REGStr[i], (OptionsList[i].pStr ? 1 : 0));
		} else {
			str = kfh.GetString(REGStr[i], OptionsList[i].pStr);
			if (str.size() * sizeof(TCHAR) > OptionsList[i].OptionSize) {
				str.resize(OptionsList[i].OptionSize / sizeof(TCHAR));
			}
			lstrcpy((TCHAR *)OptionsList[i].Option, str.c_str());
		}
	}
}

const int DIALOG_WIDTH = 78;
const int DIALOG_HEIGHT = 22;
const int DC = DIALOG_WIDTH / 2 - 1;

int Config()
{
	static const MyInitDialogItem InitItems[] = {
			/* 0*/ {DI_DOUBLEBOX, 3, 1, DIALOG_WIDTH - 4, DIALOG_HEIGHT - 2, 0, MConfigTitle},

			/* 1*/ {DI_CHECKBOX, 5, 2, 0, 0, 0, MConfigAddToDisksMenu},
			/* 2*/
			{DI_FIXEDIT, 7, 3, 7, 3,
					0
#ifdef UNICODE
							| DIF_HIDDEN
#endif
					,
					-1},
			/* 3*/
			{DI_TEXT, 9, 3, 0, 0,
					0
#ifdef UNICODE
							| DIF_HIDDEN
#endif
					,
					MConfigDisksMenuDigit},
			/* 4*/ {DI_CHECKBOX, DC, 2, 0, 0, 0, MConfigAddToPluginsMenu},
			/* 5*/ {DI_TEXT, 5, 4, 0, 0, DIF_BOXCOLOR | DIF_SEPARATOR, -1},

			/* 6*/ {DI_CHECKBOX, 5, 5, 0, 0, 0, MConfigCommonPanel},
			/* 7*/ {DI_CHECKBOX, 5, 6, 0, 0, 0, MSafeModePanel},
			/* 8*/ {DI_CHECKBOX, 5, 7, 0, 0, 0, MAnyInPanel},
			/* 9*/ {DI_CHECKBOX, 5, 8, 0, 0, DIF_3STATE, MCopyContents},
			/*10*/ {DI_CHECKBOX, DC, 5, 0, 0, 0, MReplaceInFilelist},
			/*11*/ {DI_CHECKBOX, DC, 6, 0, 0, 0, MMenuForFilelist},
			/*12*/ {DI_CHECKBOX, DC, 7, 0, 0, 0, MNewPanelForSearchResults},

			/*13*/ {DI_TEXT, 5, 9, 0, 0, DIF_BOXCOLOR | DIF_SEPARATOR, -1},

			/*14*/ {DI_TEXT, 5, 10, 0, 0, 0, MColumnTypes},
			/*15*/ {DI_EDIT, 5, 11, 36, 11, 0, -1},
			/*16*/ {DI_TEXT, 5, 12, 0, 0, 0, MColumnWidths},
			/*17*/ {DI_EDIT, 5, 13, 36, 13, 0, -1},
			/*18*/ {DI_TEXT, DC, 10, 0, 0, 0, MStatusColumnTypes},
			/*19*/ {DI_EDIT, DC, 11, 72, 11, 0, -1},
			/*20*/ {DI_TEXT, DC, 12, 0, 0, 0, MStatusColumnWidths},
			/*21*/ {DI_EDIT, DC, 13, 72, 13, 0, -1},
			/*22*/ {DI_CHECKBOX, 5, 14, 0, 0, 0, MFullScreenPanel},

			/*23*/ {DI_TEXT, 5, 15, 0, 0, DIF_BOXCOLOR | DIF_SEPARATOR, -1},

			/*24*/ {DI_TEXT, 5, 16, 0, 0, 0, MMask},
			/*25*/ {DI_EDIT, 5, 17, 36, 17, 0, -1},
			/*26*/ {DI_TEXT, DC, 16, 0, 0, 0, MPrefix},
			/*27*/ {DI_EDIT, DC, 17, 72, 17, 0, -1},

			/*28*/ {DI_TEXT, 5, 18, 0, 0, DIF_BOXCOLOR | DIF_SEPARATOR, -1},

			/*29*/ {DI_BUTTON, 0, 19, 0, 0, DIF_CENTERGROUP, MOk},
			/*30*/ {DI_BUTTON, 0, 19, 0, 0, DIF_CENTERGROUP, MCancel}};

	int i;
	struct FarDialogItem DialogItems[ARRAYSIZE(InitItems)];

	InitDialogItems(InitItems, DialogItems, ARRAYSIZE(InitItems));
	DialogItems[29].DefaultButton = 1;
	DialogItems[2].Focus = 1;

	GetOptions();
	for (i = AddToDisksMenu; i <= Prefix; i++)
		if (i < ColumnTypes)
			DialogItems[OptionsList[i].DialogItem].Selected = *(int *)(OptionsList[i].Option);
		else
#ifndef UNICODE
			lstrcpy(DialogItems[OptionsList[i].DialogItem].Data, (char *)OptionsList[i].Option);
#else
			DialogItems[OptionsList[i].DialogItem].PtrData = (TCHAR *)OptionsList[i].Option;
#endif

#ifndef UNICODE
	int Ret = Info.Dialog(Info.ModuleNumber, -1, -1, DIALOG_WIDTH, DIALOG_HEIGHT, "Config", DialogItems,
			ARRAYSIZE(DialogItems));
#else
	HANDLE hDlg = Info.DialogInit(Info.ModuleNumber, -1, -1, DIALOG_WIDTH, DIALOG_HEIGHT, L"Config",
			DialogItems, ARRAYSIZE(DialogItems), 0, 0, NULL, 0);
	if (hDlg == INVALID_HANDLE_VALUE)
		return FALSE;

	int Ret = Info.DialogRun(hDlg);
#endif

	KeyFileHelper kfh(INI_LOCATION);
	if ((unsigned)Ret >= ARRAYSIZE(InitItems) - 1)
		goto done;

	for (i = AddToDisksMenu; i <= Prefix; i++) {
		if (i < ColumnTypes) {
			*((int *)OptionsList[i].Option) = GetCheck(OptionsList[i].DialogItem);
			kfh.SetInt(INI_SECTION, REGStr[i], *(int *)OptionsList[i].Option);
		} else {
			FSF.Trim(lstrcpy((TCHAR *)OptionsList[i].Option, GetDataPtr(OptionsList[i].DialogItem)));
			kfh.SetString(INI_SECTION, REGStr[i], (wchar_t *)OptionsList[i].Option);
		}
	}
	kfh.Save();

	if (StartupOptFullScreenPanel != Opt.FullScreenPanel || StartupOptCommonPanel != Opt.CommonPanel) {
		const TCHAR *MsgItems[] = {GetMsg(MTempPanel), GetMsg(MConfigNewOption), GetMsg(MOk)};
		Info.Message(Info.ModuleNumber, 0, NULL, MsgItems, ARRAYSIZE(MsgItems), 1);
	}
	if (GetTmpPanelModule()) {
		// plugins menus could change - purge all information cached by far2l
		Info.PluginsControl(NULL, PCTL_CACHEFORGET, PLT_PATH, (LONG_PTR)GetTmpPanelModule());
	}
done:
#ifdef UNICODE
	Info.DialogFree(hDlg);
#endif
	return ((unsigned)Ret < ARRAYSIZE(InitItems));
}
