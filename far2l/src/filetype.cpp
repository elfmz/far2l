/*
filetype.cpp

Работа с ассоциациями файлов
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

#include "filetype.hpp"
#include "lang.hpp"
#include "keys.hpp"
#include "dialog.hpp"
#include "vmenu.hpp"
#include "ctrlobj.hpp"
#include "cmdline.hpp"
#include "history.hpp"
#include "filepanels.hpp"
#include "panel.hpp"
#include "rdrwdsk.hpp"
#include "savescr.hpp"
#include "CFileMask.hpp"
#include "message.hpp"
#include "interf.hpp"
#include "config.hpp"
#include "execute.hpp"
#include "fnparce.hpp"
#include "strmix.hpp"
#include "dirmix.hpp"
#include "ConfigRW.hpp"

struct FileTypeStrings
{
	const char *Help, *HelpModify, *State, *Associations, *TypeFmt, *Type0, *Execute, *Desc, *Mask, *View,
			*Edit, *AltExec, *AltView, *AltEdit;
};

const FileTypeStrings FTS = {"FileAssoc", "FileAssocModify", "State", "Associations", "Associations/Type%d",
		"Associations/Type", "Execute", "Description", "Mask", "View", "Edit", "AltExec", "AltView",
		"AltEdit"};

static int GetDescriptionWidth(ConfigReader &cfg_reader, const wchar_t *Name = nullptr)
{
	int Width = 0;
	//	RenumKeyRecord(FTS.Associations,FTS.TypeFmt,FTS.Type0);
	for (int NumLine = 0;; NumLine++) {
		cfg_reader.SelectSectionFmt(FTS.TypeFmt, NumLine);
		FARString strMask;
		if (!cfg_reader.GetString(strMask, FTS.Mask, L""))
			break;

		CFileMask FMask;

		if (!FMask.Set(strMask, FMF_SILENT))
			continue;

		FARString strDescription = cfg_reader.GetString(FTS.Desc, L"");
		int CurWidth;

		if (!Name) {
			CurWidth = HiStrCellsCount(strDescription);
		} else {
			if (!FMask.Compare(Name))
				continue;

			FARString strExpandedDesc = strDescription;
			SubstFileName(strExpandedDesc, Name, nullptr, nullptr, TRUE);
			CurWidth = HiStrCellsCount(strExpandedDesc);
		}

		if (CurWidth > Width)
			Width = CurWidth;
	}

	if (Width > ScrX / 2)
		Width = ScrX / 2;

	return (Width);
}

/*
	$ 14.01.2001 SVS
	Добавим интелектуальности.
	Если встречается "IF" и оно выполняется, то команда
	помещается в список

	Вызывается для F3, F4 - ассоциации
	Enter в ком строке - ассоциации.
*/
/*
	$ 06.07.2001
	+ Используем CFileMask вместо GetCommaWord, этим самым добиваемся того, что
	можно использовать маски исключения
	- Убрал непонятный мне запрет на использование маски файлов типа "*.*"
	(был когда-то, вроде, такой баг-репорт)
*/
bool ProcessLocalFileTypes(const wchar_t *Name, int Mode, bool CanAddHistory, FARString &strCurDir)
{
	ConfigReader cfg_reader;
	// RenumKeyRecord(FTS.Associations,FTS.TypeFmt,FTS.Type0);
	MenuItemEx TypesMenuItem;
	VMenu TypesMenu(Msg::SelectAssocTitle, nullptr, 0, ScrY - 4);
	TypesMenu.SetHelp(FARString(FTS.Help));
	TypesMenu.SetFlags(VMENU_WRAPMODE);
	TypesMenu.SetPosition(-1, -1, 0, 0);
	int DizWidth = GetDescriptionWidth(cfg_reader, Name);
	int ActualCmdCount = 0;		// отображаемых ассоциаций в меню
	CFileMask FMask;			// для работы с масками файлов
	FARString strCommand, strDescription, strMask;
	int CommandCount = 0;

	for (int I = 0;; I++) {
		strCommand.Clear();
		cfg_reader.SelectSectionFmt(FTS.TypeFmt, I);
		if (!cfg_reader.GetString(strMask, FTS.Mask, L""))
			break;

		if (FMask.Set(strMask, FMF_SILENT)) {
			if (FMask.Compare(Name)) {
				LPCSTR Type = nullptr;

				switch (Mode) {
					case FILETYPE_EXEC:
						Type = FTS.Execute;
						break;
					case FILETYPE_VIEW:
						Type = FTS.View;
						break;
					case FILETYPE_EDIT:
						Type = FTS.Edit;
						break;
					case FILETYPE_ALTEXEC:
						Type = FTS.AltExec;
						break;
					case FILETYPE_ALTVIEW:
						Type = FTS.AltView;
						break;
					case FILETYPE_ALTEDIT:
						Type = FTS.AltEdit;
						break;
				}

				DWORD State = cfg_reader.GetUInt(FTS.State, 0xffffffff);

				if (State & (1 << Mode)) {
					FARString strNewCommand = cfg_reader.GetString(Type, L"");

					if (!strNewCommand.IsEmpty()) {
						strCommand = strNewCommand;
						strDescription = cfg_reader.GetString(FTS.Desc, L"");
						CommandCount++;
					}
				}
			}

			if (strCommand.IsEmpty())
				continue;
		}

		TypesMenuItem.Clear();
		FARString strCommandText = strCommand;
		SubstFileName(strCommandText, Name, nullptr, nullptr, TRUE);

		ActualCmdCount++;
		FARString strMenuText;

		if (DizWidth) {
			FARString strTitle;

			if (!strDescription.IsEmpty()) {
				strTitle = strDescription;
				SubstFileName(strTitle, Name, nullptr, nullptr, TRUE);
			}

			size_t Pos = 0;
			bool Ampersand = strTitle.Pos(Pos, L'&');

			if (DizWidth + Ampersand > ScrX / 2 && Ampersand && static_cast<int>(Pos) > DizWidth)
				Ampersand = false;

			strMenuText.Format(L"%-*.*ls %lc ", DizWidth + Ampersand, DizWidth + Ampersand, strTitle.CPtr(),
					BoxSymbols[BS_V1]);
		}

		TruncStr(strCommandText, ScrX - DizWidth - 14);
		strMenuText+= strCommandText;
		TypesMenuItem.strName = strMenuText;
		TypesMenuItem.SetSelect(!I);
		TypesMenu.SetUserData(strCommand.CPtr(), 0, TypesMenu.AddItem(&TypesMenuItem));
	}

	if (!CommandCount)
		return false;

	if (!ActualCmdCount)
		return true;

	int ExitCode = 0;

	if (ActualCmdCount > 1) {
		TypesMenu.Process();
		ExitCode = TypesMenu.Modal::GetExitCode();

		if (ExitCode < 0)
			return true;
	}

	int Size = TypesMenu.GetUserDataSize(ExitCode);
	LPWSTR Command = strCommand.GetBuffer(Size / sizeof(wchar_t));
	TypesMenu.GetUserData(Command, Size, ExitCode);
	strCommand.ReleaseBuffer(Size);
	FARString strListName, strAnotherListName;
	/*int PreserveLFN=*/SubstFileName(strCommand, Name, &strListName, &strAnotherListName);
	bool ListFileUsed = !strListName.IsEmpty() || !strAnotherListName.IsEmpty();

	{
		// PreserveLongName PreserveName(PreserveLFN);
		RemoveExternalSpaces(strCommand);

		if (!strCommand.IsEmpty()) {
			bool isSilent = (strCommand.At(0) == L'@');

			if (isSilent) {
				strCommand.LShift(1);
			} else if (CanAddHistory && !(Opt.ExcludeCmdHistory & EXCLUDECMDHISTORY_NOTFARASS))		// AN
			{
				//CtrlObject->CmdHistory->AddToHistory(strCommand);
				CtrlObject->CmdHistory->AddToHistoryExtra(strCommand,strCurDir);
			}

			// ProcessOSAliases(strCommand);

			if (CtrlObject->CmdLine->ProcessFarCommands(strCommand))	// far commands always not silent
				;
			else if (!isSilent) {
				CtrlObject->CmdLine->ExecString(strCommand, false, false, ListFileUsed);
			} else {
#if 1
				SaveScreen SaveScr;
				CtrlObject->Cp()->LeftPanel->CloseFile();
				CtrlObject->Cp()->RightPanel->CloseFile();
				Execute(strCommand, 0, 0, ListFileUsed, true);
#else
				// здесь была бага с прорисовкой (и... вывод данных
				// на команду "@type !@!" пропадал с экрана)
				// сделаем по аналогии с CommandLine::CmdExecute()
				{
					RedrawDesktop RdrwDesktop(TRUE);
					Execute(strCommand, 0, 0, ListFileUsed);
					ScrollScreen(1);	// обязательно, иначе деструктор RedrawDesktop
										// проредравив экран забьет последнюю строку вывода.
				}
				CtrlObject->Cp()->LeftPanel->UpdateIfChanged(UIC_UPDATE_FORCE);
				CtrlObject->Cp()->RightPanel->UpdateIfChanged(UIC_UPDATE_FORCE);
				CtrlObject->Cp()->Redraw();
#endif
			}
			if (FrameManager->GetCurrentFrame()->GetType() == MODALTYPE_VIEWER) {
				TypesMenu.ResetCursor();
			}
		}
	}

	if (!strListName.IsEmpty())
		QueueDeleteOnClose(strListName);

	if (!strAnotherListName.IsEmpty())
		QueueDeleteOnClose(strAnotherListName);

	return true;
}

void ProcessGlobalFileTypes(const wchar_t *Name, bool RunAs, bool CanAddHistory, FARString &strCurDir)
{
	FARString strName(Name);
	EscapeSpace(strName);
	CtrlObject->CmdLine->ExecString(strName, true, true, false, false, RunAs);

	if (CanAddHistory && !(Opt.ExcludeCmdHistory & EXCLUDECMDHISTORY_NOTWINASS)) {
		//CtrlObject->CmdHistory->AddToHistory(strName);
		CtrlObject->CmdHistory->AddToHistoryExtra(strName,strCurDir);
	}
}

/*
	Используется для запуска внешнего редактора и вьювера
*/
void ProcessExternal(const wchar_t *Command, const wchar_t *Name, bool CanAddHistory, FARString &strCurDir)
{
	FARString strListName, strAnotherListName;
	FARString strFullName;
	FARString strExecStr = Command;
	FARString strFullExecStr = Command;
	{
		/*int PreserveLFN=*/SubstFileName(strExecStr, Name, &strListName, &strAnotherListName);
		bool ListFileUsed = !strListName.IsEmpty() || !strAnotherListName.IsEmpty();

		// PreserveLongName PreserveName(PreserveLFN);
		ConvertNameToFull(Name, strFullName);
		// BUGBUGBUGBUGBUGBUG !!! Same ListNames!!!
		SubstFileName(strFullExecStr, strFullName, &strListName, &strAnotherListName);

		if (CanAddHistory) {
			CtrlObject->ViewHistory->AddToHistory(strFullExecStr, 1 | 2);
		}

		if (strExecStr.At(0) != L'@') {
			CtrlObject->CmdLine->ExecString(strExecStr, 0, 0, ListFileUsed, true);
		} else {
			SaveScreen SaveScr;
			CtrlObject->Cp()->LeftPanel->CloseFile();
			CtrlObject->Cp()->RightPanel->CloseFile();
			Execute(strExecStr.CPtr() + 1, 0, 0, ListFileUsed);
		}
	}

	if (!strListName.IsEmpty())
		QueueDeleteOnClose(strListName);

	if (!strAnotherListName.IsEmpty())
		QueueDeleteOnClose(strAnotherListName);
}

static int FillFileTypesMenu(VMenu *TypesMenu, int MenuPos)
{
	ConfigReader cfg_reader;
	int DizWidth = GetDescriptionWidth(cfg_reader);
	MenuItemEx TypesMenuItem;
	TypesMenu->DeleteItems();
	int NumLine = 0;

	for (;; NumLine++) {
		cfg_reader.SelectSectionFmt(FTS.TypeFmt, NumLine);
		FARString strMask;
		if (!cfg_reader.GetString(strMask, FTS.Mask, L""))
			break;

		TypesMenuItem.Clear();

		FARString strMenuText;

		if (DizWidth) {
			FARString strDescription = cfg_reader.GetString(FTS.Desc, L"");
			FARString strTitle = strDescription;
			size_t Pos = 0;
			bool Ampersand = strTitle.Pos(Pos, L'&');

			if (DizWidth + Ampersand > ScrX / 2 && Ampersand && static_cast<int>(Pos) > DizWidth)
				Ampersand = false;

			strMenuText.Format(L"%-*.*ls %lc ", DizWidth + Ampersand, DizWidth + Ampersand, strTitle.CPtr(),
					BoxSymbols[BS_V1]);
		}

		// TruncStr(strMask,ScrX-DizWidth-14);
		strMenuText+= strMask;
		TypesMenuItem.strName = strMenuText;
		TypesMenuItem.SetSelect(NumLine == MenuPos);
		TypesMenu->AddItem(&TypesMenuItem);
	}

	TypesMenuItem.strName.Clear();
	TypesMenuItem.SetSelect(NumLine == MenuPos);
	TypesMenu->AddItem(&TypesMenuItem);
	return NumLine;
}

void MoveMenuItem(int Pos, int NewPos)
{
	if (Pos != NewPos) {
		ConfigWriter().MoveIndexedSection(FTS.Type0, Pos, NewPos);
	}
}

enum EDITTYPERECORD
{
	ETR_DOUBLEBOX,
	ETR_TEXT_MASKS,
	ETR_EDIT_MASKS,
	ETR_TEXT_DESCR,
	ETR_EDIT_DESCR,
	ETR_SEPARATOR1,
	ETR_COMBO_EXEC,
	ETR_EDIT_EXEC,
	ETR_COMBO_ALTEXEC,
	ETR_EDIT_ALTEXEC,
	ETR_COMBO_VIEW,
	ETR_EDIT_VIEW,
	ETR_COMBO_ALTVIEW,
	ETR_EDIT_ALTVIEW,
	ETR_COMBO_EDIT,
	ETR_EDIT_EDIT,
	ETR_COMBO_ALTEDIT,
	ETR_EDIT_ALTEDIT,
	ETR_SEPARATOR2,
	ETR_BUTTON_OK,
	ETR_BUTTON_CANCEL,
};

LONG_PTR WINAPI EditTypeRecordDlgProc(HANDLE hDlg, int Msg, int Param1, LONG_PTR Param2)
{
	switch (Msg) {
		case DN_BTNCLICK:

			switch (Param1) {
				case ETR_COMBO_EXEC:
				case ETR_COMBO_ALTEXEC:
				case ETR_COMBO_VIEW:
				case ETR_COMBO_ALTVIEW:
				case ETR_COMBO_EDIT:
				case ETR_COMBO_ALTEDIT:
					SendDlgMessage(hDlg, DM_ENABLE, Param1 + 1, Param2 == BSTATE_CHECKED ? TRUE : FALSE);
					break;
			}

			break;
		case DN_CLOSE:

			if (Param1 == ETR_BUTTON_OK) {
				BOOL Result = TRUE;
				LPCWSTR Masks = reinterpret_cast<LPCWSTR>(
						SendDlgMessage(hDlg, DM_GETCONSTTEXTPTR, ETR_EDIT_MASKS, 0));
				CFileMask FMask;

				if (!FMask.Set(Masks, 0)) {
					Result = FALSE;
				}

				return Result;
			}

			break;
	}

	return DefDlgProc(hDlg, Msg, Param1, Param2);
}

static bool EditTypeRecord(int EditPos, int TotalRecords, bool NewRec)
{
	bool Result = false;
	const int DlgX = 76, DlgY = 23;
	DialogDataEx EditDlgData[] = {
			{DI_DOUBLEBOX, 3, 1,        DlgX - 4, DlgY - 2, {},                    0,                             Msg::FileAssocTitle  },
			{DI_TEXT,      5, 2,        0,        2,        {},                    0,                             Msg::FileAssocMasks  },
			{DI_EDIT,      5, 3,        DlgX - 6, 3,        {(DWORD_PTR)L"Masks"}, DIF_FOCUS | DIF_HISTORY,       L""                  },
			{DI_TEXT,      5, 4,        0,        4,        {},                    0,                             Msg::FileAssocDescr  },
			{DI_EDIT,      5, 5,        DlgX - 6, 5,        {},                    0,                             L""                  },
			{DI_TEXT,      3, 6,        0,        6,        {},                    DIF_SEPARATOR,                 L""                  },
			{DI_CHECKBOX,  5, 7,        0,        7,        {1},                   0,                             Msg::FileAssocExec   },
			{DI_EDIT,      9, 8,        DlgX - 6, 8,        {},                    DIF_EDITPATH,                  L""                  },
			{DI_CHECKBOX,  5, 9,        0,        9,        {1},                   0,                             Msg::FileAssocAltExec},
			{DI_EDIT,      9, 10,       DlgX - 6, 10,       {},                    DIF_EDITPATH,                  L""                  },
			{DI_CHECKBOX,  5, 11,       0,        11,       {1},                   0,                             Msg::FileAssocView   },
			{DI_EDIT,      9, 12,       DlgX - 6, 12,       {},                    DIF_EDITPATH,                  L""                  },
			{DI_CHECKBOX,  5, 13,       0,        13,       {1},                   0,                             Msg::FileAssocAltView},
			{DI_EDIT,      9, 14,       DlgX - 6, 14,       {},                    DIF_EDITPATH,                  L""                  },
			{DI_CHECKBOX,  5, 15,       0,        15,       {1},                   0,                             Msg::FileAssocEdit   },
			{DI_EDIT,      9, 16,       DlgX - 6, 16,       {},                    DIF_EDITPATH,                  L""                  },
			{DI_CHECKBOX,  5, 17,       0,        17,       {1},                   0,                             Msg::FileAssocAltEdit},
			{DI_EDIT,      9, 18,       DlgX - 6, 18,       {},                    DIF_EDITPATH,                  L""                  },
			{DI_TEXT,      3, DlgY - 4, 0,        DlgY - 4, {},                    DIF_SEPARATOR,                 L""                  },
			{DI_BUTTON,    0, DlgY - 3, 0,        DlgY - 3, {},                    DIF_DEFAULT | DIF_CENTERGROUP, Msg::Ok              },
			{DI_BUTTON,    0, DlgY - 3, 0,        DlgY - 3, {},                    DIF_CENTERGROUP,               Msg::Cancel          }
	};
	MakeDialogItemsEx(EditDlgData, EditDlg);

	if (!NewRec) {
		ConfigReader cfg_reader;
		cfg_reader.SelectSectionFmt(FTS.TypeFmt, EditPos);
		EditDlg[ETR_EDIT_MASKS].strData = cfg_reader.GetString(FTS.Mask, L"");
		EditDlg[ETR_EDIT_DESCR].strData = cfg_reader.GetString(FTS.Desc, L"");
		EditDlg[ETR_EDIT_EXEC].strData = cfg_reader.GetString(FTS.Execute, L"");
		EditDlg[ETR_EDIT_ALTEXEC].strData = cfg_reader.GetString(FTS.AltExec, L"");
		EditDlg[ETR_EDIT_VIEW].strData = cfg_reader.GetString(FTS.View, L"");
		EditDlg[ETR_EDIT_ALTVIEW].strData = cfg_reader.GetString(FTS.AltView, L"");
		EditDlg[ETR_EDIT_EDIT].strData = cfg_reader.GetString(FTS.Edit, L"");
		EditDlg[ETR_EDIT_ALTEDIT].strData = cfg_reader.GetString(FTS.AltEdit, L"");
		DWORD State = cfg_reader.GetUInt(FTS.State, 0xffffffff);

		for (int i = FILETYPE_EXEC, Item = ETR_COMBO_EXEC; i <= FILETYPE_ALTEDIT; i++, Item+= 2) {
			if (!(State & (1 << i))) {
				EditDlg[Item].Selected = BSTATE_UNCHECKED;
				EditDlg[Item + 1].Flags|= DIF_DISABLE;
			}
		}
	}

	Dialog Dlg(EditDlg, ARRAYSIZE(EditDlg), EditTypeRecordDlgProc);
	Dlg.SetHelp(FARString(FTS.HelpModify));
	Dlg.SetPosition(-1, -1, DlgX, DlgY);
	Dlg.Process();

	if (Dlg.GetExitCode() == ETR_BUTTON_OK) {
		ConfigWriter cfg_writer;
		cfg_writer.SelectSectionFmt(FTS.TypeFmt, EditPos);

		if (NewRec) {
			cfg_writer.ReserveIndexedSection(FTS.Type0, (unsigned int)EditPos);
		}

		cfg_writer.SetString(FTS.Mask, EditDlg[ETR_EDIT_MASKS].strData);
		cfg_writer.SetString(FTS.Desc, EditDlg[ETR_EDIT_DESCR].strData);
		cfg_writer.SetString(FTS.Execute, EditDlg[ETR_EDIT_EXEC].strData);
		cfg_writer.SetString(FTS.AltExec, EditDlg[ETR_EDIT_ALTEXEC].strData);
		cfg_writer.SetString(FTS.View, EditDlg[ETR_EDIT_VIEW].strData);
		cfg_writer.SetString(FTS.AltView, EditDlg[ETR_EDIT_ALTVIEW].strData);
		cfg_writer.SetString(FTS.Edit, EditDlg[ETR_EDIT_EDIT].strData);
		cfg_writer.SetString(FTS.AltEdit, EditDlg[ETR_EDIT_ALTEDIT].strData);
		DWORD State = 0;

		for (int i = FILETYPE_EXEC, Item = ETR_COMBO_EXEC; i <= FILETYPE_ALTEDIT; i++, Item+= 2) {
			if (EditDlg[Item].Selected == BSTATE_CHECKED) {
				State|= (1 << i);
			}
		}

		cfg_writer.SetUInt(FTS.State, State);
		Result = true;
	}

	return Result;
}

static bool DeleteTypeRecord(int DeletePos)
{
	bool Result = false;
	FARString strItemName;

	{
		ConfigReader cfg_reader;
		cfg_reader.SelectSectionFmt(FTS.TypeFmt, DeletePos);
		strItemName = cfg_reader.GetString(FTS.Mask, L"");
		InsertQuote(strItemName);
	}

	if (!Message(MSG_WARNING, 2, Msg::AssocTitle, Msg::AskDelAssoc, strItemName, Msg::Delete, Msg::Cancel)) {
		ConfigWriter cfg_writer;
		cfg_writer.SelectSectionFmt(FTS.TypeFmt, DeletePos);
		cfg_writer.RemoveSection();
		cfg_writer.DefragIndexedSections(FTS.Type0);
		Result = true;
	}

	return Result;
}

void EditFileTypes()
{
	int NumLine = 0;
	int MenuPos = 0;
	// RenumKeyRecord(FTS.Associations,FTS.TypeFmt,FTS.Type0);
	VMenu TypesMenu(Msg::AssocTitle, nullptr, 0, ScrY - 4);
	TypesMenu.SetHelp(FARString(FTS.Help));
	TypesMenu.SetFlags(VMENU_WRAPMODE);
	TypesMenu.SetPosition(-1, -1, 0, 0);
	TypesMenu.SetBottomTitle(Msg::AssocBottom);
	while (1) {
		bool MenuModified = true;

		while (!TypesMenu.Done()) {
			if (MenuModified) {
				TypesMenu.Hide();
				NumLine = FillFileTypesMenu(&TypesMenu, MenuPos);
				TypesMenu.SetPosition(-1, -1, -1, -1);
				TypesMenu.Show();
				MenuModified = false;
			}

			const auto Key = TypesMenu.ReadInput();
			MenuPos = TypesMenu.GetSelectPos();

			switch (Key) {
				case KEY_NUMDEL:
				case KEY_DEL:

					if (MenuPos < NumLine)
						DeleteTypeRecord(MenuPos);

					MenuModified = true;
					break;
				case KEY_NUMPAD0:
				case KEY_INS:
					EditTypeRecord(MenuPos, NumLine, true);
					MenuModified = true;
					break;
				case KEY_NUMENTER:
				case KEY_ENTER:
				case KEY_F4:

					if (MenuPos < NumLine)
						EditTypeRecord(MenuPos, NumLine, false);

					MenuModified = true;
					break;
				case KEY_CTRLUP:
				case KEY_CTRLDOWN: {
					if (MenuPos != TypesMenu.GetItemCount() - 1) {
						if (!(Key == KEY_CTRLUP && !MenuPos)
								&& !(Key == KEY_CTRLDOWN && MenuPos == TypesMenu.GetItemCount() - 2)) {
							int NewMenuPos = MenuPos + (Key == KEY_CTRLUP ? -1 : +1);
							MoveMenuItem(MenuPos, NewMenuPos);
							MenuPos = NewMenuPos;
							MenuModified = true;
						}
					}
				} break;
				default:
					TypesMenu.ProcessInput();
					break;
			}
		}

		int ExitCode = TypesMenu.Modal::GetExitCode();

		if (ExitCode != -1) {
			MenuPos = ExitCode;
			TypesMenu.ClearDone();
			TypesMenu.WriteInput(KEY_F4);
			continue;
		}

		break;
	}
}
