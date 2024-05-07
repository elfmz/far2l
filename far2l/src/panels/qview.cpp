/*
qview.cpp

Quick view panel
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

#include "qview.hpp"
#include "macroopcode.hpp"
#include "lang.hpp"
#include "colors.hpp"
#include "keys.hpp"
#include "filepanels.hpp"
#include "help.hpp"
#include "viewer.hpp"
#include "cmdline.hpp"
#include "ctrlobj.hpp"
#include "interf.hpp"
#include "execute.hpp"
#include "dirinfo.hpp"
#include "pathmix.hpp"
#include "mix.hpp"
#include "constitle.hpp"
#include "syslog.hpp"

static int LastWrapMode = -1;
static int LastWrapType = -1;

QuickView::QuickView()
	:
	QView(nullptr), Directory(0), PrevMacroMode(-1)
{
	Type = QVIEW_PANEL;
	if (LastWrapMode < 0) {
		LastWrapMode = Opt.ViOpt.ViewerIsWrap;
		LastWrapType = Opt.ViOpt.ViewerWrap;
	}
}

QuickView::~QuickView()
{
	CloseFile();
	SetMacroMode(TRUE);
}

FARString &QuickView::GetTitle(FARString &strTitle, int SubLen, int TruncSize)
{
	strTitle = L" ";
	strTitle+= Msg::QuickViewTitle;
	strTitle+= L" ";
	TruncStr(strTitle, X2 - X1 - 3);
	return strTitle;
}

void QuickView::DisplayObject()
{
	if (Flags.Check(FSCROBJ_ISREDRAWING))
		return;

	Flags.Set(FSCROBJ_ISREDRAWING);
	FARString strTitle;

	if (!QView && !ProcessingPluginCommand)
		CtrlObject->Cp()->GetAnotherPanel(this)->UpdateViewPanel();

	if (QView)
		QView->SetPosition(X1 + 1, Y1 + 1, X2 - 1, Y2 - 3);

	Box(X1, Y1, X2, Y2, FarColorToReal(COL_PANELBOX), DOUBLE_BOX);
	SetScreen(X1 + 1, Y1 + 1, X2 - 1, Y2 - 1, L' ', FarColorToReal(COL_PANELTEXT));
	SetFarColor(Focus ? COL_PANELSELECTEDTITLE : COL_PANELTITLE);
	GetTitle(strTitle);

	if (!strTitle.IsEmpty()) {
		GotoXY(X1 + (X2 - X1 + 1 - (int)strTitle.GetLength()) / 2, Y1);
		Text(strTitle);
	}

	DrawSeparator(Y2 - 2);
	SetFarColor(COL_PANELTEXT);
	GotoXY(X1 + 1, Y2 - 1);
	FS << fmt::Cells() << fmt::LeftAlign() << fmt::Size(X2 - X1 - 1) << PointToName(strCurFileName);

	if (!strCurFileType.IsEmpty()) {
		FARString strTypeText = L" ";
		strTypeText+= strCurFileType;
		strTypeText+= L" ";
		TruncStr(strTypeText, X2 - X1 - 1);
		SetFarColor(COL_PANELSELECTEDINFO);
		GotoXY(X1 + (X2 - X1 + 1 - (int)strTypeText.GetLength()) / 2, Y2 - 2);
		Text(strTypeText);
	}

	if (Directory) {
		FormatString FString;
		FString << Msg::QuickViewFolder << L" \"" << strCurFileName << L"\"";
		SetFarColor(COL_PANELTEXT);
		GotoXY(X1 + 2, Y1 + 2);
		PrintText(FString);

		/*if ((apiGetFileAttributes(strCurFileName)&FILE_ATTRIBUTE_REPARSE_POINT) == FILE_ATTRIBUTE_REPARSE_POINT)
		{
			FARString strJuncName;
			DWORD ReparseTag=0;

			if (GetReparsePointInfo(strCurFileName, strJuncName,&ReparseTag))
			{
				int ID_Msg=Msg::QuickViewJunction;

				if (ReparseTag==IO_REPARSE_TAG_MOUNT_POINT)
				{
					if (IsLocalVolumeRootPath(strJuncName))
					{
						ID_Msg=Msg::QuickViewVolMount;
					}
				}
				else if (ReparseTag==IO_REPARSE_TAG_SYMLINK)
				{
					ID_Msg=Msg::QuickViewSymlink;
				}

				//"\??\D:\Junc\Src\"
				NormalizeSymlinkName(strJuncName);
				TruncPathStr(strJuncName,X2-X1-1-StrLength((ID_Msg)));
				FString.Clear();
				FString<<(ID_Msg)<<L" \""<<strJuncName<<L"\"";
				SetFarColor(COL_PANELTEXT);
				GotoXY(X1+2,Y1+3);
				PrintText(FString);
			}
		}*/

		if (Directory == 1 || Directory == 4) {
			GotoXY(X1 + 2, Y1 + 4);
			PrintText(Msg::QuickViewContains);
			GotoXY(X1 + 2, Y1 + 6);
			PrintText(Msg::QuickViewFolders);
			SetFarColor(COL_PANELINFOTEXT);
			FString.Clear();
			FString << DirCount;
			PrintText(FString);
			SetFarColor(COL_PANELTEXT);
			GotoXY(X1 + 2, Y1 + 7);
			PrintText(Msg::QuickViewFiles);
			SetFarColor(COL_PANELINFOTEXT);
			FString.Clear();
			FString << FileCount;
			PrintText(FString);
			SetFarColor(COL_PANELTEXT);
			GotoXY(X1 + 2, Y1 + 8);
			PrintText(Msg::QuickViewBytes);
			SetFarColor(COL_PANELINFOTEXT);
			FARString strSize;
			InsertCommas(FileSize, strSize);
			PrintText(strSize);
			SetFarColor(COL_PANELTEXT);
			GotoXY(X1 + 2, Y1 + 9);
			PrintText(Msg::QuickViewPhysical);
			SetFarColor(COL_PANELINFOTEXT);
			InsertCommas(PhysicalSize, strSize);
			PrintText(strSize);
			SetFarColor(COL_PANELTEXT);
			GotoXY(X1 + 2, Y1 + 10);
			PrintText(Msg::QuickViewRatio);
			SetFarColor(COL_PANELINFOTEXT);
			FString.Clear();
			FString << ToPercent64(PhysicalSize, FileSize) << L"%";
			PrintText(FString);

			if (Directory == 1 && ClusterSize) {
				SetFarColor(COL_PANELTEXT);
				GotoXY(X1 + 2, Y1 + 12);
				PrintText(Msg::QuickViewCluster);
				SetFarColor(COL_PANELINFOTEXT);
				FARString strSize;
				InsertCommas(ClusterSize, strSize);
				PrintText(strSize);
			}
		}
	} else if (QView)
		QView->Show();

	Flags.Clear(FSCROBJ_ISREDRAWING);
}

int64_t QuickView::VMProcess(MacroOpcode OpCode, void *vParam, int64_t iParam)
{
	if (!Directory && QView)
		return QView->VMProcess(OpCode, vParam, iParam);

	switch (OpCode) {
		case MCODE_C_EMPTY:
			return 1;
	}

	return 0;
}

int QuickView::ProcessKey(FarKey Key)
{
	if (!IsVisible())
		return FALSE;

	if (Key >= KEY_RCTRL0 && Key <= KEY_RCTRL9) {
		ExecShortcutFolder(Key - KEY_RCTRL0);
		return TRUE;
	}

	if (Key == KEY_F1) {
		Help::Present(L"QViewPanel");
		return TRUE;
	}

	if (Key == KEY_F3 || Key == KEY_NUMPAD5 || Key == KEY_SHIFTNUMPAD5) {
		Panel *AnotherPanel = CtrlObject->Cp()->GetAnotherPanel(this);

		if (AnotherPanel->GetType() == FILE_PANEL)
			AnotherPanel->ProcessKey(KEY_F3);

		return TRUE;
	}

	if (Key == KEY_ADD || Key == KEY_SUBTRACT) {
		Panel *AnotherPanel = CtrlObject->Cp()->GetAnotherPanel(this);

		if (AnotherPanel->GetType() == FILE_PANEL)
			AnotherPanel->ProcessKey(Key == KEY_ADD ? KEY_DOWN : KEY_UP);

		return TRUE;
	}

	if (QView && !Directory && Key >= 256) {
		int ret = QView->ProcessKey(Key);

		if (Key == KEY_F4 || Key == KEY_F8 || Key == KEY_F2 || Key == KEY_SHIFTF2) {
			DynamicUpdateKeyBar();
			CtrlObject->MainKeyBar->Redraw();
		}

		if (Key == KEY_F7 || Key == KEY_SHIFTF7) {
			// int64_t Pos;
			// int Length;
			// DWORD Flags;
			// QView->GetSelectedParam(Pos,Length,Flags);
			Redraw();
			CtrlObject->Cp()->GetAnotherPanel(this)->Redraw();
			// QView->SelectText(Pos,Length,Flags|1);
		}

		return ret;
	}

	return FALSE;
}

int QuickView::ProcessMouse(MOUSE_EVENT_RECORD *MouseEvent)
{
	int RetCode;

	if (!IsVisible())
		return FALSE;

	if (Panel::PanelProcessMouse(MouseEvent, RetCode))
		return (RetCode);

	SetFocus();

	if (QView && !Directory)
		return (QView->ProcessMouse(MouseEvent));

	return FALSE;
}

void QuickView::Update(int Mode)
{
	if (!EnableUpdate)
		return;

	if (strCurFileName.IsEmpty())
		CtrlObject->Cp()->GetAnotherPanel(this)->UpdateViewPanel();

	Redraw();
}

void QuickView::ShowFile(const wchar_t *FileName, int TempFile, HANDLE hDirPlugin)
{
	DWORD FileAttr = 0;
	CloseFile();
	QView = nullptr;

	if (!IsVisible())
		return;

	if (!FileName) {
		ProcessingPluginCommand++;
		Show();
		ProcessingPluginCommand--;
		return;
	}

	if (!hDirPlugin) {
		FileAttr = apiGetFileAttributes(FileName);
		if (FileAttr != INVALID_FILE_ATTRIBUTES && (FileAttr & FILE_ATTRIBUTE_DEVICE) != 0)		// avoid stuck
			return;
	}

	bool SameFile = !StrCmp(strCurFileName, FileName);
	strCurFileName = FileName;
	//	size_t pos;

	if (hDirPlugin || (FileAttr != INVALID_FILE_ATTRIBUTES && (FileAttr & FILE_ATTRIBUTE_DIRECTORY))) {
		//
		strCurFileType.Clear();

		if (SameFile && !hDirPlugin) {
			Directory = 1;
		} else if (hDirPlugin) {
			int ExitCode =
					GetPluginDirInfo(hDirPlugin, strCurFileName, DirCount, FileCount, FileSize, PhysicalSize);
			if (ExitCode)
				Directory = 4;
			else
				Directory = 3;
		} else {
			int ExitCode = GetDirInfo(Msg::QuickViewTitle, strCurFileName, DirCount, FileCount, FileSize,
					PhysicalSize, ClusterSize, 500, nullptr,
					GETDIRINFO_ENHBREAK | GETDIRINFO_SCANSYMLINKDEF | GETDIRINFO_DONTREDRAWFRAME);

			if (ExitCode == 1)
				Directory = 1;
			else if (ExitCode == -1)
				Directory = 2;
			else
				Directory = 3;
		}
	} else {
		if (!strCurFileName.IsEmpty()) {
			QView = new Viewer(true);
			QView->SetRestoreScreenMode(FALSE);
			QView->SetPosition(X1 + 1, Y1 + 1, X2 - 1, Y2 - 3);
			QView->SetStatusMode(0);
			QView->EnableHideCursor(0);
			OldWrapMode = QView->GetWrapMode();
			OldWrapType = QView->GetWrapType();
			QView->SetWrapMode(LastWrapMode);
			QView->SetWrapType(LastWrapType);
			QView->OpenFile(std::make_shared<FileHolder>(strCurFileName), FALSE);
		}
	}

	if (TempFile)
		ConvertNameToFull(strCurFileName, strTempName);

	Redraw();

	if (CtrlObject->Cp()->ActivePanel == this) {
		DynamicUpdateKeyBar();
		CtrlObject->MainKeyBar->Redraw();
	}
}

void QuickView::CloseFile()
{
	if (QView) {
		LastWrapMode = QView->GetWrapMode();
		LastWrapType = QView->GetWrapType();
		QView->SetWrapMode(OldWrapMode);
		QView->SetWrapType(OldWrapType);
		delete QView;
		QView = nullptr;
	}

	strCurFileType.Clear();
	QViewDelTempName();
	Directory = 0;
}

void QuickView::QViewDelTempName()
{
	if (!strTempName.IsEmpty()) {
		if (QView) {
			LastWrapMode = QView->GetWrapMode();
			LastWrapType = QView->GetWrapType();
			QView->SetWrapMode(OldWrapMode);
			QView->SetWrapType(OldWrapType);
			delete QView;
			QView = nullptr;
		}

		apiSetFileAttributes(strTempName, FILE_ATTRIBUTE_ARCHIVE);
		apiDeleteFile(strTempName);		// BUGBUG
		CutToSlash(strTempName);
		apiRemoveDirectory(strTempName);
		strTempName.Clear();
	}
}

void QuickView::PrintText(const wchar_t *Str)
{
	if (WhereY() > Y2 - 3 || WhereX() > X2 - 2)
		return;

	FS << fmt::Cells() << fmt::Truncate(X2 - 2 - WhereX() + 1) << Str;
}

int QuickView::UpdateIfChanged(int UpdateMode)
{
	if (IsVisible() && !strCurFileName.IsEmpty() && Directory == 2) {
		FARString strViewName = strCurFileName;
		ShowFile(strViewName, !strTempName.IsEmpty(), nullptr);
		return TRUE;
	}

	return FALSE;
}

void QuickView::SetTitle()
{
	if (GetFocus()) {
		FARString strTitleDir(L"{");

		if (!strCurFileName.IsEmpty()) {
			strTitleDir+= strCurFileName;
			strTitleDir+= L" - QuickView";
		} else {
			FARString strCmdText;
			CtrlObject->CmdLine->GetString(strCmdText);
			strTitleDir+= strCmdText;
		}

		strTitleDir+= L"}";

		ConsoleTitle::SetFarTitle(strTitleDir);
	}
}

void QuickView::SetFocus()
{
	Panel::SetFocus();
	SetTitle();
	SetMacroMode(FALSE);
}

void QuickView::KillFocus()
{
	Panel::KillFocus();
	SetMacroMode(TRUE);
}

void QuickView::SetMacroMode(int Restore)
{
	if (!CtrlObject)
		return;

	if (PrevMacroMode == -1)
		PrevMacroMode = CtrlObject->Macro.GetMode();

	CtrlObject->Macro.SetMode(Restore ? PrevMacroMode : MACRO_QVIEWPANEL);
}

int QuickView::GetCurName(FARString &strName)
{
	if (!strCurFileName.IsEmpty()) {
		strName = strCurFileName;
		return (TRUE);
	}

	return (FALSE);
}

BOOL QuickView::UpdateKeyBar()
{
	KeyBar *KB = CtrlObject->MainKeyBar;
	KB->SetAllGroup(KBL_MAIN, Msg::QViewF1, 12);
	KB->SetAllGroup(KBL_SHIFT, Msg::QViewShiftF1, 12);
	KB->SetAllGroup(KBL_ALT, Msg::QViewAltF1, 12);
	KB->SetAllGroup(KBL_CTRL, Msg::QViewCtrlF1, 12);
	KB->SetAllGroup(KBL_CTRLSHIFT, Msg::QViewCtrlShiftF1, 12);
	KB->SetAllGroup(KBL_CTRLALT, Msg::QViewCtrlAltF1, 12);
	KB->SetAllGroup(KBL_ALTSHIFT, Msg::QViewAltShiftF1, 12);
	KB->SetAllGroup(KBL_CTRLALTSHIFT, Msg::QViewCtrlAltShiftF1, 12);
	DynamicUpdateKeyBar();
	return TRUE;
}

void QuickView::DynamicUpdateKeyBar()
{
	KeyBar *KB = CtrlObject->MainKeyBar;

	if (Directory || !QView) {
		KB->Change(Msg::F2, 2 - 1);
		KB->Change(L"", 4 - 1);
		KB->Change(L"", 8 - 1);
		KB->Change(KBL_SHIFT, L"", 2 - 1);
		KB->Change(KBL_SHIFT, L"", 8 - 1);
		KB->Change(KBL_ALT, Msg::AltF8, 8 - 1);		//
	} else {
		if (QView->GetHexMode())
			KB->Change(Msg::ViewF4Text, 4 - 1);
		else
			KB->Change(Msg::QViewF4, 4 - 1);

		if (QView->GetCodePage() != WINPORT(GetOEMCP)())
			KB->Change(Msg::ViewF8DOS, 8 - 1);
		else
			KB->Change(Msg::QViewF8, 8 - 1);

		if (!QView->GetWrapMode()) {
			if (QView->GetWrapType())
				KB->Change(Msg::ViewShiftF2, 2 - 1);
			else
				KB->Change(Msg::ViewF2, 2 - 1);
		} else
			KB->Change(Msg::ViewF2Unwrap, 2 - 1);

		if (QView->GetWrapType())
			KB->Change(KBL_SHIFT, Msg::ViewF2, 2 - 1);
		else
			KB->Change(KBL_SHIFT, Msg::ViewShiftF2, 2 - 1);
	}

	KB->ReadRegGroup(L"QView", Opt.strLanguage);
	KB->SetAllRegGroup();
}
