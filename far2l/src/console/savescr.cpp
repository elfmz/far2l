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
	if (!ScreenBuf.empty())
		RestoreArea();

	_OT(SysLog(L"[%p] SaveScreen::~SaveScreen()", this));
}

void SaveScreen::Discard()
{
	ScreenBuf.clear();
	X2 = X1;
	Y2 = Y1;
}

void SaveScreen::RestoreArea(int RestoreCursor)
{
	fprintf(stderr, "SaveScreen::RestoreArea %d %d %d %d %s\n", X1, Y1, X2, Y2, ScreenBuf.empty() ? "EMPTY" : "");
	if (!ScreenBuf.empty()) {
		PutText(X1, Y1, X2, Y2, ScreenBuf.data());

		if (RestoreCursor) {
			SetCursorType(CurVisible, CurSize);
			MoveCursor(CurPosX, CurPosY);
		}
	}
}

void SaveScreen::SaveArea(int nX1, int nY1, int nX2, int nY2)
{
	if (nX2 < nX1 || nY2 < nY1) {
		fprintf(stderr, "SaveScreen::SaveArea %d %d %d %d - BAD\n", nX1, nY1, nX2, nY2);
		return;
	}

	fprintf(stderr, "SaveScreen::SaveArea %d %d %d %d\n", nX1, nY1, nX2, nY2);
	X1 = nX1;
	Y1 = nY1;
	X2 = nX2;
	Y2 = nY2;

	ScreenBuf.resize((X2 - X1 + 1) * (Y2 - Y1 + 1));
	GetText(X1, Y1, X2, Y2, ScreenBuf.data(), ScreenBuf.size() * sizeof(CHAR_INFO));
	GetCursorPos(CurPosX, CurPosY);
	GetCursorType(CurVisible, CurSize);
}

void SaveScreen::AppendArea(SaveScreen *NewArea)
{
	if (ScreenBuf.empty() || NewArea->ScreenBuf.empty())
		return;

	CHAR_INFO *Buf = ScreenBuf.data(), *NewBuf = NewArea->ScreenBuf.data();

	for (int X = X1; X <= X2; X++)
		if (X >= NewArea->X1 && X <= NewArea->X2)
			for (int Y = Y1; Y <= Y2; Y++)
				if (Y >= NewArea->Y1 && Y <= NewArea->Y2)
					Buf[X - X1 + (X2 - X1 + 1) * (Y - Y1)] =
							NewBuf[X - NewArea->X1 + (NewArea->X2 - NewArea->X1 + 1) * (Y - NewArea->Y1)];
}

const CHAR_INFO &SaveScreen::Read(int X, int Y) const
{
	if (X < X1 || X > X2 || Y < Y1 || Y > Y2 || ScreenBuf.empty()) {
		fprintf(stderr, "SaveScreen::Read(%d, %d) - out of bounds: %d %d %d %d\n", X, Y, X1, Y1, X2, Y2);
		static const CHAR_INFO s_dummy_out{};
		return s_dummy_out;
	}

	return ScreenBuf[ size_t(Y - Y1) * size_t(X2 - X1 + 1) + size_t(X - X1) ];
}
