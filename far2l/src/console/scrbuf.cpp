/*
scrbuf.cpp

Буферизация вывода на экран, весь вывод идет через этот буфер
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

#include "scrbuf.hpp"
#include "colors.hpp"
#include "ctrlobj.hpp"
#include "syslog.hpp"
#include "interf.hpp"
#include "farcolors.hpp"
#include "config.hpp"
#include "DList.hpp"
#include "console.hpp"
#include "savescr.hpp"

#include <new>

enum
{
	SBFLAGS_FLUSHED        = 0x00000001,
	SBFLAGS_FLUSHEDCURPOS  = 0x00000002,
	SBFLAGS_FLUSHEDCURTYPE = 0x00000004,
	SBFLAGS_USESHADOW      = 0x00000008,
};

// #if defined(SYSLOG_OT)
//  #define DIRECT_SCREEN_OUT
// #endif

#ifdef DIRECT_RT
extern int DirectRT;
#endif

ScreenBuf ScrBuf;

static bool AreSameCharInfoBuffers(const CHAR_INFO *left, const CHAR_INFO *right, size_t count)
{	// use this instead of memcmp cuz it can produce wrong results due to uninitialized alignment gaps
	for (; count; --count, ++left, ++right) {
		if (left->Char.UnicodeChar != right->Char.UnicodeChar)
			return false;
		if (left->Attributes != right->Attributes)
			return false;
	}
	return true;
}

ScreenBuf::ScreenBuf()
	:
	Buf(nullptr),
	Shadow(nullptr),
	MacroCharUsed(false),
	ElevationCharUsed(false),
	BufX(0),
	BufY(0),
	CurX(0),
	CurY(0),
	CurVisible(false),
	CurSize(0),
	LockCount(0)
{
	SBFlags.Set(SBFLAGS_FLUSHED | SBFLAGS_FLUSHEDCURPOS | SBFLAGS_FLUSHEDCURTYPE);
}

ScreenBuf::~ScreenBuf()
{
	if (Buf)
		delete[] Buf;

	if (Shadow)
		delete[] Shadow;
}

void ScreenBuf::AllocBuf(int X, int Y)
{
	CriticalSectionLock Lock(CS);

	if (X == BufX && Y == BufY)
		return;

	if (Buf) {
		delete[] Buf;
		Buf = nullptr;
	}

	if (Shadow) {
		delete[] Shadow;
		Shadow = nullptr;
	}

	if (X <= 0 || Y <= 0) {
		BufX = 0;
		BufY = 0;
		return;
	}

	unsigned Cnt = X * Y;
	Buf = new (std::nothrow) CHAR_INFO[Cnt]();
	Shadow = new (std::nothrow) CHAR_INFO[Cnt]();

	if (!Buf || !Shadow) {
		fprintf(stderr, "FATAL: Failed to allocate screen buffer (%d x %d)\n", X, Y);
		delete[] Buf;
		delete[] Shadow;
		Buf = nullptr;
		Shadow = nullptr;
		BufX = 0;
		BufY = 0;
		abort();
	}

	BufX = X;
	BufY = Y;
	//ResetShadow();
}

/*
	Заполнение виртуального буфера значением из консоли.
*/
void ScreenBuf::FillBuf()
{
	CriticalSectionLock Lock(CS);
	if (!Buf || !Shadow)
		return;
	COORD BufferSize = {BufX, BufY}, BufferCoord = {0, 0};
	SMALL_RECT ReadRegion = {0, 0, (SHORT)(BufX - 1), (SHORT)(BufY - 1)};
	Console.ReadOutput(*Buf, BufferSize, BufferCoord, ReadRegion);
	memcpy(Shadow, Buf, BufX * BufY * sizeof(CHAR_INFO));
	SBFlags.Set(SBFLAGS_USESHADOW);
	COORD CursorPosition;
	Console.GetCursorPosition(CursorPosition);
	CurX = CursorPosition.X;
	CurY = CursorPosition.Y;
}

void ScreenBuf::FillBufWithRecompose(HANDLE Console)
{
	COORD BufferSize = {BufX, BufY};
	SMALL_RECT ReadRegion = {0, 0, (SHORT)(BufX - 1), (SHORT)(BufY - 1)};
	CONSOLE_SCREEN_BUFFER_INFO csbi{};
	SBFlags.Clear(SBFLAGS_FLUSHED);
	if (WINPORT(GetConsoleScreenBufferInfo)(Console, &csbi) && (csbi.dwSize.X != BufX || csbi.dwSize.Y != BufY)) {
		// in case size different - fork&resize original console to enable lines recompisition to its job
		ConsoleForkScope TmpConsole(Console);
		if (TmpConsole) {
			WINPORT(SetConsoleScreenBufferSize)(TmpConsole.Handle(), BufferSize);
			WINPORT(ReadConsoleOutput)(TmpConsole.Handle(), Buf, BufferSize, COORD{0,0}, &ReadRegion);
			TmpConsole.Discard();
			return;
		}
	}
	WINPORT(ReadConsoleOutput)(Console, Buf, BufferSize, COORD{0,0}, &ReadRegion);
}

/*
	Записать Text в виртуальный буфер
*/
void ScreenBuf::Write(int X, int Y, const CHAR_INFO *Text, int TextLength)
{
	CriticalSectionLock Lock(CS);

	if (X < 0) {
		Text-= X;
		TextLength = Max(0, TextLength + X);
		X = 0;
	}

	if (!Buf || X >= BufX || Y >= BufY || !TextLength || Y < 0) {
		return;
	}

	if (X + TextLength >= BufX) {
		TextLength = BufX - X;	//??
		if (TextLength <= 0) {
			return;
		}
	}

	CHAR_INFO *PtrBuf = Buf + Y * BufX + X;
	for (int i = 0; i < TextLength; i++) {
		SetVidChar(PtrBuf[i], Text[i].Char.UnicodeChar);
		PtrBuf[i].Attributes = Text[i].Attributes;
	}

	SBFlags.Clear(SBFLAGS_FLUSHED);
#ifdef DIRECT_SCREEN_OUT
	Flush();
#elif defined(DIRECT_RT)
	if (DirectRT)
		Flush();

#endif
}

void ScreenBuf::SetExplicitLineBreak(int Y)
{
	if (!Buf || Y < 0 || Y >= BufY)
		return;

	int LastNonSpace = 0;
	CHAR_INFO *PtrBuf = Buf + Y * BufX;
	for (int i = 0; i < BufX; i++) {
		if ((PtrBuf[i].Char.UnicodeChar != ' ' && PtrBuf[i].Char.UnicodeChar != 0)
				|| (i != 0 && (PtrBuf[i].Attributes & BACKGROUND_RGB) != (PtrBuf[i - 1].Attributes & BACKGROUND_RGB))) {
			LastNonSpace = i;
		}
		PtrBuf[i].Attributes &= ~EXPLICIT_LINE_BREAK;
	}
	PtrBuf[LastNonSpace].Attributes |= EXPLICIT_LINE_BREAK;
}

/*
	Читать блок из виртуального буфера.
*/
void ScreenBuf::Read(int X1, int Y1, int X2, int Y2, CHAR_INFO *Text, int MaxTextLength)
{
	CriticalSectionLock Lock(CS);
	if (!Buf)
		return;

	int Width = X2 - X1 + 1;
	int Height = Y2 - Y1 + 1;

	for (int I = 0, Idx = 0; I < Height; I++, Idx+= Width)
		memcpy(Text + Idx, Buf + (Y1 + I) * BufX + X1,
				Min((int)sizeof(CHAR_INFO) * Width, MaxTextLength));
}

void ScreenBuf::ApplyShadow(int X1, int Y1, int X2, int Y2, SaveScreen *ss)
{
	CriticalSectionLock Lock(CS);
	if (!Buf)
		return;

	int Width = X2 - X1 + 1;
	int Height = Y2 - Y1 + 1;

	for (int I = 0; I < Height; I++) {
		CHAR_INFO *DstBuf = Buf + (Y1 + I) * BufX + X1;
		const CHAR_INFO *SrcBuf = ss ? &ss->Read(X1, Y1 + I) : DstBuf;

		for (int J = 0; J < Width; J++, ++DstBuf, ++SrcBuf) {
			uint64_t attr = SrcBuf->Attributes;
			uint8_t cc = attr & 0x07;
			DstBuf->Attributes = ((attr & 0xFEFEFEFEFEFE0000ULL) >> 1) |
									(attr & 0x000000000000FF00ULL) | (cc ? cc : 8);
		}
	}

#ifdef DIRECT_SCREEN_OUT
	Flush();
#elif defined(DIRECT_RT)

	if (DirectRT)
		Flush();

#endif
}


/*
	Изменить значение цветовых атрибутов в соответствии с маской
	(в основном применяется для "создания" тени)
*/
void ScreenBuf::ApplyColorMask(int X1, int Y1, int X2, int Y2, DWORD64 ColorMask)
{
	CriticalSectionLock Lock(CS);
	if (!Buf)
		return;

	int Width = X2 - X1 + 1;
	int Height = Y2 - Y1 + 1;

	for (int I = 0; I < Height; I++) {
		CHAR_INFO *PtrBuf = Buf + (Y1 + I) * BufX + X1;

		for (int J = 0; J < Width; J++, ++PtrBuf) {
			if (!(PtrBuf->Attributes&= ~ColorMask))
				PtrBuf->Attributes = 0x08;
		}
	}

#ifdef DIRECT_SCREEN_OUT
	Flush();
#elif defined(DIRECT_RT)

	if (DirectRT)
		Flush();

#endif
}

/*
	Непосредственное изменение цветовых атрибутов
*/
void ScreenBuf::ApplyColor(int X1, int Y1, int X2, int Y2, DWORD64 Color)
{
	CriticalSectionLock Lock(CS);
	if (!Buf)
		return;

	if (X1 <= ScrX && Y1 <= ScrY && X2 >= 0 && Y2 >= 0) {
		X1 = Max(0, X1);
		X2 = Min(static_cast<int>(ScrX), X2);
		Y1 = Max(0, Y1);
		Y2 = Min(static_cast<int>(ScrY), Y2);

		int Width = X2 - X1 + 1;
		int Height = Y2 - Y1 + 1;

		for (int I = 0; I < Height; I++) {
			CHAR_INFO *PtrBuf = Buf + (Y1 + I) * BufX + X1;

			for (int J = 0; J < Width; J++, ++PtrBuf)
				PtrBuf->Attributes = Color;
		}

#ifdef DIRECT_SCREEN_OUT
		Flush();
#elif defined(DIRECT_RT)

		if (DirectRT)
			Flush();

#endif
	}
}

/*
	Непосредственное изменение цветовых атрибутов с заданным цветом исключением
*/
void ScreenBuf::ApplyColor(int X1, int Y1, int X2, int Y2, DWORD64 Color, DWORD64 ExceptColor)
{
	CriticalSectionLock Lock(CS);
	if (!Buf)
		return;

	if (X1 <= ScrX && Y1 <= ScrY && X2 >= 0 && Y2 >= 0) {
		X1 = Max(0, X1);
		X2 = Min(static_cast<int>(ScrX), X2);
		Y1 = Max(0, Y1);
		Y2 = Min(static_cast<int>(ScrY), Y2);

		for (int I = 0; I < Y2 - Y1 + 1; I++) {
			CHAR_INFO *PtrBuf = Buf + (Y1 + I) * BufX + X1;

			for (int J = 0; J < X2 - X1 + 1; J++, ++PtrBuf)
				if (PtrBuf->Attributes != ExceptColor)
					PtrBuf->Attributes = Color;
		}

#ifdef DIRECT_SCREEN_OUT
		Flush();
#elif defined(DIRECT_RT)

		if (DirectRT)
			Flush();

#endif
	}
}

/*
	Закрасить прямоугольник символом Ch и цветом Color
*/
void ScreenBuf::FillRect(int X1, int Y1, int X2, int Y2, WCHAR Ch, DWORD64 Color)
{
	CriticalSectionLock Lock(CS);
	if (!Buf)
		return;

	int Width = X2 - X1 + 1;
	int Height = Y2 - Y1 + 1;
	CHAR_INFO CI;
	CI.Attributes = Color;
	SetVidChar(CI, Ch);

	for (int I = 0; I < Height; I++) {
		CHAR_INFO *PtrBuf = Buf + (Y1 + I) * BufX + X1;
		for (int J = 0; J < Width; J++, ++PtrBuf)
			*PtrBuf = CI;
	}

	SBFlags.Clear(SBFLAGS_FLUSHED);
#ifdef DIRECT_SCREEN_OUT
	Flush();
#elif defined(DIRECT_RT)

	if (DirectRT)
		Flush();

#endif
}

/*
	"Сбросить" виртуальный буфер на консоль
*/
void ScreenBuf::Flush()
{
	ConsoleRepaintsDeferScope crds(NULL);

	CriticalSectionLock Lock(CS);

	if (!Buf || !Shadow)
		return;

	if (!LockCount) {
		if (CtrlObject && (CtrlObject->Macro.IsRecording() || CtrlObject->Macro.IsExecuting())) {
			MacroChar = Buf[0];
			MacroCharUsed = true;

			if (CtrlObject->Macro.IsRecording()) {
				Buf[0].Char.UnicodeChar = L'R';
				Buf[0].Attributes = 0xCF;
			} else {
				Buf[0].Char.UnicodeChar = L'P';
				Buf[0].Attributes = 0x2F;
			}
		}

		if (!SBFlags.Check(SBFLAGS_FLUSHEDCURTYPE) && !CurVisible) {
			CONSOLE_CURSOR_INFO cci = {CurSize, CurVisible};
			Console.SetCursorInfo(cci);
			SBFlags.Set(SBFLAGS_FLUSHEDCURTYPE);
		}

		if (!SBFlags.Check(SBFLAGS_FLUSHED)) {
			SBFlags.Set(SBFLAGS_FLUSHED);

			if (WaitInMainLoop && Opt.Clock && !ProcessShowClock) {
				ShowTime(FALSE);
			}

			DList<SMALL_RECT> WriteList;
			bool Changes = false;

			if (SBFlags.Check(SBFLAGS_USESHADOW)) {
				PCHAR_INFO PtrBuf = Buf, PtrShadow = Shadow;

				{
					bool Started = false;
					SMALL_RECT WriteRegion = {(SHORT)(BufX - 1), (SHORT)(BufY - 1), 0, 0};

					for (SHORT I = 0; I < BufY; I++) {
						for (SHORT J = 0; J < BufX; J++, ++PtrBuf, ++PtrShadow) {
							if (!AreSameCharInfoBuffers(PtrBuf, PtrShadow, 1)) {
								WriteRegion.Left = Min(WriteRegion.Left, J);
								WriteRegion.Top = Min(WriteRegion.Top, I);
								WriteRegion.Right = Max(WriteRegion.Right, J);
								WriteRegion.Bottom = Max(WriteRegion.Bottom, I);
								Changes = true;
								Started = true;
							} else if (Started && I > WriteRegion.Bottom && J >= WriteRegion.Left) {
								bool Merge = false;
								PSMALL_RECT Last = WriteList.Last();

								if (Last) {
#define MAX_DELTA 5

									if (WriteRegion.Top - 1 == Last->Bottom
											&& ((WriteRegion.Left >= Last->Left
														&& WriteRegion.Left - Last->Left < MAX_DELTA)
													|| (Last->Right >= WriteRegion.Right
															&& Last->Right - WriteRegion.Right
																	< MAX_DELTA))) {
										Last->Bottom = WriteRegion.Bottom;
										Last->Left = Min(Last->Left, WriteRegion.Left);
										Last->Right = Max(Last->Right, WriteRegion.Right);
										Merge = true;
									}
								}

								if (!Merge)
									WriteList.Push(&WriteRegion);

								WriteRegion.Left = BufX - 1;
								WriteRegion.Top = BufY - 1;
								WriteRegion.Right = 0;
								WriteRegion.Bottom = 0;
								Started = false;
							}
						}
					}

					if (Started) {
						WriteList.Push(&WriteRegion);
					}
				}
			} else {
				Changes = true;
				SMALL_RECT WriteRegion = {0, 0, (SHORT)(BufX - 1), (SHORT)(BufY - 1)};
				WriteList.Push(&WriteRegion);
			}

			if (Changes) {
				for (PSMALL_RECT PtrRect = WriteList.First(); PtrRect; PtrRect = WriteList.Next(PtrRect)) {
					COORD BufferSize = {BufX, BufY}, BufferCoord = {PtrRect->Left, PtrRect->Top};
					SMALL_RECT WriteRegion = *PtrRect;
					Console.WriteOutput(*Buf, BufferSize, BufferCoord, WriteRegion);
				}
				memcpy(Shadow, Buf, BufX * BufY * sizeof(CHAR_INFO));
			}
		}

		if (MacroCharUsed) {
			Buf[0] = MacroChar;
		}

		if (ElevationCharUsed && BufX > 0 && BufY > 0) {
			Buf[BufX * BufY - 1] = ElevationChar;
		}

		if (!SBFlags.Check(SBFLAGS_FLUSHEDCURPOS)) {
			COORD C = {CurX, CurY};
			Console.SetCursorPosition(C);
			SBFlags.Set(SBFLAGS_FLUSHEDCURPOS);
		}

		if (!SBFlags.Check(SBFLAGS_FLUSHEDCURTYPE) && CurVisible) {
			CONSOLE_CURSOR_INFO cci = {CurSize, CurVisible};
			Console.SetCursorInfo(cci);
			SBFlags.Set(SBFLAGS_FLUSHEDCURTYPE);
		}

		SBFlags.Set(SBFLAGS_USESHADOW | SBFLAGS_FLUSHED);
	}
}

void ScreenBuf::Lock()
{
	LockCount++;
}

void ScreenBuf::Unlock()
{
	if (LockCount > 0)
		LockCount--;
}

void ScreenBuf::ResetShadow()
{
	SBFlags.Clear(SBFLAGS_FLUSHED | SBFLAGS_FLUSHEDCURTYPE | SBFLAGS_FLUSHEDCURPOS | SBFLAGS_USESHADOW);
}

void ScreenBuf::MoveCursor(int X, int Y)
{
	CriticalSectionLock Lock(CS);

	if (CurX < 0 || CurY < 0 || CurX > ScrX || CurY > ScrY) {
		CurVisible = FALSE;
	}

	if (X != CurX || Y != CurY || !CurVisible) {
		CurX = X;
		CurY = Y;
		SBFlags.Clear(SBFLAGS_FLUSHEDCURPOS);
	}
}

void ScreenBuf::GetCursorPos(SHORT &X, SHORT &Y)
{
	X = CurX;
	Y = CurY;
}

void ScreenBuf::SetCursorType(bool Visible, DWORD Size)
{
	/*
		$ 09.01.2001 SVS
		По наводке ER - в SetCursorType не дергать раньше
		времени установку курсора
	*/
	if (CurVisible != Visible || CurSize != Size) {
		CurVisible = Visible;
		CurSize = Size;
		SBFlags.Clear(SBFLAGS_FLUSHEDCURTYPE);
	}
}

void ScreenBuf::GetCursorType(bool &Visible, DWORD &Size)
{
	Visible = CurVisible;
	Size = CurSize;
}

void ScreenBuf::RestoreMacroChar()
{
	if (MacroCharUsed) {
		Write(0, 0, &MacroChar, 1);
		MacroCharUsed = false;
	}
}

void ScreenBuf::RestoreElevationChar()
{
	if (ElevationCharUsed) {
		Write(BufX - 1, BufY - 1, &ElevationChar, 1);
		ElevationCharUsed = false;
	}
}

// проскроллировать буфер на одну строку вверх.
void ScreenBuf::Scroll(int Num)
{
	CriticalSectionLock Lock(CS);
	if (!Buf)
		return;

	if (Num > 0 && Num < BufY)
		memmove(Buf, Buf + Num * BufX, (BufY - Num) * BufX * sizeof(CHAR_INFO));

#ifdef DIRECT_SCREEN_OUT
	Flush();
#elif defined(DIRECT_RT)

	if (DirectRT)
		Flush();

#endif
}
