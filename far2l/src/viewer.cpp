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
#include "palette.hpp"
#include "keys.hpp"
#include "help.hpp"
#include "dialog.hpp"
#include "panel.hpp"
#include "filepanels.hpp"
#include "fileview.hpp"
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
#include "execute.hpp"
#include "constitle.hpp"
#include "console.hpp"
#include "AnsiEsc.hpp"
#include "wakeful.hpp"
#include "WideMB.h"
#include "UtfConvert.hpp"

#define MAX_VIEWLINE 0x2000

static void PR_ViewerSearchMsg();
static void ViewerSearchMsg(const wchar_t *Name, int Percent);

static int InitHex = FALSE, SearchHex = FALSE;

static int NextViewerID = 0;

static int CalcByteDistance(UINT CodePage, const wchar_t *begin, const wchar_t *end)
{
	if (begin > end)
		return -1;
#if (__WCHAR_MAX__ > 0xffff)
	if ((CodePage == CP_UTF32LE) || (CodePage == CP_UTF32BE)) {
		return (end - begin) * 4;
	}

	int distance;

	if ((CodePage == CP_UTF16LE) || (CodePage == CP_UTF16BE)) {
		distance = UtfCalcSpace<wchar_t, uint16_t>(begin, end - begin, false);
		distance*= sizeof(uint16_t);

	} else if (CodePage == CP_UTF8) {
		distance = UtfCalcSpace<wchar_t, uint8_t>(begin, end - begin, false);
	} else {	// one-byte code page?
		distance = end - begin;
	}

#else
	if ((CodePage == CP_UTF16LE) || (CodePage == CP_UTF16BE)) {
		return (end - begin) * 2;
	}

	int distance;

	if (CodePage == CP_UTF8) {
		distance = UtfCalcSpace<wchar_t, uint8_t>(begin, end - begin, false);

	} else {	// one-byte code page?
		distance = end - begin;
	}

#endif

	return distance;
}

static int CalcCodeUnitsDistance(UINT CodePage, const wchar_t *begin, const wchar_t *end)
{
	int distance = CalcByteDistance(CodePage, begin, end);
	if (distance > 0)
		switch (CodePage) {
			case CP_UTF32LE:
			case CP_UTF32BE:
				distance/= 4;
				break;
			case CP_UTF16LE:
			case CP_UTF16BE:
				distance/= 2;
				break;
		}
	return distance;
}

Viewer::Viewer(bool bQuickView, UINT aCodePage)
	:
	ViOpt(Opt.ViOpt), m_bQuickView(bQuickView)
{
	_OT(SysLog(L"[%p] Viewer::Viewer()", this));

	strLastSearchStr = strGlobalSearchString;
	LastSearchCase = GlobalSearchCase;
	LastSearchRegexp = Opt.ViOpt.SearchRegexp;
	LastSearchWholeWords = GlobalSearchWholeWords;
	LastSearchReverse = GlobalSearchReverse;
	LastSearchHex = GlobalSearchHex;
	VM.CodePage = DefCodePage = aCodePage;
	// Вспомним тип врапа
	VM.Wrap = Opt.ViOpt.ViewerIsWrap;
	VM.WordWrap = Opt.ViOpt.ViewerWrap;
	VM.Hex = InitHex;
	VM.Processed = 0;
	ViewKeyBar = nullptr;
	FilePos = 0;
	LeftPos = 0;
	SecondPos = 0;
	FileSize = 0;
	LastPage = 0;
	SelectPos = SelectSize = 0;
	LastSelPos = 0;
	SetStatusMode(TRUE);
	HideCursor = TRUE;
	CodePageChangedByUser = FALSE;
	memset(&BMSavePos, 0xff, sizeof(BMSavePos));
	memset(UndoData, 0xff, sizeof(UndoData));
	LastKeyUndo = FALSE;
	InternalKey = FALSE;
	ViewerID = ::NextViewerID++;
	CtrlObject->Plugins.CurViewer = this;
	OpenFailed = false;
	HostFileViewer = nullptr;
	bVE_READ_Sent = false;
}

FARString Viewer::ComposeCacheName()
{
	//	FARString strCacheName=strPluginData.IsEmpty()?FHP->GetPathName():strPluginData+PointToName(FHP->GetPathName());
	FARString strCacheName =
			strPluginData.IsEmpty() ? strFullFileName : strPluginData + PointToName(FHP->GetPathName());
	if (VM.Processed) {
		strCacheName+= L":PROCESSED";
	}

	return strCacheName;
}

void Viewer::SavePosCache()
{
	if (Opt.OnlyEditorViewerUsed == Options::ONLY_VIEWER_ON_CMDOUT || !CtrlObject)
		return;

	FARString strCacheName = ComposeCacheName();

	if (Opt.ViOpt.SavePos) {
		UINT CodePage = 0;

		if (CodePageChangedByUser) {
			CodePage = VM.CodePage;
		}

		PosCache poscache = {};
		poscache.Param[0] = FilePos;
		poscache.Param[1] = LeftPos;
		poscache.Param[2] = VM.Hex;
		//=poscache.Param[3];
		poscache.Param[4] = CodePage;

		if (Opt.ViOpt.SaveShortPos) {
			poscache.Position[0] = BMSavePos.SavePosAddr;
			poscache.Position[1] = BMSavePos.SavePosLeft;
			// poscache.Position[2]=;
			// poscache.Position[3]=;
		}

		CtrlObject->ViewerPosCache->AddPosition(strCacheName, poscache);
	} else {
		CtrlObject->ViewerPosCache->ResetPosition(strCacheName);
	}
}

Viewer::~Viewer()
{
	KeepInitParameters();

	if (ViewFile.Opened()) {
		ViewFile.Close();
		SavePosCache();
	}

	_tran(SysLog(L"[%p] Viewer::~Viewer, TempViewName=[%ls]", this, TempViewName));
	/*
		$ 11.10.2001 IS
		Удаляем файл только, если нет открытых фреймов с таким именем.
	*/

	if (!strProcessedViewName.IsEmpty()) {
		unlink(strProcessedViewName.GetMB().c_str());
		CutToSlash(strProcessedViewName);
		if (!strProcessedViewName.IsEmpty()) {
			rmdir(strProcessedViewName.GetMB().c_str());
			strProcessedViewName.Clear();
		}
	}

	if (!OpenFailed && bVE_READ_Sent) {
		CtrlObject->Plugins.CurViewer = this;	// HostFileViewer;
		CtrlObject->Plugins.ProcessViewerEvent(VE_CLOSE, &ViewerID);
	}

	if (CtrlObject->Plugins.CurViewer == this) {
		CtrlObject->Plugins.CurViewer = nullptr;
	}
}

void Viewer::KeepInitParameters()
{
	strGlobalSearchString = strLastSearchStr;
	GlobalSearchCase = LastSearchCase;
	GlobalSearchWholeWords = LastSearchWholeWords;
	GlobalSearchReverse = LastSearchReverse;
	GlobalSearchHex = LastSearchHex;
	Opt.ViOpt.ViewerIsWrap = VM.Wrap;
	Opt.ViOpt.ViewerWrap = VM.WordWrap;
	Opt.ViOpt.SearchRegexp = LastSearchRegexp;
	InitHex = VM.Hex;
}

int Viewer::OpenFile(FileHolderPtr NewFileHolder, int warning)
{
	VM.CodePage = DefCodePage;
	DefCodePage = CP_AUTODETECT;
	OpenFailed = false;

	ViewFile.Close();

	const auto &GotPathName = NewFileHolder->GetPathName();
	DWORD FileAttr = apiGetFileAttributes(GotPathName);
	if (FileAttr != INVALID_FILE_ATTRIBUTES && (FileAttr & FILE_ATTRIBUTE_DEVICE) != 0) {	// avoid stuck
		OpenFailed = TRUE;
		return FALSE;
	}

	SelectSize = 0;		// Сбросим выделение
	//strFileName = NewFileHolder->GetPathName();

	FARString OpenPathName = GotPathName;
	//	Processed mode:
	//		Renders ANSI ESC coloring sequences
	//		For all files beside *.ansi/*.ans runs view.sh
	//			that doing 'processing' of file and writes output
	//			into temporary filename. That temporary file then
	//			viewed instead of original one's.
	const wchar_t *ext = wcsrchr(GotPathName, L'.');
	if (ext && (wcscasecmp(ext, L".ansi") == 0 || wcscasecmp(ext, L".ans") == 0)) {
		VM.Processed = 1;
		VM.Wrap = 0;
		if (VM.CodePage == CP_AUTODETECT) {
			VM.CodePage = 437;
		}
	} else if (VM.Processed) {
		if (strProcessedViewName.IsEmpty()) {
			if (FarMkTempEx(strProcessedViewName, L"view")) {
				strProcessedViewName+= PointToName(GotPathName);

				std::string cmd = GetMyScriptQuoted("view.sh");
				std::string strFile = GotPathName.GetMB();

				QuoteCmdArgIfNeed(strFile);
				cmd+= ' ';
				cmd+= strFile;

				strFile = strProcessedViewName.GetMB();
				QuoteCmdArgIfNeed(strFile);
				cmd+= ' ';
				cmd+= strFile;
				int r = farExecuteA(cmd.c_str(), 0);
				if (r == 0) {
					OpenPathName = strProcessedViewName.CPtr();
				} else {
					unlink(strProcessedViewName.GetMB().c_str());
					strProcessedViewName.Clear();
				}
			}
		} else {
			OpenPathName = strProcessedViewName.CPtr();
		}
	}

	ViewFile.Open(OpenPathName.GetMB());	// strFileName.GetMB()

	if (!ViewFile.Opened()) {
		/*
			$ 04.07.2000 tran
			+ 'warning' flag processing, in QuickView it is FALSE
			so don't show red message box
		*/
		if (warning) {
			Message(MSG_WARNING | MSG_ERRORTYPE, 1, Msg::ViewerTitle, Msg::ViewerCannotOpenFile, GotPathName, Msg::Ok);
		}

		OpenFailed = true;
		return FALSE;
	}

	FHP = NewFileHolder;
	CodePageChangedByUser = FALSE;

	ConvertNameToFull(GotPathName, strFullFileName);
	apiGetFindDataForExactPathName(GotPathName, ViewFindData);
	UINT CachedCodePage = 0;

	if (Opt.ViOpt.SavePos) {
		int64_t NewLeftPos, NewFilePos;
		FARString strCacheName = ComposeCacheName();
		memset(&BMSavePos, 0xff, sizeof(BMSavePos));	// заполним с -1
		PosCache poscache = {};

		if (Opt.ViOpt.SaveShortPos) {
			poscache.Position[0] = BMSavePos.SavePosAddr;
			poscache.Position[1] = BMSavePos.SavePosLeft;
			// poscache.Position[2]=;
			// poscache.Position[3]=;
		}

		CtrlObject->ViewerPosCache->GetPosition(strCacheName, poscache);
		NewFilePos = poscache.Param[0];
		NewLeftPos = poscache.Param[1];
		VM.Hex = (int)poscache.Param[2];
		//=poscache.Param[3];
		CachedCodePage = (UINT)poscache.Param[4];

		// Проверяем поддерживается или нет загруженная из кэша кодовая страница
		if (CachedCodePage && !IsCodePageSupported(CachedCodePage))
			CachedCodePage = 0;
		LastSelPos = FilePos = NewFilePos;
		LeftPos = NewLeftPos;
	} else {
		FilePos = 0;
	}

	/*
		$ 26.07.2002 IS
		Автоопределение Unicode не должно зависеть от опции
		"Автоопределение таблицы символов", т.к. Unicode не есть
		_таблица символов_ для перекодировки.
	*/
	// if(ViOpt.AutoDetectTable)
	{
		bool Detect = false;
		UINT CodePage = 0;

		if (VM.CodePage == CP_AUTODETECT || IsUnicodeOrUtfCodePage(VM.CodePage)) {
			Detect = GetFileFormat2(GotPathName, CodePage, nullptr, Opt.ViOpt.AutoDetectCodePage != 0, true);
		}

		if (VM.CodePage == CP_AUTODETECT) {
			if (Detect) {
				VM.CodePage = CodePage;
			}

			if (CachedCodePage) {
				VM.CodePage = CachedCodePage;
				CodePageChangedByUser = TRUE;
			}

			if (VM.CodePage == CP_AUTODETECT)
				VM.CodePage = Opt.ViOpt.DefaultCodePage;
		} else {
			CodePageChangedByUser = TRUE;
		}

		// BUGBUG
		// пока что запретим переключать hex в UTF8/UTF32, ибо не работает.
		if (VM.Hex && (VM.CodePage == CP_UTF8 || VM.CodePage == CP_UTF32LE || VM.CodePage == CP_UTF32BE)) {
			VM.CodePage = WINPORT(GetACP)();
		}

		if (!IsUnicodeOrUtfCodePage(VM.CodePage)) {
			ViewFile.SetPointer(0);
		}
	}
	SetFileSize();

	if (FilePos > FileSize)
		FilePos = 0;

	SetCRSym();
	// if (ViOpt.AutoDetectTable && !TableChangedByUser)
	//{
	// }
	ChangeViewKeyBar();
	AdjustWidth();
	CtrlObject->Plugins.CurViewer = this;	// HostFileViewer;
	/*
		$ 15.09.2001 tran
		пора легализироваться
	*/
	CtrlObject->Plugins.ProcessViewerEvent(VE_READ, nullptr);
	bVE_READ_Sent = true;
	return TRUE;
}

/*
	$ 27.04.2001 DJ
	функция вычисления ширины в зависимости от наличия скроллбара
*/

void Viewer::AdjustWidth()
{
	Width = X2 - X1 + 1;
	XX2 = X2;

	if (ViOpt.ShowScrollbar && !m_bQuickView) {
		Width--;
		XX2--;
	}
}

void Viewer::SetCRSym()
{
	if (!ViewFile.Opened())
		return;

	wchar_t Buf[2048];
	int CRCount = 0, LFCount = 0;
	int ReadSize, I;
	vseek(0, SEEK_SET);
	ReadSize = vread(Buf, ARRAYSIZE(Buf));

	for (I = 0; I < ReadSize; I++)
		switch (Buf[I]) {
			case L'\n':
				LFCount++;
				break;
			case L'\r':

				if (I + 1 >= ReadSize || Buf[I + 1] != L'\n')
					CRCount++;

				break;
		}

	if (LFCount < CRCount)
		CRSym = L'\r';
	else
		CRSym = L'\n';
}

void Viewer::ShowPage(int nMode)
{
	int I, Y;
	AdjustWidth();

	if (!ViewFile.Opened()) {
		if (!FHP->GetPathName().IsEmpty() && ((nMode == SHOW_RELOAD) || (nMode == SHOW_HEX))) {
			SetScreen(X1, Y1, X2, Y2, L' ', FarColorToReal(COL_VIEWERTEXT));
			GotoXY(X1, Y1);
			SetFarColor(COL_WARNDIALOGTEXT);
			FS << fmt::Cells() << fmt::Truncate(XX2 - X1 + 1) << Msg::ViewerCannotOpenFile;
			ShowStatus();
		}

		return;
	}

	if (HideCursor)
		SetCursorType(0, 10);

	vseek(FilePos, SEEK_SET);

	if (!SelectSize)
		SelectPos = FilePos;

	switch (nMode) {
		case SHOW_HEX:
			CtrlObject->Plugins.CurViewer = this;	// HostFileViewer;
			ShowHex();
			break;
		case SHOW_RELOAD:
			CtrlObject->Plugins.CurViewer = this;	// HostFileViewer;

			for (I = 0, Y = Y1; Y <= Y2; Y++, I++) {
				Strings[I].nFilePos = vtell();

				if (Y == Y1 + 1 && !ViewFile.Eof())
					SecondPos = vtell();

				ReadString(Strings[I], -1, MAX_VIEWLINE);
			}
			break;

		case SHOW_UP:
			Strings.ScrollUp();
			Strings[0].nFilePos = FilePos;
			SecondPos = Strings[1].nFilePos;
			ReadString(Strings[0], (int)(SecondPos - FilePos), MAX_VIEWLINE);
			break;

		case SHOW_DOWN:
			vseek(Strings[Y2 - Y1].nFilePos, SEEK_SET);
			Strings.ScrollDown();
			FilePos = Strings[0].nFilePos;
			SecondPos = Strings[1].nFilePos;
			ReadString(Strings[Y2 - Y1], -1, MAX_VIEWLINE);
			Strings[Y2 - Y1].nFilePos = vtell();
			ReadString(Strings[Y2 - Y1], -1, MAX_VIEWLINE);
			break;
	}

	if (nMode != SHOW_HEX) {
		std::unique_ptr<ViewerPrinter> printer;
		if (VM.Processed)
			printer.reset(new AnsiEsc::Printer(B_BLACK | F_WHITE));
		else
			printer.reset(new PlainViewerPrinter(FarColorToReal(COL_VIEWERTEXT)));
		if (IsUnicodeOrUtfCodePage(VM.CodePage))
			printer->EnableBOMSkip();

		for (I = 0, Y = Y1; Y <= Y2; Y++, I++) {
			int StrLen = printer->Length(Strings[I].Chars());
			GotoXY(X1, Y);

			printer->Print(LeftPos, Width, Strings[I].Chars());

			auto &StringI = Strings[I];
			if (SelectSize && StringI.bSelection) {
				auto visualSelStart = printer->Length(StringI.Chars(), StringI.nSelStart);
				auto visualSelLength = printer->Length(StringI.Chars(StringI.nSelStart),
						StringI.nSelEnd - StringI.nSelStart);

				if (!VM.Wrap && AdjustSelPosition
						&& (visualSelStart < LeftPos
								|| (visualSelStart > LeftPos
										&& visualSelStart + visualSelLength > LeftPos + XX2 - X1))) {
					LeftPos = visualSelStart > 1 ? visualSelStart - 1 : 0;
					AdjustSelPosition = FALSE;
					Show();
					return;
				}

				int SelX1 = X1, SelSkip = 0;
				if (visualSelStart > LeftPos)
					SelX1+= visualSelStart - LeftPos;
				else if (visualSelStart < LeftPos)
					SelSkip = LeftPos - visualSelStart;

				if (visualSelLength > SelSkip) {
					GotoXY((int)SelX1, Y);
					//					PlainViewerPrinter selPrinter(COL_VIEWERSELECTEDTEXT);
					//					if (IsUnicodeOrUtfCodePage(VM.CodePage))
					//						selPrinter.EnableBOMSkip();
					printer->SetSelection(true);
					printer->Print(SelSkip, visualSelLength - SelSkip, StringI.Chars(StringI.nSelStart));
					printer->SetSelection(false);
				}
			}

			if (StrLen > LeftPos + Width && ViOpt.ShowArrows) {
				GotoXY(XX2, Y);
				SetFarColor(COL_VIEWERARROWS);
				BoxText(0xbb);
			}

			if (LeftPos > 0 && ViOpt.ShowArrows && !Strings[I].IsEmpty()) {
				GotoXY(X1, Y);
				SetFarColor(COL_VIEWERARROWS);
				BoxText(0xab);
			}
		}
	}

	DrawScrollbar();
	ShowStatus();
}

void Viewer::DisplayObject()
{
	ShowPage(VM.Hex ? SHOW_HEX : SHOW_RELOAD);
}

void Viewer::ShowHex()
{
	wchar_t OutStr[MAX_VIEWLINE], TextStr[20];
	int EndFile;
	//	int64_t SelSize;
	WCHAR Ch;
	int X, Y, TextPos;
	int SelStart, SelEnd;
	bool bSelStartFound = false, bSelEndFound = false;
	int64_t HexLeftPos = ((LeftPos > 80 - ObjWidth) ? Max(80 - ObjWidth, 0) : LeftPos);

	for (EndFile = 0, Y = Y1; Y <= Y2; Y++) {
		bSelStartFound = false;
		bSelEndFound = false;
		//		SelSize=0;
		SetFarColor(COL_VIEWERTEXT);
		GotoXY(X1, Y);

		if (EndFile) {
			FS << fmt::Cells() << fmt::Expand(ObjWidth) << L"";
			continue;
		}

		if (Y == Y1 + 1 && !ViewFile.Eof())
			SecondPos = vtell();

		INT64 Ptr = 0;
		ViewFile.GetPointer(Ptr);
		swprintf(OutStr, ARRAYSIZE(OutStr), L"%010llX: ", Ptr);
		TextPos = 0;
		int HexStrStart = (int)wcslen(OutStr);
		SelStart = HexStrStart;
		SelEnd = SelStart;
		int64_t fpos = vtell();

		if (fpos > SelectPos)
			bSelStartFound = true;

		if (fpos < SelectPos + SelectSize - 1)
			bSelEndFound = true;

		if (!SelectSize)
			bSelStartFound = bSelEndFound = false;

		const wchar_t BorderLine[] = {BoxSymbols[BS_V1], L' ', 0};

		if (VM.CodePage == CP_UTF16LE || VM.CodePage == CP_UTF16BE) {
			for (X = 0; X < 8; X++) {
				int64_t fpos = vtell();

				if (SelectSize > 0 && (SelectPos == fpos)) {
					bSelStartFound = true;
					SelStart = (int)wcslen(OutStr);
					//					SelSize=SelectSize;
					/*
						$ 22.01.2001 IS
						Внимание! Возможно, это не совсем верное решение проблемы
						выделения из плагинов, но мне пока другого в голову не пришло.
						Я приравниваю SelectSize нулю в Process*
					*/
					// SelectSize=0;
				}

				if (SelectSize > 0 && (fpos == (SelectPos + SelectSize - 1))) {
					bSelEndFound = true;
					SelEnd = (int)wcslen(OutStr) + 3;
					//					SelSize=SelectSize;
				}

				if (!vgetc(Ch)) {
					/*
						$ 28.06.2000 tran
						убираем показ пустой строки, если длина
						файла кратна 16
					*/
					EndFile = 1;
					LastPage = 1;

					if (!X) {
						wcscpy(OutStr, L"");
						break;
					}

					wcscat(OutStr, L"     ");
					TextStr[TextPos++] = L' ';
				} else {
					WCHAR OutChar = (VM.CodePage == CP_UTF16BE) ? RevBytes(uint16_t(Ch)) : Ch;

					int OutStrLen = StrLength(OutStr);
					swprintf(OutStr + OutStrLen, ARRAYSIZE(OutStr) - OutStrLen, L"%02X%02X ",
							(unsigned int)HIBYTE(OutChar), (unsigned int)LOBYTE(OutChar));

					if (!Ch) {
						Ch = L' ';
					}

					TextStr[TextPos++] = Ch;
					LastPage = 0;
				}

				if (X == 3)
					wcscat(OutStr, BorderLine);
			}
		} else {
			for (X = 0; X < 16; X++) {
				int64_t fpos = vtell();

				if (SelectSize > 0 && (SelectPos == fpos)) {
					bSelStartFound = true;
					SelStart = (int)wcslen(OutStr);
					//					SelSize=SelectSize;
					/*
						$ 22.01.2001 IS
						Внимание! Возможно, это не совсем верное решение проблемы
						выделения из плагинов, но мне пока другого в голову не пришло.
						Я приравниваю SelectSize нулю в Process*
					*/
					// SelectSize=0;
				}

				if (SelectSize > 0 && (fpos == (SelectPos + SelectSize - 1))) {
					bSelEndFound = true;
					SelEnd = (int)wcslen(OutStr) + 1;
					//					SelSize=SelectSize;
				}

				if (!vgetc(Ch)) {
					/*
						$ 28.06.2000 tran
						убираем показ пустой строки, если длина
						файла кратна 16
					*/
					EndFile = 1;
					LastPage = 1;

					if (!X) {
						wcscpy(OutStr, L"");
						break;
					}

					/*
						$ 03.07.2000 tran
						- вместо 5 пробелов тут надо 3
					*/
					wcscat(OutStr, L"   ");
					TextStr[TextPos++] = L' ';
				} else {
					char NewCh;
					WINPORT(WideCharToMultiByte)(VM.CodePage, 0, &Ch, 1, &NewCh, 1, " ", nullptr);
					int OutStrLen = StrLength(OutStr);
					swprintf(OutStr + OutStrLen, ARRAYSIZE(OutStr) - OutStrLen, L"%02X ",
							(unsigned int)(unsigned char)NewCh);

					if (!Ch)
						Ch = L' ';

					TextStr[TextPos++] = Ch;
					LastPage = 0;
				}

				if (X == 7)
					wcscat(OutStr, BorderLine);
			}
		}

		TextStr[TextPos] = 0;
		wcscat(TextStr, L" ");

		if ((SelEnd <= SelStart) && bSelStartFound)
			SelEnd = (int)wcslen(OutStr) - 2;

		wcscat(OutStr, L" ");
		wcscat(OutStr, TextStr);
#if 0

		for (size_t I=0; I < wcslen(OutStr); ++I)
			if (OutStr[I] == (wchar_t)0xFFFF)
				OutStr[I]=L'?';

#endif

		if (StrLength(OutStr) > HexLeftPos) {
			FS << fmt::Cells() << fmt::LeftAlign() << fmt::Size(ObjWidth)
				<< OutStr + static_cast<size_t>(HexLeftPos);
		} else {
			FS << fmt::Cells() << fmt::Expand(ObjWidth) << L"";
		}

		if (bSelStartFound && bSelEndFound) {
			SetFarColor(COL_VIEWERSELECTEDTEXT);
			GotoXY((int)((int64_t)X1 + SelStart - HexLeftPos), Y);
			FS << fmt::Cells() << fmt::Truncate(SelEnd - SelStart + 1)
				<< OutStr + static_cast<size_t>(SelStart);
			//			SelSize = 0;
		}
	}
}

/*
	$ 27.04.2001 DJ
	отрисовка скроллбара - в отдельную функцию
*/

void Viewer::DrawScrollbar()
{
	if (ViOpt.ShowScrollbar) {
		if (m_bQuickView)
			SetFarColor(COL_PANELSCROLLBAR);
		else
			SetFarColor(COL_VIEWERSCROLLBAR);

		if (!VM.Hex) {
			ScrollBar(X2 + (m_bQuickView ? 1 : 0), Y1, Y2 - Y1 + 1,
					(LastPage ? (!FilePos ? 0 : 100) : ToPercent64(FilePos, FileSize)), 100);
		} else {
			UINT64 Total = FileSize / 16 + (FileSize % 16 ? 1 : 0);
			UINT64 Top = FilePos / 16 + (FilePos % 16 ? 1 : 0);
			ScrollBarEx(X2 + (m_bQuickView ? 1 : 0), Y1, Y2 - Y1 + 1, LastPage ? Top ? Total : 0 : Top,
					Total);
		}
	}
}

FARString &Viewer::GetTitle(FARString &strName, int, int)
{
	if (!strTitle.IsEmpty()) {
		strName = strTitle;
	} else {
		if (!IsAbsolutePath(FHP->GetPathName())) {
			FARString strPath;
			ViewNamesList.GetCurDir(strPath);
			AddEndSlash(strPath);
			strName = strPath + FHP->GetPathName();
		} else {
			strName = FHP->GetPathName();
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
	ShowStatusLine = Mode;
}

void Viewer::ReadString(ViewerString &rString, int MaxSize, int StrSize)
{
	WCHAR Ch, Ch2;
	int64_t OutPtr;
	bool bSelStartFound = false, bSelEndFound = false;
	rString.bSelection = false;
	AdjustWidth();
	OutPtr = 0;
	rString.SetChar(0, 0);

	if (VM.Hex) {
		wchar_t piece[16 + 1]{0};
		size_t len = ARRAYSIZE(piece) - 1;
		// Alter-1: ::vread accepts number of codepoint units:
		// 4-bytes for UTF32, 2-bytes for UTF16 and 1-bytes for everything else
		// But we always display 16 bytes
		switch (VM.CodePage) {
			case CP_UTF32LE:
			case CP_UTF32BE:
				len/= 4;
				break;
			case CP_UTF16LE:
			case CP_UTF16BE:
				len/= 2;
				break;
		}	// TODO: ???

		OutPtr = vread(piece, len);
		piece[OutPtr] = 0;
		rString.SetChars(0, piece, (size_t)(OutPtr + 1));
	} else {
		bool CRSkipped = false;

		if (SelectSize && vtell() > SelectPos) {
			rString.nSelStart = 0;
			bSelStartFound = true;
		}

		for (;;) {
			if (OutPtr >= StrSize - 16)
				break;

			/*
				$ 12.07.2000 SVS
				! Wrap - трехпозиционный
			*/
			if (VM.Wrap && OutPtr > XX2 - X1) {
				/*
					$ 11.07.2000 tran
					+ warp are now WORD-WRAP
				*/
				int64_t SavePos = vtell();
				WCHAR TmpChar = 0;

				if (vgetc(Ch) && Ch != CRSym && (Ch != L'\r' || (vgetc(TmpChar) && TmpChar != CRSym))) {
					vseek(SavePos, SEEK_SET);

					if (VM.WordWrap) {
						if (!IsSpace(Ch) && !IsSpace(*rString.Chars((size_t)OutPtr))) {
							int64_t SavePtr = OutPtr;

							/*
								$ 18.07.2000 tran
								добавил в качестве wordwrap разделителей , ; > )
							*/
							while (OutPtr) {
								Ch2 = *rString.Chars((size_t)OutPtr);

								if (IsSpace(Ch2) || Ch2 == L',' || Ch2 == L';' || Ch2 == L'>' || Ch2 == L')')
									break;

								OutPtr--;
							}

							Ch2 = *rString.Chars((size_t)OutPtr);

							if (Ch2 == L',' || Ch2 == L';' || Ch2 == L')' || Ch2 == L'>')
								OutPtr++;
							else
								while (OutPtr <= SavePtr && IsSpace(*rString.Chars((size_t)OutPtr)))
									OutPtr++;

							if (OutPtr < SavePtr && OutPtr) {
								vseek(-CalcCodeUnitsDistance(VM.CodePage, rString.Chars((size_t)OutPtr),
											rString.Chars((size_t)SavePtr)),
										SEEK_CUR);
							} else
								OutPtr = SavePtr;
						}

						/*
							$ 13.09.2000 tran
							remove space at WWrap
						*/

						if (IsSpace(Ch)) {
							int64_t lastpos;
							for (;;) {
								lastpos = vtell();
								if (!vgetc(Ch) || !IsSpace(Ch))
									break;
							}
							vseek(lastpos, SEEK_SET);
						}

					}	// wwrap
				}

				break;
			}

			if (SelectSize > 0 && SelectPos == vtell()) {
				rString.nSelStart = OutPtr + (CRSkipped ? 1 : 0);
				;
				bSelStartFound = true;
			}

			if (MaxSize == 0)
				break;

			if (!vgetc(Ch))
				break;

			if (MaxSize > 0) {
				if (VM.CodePage == CP_UTF8) {
					MaxSize-= UtfCalcSpace<wchar_t, char>(&Ch, 1, false);
					if (MaxSize < 0) {
						MaxSize = 0;
					}
				} else {
					MaxSize--;
				}
			}

			if (Ch == CRSym)
				break;

			if (CRSkipped) {
				CRSkipped = false;
				rString.SetChar(size_t(OutPtr++), L'\r');
			}

			if (Ch == L'\t') {
				do {
					rString.SetChar(size_t(OutPtr++), L' ');
				} while ((OutPtr % ViOpt.TabSize) && ((int)OutPtr < (MAX_VIEWLINE - 1)));

				if (VM.Wrap && OutPtr > XX2 - X1)
					rString.SetChar(XX2 - X1 + 1, 0);

				continue;
			}

			/*
				$ 20.09.01 IS
				Баг: не учитывали левую границу при свертке
			*/
			if (Ch == L'\r') {
				CRSkipped = true;

				if (OutPtr >= XX2 - X1) {
					int64_t SavePos = vtell();
					WCHAR nextCh = 0;

					if (vgetc(nextCh) && nextCh != CRSym) {
						CRSkipped = false;
					}

					vseek(SavePos, SEEK_SET);
				}

				if (CRSkipped)
					continue;
			}

			if (!Ch || Ch == L'\n')
				Ch = L' ';

			rString.SetChar(size_t(OutPtr++), Ch);
			rString.SetChar(size_t(OutPtr), 0);

			if (SelectSize > 0 && (SelectPos + SelectSize) == vtell()) {
				rString.nSelEnd = OutPtr;
				bSelEndFound = true;
			}
		}
	}

	rString.SetChar(size_t(OutPtr), 0);

	if (!bSelEndFound && SelectSize && vtell() < SelectPos + SelectSize) {
		bSelEndFound = true;
		rString.nSelEnd = wcslen(rString.Chars());
	}

	if (bSelStartFound) {
		if (rString.nSelStart > (int64_t)wcslen(rString.Chars()))
			bSelStartFound = false;

		if (bSelEndFound)
			if (rString.nSelStart > rString.nSelEnd)
				bSelStartFound = false;
	}

	LastPage = ViewFile.Eof();

	if (bSelStartFound && bSelEndFound)
		rString.bSelection = true;
}

int64_t Viewer::VMProcess(MacroOpcode OpCode, void *vParam, int64_t iParam)
{
	switch (OpCode) {
		case MCODE_C_EMPTY:
			return (int64_t)!FileSize;
		case MCODE_C_SELECTED:
			return (int64_t)(SelectSize ? TRUE : FALSE);
		case MCODE_C_EOF:
			return (int64_t)(LastPage || !ViewFile.Opened());
		case MCODE_C_BOF:
			return (int64_t)(!FilePos || !ViewFile.Opened());
		case MCODE_V_VIEWERSTATE: {
			DWORD MacroViewerState = 0;
			MacroViewerState|= VM.Wrap ? 0x00000008 : 0;
			MacroViewerState|= VM.WordWrap ? 0x00000010 : 0;
			MacroViewerState|= VM.Hex ? 0x00000020 : 0;
			MacroViewerState|= Opt.OnlyEditorViewerUsed ? 0x08000000 | 0x00000800 : 0;
			MacroViewerState|= HostFileViewer && !HostFileViewer->GetCanLoseFocus() ? 0x00000800 : 0;
			return (int64_t)MacroViewerState;
		}
		case MCODE_V_ITEMCOUNT:		// ItemCount - число элементов в текущем объекте
			return (int64_t)GetViewFileSize();
		case MCODE_V_CURPOS:		// CurPos - текущий индекс в текущем объекте
			return (int64_t)(GetViewFilePos() + 1);
	}

	return 0;
}

/*
	$ 28.01.2001
	- Путем проверки ViewFile на nullptr избавляемся от падения
*/
int Viewer::ProcessKey(FarKey Key)
{
	// Pressing Alt together with PageDown/PageUp allows to smoothly
	// boost scrolling speed, releasing Alt while keeping PageDown/PageUp
	// will continue scrolling with selected speed. Pressing any other key
	// or releasing keys until KEY_IDLE event dismisses scroll speed boost.
	if (Key == KEY_ALTPGUP || Key == KEY_ALTPGDN) {
		if (iBoostPg < 0x100000)
			++iBoostPg;
	} else if (Key != KEY_PGUP && Key != KEY_PGDN && Key != KEY_NONE && iBoostPg != 0) {
		fprintf(stderr, "Dismiss iBoostPg=%u due to Key=0x%x\n", iBoostPg, Key);
		iBoostPg = 0;
	}

	if (Key == KEY_SHIFTLEFT || Key == KEY_SHIFTRIGHT) {
		if (SelectSize > 0 && (SelectPos > 0 || Key == KEY_SHIFTRIGHT)) {
			int64_t NewSelectSize = SelectSize + 1;
			int64_t NewSelectPos = SelectPos;
			if (Key == KEY_SHIFTLEFT)
				--NewSelectPos;
			//			fprintf(stderr, "SELECTIO CHANGE: [%ld +%ld)\n", (unsigned long)NewSelectPos, NewSelectSize);
			SelectText(NewSelectPos, NewSelectSize, SelectFlags);
		}
		return TRUE;
	}

	/*
		$ 22.01.2001 IS
		Происходят какие-то манипуляции -> снимем выделение
	*/
	if (!ViOpt.PersistentBlocks && Key != KEY_IDLE && Key != KEY_NONE
			&& !(Key == KEY_CTRLINS || Key == KEY_CTRLNUMPAD0) && Key != KEY_CTRLC)
		SelectSize = 0;

	if (!InternalKey && !LastKeyUndo
			&& (FilePos != UndoData[0].UndoAddr || LeftPos != UndoData[0].UndoLeft)) {
		for (int i = ARRAYSIZE(UndoData) - 1; i > 0; i--) {
			UndoData[i].UndoAddr = UndoData[i - 1].UndoAddr;
			UndoData[i].UndoLeft = UndoData[i - 1].UndoLeft;
		}

		UndoData[0].UndoAddr = FilePos;
		UndoData[0].UndoLeft = LeftPos;
	}

	if (Key != KEY_ALTBS && Key != KEY_CTRLZ && Key != KEY_NONE && Key != KEY_IDLE)
		LastKeyUndo = FALSE;

	if (Key >= KEY_CTRL0 && Key <= KEY_CTRL9) {
		int Pos = Key - KEY_CTRL0;

		if (BMSavePos.SavePosAddr[Pos] != POS_NONE) {
			FilePos = BMSavePos.SavePosAddr[Pos];
			LeftPos = BMSavePos.SavePosLeft[Pos];
			//			LastSelPos=FilePos;
			Show();
		}

		return TRUE;
	}

	if (Key >= KEY_CTRLSHIFT0 && Key <= KEY_CTRLSHIFT9)
		Key = Key - KEY_CTRLSHIFT0 + KEY_RCTRL0;

	if (Key >= KEY_RCTRL0 && Key <= KEY_RCTRL9) {
		int Pos = Key - KEY_RCTRL0;
		BMSavePos.SavePosAddr[Pos] = FilePos;
		BMSavePos.SavePosLeft[Pos] = LeftPos;
		return TRUE;
	}

	switch (Key) {
		case KEY_F1: {
			Help::Present(L"Viewer");
			return TRUE;
		}
		case KEY_CTRLU: {
			//		if (SelectSize)
			{
				SelectSize = 0;
				Show();
			}
			return TRUE;
		}
		case KEY_CTRLC:
		case KEY_CTRLINS:
		case KEY_CTRLNUMPAD0: {
			if (SelectSize && ViewFile.Opened()) {
				wchar_t *SelData;
				size_t DataSize = (size_t)SelectSize;	// + (IsFullWideCodePage(VM.CodePage) ? sizeof(wchar_t) : 1);
				switch (VM.CodePage) {
					case CP_UTF32LE:
					case CP_UTF32BE:
						DataSize+= 4;
						break;
					case CP_UTF16LE:
					case CP_UTF16BE:
						DataSize+= 2;
						break;
					default:
						DataSize++;
				}
				int64_t CurFilePos = vtell();

				if ((SelData = (wchar_t *)malloc(DataSize * sizeof(wchar_t)))) {
					wmemset(SelData, 0, DataSize);
					vseek(SelectPos, SEEK_SET);
					vread(SelData, (int)SelectSize);
					CopyToClipboard(SelData);
					free(SelData);
					vseek(CurFilePos, SEEK_SET);
				}
			}

			return TRUE;
		}
		// включить/выключить скролбар
		case KEY_CTRLS: {
			ViOpt.ShowScrollbar = !ViOpt.ShowScrollbar;
			Opt.ViOpt.ShowScrollbar = ViOpt.ShowScrollbar;

			if (m_bQuickView)
				CtrlObject->Cp()->ActivePanel->Redraw();

			Show();
			return (TRUE);
		}
		case KEY_IDLE: {
			if (ViewFile.Opened()) {
				// TODO: strFullFileName -> if (DriveType!=DRIVE_REMOVABLE && !IsDriveTypeCDROM(DriveType))
				{
					FAR_FIND_DATA_EX NewViewFindData;

					if (!apiGetFindDataForExactPathName(strFullFileName, NewViewFindData))
						return TRUE;

					ViewFile.ActualizeFileSize();
					vseek(0, SEEK_END);
					int64_t CurFileSize = vtell();

					if (ViewFindData.ftLastWriteTime.dwLowDateTime
									!= NewViewFindData.ftLastWriteTime.dwLowDateTime
							|| ViewFindData.ftLastWriteTime.dwHighDateTime
									!= NewViewFindData.ftLastWriteTime.dwHighDateTime
							|| CurFileSize != FileSize) {
						ViewFindData = NewViewFindData;
						FileSize = CurFileSize;

						if (FilePos > FileSize)
							ProcessKey(KEY_CTRLEND);
						else {
							int64_t PrevLastPage = LastPage;
							Show();

							if (PrevLastPage && !LastPage) {
								ProcessKey(KEY_CTRLEND);
								LastPage = TRUE;
							}
						}
					}
				}
			}

			if (Opt.ViewerEditorClock && HostFileViewer && HostFileViewer->IsFullScreen()
					&& Opt.ViOpt.ShowTitleBar)
				ShowTime(FALSE);

			return TRUE;
		}
		case KEY_ALTBS:
		case KEY_CTRLZ: {
			for (size_t I = 1; I < ARRAYSIZE(UndoData); I++) {
				UndoData[I - 1].UndoAddr = UndoData[I].UndoAddr;
				UndoData[I - 1].UndoLeft = UndoData[I].UndoLeft;
			}

			if (UndoData[0].UndoAddr != -1) {
				FilePos = UndoData[0].UndoAddr;
				LeftPos = UndoData[0].UndoLeft;
				UndoData[ARRAYSIZE(UndoData) - 1].UndoAddr = -1;
				UndoData[ARRAYSIZE(UndoData) - 1].UndoLeft = -1;
				Show();
				//				LastSelPos=FilePos;
			}

			return TRUE;
		}
		case KEY_ADD:
		case KEY_SUBTRACT: {
			if (!FHP->IsTemporary())	// if viewing observed (typically temporary) file - dont allow to switch to another file
			{
				FARString strName;
				bool NextFileFound;

				if (Key == KEY_ADD)
					NextFileFound = ViewNamesList.GetNextName(strName);
				else
					NextFileFound = ViewNamesList.GetPrevName(strName);

				if (NextFileFound) {
					if (Opt.ViOpt.SavePos) {
						FARString strCacheName = ComposeCacheName();
						UINT CodePage = 0;

						if (CodePageChangedByUser)
							CodePage = VM.CodePage;

						{
							PosCache poscache = {};
							poscache.Param[0] = FilePos;
							poscache.Param[1] = LeftPos;
							poscache.Param[2] = VM.Hex;
							//=poscache.Param[3];
							poscache.Param[4] = CodePage;

							if (Opt.ViOpt.SaveShortPos) {
								poscache.Position[0] = BMSavePos.SavePosAddr;
								poscache.Position[1] = BMSavePos.SavePosLeft;
								// poscache.Position[2]=;
								// poscache.Position[3]=;
							}

							CtrlObject->ViewerPosCache->AddPosition(strCacheName, poscache);
							memset(&BMSavePos, 0xff, sizeof(BMSavePos));	//??!!??
						}
					}

					if (PointToName(strName) == strName) {
						FARString strViewDir;
						ViewNamesList.GetCurDir(strViewDir);

						if (!strViewDir.IsEmpty())
							FarChDir(strViewDir);
					}

					if (OpenFile(std::make_shared<FileHolder>(strName), TRUE)) {
						SecondPos = 0;
						Show();
					}

					ShowConsoleTitle();
				}
			}

			return TRUE;
		}
		case KEY_SHIFTF2: {
			ProcessTypeWrapMode(!VM.WordWrap);
			return TRUE;
		}
		case KEY_F2: {
			ProcessWrapMode(!VM.Wrap);
			return TRUE;
		}
		case KEY_F4: {
			ProcessHexMode(!VM.Hex);
			return TRUE;
		}
		case KEY_F5: {
			SavePosCache();
			VM.Processed = !VM.Processed;
			ChangeViewKeyBar();
			if (VM.Processed || !strProcessedViewName.IsEmpty()) {
				DefCodePage = VM.CodePage;
				OpenFile(FHP, TRUE);
			}
			Show();
			return true;
		}
		case KEY_F7: {
			Search(0, 0);
			return TRUE;
		}
		case KEY_SHIFTF7:
		case KEY_SPACE: {
			Search(1, 0);
			return TRUE;
		}
		case KEY_ALTF7: {
			SearchFlags.Set(REVERSE_SEARCH);
			Search(1, 0);
			SearchFlags.Clear(REVERSE_SEARCH);
			return TRUE;
		}
		case KEY_F8: {
			switch (VM.CodePage) {
				case CP_UTF32LE:
				case CP_UTF32BE:
					FilePos*= 4;
					SetFileSize();
					SelectPos = 0;
					SelectSize = 0;
					break;
				case CP_UTF16LE:
				case CP_UTF16BE:
					FilePos*= 2;
					SetFileSize();
					SelectPos = 0;
					SelectSize = 0;
					break;
			}

			//VM.CodePage = VM.CodePage == WINPORT(GetOEMCP)() ? WINPORT(GetACP)() : WINPORT(GetOEMCP)();
			if (VM.CodePage == CP_UTF8)
				VM.CodePage = WINPORT(GetACP)();
			else if (VM.CodePage == WINPORT(GetACP)() )
				VM.CodePage = WINPORT(GetOEMCP)();
			else // if (VM.CodePage == WINPORT(GetOEMCP)() )
				VM.CodePage = VM.Hex ? WINPORT(GetACP)() : CP_UTF8; // STUB - для hex UTF8/UTF32 сейчас не работает

			ChangeViewKeyBar();
			Show();
			//			LastSelPos=FilePos;
			CodePageChangedByUser = TRUE;
			return TRUE;
		}
		case KEY_SHIFTF8: {
			UINT nCodePage = SelectCodePage(VM.CodePage, true, true, false, true);
			if (nCodePage == CP_AUTODETECT) {
				if (!GetFileFormat2(FHP->GetPathName(), nCodePage, nullptr, true, true))
					return TRUE;
			}

			// BUGBUG
			// пока что запретим переключать hex в UTF8/UTF32, ибо не работает.
			if (VM.Hex && (nCodePage == CP_UTF8 || nCodePage == CP_UTF32LE || nCodePage == CP_UTF32BE)) {
				return TRUE;
			}

			if (nCodePage != (UINT)-1) {
				CodePageChangedByUser = TRUE;

				if (IsFullWideCodePage(VM.CodePage) && !IsFullWideCodePage(nCodePage)) {
					FilePos*= sizeof(wchar_t);
					SelectPos = 0;
					SelectSize = 0;
					SetCRSym();
				} else if (!IsFullWideCodePage(VM.CodePage) && IsFullWideCodePage(nCodePage)) {
					FilePos = (FilePos + (FilePos & 3)) / 4;	//????
					SelectPos = 0;
					SelectSize = 0;
					SetCRSym();
				}

				VM.CodePage = nCodePage;
				SetFileSize();
				ChangeViewKeyBar();
				Show();
				//				LastSelPos=FilePos;
			}

			return TRUE;
		}
		case KEY_ALTF8: {
			if (ViewFile.Opened())
				GoTo();

			return TRUE;
		}
		case KEY_F11: {
			CtrlObject->Plugins.CommandsMenu(MODALTYPE_VIEWER, 0, L"Viewer");
			Show();
			return TRUE;
		}
		/*
			$ 27.06.2001 VVM
			+ С альтом скролим по 1
		*/
		case KEY_MSWHEEL_UP:
		case (KEY_MSWHEEL_UP | KEY_ALT): {
			int Roll = Key & KEY_ALT ? 1 : Opt.MsWheelDeltaView;

			for (int i = 0; i < Roll; i++)
				ProcessKey(KEY_UP);

			return TRUE;
		}
		case KEY_MSWHEEL_DOWN:
		case (KEY_MSWHEEL_DOWN | KEY_ALT): {
			int Roll = Key & KEY_ALT ? 1 : Opt.MsWheelDeltaView;

			for (int i = 0; i < Roll; i++)
				ProcessKey(KEY_DOWN);

			return TRUE;
		}
		case KEY_MSWHEEL_LEFT:
		case (KEY_MSWHEEL_LEFT | KEY_ALT): {
			int Roll = Key & KEY_ALT ? 1 : Opt.MsHWheelDeltaView;

			for (int i = 0; i < Roll; i++)
				ProcessKey(KEY_LEFT);

			return TRUE;
		}
		case KEY_MSWHEEL_RIGHT:
		case (KEY_MSWHEEL_RIGHT | KEY_ALT): {
			int Roll = Key & KEY_ALT ? 1 : Opt.MsHWheelDeltaView;

			for (int i = 0; i < Roll; i++)
				ProcessKey(KEY_RIGHT);

			return TRUE;
		}
		case KEY_UP:
		case KEY_NUMPAD8:
		case KEY_SHIFTNUMPAD8: {
			if (FilePos > 0 && ViewFile.Opened()) {
				Up();

				if (VM.Hex) {
					size_t len = 0x8;
					switch (VM.CodePage) {
						case CP_UTF32LE:
						case CP_UTF32BE:
							len*= 4;
							break;
						case CP_UTF16LE:
						case CP_UTF16BE:
							len*= 2;
							break;
					}
					FilePos&= ~(len - 1);
					Show();
				} else
					ShowPage(SHOW_UP);
			}

			//			LastSelPos=FilePos;
			return TRUE;
		}
		case KEY_DOWN:
		case KEY_NUMPAD2:
		case KEY_SHIFTNUMPAD2: {
			if (!LastPage && ViewFile.Opened()) {
				if (VM.Hex) {
					FilePos = SecondPos;
					Show();
				} else
					ShowPage(SHOW_DOWN);
			}

			//			LastSelPos=FilePos;
			return TRUE;
		}

		case KEY_ALTPGUP:
		case KEY_PGUP:
		case KEY_NUMPAD9:
		case KEY_SHIFTNUMPAD9:
		case KEY_CTRLUP: {
			if (ViewFile.Opened()) {
				const auto InitialFilePos = FilePos;
				for (unsigned boost = 0; boost <= iBoostPg; boost+= 4) {
					for (int i = Y1; i < Y2; i++)
						Up();

					if ((FilePos - InitialFilePos) * 256 >= FileSize) {		// limit speed boost by FileSize/256 per keypress
																			//						fprintf(stderr, "iBoostPg limited at %u\n", boost);
						iBoostPg = boost;
						break;
					}
				}

				Show();
				//				LastSelPos=FilePos;
			}

			return TRUE;
		}

		case KEY_ALTPGDN:
		case KEY_PGDN:
		case KEY_NUMPAD3:
		case KEY_SHIFTNUMPAD3:
		case KEY_CTRLDOWN: {
			ViewerString vString;
			const auto InitialFilePos = FilePos;
			for (unsigned boost = 0; boost <= iBoostPg; boost+= 4) {
				if (LastPage || !ViewFile.Opened()) {
					return TRUE;
				}

				vseek(FilePos, SEEK_SET);

				for (int i = Y1; i < Y2; i++) {
					ReadString(vString, -1, MAX_VIEWLINE);

					if (LastPage) {
						return TRUE;
					}
				}

				FilePos = vtell();

				for (int i = Y1; i <= Y2; i++)
					ReadString(vString, -1, MAX_VIEWLINE);

				/*
					$ 02.06.2003 VVM
					+ Старое поведение оставим на Ctrl-Down
				*/

				/*
					$ 21.05.2003 VVM
					+ По PgDn листаем всегда по одной странице,
					даже если осталась всего одна строчка.
					Удобно тексты читать
				*/
				if (LastPage && Key == KEY_CTRLDOWN) {
					InternalKey++;
					ProcessKey(KEY_CTRLPGDN);
					InternalKey--;
					return TRUE;
				}

				if ((FilePos - InitialFilePos) * 256 >= FileSize) {		// limit speed boost by FileSize/256 per keypress
																		//					fprintf(stderr, "iBoostPg limited at %u\n", boost);
					iBoostPg = boost;
					break;
				}
			}
			Show();

			//			LastSelPos=FilePos;
			return TRUE;
		}
		case KEY_LEFT:
		case KEY_NUMPAD4:
		case KEY_SHIFTNUMPAD4: {
			if (LeftPos > 0 && ViewFile.Opened()) {
				if (VM.Hex && LeftPos > 80 - Width)
					LeftPos = Max(80 - Width, 1);

				LeftPos--;
				Show();
			}

			//			LastSelPos=FilePos;
			return TRUE;
		}
		case KEY_RIGHT:
		case KEY_NUMPAD6:
		case KEY_SHIFTNUMPAD6: {
			if (LeftPos < MAX_VIEWLINE && ViewFile.Opened() && !VM.Hex && !VM.Wrap) {
				LeftPos++;
				Show();
			}

			//			LastSelPos=FilePos;
			return TRUE;
		}
		case KEY_CTRLLEFT:
		case KEY_CTRLNUMPAD4: {
			if (ViewFile.Opened()) {
				if (VM.Hex) {
					FilePos--;

					if (FilePos < 0)
						FilePos = 0;
				} else {
					LeftPos-= 20;

					if (LeftPos < 0)
						LeftPos = 0;
				}

				Show();
				//				LastSelPos=FilePos;
			}

			return TRUE;
		}
		case KEY_CTRLRIGHT:
		case KEY_CTRLNUMPAD6: {
			if (ViewFile.Opened()) {
				if (VM.Hex) {
					FilePos++;

					if (FilePos >= FileSize)
						FilePos = FileSize - 1;		//??
				} else if (!VM.Wrap) {
					LeftPos+= 20;

					if (LeftPos > MAX_VIEWLINE)
						LeftPos = MAX_VIEWLINE;
				}

				Show();
				//				LastSelPos=FilePos;
			}

			return TRUE;
		}
		case KEY_CTRLSHIFTLEFT:
		case KEY_CTRLSHIFTNUMPAD4:

			// Перейти на начало строк
			if (ViewFile.Opened()) {
				LeftPos = 0;
				Show();
			}

			return TRUE;
		case KEY_CTRLSHIFTRIGHT:
		case KEY_CTRLSHIFTNUMPAD6: {
			// Перейти на конец строк
			if (ViewFile.Opened()) {
				int I, Y, Len, MaxLen = 0;

				for (I = 0, Y = Y1; Y <= Y2; Y++, I++) {
					Len = StrLength(Strings[I].Chars());

					if (Len > MaxLen)
						MaxLen = Len;
				}	/* for */

				if (MaxLen > Width)
					LeftPos = MaxLen - Width;
				else
					LeftPos = 0;

				Show();
			}	/* if */

			return TRUE;
		}
		case KEY_CTRLHOME:
		case KEY_CTRLNUMPAD7:
		case KEY_HOME:
		case KEY_NUMPAD7:
		case KEY_SHIFTNUMPAD7:

			// Перейти на начало файла
			if (ViewFile.Opened())
				LeftPos = 0;

		case KEY_CTRLPGUP:
		case KEY_CTRLNUMPAD9:

			if (ViewFile.Opened()) {
				FilePos = 0;
				Show();
				//				LastSelPos=FilePos;
			}

			return TRUE;
		case KEY_CTRLEND:
		case KEY_CTRLNUMPAD1:
		case KEY_END:
		case KEY_NUMPAD1:
		case KEY_SHIFTNUMPAD1:

			// Перейти на конец файла
			if (ViewFile.Opened())
				LeftPos = 0;

		case KEY_CTRLPGDN:
		case KEY_CTRLNUMPAD3:

			if (ViewFile.Opened()) {
				/*
					$ 15.08.2002 IS
					Для обычного режима, если последняя строка не содержит перевод
					строки, крутанем вверх на один раз больше - иначе визуально
					обработка End (и подобных) на такой строке отличается от обработки
					Down.
				*/
				unsigned int max_counter = Y2 - Y1;

				if (VM.Hex)
					vseek(0, SEEK_END);
				else {
					vseek(-1, SEEK_END);
					WCHAR LastSym = 0;

					if (vgetc(LastSym) && LastSym != CRSym)
						++max_counter;
				}

				FilePos = vtell();

				/*
					{
						char Buf[100];
						sprintf(Buf,"%llX",FilePos);
						Message(0,1,"End",Buf,"Ok");
					}
				*/
				for (int i = 0; static_cast<unsigned int>(i) < max_counter; i++)
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
						case CP_UTF32LE:
						case CP_UTF32BE:
							len*= 4;
							break;
						case CP_UTF16LE:
						case CP_UTF16BE:
							len*= 2;
							break;
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
				//				LastSelPos=FilePos;
			}

			return TRUE;
		default:

			if (Key >= L' ' && WCHAR_IS_VALID(Key)) {
				Search(0, Key);
				return TRUE;
			}
	}

	return FALSE;
}

int Viewer::ProcessMouse(MOUSE_EVENT_RECORD *MouseEvent)
{
	if (!(MouseEvent->dwButtonState & 3))
		return FALSE;

	// Shift + Mouse click -> adhoc quick edit
	if ((MouseEvent->dwControlKeyState & SHIFT_PRESSED) != 0 && (MouseEvent->dwEventFlags & MOUSE_MOVED) == 0
			&& (MouseEvent->dwButtonState & FROM_LEFT_1ST_BUTTON_PRESSED) != 0) {
		WINPORT(BeginConsoleAdhocQuickEdit)();
		return TRUE;
	}

	/*
		$ 22.01.2001 IS
		Происходят какие-то манипуляции -> снимем выделение
	*/
	//	SelectSize=0;

	/*
		$ 10.09.2000 SVS
		! Постоянный скроллинг при нажатой клавише
		Обыкновенный захват мыши
	*/
	/*
		$ 02.10.2000 SVS
		> Если нажать в самом низу скролбара, вьюер отмотается на страницу
		> ниже нижней границы текста. Перед глазами будет пустой экран.
	*/
	if (ViOpt.ShowScrollbar && MouseX == X2 + (m_bQuickView ? 1 : 0)) {
		/*
			$ 01.09.2000 SVS
			Небольшая бага с тыканием в верхнюю позицию ScrollBar`а
		*/
		if (MouseY == Y1)
			while (IsMouseButtonPressed())
				ProcessKey(KEY_UP);
		else if (MouseY == Y2) {
			while (IsMouseButtonPressed()) {
				//				_SVS(SysLog(L"Viewer/ KEY_DOWN= %i, %i",FilePos,FileSize));
				ProcessKey(KEY_DOWN);
			}
		} else if (MouseY == Y1 + 1)
			ProcessKey(KEY_CTRLHOME);
		else if (MouseY == Y2 - 1)
			ProcessKey(KEY_CTRLEND);
		else {
			while (IsMouseButtonPressed()) {
				/*
					$ 14.05.2001 DJ
					более точное позиционирование; корректная работа на больших файлах
				*/
				FilePos = (FileSize - 1) / (Y2 - Y1 - 1) * (MouseY - Y1);
				int Perc;

				if (FilePos > FileSize) {
					FilePos = FileSize;
					Perc = 100;
				} else if (FilePos < 0) {
					FilePos = 0;
					Perc = 0;
				} else
					Perc = ToPercent64(FilePos, FileSize);

				//_SVS(SysLog(L"Viewer/ ToPercent()=%i, %lld, %lld, Mouse=[%d:%d]",Perc,FilePos,FileSize,MsX,MsY));
				if (Perc == 100)
					ProcessKey(KEY_CTRLEND);
				else if (!Perc)
					ProcessKey(KEY_CTRLHOME);
				else {
					/*
						$ 27.04.2001 DJ
						не рвем строки посередине
					*/
					AdjustFilePos();
					Show();
				}
			}
		}

		return (TRUE);
	}

	/*
		$ 16.12.2000 tran
		шелчок мышью на статус баре
	*/

	/*
		$ 12.10.2001 SKV
		угу, а только если он нсть, statusline...
	*/
	if (MouseY == (Y1 - 1) && (HostFileViewer && HostFileViewer->IsTitleBarVisible()))		// Status line
	{
		int XCodePage, XPos, NameLength;
		NameLength = ObjWidth - 40;

		if (Opt.ViewerEditorClock && HostFileViewer && HostFileViewer->IsFullScreen())
			NameLength-= 6;

		if (NameLength < 20)
			NameLength = 20;

		XCodePage = NameLength + 1;
		XPos = NameLength + 1 + 10 + 1 + 10 + 1;

		while (IsMouseButtonPressed())
			;

		if (MouseY != Y1 - 1)
			return TRUE;

		//_D(SysLog(L"MsX=%i, XTable=%i, XPos=%i",MsX,XTable,XPos));
		if (MouseX >= XCodePage && MouseX <= XCodePage + 10) {
			ProcessKey(KEY_SHIFTF8);
			return (TRUE);
		}

		if (MouseX >= XPos && MouseX <= XPos + 7 + 1 + 4 + 1 + 3) {
			ProcessKey(KEY_ALTF8);
			return (TRUE);
		}
	}

	if (MouseX < X1 || MouseX > X2 || MouseY < Y1 || MouseY > Y2)
		return FALSE;

	if (MouseX < X1 + 7)
		while (IsMouseButtonPressed() && MouseX < X1 + 7)
			ProcessKey(KEY_LEFT);
	else if (MouseX > X2 - 7)
		while (IsMouseButtonPressed() && MouseX > X2 - 7)
			ProcessKey(KEY_RIGHT);
	else if (MouseY < Y1 + (Y2 - Y1) / 2)
		while (IsMouseButtonPressed() && MouseY < Y1 + (Y2 - Y1) / 2)
			ProcessKey(KEY_UP);
	else
		while (IsMouseButtonPressed() && MouseY >= Y1 + (Y2 - Y1) / 2)
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
	int BufSize, I;

	if (FilePos > ((int64_t)(sizeof(Buf) / sizeof(wchar_t))) - 1)
		BufSize = (sizeof(Buf) / sizeof(wchar_t)) - 1;
	else if (FilePos != 0)
		BufSize = (int)FilePos;
	else
		return;

	LastPage = 0;

	if (VM.Hex) {
		// Alter-1: here we use BYTE COUNT, while in Down handler we use ::vread which may
		// accept either CHARACTER COUNT or w_char count.
		// int UpSize=IsFullWideCodePage(VM.CodePage) ? 8 : 8 * sizeof(wchar_t);
		int UpSize = 16;	// always have 16 bytes per row

		if (FilePos < (int64_t)UpSize)
			FilePos = 0;
		else
			FilePosShiftLeft(UpSize);

		return;
	}

	vseek(FilePos - (int64_t)BufSize, SEEK_SET);
	I = BufSize = vread(Buf, BufSize, true);
	if (I == -1) {
		return;
	}

	wchar_t CRSymEncoded = (unsigned int)CRSym;
	wchar_t CRRSymEncoded = (unsigned int)'\r';
	switch (VM.CodePage) {
		case CP_UTF32BE:
			CRSymEncoded<<= 24;
			CRRSymEncoded<<= 24;
			break;
		case CP_UTF16BE:
			CRSymEncoded<<= 8;
			CRRSymEncoded<<= 8;
			break;
	}

	if (I > 0 && Buf[I - 1] == CRSymEncoded) {
		--I;
	}

	if (I > 0 && CRSym == L'\n' && Buf[I - 1] == CRRSymEncoded) {
		--I;
	}

	while (I > 0 && Buf[I - 1] != CRSymEncoded) {
		--I;
	}

	int64_t WholeLineLength = (BufSize - I);	// in code units

	if (VM.Wrap && Width > 0) {
		vseek(FilePos - WholeLineLength, SEEK_SET);
		int WrapBufSize = vread(Buf, WholeLineLength, false);

		// we might read more code units and could actually overflow current position
		// so try to find out exact substring that matches into line start and current position
		Buf[WrapBufSize] = 0;
		//		fprintf(stderr, "WrapBufSize=%d WholeLineLength=%d LINE='%ls'\n", WrapBufSize, WholeLineLength, &Buf[0]);
		//		fprintf(stderr, "LINE1: '%ls'\n", &Buf[0]);
		while (WrapBufSize) {
			int distance = CalcCodeUnitsDistance(VM.CodePage, &Buf[0], &Buf[WrapBufSize]);
			if (distance <= WholeLineLength) {
				while (WrapBufSize
						&& distance == CalcCodeUnitsDistance(VM.CodePage, &Buf[0], &Buf[WrapBufSize - 1])) {
					--WrapBufSize;
				}
				break;
			}
			--WrapBufSize;
		}
		Buf[WrapBufSize] = 0;
		//		fprintf(stderr, "Matching LINE: '%ls'\n", &Buf[0]);

		if (VM.WordWrap) {
			//	khffgkjkfdg dfkghd jgfhklf |
			//	sdflksj lfjghf fglh lf     |
			//	dfdffgljh ldgfhj           |
			bool LeadingSpaces = WrapBufSize > 0 && IsSpace(Buf[0]);
			for (I = 0; I < WrapBufSize;) {
				if (!IsSpace(Buf[I])) {
					int CurLineStart = LeadingSpaces ? 0 : I; // Keep spaces at beginning of wrapped line: #2246
					for (int LastFitEnd = CurLineStart + 1;; ++I) {
						if (I == WrapBufSize) {
							int distance =
									CalcCodeUnitsDistance(VM.CodePage, &Buf[CurLineStart], &Buf[WrapBufSize]);
							FilePosShiftLeft((distance > 0) ? distance : 1);
							return;
						}

						if (!IsSpace(Buf[I]) && (I + 1 == WrapBufSize || IsSpace(Buf[I + 1]))) {
							if (CalcStrSize(&Buf[CurLineStart], I + 1 - CurLineStart) > Width) {
								I = LastFitEnd;
								break;
							}
							LastFitEnd = I + 1;
						}
					}
					LeadingSpaces = false;

				} else {
					++I;
				}
			}
		}

		for (int PrevSublineLength = 0, CurLineStart = I = 0;; ++I) {
			if (I == WrapBufSize) {
				int distance = CalcCodeUnitsDistance(VM.CodePage, &Buf[CurLineStart], &Buf[WrapBufSize]);
				FilePosShiftLeft((distance > 0) ? distance : 1);
				return;
			}

			int SublineLength = CalcStrSize(&Buf[CurLineStart], I - CurLineStart);
			if (SublineLength > PrevSublineLength) {
				if (SublineLength >= Width) {
					CurLineStart = I;
					PrevSublineLength = 0;
				} else {
					PrevSublineLength = SublineLength;
				}
			}
		}
	}

	//	fprintf(stderr, "!!!!!!!!!!!!NOWRAP!!!!!!!!!!!!\n");
	FilePosShiftLeft((WholeLineLength > 0) ? WholeLineLength : 1);
}

int Viewer::CalcStrSize(const wchar_t *Str, int Length)
{
	int Size, I;

	for (Size = 0, I = 0; I < Length; I++)
		switch (Str[I]) {
			case L'\t':
				Size+= ViOpt.TabSize - (Size % ViOpt.TabSize);
				break;
			case L'\n':
			case L'\r':
				break;
			default:
				Size++;
				break;
		}

	return (Size);
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
	Viewer::ViewKeyBar = ViewKeyBar;
	ChangeViewKeyBar();
}

void Viewer::ChangeViewKeyBar()
{
	if (ViewKeyBar) {
		/*
			$ 12.07.2000 SVS
			Wrap имеет 3 позиции
		*/
		/*
			$ 15.07.2000 SVS
			Wrap должен показываться следующий, а не текущий
		*/
		ViewKeyBar->Change(VM.Wrap ? Msg::ViewF2Unwrap : (VM.WordWrap ? Msg::ViewShiftF2 : Msg::ViewF2), 1);
		ViewKeyBar->Change(KBL_SHIFT, VM.WordWrap ? Msg::ViewF2 : Msg::ViewShiftF2, 1);

		if (VM.Hex)
			ViewKeyBar->Change(Msg::ViewF4Text, 3);
		else
			ViewKeyBar->Change(Msg::ViewF4, 3);

		if (VM.CodePage == CP_UTF8)
			ViewKeyBar->Change(Msg::ViewF8, 7);
		else if (VM.CodePage == WINPORT(GetACP)())
			ViewKeyBar->Change(Msg::ViewF8DOS, 7);
		else
			ViewKeyBar->Change(VM.Hex ? Msg::ViewF8 : Msg::ViewF8UTF8, 7); // STUB - для hex UTF8/UTF32 сейчас не работает

		if (VM.Processed)
			ViewKeyBar->Change(Msg::ViewF5Raw, 4);
		else
			ViewKeyBar->Change(Msg::ViewF5Processed, 4);

		ViewKeyBar->Redraw();
	}

	//	ViewerMode vm=VM;
	CtrlObject->Plugins.CurViewer = this;	// HostFileViewer;
	//	CtrlObject->Plugins.ProcessViewerEvent(VE_MODE,&vm);
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

LONG_PTR WINAPI ViewerSearchDlgProc(HANDLE hDlg, int Msg, int Param1, LONG_PTR Param2)
{
	switch (Msg) {
		case DN_INITDIALOG: {
			SendDlgMessage(hDlg, DM_SDSETVISIBILITY,
					SendDlgMessage(hDlg, DM_GETCHECK, SD_RADIO_HEX, 0) == BSTATE_CHECKED, 0);
			SendDlgMessage(hDlg, DM_EDITUNCHANGEDFLAG, SD_EDIT_TEXT, 1);
			SendDlgMessage(hDlg, DM_EDITUNCHANGEDFLAG, SD_EDIT_HEX, 1);
			return TRUE;
		}
		case DM_SDSETVISIBILITY: {
			SendDlgMessage(hDlg, DM_SHOWITEM, SD_EDIT_TEXT, !Param1);
			SendDlgMessage(hDlg, DM_SHOWITEM, SD_EDIT_HEX, Param1);
			SendDlgMessage(hDlg, DM_ENABLE, SD_CHECKBOX_CASE, !Param1);
			SendDlgMessage(hDlg, DM_ENABLE, SD_CHECKBOX_WORDS, !Param1);
			// SendDlgMessage(hDlg,DM_ENABLE,SD_CHECKBOX_REGEXP,!Param1);
			return TRUE;
		}
		case DN_BTNCLICK: {
			if ((Param1 == SD_RADIO_TEXT || Param1 == SD_RADIO_HEX) && Param2) {
				SendDlgMessage(hDlg, DM_ENABLEREDRAW, FALSE, 0);
				bool Hex = (Param1 == SD_RADIO_HEX);
				FARString strDataStr;
				Transform(strDataStr,
						(const wchar_t *)SendDlgMessage(hDlg, DM_GETCONSTTEXTPTR,
								Hex ? SD_EDIT_TEXT : SD_EDIT_HEX, 0),
						Hex ? L'X' : L'S');
				SendDlgMessage(hDlg, DM_SETTEXTPTR, Hex ? SD_EDIT_HEX : SD_EDIT_TEXT,
						(LONG_PTR)strDataStr.CPtr());
				SendDlgMessage(hDlg, DM_SDSETVISIBILITY, Hex, 0);

				if (!strDataStr.IsEmpty()) {
					SendDlgMessage(hDlg, DM_EDITUNCHANGEDFLAG, Hex ? SD_EDIT_HEX : SD_EDIT_TEXT,
							SendDlgMessage(hDlg, DM_EDITUNCHANGEDFLAG, Hex ? SD_EDIT_TEXT : SD_EDIT_HEX, -1));
				}

				SendDlgMessage(hDlg, DM_ENABLEREDRAW, TRUE, 0);
				return TRUE;
			}
		}
		case DN_HOTKEY: {
			if (Param1 == SD_TEXT_SEARCH) {
				SendDlgMessage(hDlg, DM_SETFOCUS,
						(SendDlgMessage(hDlg, DM_GETCHECK, SD_RADIO_HEX, 0) == BSTATE_CHECKED)
								? SD_EDIT_HEX
								: SD_EDIT_TEXT,
						0);
				return FALSE;
			}
		}
	}

	return DefDlgProc(hDlg, Msg, Param1, Param2);
}

static void PR_ViewerSearchMsg()
{
	PreRedrawItem preRedrawItem = PreRedraw.Peek();
	ViewerSearchMsg((const wchar_t *)preRedrawItem.Param.Param1, (int)(INT_PTR)preRedrawItem.Param.Param2);
}

void ViewerSearchMsg(const wchar_t *MsgStr, int Percent)
{
	FARString strProgress;

	if (Percent >= 0) {
		FormatString strPercent;
		strPercent << Percent;

		size_t PercentLength = Max(strPercent.strValue().GetLength(), (size_t)3);
		size_t Length =
				Max(Min(static_cast<int>(MAX_WIDTH_MESSAGE - 2), StrLength(MsgStr)), 40) - PercentLength - 2;
		size_t CurPos = Min(Percent, 100) * Length / 100;
		strProgress.Reserve(Length);
		strProgress.Append(BoxSymbols[BS_X_DB], CurPos);
		strProgress.Append(BoxSymbols[BS_X_B0], Length - CurPos);
		FormatString strTmp;
		strTmp << L" " << fmt::Expand(PercentLength) << strPercent << L"%";
		strProgress+= strTmp;
	}

	Message(0, 0, Msg::ViewSearchTitle, (SearchHex ? Msg::ViewSearchingHex : Msg::ViewSearchingFor), MsgStr,
			strProgress.IsEmpty() ? nullptr : strProgress.CPtr());
	PreRedrawItem preRedrawItem = PreRedraw.Peek();
	preRedrawItem.Param.Param1 = (void *)MsgStr;
	preRedrawItem.Param.Param2 = (LPVOID)(INT_PTR)Percent;
	PreRedraw.SetParam(preRedrawItem.Param);
}

/*
	$ 27.01.2003 VVM
	+ Параметр Next может принимать значения:
	0 - Новый поиск
	1 - Продолжить поиск со следующей позиции
	2 - Продолжить поиск с начала файла
*/

static inline bool CheckBufMatchesCaseInsensitive(size_t MatchLen, const wchar_t *Buf,
		const wchar_t *MatchUpperCase, const wchar_t *MatchLowerCase)
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

void Viewer::Search(int Next, int FirstChar)
{
	const wchar_t *TextHistoryName = L"SearchText";
	const wchar_t *HexMask = L"HH HH HH HH HH HH HH HH HH HH HH HH HH HH HH HH HH HH HH HH HH HH ";
	DialogDataEx SearchDlgData[] = {
		{DI_DOUBLEBOX,   3,  1,  72, 11, {0}, 0, Msg::ViewSearchTitle},
		{DI_TEXT,        5,  2,  0,  2,  {0}, 0, Msg::ViewSearchFor},
		{DI_EDIT,        5,  3,  70, 3,  {(DWORD_PTR)TextHistoryName}, DIF_FOCUS | DIF_HISTORY | DIF_USELASTHISTORY, L""},
		{DI_FIXEDIT,     5,  3,  70, 3,  {(DWORD_PTR)HexMask}, DIF_MASKEDIT, L""},
		{DI_TEXT,        3,  4,  0,  4,  {0}, DIF_SEPARATOR, L""},
		{DI_RADIOBUTTON, 5,  5,  0,  5,  {1}, DIF_GROUP, Msg::ViewSearchForText},
		{DI_RADIOBUTTON, 5,  6,  0,  6,  {0}, 0, Msg::ViewSearchForHex},
		{DI_CHECKBOX,    40, 5,  0,  5,  {0}, 0, Msg::ViewSearchCase},
		{DI_CHECKBOX,    40, 6,  0,  6,  {0}, 0, Msg::ViewSearchWholeWords},
		{DI_CHECKBOX,    40, 7,  0,  7,  {0}, 0, Msg::ViewSearchReverse},
		{DI_CHECKBOX,    40, 8,  0,  8,  {0}, DIF_DISABLE, Msg::ViewSearchRegexp},
		{DI_TEXT,        3,  9,  0,  9,  {0}, DIF_SEPARATOR, L""},
		{DI_BUTTON,      0,  10, 0,  10, {0}, DIF_DEFAULT | DIF_CENTERGROUP, Msg::ViewSearchSearch},
		{DI_BUTTON,      0,  10, 0,  10, {0}, DIF_CENTERGROUP, Msg::ViewSearchCancel}
	};
	MakeDialogItemsEx(SearchDlgData, SearchDlg);
	FARString strSearchStr;
	FARString strMsgStr;
	int64_t MatchPos = 0;
	int Case, WholeWords, ReverseSearch, SearchRegexp;

	if (!ViewFile.Opened() || (Next && strLastSearchStr.IsEmpty()))
		return;

	if (!strLastSearchStr.IsEmpty())
		strSearchStr = strLastSearchStr;
	else
		strSearchStr.Clear();

	SearchDlg[SD_RADIO_TEXT].Selected = !LastSearchHex;
	SearchDlg[SD_RADIO_HEX].Selected = LastSearchHex;
	SearchDlg[SD_CHECKBOX_CASE].Selected = LastSearchCase;
	SearchDlg[SD_CHECKBOX_WORDS].Selected = LastSearchWholeWords;
	SearchDlg[SD_CHECKBOX_REVERSE].Selected = LastSearchReverse;
	SearchDlg[SD_CHECKBOX_REGEXP].Selected = LastSearchRegexp;

	if (SearchFlags.Check(REVERSE_SEARCH))
		SearchDlg[SD_CHECKBOX_REVERSE].Selected = !SearchDlg[SD_CHECKBOX_REVERSE].Selected;

	if (IsFullWideCodePage(VM.CodePage)) {
		SearchDlg[SD_RADIO_TEXT].Selected = TRUE;
		SearchDlg[SD_RADIO_HEX].Flags|= DIF_DISABLE;
		SearchDlg[SD_RADIO_HEX].Selected = FALSE;
	}

	if (SearchDlg[SD_RADIO_HEX].Selected)
		SearchDlg[SD_EDIT_HEX].strData = strSearchStr;
	else
		SearchDlg[SD_EDIT_TEXT].strData = strSearchStr;

	if (!Next) {
		SearchFlags.Flags = 0;
		Dialog Dlg(SearchDlg, ARRAYSIZE(SearchDlg), ViewerSearchDlgProc);
		Dlg.SetPosition(-1, -1, 76, 13);
		Dlg.SetHelp(L"ViewerSearch");

		if (FirstChar) {
			Dlg.InitDialog();
			Dlg.Show();
			Dlg.ProcessKey(FirstChar);
		}

		Dlg.Process();

		if (Dlg.GetExitCode() != SD_BUTTON_OK)
			return;
	}

	SearchHex = SearchDlg[SD_RADIO_HEX].Selected;
	Case = SearchDlg[SD_CHECKBOX_CASE].Selected;
	WholeWords = SearchDlg[SD_CHECKBOX_WORDS].Selected;
	ReverseSearch = SearchDlg[SD_CHECKBOX_REVERSE].Selected;
	SearchRegexp = SearchDlg[SD_CHECKBOX_REGEXP].Selected;

	if (SearchHex) {
		strSearchStr = SearchDlg[SD_EDIT_HEX].strData;
		RemoveTrailingSpaces(strSearchStr);
	} else {
		strSearchStr = SearchDlg[SD_EDIT_TEXT].strData;
	}

	strLastSearchStr = strSearchStr;
	LastSearchHex = SearchHex;
	LastSearchCase = Case;
	LastSearchWholeWords = WholeWords;

	if (!SearchFlags.Check(REVERSE_SEARCH))
		LastSearchReverse = ReverseSearch;

	LastSearchRegexp = SearchRegexp;

	int SearchWChars, SearchCodeUnits;
	bool Match = false;
	{
		TPreRedrawFuncGuard preRedrawFuncGuard(PR_ViewerSearchMsg);
		// SaveScreen SaveScr;
		SetCursorType(FALSE, 0);
		strMsgStr = strSearchStr;

		if (strMsgStr.GetLength() + 18 > static_cast<DWORD>(ObjWidth))
			TruncStrFromEnd(strMsgStr, ObjWidth - 18);

		InsertQuote(strMsgStr);

		if (SearchHex) {
			if (!strSearchStr.GetLength())
				return;

			Transform(strSearchStr, strSearchStr, L'S');
			WholeWords = 0;
		}

		SearchWChars = (int)strSearchStr.GetLength();
		if (!SearchWChars)
			return;

		SearchCodeUnits =
				CalcCodeUnitsDistance(VM.CodePage, strSearchStr.CPtr(), strSearchStr.CPtr() + SearchWChars);
		FARString strSearchStrLowerCase;

		if (!Case && !SearchHex) {
			strSearchStrLowerCase = strSearchStr;
			strSearchStr.Upper();
			strSearchStrLowerCase.Lower();
		}

		SelectSize = 0;

		if (Next) {
			if (Next == 2) {
				SearchFlags.Set(SEARCH_MODE2);
				LastSelPos = ReverseSearch ? FileSize : 0;
			}
#if 0
			else
			{
				LastSelPos = SelectPos + (ReverseSearch?-1:1);
			}
#endif
		} else {
			LastSelPos = FilePos;

			if (!LastSelPos || LastSelPos == FileSize) {
				SearchFlags.Set(SEARCH_MODE2);
				LastSelPos = ReverseSearch ? FileSize : 0;
			}
		}

		vseek(LastSelPos, SEEK_SET);
		Match = false;

		if (SearchWChars > 0 && (!ReverseSearch || LastSelPos >= 0)) {
			wchar_t Buf[16384];

			int ReadSize;
			wakeful W;
			INT64 StartPos = vtell();
			DWORD StartTime = WINPORT(GetTickCount)();

			while (!Match) {
				/*
					$ 01.08.2000 KM
					Изменена строка if (ReverseSearch && CurPos<0) на if (CurPos<0),
					так как при обычном прямом и LastSelPos=0xFFFFFFFF, поиск
					заканчивался так и не начавшись.
				*/
				// if (CurPos<0)
				//	CurPos=0;
				// vseek(CurPos,SEEK_SET);
				int BufSize = ARRAYSIZE(Buf);
				int64_t CurPos = vtell();
				if (ReverseSearch) {
					/*
						$ 01.08.2000 KM
						Изменёно вычисление CurPos с учётом Whole words
					*/
					CurPos-= ARRAYSIZE(Buf) - SearchCodeUnits - !!WholeWords;
					if (CurPos < 0) {
						BufSize+= (int)CurPos;
						CurPos = 0;
					}
					vseek(CurPos, SEEK_SET);
				}

				if ((ReadSize = vread(Buf, BufSize, SearchHex != 0)) <= 0)
					break;

				DWORD CurTime = WINPORT(GetTickCount)();

				if (CurTime - StartTime > RedrawTimeout) {
					StartTime = CurTime;

					INT64 Total = ReverseSearch ? StartPos : FileSize - StartPos;
					INT64 Current = _abs64(CurPos - StartPos);
					int Percent = Total > 0 ? static_cast<int>(Current * 100 / Total) : -1;
					// В случае если файл увеличивается размере, то количество
					// процентов может быть больше 100. Обрабатываем эту ситуацию.
					if (Percent > 100) {
						SetFileSize();
						Total = FileSize - StartPos;
						Percent = Total > 0 ? static_cast<int>(Current * 100 / Total) : -1;
						if (Percent > 100) {
							Percent = 100;
						}
					}
					ViewerSearchMsg(strMsgStr, Percent);

					if (CheckForEscSilent()) {
						if (ConfirmAbortOp()) {
							Redraw();
							return;
						}
					}
				}

				/*
					$ 01.08.2000 KM
					Убран кусок текста после приведения поисковой строки
					и Buf к единому регистру, если поиск не регистрозависимый
					или не ищется Hex-строка и в связи с этим переработан код поиска
				*/
				int MaxSize = ReadSize - SearchWChars + 1;
				int Increment = ReverseSearch ? -1 : +1;

				for (int I = ReverseSearch ? MaxSize - 1 : 0; I < MaxSize && I >= 0; I+= Increment) {
					/*
						$ 01.08.2000 KM
						Обработка поиска "Whole words"
					*/
					bool locResultLeft = false;
					bool locResultRight = false;

					if (WholeWords) {
						if (I) {
							if (IsSpace(Buf[I - 1]) || IsEol(Buf[I - 1])
									|| (wcschr(Opt.strWordDiv, Buf[I - 1])))
								locResultLeft = true;
						} else {
							locResultLeft = true;
						}

						if (ReadSize != BufSize && I + SearchWChars >= ReadSize)
							locResultRight = true;
						else if (I + SearchWChars < ReadSize
								&& (IsSpace(Buf[I + SearchWChars]) || IsEol(Buf[I + SearchWChars])
										|| (wcschr(Opt.strWordDiv, Buf[I + SearchWChars]))))
							locResultRight = true;
					} else {
						locResultLeft = true;
						locResultRight = true;
					}

					if (locResultLeft && locResultRight) {
						if (!Case && !SearchHex) {
							Match = CheckBufMatchesCaseInsensitive(SearchWChars, &Buf[I], strSearchStr.CPtr(),
									strSearchStrLowerCase.CPtr());
						} else {
							Match = CheckBufMatchesCaseSensitive(SearchWChars, &Buf[I], strSearchStr.CPtr());
						}
						if (Match) {
							MatchPos = CurPos + CalcCodeUnitsDistance(VM.CodePage, Buf, Buf + I);
							break;
						}
					}
				}

				if (ReverseSearch) {
					if (CurPos <= 0)
						break;

					vseek(CurPos, SEEK_SET);
				} else {
					if (vtell() >= FileSize)
						break;

					vseek(-(SearchCodeUnits + !!WholeWords), SEEK_CUR);
				}
			}
		}
	}

	if (Match) {
		/*
			$ 24.01.2003 KM
			! По окончании поиска отступим от верха экрана на
			треть отображаемой высоты.
		*/
		SelectText(MatchPos, SearchCodeUnits, ReverseSearch ? 0x2 : 0);

		// Покажем найденное на расстоянии трети экрана от верха.
		int FromTop = (ScrY - (Opt.ViOpt.ShowKeyBar ? 2 : 1)) / 4;

		if (FromTop < 0 || FromTop > ScrY)
			FromTop = 0;

		for (int i = 0; i < FromTop; i++)
			Up();

		AdjustSelPosition = TRUE;
		Show();
		AdjustSelPosition = FALSE;
	} else {
		// Show();
		/*
			$ 27.01.2003 VVM
			+ После окончания поиска спросим о переходе поиска в начало/конец
		*/
		if (SearchFlags.Check(SEARCH_MODE2))
			Message(MSG_WARNING, 1, Msg::ViewSearchTitle,
					(SearchHex ? Msg::ViewSearchCannotFindHex : Msg::ViewSearchCannotFind), strMsgStr,
					Msg::Ok);
		else {
			if (!Message(MSG_WARNING, 2, Msg::ViewSearchTitle,
						(SearchHex ? Msg::ViewSearchCannotFindHex : Msg::ViewSearchCannotFind), strMsgStr,
						(ReverseSearch ? Msg::ViewSearchFromEnd : Msg::ViewSearchFromBegin), Msg::HYes,
						Msg::HNo))
				Search(2, 0);
		}
	}
}

/*
void Viewer::ConvertToHex(char *SearchStr,int &SearchLength)
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
			} else {
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
}
*/

int Viewer::HexToNum(int Hex)
{
	Hex = toupper(Hex);

	if (Hex >= '0' && Hex <= '9')
		return (Hex - '0');

	if (Hex >= 'A' && Hex <= 'F')
		return (Hex - 'A' + 10);

	return (-1000);
}

int Viewer::GetWrapMode()
{
	return (VM.Wrap);
}

void Viewer::SetWrapMode(int Wrap)
{
	Viewer::VM.Wrap = Wrap;
}

void Viewer::EnableHideCursor(int HideCursor)
{
	Viewer::HideCursor = HideCursor;
}

int Viewer::GetWrapType()
{
	return (VM.WordWrap);
}

void Viewer::SetWrapType(int TypeWrap)
{
	Viewer::VM.WordWrap = TypeWrap;
}

void Viewer::GetFileName(FARString &strName)
{
	strName = strFullFileName;
}

void Viewer::SetProcessed(bool Processed)
{
	VM.Processed = Processed;
	ChangeViewKeyBar();
	Show();
}

void Viewer::ShowConsoleTitle()
{
	FARString strTitle;
	strTitle.Format(Msg::InViewer, PointToName(FHP->GetPathName()));
	ConsoleTitle::SetFarTitle(strTitle);
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
	FilePos = Pos;
}

void Viewer::SetPluginData(const wchar_t *PluginData)
{
	Viewer::strPluginData = NullToEmpty(PluginData);
}

void Viewer::SetNamesList(NamesList *List)
{
	if (List)
		List->MoveData(ViewNamesList);
}

int Viewer::vread(wchar_t *Buf, int Count, bool Raw)
{
	if (VM.CodePage == CP_WIDE_LE || VM.CodePage == CP_WIDE_BE) {
		DWORD ReadSize = ViewFile.Read((char *)Buf, Count * sizeof(wchar_t));
		DWORD ResultedCount = ReadSize / sizeof(wchar_t);
		if ((ReadSize % sizeof(wchar_t)) != 0 && (int)ResultedCount < Count) {
			memset(((char *)Buf) + ReadSize, 0, sizeof(wchar_t) - (ReadSize % sizeof(wchar_t)));
			++ResultedCount;
		}

		if (VM.CodePage == CP_WIDE_BE && !Raw) {
			RevBytes(Buf, ResultedCount);
		}

		return ResultedCount;
	} else if (VM.CodePage == CP_UTF8 && !Raw) {
		INT64 Ptr;
		ViewFile.GetPointer(Ptr);
		int ResultedCount = 0;
		for (DWORD WantViewSize = Count; ResultedCount < Count;) {
			DWORD ViewSize = WantViewSize;
			char *SrcView = (char *)ViewFile.ViewBytesAt(Ptr, ViewSize);
			if (!ViewSize) {
				break;
			}

			size_t SourcedCount = ViewSize;
			size_t SinkedCount = (Count - ResultedCount);

			const auto cr = MB2Wide_Unescaped(SrcView, SourcedCount, &Buf[ResultedCount], SinkedCount, false);

			Ptr+= SourcedCount;
			ResultedCount+= (int)SinkedCount;

			if ((cr & CONV_NEED_MORE_SRC) != 0 && SourcedCount == 0) {
				if (ViewSize < WantViewSize) {
					if (ResultedCount >= Count) {
						break;
					}
					Buf[ResultedCount] = WCHAR_REPLACEMENT;
					++ResultedCount;
					++Ptr;

				} else {
					WantViewSize+= (4 + WantViewSize / 4);
				}

			} else if ((cr & CONV_NEED_MORE_DST) != 0 || (cr & CONV_NEED_MORE_SRC) == 0) {
				break;
			}
		}

		ViewFile.SetPointer(Ptr);
		return ResultedCount;
	} else {
		INT64 Ptr;
		ViewFile.GetPointer(Ptr);

		DWORD ReadSize = Count;
		if (VM.CodePage == CP_UTF16LE || VM.CodePage == CP_UTF16BE) {
			ReadSize*= 2;
		}

		LPBYTE View = ViewFile.ViewBytesAt(Ptr, ReadSize);

		if (Count == 1 && ReadSize == 2 && !Raw && (VM.CodePage == CP_UTF16LE || VM.CodePage == CP_UTF16BE)) {
			// Если UTF16 то простой ли это символ или нет?
			if (*(uint16_t *)View >= 0xd800 && *(uint16_t *)View <= 0xdfff) {
				ReadSize+= 2;
				View = ViewFile.ViewBytesAt(Ptr, ReadSize);
			}
		}

		ViewFile.SetPointer(Ptr + ReadSize);

		if (!View || !ReadSize)
			return 0;

		if (Raw) {
			if (VM.CodePage == CP_UTF16LE || VM.CodePage == CP_UTF16BE) {
				ReadSize/= 2;
				for (DWORD i = 0; i < ReadSize; ++i) {
					Buf[i] = (unsigned char)View[i * 2 + 1];
					Buf[i]<<= 8;
					Buf[i]|= (unsigned char)View[i * 2];
				}
			} else {
				for (DWORD i = 0; i < ReadSize; i++) {
					Buf[i] = (wchar_t)(unsigned char)View[i];
				}
			}
		} else {
			ReadSize = WINPORT(MultiByteToWideChar)(VM.CodePage, 0, (const char *)View, ReadSize, Buf, Count);
		}

		return ReadSize;
	}
}

void Viewer::vseek(int64_t Offset, int Whence)
{
	switch (VM.CodePage) {
		case CP_UTF32BE:
		case CP_UTF32LE:
			Offset*= 4;
			break;
		case CP_UTF16BE:
		case CP_UTF16LE:
			Offset*= 2;
			break;
	}
	ViewFile.SetPointer(Offset, Whence);
}

int64_t Viewer::vtell()
{
	INT64 Ptr = 0;
	ViewFile.GetPointer(Ptr);
	switch (VM.CodePage) {
		case CP_UTF32BE:
		case CP_UTF32LE:
			Ptr = (Ptr + (Ptr & 3)) / 4;
			break;
		case CP_UTF16BE:
		case CP_UTF16LE:
			Ptr = (Ptr + (Ptr & 1)) / 2;
			break;
	}
	return Ptr;
}

bool Viewer::vgetc(WCHAR &C)
{
	bool Result = false;

	if (vread(&C, 1)) {
		Result = true;
	}
	return Result;
}

#define RB_PRC 3
#define RB_HEX 4
#define RB_DEC 5

void Viewer::GoTo(int ShowDlg, int64_t Offset, DWORD Flags)
{
	int64_t Relative = 0;
	const wchar_t *LineHistoryName = L"ViewerOffset";
	DialogDataEx GoToDlgData[] = {
		{DI_DOUBLEBOX,   3, 1, 31, 7, {0}, 0,Msg::ViewerGoTo },
		{DI_EDIT,        5, 2, 29, 2, {(DWORD_PTR)LineHistoryName}, DIF_FOCUS | DIF_DEFAULT | DIF_HISTORY | DIF_USELASTHISTORY, L""},
		{DI_TEXT,        3, 3, 0,  3, {0}, DIF_SEPARATOR, L""},
		{DI_RADIOBUTTON, 5, 4, 0,  4, {0}, DIF_GROUP,     Msg::GoToPercent},
		{DI_RADIOBUTTON, 5, 5, 0,  5, {0}, 0, Msg::GoToHex    },
		{DI_RADIOBUTTON, 5, 6, 0,  6, {0}, 0, Msg::GoToDecimal}
	};
	MakeDialogItemsEx(GoToDlgData, GoToDlg);
	static int PrevMode = 0;
	GoToDlg[3].Selected = GoToDlg[4].Selected = GoToDlg[5].Selected = 0;

	if (VM.Hex)
		PrevMode = 1;

	GoToDlg[PrevMode + 3].Selected = TRUE;
	{
		if (ShowDlg) {
			Dialog Dlg(GoToDlg, ARRAYSIZE(GoToDlg));
			Dlg.SetHelp(L"ViewerGotoPos");
			Dlg.SetPosition(-1, -1, 35, 9);
			Dlg.Process();

			if (Dlg.GetExitCode() <= 0)
				return;

			if (GoToDlg[1].strData.At(0) == L'+' || GoToDlg[1].strData.At(0) == L'-')		// юзер хочет относительности
			{
				if (GoToDlg[1].strData.At(0) == L'+')
					Relative = 1;
				else
					Relative = -1;

				GoToDlg[1].strData.LShift(1);
			}

			if (GoToDlg[1].strData.Contains(L'%'))		// он хочет процентов
			{
				GoToDlg[RB_HEX].Selected = GoToDlg[RB_DEC].Selected = 0;
				GoToDlg[RB_PRC].Selected = 1;
			} else if (!StrCmpNI(GoToDlg[1].strData, L"0x", 2) || GoToDlg[1].strData.At(0) == L'$' || GoToDlg[1].strData.Contains(L'h')
					|| GoToDlg[1].strData.Contains(L'H'))		// он умный - hex код ввел!
			{
				GoToDlg[RB_PRC].Selected = GoToDlg[RB_DEC].Selected = 0;
				GoToDlg[RB_HEX].Selected = 1;

				if (!StrCmpNI(GoToDlg[1].strData, L"0x", 2))
					GoToDlg[1].strData.LShift(2);
				else if (GoToDlg[1].strData.At(0) == L'$')
					GoToDlg[1].strData.LShift(1);

				// Relative=0; // при hex значении никаких относительных значений?
			}

			if (GoToDlg[RB_PRC].Selected) {
				// int cPercent=ToPercent64(FilePos,FileSize);
				PrevMode = 0;
				int Percent = _wtoi(GoToDlg[1].strData);

				// if ( Relative && (cPercent+Percent*Relative<0) || (cPercent+Percent*Relative>100)) // за пределы - низя
				//	return;
				if (Percent > 100)
					return;

				// if ( Percent<0 )
				//	Percent=0;
				Offset = FileSize / 100 * Percent;

				switch (VM.CodePage) {
					case CP_UTF32LE:
					case CP_UTF32BE:
						Offset*= 4;
						break;
					case CP_UTF16LE:
					case CP_UTF16BE:
						Offset*= 2;
						break;
				}

				while (ToPercent64(Offset, FileSize) < Percent)
					Offset++;
			}

			if (GoToDlg[RB_HEX].Selected) {
				PrevMode = 1;
				Offset = wcstoull(GoToDlg[1].strData, nullptr, 16);
			}

			if (GoToDlg[RB_DEC].Selected) {
				PrevMode = 2;
				Offset = wcstoull(GoToDlg[1].strData, nullptr, 10);
			}
		}		// ShowDlg
		else {
			Relative = Flags & VSP_RELATIVE;
			if (Offset < 0)
				Relative = -Relative;

			if (Flags & VSP_PERCENT) {
				int64_t Percent = Offset;

				if (Percent > 100)
					return;

				// if ( Percent<0 )
				//	Percent=0;
				Offset = FileSize / 100 * Percent;

				switch (VM.CodePage) {
					case CP_UTF32LE:
					case CP_UTF32BE:
						Offset*= 4;
						break;
					case CP_UTF16LE:
					case CP_UTF16BE:
						Offset*= 2;
						break;
				}

				while (ToPercent64(Offset, FileSize) < Percent)
					Offset++;
			}
		}

		if (Relative) {
			if (Relative == -1 && Offset > FilePos)		// меньше нуля, if (FilePos<0) не пройдет - FilePos у нас uint32_t
				FilePos = 0;
			else
				switch (VM.CodePage) {
					case CP_UTF32LE:
					case CP_UTF32BE:
						FilePos = FilePos + Offset * Relative / 4;
						break;

					case CP_UTF16LE:
					case CP_UTF16BE:
						FilePos = FilePos + Offset * Relative / 2;
						break;

					default:
						FilePos = FilePos + Offset * Relative;
				}

		} else
			switch (VM.CodePage) {
				case CP_UTF32LE:
				case CP_UTF32BE:
					FilePos = Offset / 4;
					break;

				case CP_UTF16LE:
				case CP_UTF16BE:
					FilePos = Offset / 2;
					break;

				default:
					FilePos = Offset;
			}

		if (FilePos > FileSize || FilePos < 0)	// и куда его несет?
			FilePos = FileSize;					// там все равно ничего нету
	}
	// коррекция
	AdjustFilePos();

	//	LastSelPos=FilePos;
	if (!(Flags & VSP_NOREDRAW))
		Show();
}

void Viewer::AdjustFilePos()
{
	if (!VM.Hex) {
		wchar_t Buf[4096];
		int64_t StartLinePos = -1, GotoLinePos = FilePos - (int64_t)sizeof(Buf) / sizeof(wchar_t);

		if (GotoLinePos < 0)
			GotoLinePos = 0;

		vseek(GotoLinePos, SEEK_SET);
		int ReadSize = (int)Min((int64_t)ARRAYSIZE(Buf), (int64_t)(FilePos - GotoLinePos));
		ReadSize = vread(Buf, ReadSize);

		for (int I = ReadSize - 1; I >= 0; I--)
			if (Buf[I] == (wchar_t)CRSym) {
				StartLinePos = GotoLinePos + I;
				break;
			}

		vseek(FilePos + 1, SEEK_SET);

		if (VM.Hex) {
			size_t len = 8;
			switch (VM.CodePage) {
				case CP_UTF32LE:
				case CP_UTF32BE:
					len*= 4;
					break;

				case CP_UTF16LE:
				case CP_UTF16BE:
					len*= 2;
					break;
			}

			FilePos&= ~(len - 1);
		} else {
			if (FilePos != StartLinePos)
				Up();
		}
	}
}

void Viewer::SetFileSize()
{
	if (!ViewFile.Opened())
		return;

	UINT64 uFileSize = 0;	// BUGBUG, sign
	ViewFile.GetSize(uFileSize);
	FileSize = uFileSize;

	/*
		$ 20.02.2003 IS
		Везде сравниваем FilePos с FileSize, FilePos для юникодных файлов
		уменьшается в два раза, поэтому FileSize тоже надо уменьшать
	*/
	switch (VM.CodePage) {
		case CP_UTF32LE:
		case CP_UTF32BE:
			FileSize = (FileSize + (FileSize & 3)) / 4;
			break;

		case CP_UTF16LE:
		case CP_UTF16BE:
			FileSize = (FileSize + (FileSize & 1)) / 2;
			break;
	}
}

void Viewer::GetSelectedParam(int64_t &Pos, int64_t &Length, DWORD &Flags)
{
	Pos = SelectPos;
	Length = SelectSize;
	Flags = SelectFlags;
}

/*
	$ 19.01.2001 SVS
	Выделение - в качестве самостоятельной функции.
	Flags=
		0x01 - показывать (делать Show())
		0x02 - "обратный поиск" ?
*/
void Viewer::SelectText(const int64_t &MatchPos, const int64_t &SearchLength, const DWORD Flags)
{
	if (!ViewFile.Opened())
		return;

	wchar_t Buf[MAX_VIEWLINE];
	int64_t StartLinePos = -1, SearchLinePos = MatchPos - sizeof(Buf) / sizeof(wchar_t);

	if (SearchLinePos < 0)
		SearchLinePos = 0;

	vseek(SearchLinePos, SEEK_SET);
	int ReadSize = (int)Min((int64_t)ARRAYSIZE(Buf), (int64_t)(MatchPos - SearchLinePos));
	ReadSize = vread(Buf, ReadSize);

	for (int I = ReadSize - 1; I >= 0; I--)
		if (Buf[I] == (wchar_t)CRSym) {
			StartLinePos = SearchLinePos + I;
			break;
		}

	// MessageBeep(0);
	vseek(MatchPos + 1, SEEK_SET);
	SelectPos = FilePos = MatchPos;
	SelectSize = SearchLength;
	SelectFlags = Flags;

	//	LastSelPos=SelectPos+((Flags&0x2) ? -1:1);
	LastSelPos = SelectPos + SearchLength * ((Flags & 0x2) ? -1 : 1);
	if (VM.Hex) {
		size_t len = 8;
		switch (VM.CodePage) {
			case CP_UTF32LE:
			case CP_UTF32BE:
				len*= 4;
				break;

			case CP_UTF16LE:
			case CP_UTF16BE:
				len*= 2;
				break;
		}
		FilePos&= ~(len - 1);
	} else {
		if (SelectPos != StartLinePos) {
			Up();
			Show();		// update OutStr
		}

		int64_t Length = SelectPos - StartLinePos - 1;

		if (VM.Wrap)
			Length%= Width + 1;		//??

		if (Length <= Width)
			LeftPos = 0;

		if (Length - LeftPos > Width || Length < LeftPos) {
			LeftPos = Length;

			if (LeftPos > (MAX_VIEWLINE - 1) || LeftPos < 0)
				LeftPos = 0;
			else if (LeftPos > 10)
				LeftPos-= 10;
		}
	}

	if (Flags & 1) {
		AdjustSelPosition = TRUE;
		Show();
		AdjustSelPosition = FALSE;
	}
}

int Viewer::ViewerControl(int Command, void *Param)
{
	switch (Command) {
		case VCTL_GETINFO: {
			if (Param) {
				ViewerInfo *Info = (ViewerInfo *)Param;
				memset(&Info->ViewerID, 0, Info->StructSize - sizeof(Info->StructSize));
				Info->ViewerID = ViewerID;
				Info->FileName = strFullFileName;
				Info->WindowSizeX = ObjWidth;
				Info->WindowSizeY = Y2 - Y1 + 1;
				Info->FilePos = FilePos;
				Info->FileSize = FileSize;
				Info->CurMode = VM;
				Info->Options = 0;

				if (Opt.ViOpt.SavePos)
					Info->Options|= VOPT_SAVEFILEPOSITION;

				if (ViOpt.AutoDetectCodePage)
					Info->Options|= VOPT_AUTODETECTCODEPAGE;

				Info->TabSize = ViOpt.TabSize;
				Info->LeftPos = LeftPos;
				return TRUE;
			}

			break;
		}
		/*
			Param = ViewerSetPosition
				сюда же будет записано новое смещение
				В основном совпадает с переданным
		*/
		case VCTL_SETPOSITION: {
			if (Param) {
				ViewerSetPosition *vsp = (ViewerSetPosition *)Param;
				bool isReShow = vsp->StartPos != FilePos;

				if ((LeftPos = vsp->LeftPos) < 0)
					LeftPos = 0;

				/*
					$ 20.01.2003 IS
					Если кодировка - юникод, то оперируем числами, уменьшенными в
					2 раза. Поэтому увеличим StartPos в 2 раза, т.к. функция
					GoTo принимает смещения в _байтах_.
				*/

				int64_t NewPos = vsp->StartPos;

				switch (VM.CodePage) {
					case CP_UTF32LE:
					case CP_UTF32BE:
						NewPos*= 4;
						break;

					case CP_UTF16LE:
					case CP_UTF16BE:
						NewPos*= 2;
						break;
				}

				GoTo(FALSE, NewPos, vsp->Flags);

				if (isReShow && !(vsp->Flags & VSP_NOREDRAW))
					ScrBuf.Flush();

				if (!(vsp->Flags & VSP_NORETNEWPOS)) {
					vsp->StartPos = FilePos;
					vsp->LeftPos = LeftPos;
				}

				return TRUE;
			}

			break;
		}
		// Param=ViewerSelect
		case VCTL_SELECT: {
			if (Param) {
				ViewerSelect *vs = (ViewerSelect *)Param;
				int64_t SPos = vs->BlockStartPos;
				int SSize = vs->BlockLen;

				if (SPos < FileSize) {
					if (SPos + SSize > FileSize) {
						SSize = (int)(FileSize - SPos);
					}

					SelectText(SPos, SSize, 0x1);
					ScrBuf.Flush();
					return TRUE;
				}
			} else {
				SelectSize = 0;
				Show();
			}

			break;
		}
		/*
			Функция установки Keybar Labels
			Param = nullptr - восстановить, пред. значение
			Param = -1 - обновить полосу (перерисовать)
			Param = KeyBarTitles
		*/
		case VCTL_SETKEYBAR: {
			KeyBarTitles *Kbt = (KeyBarTitles *)Param;

			if (!Kbt) {		// восстановить пред значение!
				if (HostFileViewer)
					HostFileViewer->InitKeyBar();
			} else {
				if ((LONG_PTR)Param != (LONG_PTR)-1)	// не только перерисовать?
				{
					for (int i = 0; i < 12; i++) {
						if (Kbt->Titles[i])
							ViewKeyBar->Change(KBL_MAIN, Kbt->Titles[i], i);

						if (Kbt->CtrlTitles[i])
							ViewKeyBar->Change(KBL_CTRL, Kbt->CtrlTitles[i], i);

						if (Kbt->AltTitles[i])
							ViewKeyBar->Change(KBL_ALT, Kbt->AltTitles[i], i);

						if (Kbt->ShiftTitles[i])
							ViewKeyBar->Change(KBL_SHIFT, Kbt->ShiftTitles[i], i);

						if (Kbt->CtrlShiftTitles[i])
							ViewKeyBar->Change(KBL_CTRLSHIFT, Kbt->CtrlShiftTitles[i], i);

						if (Kbt->AltShiftTitles[i])
							ViewKeyBar->Change(KBL_ALTSHIFT, Kbt->AltShiftTitles[i], i);

						if (Kbt->CtrlAltTitles[i])
							ViewKeyBar->Change(KBL_CTRLALT, Kbt->CtrlAltTitles[i], i);
					}
				}

				ViewKeyBar->Refresh(true);
				ScrBuf.Flush();		//?????
			}

			return TRUE;
		}
		// Param=0
		case VCTL_REDRAW: {
			ChangeViewKeyBar();
			Show();
			ScrBuf.Flush();
			return TRUE;
		}
		// Param=0
		case VCTL_QUIT: {
			/*
				$ 28.12.2002 IS
				Разрешаем выполнение VCTL_QUIT только для вьюера, который
				не является панелью информации и быстрого просмотра (т.е.
				фактически панелей на экране не видно)
			*/
			if (FrameManager->IsPanelsActive())
				return FALSE;

			/*
				$ 29.09.2002 IS
				без этого не закрывался вьюер, а просили именно это
			*/
			FrameManager->DeleteFrame(HostFileViewer);

			if (HostFileViewer)
				HostFileViewer->SetExitCode(0);

			return TRUE;
		}
		/*
			Функция установки режимов
				Param = ViewerSetMode
		*/
		case VCTL_SETMODE: {
			ViewerSetMode *vsmode = (ViewerSetMode *)Param;

			if (vsmode) {
				bool isRedraw = vsmode->Flags & VSMFL_REDRAW ? true : false;

				switch (vsmode->Type) {
					case VSMT_HEX:
						ProcessHexMode(vsmode->Param.iParam, isRedraw);
						return TRUE;
					case VSMT_WRAP:
						ProcessWrapMode(vsmode->Param.iParam, isRedraw);
						return TRUE;
					case VSMT_WORDWRAP:
						ProcessTypeWrapMode(vsmode->Param.iParam, isRedraw);
						return TRUE;
				}
			}

			return FALSE;
		}
	}

	return FALSE;
}

int Viewer::ProcessHexMode(int newMode, bool isRedraw)
{
	// BUGBUG
	// До тех пор, пока не будет реализован адекватный hex-просмотр в UTF8 - будем смотреть в OEM.
	// Ибо сейчас это не просмотр, а генератор однотипных унылых багрепортов.
	if (VM.CodePage == CP_UTF8 && newMode) {
		VM.CodePage = WINPORT(GetACP)();
	}

	int oldHex = VM.Hex;
	VM.Hex = newMode & 1;

	if (isRedraw) {
		ChangeViewKeyBar();
		Show();
	}

	// LastSelPos=FilePos;
	return oldHex;
}

int Viewer::ProcessWrapMode(int newMode, bool isRedraw)
{
	int oldWrap = VM.Wrap;
	VM.Wrap = newMode & 1;

	if (VM.Wrap)
		LeftPos = 0;

	if (!VM.Wrap && LastPage)
		Up();

	if (isRedraw) {
		ChangeViewKeyBar();
		Show();
	}

	Opt.ViOpt.ViewerIsWrap = VM.Wrap;
	//	LastSelPos=FilePos;
	return oldWrap;
}

int Viewer::ProcessTypeWrapMode(int newMode, bool isRedraw)
{
	int oldTypeWrap = VM.WordWrap;
	VM.WordWrap = newMode & 1;

	if (!VM.Wrap) {
		VM.Wrap = !VM.Wrap;
		LeftPos = 0;
	}

	if (isRedraw) {
		ChangeViewKeyBar();
		Show();
	}

	Opt.ViOpt.ViewerWrap = VM.WordWrap;
	// LastSelPos=FilePos;
	return oldTypeWrap;
}
