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

#include "console.hpp"
#include "config.hpp"
#include "palette.hpp"
#include "colors.hpp"
#include "interf.hpp"

console Console;

//#define WINPORT(NAME) NAME

bool console::Allocate()
{
	return TRUE;//AllocConsole()!=FALSE;
}

bool console::Free()
{
	return TRUE;//FreeConsole()!=FALSE;
}

HANDLE console::GetInputHandle()
{
	return stdin;//GetStdHandle(STD_INPUT_HANDLE);
}

HANDLE console::GetOutputHandle()
{
	return stdout;//GetStdHandle(STD_OUTPUT_HANDLE);
}

HANDLE console::GetErrorHandle()
{
	return stderr;//GetStdHandle(STD_ERROR_HANDLE);
}

bool console::GetSize(COORD& Size)
{
	bool Result=false;
	CONSOLE_SCREEN_BUFFER_INFO ConsoleScreenBufferInfo;
	if(WINPORT(GetConsoleScreenBufferInfo)(GetOutputHandle(), &ConsoleScreenBufferInfo))
	{
		if(Opt.WindowMode)
		{
			Size.X=ConsoleScreenBufferInfo.srWindow.Right-ConsoleScreenBufferInfo.srWindow.Left+1;
			Size.Y=ConsoleScreenBufferInfo.srWindow.Bottom-ConsoleScreenBufferInfo.srWindow.Top+1;
		}
		else
		{
			Size=ConsoleScreenBufferInfo.dwSize;
		}
		Result=true;
	}
	return Result;
}

bool console::SetSize(COORD Size)
{
	bool Result=false;
	if(Opt.WindowMode)
	{
		CONSOLE_SCREEN_BUFFER_INFO csbi;
		WINPORT(GetConsoleScreenBufferInfo)(GetOutputHandle(), &csbi);
		csbi.srWindow.Left=0;
		csbi.srWindow.Right=Size.X-1;
		csbi.srWindow.Bottom=csbi.dwSize.Y-1;
		csbi.srWindow.Top=csbi.srWindow.Bottom-(Size.Y-1);
		COORD WindowCoord={(SHORT)(csbi.srWindow.Right-csbi.srWindow.Left+1), (SHORT)(csbi.srWindow.Bottom-csbi.srWindow.Top+1)};
		if(WindowCoord.X>csbi.dwSize.X || WindowCoord.Y>csbi.dwSize.Y)
		{
			WindowCoord.X=Max(WindowCoord.X,csbi.dwSize.X);
			WindowCoord.Y=Max(WindowCoord.Y,csbi.dwSize.Y);
			WINPORT(SetConsoleScreenBufferSize)(GetOutputHandle(), WindowCoord);
		}
		Result=SetWindowRect(csbi.srWindow);
	}
	else
	{
		Result=WINPORT(SetConsoleScreenBufferSize)(GetOutputHandle(), Size)!=FALSE;
	}
	return Result;
}

bool console::GetWindowRect(SMALL_RECT& ConsoleWindow)
{
	bool Result=false;
	CONSOLE_SCREEN_BUFFER_INFO ConsoleScreenBufferInfo;
	if(WINPORT(GetConsoleScreenBufferInfo)(GetOutputHandle(), &ConsoleScreenBufferInfo))
	{
		ConsoleWindow=ConsoleScreenBufferInfo.srWindow;
		Result=true;
	}
	return Result;
}

bool console::SetWindowRect(const SMALL_RECT& ConsoleWindow)
{
	return WINPORT(SetConsoleWindowInfo)(GetOutputHandle(), true, &ConsoleWindow)!=FALSE;
}

bool console::GetWorkingRect(SMALL_RECT& WorkingRect)
{
	bool Result=false;
	CONSOLE_SCREEN_BUFFER_INFO csbi;
	if(WINPORT(GetConsoleScreenBufferInfo)(GetOutputHandle(), &csbi))
	{
		WorkingRect.Bottom=csbi.dwSize.Y-1;
		WorkingRect.Left=0;
		WorkingRect.Right=WorkingRect.Left+ScrX;
		WorkingRect.Top=WorkingRect.Bottom-ScrY;
		Result=true;
	}
	return Result;
}

bool console::GetTitle(FARString &strTitle)
{
	DWORD dwSize = 0;
	DWORD dwBufferSize = MAX_PATH;
	wchar_t *lpwszTitle = nullptr;

	do
	{
		dwBufferSize <<= 1;
		lpwszTitle = (wchar_t*)xf_realloc_nomove(lpwszTitle, dwBufferSize*sizeof(wchar_t));
		dwSize = WINPORT(GetConsoleTitle)(lpwszTitle, dwBufferSize);
	}
	while (!dwSize && WINPORT(GetLastError)() == ERROR_SUCCESS);

	if (dwSize)
		strTitle = lpwszTitle;

	xf_free(lpwszTitle);
	return dwSize!=0;
}

bool console::SetTitle(LPCWSTR Title)
{
	return WINPORT(SetConsoleTitle)(Title)!=FALSE;
}

bool console::GetKeyboardLayoutName(FARString &strName)
{
	bool Result=false;
	strName.Clear();
/*	if (ifn.pfnGetConsoleKeyboardLayoutName)
	{
		wchar_t *p = strName.GetBuffer(KL_NAMELENGTH+1);
		if (p && ifn.pfnGetConsoleKeyboardLayoutName(p))
		{
			Result=true;
		}
		strName.ReleaseBuffer();
	}
	else*/
	{
		WINPORT(SetLastError)(ERROR_CALL_NOT_IMPLEMENTED);
	}
	return Result;
}

UINT console::GetInputCodepage()
{
	return CP_ACP;//GetConsoleCP();
}

bool console::SetInputCodepage(UINT Codepage)
{
	return TRUE;//SetConsoleCP(Codepage)!=FALSE;
}

UINT console::GetOutputCodepage()
{
	return CP_ACP; //GetConsoleOutputCP();
}

bool console::SetOutputCodepage(UINT Codepage)
{
	return TRUE;//SetConsoleOutputCP(Codepage)!=FALSE;
}

bool console::SetControlHandler(PHANDLER_ROUTINE HandlerRoutine, bool Add)
{
	return WINPORT(SetConsoleCtrlHandler)(HandlerRoutine, Add)!=FALSE;
}

bool console::GetMode(HANDLE ConsoleHandle, DWORD& Mode)
{
	return WINPORT(GetConsoleMode)(ConsoleHandle, &Mode)!=FALSE;
}

bool console::SetMode(HANDLE ConsoleHandle, DWORD Mode)
{
	return WINPORT(SetConsoleMode)(ConsoleHandle, Mode)!=FALSE;
}

bool console::PeekInput(INPUT_RECORD& Buffer, DWORD Length, DWORD& NumberOfEventsRead)
{
	bool Result=WINPORT(PeekConsoleInput)(GetInputHandle(), &Buffer, Length, &NumberOfEventsRead)!=FALSE;
	if(Opt.WindowMode && Buffer.EventType==MOUSE_EVENT)
	{
		Buffer.Event.MouseEvent.dwMousePosition.Y=Max(0, Buffer.Event.MouseEvent.dwMousePosition.Y-GetDelta());
		COORD Size;
		GetSize(Size);
		Buffer.Event.MouseEvent.dwMousePosition.X=Min(Buffer.Event.MouseEvent.dwMousePosition.X, static_cast<SHORT>(Size.X-1));
	}
	return Result;
}

bool console::ReadInput(INPUT_RECORD& Buffer, DWORD Length, DWORD& NumberOfEventsRead)
{
	bool Result=WINPORT(ReadConsoleInput)(GetInputHandle(), &Buffer, Length, &NumberOfEventsRead)!=FALSE;
	if(Opt.WindowMode && Buffer.EventType==MOUSE_EVENT)
	{
		Buffer.Event.MouseEvent.dwMousePosition.Y=Max(0, Buffer.Event.MouseEvent.dwMousePosition.Y-GetDelta());
		COORD Size;
		GetSize(Size);
		Buffer.Event.MouseEvent.dwMousePosition.X=Min(Buffer.Event.MouseEvent.dwMousePosition.X, static_cast<SHORT>(Size.X-1));
	}
	return Result;
}

bool console::WriteInput(INPUT_RECORD& Buffer, DWORD Length, DWORD& NumberOfEventsWritten)
{
	if(Opt.WindowMode && Buffer.EventType==MOUSE_EVENT)
	{
		Buffer.Event.MouseEvent.dwMousePosition.Y+=GetDelta();
	}
	return WINPORT(WriteConsoleInput)(GetInputHandle(), &Buffer, Length, &NumberOfEventsWritten)!=FALSE;
}

// пишем/читаем порциями по 32 K, иначе проблемы.
const unsigned int MAXSIZE=0x8000;

bool console::ReadOutput(CHAR_INFO& Buffer, COORD BufferSize, COORD BufferCoord, SMALL_RECT& ReadRegion)
{
	bool Result=false;
	int Delta=Opt.WindowMode?GetDelta():0;
	ReadRegion.Top+=Delta;
	ReadRegion.Bottom+=Delta;

	// skip unused region
	PCHAR_INFO BufferStart=&Buffer+BufferCoord.Y*BufferSize.X;
	BufferSize.Y-=BufferCoord.Y;
	BufferCoord.Y=0;

	if(BufferSize.X*BufferSize.Y*sizeof(CHAR_INFO)>MAXSIZE)
	{
		BufferSize.Y=static_cast<SHORT>(Max(static_cast<int>(MAXSIZE/(BufferSize.X*sizeof(CHAR_INFO))),1));
		int Height=ReadRegion.Bottom-ReadRegion.Top+1;
		int Start=ReadRegion.Top;
		SMALL_RECT SavedWriteRegion=ReadRegion;
		for(int i=0;i<Height;i+=BufferSize.Y)
		{
			ReadRegion=SavedWriteRegion;
			ReadRegion.Top=Start+i;
			PCHAR_INFO BufPtr=BufferStart+i*BufferSize.X;
			Result=WINPORT(ReadConsoleOutput)(GetOutputHandle(), BufPtr, BufferSize, BufferCoord, &ReadRegion)!=FALSE;
		}
	}
	else
	{
		Result=WINPORT(ReadConsoleOutput)(GetOutputHandle(), BufferStart, BufferSize, BufferCoord, &ReadRegion)!=FALSE;
	}

	if(Opt.WindowMode)
	{
		ReadRegion.Top-=Delta;
		ReadRegion.Bottom-=Delta;
	}

	return Result;
}

bool console::WriteOutput(const CHAR_INFO& Buffer, COORD BufferSize, COORD BufferCoord, SMALL_RECT& WriteRegion)
{
	bool Result=false;
	int Delta=Opt.WindowMode?GetDelta():0;
	WriteRegion.Top+=Delta;
	WriteRegion.Bottom+=Delta;

	// skip unused region
	const CHAR_INFO* BufferStart=&Buffer+BufferCoord.Y*BufferSize.X;
	BufferSize.Y-=BufferCoord.Y;
	BufferCoord.Y=0;

	if(BufferSize.X*BufferSize.Y*sizeof(CHAR_INFO)>MAXSIZE)
	{
		BufferSize.Y=static_cast<SHORT>(Max(static_cast<int>(MAXSIZE/(BufferSize.X*sizeof(CHAR_INFO))),1));
		int Height=WriteRegion.Bottom-WriteRegion.Top+1;
		int Start=WriteRegion.Top;
		SMALL_RECT SavedWriteRegion=WriteRegion;
		for(int i=0;i<Height;i+=BufferSize.Y)
		{
			WriteRegion=SavedWriteRegion;
			WriteRegion.Top=Start+i;
			const CHAR_INFO* BufPtr=BufferStart+i*BufferSize.X;
			Result=WINPORT(WriteConsoleOutput)(GetOutputHandle(), BufPtr, BufferSize, BufferCoord, &WriteRegion)!=FALSE;
		}
	}
	else
	{
		Result=WINPORT(WriteConsoleOutput)(GetOutputHandle(), BufferStart, BufferSize, BufferCoord, &WriteRegion)!=FALSE;
	}

	if(Opt.WindowMode)
	{
		WriteRegion.Top-=Delta;
		WriteRegion.Bottom-=Delta;
	}

	return Result;
}

bool console::Write(LPCWSTR Buffer, DWORD NumberOfCharsToWrite)
{
	DWORD NumberOfCharsWritten;
	return WINPORT(WriteConsole)(GetOutputHandle(), Buffer, NumberOfCharsToWrite, &NumberOfCharsWritten, nullptr)!=FALSE;
}

bool console::GetTextAttributes(WORD& Attributes)
{
	bool Result=false;
	CONSOLE_SCREEN_BUFFER_INFO ConsoleScreenBufferInfo;
	if(WINPORT(GetConsoleScreenBufferInfo)(GetOutputHandle(), &ConsoleScreenBufferInfo))
	{
		Attributes=ConsoleScreenBufferInfo.wAttributes;
		Result=true;
	}
	return Result;
}

bool console::SetTextAttributes(WORD Attributes)
{
	return WINPORT(SetConsoleTextAttribute)(GetOutputHandle(), Attributes)!=FALSE;
}

bool console::GetCursorInfo(CONSOLE_CURSOR_INFO& ConsoleCursorInfo)
{
	return WINPORT(GetConsoleCursorInfo)(GetOutputHandle(), &ConsoleCursorInfo)!=FALSE;
}

bool console::SetCursorInfo(const CONSOLE_CURSOR_INFO& ConsoleCursorInfo)
{
	return WINPORT(SetConsoleCursorInfo)(GetOutputHandle(), &ConsoleCursorInfo)!=FALSE;
}

bool console::GetCursorPosition(COORD& Position)
{
	bool Result=false;
	CONSOLE_SCREEN_BUFFER_INFO ConsoleScreenBufferInfo;
	if(WINPORT(GetConsoleScreenBufferInfo)(GetOutputHandle(), &ConsoleScreenBufferInfo))
	{
		Position=ConsoleScreenBufferInfo.dwCursorPosition;
		if(Opt.WindowMode)
		{
			Position.Y-=GetDelta();
		}
		Result=true;
	}
	return Result;
}

bool console::SetCursorPosition(COORD Position)
{

	if(Opt.WindowMode)
	{
		ResetPosition();
		COORD Size;
		GetSize(Size);
		Position.X=Min(Position.X,static_cast<SHORT>(Size.X-1));
		Position.Y=Max(static_cast<SHORT>(0),Position.Y);
		Position.Y+=GetDelta();
	}
	return WINPORT(SetConsoleCursorPosition)(GetOutputHandle(), Position)!=FALSE;
}

bool console::FlushInputBuffer()
{
	return WINPORT(FlushConsoleInputBuffer)(GetInputHandle())!=FALSE;
}

bool console::GetNumberOfInputEvents(DWORD& NumberOfEvents)
{
	return WINPORT(GetNumberOfConsoleInputEvents)(GetInputHandle(), &NumberOfEvents)!=FALSE;
}

DWORD console::GetAlias(LPCWSTR Source, LPWSTR TargetBuffer, DWORD TargetBufferLength, LPCWSTR ExeName)
{
	return WINPORT(GetConsoleAlias)(const_cast<LPWSTR>(Source), TargetBuffer, TargetBufferLength, const_cast<LPWSTR>(ExeName));
}

bool console::GetDisplayMode(DWORD& Mode)
{
	return WINPORT(GetConsoleDisplayMode)(&Mode)!=FALSE;
}

COORD console::GetLargestWindowSize()
{
	return WINPORT(GetLargestConsoleWindowSize)(GetOutputHandle());
}

bool console::SetActiveScreenBuffer(HANDLE ConsoleOutput)
{
	return WINPORT(SetConsoleActiveScreenBuffer)(ConsoleOutput)!=FALSE;
}

bool console::ClearExtraRegions(WORD Color)
{
	CONSOLE_SCREEN_BUFFER_INFO csbi;
	WINPORT(GetConsoleScreenBufferInfo)(GetOutputHandle(), &csbi);
	DWORD TopSize = csbi.dwSize.X*csbi.srWindow.Top;
	DWORD CharsWritten;
	COORD TopCoord={0,0};
	WINPORT(FillConsoleOutputCharacter)(GetOutputHandle(), L' ', TopSize, TopCoord, &CharsWritten);
	WINPORT(FillConsoleOutputAttribute)(GetOutputHandle(), Color, TopSize, TopCoord, &CharsWritten );

	DWORD RightSize = csbi.dwSize.X-csbi.srWindow.Right;
	COORD RightCoord={csbi.srWindow.Right,(SHORT)GetDelta()};
	for(; RightCoord.Y<csbi.dwSize.Y; RightCoord.Y++)
	{
		WINPORT(FillConsoleOutputCharacter)(GetOutputHandle(), L' ', RightSize, RightCoord, &CharsWritten);
		WINPORT(FillConsoleOutputAttribute)(GetOutputHandle(), Color, RightSize, RightCoord, &CharsWritten);
	}
	return true;
}

bool console::ScrollWindow(int Lines,int Columns)
{
	bool process=false;
	CONSOLE_SCREEN_BUFFER_INFO csbi;
	WINPORT(GetConsoleScreenBufferInfo)(GetOutputHandle(), &csbi);

	if((Lines<0 && csbi.srWindow.Top) || (Lines>0 && csbi.srWindow.Bottom!=csbi.dwSize.Y-1))
	{
		csbi.srWindow.Top+=Lines;
		csbi.srWindow.Bottom+=Lines;

		if(csbi.srWindow.Top<0)
		{
			csbi.srWindow.Bottom-=csbi.srWindow.Top;
			csbi.srWindow.Top=0;
		}

		if(csbi.srWindow.Bottom>=csbi.dwSize.Y)
		{
			csbi.srWindow.Top-=(csbi.srWindow.Bottom-(csbi.dwSize.Y-1));
			csbi.srWindow.Bottom=csbi.dwSize.Y-1;
		}
		process=true;
	}

	if((Columns<0 && csbi.srWindow.Left) || (Columns>0 && csbi.srWindow.Right!=csbi.dwSize.X-1))
	{
		csbi.srWindow.Left+=Columns;
		csbi.srWindow.Right+=Columns;

		if(csbi.srWindow.Left<0)
		{
			csbi.srWindow.Right-=csbi.srWindow.Left;
			csbi.srWindow.Left=0;
		}

		if(csbi.srWindow.Right>=csbi.dwSize.X)
		{
			csbi.srWindow.Left-=(csbi.srWindow.Right-(csbi.dwSize.X-1));
			csbi.srWindow.Right=csbi.dwSize.X-1;
		}
		process=true;
	}

	if (process)
	{
		SetWindowRect(csbi.srWindow);
		return true;
	}

	return false;
}

bool console::ScrollScreenBuffer(int Lines)
{
	CONSOLE_SCREEN_BUFFER_INFO csbi;
	WINPORT(GetConsoleScreenBufferInfo)(GetOutputHandle(), &csbi);
	SMALL_RECT ScrollRectangle={0, 0, (SHORT)(csbi.dwSize.X-1), (SHORT)(csbi.dwSize.Y-1)};
	COORD DestinationOrigin={0,(SHORT)(-Lines)};
	CHAR_INFO Fill={L' ', FarColorToReal(COL_COMMANDLINEUSERSCREEN)};
	return WINPORT(ScrollConsoleScreenBuffer)(GetOutputHandle(), &ScrollRectangle, nullptr, DestinationOrigin, &Fill)!=FALSE;
}

bool console::ResetPosition()
{
	CONSOLE_SCREEN_BUFFER_INFO csbi;
	WINPORT(GetConsoleScreenBufferInfo)(GetOutputHandle(), &csbi);
	if(csbi.srWindow.Left || csbi.srWindow.Bottom!=csbi.dwSize.Y-1)
	{
		csbi.srWindow.Right-=csbi.srWindow.Left;
		csbi.srWindow.Left=0;
		csbi.srWindow.Top+=csbi.dwSize.Y-1-csbi.srWindow.Bottom;
		csbi.srWindow.Bottom=csbi.dwSize.Y-1;
		SetWindowRect(csbi.srWindow);
	}
	return true;
}

int console::GetDelta()
{
	CONSOLE_SCREEN_BUFFER_INFO csbi;
	WINPORT(GetConsoleScreenBufferInfo)(GetOutputHandle(), &csbi);
	return csbi.dwSize.Y-(csbi.srWindow.Bottom-csbi.srWindow.Top+1);
}

BOOL console::IsZoomed()
{
	//todo
	return FALSE;
}

BOOL console::IsIconic()
{
	//todo
	return FALSE;
}
