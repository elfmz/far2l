/*
console.cpp

Console functions
*/
/*
Copyright (c) 2010 Far Group
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

#include <StackHeapArray.hpp>
#include "console.hpp"
#include "config.hpp"
#include "palette.hpp"
#include "colors.hpp"
#include "interf.hpp"

console Console;

// #define WINPORT(NAME) NAME

bool console::Allocate()
{
	return TRUE;	// AllocConsole()!=FALSE;
}

bool console::Free()
{
	return TRUE;	// FreeConsole()!=FALSE;
}

HANDLE console::GetInputHandle()
{
	return NULL;	// GetStdHandle(STD_INPUT_HANDLE);
}

HANDLE console::GetOutputHandle()
{
	return NULL;	// GetStdHandle(STD_OUTPUT_HANDLE);
}

HANDLE console::GetErrorHandle()
{
	return NULL;	// GetStdHandle(STD_ERROR_HANDLE);
}

bool console::GetSize(COORD &Size)
{
	bool Result = false;
	CONSOLE_SCREEN_BUFFER_INFO ConsoleScreenBufferInfo;
	if (WINPORT(GetConsoleScreenBufferInfo)(GetOutputHandle(), &ConsoleScreenBufferInfo)) {
		Size = ConsoleScreenBufferInfo.dwSize;
		Result = true;
	}
	return Result;
}

bool console::SetSize(COORD Size)
{
	return WINPORT(SetConsoleScreenBufferSize)(GetOutputHandle(), Size) != FALSE;
}

bool console::GetWindowRect(SMALL_RECT &ConsoleWindow)
{
	bool Result = false;
	CONSOLE_SCREEN_BUFFER_INFO ConsoleScreenBufferInfo;
	if (WINPORT(GetConsoleScreenBufferInfo)(GetOutputHandle(), &ConsoleScreenBufferInfo)) {
		ConsoleWindow = ConsoleScreenBufferInfo.srWindow;
		Result = true;
	}
	return Result;
}

bool console::SetWindowRect(const SMALL_RECT &ConsoleWindow)
{
	return WINPORT(SetConsoleWindowInfo)(GetOutputHandle(), true, &ConsoleWindow) != FALSE;
}

bool console::GetWorkingRect(SMALL_RECT &WorkingRect)
{
	bool Result = false;
	CONSOLE_SCREEN_BUFFER_INFO csbi;
	if (WINPORT(GetConsoleScreenBufferInfo)(GetOutputHandle(), &csbi)) {
		WorkingRect.Bottom = csbi.dwSize.Y - 1;
		WorkingRect.Left = 0;
		WorkingRect.Right = WorkingRect.Left + ScrX;
		WorkingRect.Top = WorkingRect.Bottom - ScrY;
		Result = true;
	}
	return Result;
}

bool console::GetTitle(FARString &strTitle)
{
	for (DWORD Size = 0x100; Size; Size<<= 1) {
		StackHeapArray<wchar_t, 0x200> buf(Size);
		if (!buf.Get())
			break;

		if (WINPORT(GetConsoleTitle)(NULL, buf.Get(), Size) < Size) {
			strTitle = buf.Get();
			return true;
		}
	}

	return false;
}

bool console::SetTitle(LPCWSTR Title)
{
	return WINPORT(SetConsoleTitle)(NULL, Title) != FALSE;
}

UINT console::GetInputCodepage()
{
	return CP_ACP;	// GetConsoleCP();
}

bool console::SetInputCodepage(UINT Codepage)
{
	return TRUE;	// SetConsoleCP(Codepage)!=FALSE;
}

UINT console::GetOutputCodepage()
{
	return CP_ACP;	// GetConsoleOutputCP();
}

bool console::SetOutputCodepage(UINT Codepage)
{
	return TRUE;	// SetConsoleOutputCP(Codepage)!=FALSE;
}

bool console::SetControlHandler(PHANDLER_ROUTINE HandlerRoutine, bool Add)
{
	return WINPORT(SetConsoleCtrlHandler)(HandlerRoutine, Add) != FALSE;
}

bool console::GetMode(HANDLE ConsoleHandle, DWORD &Mode)
{
	return WINPORT(GetConsoleMode)(ConsoleHandle, &Mode) != FALSE;
}

bool console::SetMode(HANDLE ConsoleHandle, DWORD Mode)
{
	return WINPORT(SetConsoleMode)(ConsoleHandle, Mode) != FALSE;
}

bool console::InspectStickyKeyEvent(INPUT_RECORD &ir)
{
	if (ir.EventType != KEY_EVENT)
		return false;

	if (ir.Event.KeyEvent.uChar.UnicodeChar == ' ' && ir.Event.KeyEvent.wVirtualKeyCode == VK_SPACE) {
		if (ir.Event.KeyEvent.bKeyDown) {
			if ((ir.Event.KeyEvent.dwControlKeyState
						& (LEFT_CTRL_PRESSED | RIGHT_CTRL_PRESSED | RIGHT_ALT_PRESSED | LEFT_ALT_PRESSED))
					!= 0) {
				_StickyControlKeyState|= ir.Event.KeyEvent.dwControlKeyState;
				_StickySkipKeyUp = true;
				return true;
			}
		}
	}

	if (_StickySkipKeyUp) {
		if (!ir.Event.KeyEvent.bKeyDown)
			return true;

		_StickySkipKeyUp = false;
	}

	if (_StickyControlKeyState) {
		ir.Event.KeyEvent.dwControlKeyState|= _StickyControlKeyState;
		if (!ir.Event.KeyEvent.bKeyDown)
			_StickyControlKeyState = 0;
	}

	return false;
}

bool console::PeekInput(INPUT_RECORD &Buffer)
{
	for (;;) {
		DWORD NumberOfEventsRead = 0;
		if (!WINPORT(PeekConsoleInput)(GetInputHandle(), &Buffer, 1, &NumberOfEventsRead)
				|| !NumberOfEventsRead)
			return false;

		if (!InspectStickyKeyEvent(Buffer))
			break;

		if (!WINPORT(ReadConsoleInput)(GetInputHandle(), &Buffer, 1, &NumberOfEventsRead)
				|| !NumberOfEventsRead) {
			fprintf(stderr, "console::PeekInput: failed to skip sticky input event\n");
			WINPORT(Sleep)(100);
		}
	}

	return true;
}

bool console::ReadInput(INPUT_RECORD &Buffer)
{
	DWORD NumberOfEventsRead = 0;
	do {
		if (!WINPORT(ReadConsoleInput)(GetInputHandle(), &Buffer, 1, &NumberOfEventsRead)
				|| !NumberOfEventsRead)
			return false;

	} while (InspectStickyKeyEvent(Buffer));

	return true;
}

bool console::WriteInput(INPUT_RECORD &Buffer, DWORD Length, DWORD &NumberOfEventsWritten)
{
	return WINPORT(WriteConsoleInput)(GetInputHandle(), &Buffer, Length, &NumberOfEventsWritten) != FALSE;
}

// пишем/читаем порциями по 32 K, иначе проблемы.
const unsigned int MAXSIZE = 0x8000;

bool console::ReadOutput(CHAR_INFO &Buffer, COORD BufferSize, COORD BufferCoord, SMALL_RECT &ReadRegion)
{
	bool Result = false;

	// skip unused region
	PCHAR_INFO BufferStart = &Buffer + BufferCoord.Y * BufferSize.X;
	BufferSize.Y-= BufferCoord.Y;
	BufferCoord.Y = 0;

	if (BufferSize.X * BufferSize.Y * sizeof(CHAR_INFO) > MAXSIZE) {
		BufferSize.Y =
				static_cast<SHORT>(Max(static_cast<int>(MAXSIZE / (BufferSize.X * sizeof(CHAR_INFO))), 1));
		int Height = ReadRegion.Bottom - ReadRegion.Top + 1;
		int Start = ReadRegion.Top;
		SMALL_RECT SavedWriteRegion = ReadRegion;
		for (int i = 0; i < Height; i+= BufferSize.Y) {
			ReadRegion = SavedWriteRegion;
			ReadRegion.Top = Start + i;
			PCHAR_INFO BufPtr = BufferStart + i * BufferSize.X;
			Result = WINPORT(ReadConsoleOutput)(GetOutputHandle(), BufPtr, BufferSize, BufferCoord,
							&ReadRegion)
					!= FALSE;
		}
	} else {
		Result = WINPORT(ReadConsoleOutput)(GetOutputHandle(), BufferStart, BufferSize, BufferCoord,
						&ReadRegion)
				!= FALSE;
	}

	return Result;
}

bool console::WriteOutput(const CHAR_INFO &Buffer, COORD BufferSize, COORD BufferCoord,
		SMALL_RECT &WriteRegion)
{
	bool Result = false;

	// skip unused region
	const CHAR_INFO *BufferStart = &Buffer + BufferCoord.Y * BufferSize.X;
	BufferSize.Y-= BufferCoord.Y;
	BufferCoord.Y = 0;

	if (BufferSize.X * BufferSize.Y * sizeof(CHAR_INFO) > MAXSIZE) {
		BufferSize.Y =
				static_cast<SHORT>(Max(static_cast<int>(MAXSIZE / (BufferSize.X * sizeof(CHAR_INFO))), 1));
		int Height = WriteRegion.Bottom - WriteRegion.Top + 1;
		int Start = WriteRegion.Top;
		SMALL_RECT SavedWriteRegion = WriteRegion;
		for (int i = 0; i < Height; i+= BufferSize.Y) {
			WriteRegion = SavedWriteRegion;
			WriteRegion.Top = Start + i;
			const CHAR_INFO *BufPtr = BufferStart + i * BufferSize.X;
			Result = WINPORT(WriteConsoleOutput)(GetOutputHandle(), BufPtr, BufferSize, BufferCoord,
							&WriteRegion)
					!= FALSE;
		}
	} else {
		Result = WINPORT(WriteConsoleOutput)(GetOutputHandle(), BufferStart, BufferSize, BufferCoord,
						&WriteRegion)
				!= FALSE;
	}

	return Result;
}

bool console::Write(LPCWSTR Buffer, DWORD NumberOfCharsToWrite)
{
	DWORD NumberOfCharsWritten;
	return WINPORT(
				WriteConsole)(GetOutputHandle(), Buffer, NumberOfCharsToWrite, &NumberOfCharsWritten, nullptr)
			!= FALSE;
}

bool console::GetTextAttributes(uint64_t &Attributes)
{
	bool Result = false;
	CONSOLE_SCREEN_BUFFER_INFO ConsoleScreenBufferInfo;
	if (WINPORT(GetConsoleScreenBufferInfo)(GetOutputHandle(), &ConsoleScreenBufferInfo)) {
		Attributes = ConsoleScreenBufferInfo.wAttributes;
		Result = true;
	}
	return Result;
}

bool console::SetTextAttributes(uint64_t Attributes)
{
	return WINPORT(SetConsoleTextAttribute)(GetOutputHandle(), Attributes) != FALSE;
}

bool console::GetCursorInfo(CONSOLE_CURSOR_INFO &ConsoleCursorInfo)
{
	return WINPORT(GetConsoleCursorInfo)(GetOutputHandle(), &ConsoleCursorInfo) != FALSE;
}

bool console::SetCursorInfo(const CONSOLE_CURSOR_INFO &ConsoleCursorInfo)
{
	return WINPORT(SetConsoleCursorInfo)(GetOutputHandle(), &ConsoleCursorInfo) != FALSE;
}

bool console::GetCursorPosition(COORD &Position)
{
	bool Result = false;
	CONSOLE_SCREEN_BUFFER_INFO ConsoleScreenBufferInfo;
	if (WINPORT(GetConsoleScreenBufferInfo)(GetOutputHandle(), &ConsoleScreenBufferInfo)) {
		Position = ConsoleScreenBufferInfo.dwCursorPosition;
		Result = true;
	}
	return Result;
}

bool console::SetCursorPosition(COORD Position)
{
	return WINPORT(SetConsoleCursorPosition)(GetOutputHandle(), Position) != FALSE;
}

bool console::FlushInputBuffer()
{
	return WINPORT(FlushConsoleInputBuffer)(GetInputHandle()) != FALSE;
}

bool console::GetNumberOfInputEvents(DWORD &NumberOfEvents)
{
	return WINPORT(GetNumberOfConsoleInputEvents)(GetInputHandle(), &NumberOfEvents) != FALSE;
}

bool console::GetDisplayMode(DWORD &Mode)
{
	return WINPORT(GetConsoleDisplayMode)(&Mode) != FALSE;
}

COORD console::GetLargestWindowSize()
{
	return WINPORT(GetLargestConsoleWindowSize)(GetOutputHandle());
}

bool console::SetActiveScreenBuffer(HANDLE ConsoleOutput)
{
	return WINPORT(SetConsoleActiveScreenBuffer)(ConsoleOutput) != FALSE;
}

bool console::ClearExtraRegions(WORD Color)
{
	CONSOLE_SCREEN_BUFFER_INFO csbi;
	WINPORT(GetConsoleScreenBufferInfo)(GetOutputHandle(), &csbi);
	DWORD TopSize = csbi.dwSize.X * csbi.srWindow.Top;
	DWORD CharsWritten;
	COORD TopCoord = {0, 0};
	WINPORT(FillConsoleOutputCharacter)(GetOutputHandle(), L' ', TopSize, TopCoord, &CharsWritten);
	WINPORT(FillConsoleOutputAttribute)(GetOutputHandle(), Color, TopSize, TopCoord, &CharsWritten);

	DWORD RightSize = csbi.dwSize.X - csbi.srWindow.Right;
	COORD RightCoord = {csbi.srWindow.Right, (SHORT)GetDelta()};
	for (; RightCoord.Y < csbi.dwSize.Y; RightCoord.Y++) {
		WINPORT(FillConsoleOutputCharacter)(GetOutputHandle(), L' ', RightSize, RightCoord, &CharsWritten);
		WINPORT(FillConsoleOutputAttribute)(GetOutputHandle(), Color, RightSize, RightCoord, &CharsWritten);
	}
	return true;
}

bool console::ScrollWindow(int Lines, int Columns)
{
	bool process = false;
	CONSOLE_SCREEN_BUFFER_INFO csbi;
	WINPORT(GetConsoleScreenBufferInfo)(GetOutputHandle(), &csbi);

	if ((Lines < 0 && csbi.srWindow.Top) || (Lines > 0 && csbi.srWindow.Bottom != csbi.dwSize.Y - 1)) {
		csbi.srWindow.Top+= Lines;
		csbi.srWindow.Bottom+= Lines;

		if (csbi.srWindow.Top < 0) {
			csbi.srWindow.Bottom-= csbi.srWindow.Top;
			csbi.srWindow.Top = 0;
		}

		if (csbi.srWindow.Bottom >= csbi.dwSize.Y) {
			csbi.srWindow.Top-= (csbi.srWindow.Bottom - (csbi.dwSize.Y - 1));
			csbi.srWindow.Bottom = csbi.dwSize.Y - 1;
		}
		process = true;
	}

	if ((Columns < 0 && csbi.srWindow.Left) || (Columns > 0 && csbi.srWindow.Right != csbi.dwSize.X - 1)) {
		csbi.srWindow.Left+= Columns;
		csbi.srWindow.Right+= Columns;

		if (csbi.srWindow.Left < 0) {
			csbi.srWindow.Right-= csbi.srWindow.Left;
			csbi.srWindow.Left = 0;
		}

		if (csbi.srWindow.Right >= csbi.dwSize.X) {
			csbi.srWindow.Left-= (csbi.srWindow.Right - (csbi.dwSize.X - 1));
			csbi.srWindow.Right = csbi.dwSize.X - 1;
		}
		process = true;
	}

	if (process) {
		SetWindowRect(csbi.srWindow);
		return true;
	}

	return false;
}

bool console::ScrollScreenBuffer(int Lines)
{
	CONSOLE_SCREEN_BUFFER_INFO csbi;
	WINPORT(GetConsoleScreenBufferInfo)(GetOutputHandle(), &csbi);
	SMALL_RECT ScrollRectangle = {0, 0, (SHORT)(csbi.dwSize.X - 1), (SHORT)(csbi.dwSize.Y - 1)};
	COORD DestinationOrigin = {0, (SHORT)(-Lines)};
	CHAR_INFO Fill = {{L' '}, FarColorToReal(COL_COMMANDLINEUSERSCREEN)};
	return WINPORT(ScrollConsoleScreenBuffer)(GetOutputHandle(), &ScrollRectangle, nullptr, DestinationOrigin,
				&Fill)
			!= FALSE;
}

bool console::ResetPosition()
{
	CONSOLE_SCREEN_BUFFER_INFO csbi;
	WINPORT(GetConsoleScreenBufferInfo)(GetOutputHandle(), &csbi);
	if (csbi.srWindow.Left || csbi.srWindow.Bottom != csbi.dwSize.Y - 1) {
		csbi.srWindow.Right-= csbi.srWindow.Left;
		csbi.srWindow.Left = 0;
		csbi.srWindow.Top+= csbi.dwSize.Y - 1 - csbi.srWindow.Bottom;
		csbi.srWindow.Bottom = csbi.dwSize.Y - 1;
		SetWindowRect(csbi.srWindow);
	}
	return true;
}

int console::GetDelta()
{
	CONSOLE_SCREEN_BUFFER_INFO csbi;
	WINPORT(GetConsoleScreenBufferInfo)(GetOutputHandle(), &csbi);
	return csbi.dwSize.Y - (csbi.srWindow.Bottom - csbi.srWindow.Top + 1);
}
