/*
FAR manager incremental search plugin, search as you type in editor.
Copyright (C) 1999-2019, Stanislav V. Mekhanoshin

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/
#include "incsrch.h"

static const TCHAR sDirIncSearch[] =
#if defined(WINPORT_DIRECT)
		_T("/Incremental Search");
#else
		_T("\\Incremental Search");
#endif

static const TCHAR sHlfConfig[] = _T("Cfg");
static const TCHAR sHlfMenu[] = _T("Menu");

INT_PTR ModuleNumber;
FARAPIGETMSG apiGetMsg;
FARAPIMENU apiMenu;
FARAPITEXT apiText;
FARAPIEDITORCONTROL apiEditorControl;
#ifdef VIEWVER_SUPPORT
FARAPIVIEWERCONTROL apiViewerControl;
int iViewerStatusColor;
#endif
#if defined(WINPORT_DIRECT)
FARAPIDIALOGINIT apiDialogInit;
FARAPIDIALOGRUN apiDialogRun;
FARAPIDIALOGFREE apiDialogFree;
FARAPISENDDLGMESSAGE apiSendDlgMessage;
FARSTDSNPRINTF apiSnprintf;
#else
FARAPIDIALOG apiDialog;
FARAPICHARTABLE apiCharTable;
#endif

BOOL bEscape;
BOOL bTermEvent;
int nEvents;
#if !defined(WINPORT_DIRECT)
HANDLE hInputHandle;
#endif
BOOL bNotFound;
KbdCommand aEvents[PREVIEW_EVENTS];
INPUT_RECORD Event;

static TCHAR PluginRootKey[_MAX_PATH];

BOOL bThisUseSelection;
BOOL bThisAutoNext;
BOOL bStopOnFound;
BOOL bReverse = FALSE;

BOOL bCaseSensitive = FALSE;
BOOL bKeepSelection = TRUE;
BOOL bBeepOnMismatch = FALSE;
BOOL bRestartEOF = FALSE;
BOOL bUseSelection = TRUE;
BOOL bAutoNext = FALSE;
BOOL bBSunroll = TRUE;

TCHAR sStr[MAX_STR];
int nLen;

struct EditorInfo ei;
#if !defined(WINPORT_DIRECT)
struct EditorConvertText ect;
#endif
struct EditorSetPosition esp;
struct EditorGetString egs;

void RestoreConfig(void);
void SaveConfig(void);

#if !defined(WINPORT_DIRECT)
#ifndef SHAREDSYMBOL
#define SHAREDSYMBOL
#endif
#ifndef EXP_NAME
#define EXP_NAME(x) x
#endif
#endif

void __plugin WINAPI EXP_NAME(SetStartupInfo)(const struct PluginStartupInfo *pInfo)
{
	ModuleNumber = pInfo->ModuleNumber;
	apiMenu = pInfo->Menu;
	apiGetMsg = pInfo->GetMsg;
#if defined(WINPORT_DIRECT)
	apiDialogInit = pInfo->DialogInit;
	apiDialogRun = pInfo->DialogRun;
	apiDialogFree = pInfo->DialogFree;
	apiSendDlgMessage = pInfo->SendDlgMessage;
	apiSnprintf = pInfo->FSF->snprintf;
#else
	apiDialog = pInfo->Dialog;
	apiCharTable = pInfo->CharTable;
#endif
	apiText = pInfo->Text;
	apiEditorControl = pInfo->EditorControl;
	_tstrcpy(PluginRootKey, pInfo->RootKey);
	_tstrcpy(PluginRootKey + _tstrlen(PluginRootKey), sDirIncSearch);
#ifdef VIEWVER_SUPPORT
	apiViewerControl = pInfo->ViewerControl;
	iViewerStatusColor = pInfo->AdvControl(ModuleNumber, ACTL_GETCOLOR, (void *)COL_VIEWERSTATUS);
#endif
	RestoreConfig();
}

int __plugin WINAPI EXP_NAME(Configure)(int ItemNumber)
{
	struct FarDialogItem DialogItems[11];
	static DialogTemplateItem DialogTemplate[10] = {
		{DI_CHECKBOX, 5, 2,  FALSE, DIFT_MSGNUM,                   MCaseSensitive },
		{DI_CHECKBOX, 5, 3,  FALSE, DIFT_MSGNUM,                   MRestartEOF    },
		{DI_CHECKBOX, 5, 4,  FALSE, DIFT_MSGNUM,                   MKeepSelection },
		{DI_CHECKBOX, 5, 5,  FALSE, DIFT_MSGNUM,                   MBeepOnMismatch},
		{DI_CHECKBOX, 5, 6,  FALSE, DIFT_MSGNUM,                   MUseSelection  },
		{DI_CHECKBOX, 5, 7,  FALSE, DIFT_MSGNUM,                   MAutoNext      },
		{DI_CHECKBOX, 5, 8,  FALSE, DIFT_MSGNUM,                   MBSunroll      },
		{DI_TEXT,     5, 9,  FALSE, DIF_BOXCOLOR | DIF_SEPARATOR,  0              },
		{DI_BUTTON,   5, 10, FALSE, DIF_CENTERGROUP | DIFT_MSGNUM, MOk            },
		{DI_BUTTON,   5, 10, FALSE, DIF_CENTERGROUP | DIFT_MSGNUM, MCancel        }
	};

	(void)ItemNumber;

	DialogTemplate[0].Selected = (signed char)bCaseSensitive;
	DialogTemplate[1].Selected = (signed char)bRestartEOF;
	DialogTemplate[2].Selected = (signed char)bKeepSelection;
	DialogTemplate[3].Selected = (signed char)bBeepOnMismatch;
	DialogTemplate[4].Selected = (signed char)bUseSelection;
	DialogTemplate[5].Selected = (signed char)bAutoNext;
	DialogTemplate[6].Selected = (signed char)bBSunroll;

	if (DialogFromTemplate(GetMsg(MIncSearch), DialogTemplate, DialogItems,
				sizeof(DialogItems) / sizeof(DialogItems[0]), (TCHAR *)sHlfConfig, 1, 9)
			!= 9)
		return FALSE;

	bCaseSensitive = DialogItems[1].Selected;
	bRestartEOF = DialogItems[2].Selected;
	bKeepSelection = DialogItems[3].Selected;
	bBeepOnMismatch = DialogItems[4].Selected;
	bUseSelection = DialogItems[5].Selected;
	bAutoNext = DialogItems[6].Selected;
	bBSunroll = DialogItems[7].Selected;

	SaveConfig();
	return TRUE;
}

void __plugin WINAPI EXP_NAME(GetPluginInfo)(struct PluginInfo *Info)
{
	static const TCHAR *sPtr;

	sPtr = GetMsg(MIncSearch);
	Info->StructSize = sizeof(*Info);
	Info->Flags =
#ifdef VIEWVER_SUPPORT
			PF_VIEWER |
#endif
			PF_DISABLEPANELS | PF_EDITOR;
	Info->DiskMenuStringsNumber = 0;
	Info->PluginMenuStrings = &sPtr;
	Info->PluginMenuStringsNumber = 1;
	Info->PluginConfigStrings = &sPtr;
	Info->PluginConfigStringsNumber = 1;
}

HANDLE __plugin WINAPI EXP_NAME(OpenPlugin)(int OpenFrom, INT_PTR Item)
{
	struct FarMenuItem aMenuItems[7];
	int nItems;

	(void)OpenFrom;
	(void)Item;

	bThisUseSelection = bUseSelection;
	bThisAutoNext = bAutoNext;
	bStopOnFound = FALSE;

	zeromem(aMenuItems, sizeof(aMenuItems));
#if defined(WINPORT_DIRECT)
	aMenuItems[0].Text = GetMsg(MSearchForward);
	aMenuItems[1].Text = GetMsg(MSearchBackward);
#else
	_tstrcpy(aMenuItems[0].Text, GetMsg(MSearchForward));
	_tstrcpy(aMenuItems[1].Text, GetMsg(MSearchBackward));
#endif
	aMenuItems[2].Separator = TRUE;
	if (OpenFrom == OPEN_EDITOR) {
#if defined(WINPORT_DIRECT)
		aMenuItems[3].Text = GetMsg(MFindNext);
		aMenuItems[4].Text = GetMsg(MFindPrevious);
#else
		_tstrcpy(aMenuItems[3].Text, GetMsg(MFindNext));
		_tstrcpy(aMenuItems[4].Text, GetMsg(MFindPrevious));
#endif
		aMenuItems[5].Separator = TRUE;
		nItems = sizeof(aMenuItems) / sizeof(aMenuItems[0]);
	} else {
		nItems = 4;
	}
#if defined(WINPORT_DIRECT)
	aMenuItems[nItems - 1].Text = GetMsg(MSetup);
#else
	_tstrcpy(aMenuItems[nItems - 1].Text, GetMsg(MSetup));
#endif
	aMenuItems[bReverse ? 1 : 0].Selected = TRUE;
Menu:
	switch (apiMenu(ModuleNumber, -1, -1, 0, FMENU_WRAPMODE, GetMsg(MIncSearch), NULL, (TCHAR *)sHlfMenu,
			NULL, NULL, aMenuItems, nItems)) {
		case 0:
			bReverse = FALSE;
			break;
		case 1:
			bReverse = TRUE;
			break;
		case 3:
			if (OpenFrom == OPEN_EDITOR) {
				bReverse = FALSE;
				goto Auto;
				case 4:
					bReverse = TRUE;
				Auto:
					bThisUseSelection = TRUE;
					bThisAutoNext = TRUE;
					bStopOnFound = TRUE;
					break;
			}
		case 6:
			EXP_NAME(Configure)(0);
			goto Menu;
		default:
			return INVALID_HANDLE_VALUE;
	}
	if (OpenFrom == OPEN_EDITOR)
		SearchLoopEditor();
#ifdef VIEWVER_SUPPORT
	else
		SearchLoopViewer();
#endif
	return INVALID_HANDLE_VALUE;
}
