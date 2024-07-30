/*
fileview.cpp

Просмотр файла - надстройка над viewer.cpp
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

#include <limits>
#include "fileview.hpp"
#include "codepage.hpp"
#include "lang.hpp"
#include "keys.hpp"
#include "ctrlobj.hpp"
#include "filepanels.hpp"
#include "panel.hpp"
#include "history.hpp"
#include "manager.hpp"
#include "fileedit.hpp"
#include "cmdline.hpp"
#include "savescr.hpp"
#include "syslog.hpp"
#include "interf.hpp"
#include "keyboard.hpp"
#include "message.hpp"
#include "config.hpp"
#include "strmix.hpp"
#include "mix.hpp"
#include "fileholder.hpp"
#include "GrepFile.hpp"
#include "exitcode.hpp"

FileViewer::FileViewer(FileHolderPtr NewFileHolder, int EnableSwitch, int DisableHistory, int DisableEdit,
		long ViewStartPos, const wchar_t *PluginData, NamesList *ViewNamesList, int ToSaveAs, UINT aCodePage)
	:
	View(false, aCodePage), FullScreen(TRUE), DisableEdit(DisableEdit)
{
	_OT(SysLog(L"[%p] FileViewer::FileViewer(I variant...)", this));
	SetPosition(0, 0, ScrX, ScrY);
	Init(NewFileHolder, EnableSwitch, DisableHistory, ViewStartPos, PluginData, ViewNamesList, ToSaveAs);
}

FileViewer::FileViewer(FileHolderPtr NewFileHolder, int EnableSwitch, int DisableHistory, const wchar_t *Title,
		int X1, int Y1, int X2, int Y2, UINT aCodePage)
	:
	View(false, aCodePage)
{
	_OT(SysLog(L"[%p] FileViewer::FileViewer(II variant...)", this));
	DisableEdit = TRUE;

	if (X1 < 0)
		X1 = 0;

	if (X2 < 0 || X2 > ScrX)
		X2 = ScrX;

	if (Y1 < 0)
		Y1 = 0;

	if (Y2 < 0 || Y2 > ScrY)
		Y2 = ScrY;

	if (X1 >= X2) {
		X1 = 0;
		X2 = ScrX;
	}

	if (Y1 >= Y2) {
		Y1 = 0;
		Y2 = ScrY;
	}

	SetPosition(X1, Y1, X2, Y2);
	FullScreen = (!X1 && !Y1 && X2 == ScrX && Y2 == ScrY);
	View.SetTitle(Title);
	Init(NewFileHolder, EnableSwitch, DisableHistory, -1, L"", nullptr, FALSE);
}

void FileViewer::Init(FileHolderPtr NewFileHolder, int EnableSwitch, int disableHistory,	///
		long ViewStartPos, const wchar_t *PluginData, NamesList *ViewNamesList, int ToSaveAs)
{
	RedrawTitle = FALSE;
	ViewKeyBar.SetOwner(this);
	ViewKeyBar.SetPosition(X1, Y2, X2, Y2);
	KeyBarVisible = Opt.ViOpt.ShowKeyBar;
	TitleBarVisible = Opt.ViOpt.ShowTitleBar;
	int OldMacroMode = CtrlObject->Macro.GetMode();
	MacroMode = MACRO_VIEWER;
	CtrlObject->Macro.SetMode(MACRO_VIEWER);
	View.SetPluginData(PluginData);
	View.SetHostFileViewer(this);
	DisableHistory = disableHistory;	///
	strName = NewFileHolder->GetPathName();
	SetCanLoseFocus(EnableSwitch);
	SaveToSaveAs = ToSaveAs;
	InitKeyBar();

	if (!View.OpenFile(NewFileHolder, TRUE))		// $ 04.07.2000 tran + add TRUE as 'warning' parameter
	{
		DisableHistory = TRUE;				// $ 26.03.2002 DJ - при неудаче открытия - не пишем мусор в историю
		// FrameManager->DeleteFrame(this); // ЗАЧЕМ? Вьювер то еще не помещен в очередь манагера!
		ExitCode = FALSE;
		CtrlObject->Macro.SetMode(OldMacroMode);
		return;
	}

	if (ViewStartPos != -1)
		View.SetFilePos(ViewStartPos);

	if (ViewNamesList)
		View.SetNamesList(ViewNamesList);

	ExitCode = TRUE;
	ViewKeyBar.Refresh(true);

	if (!Opt.ViOpt.ShowKeyBar)
		ViewKeyBar.Hide0();

	ShowConsoleTitle();
	AutoClose = false;
	F3KeyOnly = true;

	if (EnableSwitch) {
		FrameManager->InsertFrame(this);
	} else {
		FrameManager->ExecuteFrame(this);
	}
}

void FileViewer::InitKeyBar()
{
	ViewKeyBar.SetAllGroup(KBL_MAIN, Opt.OnlyEditorViewerUsed ? Msg::SingleViewF1 : Msg::ViewF1, 12);
	ViewKeyBar.SetAllGroup(KBL_SHIFT, Msg::ViewShiftF1, 12);

	ViewKeyBar.SetAllGroup(KBL_ALT, Opt.OnlyEditorViewerUsed ? Msg::SingleViewAltF1 : Msg::ViewAltF1, 12);
	ViewKeyBar.SetAllGroup(KBL_CTRL, Opt.OnlyEditorViewerUsed ? Msg::SingleViewCtrlF1 : Msg::ViewCtrlF1, 12);
	ViewKeyBar.SetAllGroup(KBL_CTRLSHIFT,
			Opt.OnlyEditorViewerUsed ? Msg::SingleViewCtrlShiftF1 : Msg::ViewCtrlShiftF1, 12);
	ViewKeyBar.SetAllGroup(KBL_CTRLALT,
			Opt.OnlyEditorViewerUsed ? Msg::SingleViewCtrlAltF1 : Msg::ViewCtrlAltF1, 12);
	ViewKeyBar.SetAllGroup(KBL_ALTSHIFT,
			Opt.OnlyEditorViewerUsed ? Msg::SingleViewAltShiftF1 : Msg::ViewAltShiftF1, 12);
	ViewKeyBar.SetAllGroup(KBL_CTRLALTSHIFT,
			Opt.OnlyEditorViewerUsed ? Msg::SingleViewCtrlAltShiftF1 : Msg::ViewCtrlAltShiftF1, 12);

	if (DisableEdit)
		ViewKeyBar.Change(KBL_MAIN, L"", 6 - 1);

	if (!GetCanLoseFocus())
		ViewKeyBar.Change(KBL_MAIN, L"", 12 - 1);

	if (!GetCanLoseFocus())
		ViewKeyBar.Change(KBL_ALT, L"", 11 - 1);

	if (!Opt.UsePrintManager || CtrlObject->Plugins.FindPlugin(SYSID_PRINTMANAGER))
		ViewKeyBar.Change(KBL_ALT, L"", 5 - 1);

	ViewKeyBar.ReadRegGroup(L"Viewer", Opt.strLanguage);
	ViewKeyBar.SetAllRegGroup();
	SetKeyBar(&ViewKeyBar);
	View.SetPosition(X1, Y1 + (Opt.ViOpt.ShowTitleBar ? 1 : 0), X2, Y2 - (Opt.ViOpt.ShowKeyBar ? 1 : 0));
	View.SetViewKeyBar(&ViewKeyBar);
}

void FileViewer::Show()
{
	if (FullScreen) {
		if (Opt.ViOpt.ShowKeyBar) {
			ViewKeyBar.SetPosition(0, ScrY, ScrX, ScrY);
			ViewKeyBar.Redraw();
		}

		SetPosition(0, 0, ScrX, ScrY - (Opt.ViOpt.ShowKeyBar ? 1 : 0));
		View.SetPosition(0, (Opt.ViOpt.ShowTitleBar ? 1 : 0), ScrX, ScrY - (Opt.ViOpt.ShowKeyBar ? 1 : 0));
	}

	ScreenObject::Show();
	ShowStatus();
}

void FileViewer::DisplayObject()
{
	View.Show();
}

int64_t FileViewer::VMProcess(MacroOpcode OpCode, void *vParam, int64_t iParam)
{
	return View.VMProcess(OpCode, vParam, iParam);
}

int FileViewer::ProcessKey(FarKey Key)
{
	if (RedrawTitle
			&& (((unsigned int)Key & 0x00ffffff) < KEY_END_FKEY
					|| IS_INTERNAL_KEY_REAL((unsigned int)Key & 0x00ffffff)))
		ShowConsoleTitle();

	if (Key != KEY_F3 && Key != KEY_IDLE)
		F3KeyOnly = false;

	if (Key == KEY_ESC && UngreppedFH) {
		GrepFilterDismiss();
		return TRUE;
	}

	switch (Key) {
#if 0
			/*
				$ 30.05.2003 SVS
				Фича :-) Shift-F4 в редакторе/вьювере позволяет открывать другой редактор/вьювер
				Пока закомментим
			*/
		case KEY_SHIFTF4:
		{
			if (!Opt.OnlyEditorViewerUsed)
				CtrlObject->Cp()->ActivePanel->ProcessKey(Key);

			return TRUE;
		}
#endif
		case KEY_CTRLF7: {
			GrepFilter();
			return (TRUE);
		}
		/*
			$ 22.07.2000 tran
			+ выход по ctrl-f10 с установкой курсора на файл
		*/
		case KEY_CTRLF10: {
			if (View.GetFileHolder()->IsTemporary()) {
				// if viewing temporary file - dont allow this
				return TRUE;
			}

			SaveScreen Sc;
			FARString strFileName;
			View.GetFileName(strFileName);
			CtrlObject->Cp()->GoToFile(strFileName);
			RedrawTitle = TRUE;
			return (TRUE);
		}
		// $ 15.07.2000 tran + CtrlB switch KeyBar
		case KEY_CTRLB:
			Opt.ViOpt.ShowKeyBar = !Opt.ViOpt.ShowKeyBar;

			if (!Opt.ViOpt.ShowKeyBar)
				ViewKeyBar.Hide0();		// 0 mean - Don't purge saved screen

			ViewKeyBar.Refresh(Opt.ViOpt.ShowKeyBar);
			Show();
			KeyBarVisible = Opt.ViOpt.ShowKeyBar;
			return (TRUE);
		case KEY_CTRLSHIFTB: {
			Opt.ViOpt.ShowTitleBar = !Opt.ViOpt.ShowTitleBar;
			TitleBarVisible = Opt.ViOpt.ShowTitleBar;
			Show();
			return (TRUE);
		}
		case KEY_CTRLO:

			if (!Opt.OnlyEditorViewerUsed) {
				if (FrameManager->ShowBackground()) {
					SetCursorType(FALSE, 0);
					WaitKey();
					FrameManager->RefreshFrame();
				}
			}

			return TRUE;
		case KEY_F3:
		case KEY_NUMPAD5:
		case KEY_SHIFTNUMPAD5:
			if (F3KeyOnly)
				return TRUE;

		case KEY_ESC:
		case KEY_F10:
			FrameManager->DeleteFrame();
			return TRUE;
		case KEY_F6:

			if (!DisableEdit) {
				UINT cp = View.VM.CodePage;
				FARString strViewFileName;
				View.GetFileName(strViewFileName);
				File Edit;
				if (!Edit.Open(strViewFileName, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr,
							OPEN_EXISTING, FILE_FLAG_SEQUENTIAL_SCAN)) {
					Message(MSG_WARNING | MSG_ERRORTYPE, 1, Msg::EditTitle, Msg::EditCannotOpen,
							strViewFileName, Msg::Ok);
					return TRUE;
				}
				Edit.Close();
				SetExitCode(0);
				int64_t FilePos = View.GetFilePos();
				/*
					$ 07.07.2006 IS
					Тут косяк, замеченный при чтении warnings - FilePos теряет информацию при преобразовании int64_t -> int
					Надо бы поправить FileEditor на этот счет.
				*/
				FileEditor *ShellEditor = new FileEditor(View.GetFileHolder(), cp,
						(GetCanLoseFocus() ? FFILEEDIT_ENABLEF6 : 0)
								| (SaveToSaveAs ? FFILEEDIT_SAVETOSAVEAS : 0)
								| (DisableHistory ? FFILEEDIT_DISABLEHISTORY : 0),
						-2, static_cast<int>(FilePos), strPluginData);
				int load = ShellEditor->GetExitCode();
				if (load == XC_LOADING_INTERRUPTED || load == XC_OPEN_ERROR)
				{
					delete ShellEditor;
				}
				else
				{
					ShellEditor->SetEnableF6(TRUE);
					/* $ 07.05.2001 DJ сохраняем NamesList */
					ShellEditor->SetNamesList(View.GetNamesList());
					FrameManager->DeleteFrame(this);	// Insert уже есть внутри конструктора
					ShowTime(2);
				}
			}

			return TRUE;
			// Печать файла с использованием плагина PrintMan
		case KEY_ALTF5: {
			if (Opt.UsePrintManager && CtrlObject->Plugins.FindPlugin(SYSID_PRINTMANAGER))
				CtrlObject->Plugins.CallPlugin(SYSID_PRINTMANAGER, OPEN_VIEWER, 0);		// printman

			return TRUE;
		}
		case KEY_F9:
		case KEY_ALTSHIFTF9:
			// Работа с локальной копией ViewerOptions
			ViewerOptions ViOpt;
			ViOpt.TabSize = View.GetTabSize();
			ViOpt.AutoDetectCodePage = View.GetAutoDetectCodePage();
			ViOpt.ShowScrollbar = View.GetShowScrollbar();
			ViOpt.ShowArrows = View.GetShowArrows();
			ViOpt.PersistentBlocks = View.GetPersistentBlocks();
			ViewerConfig(ViOpt, true);
			View.SetTabSize(ViOpt.TabSize);
			View.SetAutoDetectCodePage(ViOpt.AutoDetectCodePage);
			View.SetShowScrollbar(ViOpt.ShowScrollbar);
			View.SetShowArrows(ViOpt.ShowArrows);
			View.SetPersistentBlocks(ViOpt.PersistentBlocks);

			ViewKeyBar.Refresh(Opt.ViOpt.ShowKeyBar);

			View.Show();
			return TRUE;
		case KEY_ALTF10:
			FrameManager->ExitMainLoop(TRUE);
			return TRUE;
		case KEY_ALTF11:

			if (GetCanLoseFocus())
				CtrlObject->CmdLine->ShowViewEditHistory();

			return TRUE;
		default:
			// Этот кусок - на будущее (по аналогии с редактором :-)
			// if (CtrlObject->Macro.IsExecuting() || !View.ProcessViewerInput(&ReadRec))
			{
				/*
					$ 22.03.2001 SVS
					Это помогло от залипания :-)
				*/
				if (!CtrlObject->Macro.IsExecuting())
					ViewKeyBar.Refresh(Opt.ViOpt.ShowKeyBar);

				if (!ViewKeyBar.ProcessKey(Key)) {

					if (AutoClose) {
						if (Key == (KEY_MSWHEEL_DOWN | KEY_CTRL | KEY_SHIFT))
							Key = KEY_MSWHEEL_DOWN;
						else if (Key == (KEY_MSWHEEL_UP | KEY_CTRL | KEY_SHIFT))
							Key = KEY_MSWHEEL_UP;

						if (Key == KEY_MSWHEEL_DOWN || Key == (KEY_MSWHEEL_DOWN | KEY_ALT)) {
							int64_t FilePosBefore = View.GetFilePos();
							BOOL rv = View.ProcessKey(Key);
							if (FilePosBefore == View.GetFilePos())
								ProcessKey(KEY_ESC);
							return rv;
						}
					}

					return View.ProcessKey(Key);
				}
			}
			return TRUE;
	}
}

void FileViewer::GrepFilter()
{
	if (!UngreppedFH) {
		UngreppedFH = View.GetFileHolder();
		UngreppedPos = View.GetFilePos();
	}
	auto NewFH = GrepFile(UngreppedFH);
	if (!NewFH || !View.OpenFile(NewFH, TRUE)) {
		GrepFilterDismiss();
		return;
	}

	View.SetFilePos(0);
	Show();
}

void FileViewer::GrepFilterDismiss()
{
	View.OpenFile(UngreppedFH, TRUE);
	View.SetFilePos(UngreppedPos);
	UngreppedFH.reset();
	Show();
}


int FileViewer::ProcessMouse(MOUSE_EVENT_RECORD *MouseEvent)
{
	F3KeyOnly = false;
	if (!View.ProcessMouse(MouseEvent))
		if (!ViewKeyBar.ProcessMouse(MouseEvent))
			return FALSE;

	return TRUE;
}

int FileViewer::GetTypeAndName(FARString &strType, FARString &strName)
{
	strType = Msg::ScreensView;
	View.GetFileName(strName);
	return (MODALTYPE_VIEWER);
}

void FileViewer::ShowConsoleTitle()
{
	View.ShowConsoleTitle();
	RedrawTitle = FALSE;
}

FileViewer::~FileViewer()
{
	_OT(SysLog(L"[%p] ~FileViewer::FileViewer()", this));
}

void FileViewer::OnDestroy()
{
	_OT(SysLog(L"[%p] FileViewer::OnDestroy()", this));

	if (!DisableHistory && (CtrlObject->Cp()->ActivePanel || StrCmp(strName, L"-"))) {
		FARString strFullFileName;
		View.GetFileName(strFullFileName);
		CtrlObject->ViewHistory->AddToHistory(strFullFileName, 0);
	}
}

int FileViewer::FastHide()
{
	return Opt.AllCtrlAltShiftRule & CASR_VIEWER;
}

int FileViewer::ViewerControl(int Command, void *Param)
{
	_VCTLLOG(CleverSysLog SL(L"FileViewer::ViewerControl()"));
	_VCTLLOG(SysLog(L"(Command=%ls, Param=[%d/0x%08X])", _VCTL_ToName(Command), (int)Param, Param));
	return View.ViewerControl(Command, Param);
}

FARString &FileViewer::GetTitle(FARString &Title, int LenTitle, int TruncSize)
{
	return View.GetTitle(Title, LenTitle, TruncSize);
}

int64_t FileViewer::GetViewFileSize() const
{
	return View.GetViewFileSize();
}

int64_t FileViewer::GetViewFilePos() const
{
	return View.GetViewFilePos();
}

void FileViewer::ShowStatus()
{
	FARString strName;
	FARString strStatus;

	if (!IsTitleBarVisible())
		return;

	GetTitle(strName);
	int NameLength = ScrX - 43;		//???41

	if (Opt.ViewerEditorClock && IsFullScreen())
		NameLength-= 6;

	if (NameLength < 20)
		NameLength = 20;

	TruncPathStr(strName, NameLength);
	FARString str_codepage;
	ShortReadableCodepageName(View.VM.CodePage,str_codepage);
	strStatus.Format(L"%-*ls %5ls %13llu %7.7ls %-4lld %ls%3d%%", NameLength, strName.CPtr(), str_codepage.CPtr(),
			View.FileSize, Msg::ViewerStatusCol.CPtr(), View.LeftPos, Opt.ViewerEditorClock ? L"" : L" ",
			(View.LastPage ? 100 : ToPercent64(View.FilePos, View.FileSize)));
	SetFarColor(COL_VIEWERSTATUS);
	GotoXY(X1, Y1);
	FS << fmt::Cells() << fmt::LeftAlign() << fmt::Size(View.Width + (View.ViOpt.ShowScrollbar ? 1 : 0))
		<< strStatus;

	if (Opt.ViewerEditorClock && IsFullScreen()) {
		if (X2 > 5) {
			Text(X2 - 5, Y1, FarColorToReal(COL_VIEWERTEXT), L" ");
		}
		ShowTime(FALSE);
	}
}

void FileViewer::OnChangeFocus(int focus)
{
	Frame::OnChangeFocus(focus);
	CtrlObject->Plugins.CurViewer = &View;
	int FCurViewerID = View.ViewerID;
	CtrlObject->Plugins.ProcessViewerEvent(focus ? VE_GOTFOCUS : VE_KILLFOCUS, &FCurViewerID);
}

void ModalViewFile(const std::string &pathname)
{
	FileViewer Viewer(std::make_shared<FileHolder>(pathname),
		FALSE, FALSE, FALSE, -1, nullptr, nullptr, FALSE, CP_AUTODETECT);
	Viewer.SetDynamicallyBorn(false);
	FrameManager->ExecuteModalEV();
	const int r = Viewer.GetExitCode();
	if (r != 0)
		fprintf(stderr, "%s: viewer error %d for '%s'\n", __FUNCTION__, r, pathname.c_str());
}

void ViewConsoleHistory(HANDLE con_hnd, bool modal, bool autoclose)
{
	FARString histfile(CtrlObject->CmdLine->GetConsoleLog(con_hnd, true));
	if (histfile.IsEmpty())
		return;

	std::shared_ptr<TempFileHolder> tfh(std::make_shared<TempFileHolder>(histfile, false));

	FileViewer *Viewer = new (std::nothrow) FileViewer(tfh,
		!modal, TRUE, TRUE, -1, nullptr, nullptr, FALSE, CP_UTF8);
	Viewer->SetDynamicallyBorn(!modal);
	Viewer->ProcessKey(KEY_END); // scroll to the end
	if (autoclose)
		Viewer->SetAutoClose(true);
	if (modal)
		FrameManager->ExecuteModalEV();
	const int r = Viewer->GetExitCode();
	if (!r || modal)
		delete Viewer;
}
