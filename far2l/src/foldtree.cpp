/*
foldtree.cpp

Поиск каталога по Alt-F10
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

#include "foldtree.hpp"
#include "keyboard.hpp"
#include "keys.hpp"
#include "lang.hpp"
#include "treelist.hpp"
#include "edit.hpp"
#include "ctrlobj.hpp"
#include "filepanels.hpp"
#include "help.hpp"
#include "manager.hpp"
#include "savescr.hpp"
#include "interf.hpp"
#include "config.hpp"
#include "exitcode.hpp"

void FolderTree::Present(FARString &strResultFolder, int ModalMode, int IsStandalone, int IsFullScreen)
{
	FolderTree Tree(strResultFolder, ModalMode, IsStandalone, IsFullScreen);
}

FolderTree::FolderTree(FARString &strResultFolder, int iModalMode, int IsStandalone, int IsFullScreen)
	:
	CMM(MACRO_FINDFOLDER),
	Tree(nullptr),
	FindEdit(nullptr),
	ModalMode(iModalMode),
	IsFullScreen(IsFullScreen),
	IsStandalone(IsStandalone)
{
	SetDynamicallyBorn(FALSE);
	SetRestoreScreenMode(TRUE);
	if (ModalMode != MODALTREE_FREE)
		strResultFolder.Clear();
	KeyBarVisible = TRUE;	// Заставим обновлятся кейбар
	// TopScreen=new SaveScreen;
	SetCoords();

	if ((Tree = new (std::nothrow) TreeList(FALSE))) {
		strLastName.Clear();
		Tree->SetModalMode(ModalMode);
		Tree->SetPosition(X1, Y1, X2, Y2);

		if (ModalMode == MODALTREE_FREE)
			Tree->SetRootDir(strResultFolder);

		Tree->SetVisible(TRUE);
		Tree->Update(0);

		// если было прерывание в процессе сканирования и это было дерево копира...
		if (Tree->GetExitCode()) {
			if (!(FindEdit = new (std::nothrow) Edit)) {
				SetExitCode(XC_OPEN_ERROR);
				return;
			}

			FindEdit->SetEditBeyondEnd(FALSE);
			FindEdit->SetPersistentBlocks(Opt.Dialogs.EditBlock);
			InitKeyBar();
			FrameManager->ExecuteModal(this);	// OT
		}

		strResultFolder = strNewFolder;
	} else {
		SetExitCode(XC_OPEN_ERROR);
	}
}

FolderTree::~FolderTree()
{
	// if ( TopScreen )    delete TopScreen;
	if (FindEdit)
		delete FindEdit;

	if (Tree)
		delete Tree;
}

void FolderTree::DisplayObject()
{
	// if(!TopScreen) TopScreen=new SaveScreen;
	if (ModalMode == MODALTREE_FREE) {
		FARString strSelFolder;
		Tree->GetCurDir(strSelFolder);
		// Tree->Update(UPDATE_KEEP_SELECTION);
		Tree->Update(0);
		Tree->GoToFile(strSelFolder);
	}

	Tree->Redraw();
	Shadow();
	DrawEdit();

	if (!IsFullScreen) {
		TreeKeyBar.SetPosition(0, ScrY, ScrX, ScrY);
		TreeKeyBar.Refresh(true);
	} else
		TreeKeyBar.Refresh(false);
}

void FolderTree::SetCoords()
{
	if (IsFullScreen)
		SetPosition(0, 0, ScrX, ScrY);
	else {
		if (IsStandalone)
			SetPosition(4, 2, ScrX - 4, ScrY - 4);
		else
			SetPosition(ScrX / 3, 2, ScrX - 7, ScrY - 5);
	}
}

void FolderTree::OnChangeFocus(int focus)
{
	if (focus)
		Show();
}

void FolderTree::ResizeConsole()
{
	// if ( TopScreen )
	// delete TopScreen;
	// TopScreen=nullptr;
	Hide();
	SetCoords();
	Tree->SetPosition(X1, Y1, X2, Y2);
	// ReadHelp(StackData.HelpMask);
	FrameManager->ImmediateHide();
	FrameManager->RefreshFrame();
}

void FolderTree::SetScreenPosition()
{
	if (IsFullScreen)
		TreeKeyBar.Hide();

	SetCoords();
	Show();
}

int FolderTree::FastHide()
{
	return Opt.AllCtrlAltShiftRule & CASR_DIALOG;
}

int FolderTree::GetTypeAndName(FARString &strType, FARString &strName)
{
	strType = Msg::FolderTreeType;
	strName.Clear();
	return MODALTYPE_FINDFOLDER;
}

int FolderTree::ProcessKey(FarKey Key)
{
	if (Key >= KEY_ALT_BASE + 0x01 && Key <= KEY_ALT_BASE + 255)
		Key = Lower(Key - KEY_ALT_BASE);

	switch (Key) {
		case KEY_F1: {
			Help::Present(L"FindFolder");
		} break;
		case KEY_ESC:
		case KEY_F10:
			FrameManager->DeleteFrame();
			SetExitCode(XC_MODIFIED);
			break;
		case KEY_NUMENTER:
		case KEY_ENTER:
			Tree->GetCurDir(strNewFolder);

			if (apiGetFileAttributes(strNewFolder) != INVALID_FILE_ATTRIBUTES) {
				FrameManager->DeleteFrame();
				SetExitCode(XC_MODIFIED);
			} else {
				Tree->ProcessKey(KEY_ENTER);
				DrawEdit();
			}

			break;
		case KEY_F5:
			IsFullScreen = !IsFullScreen;
			ResizeConsole();
			return TRUE;
		case KEY_CTRLR:
		case KEY_F2:
			Tree->ProcessKey(KEY_CTRLR);
			DrawEdit();
			break;
		case KEY_CTRLNUMENTER:
		case KEY_CTRLSHIFTNUMENTER:
		case KEY_CTRLENTER:
		case KEY_CTRLSHIFTENTER: {
			FARString strName;
			FindEdit->GetString(strName);
			Tree->FindPartName(strName, TRUE,
					Key == KEY_CTRLSHIFTENTER || Key == KEY_CTRLSHIFTNUMENTER ? -1 : 1, 1);
			DrawEdit();
		} break;
		case KEY_UP:
		case KEY_NUMPAD8:
		case KEY_DOWN:
		case KEY_NUMPAD2:
		case KEY_PGUP:
		case KEY_NUMPAD9:
		case KEY_PGDN:
		case KEY_NUMPAD3:
		case KEY_HOME:
		case KEY_NUMPAD7:
		case KEY_END:
		case KEY_NUMPAD1:
		case KEY_MSWHEEL_UP:
		case (KEY_MSWHEEL_UP | KEY_ALT):
		case KEY_MSWHEEL_DOWN:
		case (KEY_MSWHEEL_DOWN | KEY_ALT):
			FindEdit->SetString(L"");
			Tree->ProcessKey(Key);
			DrawEdit();
			break;
		default:

			if (Key == KEY_ADD || Key == KEY_SUBTRACT)		// OFM: Gray+/Gray- navigation
			{
				Tree->ProcessKey(Key);
				DrawEdit();
				break;
			}

			/*
				else
				{
					if((Key&(~KEY_CTRLMASK)) == KEY_ADD)
						Key='+';
					else if((Key&(~KEY_CTRLMASK)) == KEY_SUBTRACT)
						Key='-';
				}
			*/
			if (FindEdit->ProcessKey(Key)) {
				FARString strName;
				FindEdit->GetString(strName);

				if (Tree->FindPartName(strName, FALSE, 1, 1))
					strLastName = strName;
				else {
					FindEdit->SetString(strLastName);
					strName = strLastName;
				}

				DrawEdit();
			}

			break;
	}

	return TRUE;
}

int FolderTree::ProcessMouse(MOUSE_EVENT_RECORD *MouseEvent)
{
	if (TreeKeyBar.ProcessMouse(MouseEvent))
		return TRUE;

	if (MouseEvent->dwEventFlags == DOUBLE_CLICK) {
		ProcessKey(KEY_ENTER);
		return TRUE;
	}

	int MsX = MouseEvent->dwMousePosition.X;
	int MsY = MouseEvent->dwMousePosition.Y;

	if ((MsX < X1 || MsY < Y1 || MsX > X2 || MsY > Y2) && MouseEventFlags != MOUSE_MOVED) {
		if (!(MouseEvent->dwButtonState & FROM_LEFT_1ST_BUTTON_PRESSED)
				&& (PrevMouseButtonState & FROM_LEFT_1ST_BUTTON_PRESSED)
				&& (Opt.Dialogs.MouseButton & DMOUSEBUTTON_LEFT))
			ProcessKey(KEY_ESC);
		else if (!(MouseEvent->dwButtonState & RIGHTMOST_BUTTON_PRESSED)
				&& (PrevMouseButtonState & RIGHTMOST_BUTTON_PRESSED)
				&& (Opt.Dialogs.MouseButton & DMOUSEBUTTON_RIGHT))
			ProcessKey(KEY_ENTER);

		return TRUE;
	}

	if (MsY == Y2 - 2)
		FindEdit->ProcessMouse(MouseEvent);
	else {
		if (!Tree->ProcessMouse(MouseEvent))
			SetExitCode(XC_MODIFIED);
		else
			DrawEdit();
	}

	return TRUE;
}

void FolderTree::DrawEdit()
{
	int FindY = Y2 - 2;
	const wchar_t *SearchTxt = Msg::FoldTreeSearch;
	GotoXY(X1 + 1, FindY);
	SetFarColor(COL_PANELTEXT);
	FS << SearchTxt << L"  ";
	FindEdit->SetPosition(X1 + StrLength(SearchTxt) + 2, FindY, Min(X2 - 1, X1 + 25), FindY);
	FindEdit->SetObjectColor(FarColorToReal(COL_DIALOGEDIT));
	FindEdit->Show();

	if (WhereX() < X2) {
		SetFarColor(COL_PANELTEXT);
		FS << fmt::Cells() << fmt::Expand(X2 - WhereX()) << L"";
	}
}

void FolderTree::InitKeyBar()
{
	static const wchar_t *FTreeKeysLabel[] = {L"", L"", L"", L"", L"", L"", L"", L"", L"", L"", L"", L""};
	TreeKeyBar.Set(FTreeKeysLabel, ARRAYSIZE(FTreeKeysLabel));
	TreeKeyBar.SetAlt(FTreeKeysLabel, ARRAYSIZE(FTreeKeysLabel));
	TreeKeyBar.Change(KBL_MAIN, Msg::KBFolderTreeF1, 1 - 1);
	TreeKeyBar.Change(KBL_MAIN, Msg::KBFolderTreeF2, 2 - 1);
	TreeKeyBar.Change(KBL_MAIN, Msg::KBFolderTreeF5, 5 - 1);
	TreeKeyBar.Change(KBL_MAIN, Msg::KBFolderTreeF10, 10 - 1);
	TreeKeyBar.Change(KBL_ALT, Msg::KBFolderTreeAltF9, 9 - 1);
	SetKeyBar(&TreeKeyBar);
}
