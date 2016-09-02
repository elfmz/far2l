/*
interf.cpp

Êîíñîëüíûå ôóíêöèè ââîäà-âûâîäà
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
#pragma hdrstop
#include <stdarg.h>

#include "interf.hpp"
#include "keyboard.hpp"
#include "keys.hpp"
#include "colors.hpp"
#include "ctrlobj.hpp"
#include "filepanels.hpp"
#include "panel.hpp"
#include "fileedit.hpp"
#include "manager.hpp"
#include "scrbuf.hpp"
#include "syslog.hpp"
#include "registry.hpp"
#include "palette.hpp"
#include "strmix.hpp"
#include "console.hpp"

BOOL WINAPI CtrlHandler(DWORD CtrlType);

static int CurX,CurY;
static int CurColor;

CONSOLE_CURSOR_INFO InitialCursorInfo;

static SMALL_RECT windowholder_rect;

WCHAR Oem2Unicode[256];
WCHAR BoxSymbols[64];

COORD InitSize={0};
COORD CurSize={0};
SHORT ScrX=0,ScrY=0;
SHORT PrevScrX=-1,PrevScrY=-1;
DWORD InitialConsoleMode=0;
string strInitTitle;
SMALL_RECT InitWindowRect;
COORD InitialSize;

//static HICON hOldLargeIcon=nullptr, hOldSmallIcon=nullptr;

void InitConsole(int FirstInit)
{
	InitRecodeOutTable();
	Console.SetControlHandler(CtrlHandler,TRUE);
	Console.GetMode(Console.GetInputHandle(),InitialConsoleMode);
	Console.GetTitle(strInitTitle);
	Console.GetWindowRect(InitWindowRect);
	Console.GetSize(InitialSize);
	CONSOLE_CURSOR_INFO InitCursorInfo;
	Console.GetCursorInfo(InitCursorInfo);

	GetRegKey(L"Interface",L"Mouse",Opt.Mouse,1);

	// ðàçìåð êëàâèàòóðíîé î÷åðåäè = 1024 êîäà êëàâèøè
	if (!KeyQueue)
		KeyQueue=new FarQueue<DWORD>(1024);

	SetFarConsoleMode();

	/* $ 09.04.2002 DJ
	   åñëè ðàçìåð êîíñîëüíîãî áóôåðà áîëüøå ðàçìåðà îêíà, âûñòàâèì
	   èõ ðàâíûìè
	*/
	if(!Opt.WindowMode)
	{
		GetRegKey(L"System",L"WindowMode",Opt.WindowMode, 0);
	}
	if (FirstInit)
	{
		SMALL_RECT WindowRect;
		Console.GetWindowRect(WindowRect);
		GetVideoMode(InitSize);

		if(Opt.WindowMode)
		{
			Console.ResetPosition();
		}
		else
		{
			if (WindowRect.Left || WindowRect.Top || WindowRect.Right != InitSize.X-1 || WindowRect.Bottom != InitSize.Y-1)
			{
				COORD newSize;
				newSize.X = WindowRect.Right - WindowRect.Left + 1;
				newSize.Y = WindowRect.Bottom - WindowRect.Top + 1;
				Console.SetSize(newSize);
				GetVideoMode(InitSize);
			}
		}
	}

	GetVideoMode(CurSize);
	ScrBuf.FillBuf();

	// áûëî sizeof(Palette)
	/*$ 14.02.2001 SKV
	  äëÿ consoledetach íå íóæíî, ÷òî áû èíèòèëàñü ïàëèòðà.
	*/
	if (FirstInit)
		memcpy(Palette,DefaultPalette,SizeArrayPalette);

}
void CloseConsole()
{
	ScrBuf.Flush();
	Console.SetCursorInfo(InitialCursorInfo);
	ChangeConsoleMode(InitialConsoleMode);

	Console.SetTitle(strInitTitle);
	Console.SetSize(InitialSize);
	Console.SetWindowRect(InitWindowRect);
	Console.SetSize(InitialSize);

	delete KeyQueue;
	KeyQueue=nullptr;
}


void SetFarConsoleMode(BOOL SetsActiveBuffer)
{
	int Mode=ENABLE_WINDOW_INPUT;

	if (Opt.Mouse)
	{
		//ENABLE_EXTENDED_FLAGS actually disables all the extended flags.
		Mode|=ENABLE_MOUSE_INPUT|ENABLE_EXTENDED_FLAGS;
	}
	else
	{
		//åñëè âäðóã èçìåíèëè îïöèþ âî âðåìÿ ðàáîòû ôàðà, òî âêëþ÷èì òî ÷òî íàäî
		Mode|=InitialConsoleMode&(ENABLE_EXTENDED_FLAGS|ENABLE_QUICK_EDIT_MODE);
	}

	if (SetsActiveBuffer)
		Console.SetActiveScreenBuffer(Console.GetOutputHandle());

	ChangeConsoleMode(Mode);
}

void ChangeConsoleMode(int Mode)
{
	DWORD CurrentConsoleMode;
	HANDLE hConIn = Console.GetInputHandle();
	Console.GetMode(hConIn, CurrentConsoleMode);

	if (CurrentConsoleMode!=(DWORD)Mode)
		Console.SetMode(hConIn, Mode);
}

void SaveConsoleWindowRect()
{
	Console.GetWindowRect(windowholder_rect);
}

void RestoreConsoleWindowRect()
{
	SMALL_RECT WindowRect;
	Console.GetWindowRect(WindowRect);
	if(WindowRect.Right-WindowRect.Left<windowholder_rect.Right-windowholder_rect.Left ||
		WindowRect.Bottom-WindowRect.Top<windowholder_rect.Bottom-windowholder_rect.Top)
	{
		Console.SetWindowRect(windowholder_rect);
	}
}

void FlushInputBuffer()
{
	Console.FlushInputBuffer();
	MouseButtonState=0;
	MouseEventFlags=0;
}

void SetVideoMode()
{
	if (!IsFullscreen() && Opt.AltF9)
	{
		ChangeVideoMode(InitSize.X==CurSize.X && InitSize.Y==CurSize.Y);
	}
	else
	{
		ChangeVideoMode(ScrY==24?50:25,80);
	}
}

void ChangeVideoMode(int Maximized)
{
	COORD coordScreen;

	if (Maximized)
	{
		//SendMessage(Console.GetWindow(),WM_SYSCOMMAND,SC_MAXIMIZE,0);
		coordScreen = Console.GetLargestWindowSize();
		coordScreen.X+=Opt.ScrSize.DeltaXY.X;
		coordScreen.Y+=Opt.ScrSize.DeltaXY.Y;
	}
	else
	{
		//SendMessage(Console.GetWindow(),WM_SYSCOMMAND,SC_RESTORE,0);
		coordScreen = InitSize;
	}

	ChangeVideoMode(coordScreen.Y,coordScreen.X);
}

void ChangeVideoMode(int NumLines,int NumColumns)
{
	int xSize=NumColumns,ySize=NumLines;

	COORD Size;
	Console.GetSize(Size);

	SMALL_RECT srWindowRect;
	srWindowRect.Right = xSize-1;
	srWindowRect.Bottom = ySize-1;
	srWindowRect.Left = srWindowRect.Top = 0;
	
	COORD coordScreen={xSize,ySize};

	if (xSize>Size.X || ySize > Size.Y)
	{
		if (Size.X < xSize-1)
		{
			srWindowRect.Right = Size.X - 1;
			Console.SetWindowRect(srWindowRect);
			srWindowRect.Right = xSize-1;
		}

		if (Size.Y < ySize-1)
		{
			srWindowRect.Bottom=Size.Y - 1;
			Console.SetWindowRect(srWindowRect);
			srWindowRect.Bottom = ySize-1;
		}

		Console.SetSize(coordScreen);
	}

	if (!Console.SetWindowRect(srWindowRect))
	{
		Console.SetSize(coordScreen);
		Console.SetWindowRect(srWindowRect);
	}
	else
	{
		Console.SetSize(coordScreen);
	}

	// çàøëåì ýâåíò òîëüêî â ñëó÷àå, êîãäà ìàêðîñû íå èñïîëíÿþòñÿ
	if (CtrlObject && !CtrlObject->Macro.IsExecuting())
		GenerateWINDOW_BUFFER_SIZE_EVENT(NumColumns,NumLines);
}

void GenerateWINDOW_BUFFER_SIZE_EVENT(int Sx, int Sy)
{
	COORD Size;
	if (Sx==-1 || Sy==-1)
	{
		Console.GetSize(Size);
	}
	INPUT_RECORD Rec;
	Rec.EventType=WINDOW_BUFFER_SIZE_EVENT;
	Rec.Event.WindowBufferSizeEvent.dwSize.X=Sx==-1?Size.X:Sx;
	Rec.Event.WindowBufferSizeEvent.dwSize.Y=Sy==-1?Size.Y:Sy;
	DWORD Writes;
	Console.WriteInput(Rec,1,Writes);
}

void GetVideoMode(COORD& Size)
{
	//÷òîá ðåøèòü áàã âèíäû ïðèâîäÿùèé ê ïîÿâëåíèþ ñêðîëîâ è ò.ï. ïîñëå ïîòåðè ôîêóñà
	SaveConsoleWindowRect();
	Size.X=0;
	Size.Y=0;
	Console.GetSize(Size);
	ScrX=Size.X-1;
	ScrY=Size.Y-1;
	assert(ScrX>0);
	assert(ScrY>0);
	WidthNameForMessage=(ScrX*38)/100+1;

	if (PrevScrX == -1) PrevScrX=ScrX;

	if (PrevScrY == -1) PrevScrY=ScrY;

	_OT(SysLog(L"ScrX=%d ScrY=%d",ScrX,ScrY));
	ScrBuf.AllocBuf(Size.X,Size.Y);
	_OT(ViewConsoleInfo());
}

BOOL WINAPI CtrlHandler(DWORD CtrlType)
{
	/*
	    TODO: need handle
	       CTRL_CLOSE_EVENT
	       CTRL_LOGOFF_EVENT
	       CTRL_SHUTDOWN_EVENT
	*/
	if (CtrlType==CTRL_C_EVENT || CtrlType==CTRL_BREAK_EVENT)
	{
		if (CtrlType==CTRL_BREAK_EVENT)
			WriteInput(KEY_BREAK);

		if (CtrlObject && CtrlObject->Cp())
		{
			if (CtrlObject->Cp()->LeftPanel && CtrlObject->Cp()->LeftPanel->GetMode()==PLUGIN_PANEL)
				CtrlObject->Plugins.ProcessEvent(CtrlObject->Cp()->LeftPanel->GetPluginHandle(),FE_BREAK,(void *)(DWORD_PTR)CtrlType);

			if (CtrlObject->Cp()->RightPanel && CtrlObject->Cp()->RightPanel->GetMode()==PLUGIN_PANEL)
				CtrlObject->Plugins.ProcessEvent(CtrlObject->Cp()->RightPanel->GetPluginHandle(),FE_BREAK,(void *)(DWORD_PTR)CtrlType);
		}

		return TRUE;
	}

	CloseFAR=TRUE;

	/* $ 30.08.2001 IS
	   Ïðè çàêðûòèè îêíà "ïî êðåñòó" âñåãäà âîçâðàùàåì TRUE, â ïðîòèâíîì ñëó÷àå
	   Ôàð áóäåò çàêðûò ñèñòåìîé è íå áóäóò âûïîëíåíû îáû÷íûå ïðè çàêðûòèè
	   ïðîöåäóðû: îïîâåùåíû ïëàãèíû, âûçâàíû äåñòðóêòîðû, ñîõðàíåíû íàñòðîéêè è
	   ò.ï.
	*/
	if (!Opt.CloseConsoleRule)
	{
		if ((FileEditor::CurrentEditor && FileEditor::CurrentEditor->IsFileModified()) ||
		        (FrameManager && FrameManager->IsAnyFrameModified(FALSE)))
			return TRUE;

		return FALSE;
	}

	return TRUE;
}


void ShowTime(int ShowAlways)
{
	string strClockText;
	static SYSTEMTIME lasttm={0,0,0,0,0,0,0,0};
	SYSTEMTIME tm;
	WINPORT(GetLocalTime)(&tm);
	CHAR_INFO ScreenClockText[5];
	GetText(ScrX-4,0,ScrX,0,ScreenClockText,sizeof(ScreenClockText));

	if (ShowAlways==2)
	{
		memset(&lasttm,0,sizeof(lasttm));
		return;
	}

	if ((!ShowAlways && lasttm.wMinute==tm.wMinute && lasttm.wHour==tm.wHour &&
	        ScreenClockText[2].Char.UnicodeChar==L':') || ScreenSaverActive)
		return;

	ProcessShowClock++;
	lasttm=tm;
	strClockText.Format(L"%02d:%02d",tm.wHour,tm.wMinute);
	GotoXY(ScrX-4,0);
	// Çäåñü õðåíü êàêàÿ-òî ïîëó÷àåòñÿ ñ ModType - âñå âðåìÿ íå âåðíîå çíà÷åíèå!
	Frame *CurFrame=FrameManager->GetCurrentFrame();

	if (CurFrame)
	{
		int ModType=CurFrame->GetType();
		SetColor(ModType==MODALTYPE_VIEWER?COL_VIEWERCLOCK:
		         (ModType==MODALTYPE_EDITOR?COL_EDITORCLOCK:COL_CLOCK));
		Text(strClockText);
		//ScrBuf.Flush();
	}

	ProcessShowClock--;
}

void GotoXY(int X,int Y)
{
	CurX=X;
	CurY=Y;
}


int WhereX()
{
	return(CurX);
}


int WhereY()
{
	return(CurY);
}


void MoveCursor(int X,int Y)
{
	ScrBuf.MoveCursor(X,Y);
}


void GetCursorPos(SHORT& X,SHORT& Y)
{
	ScrBuf.GetCursorPos(X,Y);
}


void SetCursorType(bool Visible, DWORD Size)
{
	if (Size==(DWORD)-1 || !Visible)
		Size=IsFullscreen()?
		     (Opt.CursorSize[1]?Opt.CursorSize[1]:InitialCursorInfo.dwSize):
				     (Opt.CursorSize[0]?Opt.CursorSize[0]:InitialCursorInfo.dwSize);

	ScrBuf.SetCursorType(Visible,Size);
}

void SetInitialCursorType()
{
	ScrBuf.SetCursorType(InitialCursorInfo.bVisible!=FALSE,InitialCursorInfo.dwSize);
}


void GetCursorType(bool& Visible, DWORD& Size)
{
	ScrBuf.GetCursorType(Visible,Size);
}


void MoveRealCursor(int X,int Y)
{
	COORD C={X,Y};
	Console.SetCursorPosition(C);
}


void GetRealCursorPos(SHORT& X,SHORT& Y)
{
	COORD CursorPosition;
	Console.GetCursorPosition(CursorPosition);
	X=CursorPosition.X;
	Y=CursorPosition.Y;
}

void InitRecodeOutTable()
{
	for (size_t i=0; i<ARRAYSIZE(Oem2Unicode); i++)
	{
		char c = static_cast<char>(i);
		WINPORT(MultiByteToWideChar)(CP_OEMCP, MB_USEGLYPHCHARS, &c, 1, &Oem2Unicode[i], 1);
	}

	if (Opt.CleanAscii)
	{
		for (size_t i=0; i<0x20; i++)
			Oem2Unicode[i]=L'.';

		Oem2Unicode[0x07]=L'*';
		Oem2Unicode[0x10]=L'>';
		Oem2Unicode[0x11]=L'<';
		Oem2Unicode[0x15]=L'$';
		Oem2Unicode[0x16]=L'-';
		Oem2Unicode[0x18]=L'|';
		Oem2Unicode[0x19]=L'V';
		Oem2Unicode[0x1A]=L'>';
		Oem2Unicode[0x1B]=L'<';
		Oem2Unicode[0x1E]=L'X';
		Oem2Unicode[0x1F]=L'X';

		Oem2Unicode[0x7F]=L'.';
	}

	if (Opt.NoGraphics)
	{
		for (int i=0xB3; i<=0xDA; i++)
		{
			Oem2Unicode[i]=L'+';
		}
		Oem2Unicode[0xB3]=L'|';
		Oem2Unicode[0xBA]=L'|';
		Oem2Unicode[0xC4]=L'-';
		Oem2Unicode[0xCD]=L'=';
	}

	{
		static WCHAR _BoxSymbols[48] =
		{
			0x2591, 0x2592, 0x2593, 0x2502, 0x2524, 0x2561, 0x2562, 0x2556,
			0x2555, 0x2563, 0x2551, 0x2557, 0x255D, 0x255C, 0x255B, 0x2510,
			0x2514, 0x2534, 0x252C, 0x251C, 0x2500, 0x253C, 0x255E, 0x255F,
			0x255A, 0x2554, 0x2569, 0x2566, 0x2560, 0x2550, 0x256C, 0x2567,
			0x2568, 0x2564, 0x2565, 0x2559, 0x2558, 0x2552, 0x2553, 0x256B,
			0x256A, 0x2518, 0x250C, 0x2588, 0x2584, 0x258C, 0x2590, 0x2580,
		};
		// ïåðåä [ïåðå]èíèöèàëèçàöèåé âîññòàíîâèì áóôåð (ëèáî èç ðååñòðà, ëèáî...)
		GetRegKey(L"System",L"BoxSymbols",(BYTE *)BoxSymbols,(BYTE*)_BoxSymbols,sizeof(_BoxSymbols));

		if (Opt.NoGraphics)
		{
			for (int i=BS_V1; i<=BS_LT_H1V1; i++)
				BoxSymbols[i]=L'+';

			BoxSymbols[BS_V1]=BoxSymbols[BS_V2]=L'|';
			BoxSymbols[BS_H1]=L'-';
			BoxSymbols[BS_H2]=L'=';
		}
	}

	//_SVS(SysLogDump("Oem2Unicode",0,(LPBYTE)Oem2Unicode,sizeof(Oem2Unicode),nullptr));
}


void Text(int X, int Y, int Color, const WCHAR *Str)
{
	CurColor=FarColorToReal(Color);
	CurX=X;
	CurY=Y;
	Text(Str);
}

void Text(const WCHAR *Str)
{
	size_t Length=StrLength(Str);

	if (Length<=0)
		return;

	CHAR_INFO *BufPtr = (CHAR_INFO *)alloca(sizeof(CHAR_INFO)*Length);

	for (size_t i=0; i < Length; i++)
	{
		BufPtr[i].Char.UnicodeChar=Str[i];
		BufPtr[i].Attributes=CurColor;
	}

	ScrBuf.Write(CurX, CurY, BufPtr, static_cast<int>(Length));
	CurX+=static_cast<int>(Length);
}


void Text(int MsgId)
{
	Text(MSG(MsgId));
}

void VText(const WCHAR *Str)
{
	int Length=StrLength(Str);

	if (Length<=0)
		return;

	int StartCurX=CurX;
	WCHAR ChrStr[2]={0,0};

	for (int I=0; I<Length; I++)
	{
		GotoXY(CurX,CurY);
		ChrStr[0]=Str[I];
		Text(ChrStr);
		CurY++;
		CurX=StartCurX;
	}
}

void HiText(const wchar_t *Str,int HiColor,int isVertText)
{
	string strTextStr;
	int SaveColor;
	size_t pos;
	strTextStr = Str;

	if (!strTextStr.Pos(pos,L'&'))
	{
		if (isVertText)
			VText(strTextStr);
		else
			Text(strTextStr);
	}
	else
	{
		/*
		   &&      = '&'
		   &&&     = '&'
		              ^H
		   &&&&    = '&&'
		   &&&&&   = '&&'
		              ^H
		   &&&&&&  = '&&&'
		*/
		wchar_t *ChPtr = strTextStr.GetBuffer() + pos;
		int I=0;
		wchar_t *ChPtr2=ChPtr;

		while (*ChPtr2++ == L'&')
			++I;

		if (I&1) // íå÷åò?
		{
			*ChPtr=0;

			if (isVertText)
				VText(strTextStr);
			else
				Text(strTextStr); //BUGBUG BAD!!!

			if (ChPtr[1])
			{
				wchar_t Chr[]={ChPtr[1],0};
				SaveColor=CurColor;
				SetColor(HiColor);

				if (isVertText)
					VText(Chr);
				else
					Text(Chr);

				SetColor(SaveColor);
				string strText = (ChPtr+1);
				strTextStr.ReleaseBuffer();
				ReplaceStrings(strText,L"&&",L"&",-1);

				if (isVertText)
					VText(strText.CPtr()+1);
				else
					Text(strText.CPtr()+1);
			}
		}
		else
		{
			strTextStr.ReleaseBuffer();
			ReplaceStrings(strTextStr,L"&&",L"&",-1);

			if (isVertText)
				VText(strTextStr);
			else
				Text(strTextStr); //BUGBUG BAD!!!
		}
	}
}



void SetScreen(int X1,int Y1,int X2,int Y2,wchar_t Ch,int Color)
{
	if (X1<0) X1=0;

	if (Y1<0) Y1=0;

	if (X2>ScrX) X2=ScrX;

	if (Y2>ScrY) Y2=ScrY;

	ScrBuf.FillRect(X1,Y1,X2,Y2,Ch,FarColorToReal(Color));
}


void MakeShadow(int X1,int Y1,int X2,int Y2)
{
	if (X1<0) X1=0;

	if (Y1<0) Y1=0;

	if (X2>ScrX) X2=ScrX;

	if (Y2>ScrY) Y2=ScrY;

	ScrBuf.ApplyColorMask(X1,Y1,X2,Y2,0xF8);
}

void ChangeBlockColor(int X1,int Y1,int X2,int Y2,int Color)
{
	if (X1<0) X1=0;

	if (Y1<0) Y1=0;

	if (X2>ScrX) X2=ScrX;

	if (Y2>ScrY) Y2=ScrY;

	ScrBuf.ApplyColor(X1,Y1,X2,Y2,FarColorToReal(Color));
}

void mprintf(const WCHAR *fmt,...)
{
	va_list argptr;
	va_start(argptr,fmt);
	WCHAR OutStr[2048];
	vswprintf(OutStr,ARRAYSIZE(OutStr)-1,fmt,argptr);
	Text(OutStr);
	va_end(argptr);
}

void vmprintf(const WCHAR *fmt,...)
{
	va_list argptr;
	va_start(argptr,fmt);
	WCHAR OutStr[2048];
	vswprintf(OutStr,ARRAYSIZE(OutStr)-1,fmt,argptr);
	VText(OutStr);
	va_end(argptr);
}


void SetColor(int Color)
{
	CurColor=FarColorToReal(Color);
}

void SetRealColor(int Color)
{
	CurColor=FarColorToReal(Color);
	Console.SetTextAttributes(CurColor);
}

void ClearScreen(int Color)
{
	Color=FarColorToReal(Color);
	ScrBuf.FillRect(0,0,ScrX,ScrY,L' ',Color);
	if(Opt.WindowMode)
	{
		Console.ClearExtraRegions(Color);
	}
	ScrBuf.ResetShadow();
	ScrBuf.Flush();
	Console.SetTextAttributes(Color);
}

int GetColor()
{
	return(CurColor);
}


void ScrollScreen(int Count)
{
	ScrBuf.Scroll(Count);
	ScrBuf.FillRect(0,ScrY+1-Count,ScrX,ScrY,L' ',FarColorToReal(COL_COMMANDLINEUSERSCREEN));
}


void GetText(int X1,int Y1,int X2,int Y2,void *Dest,int DestSize)
{
	ScrBuf.Read(X1,Y1,X2,Y2,(CHAR_INFO *)Dest,DestSize);
}

void PutText(int X1,int Y1,int X2,int Y2,const void *Src)
{
	int Width=X2-X1+1;
	int Y;
	CHAR_INFO *SrcPtr=(CHAR_INFO*)Src;

	for (Y=Y1; Y<=Y2; ++Y,SrcPtr+=Width)
		ScrBuf.Write(X1,Y,SrcPtr,Width);
}

void BoxText(wchar_t Chr)
{
	wchar_t Str[]={Chr,L'\0'};
	BoxText(Str);
}


void BoxText(const wchar_t *Str,int IsVert)
{
	if (IsVert)
		VText(Str);
	else
		Text(Str);
}


/*
   Îòðèñîâêà ïðÿìîóãîëüíèêà.
*/
void Box(int x1,int y1,int x2,int y2,int Color,int Type)
{
	if (x1>=x2 || y1>=y2)
		return;

	SetColor(Color);
	Type=(Type==DOUBLE_BOX || Type==SHORT_DOUBLE_BOX);

	const size_t height=y2-y1;
	const size_t width=x2-x1+2;
	LPWSTR BufPtr = (LPWSTR)alloca(sizeof(WCHAR)*std::max(width, height));
	wmemset(BufPtr, BoxSymbols[Type?BS_V2:BS_V1], height-1);
	BufPtr[height-1]=0;
	GotoXY(x1,y1+1);
	VText(BufPtr);
	GotoXY(x2,y1+1);
	VText(BufPtr);
	BufPtr[0]=BoxSymbols[Type?BS_LT_H2V2:BS_LT_H1V1];
	wmemset(BufPtr+1, BoxSymbols[Type?BS_H2:BS_H1], width-3);
	BufPtr[width-2]=BoxSymbols[Type?BS_RT_H2V2:BS_RT_H1V1];
	BufPtr[width-1]=0;
	GotoXY(x1,y1);
	Text(BufPtr);
	BufPtr[0]=BoxSymbols[Type?BS_LB_H2V2:BS_LB_H1V1];
	BufPtr[width-2]=BoxSymbols[Type?BS_RB_H2V2:BS_RB_H1V1];
	GotoXY(x1,y2);
	Text(BufPtr);
}

bool ScrollBarRequired(UINT Length, UINT64 ItemsCount)
{
	return Length>2 && ItemsCount && Length<ItemsCount;
}

bool ScrollBarEx(UINT X1,UINT Y1,UINT Length,UINT64 TopItem,UINT64 ItemsCount)
{
	if (ScrollBarRequired(Length, ItemsCount))
	{
		Length-=2;
		ItemsCount-=2;
		UINT CaretPos=static_cast<UINT>(Round(Length*TopItem,ItemsCount));
		UINT CaretLength=Max(1U,static_cast<UINT>(Round(static_cast<UINT64>(Length*Length),ItemsCount)));

		if (!CaretPos && TopItem)
		{
			CaretPos++;
		}
		else if (CaretPos+CaretLength==Length && TopItem+2+Length<ItemsCount)
		{
			CaretPos--;
		}

		CaretPos=Min(CaretPos,Length-CaretLength);
		LPWSTR BufPtr = (LPWSTR)alloca(sizeof(WCHAR)*(Length + 3));
		wmemset(BufPtr+1,BoxSymbols[BS_X_B0],Length);
		BufPtr[0]=Oem2Unicode[0x1E];

		for (size_t i=0; i<CaretLength; i++)
			BufPtr[CaretPos+1+i]=BoxSymbols[BS_X_B2];

		BufPtr[Length+1]=Oem2Unicode[0x1F];
		BufPtr[Length+2]=0;
		GotoXY(X1,Y1);
		VText(BufPtr);
		return true;
	}

	return false;
}

void ScrollBar(int X1,int Y1,int Length, unsigned int Current, unsigned int Total)
{
	int ThumbPos;

	if ((Length-=2)<1)
		return;

	if (Total>0)
		ThumbPos=Length*Current/Total;
	else
		ThumbPos=0;

	if (ThumbPos>=Length)
		ThumbPos=Length-1;

	GotoXY(X1,Y1);
	{
		LPWSTR BufPtr = (LPWSTR)alloca(sizeof(WCHAR)*(Length + 3));
		wmemset(BufPtr+1,BoxSymbols[BS_X_B0],Length);
		BufPtr[ThumbPos+1]=BoxSymbols[BS_X_B2];
		BufPtr[0]=Oem2Unicode[0x1E];
		BufPtr[Length+1]=Oem2Unicode[0x1F];
		BufPtr[Length+2]=0;
		VText(BufPtr);
	}
}

void DrawLine(int Length,int Type, const wchar_t* UserSep)
{
	if (Length>1)
	{
		LPWSTR BufPtr = (LPWSTR)alloca(sizeof(WCHAR)*Length);
		MakeSeparator(Length,BufPtr,Type,UserSep);

		(Type >= 4 && Type <= 7) || (Type >= 10 && Type <= 11)? VText(BufPtr) : Text(BufPtr);
	}
}

// "Íàðèñîâàòü" ñåïàðàòîð â ïàìÿòè.
WCHAR* MakeSeparator(int Length,WCHAR *DestStr,int Type, const wchar_t* UserSep)
{
	wchar_t BoxType[12][3]=
	{
		// h-horiz, s-space, v-vert, b-border, 1-one line, 2-two line
		/* 00 */{L' ',                 L' ',                 BoxSymbols[BS_H1]}, //  -     h1s
		/* 01 */{BoxSymbols[BS_L_H1V2],BoxSymbols[BS_R_H1V2],BoxSymbols[BS_H1]}, // ||-||  h1b2
		/* 02 */{BoxSymbols[BS_L_H1V1],BoxSymbols[BS_R_H1V1],BoxSymbols[BS_H1]}, // |-|    h1b1
		/* 03 */{BoxSymbols[BS_L_H2V2],BoxSymbols[BS_R_H2V2],BoxSymbols[BS_H2]}, // ||=||  h2b2

		/* 04 */{L' ',                 L' ',                 BoxSymbols[BS_V1]}, //  |     v1s
		/* 05 */{BoxSymbols[BS_T_H2V1],BoxSymbols[BS_B_H2V1],BoxSymbols[BS_V1]}, // =|=    v1b2
		/* 06 */{BoxSymbols[BS_T_H1V1],BoxSymbols[BS_B_H1V1],BoxSymbols[BS_V1]}, // -|-    v1b1
		/* 07 */{BoxSymbols[BS_T_H2V2],BoxSymbols[BS_B_H2V2],BoxSymbols[BS_V2]}, // =||=   v2b2

		/* 08 */{BoxSymbols[BS_H1],    BoxSymbols[BS_H1],    BoxSymbols[BS_H1]}, // -      h1
		/* 09 */{BoxSymbols[BS_H2],    BoxSymbols[BS_H2],    BoxSymbols[BS_H2]}, // =      h2
		/* 10 */{BoxSymbols[BS_V1],    BoxSymbols[BS_V1],    BoxSymbols[BS_V1]}, // |      v1
		/* 11 */{BoxSymbols[BS_V2],    BoxSymbols[BS_V2],    BoxSymbols[BS_V2]}, // ||     v2
	};

	if (Length>1 && DestStr)
	{
		Type%=ARRAYSIZE(BoxType);
		wmemset(DestStr,BoxType[Type][2],Length);
		DestStr[0]=BoxType[Type][0];
		DestStr[Length-1]=BoxType[Type][1];
		DestStr[Length]=0;
	}

	return DestStr;
}

string& HiText2Str(string& strDest, const wchar_t *Str)
{
	const wchar_t *ChPtr;
	strDest = Str;

	if ((ChPtr=wcschr(Str,L'&')) )
	{
		/*
		   &&      = '&'
		   &&&     = '&'
		              ^H
		   &&&&    = '&&'
		   &&&&&   = '&&'
		              ^H
		   &&&&&&  = '&&&'
		*/
		int I=0;
		const wchar_t *ChPtr2=ChPtr;

		while (*ChPtr2++ == L'&')
			++I;

		if (I&1) // íå÷åò?
		{
			strDest.SetLength(ChPtr-Str);

			if (ChPtr[1])
			{
				wchar_t Chr[]={ChPtr[1],0};
				strDest+=Chr;
				string strText = (ChPtr+1);
				ReplaceStrings(strText,L"&&",L"&",-1);
				strDest+=strText.CPtr()+1;
			}
		}
		else
		{
			ReplaceStrings(strDest,L"&&",L"&",-1);
		}
	}

	return strDest;
}

int HiStrlen(const wchar_t *Str)
{
	/*
			&&      = '&'
			&&&     = '&'
                       ^H
			&&&&    = '&&'
			&&&&&   = '&&'
                       ^H
			&&&&&&  = '&&&'
	*/

	int Length=0;
	bool Hi=false;

	if (Str)
	{
		while (*Str)
		{
			if (*Str == L'&')
			{
				int Count=0;

				while (*Str == L'&')
				{
					Str++;
					Count++;
				}

				if (Count&1) //íå÷¸ò?
				{
					if (Hi)
						Length+=+1;
					else
						Hi=true;
				}

				Length+=Count/2;
			}
			else
			{
				Str++;
				Length++;
			}
		}
	}

	return Length;

}

int HiFindRealPos(const wchar_t *Str, int Pos, BOOL ShowAmp)
{
	/*
			&&      = '&'
			&&&     = '&'
                       ^H
			&&&&    = '&&'
			&&&&&   = '&&'
                       ^H
			&&&&&&  = '&&&'
	*/

	if (ShowAmp)
	{
		return Pos;
	}

	int RealPos = 0;
	int VisPos = 0;

	if (Str)
	{
		while (VisPos < Pos && *Str)
		{
			if (*Str == L'&')
			{
				Str++;
				RealPos++;

				if (*Str == L'&' && *(Str+1) == L'&' && *(Str+2) != L'&')
				{
					Str++;
					RealPos++;
				}
			}

			Str++;
			VisPos++;
			RealPos++;
		}
	}

	return RealPos;
}

int HiFindNextVisualPos(const wchar_t *Str, int Pos, int Direct)
{
	/*
			&&      = '&'
			&&&     = '&'
                       ^H
			&&&&    = '&&'
			&&&&&   = '&&'
                       ^H
			&&&&&&  = '&&&'
	*/

	if (Str)
	{
		if (Direct < 0)
		{
			if (!Pos || Pos == 1)
				return 0;

			if (Str[Pos-1] != L'&')
			{
				if (Str[Pos-2] == L'&')
				{
					if (Pos-3 >= 0 && Str[Pos-3] == L'&')
						return Pos-1;

					return Pos-2;
				}

				return Pos-1;
			}
			else
			{
				if (Pos-3 >= 0 && Str[Pos-3] == L'&')
					return Pos-3;

				return Pos-2;
			}
		}
		else
		{
			if (!Str[Pos])
				return Pos+1;

			if (Str[Pos] == L'&')
			{
				if (Str[Pos+1] == L'&' && Str[Pos+2] == L'&')
					return Pos+3;

				return Pos+2;
			}
			else
			{
				return Pos+1;
			}
		}
	}

	return 0;
}

bool IsFullscreen()
{
	bool Result=false;
	DWORD ModeFlags=0;
	/*if(Console.GetDisplayMode(ModeFlags) && ModeFlags&CONSOLE_FULLSCREEN_HARDWARE)
	{
		Result=true;
	}*/
	return Result;
}
