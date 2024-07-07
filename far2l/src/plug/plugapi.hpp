#pragma once

/*
plugapi.hpp

API, доступное плагинам (диалоги, меню, ...)
*/
/*
Copyright (c) 1996 Eugene Roshal
Copyright (c) 2000 Far Group
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions
are met:
1. Redistributions of source code must retain the above copyright
   notice, this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright
   notice, this list of conditions and the following disclaimer in the
   documentation and/or other materials provided with the distribution.
3. The name of the authors may not be used to endorse or promote products
   derived from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

//----------- PLUGIN API/FSF ---------------------------------------------------
// все эти функции, за исключение sprintf/sscanf имеют тип вызова __stdcall

wchar_t *WINAPI FarItoa(int value, wchar_t *string, int radix);
int64_t WINAPI FarAtoi64(const wchar_t *s);
wchar_t *WINAPI FarItoa64(int64_t value, wchar_t *string, int radix);
int WINAPI FarAtoi(const wchar_t *s);
void WINAPI FarQsort(void *base, size_t nelem, size_t width, int(__cdecl *fcmp)(const void *, const void *));
void WINAPI FarQsortEx(void *base, size_t nelem, size_t width,
		int(__cdecl *fcmp)(const void *, const void *, void *), void *);
void *WINAPI FarBsearch(const void *key, const void *base, size_t nelem, size_t width,
		int(__cdecl *fcmp)(const void *, const void *));

void WINAPI DeleteBuffer(void *Buffer);

void WINAPI farUpperBuf(wchar_t *Buf, int Length);
void WINAPI farLowerBuf(wchar_t *Buf, int Length);
void WINAPI farStrUpper(wchar_t *s1);
void WINAPI farStrLower(wchar_t *s1);
wchar_t WINAPI farUpper(wchar_t Ch);
wchar_t WINAPI farLower(wchar_t Ch);
int WINAPI farStrCmpNI(const wchar_t *s1, const wchar_t *s2, int n);
int WINAPI farStrCmpI(const wchar_t *s1, const wchar_t *s2);
int WINAPI farStrCmpN(const wchar_t *s1, const wchar_t *s2, int n);
int WINAPI farStrCmp(const wchar_t *s1, const wchar_t *s2);
int WINAPI farIsLower(wchar_t Ch);
int WINAPI farIsUpper(wchar_t Ch);
int WINAPI farIsAlpha(wchar_t Ch);
int WINAPI farIsAlphaNum(wchar_t Ch);

int WINAPI farGetFileOwner(const wchar_t *Computer, const wchar_t *Name, wchar_t *Owner, int Size);

int WINAPI farConvertPath(CONVERTPATHMODES Mode, const wchar_t *Src, wchar_t *Dest, int DestSize);

int WINAPI farGetReparsePointInfo(const wchar_t *Src, wchar_t *Dest, int DestSize);

int WINAPI farGetPathRoot(const wchar_t *Path, wchar_t *Root, int DestSize);

int WINAPI FarGetPluginDirList(INT_PTR PluginNumber, HANDLE hPlugin, const wchar_t *Dir,
		struct PluginPanelItem **pPanelItem, int *pItemsNumber);
void WINAPI FarFreePluginDirList(PluginPanelItem *PanelItem, int ItemsNumber);

int WINAPI FarMenuFn(INT_PTR PluginNumber, int X, int Y, int MaxHeight, DWORD Flags, const wchar_t *Title,
		const wchar_t *Bottom, const wchar_t *HelpTopic, const int *BreakKeys, int *BreakCode,
		const struct FarMenuItem *Item, int ItemsNumber);
const wchar_t *WINAPI FarGetMsgFn(INT_PTR PluginHandle, FarLangMsgID MsgId);
int WINAPI FarMessageFn(INT_PTR PluginNumber, DWORD Flags, const wchar_t *HelpTopic,
		const wchar_t *const *Items, int ItemsNumber, int ButtonsNumber);
int WINAPI FarControl(HANDLE hPlugin, int Command, int Param1, LONG_PTR Param2);
HANDLE WINAPI FarSaveScreen(int X1, int Y1, int X2, int Y2);
void WINAPI FarRestoreScreen(HANDLE hScreen);

int WINAPI FarGetDirList(const wchar_t *Dir, FAR_FIND_DATA **pPanelItem, int *pItemsNumber);
void WINAPI FarFreeDirList(FAR_FIND_DATA *PanelItem, int nItemsNumber);

int WINAPI FarViewer(const wchar_t *FileName, const wchar_t *Title, int X1, int Y1, int X2, int Y2,
		DWORD Flags, UINT CodePage);
int WINAPI FarEditor(const wchar_t *FileName, const wchar_t *Title, int X1, int Y1, int X2, int Y2,
		DWORD Flags, int StartLine, int StartChar, UINT CodePage);
int WINAPI FarCmpName(const wchar_t *pattern, const wchar_t *string, int skippath);
void WINAPI FarText(int X, int Y, uint64_t Color, const wchar_t *Str);
int WINAPI TextToCharInfo(const char *Text, uint64_t Attr, CHAR_INFO *CharInfo, int Length, DWORD Reserved);
int WINAPI FarEditorControl(int Command, void *Param);

int WINAPI FarViewerControl(int Command, void *Param);

/* Функция вывода помощи */
BOOL WINAPI FarShowHelp(const wchar_t *ModuleName, const wchar_t *HelpTopic, DWORD Flags);

/*
	Обертка вокруг GetString для плагинов - с меньшей функциональностью.
	Сделано для того, чтобы не дублировать код GetString.
*/

int WINAPI FarInputBox(const wchar_t *Title, const wchar_t *Prompt, const wchar_t *HistoryName,
		const wchar_t *SrcText, wchar_t *DestText, int DestLength, const wchar_t *HelpTopic, DWORD Flags);
/* Функция, которая будет действовать и в редакторе, и в панелях, и... */
INT_PTR WINAPI FarAdvControl(INT_PTR ModuleNumber, int Command, void *Param1, void *Param2);
// Функция расширенного диалога
HANDLE WINAPI FarDialogInit(INT_PTR PluginNumber, int X1, int Y1, int X2, int Y2, const wchar_t *HelpTopic,
		struct FarDialogItem *Item, unsigned int ItemsNumber, DWORD Reserved, DWORD Flags, FARWINDOWPROC Proc,
		LONG_PTR Param);
int WINAPI FarDialogRun(HANDLE hDlg);
void WINAPI FarDialogFree(HANDLE hDlg);
// Функция обработки диалога по умолчанию
LONG_PTR WINAPI FarDefDlgProc(HANDLE hDlg, int Msg, int Param1, LONG_PTR Param2);
// Посылка сообщения диалогу
LONG_PTR WINAPI FarSendDlgMessage(HANDLE hDlg, int Msg, int Param1, LONG_PTR Param2);

int WINAPI farPluginsControl(HANDLE hHandle, int Command, int Param1, LONG_PTR Param2);

int WINAPI farFileFilterControl(HANDLE hHandle, int Command, int Param1, LONG_PTR Param2);

int WINAPI farRegExpControl(HANDLE hHandle, int Command, LONG_PTR Param);

DWORD WINAPI farGetCurrentDirectory(DWORD Size, wchar_t *Buffer);

enum ExceptFunctionsType
{
	EXCEPT_KERNEL = -1,
	EXCEPT_SETSTARTUPINFO,
	EXCEPT_GETVIRTUALFINDDATA,
	EXCEPT_OPENPLUGIN,
	EXCEPT_OPENFILEPLUGIN,
	EXCEPT_CLOSEPLUGIN,
	EXCEPT_GETPLUGININFO,
	EXCEPT_GETOPENPLUGININFO,
	EXCEPT_GETFINDDATA,
	EXCEPT_FREEFINDDATA,
	EXCEPT_FREEVIRTUALFINDDATA,
	EXCEPT_SETDIRECTORY,
	EXCEPT_GETLINKTARGET,
	EXCEPT_GETFILES,
	EXCEPT_PUTFILES,
	EXCEPT_DELETEFILES,
	EXCEPT_MAKEDIRECTORY,
	EXCEPT_PROCESSHOSTFILE,
	EXCEPT_SETFINDLIST,
	EXCEPT_CONFIGURE,
	EXCEPT_EXITFAR,
	EXCEPT_MAYEXITFAR,
	EXCEPT_PROCESSKEY,
	EXCEPT_PROCESSEVENT,
	EXCEPT_PROCESSEDITOREVENT,
	EXCEPT_COMPARE,
	EXCEPT_PROCESSEDITORINPUT,
	EXCEPT_MINFARVERSION,
	EXCEPT_PROCESSVIEWEREVENT,
	EXCEPT_PROCESSVIEWERINPUT,
	EXCEPT_PROCESSDIALOGEVENT,
	EXCEPT_PROCESSSYNCHROEVENT,
	EXCEPT_ANALYSE,
	EXCEPT_GETCUSTOMDATA,
	EXCEPT_FREECUSTOMDATA,
#if defined(PROCPLUGINMACROFUNC)
	EXCEPT_PROCESSMACROFUNC,
#endif
};
