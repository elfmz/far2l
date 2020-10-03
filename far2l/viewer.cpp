/*
viewer.cpp

Internal viewer
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

#include <ctype.h>
#include "viewer.hpp"
#include "codepage.hpp"
#include "macroopcode.hpp"
#include "keyboard.hpp"
#include "lang.hpp"
#include "colors.hpp"
#include "keys.hpp"
#include "help.hpp"
#include "dialog.hpp"
#include "panel.hpp"
#include "filepanels.hpp"
#include "fileview.hpp"
#include "savefpos.hpp"
#include "savescr.hpp"
#include "ctrlobj.hpp"
#include "scrbuf.hpp"
#include "TPreRedrawFunc.hpp"
#include "syslog.hpp"
#include "cddrv.hpp"
#include "interf.hpp"
#include "message.hpp"
#include "clipboard.hpp"
#include "delete.hpp"
#include "dirmix.hpp"
#include "pathmix.hpp"
#include "filestr.hpp"
#include "mix.hpp"
#include "constitle.hpp"
#include "console.hpp"
#include "wakeful.hpp"
#include "../utils/include/ConvertUTF.h"

static void PR_ViewerSearchMsg();
static void ViewerSearchMsg(const wchar_t *Name,int Percent);

static int InitHex=FALSE,SearchHex=FALSE;

static int NextViewerID=0;


static int CalcByteDistance(UINT CodePage, const wchar_t* begin, const wchar_t* end)
{
	if (begin > end)
		return -1;
#if (__WCHAR_MAX__ > 0xffff)
	if ((CodePage == CP_UTF32LE) || (CodePage == CP_UTF32BE)) {
		return (end - begin) * 4;
	}

	int distance;

	if ((CodePage == CP_UTF16LE) || (CodePage == CP_UTF16BE)) {
		CalcSpaceUTF32toUTF16(&distance, (const UTF32**)&begin, (const UTF32*)end, lenientConversion);
		distance*= 2;

	} else if (CodePage == CP_UTF8) {
		CalcSpaceUTF32toUTF8(&distance, (const UTF32**)&begin, (const UTF32*)end, lenientConversion);

	} else {// one-byte code page?
		distance = end - begin;
	}

#else
	if ((CodePage == CP_UTF16LE) || (CodePage == CP_UTF16BE)) {
		return (end - begin) * 2;
	}

	int distance;

	if (CodePage == CP_UTF8) {
		CalcSpaceUTF16toUTF8(&distance, (const UTF16**)&begin, (const UTF16*)end, lenientConversion);

	} else {// one-byte code page?
		distance = end - begin;
	}

#endif

	return distance;
}

static int CalcCodeUnitsDistance(UINT CodePage, const wchar_t* begin, const wchar_t* end)
{
	int distance = CalcByteDistance(CodePage, begin, end);
	if (distance > 0) switch (CodePage) {
		case CP_UTF32LE: case CP_UTF32BE: distance/= 4; break;
		case CP_UTF16LE: case CP_UTF16BE: distance/= 2; break;
	}
	return distance;
}


Viewer::Viewer(bool bQuickView, UINT aCodePage):
	ViOpt(Opt.ViOpt),
	m_bQuickView(bQuickView)
{
	_OT(SysLog(L"[%p] Viewer::Viewer()", this));

	for (int i=0; i<=MAXSCRY; i++)
	{
		Strings[i] = new ViewerString();
		Strings[i]->lpData = new wchar_t[MAX_VIEWLINEB];
	}

	strLastSearchStr = strGlobalSearchString;
	LastSearchCase=GlobalSearchCase;
	LastSearchRegexp=Opt.ViOpt.SearchRegexp;
	LastSearchWholeWords=GlobalSearchWholeWords;
	LastSearchReverse=GlobalSearchReverse;
	LastSearchHex=GlobalSearchHex;
	VM.CodePage=DefCodePage=aCodePage;
	// Вспомним тип врапа
	VM.Wrap=Opt.ViOpt.ViewerIsWrap;
	VM.WordWrap=Opt.ViOpt.ViewerWrap;
	VM.Hex=InitHex;
	ViewKeyBar=nullptr;
	FilePos=0;
	LeftPos=0;
	SecondPos=0;
	FileSize=0;
	LastPage=0;
	SelectPos=SelectSize=0;
	LastSelPos=0;
	SetStatusMode(TRUE);
	HideCursor=TRUE;
	DeleteFolder=TRUE;
	CodePageChangedByUser=FALSE;
	memset(&BMSavePos,0xff,sizeof(BMSavePos));
	memset(UndoData,0xff,sizeof(UndoData));
	LastKeyUndo=FALSE;
	InternalKey=FALSE;
	ViewerID=::NextViewerID++;
	CtrlObject->Plugins.CurViewer=this;
	OpenFailed=false;
	HostFileViewer=nullptr;
	SelectPosOffSet=0;
	bVE_READ_Sent = false;
	Signature = false;
}


Viewer::~Viewer()
{
	KeepInitParameters();

	if (ViewFile.Opened())
	{
		ViewFile.Close();

		if (Opt.ViOpt.SavePos && Opt.OnlyEditorViewerUsed != Options::ONLY_VIEWER_ON_CMDOUT)
		{
			FARString strCacheName=strPluginData.IsEmpty()?strFullFileName:strPluginData+PointToName(strFileName);
			UINT CodePage=0;

			if (CodePageChangedByUser)
			{
				CodePage=VM.CodePage;
			}

			PosCache poscache={};
			poscache.Param[0]=FilePos;
			poscache.Param[1]=LeftPos;
			poscache.Param[2]=VM.Hex;
			//=poscache.Param[3];
			poscache.Param[4]=CodePage;

			if (Opt.ViOpt.SaveShortPos)
			{
				poscache.Position[0]=BMSavePos.SavePosAddr;
				poscache.Position[1]=BMSavePos.SavePosLeft;
				//poscache.Position[2]=;
				//poscache.Position[3]=;
			}

			CtrlObject->ViewerPosCache->AddPosition(strCacheName,poscache);
		}
	}

	_tran(SysLog(L"[%p] Viewer::~Viewer, TempViewName=[%ls]",this,TempViewName));
	/* $ 11.10.2001 IS
	   Удаляем файл только, если нет открытых фреймов с таким именем.
	*/

	if (!strTempViewName.IsEmpty() && !FrameManager->CountFramesWithName(strTempViewName))
	{
		/* $ 14.06.2002 IS
		   Если DeleteFolder сброшен, то удаляем только файл. Иначе - удаляем еще
		   и каталог.
		*/
		if (DeleteFolder)
			DeleteFileWithFolder(strTempViewName);
		else
		{
			apiSetFileAttributes(strTempViewName,FILE_ATTRIBUTE_NORMAL);
			apiDeleteFile(strTempViewName); //BUGBUG
		}
	}

	for (int i=0; i<=MAXSCRY; i++)
	{
		delete [] Strings[i]->lpData;
		delete Strings[i];
	}

	if (!OpenFailed && bVE_READ_Sent)
	{
		CtrlObject->Plugins.CurViewer=this; //HostFileViewer;
		CtrlObject->Plugins.ProcessViewerEvent(VE_CLOSE,&ViewerID);
	}
}


void Viewer::KeepInitParameters()
{
	strGlobalSearchString = strLastSearchStr;
	GlobalSearchCase=LastSearchCase;
	GlobalSearchWholeWords=LastSearchWholeWords;
	GlobalSearchReverse=LastSearchReverse;
	GlobalSearchHex=LastSearchHex;
	Opt.ViOpt.ViewerIsWrap=VM.Wrap;
	Opt.ViOpt.ViewerWrap=VM.WordWrap;
	Opt.ViOpt.SearchRegexp=LastSearchRegexp;
	InitHex=VM.Hex;
}


int Viewer::OpenFile(const wchar_t *Name,int warning)
{
	VM.CodePage=DefCodePage;
	DefCodePage=CP_AUTODETECT;
	OpenFailed=false;

	ViewFile.Close();

	SelectSize = 0; // Сбросим выделение
	strFileName = Name;

	DWORD FileAttr = apiGetFileAttributes(Name);
	if (FileAttr!=INVALID_FILE_ATTRIBUTES && (FileAttr&FILE_ATTRIBUTE_DEVICE)!=0) {//avoid stuck
		OpenFailed=TRUE;
		return FALSE;
	}

	ViewFile.Open(strFileName.GetMB());

	if (!ViewFile.Opened())
	{
		/* $ 04.07.2000 tran
		   + 'warning' flag processing, in QuickView it is FALSE
		     so don't show red message box */
		if (warning)
			Message(MSG_WARNING|MSG_ERRORTYPE,1,MSG(MViewerTitle),
			        MSG(MViewerCannotOpenFile),strFileName,MSG(MOk));

		OpenFailed=true;
		return FALSE;
	}

	CodePageChangedByUser=FALSE;

	ConvertNameToFull(strFileName,strFullFileName);
	apiGetFindDataEx(strFileName, ViewFindData);
	UINT CachedCodePage=0;

	if (Opt.ViOpt.SavePos)
	{
		int64_t NewLeftPos,NewFilePos;
		FARString strCacheName=strPluginData.IsEmpty()?strFileName:strPluginData+PointToName(strFileName);
		memset(&BMSavePos,0xff,sizeof(BMSavePos)); //заполним с -1
		PosCache poscache={};

		if (Opt.ViOpt.SaveShortPos)
		{
			poscache.Position[0]=BMSavePos.SavePosAddr;
			poscache.Position[1]=BMSavePos.SavePosLeft;
			//poscache.Position[2]=;
			//poscache.Position[3]=;
		}

		CtrlObject->ViewerPosCache->GetPosition(strCacheName,poscache);
		NewFilePos=poscache.Param[0];
		NewLeftPos=poscache.Param[1];
		VM.Hex=(int)poscache.Param[2];
		//=poscache.Param[3];
		CachedCodePage=(UINT)poscache.Param[4];

		// Проверяем поддерживается или нет загруженная из кэша кодовая страница
		if (CachedCodePage && !IsCodePageSupported(CachedCodePage))
			CachedCodePage = 0;
		LastSelPos=FilePos=NewFilePos;
		LeftPos=NewLeftPos;
	}
	else
	{
		FilePos=0;
	}

	/* $ 26.07.2002 IS
	     Автоопределение Unicode не должно зависеть от опции
	     "Автоопределение таблицы символов", т.к. Unicode не есть
	     _таблица символов_ для перекодировки.
	*/
	//if(ViOpt.AutoDetectTable)
	{
		bool Detect=false;
		UINT CodePage=0;

		if (VM.CodePage == CP_AUTODETECT || IsUnicodeOrUtfCodePage(VM.CodePage))
		{
			File f;
			f.Open(strFileName, GENERIC_READ, FILE_SHARE_READ|FILE_SHARE_WRITE, nullptr, OPEN_EXISTING ,FILE_ATTRIBUTE_NORMAL);
			Detect=GetFileFormat(f,CodePage,&Signature,Opt.ViOpt.AutoDetectCodePage!=0);

			// Проверяем поддерживается или нет задетектированная кодовая страница
			if (Detect)
				Detect = IsCodePageSupported(CodePage);
		}

		if (VM.CodePage==CP_AUTODETECT)
		{
			if (Detect)
			{
				VM.CodePage=CodePage;
			}

			if (CachedCodePage)
			{
				VM.CodePage=CachedCodePage;
				CodePageChangedByUser=TRUE;
			}

			if (VM.CodePage==CP_AUTODETECT)
				VM.CodePage = Opt.ViOpt.UTF8CodePageAsDefault ? CP_UTF8 : CP_KOI8R;
		}
		else
		{
			CodePageChangedByUser=TRUE;
		}

		// BUGBUG
		// пока что запретим переключать hex в UTF8/UTF32, ибо не работает.
		if (VM.Hex && (VM.CodePage==CP_UTF8 || VM.CodePage==CP_UTF32LE || VM.CodePage==CP_UTF32BE))
		{
			VM.CodePage=WINPORT(GetACP)();
		}

		if (!IsUnicodeOrUtfCodePage(VM.CodePage))
		{
			ViewFile.SetPointer(0);
		}
	}
	SetFileSize();

	if (FilePos>FileSize)
		FilePos=0;

	SetCRSym();
//  if (ViOpt.AutoDetectTable && !TableChangedByUser)
//  {
//  }
	ChangeViewKeyBar();
	AdjustWidth();
	CtrlObject->Plugins.CurViewer=this; // HostFileViewer;
	/* $ 15.09.2001 tran
	   пора легализироваться */
	CtrlObject->Plugins.ProcessViewerEvent(VE_READ,nullptr);
	bVE_READ_Sent = true;
	return TRUE;
}


/* $ 27.04.2001 DJ
   функция вычисления ширины в зависимости от наличия скроллбара
*/

void Viewer::AdjustWidth()
{
	Width=X2-X1+1;
	XX2=X2;

	if (ViOpt.ShowScrollbar && !m_bQuickView)
	{
		Width--;
		XX2--;
	}
}

void Viewer::SetCRSym()
{
	if (!ViewFile.Opened())
		return;

	wchar_t Buf[2048];
	int CRCount=0,LFCount=0;
	int ReadSize,I;
	vseek(0,SEEK_SET);
	ReadSize=vread(Buf,ARRAYSIZE(Buf));

	for (I=0; I<ReadSize; I++)
		switch (Buf[I])
		{
			case L'\n':
				LFCount++;
				break;
			case L'\r':

				if (I+1>=ReadSize || Buf[I+1]!=L'\n')
					CRCount++;

				break;
		}

	if (LFCount<CRCount)
		CRSym=L'\r';
	else
		CRSym=L'\n';
}

void Viewer::ShowPage(int nMode)
{
	int I,Y;
	AdjustWidth();

	if (!ViewFile.Opened())
	{
		if (!strFileName.IsEmpty() && ((nMode == SHOW_RELOAD) || (nMode == SHOW_HEX)))
		{
			SetScreen(X1,Y1,X2,Y2,L' ',COL_VIEWERTEXT);
			GotoXY(X1,Y1);
			SetColor(COL_WARNDIALOGTEXT);
			FS<<fmt::Precision(XX2-X1+1)<<MSG(MViewerCannotOpenFile);
			ShowStatus();
		}

		return;
	}

	if (HideCursor)
		SetCursorType(0,10);

	vseek(FilePos,SEEK_SET);

	if (!SelectSize)
		SelectPos=FilePos;

	switch (nMode)
	{
		case SHOW_HEX:
			CtrlObject->Plugins.CurViewer = this; //HostFileViewer;
			ShowHex();
			break;
		case SHOW_RELOAD:
			CtrlObject->Plugins.CurViewer = this; //HostFileViewer;

			for (I=0,Y=Y1; Y<=Y2; Y++,I++)
			{
				Strings[I]->nFilePos = vtell();

				if (Y==Y1+1 && !ViewFile.Eof())
					SecondPos=vtell();

				ReadString(Strings[I],-1,MAX_VIEWLINEB);
			}

			break;
		case SHOW_UP:

			for (I=Y2-Y1-1; I>=0; I--)
			{
				Strings[I+1]->nFilePos = Strings[I]->nFilePos;
				Strings[I+1]->nSelStart = Strings[I]->nSelStart;
				Strings[I+1]->nSelEnd = Strings[I]->nSelEnd;
				Strings[I+1]->bSelection = Strings[I]->bSelection;
				wcscpy(Strings[I+1]->lpData, Strings[I]->lpData);
			}

			Strings[0]->nFilePos = FilePos;
			SecondPos = Strings[1]->nFilePos;
			ReadString(Strings[0],(int)(SecondPos-FilePos),MAX_VIEWLINEB);
			break;
		case SHOW_DOWN:

			for (I=0; I<Y2-Y1; I++)
			{
				Strings[I]->nFilePos = Strings[I+1]->nFilePos;
				Strings[I]->nSelStart = Strings[I+1]->nSelStart;
				Strings[I]->nSelEnd = Strings[I+1]->nSelEnd;
				Strings[I]->bSelection = Strings[I+1]->bSelection;
				wcscpy(Strings[I]->lpData, Strings[I+1]->lpData);
			}

			FilePos = Strings[0]->nFilePos;
			SecondPos = Strings[1]->nFilePos;
			vseek(Strings[Y2-Y1]->nFilePos, SEEK_SET);
			ReadString(Strings[Y2-Y1],-1,MAX_VIEWLINEB);
			Strings[Y2-Y1]->nFilePos = vtell();
			ReadString(Strings[Y2-Y1],-1,MAX_VIEWLINEB);
			break;
	}

	if (nMode != SHOW_HEX)
	{
		for (I=0,Y=Y1; Y<=Y2; Y++,I++)
		{
			int StrLen = StrLength(Strings[I]->lpData);
			SetColor(COL_VIEWERTEXT);
			GotoXY(X1,Y);

			if (StrLen > LeftPos)
			{
				if (IsUnicodeOrUtfCodePage(VM.CodePage) && Signature && !I && !Strings[I]->nFilePos)
				{
					FS<<fmt::LeftAlign()<<fmt::Width(Width)<<fmt::Precision(Width)<<&Strings[I]->lpData[static_cast<size_t>(LeftPos+1)];
				}
				else
				{
					FS<<fmt::LeftAlign()<<fmt::Width(Width)<<fmt::Precision(Width)<<&Strings[I]->lpData[static_cast<size_t>(LeftPos)];
				}
			}
			else
			{
				FS<<fmt::Width(Width)<<L"";
			}

			if (SelectSize && Strings[I]->bSelection)
			{
				int64_t SelX1;

				if (LeftPos > Strings[I]->nSelStart)
					SelX1 = X1;
				else
					SelX1 = Strings[I]->nSelStart-LeftPos;

				if (!VM.Wrap && (Strings[I]->nSelStart < LeftPos || Strings[I]->nSelStart > LeftPos+XX2-X1))
				{
					if (AdjustSelPosition)
					{
						LeftPos = Strings[I]->nSelStart-1;
						AdjustSelPosition = FALSE;
						Show();
						return;
					}
				}
				else
				{
					SetColor(COL_VIEWERSELECTEDTEXT);
					GotoXY(static_cast<int>(X1+SelX1),Y);
					int64_t Length = Strings[I]->nSelEnd-Strings[I]->nSelStart;

					if (LeftPos > Strings[I]->nSelStart)
						Length = Strings[I]->nSelEnd-LeftPos;

					if (LeftPos > Strings[I]->nSelEnd)
						Length = 0;

					FS<<fmt::Precision(static_cast<int>(Length))<<&Strings[I]->lpData[static_cast<size_t>(SelX1+LeftPos+SelectPosOffSet)];
				}
			}

			if (StrLen > LeftPos + Width && ViOpt.ShowArrows)
			{
				GotoXY(XX2,Y);
				SetColor(COL_VIEWERARROWS);
				BoxText(0xbb);
			}

			if (LeftPos>0 && *Strings[I]->lpData  && ViOpt.ShowArrows)
			{
				GotoXY(X1,Y);
				SetColor(COL_VIEWERARROWS);
				BoxText(0xab);
			}
		}
	}

	DrawScrollbar();
	ShowStatus();
}

void Viewer::DisplayObject()
{
	ShowPage(VM.Hex?SHOW_HEX:SHOW_RELOAD);
}

void Viewer::ShowHex()
{
	wchar_t OutStr[MAX_VIEWLINE],TextStr[20];
	int EndFile;
//	int64_t SelSize;
	WCHAR Ch;
	int X,Y,TextPos;
	int SelStart, SelEnd;
	bool bSelStartFound = false, bSelEndFound = false;
	int64_t HexLeftPos=((LeftPos>80-ObjWidth) ? Max(80-ObjWidth,0):LeftPos);

	for (EndFile=0,Y=Y1; Y<=Y2; Y++)
	{
		bSelStartFound = false;
		bSelEndFound = false;
//		SelSize=0;
		SetColor(COL_VIEWERTEXT);
		GotoXY(X1,Y);

		if (EndFile)
		{
			FS<<fmt::Width(ObjWidth)<<L"";
			continue;
		}

		if (Y==Y1+1 && !ViewFile.Eof())
			SecondPos=vtell();

		INT64 Ptr=0;
		ViewFile.GetPointer(Ptr);
		swprintf(OutStr,ARRAYSIZE(OutStr),L"%010llX: ", Ptr);
		TextPos=0;
		int HexStrStart = (int)wcslen(OutStr);
		SelStart = HexStrStart;
		SelEnd = SelStart;
		int64_t fpos = vtell();

		if (fpos > SelectPos)
			bSelStartFound = true;

		if (fpos < SelectPos+SelectSize-1)
			bSelEndFound = true;

		if (!SelectSize)
			bSelStartFound = bSelEndFound = false;

		const wchar_t BorderLine[]={BoxSymbols[BS_V1],L' ',0};

		if (VM.CodePage==CP_UTF16LE || VM.CodePage==CP_UTF16BE)
		{
			for (X=0; X<8; X++)
			{
				int64_t fpos = vtell();

				if (SelectSize>0 && (SelectPos == fpos))
				{
					bSelStartFound = true;
					SelStart = (int)wcslen(OutStr);
//					SelSize=SelectSize;
					/* $ 22.01.2001 IS
					    Внимание! Возможно, это не совсем верное решение проблемы
					    выделения из плагинов, но мне пока другого в голову не пришло.
					    Я приравниваю SelectSize нулю в Process*
					*/
					//SelectSize=0;
				}

				if (SelectSize>0 && (fpos == (SelectPos+SelectSize-1)))
				{
					bSelEndFound = true;
					SelEnd = (int)wcslen(OutStr)+3;
//					SelSize=SelectSize;
				}

				if (!vgetc(Ch))
				{
					/* $ 28.06.2000 tran
					   убираем показ пустой строки, если длина
					   файла кратна 16 */
					EndFile=1;
					LastPage=1;

					if (!X)
					{
						wcscpy(OutStr,L"");
						break;
					}

					wcscat(OutStr,L"     ");
					TextStr[TextPos++]=L' ';
				}
				else
				{
					WCHAR OutChar = Ch;

					if (VM.CodePage == CP_UTF16BE) {
						swab((char*)&OutChar, (char*)&OutChar, sizeof(OutChar));
					}


					int OutStrLen=StrLength(OutStr);
					swprintf(OutStr+OutStrLen,ARRAYSIZE(OutStr)-OutStrLen,L"%02X%02X ", (unsigned int)HIBYTE(OutChar), (unsigned int)LOBYTE(OutChar));

					if (!Ch)
					{
						Ch=L' ';
					}

					TextStr[TextPos++]=Ch;
					LastPage=0;
				}

				if (X==3)
					wcscat(OutStr, BorderLine);
			}
		}
		else
		{
			for (X=0; X<16; X++)
			{
				int64_t fpos = vtell();

				if (SelectSize>0 && (SelectPos == fpos))
				{
					bSelStartFound = true;
					SelStart = (int)wcslen(OutStr);
//					SelSize=SelectSize;
					/* $ 22.01.2001 IS
					    Внимание! Возможно, это не совсем верное решение проблемы
					    выделения из плагинов, но мне пока другого в голову не пришло.
					    Я приравниваю SelectSize нулю в Process*
					*/
					//SelectSize=0;
				}

				if (SelectSize>0 && (fpos == (SelectPos+SelectSize-1)))
				{
					bSelEndFound = true;
					SelEnd = (int)wcslen(OutStr)+1;
//					SelSize=SelectSize;
				}

				if (!vgetc(Ch))
				{
					/* $ 28.06.2000 tran
					   убираем показ пустой строки, если длина
					   файла кратна 16 */
					EndFile=1;
					LastPage=1;

					if (!X)
					{
						wcscpy(OutStr,L"");
						break;
					}

					/* $ 03.07.2000 tran
					   - вместо 5 пробелов тут надо 3 */
					wcscat(OutStr,L"   ");
					TextStr[TextPos++]=L' ';
				}
				else
				{
					char NewCh;
					WINPORT(WideCharToMultiByte)(VM.CodePage, 0, &Ch,1, &NewCh,1," ",nullptr);
					int OutStrLen=StrLength(OutStr);
					swprintf(OutStr+OutStrLen,ARRAYSIZE(OutStr)-OutStrLen,L"%02X ", (unsigned int)(unsigned char)NewCh);

					if (!Ch)
						Ch=L' ';

					TextStr[TextPos++]=Ch;
					LastPage=0;
				}

				if (X==7)
					wcscat(OutStr,BorderLine);
			}
		}

		TextStr[TextPos]=0;
		wcscat(TextStr,L" ");

		if ((SelEnd <= SelStart) && bSelStartFound)
			SelEnd = (int)wcslen(OutStr)-2;

		wcscat(OutStr,L" ");
		wcscat(OutStr,TextStr);
#if 0

		for (size_t I=0; I < wcslen(OutStr); ++I)
			if (OutStr[I] == (wchar_t)0xFFFF)
				OutStr[I]=L'?';

#endif

		if (StrLength(OutStr)>HexLeftPos)
		{
			FS<<fmt::LeftAlign()<<fmt::Width(ObjWidth)<<fmt::Precision(ObjWidth)<<OutStr+static_cast<size_t>(HexLeftPos);
		}
		else
		{
			FS<<fmt::Width(ObjWidth)<<L"";
		}

		if (bSelStartFound && bSelEndFound)
		{
			SetColor(COL_VIEWERSELECTEDTEXT);
			GotoXY((int)((int64_t)X1+SelStart-HexLeftPos),Y);
			FS<<fmt::Precision(SelEnd-SelStart+1)<<OutStr+static_cast<size_t>(SelStart);
//			SelSize = 0;
		}
	}
}

/* $ 27.04.2001 DJ
   отрисовка скроллбара - в отдельную функцию
*/

void Viewer::DrawScrollbar()
{
	if (ViOpt.ShowScrollbar)
	{
		if (m_bQuickView)
			SetColor(COL_PANELSCROLLBAR);
		else
			SetColor(COL_VIEWERSCROLLBAR);

		if (!VM.Hex)
		{
			ScrollBar(X2+(m_bQuickView?1:0),Y1,Y2-Y1+1,(LastPage ? (!FilePos?0:100):ToPercent64(FilePos,FileSize)),100);
		}
		else
		{
			UINT64 Total=FileSize/16+(FileSize%16?1:0);
			UINT64 Top=FilePos/16+(FilePos%16?1:0);
			ScrollBarEx(X2+(m_bQuickView?1:0),Y1,Y2-Y1+1,LastPage?Top?Total:0:Top,Total);
		}
	}
}


FARString &Viewer::GetTitle(FARString &strName,int,int)
{
	if (!strTitle.IsEmpty())
	{
		strName = strTitle;
	}
	else
	{
		if (!IsAbsolutePath(strFileName))
		{
			FARString strPath;
			ViewNamesList.GetCurDir(strPath);
			AddEndSlash(strPath);
			strName = strPath+strFileName;
		}
		else
		{
			strName = strFileName;
		}
	}

	return strName;
}

void Viewer::ShowStatus()
{
	if (HostFileViewer)
		HostFileViewer->ShowStatus();
}


void Viewer::SetStatusMode(int Mode)
{
	ShowStatusLine=Mode;
}


void Viewer::ReadString(ViewerString *pString, int MaxSize, int StrSize)
{
	WCHAR Ch, Ch2;
	int64_t OutPtr;
	bool bSelStartFound = false, bSelEndFound = false;
	pString->bSelection = false;
	AdjustWidth();
	OutPtr=0;

	if (VM.Hex)
	{
		size_t len = 16;
		// Alter-1: ::vread accepts number of codepoint units:
		// 4-bytes for UTF32, 2-bytes for UTF16 and 1-bytes for everything else
		// But we always display 16 bytes
		switch (VM.CodePage) {
			case CP_UTF32LE: case CP_UTF32BE: len/= 4; break;
			case CP_UTF16LE: case CP_UTF16BE: len/= 2; break;
		} // TODO: ???

		OutPtr=vread(pString->lpData, len);
		pString->lpData[len] = 0;
	}
	else
	{
		bool CRSkipped=false;

		if (SelectSize && vtell() > SelectPos)
		{
			pString->nSelStart = 0;
			bSelStartFound = true;
		}

		for (;;)
		{
			if (OutPtr>=StrSize-16)
				break;

			/* $ 12.07.2000 SVS
			  ! Wrap - трехпозиционный
			*/
			if (VM.Wrap && OutPtr>XX2-X1)
			{
				/* $ 11.07.2000 tran
				   + warp are now WORD-WRAP */
				int64_t SavePos=vtell();
				WCHAR TmpChar=0;

				if (vgetc(Ch) && Ch!=CRSym && (Ch!=L'\r' || (vgetc(TmpChar) && TmpChar!=CRSym)))
				{
					vseek(SavePos,SEEK_SET);

					if (VM.WordWrap)
					{
						if (!IsSpace(Ch) && !IsSpace(pString->lpData[(int)OutPtr]))
						{
							int64_t SavePtr=OutPtr;

							/* $ 18.07.2000 tran
							   добавил в качестве wordwrap разделителей , ; > ) */
							while (OutPtr)
							{
								Ch2=pString->lpData[(int)OutPtr];

								if (IsSpace(Ch2) || Ch2==L',' || Ch2==L';' || Ch2==L'>'|| Ch2==L')')
									break;

								OutPtr--;
							}

							Ch2=pString->lpData[(int)OutPtr];

							if (Ch2==L',' || Ch2==L';' || Ch2==L')' || Ch2==L'>')
								OutPtr++;
							else
								while (IsSpace(pString->lpData[(int)OutPtr]) && OutPtr<=SavePtr)
									OutPtr++;

							if (OutPtr < SavePtr && OutPtr)
							{
								vseek(-CalcCodeUnitsDistance(VM.CodePage,
										&pString->lpData[(size_t)OutPtr],
										&pString->lpData[(size_t)SavePtr]),
									SEEK_CUR);
							}
							else
								OutPtr = SavePtr;
						}

						/* $ 13.09.2000 tran
						   remove space at WWrap */

						if (IsSpace(Ch)) {
							int64_t lastpos;
							for (;;) {
								lastpos = vtell();
								if (!vgetc(Ch) || !IsSpace(Ch)) break;
							}
							vseek(lastpos, SEEK_SET);
						}

					}// wwrap
				}

				break;
			}

			if (SelectSize > 0 && SelectPos==vtell())
			{
				pString->nSelStart = OutPtr+(CRSkipped?1:0);;
				bSelStartFound = true;
			}

			if (!(MaxSize--))
				break;

			if (!vgetc(Ch))
				break;

			if (Ch==CRSym)
				break;

			if (CRSkipped)
			{
				CRSkipped=false;
				pString->lpData[(int)OutPtr++]=L'\r';
			}

			if (Ch==L'\t')
			{
				do
				{
					pString->lpData[(int)OutPtr++]=L' ';
				}
				while ((OutPtr % ViOpt.TabSize) && ((int)OutPtr < (MAX_VIEWLINEB-1)));

				if (VM.Wrap && OutPtr>XX2-X1)
					pString->lpData[XX2-X1+1]=0;

				continue;
			}

			/* $ 20.09.01 IS
			   Баг: не учитывали левую границу при свертке
			*/
			if (Ch==L'\r')
			{
				CRSkipped=true;

				if (OutPtr>=XX2-X1)
				{
					int64_t SavePos=vtell();
					WCHAR nextCh=0;

					if (vgetc(nextCh) && nextCh!=CRSym)
					{
						CRSkipped=false;
					}

					vseek(SavePos,SEEK_SET);
				}

				if (CRSkipped)
					continue;
			}

			if (!Ch || Ch==L'\n')
				Ch=L' ';

			pString->lpData[(int)OutPtr++]=Ch;
			pString->lpData[(int)OutPtr]=0;

			if (SelectSize > 0 && (SelectPos+SelectSize)==vtell())
			{
				pString->nSelEnd = OutPtr;
				bSelEndFound = true;
			}
		}
	}

	pString->lpData[(int)OutPtr]=0;

	if (!bSelEndFound && SelectSize && vtell() < SelectPos+SelectSize)
	{
		bSelEndFound = true;
		pString->nSelEnd = wcslen(pString->lpData);
	}

	if (bSelStartFound)
	{
		if (pString->nSelStart > (int64_t)wcslen(pString->lpData))
			bSelStartFound = false;

		if (bSelEndFound)
			if (pString->nSelStart > pString->nSelEnd)
				bSelStartFound = false;
	}

	LastPage=ViewFile.Eof();

	if (bSelStartFound && bSelEndFound)
		pString->bSelection = true;
}


int64_t Viewer::VMProcess(int OpCode,void *vParam,int64_t iParam)
{
	switch (OpCode)
	{
		case MCODE_C_EMPTY:
			return (int64_t)!FileSize;
		case MCODE_C_SELECTED:
			return (int64_t)(SelectSize?TRUE:FALSE);
		case MCODE_C_EOF:
			return (int64_t)(LastPage || !ViewFile.Opened());
		case MCODE_C_BOF:
			return (int64_t)(!FilePos || !ViewFile.Opened());
		case MCODE_V_VIEWERSTATE:
		{
			DWORD MacroViewerState=0;
			MacroViewerState|=VM.Wrap?0x00000008:0;
			MacroViewerState|=VM.WordWrap?0x00000010:0;
			MacroViewerState|=VM.Hex?0x00000020:0;
			MacroViewerState|=Opt.OnlyEditorViewerUsed?0x08000000|0x00000800:0;
			MacroViewerState|=HostFileViewer && !HostFileViewer->GetCanLoseFocus()?0x00000800:0;
			return (int64_t)MacroViewerState;
		}
		case MCODE_V_ITEMCOUNT: // ItemCount - число элементов в текущем объекте
			return (int64_t)GetViewFileSize();
		case MCODE_V_CURPOS: // CurPos - текущий индекс в текущем объекте
			return (int64_t)(GetViewFilePos()+1);
	}

	return 0;
}

/* $ 28.01.2001
   - Путем проверки ViewFile на nullptr избавляемся от падения
*/
int Viewer::ProcessKey(int Key)
{
	ViewerString vString;

	/* $ 22.01.2001 IS
	     Происходят какие-то манипуляции -> снимем выделение
	*/
	if (!ViOpt.PersistentBlocks &&
	        Key!=KEY_IDLE && Key!=KEY_NONE && !(Key==KEY_CTRLINS||Key==KEY_CTRLNUMPAD0) && Key!=KEY_CTRLC)
		SelectSize=0;

	if (!InternalKey && !LastKeyUndo && (FilePos!=UndoData[0].UndoAddr || LeftPos!=UndoData[0].UndoLeft))
	{
		for (int i=ARRAYSIZE(UndoData)-1; i>0; i--)
		{
			UndoData[i].UndoAddr=UndoData[i-1].UndoAddr;
			UndoData[i].UndoLeft=UndoData[i-1].UndoLeft;
		}

		UndoData[0].UndoAddr=FilePos;
		UndoData[0].UndoLeft=LeftPos;
	}

	if (Key!=KEY_ALTBS && Key!=KEY_CTRLZ && Key!=KEY_NONE && Key!=KEY_IDLE)
		LastKeyUndo=FALSE;

	if (Key>=KEY_CTRL0 && Key<=KEY_CTRL9)
	{
		int Pos=Key-KEY_CTRL0;

		if (BMSavePos.SavePosAddr[Pos]!=POS_NONE)
		{
			FilePos=BMSavePos.SavePosAddr[Pos];
			LeftPos=BMSavePos.SavePosLeft[Pos];
//      LastSelPos=FilePos;
			Show();
		}

		return TRUE;
	}

	if (Key>=KEY_CTRLSHIFT0 && Key<=KEY_CTRLSHIFT9)
		Key=Key-KEY_CTRLSHIFT0+KEY_RCTRL0;

	if (Key>=KEY_RCTRL0 && Key<=KEY_RCTRL9)
	{
		int Pos=Key-KEY_RCTRL0;
		BMSavePos.SavePosAddr[Pos]=FilePos;
		BMSavePos.SavePosLeft[Pos]=LeftPos;
		return TRUE;
	}

	switch (Key)
	{
		case KEY_F1:
		{
			Help Hlp(L"Viewer");
			return TRUE;
		}
		case KEY_CTRLU:
		{
//      if (SelectSize)
			{
				SelectSize = 0;
				Show();
			}
			return TRUE;
		}
		case KEY_CTRLC:
		case KEY_CTRLINS:  case KEY_CTRLNUMPAD0:
		{
			if (SelectSize && ViewFile.Opened())
			{
				wchar_t *SelData;
				size_t DataSize = (size_t)SelectSize;// + (IsFullWideCodePage(VM.CodePage) ? sizeof(wchar_t) : 1);
				switch (VM.CodePage) {
					case CP_UTF32LE: case CP_UTF32BE: DataSize+= 4; break;
					case CP_UTF16LE: case CP_UTF16BE: DataSize+= 2; break;
					default: DataSize++;
				}
				int64_t CurFilePos=vtell();

				if ((SelData=(wchar_t*)xf_malloc(DataSize*sizeof(wchar_t))) )
				{
					wmemset(SelData, 0, DataSize);
					vseek(SelectPos,SEEK_SET);
					vread(SelData, (int)SelectSize);
					CopyToClipboard(SelData);
					xf_free(SelData);
					vseek(CurFilePos,SEEK_SET);
				}
			}

			return TRUE;
		}
		//   включить/выключить скролбар
		case KEY_CTRLS:
		{
			ViOpt.ShowScrollbar=!ViOpt.ShowScrollbar;
			Opt.ViOpt.ShowScrollbar=ViOpt.ShowScrollbar;

			if (m_bQuickView)
				CtrlObject->Cp()->ActivePanel->Redraw();

			Show();
			return (TRUE);
		}
		case KEY_IDLE:
		{
			if (ViewFile.Opened())
			{
				FARString strRoot;
				GetPathRoot(strFullFileName, strRoot);
				int DriveType=FAR_GetDriveType(strRoot);

				if (DriveType!=DRIVE_REMOVABLE && !IsDriveTypeCDROM(DriveType))
				{
					FAR_FIND_DATA_EX NewViewFindData;

					if (!apiGetFindDataEx(strFullFileName, NewViewFindData))
						return TRUE;

					ViewFile.ActualizeFileSize();
					vseek(0,SEEK_END);
					int64_t CurFileSize=vtell();

					if (ViewFindData.ftLastWriteTime.dwLowDateTime!=NewViewFindData.ftLastWriteTime.dwLowDateTime ||
					        ViewFindData.ftLastWriteTime.dwHighDateTime!=NewViewFindData.ftLastWriteTime.dwHighDateTime ||
					        CurFileSize!=FileSize)
					{
						ViewFindData=NewViewFindData;
						FileSize=CurFileSize;

						if (FilePos>FileSize)
							ProcessKey(KEY_CTRLEND);
						else
						{
							int64_t PrevLastPage=LastPage;
							Show();

							if (PrevLastPage && !LastPage)
							{
								ProcessKey(KEY_CTRLEND);
								LastPage=TRUE;
							}
						}
					}
				}
			}

			if (Opt.ViewerEditorClock && HostFileViewer && HostFileViewer->IsFullScreen() && Opt.ViOpt.ShowTitleBar)
				ShowTime(FALSE);

			return TRUE;
		}
		case KEY_ALTBS:
		case KEY_CTRLZ:
		{
			for (size_t I=1; I<ARRAYSIZE(UndoData); I++)
			{
				UndoData[I-1].UndoAddr=UndoData[I].UndoAddr;
				UndoData[I-1].UndoLeft=UndoData[I].UndoLeft;
			}

			if (UndoData[0].UndoAddr!=-1)
			{
				FilePos=UndoData[0].UndoAddr;
				LeftPos=UndoData[0].UndoLeft;
				UndoData[ARRAYSIZE(UndoData)-1].UndoAddr=-1;
				UndoData[ARRAYSIZE(UndoData)-1].UndoLeft=-1;
				Show();
//        LastSelPos=FilePos;
			}

			return TRUE;
		}
		case KEY_ADD:
		case KEY_SUBTRACT:
		{
			if (strTempViewName.IsEmpty())
			{
				FARString strName;
				bool NextFileFound;

				if (Key==KEY_ADD)
					NextFileFound=ViewNamesList.GetNextName(strName);
				else
					NextFileFound=ViewNamesList.GetPrevName(strName);

				if (NextFileFound)
				{
					if (Opt.ViOpt.SavePos)
					{
						FARString strCacheName=strPluginData.IsEmpty()?strFileName:strPluginData+PointToName(strFileName);
						UINT CodePage=0;

						if (CodePageChangedByUser)
							CodePage=VM.CodePage;

						{
							PosCache poscache={};
							poscache.Param[0]=FilePos;
							poscache.Param[1]=LeftPos;
							poscache.Param[2]=VM.Hex;
							//=poscache.Param[3];
							poscache.Param[4]=CodePage;

							if (Opt.ViOpt.SaveShortPos)
							{
								poscache.Position[0]=BMSavePos.SavePosAddr;
								poscache.Position[1]=BMSavePos.SavePosLeft;
								//poscache.Position[2]=;
								//poscache.Position[3]=;
							}

							CtrlObject->ViewerPosCache->AddPosition(strCacheName,poscache);
							memset(&BMSavePos,0xff,sizeof(BMSavePos)); //??!!??
						}
					}

					if (PointToName(strName) == strName)
					{
						FARString strViewDir;
						ViewNamesList.GetCurDir(strViewDir);

						if (!strViewDir.IsEmpty())
							FarChDir(strViewDir);
					}

					if (OpenFile(strName, TRUE))
					{
						SecondPos=0;
						Show();
					}

					ShowConsoleTitle();
				}
			}

			return TRUE;
		}
		case KEY_SHIFTF2:
		{
			ProcessTypeWrapMode(!VM.WordWrap);
			return TRUE;
		}
		case KEY_F2:
		{
			ProcessWrapMode(!VM.Wrap);
			return TRUE;
		}
		case KEY_F4:
		{
			ProcessHexMode(!VM.Hex);
			return TRUE;
		}
		case KEY_F7:
		{
			Search(0,0);
			return TRUE;
		}
		case KEY_SHIFTF7:
		case KEY_SPACE:
		{
			Search(1,0);
			return TRUE;
		}
		case KEY_ALTF7:
		{
			SearchFlags.Set(REVERSE_SEARCH);
			Search(1,0);
			SearchFlags.Clear(REVERSE_SEARCH);
			return TRUE;
		}
		case KEY_F8:
		{
			switch (VM.CodePage) {
				case CP_UTF32LE: case CP_UTF32BE:
					FilePos*= 4;
					SetFileSize();
					SelectPos = 0;
					SelectSize = 0;
					break;
				case CP_UTF16LE: case CP_UTF16BE:
					FilePos*= 2;
					SetFileSize();
					SelectPos = 0;
					SelectSize = 0;
					break;
			}

			VM.CodePage = VM.CodePage==WINPORT(GetOEMCP)() ? WINPORT(GetACP)() : WINPORT(GetOEMCP)();
			ChangeViewKeyBar();
			Show();
//    LastSelPos=FilePos;
			CodePageChangedByUser=TRUE;
			return TRUE;
		}
		case KEY_SHIFTF8:
		{
			UINT nCodePage = SelectCodePage(VM.CodePage, true, true);

			// BUGBUG
			// пока что запретим переключать hex в UTF8/UTF32, ибо не работает.
			if (VM.Hex && (nCodePage==CP_UTF8 || nCodePage==CP_UTF32LE || nCodePage==CP_UTF32BE))
			{
				return TRUE;
			}

			if (nCodePage!=(UINT)-1)
			{
				CodePageChangedByUser=TRUE;

				if (IsFullWideCodePage(VM.CodePage) && !IsFullWideCodePage(nCodePage))
				{
					FilePos*= sizeof(wchar_t);
					SelectPos = 0;
					SelectSize = 0;
					SetCRSym();
				}
				else if (!IsFullWideCodePage(VM.CodePage) && IsFullWideCodePage(nCodePage))
				{
					FilePos=(FilePos+(FilePos&3))/4; //????
					SelectPos = 0;
					SelectSize = 0;
					SetCRSym();
				}

				VM.CodePage=nCodePage;
				SetFileSize();
				ChangeViewKeyBar();
				Show();
//      LastSelPos=FilePos;
			}

			return TRUE;
		}
		case KEY_ALTF8:
		{
			if (ViewFile.Opened())
				GoTo();

			return TRUE;
		}
		case KEY_F11:
		{
			CtrlObject->Plugins.CommandsMenu(MODALTYPE_VIEWER,0,L"Viewer");
			Show();
			return TRUE;
		}
		/* $ 27.06.2001 VVM
		  + С альтом скролим по 1 */
		case KEY_MSWHEEL_UP:
		case(KEY_MSWHEEL_UP | KEY_ALT):
		{
			int Roll = Key & KEY_ALT?1:Opt.MsWheelDeltaView;

			for (int i=0; i<Roll; i++)
				ProcessKey(KEY_UP);

			return TRUE;
		}
		case KEY_MSWHEEL_DOWN:
		case(KEY_MSWHEEL_DOWN | KEY_ALT):
		{
			int Roll = Key & KEY_ALT?1:Opt.MsWheelDeltaView;

			for (int i=0; i<Roll; i++)
				ProcessKey(KEY_DOWN);

			return TRUE;
		}
		case KEY_MSWHEEL_LEFT:
		case(KEY_MSWHEEL_LEFT | KEY_ALT):
		{
			int Roll = Key & KEY_ALT?1:Opt.MsHWheelDeltaView;

			for (int i=0; i<Roll; i++)
				ProcessKey(KEY_LEFT);

			return TRUE;
		}
		case KEY_MSWHEEL_RIGHT:
		case(KEY_MSWHEEL_RIGHT | KEY_ALT):
		{
			int Roll = Key & KEY_ALT?1:Opt.MsHWheelDeltaView;

			for (int i=0; i<Roll; i++)
				ProcessKey(KEY_RIGHT);

			return TRUE;
		}
		case KEY_UP: case KEY_NUMPAD8: case KEY_SHIFTNUMPAD8:
		{
			if (FilePos>0 && ViewFile.Opened())
			{
				Up();

				if (VM.Hex)
				{
					size_t len = 0x8;
					switch (VM.CodePage) {
						case CP_UTF32LE: case CP_UTF32BE: len*= 4; break;
						case CP_UTF16LE: case CP_UTF16BE: len*= 2; break;
					}
					FilePos&=~ (len - 1);
					Show();
				}
				else
					ShowPage(SHOW_UP);
			}

//      LastSelPos=FilePos;
			return TRUE;
		}
		case KEY_DOWN: case KEY_NUMPAD2:  case KEY_SHIFTNUMPAD2:
		{
			if (!LastPage && ViewFile.Opened())
			{
				if (VM.Hex)
				{
					FilePos=SecondPos;
					Show();
				}
				else
					ShowPage(SHOW_DOWN);
			}

//      LastSelPos=FilePos;
			return TRUE;
		}
		case KEY_PGUP: case KEY_NUMPAD9: case KEY_SHIFTNUMPAD9: case KEY_CTRLUP:
		{
			if (ViewFile.Opened())
			{
				for (int i=Y1; i<Y2; i++)
					Up();

				Show();
//        LastSelPos=FilePos;
			}

			return TRUE;
		}
		case KEY_PGDN: case KEY_NUMPAD3:  case KEY_SHIFTNUMPAD3: case KEY_CTRLDOWN:
		{
			vString.lpData = new(std::nothrow) wchar_t[MAX_VIEWLINEB];

			if (!vString.lpData)
				return TRUE;

			if (LastPage || !ViewFile.Opened())
			{
				delete[] vString.lpData;
				return TRUE;
			}

			vseek(FilePos,SEEK_SET);

			for (int i=Y1; i<Y2; i++)
			{
				ReadString(&vString,-1, MAX_VIEWLINEB);

				if (LastPage)
				{
					delete[] vString.lpData;
					return TRUE;
				}
			}

			FilePos=vtell();

			for (int i=Y1; i<=Y2; i++)
				ReadString(&vString,-1, MAX_VIEWLINEB);

			/* $ 02.06.2003 VVM
			  + Старое поведение оставим на Ctrl-Down */

			/* $ 21.05.2003 VVM
			  + По PgDn листаем всегда по одной странице,
			    даже если осталась всего одна строчка.
			    Удобно тексты читать */
			if (LastPage && Key == KEY_CTRLDOWN)
			{
				InternalKey++;
				ProcessKey(KEY_CTRLPGDN);
				InternalKey--;
				delete[] vString.lpData;
				return TRUE;
			}

			Show();
			delete [] vString.lpData;
//      LastSelPos=FilePos;
			return TRUE;
		}
		case KEY_LEFT: case KEY_NUMPAD4: case KEY_SHIFTNUMPAD4:
		{
			if (LeftPos>0 && ViewFile.Opened())
			{
				if (VM.Hex && LeftPos>80-Width)
					LeftPos=Max(80-Width,1);

				LeftPos--;
				Show();
			}

//      LastSelPos=FilePos;
			return TRUE;
		}
		case KEY_RIGHT: case KEY_NUMPAD6: case KEY_SHIFTNUMPAD6:
		{
			if (LeftPos<MAX_VIEWLINE && ViewFile.Opened() && !VM.Hex && !VM.Wrap)
			{
				LeftPos++;
				Show();
			}

//      LastSelPos=FilePos;
			return TRUE;
		}
		case KEY_CTRLLEFT: case KEY_CTRLNUMPAD4:
		{
			if (ViewFile.Opened())
			{
				if (VM.Hex)
				{
					FilePos--;

					if (FilePos<0)
						FilePos=0;
				}
				else
				{
					LeftPos-=20;

					if (LeftPos<0)
						LeftPos=0;
				}

				Show();
//        LastSelPos=FilePos;
			}

			return TRUE;
		}
		case KEY_CTRLRIGHT: case KEY_CTRLNUMPAD6:
		{
			if (ViewFile.Opened())
			{
				if (VM.Hex)
				{
					FilePos++;

					if (FilePos >= FileSize)
						FilePos=FileSize-1; //??
				}
				else if (!VM.Wrap)
				{
					LeftPos+=20;

					if (LeftPos>MAX_VIEWLINE)
						LeftPos=MAX_VIEWLINE;
				}

				Show();
//        LastSelPos=FilePos;
			}

			return TRUE;
		}
		case KEY_CTRLSHIFTLEFT:    case KEY_CTRLSHIFTNUMPAD4:

			// Перейти на начало строк
			if (ViewFile.Opened())
			{
				LeftPos = 0;
				Show();
			}

			return TRUE;
		case KEY_CTRLSHIFTRIGHT:     case KEY_CTRLSHIFTNUMPAD6:
		{
			// Перейти на конец строк
			if (ViewFile.Opened())
			{
				int I, Y, Len, MaxLen = 0;

				for (I=0,Y=Y1; Y<=Y2; Y++,I++)
				{
					Len = StrLength(Strings[I]->lpData);

					if (Len > MaxLen)
						MaxLen = Len;
				} /* for */

				if (MaxLen > Width)
					LeftPos = MaxLen - Width;
				else
					LeftPos = 0;

				Show();
			} /* if */

			return TRUE;
		}
		case KEY_CTRLHOME:    case KEY_CTRLNUMPAD7:
		case KEY_HOME:        case KEY_NUMPAD7:   case KEY_SHIFTNUMPAD7:

			// Перейти на начало файла
			if (ViewFile.Opened())
				LeftPos=0;

		case KEY_CTRLPGUP:    case KEY_CTRLNUMPAD9:

			if (ViewFile.Opened())
			{
				FilePos=0;
				Show();
//        LastSelPos=FilePos;
			}

			return TRUE;
		case KEY_CTRLEND:     case KEY_CTRLNUMPAD1:
		case KEY_END:         case KEY_NUMPAD1: case KEY_SHIFTNUMPAD1:

			// Перейти на конец файла
			if (ViewFile.Opened())
				LeftPos=0;

		case KEY_CTRLPGDN:    case KEY_CTRLNUMPAD3:

			if (ViewFile.Opened())
			{
				/* $ 15.08.2002 IS
				   Для обычного режима, если последняя строка не содержит перевод
				   строки, крутанем вверх на один раз больше - иначе визуально
				   обработка End (и подобных) на такой строке отличается от обработки
				   Down.
				*/
				unsigned int max_counter=Y2-Y1;

				if (VM.Hex)
					vseek(0,SEEK_END);
				else
				{
					vseek(-1,SEEK_END);
					WCHAR LastSym=0;

					if (vgetc(LastSym) && LastSym!=CRSym)
						++max_counter;
				}

				FilePos=vtell();

				/*
				        {
				          char Buf[100];
				          sprintf(Buf,"%llX",FilePos);
				          Message(0,1,"End",Buf,"Ok");
				        }
				*/
				for (int i=0; static_cast<unsigned int>(i)<max_counter; i++)
					Up();

				/*
				        {
				          char Buf[100];
				          sprintf(Buf,"%llX, %d",FilePos, I);
				          Message(0,1,"Up",Buf,"Ok");
				        }
				*/
				if (VM.Hex) {
					size_t len = 0x8;
					switch (VM.CodePage) {
						case CP_UTF32LE: case CP_UTF32BE: len*= 4; break;
						case CP_UTF16LE: case CP_UTF16BE: len*= 2; break;
					}
					FilePos&= ~(len - 1);
				}

				/*
				        if (VM.Hex)
				        {
				          char Buf[100];
				          sprintf(Buf,"%llX",FilePos);
				          Message(0,1,"VM.Hex",Buf,"Ok");
				        }
				*/
				Show();
//        LastSelPos=FilePos;
			}

			return TRUE;
		default:

			if (Key>=L' ' && Key<0x10000)
			{
				Search(0,Key);
				return TRUE;
			}
	}

	return FALSE;
}

int Viewer::ProcessMouse(MOUSE_EVENT_RECORD *MouseEvent)
{
	if (!(MouseEvent->dwButtonState & 3))
		return FALSE;

	/* $ 22.01.2001 IS
	     Происходят какие-то манипуляции -> снимем выделение
	*/
//  SelectSize=0;

	/* $ 10.09.2000 SVS
	   ! Постоянный скроллинг при нажатой клавише
	     Обыкновенный захват мыши
	*/
	/* $ 02.10.2000 SVS
	  > Если нажать в самом низу скролбара, вьюер отмотается на страницу
	  > ниже нижней границы текста. Перед глазами будет пустой экран.
	*/
	if (ViOpt.ShowScrollbar && MouseX==X2+(m_bQuickView?1:0))
	{
		/* $ 01.09.2000 SVS
		   Небольшая бага с тыканием в верхнюю позицию ScrollBar`а
		*/
		if (MouseY == Y1)
			while (IsMouseButtonPressed())
				ProcessKey(KEY_UP);
		else if (MouseY==Y2)
		{
			while (IsMouseButtonPressed())
			{
//        _SVS(SysLog(L"Viewer/ KEY_DOWN= %i, %i",FilePos,FileSize));
				ProcessKey(KEY_DOWN);
			}
		}
		else if (MouseY == Y1+1)
			ProcessKey(KEY_CTRLHOME);
		else if (MouseY == Y2-1)
			ProcessKey(KEY_CTRLEND);
		else
		{
			while (IsMouseButtonPressed())
			{
				/* $ 14.05.2001 DJ
				   более точное позиционирование; корректная работа на больших файлах
				*/
				FilePos=(FileSize-1)/(Y2-Y1-1)*(MouseY-Y1);
				int Perc;

				if (FilePos > FileSize)
				{
					FilePos=FileSize;
					Perc=100;
				}
				else if (FilePos < 0)
				{
					FilePos=0;
					Perc=0;
				}
				else
					Perc=ToPercent64(FilePos,FileSize);

//_SVS(SysLog(L"Viewer/ ToPercent()=%i, %lld, %lld, Mouse=[%d:%d]",Perc,FilePos,FileSize,MsX,MsY));
				if (Perc == 100)
					ProcessKey(KEY_CTRLEND);
				else if (!Perc)
					ProcessKey(KEY_CTRLHOME);
				else
				{
					/* $ 27.04.2001 DJ
					   не рвем строки посередине
					*/
					AdjustFilePos();
					Show();
				}
			}
		}

		return (TRUE);
	}

	/* $ 16.12.2000 tran
	   шелчок мышью на статус баре */

	/* $ 12.10.2001 SKV
	  угу, а только если он нсть, statusline...
	*/
	if (MouseY == (Y1-1) && (HostFileViewer && HostFileViewer->IsTitleBarVisible()))  // Status line
	{
		int XCodePage, XPos, NameLength;
		NameLength=ObjWidth-40;

		if (Opt.ViewerEditorClock && HostFileViewer && HostFileViewer->IsFullScreen())
			NameLength-=6;

		if (NameLength<20)
			NameLength=20;

		XCodePage=NameLength+1;
		XPos=NameLength+1+10+1+10+1;

		while (IsMouseButtonPressed());

		if (MouseY != Y1-1)
			return TRUE;

		//_D(SysLog(L"MsX=%i, XTable=%i, XPos=%i",MsX,XTable,XPos));
		if (MouseX>=XCodePage && MouseX<=XCodePage+10)
		{
			ProcessKey(KEY_SHIFTF8);
			return (TRUE);
		}

		if (MouseX>=XPos && MouseX<=XPos+7+1+4+1+3)
		{
			ProcessKey(KEY_ALTF8);
			return (TRUE);
		}
	}

	if (MouseX<X1 || MouseX>X2 || MouseY<Y1 || MouseY>Y2)
		return FALSE;

	if (MouseX<X1+7)
		while (IsMouseButtonPressed() && MouseX<X1+7)
			ProcessKey(KEY_LEFT);
	else if (MouseX>X2-7)
		while (IsMouseButtonPressed() && MouseX>X2-7)
			ProcessKey(KEY_RIGHT);
	else if (MouseY<Y1+(Y2-Y1)/2)
		while (IsMouseButtonPressed() && MouseY<Y1+(Y2-Y1)/2)
			ProcessKey(KEY_UP);
	else
		while (IsMouseButtonPressed() && MouseY>=Y1+(Y2-Y1)/2)
			ProcessKey(KEY_DOWN);

	return TRUE;
}

void Viewer::FilePosShiftLeft(uint64_t Offset)
{
	if (FilePos > 0 && (uint64_t)FilePos > Offset) {
		FilePos-= Offset;
	} else
		FilePos = 0;
}

void Viewer::Up()
{
	if (!ViewFile.Opened())
		return;

	wchar_t Buf[MAX_VIEWLINE + 1];
	int BufSize,I;

	if (FilePos > ((int64_t)(sizeof(Buf)/sizeof(wchar_t))) - 1)
		BufSize = (sizeof(Buf)/sizeof(wchar_t)) - 1;
	else if (FilePos != 0)
		BufSize = (int)FilePos;
	else
		return;

	LastPage=0;

	if (VM.Hex)
	{
		// Alter-1: here we use BYTE COUNT, while in Down handler we use ::vread which may 
		// accept either CHARACTER COUNT or w_char count. 
		//int UpSize=IsFullWideCodePage(VM.CodePage) ? 8 : 8 * sizeof(wchar_t);
		int UpSize=16; // always have 16 bytes per row

		if (FilePos<(int64_t)UpSize)
			FilePos=0;
		else
			FilePosShiftLeft(UpSize);

		return;
	}

	vseek(FilePos - (int64_t)BufSize, SEEK_SET);
	I = BufSize = vread(Buf, BufSize, true);
	if (I == -1)
	{
		return;
	}

	wchar_t CRSymEncoded = (unsigned int)CRSym;
	wchar_t CRRSymEncoded = (unsigned int)'\r';
	switch (VM.CodePage)
	{
		case CP_UTF32BE:
			CRSymEncoded<<= 24;
			CRRSymEncoded<<= 24;
			break;
		case CP_UTF16BE:
			CRSymEncoded<<= 8;
			CRRSymEncoded<<= 8;
			break;
	}

	if (I > 0 && Buf[I - 1] == CRSymEncoded)
	{
		--I;
	}

	if (I > 0 && CRSym == L'\n' && Buf[I - 1] == CRRSymEncoded)
	{
		--I;
	}

	while (I > 0 && Buf[I - 1] != CRSymEncoded)
	{
		--I;
	}

	int64_t WholeLineLength = (BufSize - I); // in code units

	if (VM.Wrap && Width > 0)
	{
		vseek(FilePos - WholeLineLength, SEEK_SET);
		int WrapBufSize = vread(Buf, WholeLineLength, false);

		// we might read more code units and could actually overflow current position
		// so try to find out exact substring that matches into line start and current position
		Buf[WrapBufSize] = 0;
//		fprintf(stderr, "WrapBufSize=%d WholeLineLength=%d LINE='%ls'\n", WrapBufSize, WholeLineLength, &Buf[0]);
//		fprintf(stderr, "LINE1: '%ls'\n", &Buf[0]);
		while (WrapBufSize)
		{
			int distance = CalcCodeUnitsDistance(VM.CodePage, &Buf[0], &Buf[WrapBufSize]);
			if (distance <= WholeLineLength)
			{
				while (WrapBufSize && distance == CalcCodeUnitsDistance(VM.CodePage, &Buf[0], &Buf[WrapBufSize - 1]))
				{
					--WrapBufSize;
				}
				break;
			}
			--WrapBufSize;
		}
		Buf[WrapBufSize] = 0;
//		fprintf(stderr, "Matching LINE: '%ls'\n", &Buf[0]);

		if (VM.WordWrap)
		{
			//	khffgkjkfdg dfkghd jgfhklf |
			//	sdflksj lfjghf fglh lf     |
			//	dfdffgljh ldgfhj           |

			for (I = 0; I < WrapBufSize;)
			{
				if (!IsSpace(Buf[I]))
				{
					for (int CurLineStart = I, LastFitEnd = I + 1;; ++I)
					{
						if (I == WrapBufSize)
						{
							int distance = CalcCodeUnitsDistance(VM.CodePage, &Buf[CurLineStart], &Buf[WrapBufSize]);
							FilePosShiftLeft((distance > 0) ? distance : 1);
							return;
						}

						if (!IsSpace(Buf[I]) && (I + 1 == WrapBufSize || IsSpace(Buf[I + 1])))
						{
							if (CalcStrSize(&Buf[CurLineStart], I + 1 - CurLineStart) > Width)
							{
								I = LastFitEnd;
								break;
							}
							LastFitEnd = I + 1;
						}
					}

				} else
				{
					++I;
				}
			}
		}

		for (int PrevSublineLength = 0, CurLineStart = I = 0;; ++I)
		{
			if (I == WrapBufSize)
			{
				int distance = CalcCodeUnitsDistance(VM.CodePage, &Buf[CurLineStart], &Buf[WrapBufSize]);
				FilePosShiftLeft((distance > 0) ? distance : 1);
				return;
			}

			int SublineLength = CalcStrSize(&Buf[CurLineStart], I - CurLineStart);
			if (SublineLength > PrevSublineLength)
			{
				if (SublineLength >= Width)
				{
					CurLineStart = I;
					PrevSublineLength = 0;
				} else
				{
					PrevSublineLength = SublineLength;
				}
			}
		}
	}

//	fprintf(stderr, "!!!!!!!!!!!!NOWRAP!!!!!!!!!!!!\n");
	FilePosShiftLeft( (WholeLineLength > 0) ? WholeLineLength : 1 );
}


int Viewer::CalcStrSize(const wchar_t *Str,int Length)
{
	int Size,I;

	for (Size=0,I=0; I<Length; I++)
		switch (Str[I])
		{
			case L'\t':
				Size+=ViOpt.TabSize-(Size % ViOpt.TabSize);
				break;
			case L'\n':
			case L'\r':
				break;
			default:
				Size++;
				break;
		}

	return(Size);
}

int Viewer::GetStrBytesNum(const wchar_t *Str, int Length)
{
	if (VM.CodePage == CP_UTF32LE || VM.CodePage == CP_UTF32BE)
		return Length;

	int cnt = WINPORT(WideCharToMultiByte)(VM.CodePage, 0, Str, Length, nullptr, 0, nullptr, nullptr);

	return (VM.CodePage == CP_UTF16LE || VM.CodePage == CP_UTF16BE) ? cnt * 2 : cnt;

}

void Viewer::SetViewKeyBar(KeyBar *ViewKeyBar)
{
	Viewer::ViewKeyBar=ViewKeyBar;
	ChangeViewKeyBar();
}


void Viewer::ChangeViewKeyBar()
{
	if (ViewKeyBar)
	{
		/* $ 12.07.2000 SVS
		   Wrap имеет 3 позиции
		*/
		/* $ 15.07.2000 SVS
		   Wrap должен показываться следующий, а не текущий
		*/
		ViewKeyBar->Change(
		    MSG(
		        (!VM.Wrap)?((!VM.WordWrap)?MViewF2:MViewShiftF2)
				        :MViewF2Unwrap),1);
		ViewKeyBar->Change(KBL_SHIFT,MSG((VM.WordWrap)?MViewF2:MViewShiftF2),1);

		if (VM.Hex)
			ViewKeyBar->Change(MSG(MViewF4Text),3);
		else
			ViewKeyBar->Change(MSG(MViewF4),3);

		if (VM.CodePage != WINPORT(GetOEMCP)())
			ViewKeyBar->Change(MSG(MViewF8DOS),7);
		else
			ViewKeyBar->Change(MSG(MViewF8),7);

		ViewKeyBar->Redraw();
	}

//	ViewerMode vm=VM;
	CtrlObject->Plugins.CurViewer=this; //HostFileViewer;
//  CtrlObject->Plugins.ProcessViewerEvent(VE_MODE,&vm);
}

enum SEARCHDLG
{
	SD_DOUBLEBOX,
	SD_TEXT_SEARCH,
	SD_EDIT_TEXT,
	SD_EDIT_HEX,
	SD_SEPARATOR1,
	SD_RADIO_TEXT,
	SD_RADIO_HEX,
	SD_CHECKBOX_CASE,
	SD_CHECKBOX_WORDS,
	SD_CHECKBOX_REVERSE,
	SD_CHECKBOX_REGEXP,
	SD_SEPARATOR2,
	SD_BUTTON_OK,
	SD_BUTTON_CANCEL,

	DM_SDSETVISIBILITY = DM_USER + 1,
};

LONG_PTR WINAPI ViewerSearchDlgProc(HANDLE hDlg,int Msg,int Param1,LONG_PTR Param2)
{
	switch (Msg)
	{
		case DN_INITDIALOG:
		{
			SendDlgMessage(hDlg,DM_SDSETVISIBILITY,SendDlgMessage(hDlg,DM_GETCHECK,SD_RADIO_HEX,0) == BSTATE_CHECKED,0);
			SendDlgMessage(hDlg,DM_EDITUNCHANGEDFLAG,SD_EDIT_TEXT,1);
			SendDlgMessage(hDlg,DM_EDITUNCHANGEDFLAG,SD_EDIT_HEX,1);
			return TRUE;
		}
		case DM_SDSETVISIBILITY:
		{
			SendDlgMessage(hDlg,DM_SHOWITEM,SD_EDIT_TEXT,!Param1);
			SendDlgMessage(hDlg,DM_SHOWITEM,SD_EDIT_HEX,Param1);
			SendDlgMessage(hDlg,DM_ENABLE,SD_CHECKBOX_CASE,!Param1);
			SendDlgMessage(hDlg,DM_ENABLE,SD_CHECKBOX_WORDS,!Param1);
			//SendDlgMessage(hDlg,DM_ENABLE,SD_CHECKBOX_REGEXP,!Param1);
			return TRUE;
		}
		case DN_BTNCLICK:
		{
			if ((Param1 == SD_RADIO_TEXT || Param1 == SD_RADIO_HEX) && Param2)
			{
				SendDlgMessage(hDlg,DM_ENABLEREDRAW,FALSE,0);
				bool Hex=(Param1==SD_RADIO_HEX);
				FARString strDataStr;
				Transform(strDataStr,(const wchar_t *)SendDlgMessage(hDlg,DM_GETCONSTTEXTPTR,Hex?SD_EDIT_TEXT:SD_EDIT_HEX,0),Hex?L'X':L'S');
				SendDlgMessage(hDlg,DM_SETTEXTPTR,Hex?SD_EDIT_HEX:SD_EDIT_TEXT,(LONG_PTR)strDataStr.CPtr());
				SendDlgMessage(hDlg,DM_SDSETVISIBILITY,Hex,0);

				if (!strDataStr.IsEmpty())
				{
					SendDlgMessage(hDlg,DM_EDITUNCHANGEDFLAG,Hex?SD_EDIT_HEX:SD_EDIT_TEXT,SendDlgMessage(hDlg,DM_EDITUNCHANGEDFLAG,Hex?SD_EDIT_TEXT:SD_EDIT_HEX,-1));
				}

				SendDlgMessage(hDlg,DM_ENABLEREDRAW,TRUE,0);
				return TRUE;
			}
		}
		case DN_HOTKEY:
		{
			if (Param1==SD_TEXT_SEARCH)
			{
				SendDlgMessage(hDlg,DM_SETFOCUS,(SendDlgMessage(hDlg,DM_GETCHECK,SD_RADIO_HEX,0) == BSTATE_CHECKED)?SD_EDIT_HEX:SD_EDIT_TEXT,0);
				return FALSE;
			}
		}
	}

	return DefDlgProc(hDlg,Msg,Param1,Param2);
}

static void PR_ViewerSearchMsg()
{
	PreRedrawItem preRedrawItem=PreRedraw.Peek();
	ViewerSearchMsg((const wchar_t*)preRedrawItem.Param.Param1,(int)(INT_PTR)preRedrawItem.Param.Param2);
}

void ViewerSearchMsg(const wchar_t *MsgStr,int Percent)
{
	FARString strProgress;

	if (Percent>=0)
	{
		FormatString strPercent;
		strPercent<<Percent;

		size_t PercentLength=Max(strPercent.strValue().GetLength(),(size_t)3);
		size_t Length=Max(Min(static_cast<int>(MAX_WIDTH_MESSAGE-2),StrLength(MsgStr)),40)-PercentLength-2;
		wchar_t *Progress=strProgress.GetBuffer(Length);

		if (Progress)
		{
			size_t CurPos=Min(Percent,100)*Length/100;
			wmemset(Progress,BoxSymbols[BS_X_DB],CurPos);
			wmemset(Progress+(CurPos),BoxSymbols[BS_X_B0],Length-CurPos);
			strProgress.ReleaseBuffer(Length);
			FormatString strTmp;
			strTmp<<L" "<<fmt::Width(PercentLength)<<strPercent<<L"%";
			strProgress+=strTmp;
		}

	}

	Message(0,0,MSG(MViewSearchTitle),(SearchHex?MSG(MViewSearchingHex):MSG(MViewSearchingFor)),MsgStr,strProgress.IsEmpty()?nullptr:strProgress.CPtr());
	PreRedrawItem preRedrawItem=PreRedraw.Peek();
	preRedrawItem.Param.Param1=(void*)MsgStr;
	preRedrawItem.Param.Param2=(LPVOID)(INT_PTR)Percent;
	PreRedraw.SetParam(preRedrawItem.Param);
}

/* $ 27.01.2003 VVM
   + Параметр Next может принимать значения:
   0 - Новый поиск
   1 - Продолжить поиск со следующей позиции
   2 - Продолжить поиск с начала файла
*/


static inline bool CheckBufMatchesCaseInsensitive(size_t MatchLen, const wchar_t *Buf, const wchar_t *MatchUpperCase, const wchar_t *MatchLowerCase)
{
	for (size_t i = 0; i < MatchLen; ++i) {
		if (Buf[i] != MatchUpperCase[i] && Buf[i] != MatchLowerCase[i])
			return false;
	}

	return true;
}

static inline bool CheckBufMatchesCaseSensitive(size_t MatchLen, const wchar_t *Buf, const wchar_t *Match)
{
	for (size_t i = 0; i < MatchLen; ++i) {
		if (Buf[i] != Match[i])
			return false;
	}

	return true;
}

void Viewer::Search(int Next,int FirstChar)
{
	const wchar_t *TextHistoryName=L"SearchText";
	const wchar_t *HexMask=L"HH HH HH HH HH HH HH HH HH HH HH HH HH HH HH HH HH HH HH HH HH HH ";
	DialogDataEx SearchDlgData[]=
	{
		{DI_DOUBLEBOX,3,1,72,11,{0},0,MSG(MViewSearchTitle)},
		{DI_TEXT,5,2,0,2,{0},0,MSG(MViewSearchFor)},
		{DI_EDIT,5,3,70,3,{(DWORD_PTR)TextHistoryName},DIF_FOCUS|DIF_HISTORY|DIF_USELASTHISTORY,L""},
		{DI_FIXEDIT,5,3,70,3,{(DWORD_PTR)HexMask},DIF_MASKEDIT,L""},
		{DI_TEXT,3,4,0,4,{0},DIF_SEPARATOR,L""},
		{DI_RADIOBUTTON,5,5,0,5,{1},DIF_GROUP,MSG(MViewSearchForText)},
		{DI_RADIOBUTTON,5,6,0,6,{0},0,MSG(MViewSearchForHex)},
		{DI_CHECKBOX,40,5,0,5,{0},0,MSG(MViewSearchCase)},
		{DI_CHECKBOX,40,6,0,6,{0},0,MSG(MViewSearchWholeWords)},
		{DI_CHECKBOX,40,7,0,7,{0},0,MSG(MViewSearchReverse)},
		{DI_CHECKBOX,40,8,0,8,{0},DIF_DISABLE,MSG(MViewSearchRegexp)},
		{DI_TEXT,3,9,0,9,{0},DIF_SEPARATOR,L""},
		{DI_BUTTON,0,10,0,10,{0},DIF_DEFAULT|DIF_CENTERGROUP,MSG(MViewSearchSearch)},
		{DI_BUTTON,0,10,0,10,{0},DIF_CENTERGROUP,MSG(MViewSearchCancel)}
	};
	MakeDialogItemsEx(SearchDlgData,SearchDlg);
	FARString strSearchStr;
	FARString strMsgStr;
	int64_t MatchPos=0;
	int Case,WholeWords,ReverseSearch,SearchRegexp;

	if (!ViewFile.Opened() || (Next && strLastSearchStr.IsEmpty()))
		return;

	if (!strLastSearchStr.IsEmpty())
		strSearchStr = strLastSearchStr;
	else
		strSearchStr.Clear();

	SearchDlg[SD_RADIO_TEXT].Selected=!LastSearchHex;
	SearchDlg[SD_RADIO_HEX].Selected=LastSearchHex;
	SearchDlg[SD_CHECKBOX_CASE].Selected=LastSearchCase;
	SearchDlg[SD_CHECKBOX_WORDS].Selected=LastSearchWholeWords;
	SearchDlg[SD_CHECKBOX_REVERSE].Selected=LastSearchReverse;
	SearchDlg[SD_CHECKBOX_REGEXP].Selected=LastSearchRegexp;

	if (SearchFlags.Check(REVERSE_SEARCH))
		SearchDlg[SD_CHECKBOX_REVERSE].Selected=!SearchDlg[SD_CHECKBOX_REVERSE].Selected;

	if (IsFullWideCodePage(VM.CodePage))
	{
		SearchDlg[SD_RADIO_TEXT].Selected=TRUE;
		SearchDlg[SD_RADIO_HEX].Flags|=DIF_DISABLE;
		SearchDlg[SD_RADIO_HEX].Selected=FALSE;
	}

	if (SearchDlg[SD_RADIO_HEX].Selected)
		SearchDlg[SD_EDIT_HEX].strData = strSearchStr;
	else
		SearchDlg[SD_EDIT_TEXT].strData = strSearchStr;

	if (!Next)
	{
		SearchFlags.Flags = 0;
		Dialog Dlg(SearchDlg,ARRAYSIZE(SearchDlg),ViewerSearchDlgProc);
		Dlg.SetPosition(-1,-1,76,13);
		Dlg.SetHelp(L"ViewerSearch");

		if (FirstChar)
		{
			Dlg.InitDialog();
			Dlg.Show();
			Dlg.ProcessKey(FirstChar);
		}

		Dlg.Process();

		if (Dlg.GetExitCode()!=SD_BUTTON_OK)
			return;
	}

	SearchHex=SearchDlg[SD_RADIO_HEX].Selected;
	Case=SearchDlg[SD_CHECKBOX_CASE].Selected;
	WholeWords=SearchDlg[SD_CHECKBOX_WORDS].Selected;
	ReverseSearch=SearchDlg[SD_CHECKBOX_REVERSE].Selected;
	SearchRegexp=SearchDlg[SD_CHECKBOX_REGEXP].Selected;

	if (SearchHex)
	{
		strSearchStr = SearchDlg[SD_EDIT_HEX].strData;
		RemoveTrailingSpaces(strSearchStr);
	}
	else
	{
		strSearchStr = SearchDlg[SD_EDIT_TEXT].strData;
	}

	strLastSearchStr = strSearchStr;
	LastSearchHex=SearchHex;
	LastSearchCase=Case;
	LastSearchWholeWords=WholeWords;

	if (!SearchFlags.Check(REVERSE_SEARCH))
		LastSearchReverse=ReverseSearch;

	LastSearchRegexp=SearchRegexp;

	int SearchWChars, SearchCodeUnits;
	bool Match = false;
	{
		TPreRedrawFuncGuard preRedrawFuncGuard(PR_ViewerSearchMsg);
		//SaveScreen SaveScr;
		SetCursorType(FALSE,0);
		strMsgStr = strSearchStr;

		if (strMsgStr.GetLength()+18 > static_cast<DWORD>(ObjWidth))
			TruncStrFromEnd(strMsgStr, ObjWidth-18);

		InsertQuote(strMsgStr);

		if (SearchHex)
		{
			if (!strSearchStr.GetLength())
				return;

			Transform(strSearchStr,strSearchStr,L'S');
			WholeWords=0;
		}

		SearchWChars = (int)strSearchStr.GetLength();
		if (!SearchWChars)
			return;

		SearchCodeUnits = CalcCodeUnitsDistance(VM.CodePage, strSearchStr.CPtr(), strSearchStr.CPtr() + SearchWChars);
		FARString strSearchStrLowerCase;

		if (!Case && !SearchHex)
		{
			strSearchStrLowerCase = strSearchStr;
			strSearchStr.Upper();
			strSearchStrLowerCase.Lower();
		}

		SelectSize = 0;

		if (Next)
		{
			if (Next == 2)
			{
				SearchFlags.Set(SEARCH_MODE2);
				LastSelPos = ReverseSearch?FileSize:0;
			}
#if 0
			else
			{
				LastSelPos = SelectPos + (ReverseSearch?-1:1);
			}
#endif
		}
		else
		{
			LastSelPos = FilePos;

			if (!LastSelPos || LastSelPos == FileSize)
			{
				SearchFlags.Set(SEARCH_MODE2);
				LastSelPos = ReverseSearch?FileSize:0;
			}
		}

		vseek(LastSelPos,SEEK_SET);
		Match = false;

		if (SearchWChars>0 && (!ReverseSearch || LastSelPos>=0))
		{
			wchar_t Buf[16384];

			int ReadSize;
			wakeful W;
			INT64 StartPos=vtell();
			DWORD StartTime=WINPORT(GetTickCount)();

			while (!Match)
			{
				/* $ 01.08.2000 KM
				   Изменена строка if (ReverseSearch && CurPos<0) на if (CurPos<0),
				   так как при обычном прямом и LastSelPos=0xFFFFFFFF, поиск
				   заканчивался так и не начавшись.
				*/
				//if (CurPos<0)
				//	CurPos=0;
				//vseek(CurPos,SEEK_SET);
				int BufSize = ARRAYSIZE(Buf);
				int64_t CurPos = vtell();
				if (ReverseSearch)
				{
					/* $ 01.08.2000 KM
					   Изменёно вычисление CurPos с учётом Whole words
					*/
					CurPos-= ARRAYSIZE(Buf) - SearchCodeUnits - !!WholeWords;
					if (CurPos < 0) {
						BufSize+= (int)CurPos;
						CurPos = 0;
					}
					vseek(CurPos, SEEK_SET);
				}

				if ((ReadSize=vread(Buf,BufSize,SearchHex!=0))<=0)
					break;

				DWORD CurTime=WINPORT(GetTickCount)();

				if (CurTime-StartTime>RedrawTimeout)
				{
					StartTime=CurTime;

					if (CheckForEscSilent())
					{
						if (ConfirmAbortOp())
						{
							Redraw();
							return;
						}
					}

					INT64 Total=ReverseSearch?StartPos:FileSize-StartPos;
					INT64 Current=_abs64(CurPos-StartPos);
					int Percent=Total>0?static_cast<int>(Current*100/Total):-1;
					// В случае если файл увеличивается размере, то количество
					// процентов может быть больше 100. Обрабатываем эту ситуацию.
					if (Percent>100)
					{
						SetFileSize();
						Total=FileSize-StartPos;
						Percent=Total>0?static_cast<int>(Current*100/Total):-1;
						if (Percent>100)
						{
							Percent=100;
						}
					}
					ViewerSearchMsg(strMsgStr,Percent);
				}

				/* $ 01.08.2000 KM
				   Убран кусок текста после приведения поисковой строки
				   и Buf к единому регистру, если поиск не регистрозависимый
				   или не ищется Hex-строка и в связи с этим переработан код поиска
				*/
				int MaxSize=ReadSize-SearchWChars+1;
				int Increment=ReverseSearch ? -1:+1;

				for (int I=ReverseSearch ? MaxSize-1:0; I<MaxSize && I>=0; I+=Increment)
				{
					/* $ 01.08.2000 KM
					   Обработка поиска "Whole words"
					*/
					bool locResultLeft = false;
					bool locResultRight = false;

					if (WholeWords)
					{
						if (I)
						{
							if (IsSpace(Buf[I-1]) || IsEol(Buf[I-1]) ||
							        (wcschr(Opt.strWordDiv,Buf[I-1])))
								locResultLeft = true;
						}
						else
						{
							locResultLeft = true;
						}

						if (ReadSize!=BufSize && I+SearchWChars>=ReadSize)
							locResultRight = true;
						else if (I+SearchWChars<ReadSize &&
						         (IsSpace(Buf[I+SearchWChars]) || IsEol(Buf[I+SearchWChars]) ||
						          (wcschr(Opt.strWordDiv,Buf[I+SearchWChars]))))
							locResultRight = true;
					}
					else
					{
						locResultLeft = true;
						locResultRight = true;
					}

					if (locResultLeft && locResultRight)
					{
						if (!Case && !SearchHex) {
							Match = CheckBufMatchesCaseInsensitive(SearchWChars, &Buf[I], strSearchStr.CPtr(), strSearchStrLowerCase.CPtr());
						} else {
							Match = CheckBufMatchesCaseSensitive(SearchWChars, &Buf[I], strSearchStr.CPtr());
						}
						if (Match)
						{
							MatchPos = CurPos + CalcCodeUnitsDistance(VM.CodePage, Buf, Buf + I);
							break;
						}
					}
				}

				if (ReverseSearch)
				{
					if (CurPos <= 0)
						break;

					vseek(CurPos, SEEK_SET);
				}
				else
				{
					if (vtell() >= FileSize)
						break;

					vseek(-(SearchCodeUnits + !!WholeWords), SEEK_CUR);
				}
			}
		}
	}

	if (Match)
	{
		/* $ 24.01.2003 KM
		   ! По окончании поиска отступим от верха экрана на
		     треть отображаемой высоты.
		*/
		SelectText(MatchPos, SearchCodeUnits, ReverseSearch ? 0x2 : 0);

		// Покажем найденное на расстоянии трети экрана от верха.
		int FromTop=(ScrY-(Opt.ViOpt.ShowKeyBar?2:1))/4;

		if (FromTop<0 || FromTop>ScrY)
			FromTop=0;

		for (int i=0; i<FromTop; i++)
			Up();

		AdjustSelPosition = TRUE;
		Show();
		AdjustSelPosition = FALSE;
	}
	else
	{
		//Show();
		/* $ 27.01.2003 VVM
		   + После окончания поиска спросим о переходе поиска в начало/конец */
		if (SearchFlags.Check(SEARCH_MODE2))
			Message(MSG_WARNING,1,MSG(MViewSearchTitle),
			        (SearchHex?MSG(MViewSearchCannotFindHex):MSG(MViewSearchCannotFind)),strMsgStr,MSG(MOk));
		else
		{
			if (!Message(MSG_WARNING,2,MSG(MViewSearchTitle),
			            (SearchHex?MSG(MViewSearchCannotFindHex):MSG(MViewSearchCannotFind)),strMsgStr,
			            (ReverseSearch?MSG(MViewSearchFromEnd):MSG(MViewSearchFromBegin)),
			            MSG(MHYes),MSG(MHNo)))
				Search(2,0);
		}
	}
}


/*void Viewer::ConvertToHex(char *SearchStr,int &SearchLength)
{
  char OutStr[512],*SrcPtr;
  int OutPos=0,N=0;
  SrcPtr=SearchStr;
  while (*SrcPtr)
  {
    while (IsSpaceA(*SrcPtr))
      SrcPtr++;
    if (SrcPtr[0])
      if (!SrcPtr[1] || IsSpaceA(SrcPtr[1]))
      {
        N=HexToNum(SrcPtr[0]);
        SrcPtr++;
      }
      else
      {
        N=16*HexToNum(SrcPtr[0])+HexToNum(SrcPtr[1]);
        SrcPtr+=2;
      }
    if (N>=0)
      OutStr[OutPos++]=N;
    else
      break;
  }
  memcpy(SearchStr,OutStr,OutPos);
  SearchLength=OutPos;
}*/


int Viewer::HexToNum(int Hex)
{
	Hex=toupper(Hex);

	if (Hex>='0' && Hex<='9')
		return(Hex-'0');

	if (Hex>='A' && Hex<='F')
		return(Hex-'A'+10);

	return(-1000);
}


int Viewer::GetWrapMode()
{
	return(VM.Wrap);
}


void Viewer::SetWrapMode(int Wrap)
{
	Viewer::VM.Wrap=Wrap;
}


void Viewer::EnableHideCursor(int HideCursor)
{
	Viewer::HideCursor=HideCursor;
}


int Viewer::GetWrapType()
{
	return(VM.WordWrap);
}


void Viewer::SetWrapType(int TypeWrap)
{
	Viewer::VM.WordWrap=TypeWrap;
}


void Viewer::GetFileName(FARString &strName)
{
	strName = strFullFileName;
}


void Viewer::ShowConsoleTitle()
{
	FARString strTitle;
	strTitle.Format(MSG(MInViewer), PointToName(strFileName));
	ConsoleTitle::SetFarTitle(strTitle);
}


void Viewer::SetTempViewName(const wchar_t *Name, BOOL DeleteFolder)
{
	if (Name && *Name)
		ConvertNameToFull(Name,strTempViewName);
	else
	{
		strTempViewName.Clear();
		DeleteFolder=FALSE;
	}

	Viewer::DeleteFolder=DeleteFolder;
}


void Viewer::SetTitle(const wchar_t *Title)
{
	if (!Title)
		strTitle.Clear();
	else
		strTitle = Title;
}


void Viewer::SetFilePos(int64_t Pos)
{
	FilePos=Pos;
};


void Viewer::SetPluginData(const wchar_t *PluginData)
{
	Viewer::strPluginData = NullToEmpty(PluginData);
}


void Viewer::SetNamesList(NamesList *List)
{
	if (List)
		List->MoveData(ViewNamesList);
}

int Viewer::vread(wchar_t *Buf,int Count, bool Raw)
{
	if (VM.CodePage == CP_WIDE_LE || VM.CodePage == CP_WIDE_BE)
	{
		DWORD ReadSize = ViewFile.Read((char *)Buf, Count * sizeof(wchar_t));
		DWORD ResultedCount = ReadSize / sizeof(wchar_t);
		if ((ReadSize % sizeof(wchar_t)) != 0 && (int)ResultedCount < Count) {
			memset(((char *)Buf) + ReadSize, 0, sizeof(wchar_t) - (ReadSize % sizeof(wchar_t)));
			++ResultedCount;
		}

		if (VM.CodePage == CP_WIDE_BE && !Raw) {
			WideReverse((const wchar_t *)Buf, (wchar_t *)Buf, ResultedCount);
		}

		return ResultedCount;
	}
	else if (VM.CodePage == CP_UTF8 && !Raw)
	{
		INT64 Ptr;
		ViewFile.GetPointer(Ptr);
		int ResultedCount = 0;
		for (DWORD WantViewSize = Count; ResultedCount < Count;) {
			DWORD ViewSize = WantViewSize;
			UTF8 *SrcView = (UTF8 *)ViewFile.ViewBytesAt(Ptr, ViewSize);
			if (!ViewSize) {
				break;
			}

			const UTF8 *src = SrcView;
#if (__WCHAR_MAX__ > 0xffff)
			UTF32 *dst = (UTF32 *)&Buf[ResultedCount];
			ConversionResult cr = ConvertUTF8toUTF32(&src, src + ViewSize,
				&dst, dst + (Count - ResultedCount), lenientConversion);
			ResultedCount = dst - (UTF32 *)Buf;
#else
			UTF16 *dst = (UTF16 *)&Buf[ResultedCount];
			ConversionResult cr = ConvertUTF8toUTF16(&src, src + ViewSize,
				&dst, dst + (Count - ResultedCount), lenientConversion);
			ResultedCount = dst - (UTF16 *)Buf;
#endif

			Ptr+= (src - SrcView);

			if (cr == sourceExhausted && src == SrcView) {
				if (ViewSize < WantViewSize) {
					if (ResultedCount < Count) {
						Buf[ResultedCount] = UNI_REPLACEMENT_CHAR;
						++ResultedCount;
						++Ptr;
					} else {
						break;
					}

				} else {
					WantViewSize+= (4 + WantViewSize / 4);
				}

			} else if (cr == targetExhausted || cr == conversionOK) {
				break;
			}
		}

		ViewFile.SetPointer(Ptr);
		return ResultedCount;
	}
	else
	{
		INT64 Ptr;
		ViewFile.GetPointer(Ptr);

		DWORD ReadSize = Count;
		if (VM.CodePage == CP_UTF16LE || VM.CodePage == CP_UTF16BE ) {
			ReadSize*= 2;
		}

		LPBYTE View = ViewFile.ViewBytesAt(Ptr, ReadSize);

		if (Count == 1 && ReadSize == 2 && !Raw && (VM.CodePage==CP_UTF16LE || VM.CodePage==CP_UTF16BE ))
		{
			//Если UTF16 то простой ли это символ или нет?
			if (*(uint16_t *)View >= 0xd800 && *(uint16_t *)View <= 0xdfff) {
				ReadSize+= 2;
				View = ViewFile.ViewBytesAt(Ptr, ReadSize);
			}
		}

		ViewFile.SetPointer(Ptr + ReadSize);

		if (!View || !ReadSize)
			return 0;

		if (Raw)
		{
			if (VM.CodePage == CP_UTF16LE || VM.CodePage == CP_UTF16BE) {
				ReadSize/= 2;
				for (DWORD i = 0; i < ReadSize; ++i)
				{
					Buf[i] = (unsigned char)View[i * 2 + 1];
					Buf[i]<<= 8;
					Buf[i]|= (unsigned char)View[i * 2];
				}
			} else {
				for (DWORD i = 0; i < ReadSize; i++)
				{
					Buf[i] = (wchar_t)(unsigned char)View[i];
				}
			}
		}
		else
		{
			ReadSize = WINPORT(MultiByteToWideChar)(VM.CodePage, 0, (const char *)View, ReadSize, Buf, Count);
		}

		return ReadSize;
	}
}


void Viewer::vseek(int64_t Offset,int Whence)
{
	switch (VM.CodePage)
	{
		case CP_UTF32BE: case CP_UTF32LE: Offset*= 4; break;
		case CP_UTF16BE: case CP_UTF16LE: Offset*= 2; break;
	}
	ViewFile.SetPointer(Offset, Whence);
}


int64_t Viewer::vtell()
{
	INT64 Ptr=0;
	ViewFile.GetPointer(Ptr);
	switch (VM.CodePage)
	{
		case CP_UTF32BE: case CP_UTF32LE: Ptr=(Ptr+(Ptr&3))/4; break;
		case CP_UTF16BE: case CP_UTF16LE: Ptr=(Ptr+(Ptr&1))/2; break;
	}
	return Ptr;
}


bool Viewer::vgetc(WCHAR& C)
{
	bool Result=false;

	if (vread(&C,1))
	{
		Result=true;
	}
	return Result;
}


#define RB_PRC 3
#define RB_HEX 4
#define RB_DEC 5

void Viewer::GoTo(int ShowDlg,int64_t Offset, DWORD Flags)
{
	int64_t Relative=0;
	const wchar_t *LineHistoryName=L"ViewerOffset";
	DialogDataEx GoToDlgData[]=
	{
		{DI_DOUBLEBOX,3,1,31,7,{0},0,MSG(MViewerGoTo)},
		{DI_EDIT,5,2,29,2,{(DWORD_PTR)LineHistoryName},DIF_FOCUS|DIF_DEFAULT|DIF_HISTORY|DIF_USELASTHISTORY,L""},
		{DI_TEXT,3,3,0,3,{0},DIF_SEPARATOR,L""},
		{DI_RADIOBUTTON,5,4,0,4,{0},DIF_GROUP,MSG(MGoToPercent)},
		{DI_RADIOBUTTON,5,5,0,5,{0},0,MSG(MGoToHex)},
		{DI_RADIOBUTTON,5,6,0,6,{0},0,MSG(MGoToDecimal)}
	};
	MakeDialogItemsEx(GoToDlgData,GoToDlg);
	static int PrevMode=0;
	GoToDlg[3].Selected=GoToDlg[4].Selected=GoToDlg[5].Selected=0;

	if (VM.Hex)
		PrevMode=1;

	GoToDlg[PrevMode+3].Selected=TRUE;
	{
		if (ShowDlg)
		{
			Dialog Dlg(GoToDlg,ARRAYSIZE(GoToDlg));
			Dlg.SetHelp(L"ViewerGotoPos");
			Dlg.SetPosition(-1,-1,35,9);
			Dlg.Process();

			if (Dlg.GetExitCode()<=0)
				return;

			if (GoToDlg[1].strData.At(0)==L'+' || GoToDlg[1].strData.At(0)==L'-')       // юзер хочет относительности
			{
				if (GoToDlg[1].strData.At(0)==L'+')
					Relative=1;
				else
					Relative=-1;

				GoToDlg[1].strData.LShift(1);
			}

			if (GoToDlg[1].strData.Contains(L'%'))     // он хочет процентов
			{
				GoToDlg[RB_HEX].Selected=GoToDlg[RB_DEC].Selected=0;
				GoToDlg[RB_PRC].Selected=1;
			}
			else if (!StrCmpNI(GoToDlg[1].strData,L"0x",2)
			         || GoToDlg[1].strData.At(0)==L'$'
			         || GoToDlg[1].strData.Contains(L'h')
			         || GoToDlg[1].strData.Contains(L'H'))  // он умный - hex код ввел!
			{
				GoToDlg[RB_PRC].Selected=GoToDlg[RB_DEC].Selected=0;
				GoToDlg[RB_HEX].Selected=1;

				if (!StrCmpNI(GoToDlg[1].strData,L"0x",2))
					GoToDlg[1].strData.LShift(2);
				else if (GoToDlg[1].strData.At(0)==L'$')
					GoToDlg[1].strData.LShift(1);

				//Relative=0; // при hex значении никаких относительных значений?
			}

			if (GoToDlg[RB_PRC].Selected)
			{
				//int cPercent=ToPercent64(FilePos,FileSize);
				PrevMode=0;
				int Percent=_wtoi(GoToDlg[1].strData);

				//if ( Relative  && (cPercent+Percent*Relative<0) || (cPercent+Percent*Relative>100)) // за пределы - низя
				//  return;
				if (Percent>100)
					return;

				//if ( Percent<0 )
				//  Percent=0;
				Offset=FileSize/100*Percent;

				switch (VM.CodePage) {
					case CP_UTF32LE: case CP_UTF32BE: Offset*= 4; break;
					case CP_UTF16LE: case CP_UTF16BE: Offset*= 2; break;
				}

				while (ToPercent64(Offset,FileSize)<Percent)
					Offset++;
			}

			if (GoToDlg[RB_HEX].Selected)
			{
				PrevMode=1;
				swscanf(GoToDlg[1].strData,L"%llx",&Offset);
			}

			if (GoToDlg[RB_DEC].Selected)
			{
				PrevMode=2;
				swscanf(GoToDlg[1].strData,L"%lld",&Offset);
			}
		}// ShowDlg
		else
		{
			Relative=(Flags&VSP_RELATIVE)*(Offset<0?-1:1);

			if (Flags&VSP_PERCENT)
			{
				int64_t Percent=Offset;

				if (Percent>100)
					return;

				//if ( Percent<0 )
				//  Percent=0;
				Offset=FileSize/100*Percent;

				switch (VM.CodePage) {
					case CP_UTF32LE: case CP_UTF32BE: Offset*= 4; break;
					case CP_UTF16LE: case CP_UTF16BE: Offset*= 2; break;
				}

				while (ToPercent64(Offset,FileSize)<Percent)
					Offset++;
			}
		}

		if (Relative)
		{
			if (Relative==-1 && Offset>FilePos)   // меньше нуля, if (FilePos<0) не пройдет - FilePos у нас uint32_t
				FilePos=0;
			else switch (VM.CodePage) {
					case CP_UTF32LE: case CP_UTF32BE: 
						FilePos = FilePos+Offset*Relative / 4;
						break;

					case CP_UTF16LE: case CP_UTF16BE:
						FilePos = FilePos+Offset*Relative / 2;
						break;

					default:
						FilePos = FilePos+Offset*Relative;

				}

		}
		else switch (VM.CodePage) {
			case CP_UTF32LE: case CP_UTF32BE: 
				FilePos = Offset / 4;
				break;

			case CP_UTF16LE: case CP_UTF16BE:
				FilePos = Offset / 2;
				break;

			default:
				FilePos = Offset;
		}

		if (FilePos>FileSize || FilePos<0)     // и куда его несет?
			FilePos=FileSize;     // там все равно ничего нету
	}
	// коррекция
	AdjustFilePos();

//  LastSelPos=FilePos;
	if (!(Flags&VSP_NOREDRAW))
		Show();
}

void Viewer::AdjustFilePos()
{
	if (!VM.Hex)
	{
		wchar_t Buf[4096];
		int64_t StartLinePos=-1,GotoLinePos=FilePos-(int64_t)sizeof(Buf)/sizeof(wchar_t);

		if (GotoLinePos<0)
			GotoLinePos=0;

		vseek(GotoLinePos,SEEK_SET);
		int ReadSize=(int)Min((int64_t)ARRAYSIZE(Buf),(int64_t)(FilePos-GotoLinePos));
		ReadSize=vread(Buf,ReadSize);


		for (int I=ReadSize-1; I>=0; I--)
            if (Buf[I]==(wchar_t)CRSym)
			{
				StartLinePos=GotoLinePos+I;
				break;
			}

		vseek(FilePos+1,SEEK_SET);

		if (VM.Hex) {
			size_t len = 8;
			switch (VM.CodePage) {
				case CP_UTF32LE: case CP_UTF32BE: 
					len*= 4;
				break;

				case CP_UTF16LE: case CP_UTF16BE:
					len*= 2;
				break;
			}

			FilePos&= ~(len - 1);
		}
		else
		{
			if (FilePos!=StartLinePos)
				Up();
		}
	}
}

void Viewer::SetFileSize()
{
	if (!ViewFile.Opened())
		return;

	UINT64 uFileSize=0; // BUGBUG, sign
	ViewFile.GetSize(uFileSize);
	FileSize=uFileSize;

	/* $ 20.02.2003 IS
	   Везде сравниваем FilePos с FileSize, FilePos для юникодных файлов
	   уменьшается в два раза, поэтому FileSize тоже надо уменьшать
	*/
	switch (VM.CodePage) {
		case CP_UTF32LE: case CP_UTF32BE: 
			FileSize=(FileSize+(FileSize&3)) / 4;
		break;

		case CP_UTF16LE: case CP_UTF16BE:
			FileSize=(FileSize+(FileSize&1)) / 2;
		break;
	}
}


void Viewer::GetSelectedParam(int64_t &Pos, int64_t &Length, DWORD &Flags)
{
	Pos=SelectPos;
	Length=SelectSize;
	Flags=SelectFlags;
}

/* $ 19.01.2001 SVS
   Выделение - в качестве самостоятельной функции.
   Flags=0x01 - показывать (делать Show())
         0x02 - "обратный поиск" ?
*/
void Viewer::SelectText(const int64_t &MatchPos,const int64_t &SearchLength, const DWORD Flags)
{
	if (!ViewFile.Opened())
		return;

	wchar_t Buf[1024];
	int64_t StartLinePos=-1,SearchLinePos=MatchPos-sizeof(Buf)/sizeof(wchar_t);

	if (SearchLinePos<0)
		SearchLinePos=0;

	vseek(SearchLinePos,SEEK_SET);
	int ReadSize=(int)Min((int64_t)ARRAYSIZE(Buf),(int64_t)(MatchPos-SearchLinePos));
	ReadSize=vread(Buf,ReadSize);

	for (int I=ReadSize-1; I>=0; I--)
        if (Buf[I]==(wchar_t)CRSym)
		{
			StartLinePos=SearchLinePos+I;
			break;
		}

//MessageBeep(0);
	vseek(MatchPos+1,SEEK_SET);
	SelectPos=FilePos=MatchPos;
	SelectSize=SearchLength;
	SelectFlags=Flags;

//  LastSelPos=SelectPos+((Flags&0x2) ? -1:1);
	LastSelPos = SelectPos + SearchLength * ((Flags & 0x2) ? -1 : 1);
	if (VM.Hex)
	{
		size_t len = 8;
		switch (VM.CodePage) {
			case CP_UTF32LE: case CP_UTF32BE: 
				len*= 4;
			break;

			case CP_UTF16LE: case CP_UTF16BE:
				len*= 2;
			break;
		}
		FilePos&= ~(len - 1);
	}
	else
	{
		if (SelectPos!=StartLinePos)
		{
			Up();
			Show();  //update OutStr
		}

		/* $ 13.03.2001 IS
		   Если найденное расположено в самой первой строке юникодного файла и файл
		   имеет в начале fffe или feff, то для более правильного выделения, его
		   позицию нужно уменьшить на единицу (из-за того, что пустой символ не
		   показывается)
		*/
		SelectPosOffSet=(IsUnicodeOrUtfCodePage(VM.CodePage) && Signature
		                 && (MatchPos+SelectSize<=ObjWidth && MatchPos<(int64_t)StrLength(Strings[0]->lpData)))?1:0;
		SelectPos-=SelectPosOffSet;
		int64_t Length=SelectPos-StartLinePos-1;

		if (VM.Wrap)
			Length%=Width+1; //??

		if (Length<=Width)
			LeftPos=0;

		if (Length-LeftPos>Width || Length<LeftPos)
		{
			LeftPos=Length;

			if (LeftPos>(MAX_VIEWLINE-1) || LeftPos<0)
				LeftPos=0;
			else if (LeftPos>10)
				LeftPos-=10;
		}
	}

	if (Flags&1)
	{
		AdjustSelPosition = TRUE;
		Show();
		AdjustSelPosition = FALSE;
	}
}

int Viewer::ViewerControl(int Command,void *Param)
{
	switch (Command)
	{
		case VCTL_GETINFO:
		{
			if (Param)
			{
				ViewerInfo *Info=(ViewerInfo *)Param;
				memset(&Info->ViewerID,0,Info->StructSize-sizeof(Info->StructSize));
				Info->ViewerID=ViewerID;
				Info->FileName=strFullFileName;
				Info->WindowSizeX=ObjWidth;
				Info->WindowSizeY=Y2-Y1+1;
				Info->FilePos=FilePos;
				Info->FileSize=FileSize;
				Info->CurMode=VM;
				Info->Options=0;

				if (Opt.ViOpt.SavePos)   Info->Options|=VOPT_SAVEFILEPOSITION;

				if (ViOpt.AutoDetectCodePage)     Info->Options|=VOPT_AUTODETECTCODEPAGE;

				Info->TabSize=ViOpt.TabSize;
				Info->LeftPos=LeftPos;
				return TRUE;
			}

			break;
		}
		/*
		   Param = ViewerSetPosition
		           сюда же будет записано новое смещение
		           В основном совпадает с переданным
		*/
		case VCTL_SETPOSITION:
		{
			if (Param)
			{
				ViewerSetPosition *vsp=(ViewerSetPosition*)Param;
				bool isReShow=vsp->StartPos != FilePos;

				if ((LeftPos=vsp->LeftPos) < 0)
					LeftPos=0;

				/* $ 20.01.2003 IS
				     Если кодировка - юникод, то оперируем числами, уменьшенными в
				     2 раза. Поэтому увеличим StartPos в 2 раза, т.к. функция
				     GoTo принимает смещения в _байтах_.
				*/

				int64_t NewPos = vsp->StartPos;

				switch (VM.CodePage) {
					case CP_UTF32LE: case CP_UTF32BE: 
						NewPos*= 4;
					break;

					case CP_UTF16LE: case CP_UTF16BE:
						NewPos*= 2;
					break;
				}

				GoTo(FALSE, NewPos, vsp->Flags);

				if (isReShow && !(vsp->Flags&VSP_NOREDRAW))
					ScrBuf.Flush();

				if (!(vsp->Flags&VSP_NORETNEWPOS))
				{
					vsp->StartPos=FilePos;
					vsp->LeftPos=LeftPos;
				}

				return TRUE;
			}

			break;
		}
		// Param=ViewerSelect
		case VCTL_SELECT:
		{
			if (Param)
			{
				ViewerSelect *vs=(ViewerSelect *)Param;
				int64_t SPos=vs->BlockStartPos;
				int SSize=vs->BlockLen;

				if (SPos < FileSize)
				{
					if (SPos+SSize > FileSize)
					{
						SSize=(int)(FileSize-SPos);
					}

					SelectText(SPos,SSize,0x1);
					ScrBuf.Flush();
					return TRUE;
				}
			}
			else
			{
				SelectSize = 0;
				Show();
			}

			break;
		}
		/* Функция установки Keybar Labels
		     Param = nullptr - восстановить, пред. значение
		     Param = -1   - обновить полосу (перерисовать)
		     Param = KeyBarTitles
		*/
		case VCTL_SETKEYBAR:
		{
			KeyBarTitles *Kbt=(KeyBarTitles*)Param;

			if (!Kbt)
			{        // восстановить пред значение!
				if (HostFileViewer)
					HostFileViewer->InitKeyBar();
			}
			else
			{
				if ((LONG_PTR)Param != (LONG_PTR)-1) // не только перерисовать?
				{
					for (int i=0; i < 12; i++)
					{
						if (Kbt->Titles[i])
							ViewKeyBar->Change(KBL_MAIN,Kbt->Titles[i],i);

						if (Kbt->CtrlTitles[i])
							ViewKeyBar->Change(KBL_CTRL,Kbt->CtrlTitles[i],i);

						if (Kbt->AltTitles[i])
							ViewKeyBar->Change(KBL_ALT,Kbt->AltTitles[i],i);

						if (Kbt->ShiftTitles[i])
							ViewKeyBar->Change(KBL_SHIFT,Kbt->ShiftTitles[i],i);

						if (Kbt->CtrlShiftTitles[i])
							ViewKeyBar->Change(KBL_CTRLSHIFT,Kbt->CtrlShiftTitles[i],i);

						if (Kbt->AltShiftTitles[i])
							ViewKeyBar->Change(KBL_ALTSHIFT,Kbt->AltShiftTitles[i],i);

						if (Kbt->CtrlAltTitles[i])
							ViewKeyBar->Change(KBL_CTRLALT,Kbt->CtrlAltTitles[i],i);
					}
				}

				ViewKeyBar->Show();
				ScrBuf.Flush(); //?????
			}

			return TRUE;
		}
		// Param=0
		case VCTL_REDRAW:
		{
			ChangeViewKeyBar();
			Show();
			ScrBuf.Flush();
			return TRUE;
		}
		// Param=0
		case VCTL_QUIT:
		{
			/* $ 28.12.2002 IS
			   Разрешаем выполнение VCTL_QUIT только для вьюера, который
			   не является панелью информации и быстрого просмотра (т.е.
			   фактически панелей на экране не видно)
			*/
			if (!FrameManager->IsPanelsActive())
			{
				/* $ 29.09.2002 IS
				   без этого не закрывался вьюер, а просили именно это
				*/
				FrameManager->DeleteFrame(HostFileViewer);

				if (HostFileViewer)
					HostFileViewer->SetExitCode(0);

				return TRUE;
			}
		}
		/* Функция установки режимов
		     Param = ViewerSetMode
		*/
		case VCTL_SETMODE:
		{
			ViewerSetMode *vsmode=(ViewerSetMode *)Param;

			if (vsmode)
			{
				bool isRedraw=vsmode->Flags&VSMFL_REDRAW?true:false;

				switch (vsmode->Type)
				{
					case VSMT_HEX:
						ProcessHexMode(vsmode->Param.iParam,isRedraw);
						return TRUE;
					case VSMT_WRAP:
						ProcessWrapMode(vsmode->Param.iParam,isRedraw);
						return TRUE;
					case VSMT_WORDWRAP:
						ProcessTypeWrapMode(vsmode->Param.iParam,isRedraw);
						return TRUE;
				}
			}

			return FALSE;
		}
	}

	return FALSE;
}

BOOL Viewer::isTemporary()
{
	return !strTempViewName.IsEmpty();
}

int Viewer::ProcessHexMode(int newMode, bool isRedraw)
{
	// BUGBUG
	// До тех пор, пока не будет реализован адекватный hex-просмотр в UTF8 - будем смотреть в OEM.
	// Ибо сейчас это не просмотр, а генератор однотипных унылых багрепортов.
	if (VM.CodePage==CP_UTF8 && newMode)
	{
		VM.CodePage=WINPORT(GetACP)();
	}

	int oldHex=VM.Hex;
	VM.Hex=newMode&1;

	if (isRedraw)
	{
		ChangeViewKeyBar();
		Show();
	}

// LastSelPos=FilePos;
	return oldHex;
}

int Viewer::ProcessWrapMode(int newMode, bool isRedraw)
{
	int oldWrap=VM.Wrap;
	VM.Wrap=newMode&1;

	if (VM.Wrap)
		LeftPos = 0;

	if (!VM.Wrap && LastPage)
		Up();

	if (isRedraw)
	{
		ChangeViewKeyBar();
		Show();
	}

	Opt.ViOpt.ViewerIsWrap=VM.Wrap;
//  LastSelPos=FilePos;
	return oldWrap;
}

int Viewer::ProcessTypeWrapMode(int newMode, bool isRedraw)
{
	int oldTypeWrap=VM.WordWrap;
	VM.WordWrap=newMode&1;

	if (!VM.Wrap)
	{
		VM.Wrap=!VM.Wrap;
		LeftPos = 0;
	}

	if (isRedraw)
	{
		ChangeViewKeyBar();
		Show();
	}

	Opt.ViOpt.ViewerWrap=VM.WordWrap;
//LastSelPos=FilePos;
	return oldTypeWrap;
}
