/*
scrsaver.cpp

ScreenSaver
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

#include "colors.hpp"
#include "chgprior.hpp"
#include "savescr.hpp"
#include "manager.hpp"
#include "interf.hpp"
#include "keyboard.hpp"
#include "config.hpp"
#include "scrsaver.hpp"
#include "console.hpp"
#include <time.h>

#define randomize() srand(time(NULL))
#define random(x)   (rand() % (x))

enum
{
	STAR_NONE,
	STAR_NORMAL,
	STAR_PLANET,
	STAR_EGGSTRA
};

static struct
{
	int X;
	int Y;
	int Type;
	int Color;
	int Speed;
} Star[16];

static wchar_t StarSymbol[3][2] = {
		{0x25cf, 0x0000},		// ●
		{0x2022, 0x0000},		// •
		{0x00B7, 0x0000}		// ·
};

static wchar_t PlanetSymbol[3][2] = {
		{0x25c9, 0x0000},		// ◉
		{0x25ce, 0x0000},		// ◎
		{0x00B0, 0x0000},		// °
};

static wchar_t EggstraSymbol[3][2] = {
#if defined(__linux__)
		{0x1F427, 0x0000},		// 🐧
		{0x1F427, 0x0000},		// 🐧 // 0x1F425🐥
		{0x1F427, 0x0000},		// 🐧 // 0x1F423🐣

#elif defined(__APPLE__)
		{0x1F34E, 0x0000},		// 🍎
		{0x1F34E, 0x0000},		// 🍎
		{0x1F34E, 0x0000},		// 🍎

#elif defined(__FreeBSD__)
		{0x1F608, 0x0000},		// 😈
		{0x1F608, 0x0000},		// 😈
		{0x1F608, 0x0000},		// 😈

#elif defined(__DragonFly__)
		{0x1F608, 0x0000},		// 😈
		{0x1F608, 0x0000},		// 😈
		{0x1F608, 0x0000},		// 😈

#else
		{0x1F95A, 0x0000},		// 🥚
		{0x1F95A, 0x0000},		// 🥚
		{0x1F95A, 0x0000},		// 🥚
#endif
};

static void ShowSaver(int Step)
{
	for (size_t I = 0; I < ARRAYSIZE(Star); I++)
		if (Star[I].Type != STAR_NONE && !(Step % Star[I].Speed)) {
			SetColor(F_LIGHTCYAN | B_BLACK);
			GotoXY(Star[I].X / 100, Star[I].Y / 100);
			Text(L" ");
			int dx = Star[I].X / 100 - ScrX / 2;
			Star[I].X+= dx * 10 + ((dx < 0) ? -1 : 1);
			int dy = Star[I].Y / 100 - ScrY / 2;
			Star[I].Y+= dy * 10 + ((dy < 0) ? -1 : 1);

			if (Star[I].X < 0 || Star[I].X / 100 > ScrX || Star[I].Y < 0 || Star[I].Y / 100 > ScrY)
				Star[I].Type = STAR_NONE;
			else {
				SetColor(Star[I].Color | B_BLACK);
				GotoXY(Star[I].X / 100, Star[I].Y / 100);

				if (abs(dx) > 3 * ScrX / 8 || abs(dy) > 3 * ScrY / 8) {
					if (Star[I].Type == STAR_NORMAL) {
						SetColor(F_WHITE | B_BLACK);
						Text(StarSymbol[0]);
					} else {
						SetColor(Star[I].Color | FOREGROUND_INTENSITY | B_BLACK);
						Text((Star[I].Type == STAR_PLANET) ? PlanetSymbol[0] : EggstraSymbol[0]);
					}
				} else if (abs(dx) > ScrX / 7 || abs(dy) > ScrY / 7) {
					if (Star[I].Type == STAR_NORMAL) {
						if (abs(dx) > ScrX / 4 || abs(dy) > ScrY / 4)
							SetColor(F_LIGHTCYAN | B_BLACK);
						else
							SetColor(F_CYAN | B_BLACK);

						Text(StarSymbol[1]);
					} else {
						SetColor(Star[I].Color | FOREGROUND_INTENSITY | B_BLACK);
						Text((Star[I].Type == STAR_PLANET) ? PlanetSymbol[1] : EggstraSymbol[1]);
					}
				} else {
					if (Star[I].Type == STAR_NORMAL) {
						SetColor(F_CYAN | B_BLACK);
						Text(StarSymbol[2]);
					} else {
						SetColor(Star[I].Color | B_BLACK);
						Text((Star[I].Type == STAR_PLANET) ? PlanetSymbol[2] : EggstraSymbol[2]);
					}
				}
			}
		}

	for (size_t I = 0; I < ARRAYSIZE(Star); I++)
		if (Star[I].Type == STAR_NONE) {
			static const int Colors[] = {F_MAGENTA, F_RED, F_BLUE};
			int rnd_type = random(7700);
			if (rnd_type == 0)
				Star[I].Type = STAR_EGGSTRA;
			else if (rnd_type < 300)
				Star[I].Type = STAR_PLANET;
			else
				Star[I].Type = STAR_NORMAL;
			Star[I].X = (ScrX / 2 - ScrX / 4 + random(ScrX / 2)) * 100;
			Star[I].Y = (ScrY / 2 - ScrY / 4 + random(ScrY / 2)) * 100;
			Star[I].Color = Colors[random(ARRAYSIZE(Colors))];
			Star[I].Speed = (Star[I].Type == STAR_PLANET) ? 1 : 2;
			break;
		}
}

int ScreenSaver(int EnableExit)
{
	INPUT_RECORD rec;
	clock_t WaitTime;

	if (ScreenSaverActive)
		return 1;

	ChangePriority ChPriority(ChangePriority::IDLE);

	for (WaitTime = GetProcessUptimeMSec(); GetProcessUptimeMSec() - WaitTime < 500;) {
		if (PeekInputRecord(&rec))
			return 1;

		WINPORT(Sleep)(100);
	}

	ScreenSaverActive = TRUE;
	CONSOLE_CURSOR_INFO CursorInfo;
	Console.GetCursorInfo(CursorInfo);
	{
		SaveScreen SaveScr;
		SetCursorType(0, 10);
		randomize();
		SetScreen(0, 0, ScrX, ScrY, L' ', F_LIGHTGRAY | B_BLACK);

		for (size_t I = 0; I < ARRAYSIZE(Star); I++) {
			Star[I].Type = STAR_NONE;
			Star[I].Color = 0;
		}

		int Step = 0;

		while (!PeekInputRecord(&rec)) {
			if (EnableExit && CheckForInactivityExit())
				return 0;

			WINPORT(Sleep)(50);
			ShowSaver(Step++);
		}
	}
	SetCursorType(CursorInfo.bVisible != FALSE, CursorInfo.dwSize);
	ScreenSaverActive = FALSE;
	if (!WinPortTesting())
		FlushInputBuffer();
	StartIdleTime = GetProcessUptimeMSec();
	return 1;
}
