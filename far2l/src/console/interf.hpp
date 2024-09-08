#pragma once

/*
interf.hpp

Консольные функции ввода-вывода
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

#include "strmix.hpp"
#include "lang.hpp"

extern WCHAR Oem2Unicode[];
extern WCHAR BoxSymbols[];
extern COORD CurSize;
extern SHORT ScrX, ScrY;
extern SHORT PrevScrX, PrevScrY;
extern DWORD InitialConsoleMode;

// типы рамок
enum
{
	NO_BOX,
	SINGLE_BOX,
	SHORT_SINGLE_BOX,
	DOUBLE_BOX,
	SHORT_DOUBLE_BOX
};

void ShowTime(int ShowAlways);

/*
	$ 14.02.2001 SKV
	Инитить ли палитру default значениями.
	По умолчанию - да.
	С 0 используется для ConsoleDetach.
*/
void InitConsole();
void CloseConsole();
void SetFarConsoleMode(BOOL SetsActiveBuffer = FALSE);
void ChangeConsoleMode(int Mode);
void FlushInputBuffer();
void ToggleVideoMode();
void GetVideoMode(COORD &Size);
void GenerateWINDOW_BUFFER_SIZE_EVENT(int Sx = -1, int Sy = -1, bool Damaged = false);
void SaveConsoleWindowRect();
void RestoreConsoleWindowRect();

void GotoXY(int X, int Y);
int WhereX();
int WhereY();
void MoveCursor(int X, int Y);
void GetCursorPos(SHORT &X, SHORT &Y);
void SetCursorType(bool Visible, DWORD Size);
void SetInitialCursorType();
void GetCursorType(bool &Visible, DWORD &Size);
void MoveRealCursor(int X, int Y);
void GetRealCursorPos(SHORT &X, SHORT &Y);
void ScrollScreen(int Count);

void Text(int X, int Y, uint64_t Color, const WCHAR *Str);
void Text(int X, int Y, uint64_t Color, const WCHAR *Str, size_t Length);
void Text(const WCHAR Ch, uint64_t Color, size_t Length);
void Text(const WCHAR Ch, size_t Length);
void Text(const WCHAR *Str, size_t Length = (size_t)-1);
void TextEx(const WCHAR *Str, size_t Length = (size_t)-1);

void Text(FarLangMsg MsgId);
void VText(const WCHAR *Str);
void HiText(const WCHAR *Str, uint64_t HiColor, int isVertText = 0);
void mprintf(const wchar_t *fmt, ...);
void vmprintf(const wchar_t *fmt, ...);
void PutText(int X1, int Y1, int X2, int Y2, const void *Src);
void GetText(int X1, int Y1, int X2, int Y2, void *Dest, int DestSize);
void BoxText(wchar_t Chr);
void BoxText(const wchar_t *Str, int IsVert = 0);

void SetScreen(int X1, int Y1, int X2, int Y2, wchar_t Ch, uint64_t Color);
void MakeShadow(int X1, int Y1, int X2, int Y2, SaveScreen *ss = NULL);
void ChangeBlockColor(int X1, int Y1, int X2, int Y2, uint64_t Color);
void SetColor(uint64_t Color, bool ApplyToConsole = false);
void SetFarColor(uint16_t Color, bool ApplyToConsole = false);
void FarTrueColorFromRGB(FarTrueColor &out, DWORD rgb, bool used);
void FarTrueColorFromRGB(FarTrueColor &out, DWORD rgb);
void FarTrueColorFromAttributes(FarTrueColorForeAndBack &TFB, DWORD64 Attrs);
void FarTrueColorToAttributes(DWORD64 &Attrs, const FarTrueColorForeAndBack &TFB);
DWORD64 ComposeColor(WORD BaseColor, const FarTrueColorForeAndBack *TFB);
void ComposeAndSetColor(WORD BaseColor, const FarTrueColorForeAndBack *TFB, bool ApplyToConsole = false);
void ClearScreen(uint64_t Color);
uint64_t GetColor();

void Box(int x1, int y1, int x2, int y2, uint64_t Color, int Type);
void ScrollBar(int X1, int Y1, int Length, unsigned int Current, unsigned int Total);
bool ScrollBarRequired(UINT Length, UINT64 ItemsCount);
bool ScrollBarEx(UINT X1, UINT Y1, UINT Length, UINT64 TopItem, UINT64 ItemsCount);
void DrawLine(int Length, int Type, const wchar_t *UserSep = nullptr);
#define ShowSeparator(Length, Type)              DrawLine(Length, Type)
#define ShowUserSeparator(Length, Type, UserSep) DrawLine(Length, Type, UserSep)
WCHAR *MakeSeparator(int Length, WCHAR *DestStr, int Type = 1, const wchar_t *UserSep = nullptr);

void InitRecodeOutTable();

int WINAPI TextToCharInfo(const char *Text, WORD Attr, CHAR_INFO *CharInfo, int Length, DWORD Reserved);

inline void SetVidChar(CHAR_INFO &CI, COMP_CHAR Chr)
{
	CI.Char.UnicodeChar = (Chr < L'\x20' || Chr == L'\x7f') ? Oem2Unicode[Chr] : Chr;
}

int HiStrCellsCount(const wchar_t *Str);
int HiFindRealPos(const wchar_t *Str, int Pos, BOOL ShowAmp);
int HiFindNextVisualPos(const wchar_t *Str, int Pos, int Direct);
FARString &HiText2Str(FARString &strDest, const wchar_t *Str);
#define RemoveHighlights(Str) RemoveChar(Str, L'&')

bool CheckForInactivityExit();
void CheckForPendingCtrlHandleEvent();
