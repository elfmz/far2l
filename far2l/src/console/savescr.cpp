/*
savescr.cpp

Сохраняем и восстанавливааем экран кусками и целиком
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

#include "headers.hpp"

#include "savescr.hpp"
#include "colors.hpp"
#include "syslog.hpp"
#include "interf.hpp"
#include "farcolors.hpp"
#include "console.hpp"

SaveScreen::SaveScreen()
{
	_OT(SysLog(L"[%p] SaveScreen::SaveScreen()", this));
	SaveArea(0, 0, ScrX, ScrY);
}

SaveScreen::SaveScreen(int X1, int Y1, int X2, int Y2)
{
	_OT(SysLog(L"[%p] SaveScreen::SaveScreen(X1=%i,Y1=%i,X2=%i,Y2=%i)", this, X1, Y1, X2, Y2));

	X1 = Min(static_cast<int>(ScrX), Max(0, X1));
	X2 = Min(static_cast<int>(ScrX), Max(0, X2));
	Y1 = Min(static_cast<int>(ScrY), Max(0, Y1));
	Y2 = Min(static_cast<int>(ScrY), Max(0, Y2));

	SaveArea(X1, Y1, X2, Y2);
}

SaveScreen::~SaveScreen()
{
	if (!ScreenBuf)
		return;

	_OT(SysLog(L"[%p] SaveScreen::~SaveScreen()", this));
	RestoreArea();
	delete[] ScreenBuf;
}

void SaveScreen::Discard()
{
	if (!ScreenBuf)
		return;

	delete[] ScreenBuf;
	ScreenBuf = nullptr;
}

void SaveScreen::RestoreArea(int RestoreCursor)
{
	if (!ScreenBuf) {
		fprintf(stderr, "SaveScreen::RestoreArea: no ScreenBuf\n");
		return;
	}
	fprintf(stderr, "*** SaveScreen::RestoreArea %d %d %d %d %d %d\n", X1, Y1, X2, Y2, vWidth, vHeight);
	PutText(X1, Y1, X2, Y2, ScreenBuf);
	if (vWidth > 0 && vWidth > X2 + 1 - X1) {
		SetScreen(X2, Y1, X1 + vWidth - 1, Y2, L' ', 0);
	}
	if (vHeight > 0 && vHeight > Y2 + 1 - Y1) {
		SetScreen(X1, Y2, X2, Y1 + vHeight - 1, L' ', 0);
	}
	if (vWidth > 0 && vWidth > X2 + 1 - X1 && vHeight > 0 && vHeight > Y2 + 1 - Y1) {
		SetScreen(X2, Y2, X1 + vWidth - 1, Y1 + vHeight - 1, L' ', 0);
	}

	if (RestoreCursor) {
		SetCursorType(CurVisible, CurSize);
		MoveCursor(CurPosX, CurPosY);
	}
}

void SaveScreen::SaveArea(int X1, int Y1, int X2, int Y2)
{
	SaveScreen::X1 = X1;
	SaveScreen::Y1 = Y1;
	SaveScreen::X2 = X2;
	SaveScreen::Y2 = Y2;
	vWidth = vHeight = -1;

	ScreenBuf = new (std::nothrow) CHAR_INFO[ScreenBufCharCount()];

	if (!ScreenBuf)
		return;

	GetText(X1, Y1, X2, Y2, ScreenBuf, ScreenBufCharCount() * sizeof(CHAR_INFO));
	GetCursorPos(CurPosX, CurPosY);
	GetCursorType(CurVisible, CurSize);
}

void SaveScreen::SaveArea()
{
	if (!ScreenBuf)
		return;

	GetText(X1, Y1, X2, Y2, ScreenBuf, ScreenBufCharCount() * sizeof(CHAR_INFO));
	GetCursorPos(CurPosX, CurPosY);
	GetCursorType(CurVisible, CurSize);
}

void SaveScreen::AppendArea(SaveScreen *NewArea)
{
	CHAR_INFO *Buf = ScreenBuf, *NewBuf = NewArea->ScreenBuf;

	if (!Buf || !NewBuf)
		return;

	for (int X = X1; X <= X2; X++)
		if (X >= NewArea->X1 && X <= NewArea->X2)
			for (int Y = Y1; Y <= Y2; Y++)
				if (Y >= NewArea->Y1 && Y <= NewArea->Y2)
					Buf[X - X1 + (X2 - X1 + 1) * (Y - Y1)] =
							NewBuf[X - NewArea->X1 + (NewArea->X2 - NewArea->X1 + 1) * (Y - NewArea->Y1)];
}

void SaveScreen::VirtualResize(int W, int H)
{
	vWidth = W;
	vHeight = H;
}

int SaveScreen::ScreenBufCharCount()
{
	return (X2 - X1 + 1) * (Y2 - Y1 + 1);
}

void SaveScreen::CharCopy(PCHAR_INFO ToBuffer, PCHAR_INFO FromBuffer, int Count)
{
	memcpy(ToBuffer, FromBuffer, Count * sizeof(CHAR_INFO));
}

void SaveScreen::CleanupBuffer(PCHAR_INFO Buffer, size_t BufSize)
{
	uint64_t Attr = FarColorToReal(COL_COMMANDLINEUSERSCREEN);

	for (size_t i = 0; i < BufSize; i++) {
		Buffer[i].Attributes = Attr;
		Buffer[i].Char.UnicodeChar = L' ';
	}
}

void SaveScreen::DumpBuffer(const wchar_t *Title)
{
	SaveScreenDumpBuffer(Title, GetBufferAddress(), X1, Y1, X2, Y2, nullptr);
}
