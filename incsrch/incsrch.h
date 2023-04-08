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
#pragma once

#ifdef _MSC_VER
#pragma warning(disable : 4514)
#pragma warning(push, 1)
#endif
#include "minwin.h"
#include <string.h>
#include <limits.h>
#include <stddef.h>
#if !defined(__APPLE__) && !defined(__FreeBSD__) && !defined(__DragonFly__)
#include <malloc.h>
#endif
#include <stdlib.h>

#if defined(WINPORT_DIRECT)

static inline wchar_t __cdecl Upper(wchar_t Ch)
{
	WINPORT(CharUpperBuff)(&Ch, 1);
	return Ch;
}

#define __int64         __int64_t
#define _MAX_PATH       MAX_PATH
#define _tstrcpy(a, b)  lstrcpy((a), (b))
#define _tstrlen(a)     lstrlen((a))
#define _tstrnlen(a, n) lstrnlen((a), (n))
#else
#include <tchar.h>
#define _tstrcpy(a, b)  strcpy((a), (b))
#define _tstrlen(a)     strlen((a))
#define _tstrnlen(a, n) sstrnlen((a), (n))
#if !defined(INT_PTR)
#define INT_PTR int
#endif
#endif

#include "farplug-wide.h"
#include "farcolor.h"
#ifdef _MSC_VER
#pragma warning(pop)
#endif
#include "srchlng.h"

#define MAX_STR                80
#define TITLE_LEN              26
#define TITLE_PREFIX_STR       _T("^: ")
#define TITLE_PREFIX_STR_DWORD 0x203A5E
#define TITLE_PREFIX_LEN       3
#define PREVIEW_EVENTS         MAX_STR

#if defined(__WATCOMC__)
#define __plugin __declspec(dllexport)
#elif defined(WINPORT_DIRECT)
#define __plugin SHAREDSYMBOL
#else
#define __plugin
#endif

#if !defined(WINPORT_DIRECT) && !defined(EXP_NAME)
#define EXP_NAME(x) x
#endif

#ifdef __cplusplus
extern "C" {
#endif

extern INT_PTR ModuleNumber;
extern FARAPIMENU apiMenu;
extern FARAPIGETMSG apiGetMsg;
extern FARAPITEXT apiText;
extern FARAPIEDITORCONTROL apiEditorControl;
#ifdef VIEWVER_SUPPORT
extern FARAPIVIEWERCONTROL apiViewerControl;
extern int iViewerStatusColor;
#endif
#if defined(WINPORT_DIRECT)
extern FARAPIDIALOGINIT apiDialogInit;
extern FARAPIDIALOGRUN apiDialogRun;
extern FARAPIDIALOGFREE apiDialogFree;
extern FARAPISENDDLGMESSAGE apiSendDlgMessage;
extern FARSTDSNPRINTF apiSnprintf;
#else
extern FARAPIDIALOG apiDialog;
extern FARAPICHARTABLE apiCharTable;
#endif

#define GetMsg(MsgId) apiGetMsg(ModuleNumber, MsgId)

extern struct EditorInfo ei;
extern struct EditorConvertText ect;
extern struct EditorSetPosition esp;
extern struct EditorGetString egs;

extern TCHAR sStr[MAX_STR];
extern int nLen;

extern void SearchLoopEditor(void);
extern void SearchLoopViewer(void);
extern BOOL WaitInput(BOOL Infinite);

#define DIFT_MSGNUM 0x80000000

typedef struct _DialogTemplateItem
{
	unsigned char Type;
	signed char X1;
	signed char Y1;
	signed char Selected;
	unsigned int Flags;
	INT_PTR Data;	// Message number or pointer to string
} DialogTemplateItem;

extern int DialogFromTemplate(const TCHAR *sTitle, const DialogTemplateItem *aTplItems,
		struct FarDialogItem *aDialogItems, int nItemsNumber, TCHAR *sHelpTopic, int nFocus,
		int nDefaultButton);

extern BOOL CollectEvents(void);
extern void ShowTitle(int OpenFrom);
extern void SelectFound(BOOL bRedraw);
extern void StatusMessage(int Index);
extern void SetPosition(int nLine, int nPos, int nLeftPos);
extern void PositionToView(int nLine, int nStartPos);
extern void PasteSearchText(void);

#define KC_CHAR     1
#define KC_BACK     2
#define KC_NEXT     4
#define KC_PREV     8
#define KC_HELP     16
#define KC_CLEARMSG 32
#define KC_FAILKILL 128

typedef struct _KbdCommand
{
	unsigned char Flags;
	unsigned char AsciiChar;
} KbdCommand;

extern KbdCommand aEvents[PREVIEW_EVENTS];
extern int nEvents;

extern BOOL bEscape;
extern BOOL bTermEvent;
extern INPUT_RECORD Event;
#if !defined(WINPORT_DIRECT)
extern HANDLE hInputHandle;
#endif
extern BOOL bNotFound;

/* config options */
extern BOOL bCaseSensitive;
extern BOOL bKeepSelection;
extern BOOL bBeepOnMismatch;
extern BOOL bRestartEOF;
extern BOOL bUseSelection;
extern BOOL bAutoNext;
extern BOOL bBSunroll;

extern BOOL bThisUseSelection;
extern BOOL bThisAutoNext;
extern BOOL bStopOnFound;
extern BOOL bReverse;

#ifdef __cplusplus
}
#endif

#include "loc.h"
#include "misc.h"

#ifdef _MSC_VER
#define _heapshrink _heapmin
#endif
