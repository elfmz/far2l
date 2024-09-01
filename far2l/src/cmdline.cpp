/*
cmdline.cpp

Командная строка
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

#include "cmdline.hpp"
#include "execute.hpp"
#include "macroopcode.hpp"
#include "keys.hpp"
#include "lang.hpp"
#include "ctrlobj.hpp"
#include "manager.hpp"
#include "message.hpp"
#include "history.hpp"
#include "filepanels.hpp"
#include "panel.hpp"
#include "foldtree.hpp"
#include "treelist.hpp"
#include "fileview.hpp"
#include "fileedit.hpp"
#include "rdrwdsk.hpp"
#include "savescr.hpp"
#include "scrbuf.hpp"
#include "interf.hpp"
#include "syslog.hpp"
#include "config.hpp"
#include "ConfigOptEdit.hpp"
#include "ConfigOptSaveLoad.hpp"
#include "usermenu.hpp"
#include "datetime.hpp"
#include "pathmix.hpp"
#include "dirmix.hpp"
#include "strmix.hpp"
#include "keyboard.hpp"
#include "vmenu.hpp"
#include "CachedCreds.hpp"
#include "exitcode.hpp"
#include "vtlog.h"
#include "vtshell.h"
#include "vtcompletor.h"
#include <limits>

#include "clipboard.hpp"
#include "farversion.h"
#include <sys/utsname.h>

CommandLine::CommandLine()
	:
	CmdStr(CtrlObject->Cp(), 0, true, CtrlObject->CmdHistory, 0,
			(Opt.CmdLine.AutoComplete ? EditControl::EC_ENABLEAUTOCOMPLETE : 0)
					| EditControl::EC_ENABLEFNCOMPLETE
					| EditControl::EC_ENABLEFNCOMPLETE_ESCAPED),
	BackgroundScreen(nullptr),
	LastCmdPartLength(-1),
	PushDirStackSize(0)
{
	CmdStr.SetEditBeyondEnd(FALSE);
	SetPersistentBlocks(Opt.CmdLine.EditBlock);
	SetDelRemovesBlocks(Opt.CmdLine.DelRemovesBlocks);
}

CommandLine::~CommandLine()
{
	if (BackgroundScreen)
		delete BackgroundScreen;
}

void CommandLine::SetVisible(bool Visible)
{
	ScreenObject::SetVisible(Visible);
	bool CurVisible;
	DWORD CurSize;
	GetCursorType(CurVisible, CurSize);
	SetCursorType(Visible, CurSize);
}

void CommandLine::SetPersistentBlocks(int Mode)
{
	CmdStr.SetPersistentBlocks(Mode);
}

void CommandLine::SetDelRemovesBlocks(int Mode)
{
	CmdStr.SetDelRemovesBlocks(Mode);
}

void CommandLine::SetAutoComplete(int Mode)
{
	if (Mode) {
		CmdStr.EnableAC(true);
	} else {
		CmdStr.DisableAC(true);
	}
}

void CommandLine::DisplayObject()
{
	_OT(SysLog(L"[%p] CommandLine::DisplayObject()", this));
	if (!IsVisible())
		return;

	FARString strTruncDir;
	GetPrompt(strTruncDir);
	TruncPathStr(strTruncDir, (X2 - X1) / 2);
	GotoXY(X1, Y1);
	SetFarColor(COL_COMMANDLINEPREFIX);
	Text(strTruncDir);
	CmdStr.SetObjectColor(FarColorToReal(COL_COMMANDLINE), FarColorToReal(COL_COMMANDLINESELECTED));
	CmdStr.SetPosition(X1 + (int)strTruncDir.CellsCount(), Y1, X2, Y2);

	CmdStr.Show();

	DrawComboBoxMark(0x2191);
}

void CommandLine::DrawComboBoxMark(wchar_t MarkChar)
{
	wchar_t MarkWz[2] = {MarkChar, 0};
	GotoXY(X2 + 1, Y1);
	SetFarColor(COL_COMMANDLINEPREFIX);
	Text(MarkWz);
}

void CommandLine::SetCurPos(int Pos, int LeftPos)
{
	CmdStr.SetLeftPos(LeftPos);
	CmdStr.SetCurPos(Pos);
	CmdStr.Redraw();
}

int64_t CommandLine::VMProcess(MacroOpcode OpCode, void *vParam, int64_t iParam)
{
	if (OpCode >= MCODE_C_CMDLINE_BOF && OpCode <= MCODE_C_CMDLINE_SELECTED)
		return CmdStr.VMProcess(OpCode - MCODE_C_CMDLINE_BOF + MCODE_C_BOF, vParam, iParam);

	if (OpCode >= MCODE_C_BOF && OpCode <= MCODE_C_SELECTED)
		return CmdStr.VMProcess(OpCode, vParam, iParam);

	if (OpCode == MCODE_V_ITEMCOUNT || OpCode == MCODE_V_CURPOS)
		return CmdStr.VMProcess(OpCode, vParam, iParam);

	if (OpCode == MCODE_V_CMDLINE_ITEMCOUNT || OpCode == MCODE_V_CMDLINE_CURPOS)
		return CmdStr.VMProcess(OpCode - MCODE_V_CMDLINE_ITEMCOUNT + MCODE_V_ITEMCOUNT, vParam, iParam);

	if (OpCode == MCODE_F_EDITOR_SEL)
		return CmdStr.VMProcess(MCODE_F_EDITOR_SEL, vParam, iParam);

	return 0;
}

void CommandLine::ProcessTabCompletion()
{
	FARString strStr;
	CmdStr.GetString(strStr);
	// show all possibilities on double tab on same input string
	const bool possibilities = !strLastCompletionCmdStr.IsEmpty() && strStr == strLastCompletionCmdStr;
	strLastCompletionCmdStr = strStr;

	if (!strStr.IsEmpty()) {
		std::string cmd = strStr.GetMB();
		VTCompletor vtc;
		if (possibilities) {
			std::vector<std::string> possibilities;
			if (vtc.GetPossibilities(cmd, possibilities) && !possibilities.empty()) {
				// fprintf(stderr, "Possibilities: ");
				CmdStr.ShowCustomCompletionList(possibilities);
			}
		} else {
			if (vtc.ExpandCommand(cmd)) {
				strStr = cmd;
				CmdStr.SetString(strStr);
				CmdStr.Show();
			}
		}
	}
}

std::string CommandLine::GetConsoleLog(HANDLE con_hnd, bool colored)
{
	bool vtshell_busy = VTShell_Busy();
	if (!vtshell_busy) {
		++ProcessShowClock;
		ShowBackground();
		Redraw();
		ScrBuf.Flush();
	}
	const std::string &histfile = VTLog::GetAsFile(con_hnd, colored);
	if (!vtshell_busy) {
		--ProcessShowClock;
		Redraw();
		ScrBuf.Flush();
	}
	return histfile;
}

void CommandLine::ChangeDirFromHistory(bool PluginPath, int SelectType, FARString strDir, FARString strFile)
{
	if (SelectType == 2)
		CtrlObject->FolderHistory->SetAddMode(false, 2, true);

	// пусть плагин сам прыгает... ;-)
	Panel *Panel = CtrlObject->Cp()->ActivePanel;

	if (SelectType == 6)
		Panel = CtrlObject->Cp()->GetAnotherPanel(Panel);

	if (!PluginPath || !CtrlObject->Plugins.ProcessCommandLine(strDir, Panel)) {
		if (Panel->GetMode() == PLUGIN_PANEL || CheckShortcutFolder(&strDir, FALSE)) {
			Panel->SetCurDir(strDir, PluginPath ? FALSE : TRUE);
			//fprintf(stderr, "=== ChangeDirFromHistory():\n  strDir=\"%ls\"\n  strFile=\"%ls\"\n", strDir.CPtr(), strFile.CPtr());
			if ( !strFile.IsEmpty() && !strFile.Contains(LGOOD_SLASH) ) // only local file, not in another directory
				Panel->GoToFile(strFile);
			// restore current directory to active panel path
			if (SelectType == 6) {
				CtrlObject->Cp()->ActivePanel->SetCurPath();
			}
			Panel->Redraw();
			CtrlObject->FolderHistory->SetAddMode(true, 2, true);
		}
	}
}

void CommandLine::ProcessKey_ClearTerminalHistory()
{
	if (!Opt.Confirm.ClearVT || Message(MSG_WARNING, 2,
			Msg::ClearTerminalTitle, Msg::ClearTerminalQuestion, Msg::Ok, Msg::Cancel) == 0) {
		ClearScreen(FarColorToReal(COL_COMMANDLINEUSERSCREEN));
		SaveBackground();
		VTLog::Reset(NULL);
		ShowBackground();
		Redraw();
		//		ShellUpdatePanels(CtrlObject->Cp()->ActivePanel, FALSE);
		CtrlObject->MainKeyBar->Refresh(Opt.ShowKeyBar);
		//		CmdExecute(L"reset", true, false, true, false, false, false);
	}
}

void CommandLine::ProcessKey_ShowFolderTree()
{
	Panel *ActivePanel = CtrlObject->Cp()->ActivePanel;
	// TODO: здесь можно добавить проверку, что мы в корне диска и отсутствие файла Tree.Far...
	FARString strStr;
	FolderTree::Present(strStr, MODALTREE_ACTIVE, TRUE, FALSE);
	CtrlObject->Cp()->RedrawKeyBar();

	if (!strStr.IsEmpty()) {
		ActivePanel->SetCurDir(strStr, TRUE);
		ActivePanel->Show();
		if (ActivePanel->GetType() == TREE_PANEL)
			ActivePanel->ProcessKey(KEY_ENTER);
	} else {
		// TODO: ... а здесь проверить факт изменения/появления файла Tree.Far и мы опять же в корне (чтобы лишний раз не апдейтить панель)
		ActivePanel->Update(UPDATE_KEEP_SELECTION);
		ActivePanel->Redraw();
		Panel *AnotherPanel = CtrlObject->Cp()->GetAnotherPanel(ActivePanel);
		if (AnotherPanel->NeedUpdatePanel(ActivePanel)) {
			AnotherPanel->Update(UPDATE_KEEP_SELECTION);	//|UPDATE_SECONDARY);
			AnotherPanel->Redraw();
		}
	}
}

void CommandLine::ProcessKey_ShowFolderHistory()
{
	FARString strStr;
	int Type;
	int SelectType = CtrlObject->FolderHistory->Select(Msg::FolderHistoryTitle, L"HistoryFolders", strStr, Type);
	/* SelectType =
		0 - Esc
		1 - Enter
		2 - Shift-Enter
		3 - Ctrl-Enter
		6 - Ctrl-Shift-Enter - на пассивную панель со сменой позиции
	*/
	switch (SelectType) {
		case 1: case 2: case 6: ChangeDirFromHistory(Type == 1, SelectType, strStr); break;
		case 3: SetString(strStr); break;
	}
}

void CommandLine::ProcessKey_ShowCommandsHistory()
{
	FARString strStr;
	int Type;
	// $ 19.09.2000 SVS - При выборе из History (по Alt-F8) плагин не получал управление!
	int SelectType = CtrlObject->CmdHistory->Select(Msg::HistoryTitle, L"History", strStr, Type);
	// BUGBUG, magic numbers
	if (SelectType == 8) {
		size_t p1 = 0;
		size_t p2 = 0;
		//fprintf(stderr, "=== Alt-F8: strStr=\"%ls\"\n", strStr.CPtr());
		if (strStr.Pos(p1, L'\n')) {
			strStr.Pos(p2, L'\n', p1 + 1);
			//fprintf(stderr, "=== Alt-F8: p1=%lu p2=%lu\n", p1, p2);
			ChangeDirFromHistory(Type == 1, 1, strStr.SubStr(0, p1),
				strStr.SubStr(p1 + 1, p2 > 0 ? p2 - p1 - 1 : -1) );
			if( p2 > 0 ) {
				strStr.Remove(0, p2 + 1);
				SetString(strStr);
			}
		} else {
			ChangeDirFromHistory(Type == 1, 1, strStr);
		}

	} else if ((SelectType > 0 && SelectType <= 3) || SelectType == 7) {
		if (SelectType < 3 || SelectType == 7) {
			CmdStr.DisableAC();
		}
		SetString(strStr);
		if (SelectType < 3 || SelectType == 7) {
			ProcessKey(SelectType == 7 ? static_cast<int>(KEY_CTRLALTENTER)
									: (SelectType == 1 ? static_cast<int>(KEY_ENTER)
														: static_cast<int>(KEY_SHIFTENTER)));
			CmdStr.RevertAC();
		}
	}
}

int CommandLine::ProcessKey_Enter(FarKey Key)
{
	Panel *ActivePanel = CtrlObject->Cp()->ActivePanel;
	FARString strStr;
	CmdStr.Select(-1, 0);
	CmdStr.Show();
	CmdStr.GetString(strStr);

	if (strStr.IsEmpty())
		return FALSE;

	ActivePanel->SetCurPath();

	FARString strCurDirFromPanel;
	ActivePanel->GetCurDirPluginAware(strCurDirFromPanel);

	if (!(Opt.ExcludeCmdHistory & EXCLUDECMDHISTORY_NOTCMDLINE)) {
		CtrlObject->CmdHistory->AddToHistoryExtra(strStr, strCurDirFromPanel);
	}

	Redraw();
	if ( ProcessFarCommands(strStr.CPtr()) ) {
		CmdStr.SetString(L"", FALSE);
		Show();
		return TRUE;
	}

	// ProcessOSAliases(strStr);

	if (ActivePanel->ProcessPluginEvent(FE_COMMAND, (void *)strStr.CPtr())) {
		strCurDir = strCurDirFromPanel;
		Show();
		ActivePanel->SetTitle();

	} else {
		CmdExecute(strStr, Key == KEY_SHIFTENTER || Key == KEY_SHIFTNUMENTER, false, false, false,
				Key == KEY_CTRLALTENTER || Key == KEY_CTRLALTNUMENTER);
	}
	return TRUE;
}

int CommandLine::ProcessKey(FarKey Key)
{
	switch (Key) {
		case KEY_MSWHEEL_UP | KEY_CTRL | KEY_SHIFT:
			ViewConsoleHistory(NULL, false, true);
			return TRUE;
		case KEY_CTRLSHIFTF3: case KEY_F3:
			ViewConsoleHistory(NULL, false, false);
			return TRUE;
		case KEY_CTRLSHIFTF4: case KEY_F4:
			EditConsoleHistory(NULL, false);
			return TRUE;
		case KEY_F8:
			ProcessKey_ClearTerminalHistory();
			return TRUE;
		case KEY_F2:
			UserMenu::Present(false);
			return TRUE;
		case KEY_SHIFTF9:
			ConfigOptSave(true);
			return TRUE;
		case KEY_F10:
			FrameManager->ExitMainLoop(TRUE);
			return TRUE;
		case KEY_ALTF10:
			ProcessKey_ShowFolderTree();
			return TRUE;
		case KEY_F11:
			CtrlObject->Plugins.CommandsMenu(FALSE, FALSE, 0);
			return TRUE;
		case KEY_ALTF11:
			ShowViewEditHistory();
			CtrlObject->Cp()->Redraw();
			return TRUE;
		case KEY_ALTF12:
			ProcessKey_ShowFolderHistory();
			return TRUE;

		case KEY_ALTF8:
			ProcessKey_ShowCommandsHistory();
			return TRUE;
	}

	if (!IsVisible())
		return FALSE;

	return ProcessKeyIfVisible(Key);

}

int CommandLine::ProcessKeyIfVisible(FarKey Key)
{ // this handles key events only when CmdLine is visible
	switch (Key) {
		case KEY_NUMENTER:
		case KEY_SHIFTNUMENTER:
		case KEY_ENTER:
		case KEY_SHIFTENTER:
		case KEY_CTRLALTENTER:
		case KEY_CTRLALTNUMENTER:
			return ProcessKey_Enter(Key);

		case KEY_CTRLEND: case KEY_CTRLNUMPAD1:
			if (CmdStr.GetCurPos() == CmdStr.GetLength()) {
				if (LastCmdPartLength == -1)
					strLastCmdStr = CmdStr.GetStringAddr();

				FARString strStr = strLastCmdStr;
				int CurCmdPartLength = (int)strStr.GetLength();
				CtrlObject->CmdHistory->GetSimilar(strStr, LastCmdPartLength);

				if (LastCmdPartLength == -1) {
					strLastCmdStr = CmdStr.GetStringAddr();
					LastCmdPartLength = CurCmdPartLength;
				}
				CmdStr.DisableAC();
				CmdStr.SetString(strStr);
				CmdStr.Select(LastCmdPartLength, static_cast<int>(strStr.GetLength()));
				CmdStr.RevertAC();
				Show();
				return TRUE;
			}
			break;

		case KEY_TAB: case KEY_SHIFTTAB:
			ProcessTabCompletion();
			return TRUE;
	}

	if (Key != KEY_NONE) {
		strLastCompletionCmdStr.Clear();
	}

	if (Key == KEY_UP || Key == KEY_NUMPAD8) {
		if (CtrlObject->Cp()->LeftPanel->IsVisible() || CtrlObject->Cp()->RightPanel->IsVisible())
			return FALSE;

		Key = KEY_CTRLE;
	} else if (Key == KEY_DOWN || Key == KEY_NUMPAD2) {
		if (CtrlObject->Cp()->LeftPanel->IsVisible() || CtrlObject->Cp()->RightPanel->IsVisible())
			return FALSE;

		Key = KEY_CTRLX;
	}

	// $ 25.03.2002 VVM + При погашенных панелях колесом крутим историю
	if (!CtrlObject->Cp()->LeftPanel->IsVisible() && !CtrlObject->Cp()->RightPanel->IsVisible()) {
		switch (Key) {
			case KEY_MSWHEEL_UP:
				Key = KEY_CTRLE;
				break;
			case KEY_MSWHEEL_DOWN:
				Key = KEY_CTRLX;
				break;
			case KEY_MSWHEEL_LEFT:
				Key = KEY_CTRLS;
				break;
			case KEY_MSWHEEL_RIGHT:
				Key = KEY_CTRLD;
				break;
		}
	}

	FARString strStr;
	const wchar_t *PStr;
	switch (Key) {
		case KEY_CTRLE:
		case KEY_CTRLX:
			if (Key == KEY_CTRLE) {
				CtrlObject->CmdHistory->GetPrev(strStr);
			} else {
				CtrlObject->CmdHistory->GetNext(strStr);
			}
			CmdStr.DisableAC();
			SetString(strStr);
			CmdStr.RevertAC();
			return TRUE;

		case KEY_ESC:
			if (Key == KEY_ESC) {
				// $ 24.09.2000 SVS - Если задано поведение по "Несохранению при Esc", то позицию в хистори не меняем и ставим в первое положение.
				if (Opt.CmdHistoryRule)
					CtrlObject->CmdHistory->ResetPosition();

				PStr = L"";
			}

			SetString(PStr);
			return TRUE;
		case KEY_CTRLU:
			CmdStr.Select(-1, 0);
			CmdStr.Show();
			return TRUE;
		case KEY_OP_XLAT: {
			// 13.12.2000 SVS - ! Для CmdLine - если нет выделения, преобразуем всю строку (XLat)
			CmdStr.Xlat(Opt.XLat.Flags & XLAT_CONVERTALLCMDLINE ? TRUE : FALSE);

			// иначе неправильно работает ctrl-end
			strLastCmdStr = CmdStr.GetStringAddr();
			LastCmdPartLength = (int)strLastCmdStr.GetLength();

			return TRUE;
		}
		/*
			дополнительные клавиши для выделения в ком строке.
			ВНИМАНИЕ!
			Для сокращения кода этот кусок должен стоять перед "default"
		*/
		case KEY_ALTSHIFTLEFT:
		case KEY_ALTSHIFTNUMPAD4:
		case KEY_ALTSHIFTRIGHT:
		case KEY_ALTSHIFTNUMPAD6:
		case KEY_ALTSHIFTEND:
		case KEY_ALTSHIFTNUMPAD1:
		case KEY_ALTSHIFTHOME:
		case KEY_ALTSHIFTNUMPAD7:
			Key&= ~KEY_ALT;
		default:

			// Сбрасываем выделение на некоторых клавишах
			if (!Opt.CmdLine.EditBlock) {
				static FarKey UnmarkKeys[] = {KEY_LEFT, KEY_NUMPAD4, KEY_CTRLS, KEY_RIGHT, KEY_NUMPAD6,
						KEY_CTRLD, KEY_CTRLLEFT, KEY_CTRLNUMPAD4, KEY_CTRLRIGHT, KEY_CTRLNUMPAD6,
						KEY_CTRLHOME, KEY_CTRLNUMPAD7, KEY_CTRLEND, KEY_CTRLNUMPAD1, KEY_HOME, KEY_NUMPAD7,
						KEY_END, KEY_NUMPAD1};

				for (size_t I = 0; I < ARRAYSIZE(UnmarkKeys); I++)
					if (Key == UnmarkKeys[I]) {
						CmdStr.Select(-1, 0);
						break;
					}
			}

			if (Key == KEY_CTRLD)
				Key = KEY_RIGHT;

			if (!CmdStr.ProcessKey(Key))
				break;

			LastCmdPartLength = -1;

			if (Key == KEY_CTRLSHIFTEND || Key == KEY_CTRLSHIFTNUMPAD1) {
				CmdStr.EnableAC();
				CmdStr.AutoComplete(true, false);
				CmdStr.RevertAC();
			}

			return TRUE;
	}

	return FALSE;
}

BOOL CommandLine::SetCurDir(const wchar_t *CurDir)
{
	if (StrCmp(strCurDir, CurDir) || !TestCurrentDirectory(CurDir)) {
		strCurDir = CurDir;

		if (CtrlObject->Cp()->ActivePanel->GetMode() != PLUGIN_PANEL)
			PrepareDiskPath(strCurDir);
	}

	return TRUE;
}

int CommandLine::GetCurDir(FARString &strCurDir)
{
	strCurDir = CommandLine::strCurDir;
	return (int)strCurDir.GetLength();
}

void CommandLine::SetString(const wchar_t *Str, BOOL Redraw)
{
	if (!IsVisible())
		return;

	LastCmdPartLength = -1;
	CmdStr.SetString(Str);
	CmdStr.SetLeftPos(0);
	if (Redraw)
		CmdStr.Show();
}

void CommandLine::ExecString(const wchar_t *Str, bool SeparateWindow, bool DirectRun, bool WaitForIdle,
		bool Silent, bool RunAs)
{
	CmdStr.DisableAC();
	SetString(Str);
	CmdStr.RevertAC();
	CmdExecute(Str, SeparateWindow, DirectRun, WaitForIdle, Silent, RunAs);
}

void CommandLine::InsertString(const wchar_t *Str)
{
	if (!IsVisible())
		return;

	LastCmdPartLength = -1;
	CmdStr.InsertString(Str);
	CmdStr.Show();
}

int CommandLine::ProcessMouse(MOUSE_EVENT_RECORD *MouseEvent)
{
	if (MouseEvent->dwButtonState & FROM_LEFT_1ST_BUTTON_PRESSED && MouseEvent->dwMousePosition.X == X2 + 1) {
		return ProcessKey(KEY_ALTF8);
	}
	int r = CmdStr.ProcessMouse(MouseEvent);
	if (r == 0) {
		if (MouseEvent->dwButtonState & FROM_LEFT_1ST_BUTTON_PRESSED) {
			WINPORT(BeginConsoleAdhocQuickEdit)();
		}
	}
	return r;
}

void CommandLine::GetPrompt(FARString &strDestStr)
{
	FARString strExpandedFormatStr;
	if (Opt.CmdLine.UsePromptFormat) {
		FARString strFormatStr;
		strFormatStr = Opt.CmdLine.strPromptFormat;
		apiExpandEnvironmentStrings(strFormatStr, strExpandedFormatStr);

	} else {	// default prompt
		strExpandedFormatStr = "$p$# ";
	}

	constexpr wchar_t ChrFmt[][2] = {
			{L'A', L'&'},		// $A - & (Ampersand)
			{L'B', L'|'},		// $B - | (pipe)
			{L'C', L'('},		// $C - ( (Left parenthesis)
			{L'F', L')'},		// $F - ) (Right parenthesis)
			{L'G', L'>'},		// $G - > (greater-than sign)
			{L'L', L'<'},		// $L - < (less-than sign)
			{L'Q', L'='},		// $Q - = (equal sign)
			{L'S', L' '},		// $S - (space)
			{L'$', L'$'},		// $$ - $ (dollar sign)
	};

	const wchar_t *Format = strExpandedFormatStr;
	while (*Format) {
		if (*Format == L'$') {
			wchar_t Chr = Upper(*++Format);
			size_t I;

			for (I = 0; I < ARRAYSIZE(ChrFmt); ++I) {
				if (ChrFmt[I][0] == Chr) {
					strDestStr+= ChrFmt[I][1];
					break;
				}
			}

			if (I == ARRAYSIZE(ChrFmt)) {
				switch (Chr) {
						/*
						эти не раелизованы
						$E - Escape code (ASCII code 27)
						$V - Windows XP version number
						$_ - Carriage return and linefeed
						$M - Отображение полного имени удаленного диска, связанного с именем текущего диска, или пустой строки, если текущий диск не является сетевым.
						*/
					case L'+':		// $+ - Отображение нужного числа знаков плюс (+) в зависимости от текущей глубины стека каталогов PUSHD, по одному знаку на каждый сохраненный путь.
					{
						if (PushDirStackSize) {
							strDestStr.Append(L'+', PushDirStackSize);
						}

						break;
					}
					case L'H':		// $H - Backspace (erases previous character)
					{
						if (!strDestStr.IsEmpty())
							strDestStr.Truncate(strDestStr.GetLength() - 1);

						break;
					}
					case L'@':		// $@xx - Admin
					{
						wchar_t lb = *++Format;
						wchar_t rb = *++Format;
						if (Opt.IsUserAdmin) {
							strDestStr+= lb;
							strDestStr+= Msg::ConfigCmdlinePromptFormatAdmin;
							strDestStr+= rb;
						}
						break;
					}
					case L'D':		// $D - Current date
					case L'T':		// $T - Current time
					{
						FARString strDateTime;
						MkStrFTime(strDateTime, (Chr == L'D' ? L"%D" : L"%T"));
						strDestStr+= strDateTime;
						break;
					}
					case L'R':		// $R - Current drive and path, always full
					{
						strDestStr+= strCurDir;
						break;
					}
					case L'P':		// $P - Current drive and path, shortened home
					{
						const auto &strHome = CachedHomeDir();
						if (strCurDir.Begins(strHome)) {
							strDestStr+= L'~';
							strDestStr+= strCurDir.CPtr() + strHome.GetLength();
						} else {
							strDestStr+= strCurDir;
						}
						break;
					}
					case L'#':		// # or $ - depending of user root or not
					{
						strDestStr+= Opt.IsUserAdmin ? L"#" : L"$";
						break;
					}
					case L'U':		// User name
					{
						strDestStr+= CachedUserName();
						break;
					}
					case L'N':		// Host name
					{
						strDestStr+= CachedComputerName();
						break;
					}
				}
			}

			Format++;
		} else {
			strDestStr+= *(Format++);
		}
	}
}

void CommandLine::ShowViewEditHistory()
{
	FARString strStr;
	int Type;
	int SelectType = CtrlObject->ViewHistory->Select(Msg::ViewHistoryTitle, L"HistoryViews", strStr, Type);
	/*
	SelectType =
		0 - Esc
		1 - Enter
		2 - Shift-Enter
		3 - Ctrl-Enter
		9 - Ctrl-F10 - jump in panel to directory & file
	*/

	if (SelectType == 1 || SelectType == 2) {
		if (SelectType != 2)
			CtrlObject->ViewHistory->AddToHistory(strStr, Type);

		CtrlObject->ViewHistory->SetAddMode(false, 1, true);

		switch (Type) {
			case 0:		// вьювер
			{
				new FileViewer(std::make_shared<FileHolder>(strStr), TRUE);
				break;
			}
			case 1:		// обычное открытие в редакторе
			case 4:		// открытие с локом
			{
				// пусть файл создается
				FileEditor *FEdit =
						new FileEditor(std::make_shared<FileHolder>(strStr), CP_AUTODETECT, FFILEEDIT_CANNEWFILE | FFILEEDIT_ENABLEF6);

				if (Type == 4)
					FEdit->SetLockEditor(TRUE);

				break;
			}
			// 2 и 3 - заполняется в ProcessExternal
			case 2:
			case 3: {
				if (strStr.At(0) != L'@') {
					ExecString(strStr);
					if (Type > 2) {
						WaitForClose(strStr.CPtr());
					}
				} else {
					SaveScreen SaveScr;
					CtrlObject->Cp()->LeftPanel->CloseFile();
					CtrlObject->Cp()->RightPanel->CloseFile();
					Execute(strStr.CPtr() + 1);
					if (Type > 2) {
						WaitForClose(strStr.CPtr() + 1);
					}
				}

				break;
			}
		}

		CtrlObject->ViewHistory->SetAddMode(true, 1, true);
	}
	else if (SelectType == 3)		// скинуть из истории в ком.строку?
		SetString(strStr);
	else if (SelectType == 9)		// goto in panel to directory & file
		CtrlObject->Cp()->GoToFile(strStr);
}

void CommandLine::SaveBackground(int X1, int Y1, int X2, int Y2)
{
	if (BackgroundScreen) {
		delete BackgroundScreen;
	}

	BackgroundScreen = new SaveScreen(X1, Y1, X2, Y2);
}

void CommandLine::SaveBackground()
{
	if (BackgroundScreen) {
		//		BackgroundScreen->Discard();
		BackgroundScreen->SaveArea();
		fprintf(stderr, "CommandLine::SaveBackground: done\n");
	} else
		fprintf(stderr, "CommandLine::SaveBackground: no BackgroundScreen\n");
}
void CommandLine::ShowBackground()
{
	if (!IsVisible())
		return;

	if (BackgroundScreen) {
		BackgroundScreen->RestoreArea();
		fprintf(stderr, "CommandLine::ShowBackground: done\n");
	} else
		fprintf(stderr, "CommandLine::ShowBackground: no BackgroundScreen\n");
}

void CommandLine::CorrectRealScreenCoord()
{
	if (BackgroundScreen) {
		BackgroundScreen->CorrectRealScreenCoord();
	}
}

void CommandLine::Show()
{
	if (IsVisible())
		ScreenObject::Show();
}

void CommandLine::ResizeConsole()
{
	BackgroundScreen->Resize(ScrX + 1, ScrY + 1, 2, FALSE);
	//	this->DisplayObject();
}

void CommandLine::RedrawWithoutComboBoxMark()
{
	if (!IsVisible())
		return;

	Redraw();
	// erase \x2191 character...
	DrawComboBoxMark(L' ');
}

void FarAbout(PluginManager &Plugins)
{
	int npl;
	FARString fs, fs2, fs2copy;
	MenuItemEx mi, mis;
	mis.Flags = LIF_SEPARATOR;

	VMenu ListAbout(L"far:about",nullptr,0,ScrY-4);
	ListAbout.SetFlags(VMENU_SHOWAMPERSAND | VMENU_IGNORE_SINGLECLICK);
	ListAbout.ClearFlags(VMENU_MOUSEREACTION);
	//ListAbout.SetFlags(VMENU_WRAPMODE);
	ListAbout.SetHelp(L"SpecCmd");//L"FarAbout");
	ListAbout.SetBottomTitle(L"ESC or F10 to close, Ctrl-C or Ctrl-Ins - copy all, Ctrl-Alt-F - filtering");

	fs.Format(L"          FAR2L Version: %s", FAR_BUILD);
	ListAbout.AddItem(fs); fs2copy = fs;
	fs =      L"               Compiler: ";
#if defined (__clang__)
	fs.AppendFormat(L"Clang, version %d.%d.%d", __clang_major__, __clang_minor__, __clang_patchlevel__);
#elif defined (__INTEL_COMPILER)
	fs.AppendFormat(L"Intel C/C++, version %d (build date %d)", __INTEL_COMPILER, __INTEL_COMPILER_BUILD_DATE);
#elif defined (__GNUC__)
	fs.AppendFormat(L"GCC, version %d.%d.%d", __GNUC__, __GNUC_MINOR__, __GNUC_PATCHLEVEL__);
#else
	fs.Append(L"Unknown");
#endif
	ListAbout.AddItem(fs); fs2copy += "\n" + fs;

	fs.Format(L"               Platform: %s", FAR_PLATFORM);
	ListAbout.AddItem(fs); fs2copy += "\n" + fs;
	fs.Format(L"                Backend: %ls", WinPortBackend());
	ListAbout.AddItem(fs); fs2copy += "\n" + fs;
	fs.Format(L"    ConsoleColorPalette: %u", WINPORT(GetConsoleColorPalette)(NULL) );
	ListAbout.AddItem(fs); fs2copy += "\n" + fs;
	fs.Format(L"                  Admin: %ls", Opt.IsUserAdmin ? Msg::FarTitleAddonsAdmin : L"-");
	ListAbout.AddItem(fs); fs2copy += "\n" + fs;
	//apiGetEnvironmentVariable("FARPID", fs2);
	//fs = L"           PID: " + fs2;
	fs.Format(L"                    PID: %lu", (unsigned long)getpid());
	ListAbout.AddItem(fs); fs2copy += "\n" + fs;

	//apiGetEnvironmentVariable("FARLANG", fs2);
	fs =      L"  Main | Help languages: " + Opt.strLanguage + L" | " + Opt.strHelpLanguage;
	ListAbout.AddItem(fs); fs2copy += "\n" + fs;

	fs.Format(L"   OEM | ANSI codepages: %u | %u", WINPORT(GetOEMCP)(), WINPORT(GetACP)() );
	ListAbout.AddItem(fs); fs2copy += "\n" + fs;

	//apiGetEnvironmentVariable("FARHOME", fs2);
	fs =      L"Far directory (FARHOME): \"" + g_strFarPath.GetMB() + L"\"";
	ListAbout.AddItem(fs); fs2copy += "\n" + fs;

	fs.Format(L"       Config directory: \"%s\"", InMyConfig("",FALSE).c_str() );
	ListAbout.AddItem(fs); fs2copy += "\n" + fs;

	fs.Format(L"        Cache directory: \"%s\"", InMyCache("",FALSE).c_str() );
	ListAbout.AddItem(fs); fs2copy += "\n" + fs;

	fs.Format(L"         Temp directory: \"%s\"", InMyTemp("").c_str() );
	ListAbout.AddItem(fs); fs2copy += "\n" + fs;

	ListAbout.AddItem(L""); fs2copy += "\n";
	struct utsname un;
	fs =      L"                  uname: ";
	if (uname(&un)==0)
		fs.AppendFormat(L"%s %s %s %s", un.sysname, un.release, un.version, un.machine);
	ListAbout.AddItem(fs); fs2copy += "\n" + fs;
	fs =      L"                   Host: " + (apiGetEnvironmentVariable("HOSTNAME", fs2) ? fs2 : L"???");
	ListAbout.AddItem(fs); fs2copy += "\n" + fs;
	fs =      L"                   User: " + (apiGetEnvironmentVariable("USER", fs2) ? fs2 : L"???");
	ListAbout.AddItem(fs); fs2copy += "\n" + fs;
	fs =      L"       XDG_SESSION_TYPE: " + (apiGetEnvironmentVariable("XDG_SESSION_TYPE", fs2) ? fs2 : L"");
	ListAbout.AddItem(fs); fs2copy += "\n" + fs;
	fs =      L"                   TERM: " + (apiGetEnvironmentVariable("TERM", fs2) ? fs2 : L"");
	ListAbout.AddItem(fs); fs2copy += "\n" + fs;
	fs =      L"              COLORTERM: " + (apiGetEnvironmentVariable("COLORTERM", fs2) ? fs2 : L"");
	ListAbout.AddItem(fs); fs2copy += "\n" + fs;
	fs =      L"            GDK_BACKEND: " + (apiGetEnvironmentVariable("GDK_BACKEND", fs2) ? fs2 : L"");
	ListAbout.AddItem(fs); fs2copy += "\n" + fs;
	fs =      L"        DESKTOP_SESSION: " + (apiGetEnvironmentVariable("DESKTOP_SESSION", fs2) ? fs2 : L"");
	ListAbout.AddItem(fs); fs2copy += "\n" + fs;
	fs =      L"        WSL_DISTRO_NAME: " + (apiGetEnvironmentVariable("WSL_DISTRO_NAME", fs2) ? fs2 : L"");
	ListAbout.AddItem(fs); fs2copy += "\n" + fs;
	fs =      L"  WSL2_GUI_APPS_ENABLED: " + (apiGetEnvironmentVariable("WSL2_GUI_APPS_ENABLED", fs2) ? fs2 : L"");
	ListAbout.AddItem(fs); fs2copy += "\n" + fs;

	ListAbout.AddItem(L""); fs2copy += "\n";

	npl = Plugins.GetPluginsCount();
	fs.Format(L"      Number of plugins: %d", npl);
	ListAbout.AddItem(fs); fs2copy += "\n" + fs;

	for(int i = 0; i < npl; i++)
	{
		fs.Format(L"Plugin#%02d ",  i+1);
		mis.strName = fs;
		mi.PrefixLen = fs.GetLength()-1;

		Plugin *pPlugin = Plugins.GetPlugin(i);
		if(pPlugin == nullptr) {
			ListAbout.AddItem(&mis); fs2copy += "\n--- " + mis.strName + " ---";
			mi.strName = fs + L"!!! ERROR get plugin";
			ListAbout.AddItem(&mi); fs2copy += "\n" + mi.strName;
			continue;
		}
		mis.strName = fs + PointToName(pPlugin->GetModuleName());
		ListAbout.AddItem(&mis); fs2copy += "\n--- " + mis.strName + " ---";

		mi.strName = fs + pPlugin->GetModuleName();
		ListAbout.AddItem(&mi); fs2copy += "\n" + mi.strName;

		mi.strName = fs + L"Settings Name: " + pPlugin->GetSettingsName();
		ListAbout.AddItem(&mi); fs2copy += "\n" + mi.strName;

		int iFlags;
		PluginInfo pInfo{};
		KeyFileReadHelper kfh(PluginsIni());
		FARString fsCommandPrefix = L"";
		FARString fsDiskMenuStrings = L"";
		FARString fsPluginMenuStrings = L"";
		FARString fsPluginConfigStrings = L"";

		if (pPlugin->CheckWorkFlags(PIWF_CACHED)) {
			iFlags = kfh.GetUInt(pPlugin->GetSettingsName(), "Flags", 0);
			fsCommandPrefix = kfh.GetString(pPlugin->GetSettingsName(), "CommandPrefix", L"");
			for (int j = 0; ; j++) {
				const auto &key_name = StrPrintf("DiskMenuString%d", j);
				/*if (!kfh.HasKey(key_name))
					break;*/
				fs2 = kfh.GetString(pPlugin->GetSettingsName(), key_name, "");
				if( fs2.IsEmpty() )
					break;
				fsDiskMenuStrings.AppendFormat(L" %d=\"%ls\" ", j+1, fs2.CPtr());
			}
			for (int j = 0; ; j++) {
				const auto &key_name = StrPrintf("PluginMenuString%d", j);
				/*if (!kfh.HasKey(key_name))
					break;*/
				fs2 = kfh.GetString(pPlugin->GetSettingsName(), key_name, "");
				if( fs2.IsEmpty() )
					break;
				fsPluginMenuStrings.AppendFormat(L" %d=\"%ls\" ", j+1, fs2.CPtr());
			}
			for (int j = 0; ; j++) {
				const auto &key_name = StrPrintf("PluginConfigString%d", j);
				/*if (!kfh.HasKey(key_name))
					break;*/
				fs2 = kfh.GetString(pPlugin->GetSettingsName(), key_name, "");
				if( fs2.IsEmpty() )
					break;
				fsPluginConfigStrings.AppendFormat(L" %d=\"%ls\" ", j+1, fs2.CPtr());
			}
		}
		else {
			if (pPlugin->GetPluginInfo(&pInfo)) {
				iFlags = pInfo.Flags;
				fsCommandPrefix = pInfo.CommandPrefix;
				for (int j = 0; j < pInfo.DiskMenuStringsNumber; j++)
					fsDiskMenuStrings.AppendFormat(L" %d=\"%ls\"", j+1, pInfo.DiskMenuStrings[j]);
				for (int j = 0; j < pInfo.PluginMenuStringsNumber; j++)
					fsPluginMenuStrings.AppendFormat(L" %d=\"%ls\"", j+1, pInfo.PluginMenuStrings[j]);
				for (int j = 0; j < pInfo.PluginConfigStringsNumber; j++)
					fsPluginConfigStrings.AppendFormat(L" %d=\"%ls\"", j+1, pInfo.PluginConfigStrings[j]);
			}
			else
				iFlags = -1;
		}

		mi.strName.Format(L"%ls     %s Cached  %s Loaded ",
			fs.CPtr(),
			pPlugin->CheckWorkFlags(PIWF_CACHED) ? "[x]" : "[ ]",
			pPlugin->GetFuncFlags() & PICFF_LOADED ? "[x]" : "[ ]");
		ListAbout.AddItem(&mi); fs2copy += "\n" + mi.strName;

		if (iFlags >= 0) {
			mi.strName.Format(L"%lsF11: %s Panel   %s Dialog  %s Viewer  %s Editor ",
				fs.CPtr(),
				iFlags & PF_DISABLEPANELS ? "[ ]" : "[x]",
				iFlags & PF_DIALOG ? "[x]" : "[ ]",
				iFlags & PF_VIEWER ? "[x]" : "[ ]",
				iFlags & PF_EDITOR ? "[x]" : "[ ]");
			ListAbout.AddItem(&mi); fs2copy += "\n" + mi.strName;
		}

		mi.strName.Format(L"%ls     %s EditorInput ", fs.CPtr(), pPlugin->HasProcessEditorInput() ? "[x]" : "[ ]");
		ListAbout.AddItem(&mi); fs2copy += "\n" + mi.strName;

		if ( !fsDiskMenuStrings.IsEmpty() ) {
			mi.strName = fs + L"    DiskMenuStrings:" + fsDiskMenuStrings;
			ListAbout.AddItem(&mi); fs2copy += "\n" + mi.strName;
		}
		if ( !fsPluginMenuStrings.IsEmpty() ) {
			mi.strName = fs + L"  PluginMenuStrings:" + fsPluginMenuStrings;
			ListAbout.AddItem(&mi); fs2copy += "\n" + mi.strName;
		}
		if ( !fsPluginConfigStrings.IsEmpty() ) {
			mi.strName = fs + L"PluginConfigStrings:" + fsPluginConfigStrings;
			ListAbout.AddItem(&mi); fs2copy += "\n" + mi.strName;
		}
		if ( !fsCommandPrefix.IsEmpty() ) {
			mi.strName.Format(L"%ls      CommandPrefix: \"%ls\"", fs.CPtr(), fsCommandPrefix.CPtr());
			ListAbout.AddItem(&mi); fs2copy += "\n" + mi.strName;
		}
		
	}

	ListAbout.SetPosition(-1, -1, 0, 0);
	/*int iListExitCode = 0;
	do {
		ListAbout.Process();
		iListExitCode = ListAbout.GetExitCode();
		if (iListExitCode>=0)
			ListAbout.ClearDone(); // no close after select item by ENTER or mouse click
	} while(iListExitCode>=0);*/
	ListAbout.Show();
	do {
		while (!ListAbout.Done()) {
			FarKey Key = ListAbout.ReadInput();
			switch (Key) {
				case KEY_CTRLC:
				case KEY_CTRLINS:
				case KEY_CTRLNUMPAD0:
					CopyToClipboard(fs2copy.CPtr());
					break;
				default:
					ListAbout.ProcessInput();
					continue;
			}
		}
		if (ListAbout.GetExitCode() < 0) // exit from loop only by ESC or F10 or click outside vmenu
			break;
		ListAbout.ClearDone(); // no close after select item by ENTER or mouse click
	} while(1);
}

bool CommandLine::ProcessFarCommands(const wchar_t *CmdLine)
{
	bool b_far, b_edit = false, b_view = false;
	std::string::size_type p;
	std::wstring str_command(CmdLine);

	StrTrim(str_command);

	b_far = StrStartsFrom(str_command, L"far:");
	if (!b_far) {
		b_edit = StrStartsFrom(str_command, L"edit:");
		if (!b_edit) {
			b_view = StrStartsFrom(str_command, L"view:");
			if (!b_view) {
				return false; // not found any available prefixes
			}
		}
	}

	if (b_far && str_command == L"far:config") {
		ConfigOptEdit();
		return true; // prefix correct and was processed
	}

	if (b_far && str_command == L"far:about") {
		FarAbout(CtrlObject->Plugins);
		return true; // prefix correct and was processed
	}

	p = b_edit ? 5 // wcslen(L"edit:")
		: ( (b_far && (StrStartsFrom(str_command, L"far:edit:") || StrStartsFrom(str_command, L"far:edit ") || str_command == L"far:edit"))
			? 9 // wcslen(L"far:edit:") or wcslen(L"far:edit ")
			: 0 );
	if (p > 0) {
		p = str_command.find_first_not_of(L" \t", p);
		if (p != std::string::npos) { // after spaces found filename or command
			if (str_command[p]==L'<') { // redirect command
				p = str_command.find_first_not_of(L" \t", p+1);
				if (p != std::string::npos) {
					new FileEditor(
						std::make_shared<FileHolder>(
							ExecuteCommandAndGrabItsOutput( str_command.substr(p,std::string::npos).c_str() ) ),
						CP_AUTODETECT, FFILEEDIT_ENABLEF6 | FFILEEDIT_DISABLEHISTORY);
				}
			}
			else // filename
				new FileEditor(
					std::make_shared<FileHolder>( str_command.substr(p,std::string::npos).c_str() ),
					CP_AUTODETECT, FFILEEDIT_CANNEWFILE | FFILEEDIT_ENABLEF6);
		}
		else // new empty file
			new FileEditor(
				std::make_shared<FileHolder>( Msg::NewFileName.CPtr() ),
				CP_AUTODETECT, FFILEEDIT_CANNEWFILE | FFILEEDIT_ENABLEF6);
		return true; // prefix correct and was processed
	}

	p = b_view ? 5 // wcslen(L"view:")
		: ( (b_far && (StrStartsFrom(str_command, L"far:view:") || StrStartsFrom(str_command, L"far:view ")))
			? 9 // wcslen(L"far:view:") or wcslen(L"far:view ")
			: 0 );
	if (p > 0) {
		p = str_command.find_first_not_of(L" \t", p);
		if (p != std::string::npos) { // after spaces found filename or command
			if (str_command[p]==L'<') { // redirect command
				p = str_command.find_first_not_of(L" \t", p+1);
				if (p != std::string::npos) {
					new FileViewer(std::make_shared<FileHolder>(
							ExecuteCommandAndGrabItsOutput( str_command.substr(p,std::string::npos).c_str() ) ),
					TRUE/*EnableSwitch*/, TRUE/*DisableHistory*/, FALSE/*DisableEdit*/);
				}
			}
			else // filename
				new FileViewer(std::make_shared<FileHolder>( str_command.substr(p,std::string::npos).c_str() ),
					TRUE/*EnableSwitch*/, FALSE/*DisableHistory*/, FALSE/*DisableEdit*/);
		}
		return true; // anyway prefix correct and was processed
	}

	return false; // not found any available prefixes
}

CmdLineVisibleScope::CmdLineVisibleScope()
{
	if (CtrlObject && CtrlObject->CmdLine && !CtrlObject->CmdLine->IsVisible()) {
		CtrlObject->CmdLine->SetVisible(true);
		CtrlObject->CmdLine->Show();
	}
}

CmdLineVisibleScope::~CmdLineVisibleScope()
{
	FilePanels *cp = CtrlObject->Cp();
	if (cp) {
		cp->UpdateCmdLineVisibility();
	}
}

