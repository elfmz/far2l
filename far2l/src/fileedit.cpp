/*
fileedit.cpp

Редактирование файла - надстройка над editor.cpp
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
#include "fileedit.hpp"
#include "keyboard.hpp"
#include "codepage.hpp"
#include "lang.hpp"
#include "macroopcode.hpp"
#include "keys.hpp"
#include "ctrlobj.hpp"
#include "poscache.hpp"
#include "filepanels.hpp"
#include "panel.hpp"
#include "dialog.hpp"
#include "fileview.hpp"
#include "help.hpp"
#include "ctrlobj.hpp"
#include "manager.hpp"
#include "namelist.hpp"
#include "history.hpp"
#include "cmdline.hpp"
#include "scrbuf.hpp"
#include "savescr.hpp"
#include "chgprior.hpp"
#include "filestr.hpp"
#include "TPreRedrawFunc.hpp"
#include "syslog.hpp"
#include "interf.hpp"
#include "message.hpp"
#include "config.hpp"
#include "delete.hpp"
#include "datetime.hpp"
#include "pathmix.hpp"
#include "dirmix.hpp"
#include "strmix.hpp"
#include "exitcode.hpp"
#include "cache.hpp"
#include "constitle.hpp"
#include "wakeful.hpp"
#include "DlgGuid.hpp"
#include "filelist.hpp"

enum enumOpenEditor
{
	ID_OE_TITLE,
	ID_OE_OPENFILETITLE,
	ID_OE_FILENAME,
	ID_OE_SEPARATOR1,
	ID_OE_CODEPAGETITLE,
	ID_OE_CODEPAGE,
	ID_OE_SEPARATOR2,
	ID_OE_OK,
	ID_OE_CANCEL,
};

static const wchar_t *EOLName(const wchar_t *eol)
{
	if (wcscmp(eol, L"\n") == 0)
		return L"LF";

	if (wcscmp(eol, L"\r") == 0)
		return L"CR";

	if (wcscmp(eol, L"\r\n") == 0)
		return L"CRLF";

	if (wcscmp(eol, L"\r\r\n") == 0)
		return L"CRRLF";

	return eol;		// L"WTF";
}

LONG_PTR __stdcall hndOpenEditor(HANDLE hDlg, int msg, int param1, LONG_PTR param2)
{
	if (msg == DN_INITDIALOG) {
		int codepage = *(int *)SendDlgMessage(hDlg, DM_GETDLGDATA, 0, 0);
		FillCodePagesList(hDlg, ID_OE_CODEPAGE, codepage, true, false);
	}

	if (msg == DN_CLOSE) {
		if (param1 == ID_OE_OK) {
			int *param = (int *)SendDlgMessage(hDlg, DM_GETDLGDATA, 0, 0);
			FarListPos pos;
			SendDlgMessage(hDlg, DM_LISTGETCURPOS, ID_OE_CODEPAGE, (LONG_PTR)&pos);
			*param = (int)SendDlgMessage(hDlg, DM_LISTGETDATA, ID_OE_CODEPAGE, pos.SelectPos);
			return TRUE;
		}
	}

	return DefDlgProc(hDlg, msg, param1, param2);
}

bool dlgOpenEditor(FARString &strFileName, UINT &codepage)
{
	const wchar_t *HistoryName = L"NewEdit";
	DialogDataEx EditDlgData[] = {
		{DI_DOUBLEBOX, 3,  1, 72, 8, {}, 0, Msg::EditTitle},
		{DI_TEXT,      5,  2, 0,  2, {}, 0, Msg::EditOpenCreateLabel},
		{DI_EDIT,      5,  3, 70, 3, {(DWORD_PTR)HistoryName}, DIF_FOCUS | DIF_HISTORY | DIF_EDITEXPAND | DIF_EDITPATH, L""},
		{DI_TEXT,      3,  4, 0,  4, {}, DIF_SEPARATOR, L""},
		{DI_TEXT,      5,  5, 0,  5, {}, 0, Msg::EditCodePage},
		{DI_COMBOBOX,  25, 5, 70, 5, {}, DIF_DROPDOWNLIST | DIF_LISTWRAPMODE | DIF_LISTAUTOHIGHLIGHT, L""},
		{DI_TEXT,      3,  6, 0,  6, {}, DIF_SEPARATOR, L""},
		{DI_BUTTON,    0,  7, 0,  7, {}, DIF_DEFAULT | DIF_CENTERGROUP, Msg::Ok},
		{DI_BUTTON,    0,  7, 0,  7, {}, DIF_CENTERGROUP, Msg::Cancel}
	};
	MakeDialogItemsEx(EditDlgData, EditDlg);
	EditDlg[ID_OE_FILENAME].strData = strFileName;
	Dialog Dlg(EditDlg, ARRAYSIZE(EditDlg), (FARWINDOWPROC)hndOpenEditor, (LONG_PTR)&codepage);
	Dlg.SetPosition(-1, -1, 76, 10);
	Dlg.SetHelp(L"FileOpenCreate");
	Dlg.SetId(FileOpenCreateId);
	Dlg.Process();

	if (Dlg.GetExitCode() == ID_OE_OK) {
		strFileName = EditDlg[ID_OE_FILENAME].strData;
		ConvertHomePrefixInPath(strFileName);
		return true;
	}

	return false;
}

enum enumSaveFileAs
{
	ID_SF_TITLE,
	ID_SF_SAVEASFILETITLE,
	ID_SF_FILENAME,
	ID_SF_SEPARATOR1,
	ID_SF_CODEPAGETITLE,
	ID_SF_CODEPAGE,
	ID_SF_SIGNATURE,
	ID_SF_SEPARATOR2,
	ID_SF_SAVEASFORMATTITLE,
	ID_SF_DONOTCHANGE,
	ID_SF_DOS,
	ID_SF_UNIX,
	ID_SF_MAC,
	ID_SF_SEPARATOR3,
	ID_SF_OK,
	ID_SF_CANCEL,
};

LONG_PTR __stdcall hndSaveFileAs(HANDLE hDlg, int msg, int param1, LONG_PTR param2)
{
	static UINT codepage = 0;

	switch (msg) {
		case DN_INITDIALOG: {
			codepage = *(UINT *)SendDlgMessage(hDlg, DM_GETDLGDATA, 0, 0);
			FillCodePagesList(hDlg, ID_SF_CODEPAGE, codepage, false, false);

			if (IsUnicodeOrUtfCodePage(codepage)) {
				SendDlgMessage(hDlg, DM_ENABLE, ID_SF_SIGNATURE, TRUE);
			} else {
				SendDlgMessage(hDlg, DM_SETCHECK, ID_SF_SIGNATURE, BSTATE_UNCHECKED);
				SendDlgMessage(hDlg, DM_ENABLE, ID_SF_SIGNATURE, FALSE);
			}

			break;
		}
		case DN_CLOSE: {
			if (param1 == ID_SF_OK) {
				UINT *codepage = (UINT *)SendDlgMessage(hDlg, DM_GETDLGDATA, 0, 0);
				FarListPos pos;
				SendDlgMessage(hDlg, DM_LISTGETCURPOS, ID_SF_CODEPAGE, (LONG_PTR)&pos);
				*codepage = (UINT)SendDlgMessage(hDlg, DM_LISTGETDATA, ID_SF_CODEPAGE, pos.SelectPos);
				return TRUE;
			}

			break;
		}
		case DN_EDITCHANGE: {
			if (param1 == ID_SF_CODEPAGE) {
				FarListPos pos;
				SendDlgMessage(hDlg, DM_LISTGETCURPOS, ID_SF_CODEPAGE, (LONG_PTR)&pos);
				UINT Cp = static_cast<UINT>(
						SendDlgMessage(hDlg, DM_LISTGETDATA, ID_SF_CODEPAGE, pos.SelectPos));

				if (Cp != codepage) {
					codepage = Cp;

					if (IsUnicodeOrUtfCodePage(codepage)) {
						SendDlgMessage(hDlg, DM_SETCHECK, ID_SF_SIGNATURE, BSTATE_CHECKED);
						SendDlgMessage(hDlg, DM_ENABLE, ID_SF_SIGNATURE, TRUE);
					} else {
						SendDlgMessage(hDlg, DM_SETCHECK, ID_SF_SIGNATURE, BSTATE_UNCHECKED);
						SendDlgMessage(hDlg, DM_ENABLE, ID_SF_SIGNATURE, FALSE);
					}

					return TRUE;
				}
			}

			break;
		}
	}

	return DefDlgProc(hDlg, msg, param1, param2);
}

bool dlgSaveFileAs(FARString &strFileName, int &TextFormat, UINT &codepage, bool &AddSignature)
{
	const wchar_t *HistoryName = L"NewEdit";
	DialogDataEx EditDlgData[] = {
		{DI_DOUBLEBOX,   3,  1,  72, 15, {}, 0, Msg::EditTitle},
		{DI_TEXT,        5,  2,  0,  2,  {}, 0, Msg::EditSaveAs},
		{DI_EDIT,        5,  3,  70, 3,  {(DWORD_PTR)HistoryName}, DIF_FOCUS | DIF_HISTORY | DIF_EDITEXPAND | DIF_EDITPATH, L""},
		{DI_TEXT,        3,  4,  0,  4,  {}, DIF_SEPARATOR, L""},
		{DI_TEXT,        5,  5,  0,  5,  {}, 0, Msg::EditCodePage},
		{DI_COMBOBOX,    25, 5,  70, 5,  {}, DIF_DROPDOWNLIST | DIF_LISTWRAPMODE | DIF_LISTAUTOHIGHLIGHT, L""},
		{DI_CHECKBOX,    5,  6,  0,  6,  {AddSignature}, DIF_DISABLE, Msg::EditAddSignature},
		{DI_TEXT,        3,  7,  0,  7,  {}, DIF_SEPARATOR, L""},
		{DI_TEXT,        5,  8,  0,  8,  {}, 0, Msg::EditSaveAsFormatTitle},
		{DI_RADIOBUTTON, 5,  9,  0,  9,  {}, DIF_GROUP, Msg::EditSaveOriginal},
		{DI_RADIOBUTTON, 5,  10, 0,  10, {}, 0, Msg::EditSaveDOS},
		{DI_RADIOBUTTON, 5,  11, 0,  11, {}, 0, Msg::EditSaveUnix},
		{DI_RADIOBUTTON, 5,  12, 0,  12, {}, 0, Msg::EditSaveMac},
		{DI_TEXT,        3,  13, 0,  13, {}, DIF_SEPARATOR, L""},
		{DI_BUTTON,      0,  14, 0,  14, {}, DIF_DEFAULT | DIF_CENTERGROUP, Msg::Ok},
		{DI_BUTTON,      0,  14, 0,  14, {}, DIF_CENTERGROUP, Msg::Cancel}
	};
	MakeDialogItemsEx(EditDlgData, EditDlg);
	EditDlg[ID_SF_FILENAME].strData =
			(/*Flags.Check(FFILEEDIT_SAVETOSAVEAS)?strFullFileName:strFileName*/ strFileName);
	{
		size_t pos = 0;
		if (EditDlg[ID_SF_FILENAME].strData.Pos(pos, Msg::NewFileName))
			EditDlg[ID_SF_FILENAME].strData.Truncate(pos);
	}
	EditDlg[ID_SF_DONOTCHANGE + TextFormat].Selected = TRUE;
	Dialog Dlg(EditDlg, ARRAYSIZE(EditDlg), (FARWINDOWPROC)hndSaveFileAs, (LONG_PTR)&codepage);
	Dlg.SetPosition(-1, -1, 76, 17);
	Dlg.SetHelp(L"FileSaveAs");
	Dlg.SetId(FileSaveAsId);
	Dlg.Process();

	if ((Dlg.GetExitCode() == ID_SF_OK) && !EditDlg[ID_SF_FILENAME].strData.IsEmpty()) {
		strFileName = EditDlg[ID_SF_FILENAME].strData;
		ConvertHomePrefixInPath(strFileName);
		AddSignature = EditDlg[ID_SF_SIGNATURE].Selected != 0;

		if (EditDlg[ID_SF_DONOTCHANGE].Selected)
			TextFormat = 0;
		else if (EditDlg[ID_SF_DOS].Selected)
			TextFormat = 1;
		else if (EditDlg[ID_SF_UNIX].Selected)
			TextFormat = 2;
		else if (EditDlg[ID_SF_MAC].Selected)
			TextFormat = 3;

		return true;
	}

	return false;
}

const FileEditor *FileEditor::CurrentEditor = nullptr;

FileEditor::FileEditor(FileHolderPtr NewFileHolder, UINT codepage, DWORD InitFlags, int StartLine, int StartChar,
		const wchar_t *PluginData, int OpenModeExstFile)
	:
	BadConversion(false), SaveAsTextFormat(0)
{
	ScreenObject::SetPosition(0, 0, ScrX, ScrY);
	Flags.Set(InitFlags);
	Flags.Set(FFILEEDIT_FULLSCREEN);
	Init(NewFileHolder, codepage, nullptr, InitFlags, StartLine, StartChar, PluginData, OpenModeExstFile);
}

FileEditor::FileEditor(FileHolderPtr NewFileHolder, UINT codepage, DWORD InitFlags, int StartLine, int StartChar,
		const wchar_t *Title, int X1, int Y1, int X2, int Y2, int OpenModeExstFile)
	:
	SaveAsTextFormat(0)
{
	Flags.Set(InitFlags);

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

	ScreenObject::SetPosition(X1, Y1, X2, Y2);
	Flags.Change(FFILEEDIT_FULLSCREEN, (!X1 && !Y1 && X2 == ScrX && Y2 == ScrY));
	Init(NewFileHolder, codepage, Title, InitFlags, StartLine, StartChar, L"", OpenModeExstFile);
}

/*
	$ 07.05.2001 DJ
	в деструкторе грохаем EditNamesList, если он был создан, а в SetNamesList()
	создаем EditNamesList и копируем туда значения
*/
/*
	Вызов деструкторов идет так:
		FileEditor::~FileEditor()
		Editor::~Editor()
		...
*/
FileEditor::~FileEditor()
{
	// AY: флаг оповещающий закрытие редактора.
	m_bClosing = true;

	if (CtrlObject) {
		if (m_editor->EdOpt.SavePos)
			SaveToCache();
		else
			CtrlObject->EditorPosCache->ResetPosition(ComposeCacheName());
	}

	BitFlags FEditFlags = m_editor->Flags;
	int FEditEditorID = m_editor->EditorID;

	if (bEE_READ_Sent && CtrlObject) {
		FileEditor *save = CtrlObject->Plugins.CurEditor;
		CtrlObject->Plugins.CurEditor = this;
		CtrlObject->Plugins.ProcessEditorEvent(EE_CLOSE, &FEditEditorID);
		CtrlObject->Plugins.CurEditor = save;
	}

	delete m_editor;

	m_editor = nullptr;
	CurrentEditor = nullptr;

	delete EditNamesList;
}

void FileEditor::Init(FileHolderPtr NewFileHolder, UINT codepage, const wchar_t *Title, DWORD InitFlags,
		int StartLine, int StartChar, const wchar_t *PluginData, int OpenModeExstFile)
{
	SudoClientRegion sdc_rgn;
	class SmartLock
	{
	private:
		Editor *editor;

	public:
		SmartLock() { editor = nullptr; }
		~SmartLock()
		{
			if (editor)
				editor->Unlock();
		}

		void Set(Editor *e)
		{
			editor = e;
			editor->Lock();
		}
	};
	SmartLock __smartlock;
	SysErrorCode = 0;
	const auto *Name = NewFileHolder->GetPathName().CPtr();
	int BlankFileName = !StrCmp(Name, Msg::NewFileName);
	// AY: флаг оповещающий закрытие редактора.
	m_bClosing = false;
	bEE_READ_Sent = false;
	m_AddSignature = FB_NO;
	m_editor = new Editor;

	if (!m_editor) {
		ExitCode = XC_OPEN_ERROR;
		return;
	}
	__smartlock.Set(m_editor);

	FHP = NewFileHolder;
	m_codepage = codepage;
	m_editor->SetOwner(this);
	m_editor->SetCodePage(m_codepage);
	AttrStr.Clear();
	CurrentEditor = this;
	SetTitle(Title);
	EditNamesList = nullptr;
	KeyBarVisible = Opt.EdOpt.ShowKeyBar;
	TitleBarVisible = Opt.EdOpt.ShowTitleBar;
	// $ 17.08.2001 KM - Добавлено для поиска по AltF7. При редактировании найденного файла из архива для клавиши F2 сделать вызов ShiftF2.
	Flags.Change(FFILEEDIT_SAVETOSAVEAS,
			(InitFlags & FFILEEDIT_SAVETOSAVEAS) == FFILEEDIT_SAVETOSAVEAS || BlankFileName != 0);

	if (!*Name) {
		ExitCode = XC_OPEN_ERROR;
		return;
	}

	SetPluginData(PluginData);
	m_editor->SetHostFileEditor(this);
	SetCanLoseFocus(Flags.Check(FFILEEDIT_ENABLEF6));
	apiGetCurrentDirectory(strStartDir);

	if (!SetFileName(Name)) {
		ExitCode = XC_OPEN_ERROR;
		return;
	}

	// int FramePos=FrameManager->FindFrameByFile(MODALTYPE_EDITOR,FullFileName);
	// if (FramePos!=-1)
	if (Flags.Check(FFILEEDIT_ENABLEF6)) {
		// if (Flags.Check(FFILEEDIT_ENABLEF6))
		int FramePos = FrameManager->FindFrameByFile(MODALTYPE_EDITOR, strFullFileName);

		if (FramePos != -1) {
			int SwitchTo = FALSE;
			int MsgCode = 0;

			if (!(*FrameManager)[FramePos]->GetCanLoseFocus(TRUE) || Opt.Confirm.AllowReedit) {
				if (OpenModeExstFile == FEOPMODE_QUERY) {
					SetMessageHelp(L"EditorReload");
					MsgCode = Message(0, 3, Msg::EditTitle, strFullFileName, Msg::AskReload, Msg::Current,
							Msg::NewOpen, Msg::Reload);
				} else {
					MsgCode = (OpenModeExstFile == FEOPMODE_USEEXISTING)
							? 0
							: (OpenModeExstFile == FEOPMODE_NEWIFOPEN
											? 1
											: (OpenModeExstFile == FEOPMODE_RELOAD ? 2 : -100));
				}

				switch (MsgCode) {
					case 0:									// Current
						SwitchTo = TRUE;
						FrameManager->DeleteFrame(this);	//???
						break;
					case 1:									// NewOpen
						SwitchTo = FALSE;
						break;
					case 2:		// Reload
						FrameManager->DeleteFrame(FramePos);
						SetExitCode(-2);
						break;
					case -100:
						// FrameManager->DeleteFrame(this); //???
						SetExitCode(XC_EXISTS);
						return;
					default:
						FrameManager->DeleteFrame(this);	//???
						SetExitCode(XC_QUIT);
						return;
				}
			} else {
				SwitchTo = TRUE;
			}

			if (SwitchTo) {
				FrameManager->ActivateFrame(FramePos);
				// FrameManager->PluginCommit();
				SetExitCode((OpenModeExstFile != FEOPMODE_QUERY) ? XC_EXISTS : TRUE);
				return;
			}
		}
	}

	/*
		$ 29.11.2000 SVS
		Если файл имеет атрибут ReadOnly или System или Hidden,
		И параметр на запрос выставлен, то сначала спросим.
		$ 03.12.2000 SVS
		System или Hidden - задаются отдельно
		$ 15.12.2000 SVS
		- Shift-F4, новый файл. Выдает сообщение :-(
	*/
	DWORD FAttr = apiGetFileAttributes(Name);

	/*
		$ 05.06.2001 IS
		+ посылаем подальше всех, кто пытается отредактировать каталог
	*/
	if (FAttr != INVALID_FILE_ATTRIBUTES && FAttr & FILE_ATTRIBUTE_DIRECTORY) {
		Message(MSG_WARNING, 1, Msg::EditTitle, Msg::EditCanNotEditDirectory, Msg::Ok);
		ExitCode = XC_OPEN_ERROR;
		return;
	}

	if ((m_editor->EdOpt.ReadOnlyLock & 2) && FAttr != INVALID_FILE_ATTRIBUTES
			&& (FAttr
					& (FILE_ATTRIBUTE_READONLY |
							/*
								Hidden=0x2 System=0x4 - располагаются во 2-м полубайте,
								поэтому применяем маску 0110.0000 и
								сдвигаем на свое место => 0000.0110 и получаем
								те самые нужные атрибуты
							*/
							((m_editor->EdOpt.ReadOnlyLock & 0x60) >> 4)))) {
		if (Message(MSG_WARNING, 2, Msg::EditTitle, Name, Msg::EditRSH, Msg::EditROOpen, Msg::Yes, Msg::No)) {
			ExitCode = XC_OPEN_ERROR;
			return;
		}
	}

	m_editor->SetPosition(X1, Y1 + (TitleBarVisible ? 1 : 0), X2, Y2 - (KeyBarVisible ? 1 : 0));
	m_editor->SetStartPos(StartLine, StartChar);
	int UserBreak;

	/*
		$ 06.07.2001 IS
		При создании файла с нуля так же посылаем плагинам событие EE_READ, дабы
		не нарушать однообразие.
	*/
	if (FAttr == INVALID_FILE_ATTRIBUTES)
		Flags.Set(FFILEEDIT_NEW);

	if (BlankFileName && Flags.Check(FFILEEDIT_CANNEWFILE))
		Flags.Set(FFILEEDIT_NEW);

	if (Flags.Check(FFILEEDIT_NEW))
		m_AddSignature = FB_MAYBE;

	if (Flags.Check(FFILEEDIT_LOCKED))
		m_editor->Flags.Set(FEDITOR_LOCKMODE);

	if (!LoadFile(strFullFileName, UserBreak)) {
		if (BlankFileName) {
			Flags.Clear(FFILEEDIT_OPENFAILED);	// AY: ну так как редактор мы открываем то видимо надо и сбросить ошибку открытия
			UserBreak = 0;
		}

		if (!Flags.Check(FFILEEDIT_NEW) || UserBreak) {
			if (UserBreak != 1) {
				WINPORT(SetLastError)(SysErrorCode);
				Message(MSG_WARNING | MSG_ERRORTYPE, 1, Msg::EditTitle, Msg::EditCannotOpen, strFileName,
						Msg::Ok);
				ExitCode = XC_OPEN_ERROR;
			} else {
				ExitCode = XC_LOADING_INTERRUPTED;
			}

			// Ахтунг. Ниже комментарии оставлены в назидании потомкам (до тех пор, пока не измениться манагер)
			// FrameManager->DeleteFrame(this); // BugZ#546 - Editor валит фар!
			// CtrlObject->Cp()->Redraw(); //AY: вроде как не надо, делает проблемы с прорисовкой если в редакторе из истории попытаться выбрать несуществующий файл

			// если прервали загрузку, то фремы нужно проапдейтить, чтобы предыдущие месаги не оставались на экране
			if (!Opt.Confirm.Esc && UserBreak && ExitCode == XC_LOADING_INTERRUPTED && FrameManager)
				FrameManager->RefreshFrame();

			return;
		}

		if (m_codepage == CP_AUTODETECT)
			m_codepage = Opt.EdOpt.DefaultCodePage;

		m_editor->SetCodePage(m_codepage);
	}

	CtrlObject->Plugins.CurEditor = this;	//&FEdit;
	CtrlObject->Plugins.ProcessEditorEvent(EE_READ, nullptr);
	bEE_READ_Sent = true;
	ShowConsoleTitle();
	EditKeyBar.SetOwner(this);
	EditKeyBar.SetPosition(X1, Y2, X2, Y2);
	InitKeyBar();

	if (!KeyBarVisible)
		EditKeyBar.Hide0();

	MacroMode = MACRO_EDITOR;
	CtrlObject->Macro.SetMode(MACRO_EDITOR);

	F4KeyOnly = true;

	if (Flags.Check(FFILEEDIT_ENABLEF6))
		FrameManager->InsertFrame(this);
	else
		FrameManager->ExecuteFrame(this);
}

void FileEditor::InitKeyBar()
{
	EditKeyBar.SetAllGroup(KBL_MAIN, Opt.OnlyEditorViewerUsed ? Msg::SingleEditF1 : Msg::EditF1, 12);
	EditKeyBar.SetAllGroup(KBL_SHIFT, Opt.OnlyEditorViewerUsed ? Msg::SingleEditShiftF1 : Msg::EditShiftF1,
			12);
	EditKeyBar.SetAllGroup(KBL_ALT, Opt.OnlyEditorViewerUsed ? Msg::SingleEditAltF1 : Msg::EditAltF1, 12);
	EditKeyBar.SetAllGroup(KBL_CTRL, Opt.OnlyEditorViewerUsed ? Msg::SingleEditCtrlF1 : Msg::EditCtrlF1, 12);
	EditKeyBar.SetAllGroup(KBL_CTRLSHIFT,
			Opt.OnlyEditorViewerUsed ? Msg::SingleEditCtrlShiftF1 : Msg::EditCtrlShiftF1, 12);
	EditKeyBar.SetAllGroup(KBL_CTRLALT,
			Opt.OnlyEditorViewerUsed ? Msg::SingleEditCtrlAltF1 : Msg::EditCtrlAltF1, 12);
	EditKeyBar.SetAllGroup(KBL_ALTSHIFT,
			Opt.OnlyEditorViewerUsed ? Msg::SingleEditAltShiftF1 : Msg::EditAltShiftF1, 12);
	EditKeyBar.SetAllGroup(KBL_CTRLALTSHIFT,
			Opt.OnlyEditorViewerUsed ? Msg::SingleEditCtrlAltShiftF1 : Msg::EditCtrlAltShiftF1, 12);

	if (!GetCanLoseFocus())
		EditKeyBar.Change(KBL_SHIFT, L"", 4 - 1);

	if (Flags.Check(FFILEEDIT_SAVETOSAVEAS))
		EditKeyBar.Change(KBL_MAIN, Msg::EditShiftF2, 2 - 1);

	if (!Flags.Check(FFILEEDIT_ENABLEF6))
		EditKeyBar.Change(KBL_MAIN, L"", 6 - 1);

	if (!GetCanLoseFocus())
		EditKeyBar.Change(KBL_MAIN, L"", 12 - 1);

	if (!GetCanLoseFocus())
		EditKeyBar.Change(KBL_ALT, L"", 11 - 1);

	if (!Opt.UsePrintManager || CtrlObject->Plugins.FindPlugin(SYSID_PRINTMANAGER))
		EditKeyBar.Change(KBL_ALT, L"", 5 - 1);

	SetEditKeyBarStatefulLabels();

	EditKeyBar.ReadRegGroup(L"Editor", Opt.strLanguage);
	EditKeyBar.SetAllRegGroup();
	EditKeyBar.Refresh(true);
	m_editor->SetPosition(X1, Y1 + (TitleBarVisible ? 1 : 0), X2, Y2 - (KeyBarVisible ? 1 : 0));
	SetKeyBar(&EditKeyBar);
}

void FileEditor::SetNamesList(NamesList *Names)
{
	if (!EditNamesList)
		EditNamesList = new NamesList;

	Names->MoveData(*EditNamesList);
}

void FileEditor::Show()
{
	if (Flags.Check(FFILEEDIT_FULLSCREEN)) {
		if (KeyBarVisible) {
			EditKeyBar.SetPosition(0, ScrY, ScrX, ScrY);
			EditKeyBar.Redraw();
		}

		ScreenObject::SetPosition(0, 0, ScrX, ScrY - (KeyBarVisible ? 1 : 0));
		m_editor->SetPosition(0, (TitleBarVisible ? 1 : 0), ScrX, ScrY - (KeyBarVisible ? 1 : 0));
	}

	ScreenObject::Show();
}

void FileEditor::DisplayObject()
{
	if (!m_editor->Locked()) {
		if (m_editor->Flags.Check(FEDITOR_ISRESIZEDCONSOLE)) {
			m_editor->Flags.Clear(FEDITOR_ISRESIZEDCONSOLE);
			CtrlObject->Plugins.CurEditor = this;
			CtrlObject->Plugins.ProcessEditorEvent(EE_REDRAW, EEREDRAW_CHANGE);		// EEREDRAW_ALL);
		}

		m_editor->Show();
	}
}

int64_t FileEditor::VMProcess(MacroOpcode OpCode, void *vParam, int64_t iParam)
{
	if (OpCode == MCODE_V_EDITORSTATE) {
		DWORD MacroEditState = 0;
		MacroEditState|= Flags.Flags & FFILEEDIT_NEW ? 0x00000001 : 0;
		MacroEditState|= Flags.Flags & FFILEEDIT_ENABLEF6 ? 0x00000002 : 0;
		MacroEditState|= m_editor->Flags.Flags & FEDITOR_MODIFIED ? 0x00000008 : 0;
		MacroEditState|= m_editor->BlockStart ? 0x00000010 : 0;
		MacroEditState|= m_editor->VBlockStart ? 0x00000020 : 0;
		MacroEditState|= m_editor->Flags.Flags & FEDITOR_WASCHANGED ? 0x00000040 : 0;
		MacroEditState|= m_editor->Flags.Flags & FEDITOR_OVERTYPE ? 0x00000080 : 0;
		MacroEditState|= m_editor->Flags.Flags & FEDITOR_CURPOSCHANGEDBYPLUGIN ? 0x00000100 : 0;
		MacroEditState|= m_editor->Flags.Flags & FEDITOR_LOCKMODE ? 0x00000200 : 0;
		MacroEditState|= m_editor->EdOpt.PersistentBlocks ? 0x00000400 : 0;
		MacroEditState|= Opt.OnlyEditorViewerUsed ? 0x08000000 | 0x00000800 : 0;
		MacroEditState|= !GetCanLoseFocus() ? 0x00000800 : 0;
		return (int64_t)MacroEditState;
	}

	if (OpCode == MCODE_V_EDITORCURPOS)
		return (int64_t)(m_editor->CurLine->GetCellCurPos() + 1);

	if (OpCode == MCODE_V_EDITORCURLINE)
		return (int64_t)(m_editor->NumLine + 1);

	if (OpCode == MCODE_V_ITEMCOUNT || OpCode == MCODE_V_EDITORLINES)
		return (int64_t)(m_editor->NumLastLine);

	return m_editor->VMProcess(OpCode, vParam, iParam);
}

int FileEditor::ProcessKey(FarKey Key)
{
	return ReProcessKey(Key, FALSE);
}

static void EditorConfigOrgConflictMessage(const FARString &value, const struct FarLangMsg &problem)
{
	FARString disable_line1, disable_line2;
	disable_line1 = Msg::EditorConfigOrgDisable;
	size_t p;
	if (disable_line1.Pos(p, '\n')) {
		disable_line2 = disable_line1.SubStr(p + 1);
		disable_line1.Truncate(p);
	}
	Message(MSG_WARNING, 1, Msg::EditorConfigOrgConflict,
		Msg::EditorConfigOrgFile, value, problem, L"", disable_line1, disable_line2, Msg::Ok);
}

int FileEditor::ReProcessKey(FarKey Key, int CalledFromControl)
{
	SudoClientRegion sdc_rgn;
	if (Key != KEY_F4 && Key != KEY_IDLE)
		F4KeyOnly = false;

	if (Flags.Check(FFILEEDIT_REDRAWTITLE)
			&& (((unsigned int)Key & 0x00ffffff) < KEY_END_FKEY
					|| IS_INTERNAL_KEY_REAL((unsigned int)Key & 0x00ffffff)))
		ShowConsoleTitle();

	// BugZ#488 - Shift=enter
	if (ShiftPressed && (Key == KEY_ENTER || Key == KEY_NUMENTER)
			&& CtrlObject->Macro.IsExecuting() == MACROMODE_NOMACRO) {
		Key = Key == KEY_ENTER ? KEY_SHIFTENTER : KEY_SHIFTNUMENTER;
	}

	/*
		Все сотальные необработанные клавиши пустим далее
		$ 28.04.2001 DJ
		не передаем KEY_MACRO* плагину - поскольку ReadRec в этом случае
		никак не соответствует обрабатываемой клавише, возникают разномастные
		глюки
	*/
	if (((unsigned int)Key >= KEY_MACRO_BASE && (unsigned int)Key <= KEY_MACRO_ENDBASE)
			|| ((unsigned int)Key >= KEY_OP_BASE && (unsigned int)Key <= KEY_OP_ENDBASE))		// исключаем MACRO
	{
		;
	}

	switch (Key) {
		/*
			$ 27.09.2000 SVS
			Печать файла/блока с использованием плагина PrintMan
		*/
		case KEY_ALTF5: {
			if (Opt.UsePrintManager && CtrlObject->Plugins.FindPlugin(SYSID_PRINTMANAGER)) {
				CtrlObject->Plugins.CallPlugin(SYSID_PRINTMANAGER, OPEN_EDITOR, nullptr);	// printman
				return TRUE;
			}

			break;	// отдадим Alt-F5 на растерзание плагинам, если не установлен PrintMan
		}
		case KEY_F6: {
			if (Flags.Check(FFILEEDIT_ENABLEF6)) {
				int FirstSave = 1, NeedQuestion = 1;
				UINT cp = m_codepage;

				// проверка на "а может это говно удалили уже?"
				// возможно здесь она и не нужна!
				// хотя, раз уж были изменени, то
				if (m_editor->IsFileChanged() &&												// в текущем сеансе были изменения?
						apiGetFileAttributes(strFullFileName) == INVALID_FILE_ATTRIBUTES)		// а файл еще существует?
				{
					switch (Message(MSG_WARNING, 2, Msg::EditTitle, Msg::EditSavedChangedNonFile,
							Msg::EditSavedChangedNonFile2, Msg::HYes, Msg::HNo)) {
						case 0:

							if (ProcessKey(KEY_F2)) {
								FirstSave = 0;
								break;
							}

						default:
							return FALSE;
					}
				}

				if (!FirstSave || m_editor->IsFileChanged()
						|| apiGetFileAttributes(strFullFileName) != INVALID_FILE_ATTRIBUTES) {
					long FilePos = m_editor->GetCurPos();

					/*
						$ 01.02.2001 IS
						! Открываем вьюер с указанием длинного имени файла, а не короткого
					*/
					if (ProcessQuitKey(FirstSave, NeedQuestion)) {
						// объект будет в конце удалён в FrameManager
						auto *Viewer = new FileViewer(FHP, GetCanLoseFocus(),
								Flags.Check(FFILEEDIT_DISABLEHISTORY), FALSE, FilePos, nullptr, EditNamesList,
								Flags.Check(FFILEEDIT_SAVETOSAVEAS), cp);
						Viewer->SetPluginData(strPluginData);
					}

					ShowTime(2);
				}

				return TRUE;
			}

			break;	// отдадим F6 плагинам, если есть запрет на переключение
		}
		/*
			$ 10.05.2001 DJ
			Alt-F11 - показать view/edit history
		*/
		case KEY_ALTF11: {
			if (GetCanLoseFocus()) {
				CtrlObject->CmdLine->ShowViewEditHistory();
				return TRUE;
			}

			break;	// отдадим Alt-F11 на растерзание плагинам, если редактор модальный
		}
	}

#if 1
	BOOL ProcessedNext = TRUE;

	_SVS(if (Key == 'n' || Key == 'm'))
	_SVS(SysLog(L"%d Key='%c'", __LINE__, Key));

	if (!CalledFromControl
			&& (CtrlObject->Macro.IsRecording() == MACROMODE_RECORDING_COMMON
					|| CtrlObject->Macro.IsExecuting() == MACROMODE_EXECUTING_COMMON
					|| CtrlObject->Macro.GetCurRecord(nullptr, nullptr) == MACROMODE_NOMACRO)) {

		_SVS(if (CtrlObject->Macro.IsRecording() == MACROMODE_RECORDING_COMMON
				|| CtrlObject->Macro.IsExecuting() == MACROMODE_EXECUTING_COMMON))
		_SVS(SysLog(L"%d !!!! CtrlObject->Macro.GetCurRecord(nullptr,nullptr) != MACROMODE_NOMACRO !!!!",
				__LINE__));

		ProcessedNext = !ProcessEditorInput(FrameManager->GetLastInputRecord());
	}

	if (ProcessedNext)
#else
	if (!CalledFromControl &&		// CtrlObject->Macro.IsExecuting() || CtrlObject->Macro.IsRecording() || // пусть доходят!
			!ProcessEditorInput(FrameManager->GetLastInputRecord()))
#endif
	{

		switch (Key) {
			case KEY_F1: {
				Help::Present(L"Editor");
				return TRUE;
			}
			/*
				$ 25.04.2001 IS
				ctrl+f - вставить в строку полное имя редактируемого файла
			*/
			case KEY_CTRLF: {
				if (!m_editor->Flags.Check(FEDITOR_LOCKMODE)) {
					m_editor->Pasting++;
					m_editor->TextChanged(1);
					BOOL IsBlock = m_editor->VBlockStart || m_editor->BlockStart;

					if (!m_editor->EdOpt.PersistentBlocks && IsBlock) {
						m_editor->Flags.Clear(FEDITOR_MARKINGVBLOCK | FEDITOR_MARKINGBLOCK);
						m_editor->DeleteBlock();
					}

					// AddUndoData(CurLine->EditLine.GetStringAddr(),NumLine,
					// CurLine->EditLine.GetCurPos(),UNDO_EDIT);
					m_editor->Paste(strFullFileName);	//???
					// if (!EdOpt.PersistentBlocks)
					m_editor->UnmarkBlock();
					m_editor->Pasting--;
					m_editor->Show();	//???
				}

				return (TRUE);
			}
			/*
				$ 24.08.2000 SVS
				+ Добавляем реакцию показа бакграунда на клавишу CtrlAltShift
			*/
			case KEY_CTRLO: {
				if (!Opt.OnlyEditorViewerUsed) {
					m_editor->Hide();	// $ 27.09.2000 skv - To prevent redraw in macro with Ctrl-O

					if (FrameManager->ShowBackground()) {
						SetCursorType(FALSE, 0);
						WaitKey();
					}

					Show();
				}

				return TRUE;
			}
			case KEY_F2:
			case KEY_SHIFTF2: {
				BOOL Done = FALSE;
				FARString strOldCurDir;
				apiGetCurrentDirectory(strOldCurDir);

				while (!Done)		// бьемся до упора
				{
					if (Key == KEY_F2 && apiPathIsFile(strFullFileName)) {
						Flags.Clear(FFILEEDIT_SAVETOSAVEAS);
					} else if (!Flags.Check(FFILEEDIT_SAVETOSAVEAS)) {
						FARString strDir = strFullFileName;
						if (CutToSlash(strDir, false)			// проверим путь к файлу, может его уже снесли...
								&& !IsLocalRootPath(strDir)		// В корне?
								&& !apiPathIsDir(strDir))		// каталог существует?
						{
							Flags.Set(FFILEEDIT_SAVETOSAVEAS);
						}
					}

					UINT codepage = m_codepage;
					bool SaveAs = Key == KEY_SHIFTF2 || Flags.Check(FFILEEDIT_SAVETOSAVEAS);
					int NameChanged = FALSE;
					FARString strFullSaveAsName = strFullFileName;

					if (SaveAs) {
						FARString strSaveAsName =
								Flags.Check(FFILEEDIT_SAVETOSAVEAS) ? strFullFileName : strFileName;

						bool AddSignature = DecideAboutSignature();
						if (!dlgSaveFileAs(strSaveAsName, SaveAsTextFormat, codepage, AddSignature))
							return FALSE;

						m_AddSignature = AddSignature ? FB_YES : FB_NO;

						apiExpandEnvironmentStrings(strSaveAsName, strSaveAsName);
						Unquote(strSaveAsName);
						NameChanged = StrCmpI(strSaveAsName,
								(Flags.Check(FFILEEDIT_SAVETOSAVEAS) ? strFullFileName : strFileName));

						if (!NameChanged)
							FarChDir(strStartDir);	// ПОЧЕМУ? А нужно ли???

						if (NameChanged) {
							if (!AskOverwrite(strSaveAsName)) {
								FarChDir(strOldCurDir);
								return TRUE;
							}
						}

						ConvertNameToFull(strSaveAsName, strFullSaveAsName);	// BUGBUG, не проверяем имя на правильность
						// это не про нас, про нас ниже, все куда страшнее
						/*FARString strFileNameTemp = strSaveAsName;

						if(!SetFileName(strFileNameTemp))
						{
							WINPORT(SetLastError)(ERROR_INVALID_NAME);
								Message(MSG_WARNING|MSG_ERRORTYPE,1,Msg::EditTitle,strFileNameTemp,Msg::Ok);
							if(!NameChanged)
								FarChDir(strOldCurDir);
							continue;
							//return FALSE;
						} */

						if (!NameChanged)
							FarChDir(strOldCurDir);
					}

					ShowConsoleTitle();
					FarChDir(strStartDir);	//???
					int SaveResult = SaveFile(strFullSaveAsName, 0, SaveAs, SaveAsTextFormat, codepage,
							DecideAboutSignature());

					if (SaveResult == SAVEFILE_ERROR) {
						WINPORT(SetLastError)(SysErrorCode);

						if (Message(MSG_WARNING | MSG_ERRORTYPE, 2, Msg::EditTitle, Msg::EditCannotSave,
									strFileName, Msg::Retry, Msg::Cancel)) {
							Done = TRUE;
							break;
						}
					} else if (SaveResult == SAVEFILE_SUCCESS) {
						// здесь идет полная жопа, проверка на ошибки вообще пока отсутствует
						{
							bool bInPlace =
									/*(!IsUnicodeOrUtfCodePage(m_codepage) && !IsUnicodeOrUtfCodePage(codepage)) || */
									(m_codepage == codepage);

							if (!bInPlace) {
								m_editor->FreeAllocatedData();
								m_editor->InsertString(nullptr, 0);
							}

							SetFileName(strFullSaveAsName);
							SetCodePage(codepage);

							if (!bInPlace) {
								Message(MSG_WARNING, 1, L"WARNING!",
										L"Editor will be reopened with new file!", Msg::Ok);
								if (!ReloadFile(strFullSaveAsName))
									return TRUE;
								// TODO: возможно подобный ниже код здесь нужен (copy/paste из FileEditor::Init()). оформить его нужно по иному
								// if(!Opt.Confirm.Esc && UserBreak && ExitCode==XC_LOADING_INTERRUPTED && FrameManager)
								// FrameManager->RefreshFrame();
							}

							// перерисовывать надо как минимум когда изменилась кодировка или имя файла
							ShowConsoleTitle();
							Show();		//!!! BUGBUG
						}
						Done = TRUE;
					} else {
						Done = TRUE;
						break;
					}
				}

				return TRUE;
			}
			// $ 30.05.2003 SVS - Shift-F4 в редакторе/вьювере позволяет открывать другой редактор/вьювер (пока только редактор)
			case KEY_SHIFTF4: {
				if (!Opt.OnlyEditorViewerUsed && GetCanLoseFocus())
					CtrlObject->Cp()->ActivePanel->ProcessKey(Key);

				return TRUE;
			}
			// $ 21.07.2000 SKV + выход с позиционированием на редактируемом файле по CTRLF10
			case KEY_CTRLF10: {
				if (isTemporary()) {
					return TRUE;
				}

				FARString strFullFileNameTemp = strFullFileName;

				if (apiGetFileAttributes(strFullFileName) == INVALID_FILE_ATTRIBUTES)		// а сам файл то еще на месте?
				{
					if (!CheckShortcutFolder(&strFullFileNameTemp, FALSE))
						return FALSE;

					strFullFileNameTemp+= L"/.";	// для вваливания внутрь :-)
				}

				Panel *ActivePanel = CtrlObject->Cp()->ActivePanel;

				if (Flags.Check(FFILEEDIT_NEW)
						|| (ActivePanel && ActivePanel->FindFile(strFileName) == -1))		// Mantis#279
				{
					UpdateFileList();
					Flags.Clear(FFILEEDIT_NEW);
				}

				{
					SaveScreen Sc;
					CtrlObject->Cp()->GoToFile(strFullFileNameTemp);
					Flags.Set(FFILEEDIT_REDRAWTITLE);
				}

				return (TRUE);
			}
			case KEY_ALTF10: {
				FrameManager->ExitMainLoop(1);
				return (TRUE);
			}
			case KEY_CTRLB: {
				KeyBarVisible = !KeyBarVisible;

				if (!KeyBarVisible)
					EditKeyBar.Hide0();		// 0 mean - Don't purge saved screen

				EditKeyBar.Refresh(KeyBarVisible);
				Show();
				return (TRUE);
			}
			case KEY_CTRLSHIFTB: {
				TitleBarVisible = !TitleBarVisible;
				Show();
				return (TRUE);
			}
			case KEY_F5:
				m_editor->SetShowWhiteSpace(m_editor->GetShowWhiteSpace() ? 0 : 1);
				m_editor->Show();
				ChangeEditKeyBar();
				return TRUE;

			case KEY_SHIFTF5:
				if (EdCfg && EdCfg->TabSize > 0) {
					FARString strTmp; strTmp.Format(Msg::EditorConfigOrgValueOfIndentSize, EdCfg->TabSize);
					EditorConfigOrgConflictMessage(strTmp, Msg::EditorConfigOrgProblemIndentSize);
					return TRUE;
				}
				ChooseTabSizeMenu();
				ShowStatus();
				return TRUE;

			case KEY_CTRLF5:
				if (EdCfg && EdCfg->ExpandTabs >= 0) {
					FARString strTmp; strTmp.Format(Msg::EditorConfigOrgValueOfIndentStyle,
						EdCfg->ExpandTabs==EXPAND_NOTABS ? "tab" : EdCfg->ExpandTabs==EXPAND_NEWTABS ? "space" : "????" );
					EditorConfigOrgConflictMessage(strTmp, Msg::EditorConfigOrgProblemIndentStyle);
					return TRUE;
				}
				m_editor->SetConvertTabs(
						(m_editor->GetConvertTabs() != EXPAND_NOTABS) ? EXPAND_NOTABS : EXPAND_NEWTABS);
				m_editor->EnableSaveTabSettings();
				ChangeEditKeyBar();
				ShowStatus();
				return TRUE;

			case KEY_SHIFTF10:
				if (!ProcessKey(KEY_F2))	// учтем факт того, что могли отказаться от сохранения
					return FALSE;

			case KEY_F4:
				if (F4KeyOnly)
					return TRUE;

			case KEY_ESC:
			case KEY_F10: {
				int FirstSave = 1, NeedQuestion = 1;

				if (Key != KEY_SHIFTF10)	// KEY_SHIFTF10 не учитываем!
				{
					bool FilePlaced = apiGetFileAttributes(strFullFileName) == INVALID_FILE_ATTRIBUTES
							&& !Flags.Check(FFILEEDIT_NEW);

					if (m_editor->IsFileChanged() ||	// в текущем сеансе были изменения?
							FilePlaced)					// а сам файл то еще на месте?
					{
						int Res;

						if (m_editor->IsFileChanged() && FilePlaced)
							Res = Message(MSG_WARNING, 3, Msg::EditTitle, Msg::EditSavedChangedNonFile,
									Msg::EditSavedChangedNonFile2, Msg::HYes, Msg::HNo, Msg::HCancel);
						else if (!m_editor->IsFileChanged() && FilePlaced)
							Res = Message(MSG_WARNING, 3, Msg::EditTitle, Msg::EditSavedChangedNonFile1,
									Msg::EditSavedChangedNonFile2, Msg::HYes, Msg::HNo, Msg::HCancel);
						else
							Res = 100;

						switch (Res) {
							case 0:

								if (!ProcessKey(KEY_F2))	// попытка сначала сохранить
									NeedQuestion = 0;

								FirstSave = 0;
								break;
							case 1:
								NeedQuestion = 0;
								FirstSave = 0;
								break;
							case 100:
								FirstSave = NeedQuestion = 1;
								break;
							case 2:
							default:
								return FALSE;
						}
					} else if (!m_editor->Flags.Check(FEDITOR_MODIFIED))	//????
						NeedQuestion = 0;
				}

				if (!ProcessQuitKey(FirstSave, NeedQuestion))
					return FALSE;

				return TRUE;
			}
			case KEY_F8:
			case KEY_SHIFTF8: {
				if (EdCfg && EdCfg->CodePage > 0) {
					FARString strTmp;
					strTmp.Format(Msg::EditorConfigOrgValueOfCharset, EdCfg->CodePage);
					EditorConfigOrgConflictMessage(strTmp, Msg::EditorConfigOrgProblemCharset);
					return TRUE;
				}
				UINT codepage;
				if (Key == KEY_F8) {
					//codepage = (m_codepage == WINPORT(GetACP)() ? WINPORT(GetOEMCP)() : WINPORT(GetACP)());
					if (m_codepage == CP_UTF8)
						codepage = WINPORT(GetACP)();
					else if (m_codepage == WINPORT(GetACP)() )
						codepage = WINPORT(GetOEMCP)();
					else // if (m_codepage == WINPORT(GetOEMCP)() )
						codepage = CP_UTF8;
				} else {
					codepage = SelectCodePage(m_codepage, false, true, false, true);
					if (codepage == CP_AUTODETECT) {
						if (!GetFileFormat2(strFileName, codepage, nullptr, true, true)) {
							codepage = (UINT)-1;
						}
					}
				}
				if (codepage != (UINT)-1 && codepage != m_codepage) {
					const bool need_reload = 0
							//								|| IsFixedSingleCharCodePage(m_codepage) != IsFixedSingleCharCodePage(codepage)
							|| IsUTF8(m_codepage) != IsUTF8(codepage)
							|| IsUTF7(m_codepage) != IsUTF7(codepage)
							|| IsUTF16(m_codepage) != IsUTF16(codepage)
							|| IsUTF32(m_codepage) != IsUTF32(codepage);
					if ((!IsFileModified() || !need_reload)
							&& apiGetFileAttributes(strLoadedFileName) != INVALID_FILE_ATTRIBUTES) {	// а файл еще существует?
						Flags.Set(FFILEEDIT_CODEPAGECHANGEDBYUSER);
						if (need_reload) {
							m_codepage = codepage;
							SaveToCache();
							if (!ReloadFile(strLoadedFileName))
								return TRUE;
						} else {
							SetCodePage(codepage);
						}
						ChangeEditKeyBar();
					} else {
						FARString str_tmp, str_from, str_to;
						ShortReadableCodepageName(m_codepage,str_from);
						ShortReadableCodepageName(codepage,str_to);
						str_tmp.Format(L"Save file before changing codepage from %ls to %ls", str_from.CPtr(), str_to.CPtr());
						Message(0, 1, Msg::EditTitle, str_tmp.CPtr(), Msg::HOk, nullptr);
					}
				}
				return TRUE;
			}
			case KEY_F9:
			case KEY_ALTSHIFTF9: {
				// Работа с локальной копией EditorOptions
				EditorOptions EdOpt;
				GetEditorOptions(EdOpt);
				EditorOptions SavedEdOpt = EdOpt;
				//EditorConfig(EdOpt, true);	// $ 27.11.2001 DJ - Local в EditorConfig
				EditorConfig(EdOpt, true,	// $ 27.11.2001 DJ - Local в EditorConfig
					EdCfg ? EdCfg->ExpandTabs : -1,
					EdCfg ? EdCfg->TabSize : -1);
				EditKeyBar.Refresh(true);	//???? Нужно ли????
				SetEditorOptions(EdOpt);
				if (SavedEdOpt.TabSize != EdOpt.TabSize || SavedEdOpt.ExpandTabs != EdOpt.ExpandTabs)
					m_editor->EnableSaveTabSettings();
				EditKeyBar.Refresh(KeyBarVisible);
				if (!KeyBarVisible)
					EditKeyBar.Hide0();
				Show();
				m_editor->Show();
				return TRUE;
			}
			default: {
				if (Flags.Check(FFILEEDIT_FULLSCREEN) && CtrlObject->Macro.IsExecuting() == MACROMODE_NOMACRO)
					EditKeyBar.Refresh(KeyBarVisible);

				if (!EditKeyBar.ProcessKey(Key))
					return (m_editor->ProcessKey(Key));
			}
		}
	}
	return TRUE;
}

int FileEditor::ProcessQuitKey(int FirstSave, BOOL NeedQuestion)
{
	SudoClientRegion sdc_rgn;
	FARString strOldCurDir;
	apiGetCurrentDirectory(strOldCurDir);

	for (;;) {
		FarChDir(strStartDir);	// ПОЧЕМУ? А нужно ли???
		int SaveCode = SAVEFILE_SUCCESS;

		if (NeedQuestion) {
			SaveCode = SaveFile(strFullFileName, FirstSave, 0, FALSE);
		}

		if (SaveCode == SAVEFILE_CANCEL)
			break;

		if (SaveCode == SAVEFILE_SUCCESS) {
			/*
				$ 09.02.2002 VVM
				+ Обновить панели, если писали в текущий каталог
			*/
			if (NeedQuestion) {
				if (apiGetFileAttributes(strFullFileName) != INVALID_FILE_ATTRIBUTES) {
					UpdateFileList();
				}
			}

			FrameManager->DeleteFrame();
			SetExitCode(XC_QUIT);
			break;
		}

		if (!StrCmp(strFileName, Msg::NewFileName)) {
			if (!ProcessKey(KEY_SHIFTF2)) {
				FarChDir(strOldCurDir);
				return FALSE;
			} else
				break;
		}

		WINPORT(SetLastError)(SysErrorCode);

		if (Message(MSG_WARNING | MSG_ERRORTYPE, 2, Msg::EditTitle, Msg::EditCannotSave, strFileName,
					Msg::Retry, Msg::Cancel))
			break;

		FirstSave = 0;
	}

	FarChDir(strOldCurDir);
	return (unsigned int)GetExitCode() == XC_QUIT;
}

// сюды плавно переносить код из Editor::ReadFile()
int FileEditor::LoadFile(const wchar_t *Name, int &UserBreak)
{
	SudoClientRegion sdc_rgn;
	ChangePriority ChPriority(ChangePriority::NORMAL);
	if (Opt.EdOpt.UseEditorConfigOrg) {
		FARString strFullName;
		ConvertNameToFull(Name, strFullName);
		EdCfg.reset(new EditorConfigOrg);
		EdCfg->Populate(strFullName.GetMB().c_str());
		if (EdCfg->CodePageBOM >= 0)
			m_AddSignature = (EdCfg->CodePageBOM == 0) ? FB_NO : FB_YES;
		if (EdCfg->CodePage > 0)
			m_codepage = EdCfg->CodePage;
		if (EdCfg->EndOfLine)
			far_wcsncpy(m_editor->GlobalEOL, EdCfg->EndOfLine, ARRAYSIZE(m_editor->GlobalEOL));
		if (EdCfg->TabSize > 0)
			m_editor->SetTabSize(EdCfg->TabSize);
		if (EdCfg->ExpandTabs >= 0)
			m_editor->SetConvertTabs(EdCfg->ExpandTabs);
	} else {
		EdCfg.reset();
	}

	TPreRedrawFuncGuard preRedrawFuncGuard(Editor::PR_EditorShowMsg);
	wakeful W;
	int LastLineCR = 0;
	EditorCacheParams cp;
	UserBreak = 0;
	File EditFile;
	DWORD FileAttr = apiGetFileAttributes(Name);
	if ((FileAttr != INVALID_FILE_ATTRIBUTES && (FileAttr & FILE_ATTRIBUTE_DEVICE) != 0) ||		// avoid stuck
			!EditFile.Open(Name, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr, OPEN_EXISTING,
					FILE_FLAG_SEQUENTIAL_SCAN)) {
		SysErrorCode = WINPORT(GetLastError)();
		if ((SysErrorCode != ERROR_FILE_NOT_FOUND) && (SysErrorCode != ERROR_PATH_NOT_FOUND)) {
			UserBreak = -1;
			Flags.Set(FFILEEDIT_OPENFAILED);
		} else if (m_codepage != CP_AUTODETECT && Flags.Check(FFILEEDIT_NEW)) {
			Flags.Set(FFILEEDIT_CODEPAGECHANGEDBYUSER);
		}

		return FALSE;
	}

	/*if (GetFileType(hEdit) != FILE_TYPE_DISK)
	{
		fclose(EditFile);
		WINPORT(SetLastError)(ERROR_INVALID_NAME);
		UserBreak=-1;
		Flags.Set(FFILEEDIT_OPENFAILED);
		return FALSE;
	}*/

	if (Opt.EdOpt.FileSizeLimitLo || Opt.EdOpt.FileSizeLimitHi) {
		UINT64 FileSize = 0;
		if (EditFile.GetSize(FileSize)) {
			UINT64 MaxSize = Opt.EdOpt.FileSizeLimitHi * 0x100000000ull + Opt.EdOpt.FileSizeLimitLo;

			if (FileSize > MaxSize) {
				FARString strTempStr1, strTempStr2, strTempStr3, strTempStr4;
				// Ширина = 8 - это будет... в Kb и выше...
				FileSizeToStr(strTempStr1, FileSize, 8);
				FileSizeToStr(strTempStr2, MaxSize, 8);
				strTempStr3.Format(Msg::EditFileLong, RemoveExternalSpaces(strTempStr1).CPtr());
				strTempStr4.Format(Msg::EditFileLong2, RemoveExternalSpaces(strTempStr2).CPtr());

				if (Message(MSG_WARNING, 2, Msg::EditTitle, Name, strTempStr3, strTempStr4, Msg::EditROOpen,
							Msg::Yes, Msg::No)) {
					EditFile.Close();
					UserBreak = 1;
					Flags.Set(FFILEEDIT_OPENFAILED);
					errno = EFBIG;
					return FALSE;
				}
			}
		} else {
			ErrnoSaver ErSr;
			if (Message(MSG_WARNING | MSG_ERRORTYPE, 2, Msg::EditTitle, Name, Msg::EditFileGetSizeError,
						Msg::EditROOpen, Msg::Yes, Msg::No)) {
				EditFile.Close();
				UserBreak = 1;
				Flags.Set(FFILEEDIT_OPENFAILED);
				return FALSE;
			}
		}
	}

	m_editor->FreeAllocatedData(false);
	bool bCached = LoadFromCache(&cp);

	DWORD FileAttributes = apiGetFileAttributes(Name);
	if ((m_editor->EdOpt.ReadOnlyLock & 1) && FileAttributes != INVALID_FILE_ATTRIBUTES
			&& (FileAttributes & (FILE_ATTRIBUTE_READONLY | ((m_editor->EdOpt.ReadOnlyLock & 0x60) >> 4)))) {
		m_editor->Flags.Swap(FEDITOR_LOCKMODE);
	}

	// Проверяем поддерживается или нет загруженная кодовая страница
	if (bCached && cp.CodePage && !IsCodePageSupported(cp.CodePage))
		cp.CodePage = 0;

	GetFileString GetStr(EditFile);
	*m_editor->GlobalEOL = 0;	// BUGBUG???
	wchar_t *Str;
	int StrLength, GetCode;
	UINT dwCP = 0;
	bool Detect = false;
	if (m_codepage == CP_AUTODETECT || IsUnicodeOrUtfCodePage(m_codepage)) {
		bool bSignatureDetected = false;
		Detect = GetFileFormat(EditFile, dwCP, &bSignatureDetected, Opt.EdOpt.AutoDetectCodePage != 0);

		// Проверяем поддерживается или нет задетектировання кодовая страница
		if (Detect) {
			Detect = IsCodePageSupported(dwCP);
			if (Detect) {
				m_AddSignature = (bSignatureDetected ? FB_YES : FB_NO);
			}
		}
	}

	if (m_codepage == CP_AUTODETECT) {
		if (Detect) {
			m_codepage = dwCP;
		}

		if (bCached) {
			if (cp.CodePage) {
				m_codepage = cp.CodePage;
				Flags.Set(FFILEEDIT_CODEPAGECHANGEDBYUSER);
			}
		}

		if (m_codepage == CP_AUTODETECT)
			m_codepage = Opt.EdOpt.DefaultCodePage;
	} else {
		Flags.Set(FFILEEDIT_CODEPAGECHANGEDBYUSER);
	}

	m_editor->SetCodePage(m_codepage);	// BUGBUG

	if (!IsUnicodeOrUtfCodePage(m_codepage)) {
		EditFile.SetPointer(0, nullptr, FILE_BEGIN);
	}

	UINT64 FileSize = 0;
	EditFile.GetSize(FileSize);
	DWORD StartTime = WINPORT(GetTickCount)();

	while ((GetCode = GetStr.GetString(&Str, m_codepage, StrLength))) {
		if (GetCode == -1) {
			EditFile.Close();
			return FALSE;
		}

		LastLineCR = 0;
		DWORD CurTime = WINPORT(GetTickCount)();

		if (CurTime - StartTime > RedrawTimeout) {
			StartTime = CurTime;

			SetCursorType(FALSE, 0);
			INT64 CurPos = 0;
			EditFile.GetPointer(CurPos);
			int Percent = static_cast<int>(CurPos * 100 / FileSize);
			// В случае если во время загрузки файл увеличивается размере, то количество
			// процентов может быть больше 100. Обрабатываем эту ситуацию.
			if (Percent > 100) {
				EditFile.GetSize(FileSize);
				Percent = static_cast<int>(CurPos * 100 / FileSize);
				if (Percent > 100) {
					Percent = 100;
				}
			}
			Editor::EditorShowMsg(Msg::EditTitle, Msg::EditReading, Name, Percent);

			if (CheckForEscSilent()) {
				if (ConfirmAbortOp()) {
					UserBreak = 1;
					EditFile.Close();
					return FALSE;
				}
			}
		}

		const wchar_t *CurEOL;

		int Offset = StrLength > 3 ? StrLength - 3 : 0;

		if (!LastLineCR
				&& ((CurEOL = wmemchr(Str + Offset, L'\r', StrLength - Offset))
						|| (CurEOL = wmemchr(Str + Offset, L'\n', StrLength - Offset)))) {
			far_wcsncpy(m_editor->GlobalEOL, CurEOL, ARRAYSIZE(m_editor->GlobalEOL));
			m_editor->GlobalEOL[ARRAYSIZE(m_editor->GlobalEOL) - 1] = 0;
			LastLineCR = 1;
		}

		if (!m_editor->InsertString(Str, StrLength)) {
			EditFile.Close();
			return FALSE;
		}
	}

	BadConversion = !GetStr.IsConversionValid();
	if (BadConversion) {
		Message(MSG_WARNING, 1, Msg::Warning, Msg::EditorLoadCPWarn1, Msg::EditorLoadCPWarn2,
				Msg::EditorSaveNotRecommended, Msg::Ok);
	}

	if (LastLineCR || !m_editor->NumLastLine)
		m_editor->InsertString(L"", 0);

	EditFile.Close();
	// if ( bCached )
	m_editor->SetCacheParams(&cp);

	SysErrorCode = WINPORT(GetLastError)();
	apiGetFindDataForExactPathName(Name, FileInfo);
	EditorGetFileAttributes(Name);
	strLoadedFileName = Name;
	ChangeEditKeyBar();
	return TRUE;
}

bool FileEditor::ReloadFile(const wchar_t *Name)
{
	int UserBreak = 0;
	m_editor->ProcessKey(KEY_CTRLU);
	if (!LoadFile(Name, UserBreak))
	{
		if (UserBreak != 1) {
			WINPORT(SetLastError)(SysErrorCode);
			Message(MSG_WARNING | MSG_ERRORTYPE, 1, Msg::EditTitle, Msg::EditCannotOpen, Name,
					Msg::Ok);
			ExitCode = XC_OPEN_ERROR;
		} else {
			ExitCode = XC_LOADING_INTERRUPTED;
		}
		FrameManager->DeleteFrame(); // (!) delete Editor if reloading error or user interrupted
		return false;
	}
	m_editor->Flags.Clear(FEDITOR_MODIFIED);
	if (CtrlObject) {
		CtrlObject->Plugins.CurEditor = this;
		if (bEE_READ_Sent) {
			int FEditEditorID = m_editor->EditorID;
			CtrlObject->Plugins.ProcessEditorEvent(EE_CLOSE, &FEditEditorID);
		}
		CtrlObject->Plugins.ProcessEditorEvent(EE_READ, nullptr);
		bEE_READ_Sent = true;
	}
	Show(); // need to force redraw after reload file
	return true;
}

// TextFormat и Codepage используются ТОЛЬКО, если bSaveAs = true!
void FileEditor::SaveContent(const wchar_t *Name, BaseContentWriter *Writer, bool bSaveAs, int TextFormat,
		UINT codepage, bool AddSignature, int Phase)
{
	DWORD dwSignature = 0;
	DWORD SignLength = 0;
	switch (codepage) {
		case CP_UTF32LE:
			dwSignature = SIGN_UTF32LE;
			SignLength = 4;
			if (!bSaveAs)
				AddSignature = (m_AddSignature != FB_NO);
			break;
		case CP_UTF32BE:
			dwSignature = SIGN_UTF32BE;
			SignLength = 4;
			if (!bSaveAs)
				AddSignature = (m_AddSignature != FB_NO);
			break;
		case CP_UTF16LE:
			dwSignature = SIGN_UTF16LE;
			SignLength = 2;
			if (!bSaveAs)
				AddSignature = (m_AddSignature != FB_NO);
			break;
		case CP_UTF16BE:
			dwSignature = SIGN_UTF16BE;
			SignLength = 2;
			if (!bSaveAs)
				AddSignature = (m_AddSignature != FB_NO);
			break;
		case CP_UTF8:
			dwSignature = SIGN_UTF8;
			SignLength = 3;
			if (!bSaveAs)
				AddSignature = (m_AddSignature == FB_YES);
			break;
	}
	if (AddSignature)
		Writer->Write(&dwSignature, SignLength);

	DWORD StartTime = WINPORT(GetTickCount)();
	size_t LineNumber = 0;

	for (Edit *CurPtr = m_editor->TopList; CurPtr; CurPtr = CurPtr->m_next, LineNumber++) {
		DWORD CurTime = WINPORT(GetTickCount)();

		if (CurTime - StartTime > RedrawTimeout) {
			StartTime = CurTime;
			if (Phase == 0)
				Editor::EditorShowMsg(Msg::EditTitle, Msg::EditSaving, Name,
						(int)(LineNumber * 50 / m_editor->NumLastLine));
			else
				Editor::EditorShowMsg(Msg::EditTitle, Msg::EditSaving, Name,
						(int)(50 + (LineNumber * 50 / m_editor->NumLastLine)));
		}

		const wchar_t *SaveStr, *EndSeq;

		int Length;

		CurPtr->GetBinaryString(&SaveStr, &EndSeq, Length);

		if (!*EndSeq && CurPtr->m_next)
			EndSeq = *m_editor->GlobalEOL ? m_editor->GlobalEOL : DOS_EOL_fmt;

		if (TextFormat && *EndSeq) {
			EndSeq = m_editor->GlobalEOL;
			CurPtr->SetEOL(EndSeq);
		}

		Writer->EncodeAndWrite(codepage, SaveStr, Length);
		Writer->EncodeAndWrite(codepage, EndSeq, StrLength(EndSeq));
	}
}

void FileEditor::BaseContentWriter::EncodeAndWrite(UINT codepage, const wchar_t *Str, size_t Length)
{
	if (!Length)
		return;

	if (codepage == CP_WIDE_LE) {
		Write(Str, Length * sizeof(wchar_t));
	} else if (codepage == CP_UTF8) {
		Wide2MB(Str, Length, _tmpstr);
		Write(_tmpstr.data(), _tmpstr.size());
	} else {
		int cnt = WINPORT(WideCharToMultiByte)(codepage, 0, Str, Length, nullptr, 0, nullptr, nullptr);

		if (cnt <= 0)
			return;

		if (_tmpcvec.size() < (size_t)cnt)
			_tmpcvec.resize(cnt + 0x20);

		cnt = WINPORT(WideCharToMultiByte)(codepage, 0, Str, Length, _tmpcvec.data(), _tmpcvec.size(),
				nullptr, nullptr);
		if (cnt > 0)
			Write(_tmpcvec.data(), cnt);
	}
}

struct ContentMeasurer : FileEditor::BaseContentWriter
{
	INT64 MeasuredSize = 0;

	virtual void Write(const void *Data, size_t Length) { MeasuredSize+= Length; }
};

class ContentSaver : public FileEditor::BaseContentWriter
{
	CachedWrite CW;

public:
	ContentSaver(File &EditFile)
		:
		CW(EditFile)
	{}

	virtual void Write(const void *Data, size_t Length)
	{
		if (!CW.Write(Data, Length))
			throw WINPORT(GetLastError)();
	}

	void Flush()
	{
		if (!CW.Flush())
			throw WINPORT(GetLastError)();
	}
};

int FileEditor::SaveFile(const wchar_t *Name, int Ask, bool bSaveAs, int TextFormat, UINT codepage,
		bool AddSignature)
{
	SudoClientRegion sdc_rgn;
	if (!bSaveAs) {
		TextFormat = 0;
		codepage = m_editor->GetCodePage();
	}

	wakeful W;

	if (m_editor->Flags.Check(FEDITOR_LOCKMODE) && !m_editor->Flags.Check(FEDITOR_MODIFIED) && !bSaveAs)
		return SAVEFILE_SUCCESS;

	if (Ask) {
		if (!m_editor->Flags.Check(FEDITOR_MODIFIED))
			return SAVEFILE_SUCCESS;

		if (Ask) {
			switch (Message(MSG_WARNING, 3, Msg::EditTitle, Msg::EditAskSave, Msg::HYes, Msg::HNo,
					Msg::HCancel)) {
				case -1:
				case -2:
				case 2:							// Continue Edit
					return SAVEFILE_CANCEL;
				case 0:							// Save
					break;
				case 1:							// Not Save
					m_editor->TextChanged(0);	// 10.08.2000 skv: TextChanged() support;
					return SAVEFILE_SUCCESS;
			}
		}
	}

	int NewFile = TRUE;

	FileUnmakeWritable = apiMakeWritable(Name);
	if (FileUnmakeWritable.get()) {
		// BUGBUG
		int AskOverwrite =
				Message(MSG_WARNING, 2, Msg::EditTitle, Name, Msg::EditRO, Msg::EditOvr, Msg::Yes, Msg::No);

		if (AskOverwrite) {
			FileUnmakeWritable->Unmake();
			FileUnmakeWritable.reset();
			return SAVEFILE_CANCEL;
		}
	}

	DWORD FileAttributes = EditorGetFileAttributes(Name);
	if (FileAttributes != INVALID_FILE_ATTRIBUTES) {
		// Проверка времени модификации...
		if (!Flags.Check(FFILEEDIT_SAVEWQUESTIONS)) {
			FAR_FIND_DATA_EX FInfo;

			if (apiGetFindDataForExactPathName(Name, FInfo) && !FileInfo.strFileName.IsEmpty()) {
				int64_t RetCompare = FileTimeDifference(&FileInfo.ftLastWriteTime, &FInfo.ftLastWriteTime);

				if (RetCompare || !(FInfo.nFileSize == FileInfo.nFileSize)) {
					SetMessageHelp(L"WarnEditorSavedEx");

					switch (Message(MSG_WARNING, 3, Msg::EditTitle, Msg::EditAskSaveExt, Msg::HYes,
							Msg::EditBtnSaveAs, Msg::HCancel)) {
						case -1:
						case -2:
						case 2:		// Continue Edit
							return SAVEFILE_CANCEL;
						case 1:		// Save as

							if (ProcessKey(KEY_SHIFTF2))
								return SAVEFILE_SUCCESS;
							else
								return SAVEFILE_CANCEL;

						case 0:		// Save
							break;
					}
				}
			}
		}

		Flags.Clear(FFILEEDIT_SAVEWQUESTIONS);
		NewFile = FALSE;
	} else {
		// проверим путь к файлу, может его уже снесли...
		FARString strCreatedPath = Name;
		const wchar_t *Ptr = LastSlash(strCreatedPath);

		if (Ptr) {
			CutToSlash(strCreatedPath);
			DWORD FAttr = 0;

			if (apiGetFileAttributes(strCreatedPath) == INVALID_FILE_ATTRIBUTES) {
				// и попробуем создать.
				// Раз уж
				CreatePath(strCreatedPath);
				FAttr = apiGetFileAttributes(strCreatedPath);
			}

			if (FAttr == INVALID_FILE_ATTRIBUTES)
				return SAVEFILE_ERROR;
		}
	}

	if (BadConversion) {
		if (Message(MSG_WARNING, 2, Msg::Warning, Msg::EditDataLostWarn, Msg::EditorSaveNotRecommended,
					Msg::Ok, Msg::Cancel)) {
			return SAVEFILE_CANCEL;
		} else {
			BadConversion = false;
		}
	}

	int RetCode = SAVEFILE_SUCCESS;

	if (TextFormat)
		m_editor->Flags.Set(FEDITOR_WASCHANGED);

	switch (TextFormat) {
		case 1:
			wcscpy(m_editor->GlobalEOL, DOS_EOL_fmt);
			break;
		case 2:
			wcscpy(m_editor->GlobalEOL, UNIX_EOL_fmt);
			break;
		case 3:
			wcscpy(m_editor->GlobalEOL, MAC_EOL_fmt);
			break;
		case 4:
			wcscpy(m_editor->GlobalEOL, WIN_EOL_fmt);
			break;
	}

	if (apiGetFileAttributes(Name) == INVALID_FILE_ATTRIBUTES)
		Flags.Set(FFILEEDIT_NEW);

	{
		// SaveScreen SaveScr;
		CtrlObject->Plugins.CurEditor = this;
		//_D(SysLog(L"%08d EE_SAVE",__LINE__));

		if (!IsUnicodeOrUtfCodePage(codepage)) {
			int LineNumber = 0;
			bool BadSaveConfirmed = false;
			for (Edit *CurPtr = m_editor->TopList; CurPtr; CurPtr = CurPtr->m_next, LineNumber++) {
				const wchar_t *SaveStr, *EndSeq;
				int Length;
				CurPtr->GetBinaryString(&SaveStr, &EndSeq, Length);
				BOOL UsedDefaultCharStr = FALSE, UsedDefaultCharEOL = FALSE;
				if (Length
						&& !WINPORT(WideCharToMultiByte)(codepage, WC_NO_BEST_FIT_CHARS, SaveStr, Length,
								nullptr, 0, nullptr, &UsedDefaultCharStr))
					return SAVEFILE_ERROR;

				if (!*EndSeq && CurPtr->m_next)
					EndSeq = *m_editor->GlobalEOL ? m_editor->GlobalEOL : DOS_EOL_fmt;

				if (TextFormat && *EndSeq)
					EndSeq = m_editor->GlobalEOL;

				int EndSeqLen = StrLength(EndSeq);
				if (EndSeqLen
						&& !WINPORT(WideCharToMultiByte)(codepage, WC_NO_BEST_FIT_CHARS, EndSeq, EndSeqLen,
								nullptr, 0, nullptr, &UsedDefaultCharEOL))
					return SAVEFILE_ERROR;

				if (!BadSaveConfirmed && (UsedDefaultCharStr || UsedDefaultCharEOL)) {
					// SetMessageHelp(L"EditorDataLostWarning")
					int Result = Message(MSG_WARNING, 3, Msg::Warning, Msg::EditorSaveCPWarn1,
							Msg::EditorSaveCPWarn2, Msg::EditorSaveNotRecommended, Msg::Ok,
							Msg::EditorSaveCPWarnShow, Msg::Cancel);
					if (!Result) {
						BadSaveConfirmed = true;
						break;
					} else {
						if (Result == 1) {
							m_editor->GoToLine(LineNumber);
							if (UsedDefaultCharStr) {
								for (int Pos = 0; Pos < Length; Pos++) {
									BOOL UseDefChar = 0;
									WINPORT(WideCharToMultiByte)
									(codepage, WC_NO_BEST_FIT_CHARS, SaveStr + Pos, 1, nullptr, 0, nullptr,
											&UseDefChar);
									if (UseDefChar) {
										CurPtr->SetCurPos(Pos);
										break;
									}
								}
							} else {
								CurPtr->SetCurPos(CurPtr->GetLength());
							}
							Show();
						}
						return SAVEFILE_CANCEL;
					}
				}
			}
		}

		CtrlObject->Plugins.ProcessEditorEvent(EE_SAVE, nullptr);

		m_editor->UndoSavePos = m_editor->UndoPos;
		m_editor->Flags.Clear(FEDITOR_UNDOSAVEPOSLOST);
		// ConvertNameToFull(Name,FileName, sizeof(FileName));
		/*
			if (ConvertNameToFull(Name,m_editor->FileName, sizeof(m_editor->FileName)) >= sizeof(m_editor->FileName))
			{
				m_editor->Flags.Set(FEDITOR_OPENFAILED);
				RetCode=SAVEFILE_ERROR;
				goto end;
			}
		*/
		SetCursorType(FALSE, 0);
		TPreRedrawFuncGuard preRedrawFuncGuard(Editor::PR_EditorShowMsg);

		try {
			ContentMeasurer cm;
			SaveContent(Name, &cm, bSaveAs, TextFormat, codepage, AddSignature, 0);

			try {
				File EditFile;
				bool EditFileOpened = EditFile.Open(Name, GENERIC_WRITE, FILE_SHARE_READ, nullptr,
						OPEN_ALWAYS, FILE_ATTRIBUTE_ARCHIVE | FILE_FLAG_SEQUENTIAL_SCAN);
				if (!EditFileOpened
						&& (WINPORT(GetLastError)() == ERROR_NOT_SUPPORTED
								|| WINPORT(GetLastError)() == ERROR_CALL_NOT_IMPLEMENTED)) {
					EditFileOpened = EditFile.Open(Name, GENERIC_WRITE, FILE_SHARE_READ, nullptr,
							CREATE_ALWAYS, FILE_ATTRIBUTE_ARCHIVE | FILE_FLAG_SEQUENTIAL_SCAN);
					if (EditFileOpened) {
						fprintf(stderr, "FileEditor::SaveFile: CREATE_ALWAYS for '%ls'\n", Name);
					}
				}
				if (!EditFileOpened) {
					throw WINPORT(GetLastError)();
				}

				if (!Flags.Check(FFILEEDIT_NEW)) {
					if (!EditFile.AllocationRequire(cm.MeasuredSize))
						throw WINPORT(GetLastError)();
				}

				ContentSaver cs(EditFile);
				SaveContent(Name, &cs, bSaveAs, TextFormat, codepage, AddSignature, 1);
				cs.Flush();

				EditFile.SetEnd();

			} catch (...) {
				if (Flags.Check(FFILEEDIT_NEW))
					apiDeleteFile(Name);

				throw;
			}
		} catch (DWORD ErrorCode) {
			SysErrorCode = ErrorCode;
			RetCode = SAVEFILE_ERROR;
		} catch (std::exception &e) {
			(void)e;
			SysErrorCode = ENOMEM;
			RetCode = SAVEFILE_ERROR;
		}
	}

	if (FHP && RetCode != SAVEFILE_ERROR)
		FHP->OnFileEdited(Name);

	if (FileUnmakeWritable) {
		FileUnmakeWritable->Unmake();
		FileUnmakeWritable.reset();
		//		apiSetFileAttributes(Name,FileAttributes|FILE_ATTRIBUTE_ARCHIVE);
	}

	apiGetFindDataForExactPathName(Name, FileInfo);
	EditorGetFileAttributes(Name);

	if (m_editor->Flags.Check(FEDITOR_MODIFIED) || NewFile)
		m_editor->Flags.Set(FEDITOR_WASCHANGED);

	// Этот кусок раскомметировать в том случае, если народ решит, что
	// для если файл был залочен и мы его переписали под други именем...
	// ...то "лочка" должна быть снята.

	//	if(SaveAs)
	//		Flags.Clear(FEDITOR_LOCKMODE);
	// 28.12.2001 VVM
	// ! Проверить на успешную запись
	if (RetCode == SAVEFILE_SUCCESS) {
		m_editor->TextChanged(0);
		//		m_editor->EnableSaveTabSettings();
	}

	if (GetDynamicallyBorn())	// принудительно сбросим Title // Flags.Check(FFILEEDIT_SAVETOSAVEAS) ????????
		strTitle.Clear();

	Show();
	// ************************************
	Flags.Clear(FFILEEDIT_NEW);
	return RetCode;
}

int FileEditor::ProcessMouse(MOUSE_EVENT_RECORD *MouseEvent)
{
	F4KeyOnly = false;
	if (!EditKeyBar.ProcessMouse(MouseEvent))
		if (!ProcessEditorInput(FrameManager->GetLastInputRecord()))
			if (!m_editor->ProcessMouse(MouseEvent))
				return FALSE;

	return TRUE;
}

int FileEditor::GetTypeAndName(FARString &strType, FARString &strName)
{
	strType = Msg::ScreensEdit;
	strName = strFullFileName;
	return (MODALTYPE_EDITOR);
}

void FileEditor::ShowConsoleTitle()
{
	FARString strTitle;
	strTitle.Format(Msg::InEditor, PointToName(strFileName));
	ConsoleTitle::SetFarTitle(strTitle);
	Flags.Clear(FFILEEDIT_REDRAWTITLE);
}

void FileEditor::SetScreenPosition()
{
	if (Flags.Check(FFILEEDIT_FULLSCREEN)) {
		SetPosition(0, 0, ScrX, ScrY);
	}
}

/*
	$ 10.05.2001 DJ
	добавление в view/edit history
*/

void FileEditor::OnDestroy()
{
	_OT(SysLog(L"[%p] FileEditor::OnDestroy()", this));

	if (!Flags.Check(FFILEEDIT_DISABLEHISTORY) && StrCmpI(strFileName, Msg::NewFileName))
		CtrlObject->ViewHistory->AddToHistory(strFullFileName,
				(m_editor->Flags.Check(FEDITOR_LOCKMODE) ? 4 : 1));

	if (CtrlObject->Plugins.CurEditor == this)		//&this->FEdit)
	{
		CtrlObject->Plugins.CurEditor = nullptr;
	}
}

int FileEditor::GetCanLoseFocus(int DynamicMode)
{
	if (DynamicMode) {
		if (m_editor->IsFileModified()) {
			return FALSE;
		}
	} else {
		return CanLoseFocus;
	}

	return TRUE;
}

void FileEditor::SetLockEditor(BOOL LockMode)
{
	if (LockMode)
		m_editor->Flags.Set(FEDITOR_LOCKMODE);
	else
		m_editor->Flags.Clear(FEDITOR_LOCKMODE);
}

int FileEditor::FastHide()
{
	return Opt.AllCtrlAltShiftRule & CASR_EDITOR;
}

BOOL FileEditor::isTemporary()
{
	return (!GetDynamicallyBorn());
}

void FileEditor::ResizeConsole()
{
	m_editor->PrepareResizedConsole();
}

int FileEditor::ProcessEditorInput(INPUT_RECORD *Rec)
{
	int RetCode;
	CtrlObject->Plugins.CurEditor = this;
	RetCode = CtrlObject->Plugins.ProcessEditorInput(Rec);
	return RetCode;
}

void FileEditor::SetPluginTitle(const wchar_t *PluginTitle)
{
	if (!PluginTitle)
		strPluginTitle.Clear();
	else
		strPluginTitle = PluginTitle;
}

BOOL FileEditor::SetFileName(const wchar_t *NewFileName)
{
	strFileName = NewFileName;

	if (StrCmp(strFileName, Msg::NewFileName)) {
		ConvertNameToFull(strFileName, strFullFileName);
		FARString strFilePath = strFullFileName;

		if (CutToSlash(strFilePath, 1)) {
			FARString strCurPath;

			if (apiGetCurrentDirectory(strCurPath)) {
				DeleteEndSlash(strCurPath);

				if (!StrCmpI(strFilePath, strCurPath))
					strFileName = PointToName(strFullFileName);
			}
		}

	} else {
		strFullFileName = strStartDir;
		AddEndSlash(strFullFileName);
		strFullFileName+= strFileName;
	}

	return TRUE;
}

void FileEditor::SetTitle(const wchar_t *Title)
{
	strTitle = Title;
}

void FileEditor::SetEditKeyBarStatefulLabels()
{
	if (m_codepage == CP_UTF8)
		EditKeyBar.Change(KBL_MAIN, (Opt.OnlyEditorViewerUsed ? Msg::SingleEditF8 : Msg::EditF8), 7);
	else if (m_codepage == WINPORT(GetACP)())
		EditKeyBar.Change(KBL_MAIN, (Opt.OnlyEditorViewerUsed ? Msg::SingleEditF8DOS : Msg::EditF8DOS), 7);
	else
		EditKeyBar.Change(KBL_MAIN, (Opt.OnlyEditorViewerUsed ? Msg::SingleEditF8UTF8 : Msg::EditF8UTF8), 7);

	EditKeyBar.Change(KBL_MAIN, m_editor->GetShowWhiteSpace() ? Msg::EditF5Hide : Msg::EditF5, 4);

	EditKeyBar.Change(KBL_CTRL, m_editor->GetConvertTabs() ? Msg::EditCtrlF5 : Msg::EditCtrlF5Spaces, 4);
}

void FileEditor::ChangeEditKeyBar()
{
	SetEditKeyBarStatefulLabels();
	EditKeyBar.Redraw();
}

void FileEditor::ChooseTabSizeMenu()
{
	std::vector<std::wstring> items;
	StrExplode(items, std::wstring(Msg::EditTabWidthItems.CPtr()), L";");
	VMenu menu(Msg::EditTabWidthTitle, nullptr, 0, ScrY - 4);
	menu.SetFlags(VMENU_WRAPMODE | VMENU_AUTOHIGHLIGHT);
	menu.SetHelp(L"TabSizeMenu");
	menu.SetPosition(-1, -1, 0, 0);

	for (const auto &item : items) {
		MenuItemEx mi;
		mi.SetSelect((&item - &items[0]) == (m_editor->GetTabSize() - 1));
		mi.strName = item;
		menu.AddItem(&mi);
	}
	menu.Show();
	while (!menu.Done()) {
		menu.ReadInput();
		menu.ProcessInput();
	}

	int r = menu.GetExitCode();
	if (r >= 0) {
		m_editor->SetTabSize(r + 1);
		m_editor->EnableSaveTabSettings();
		m_editor->Show();
	}
}

FARString &FileEditor::GetTitle(FARString &strLocalTitle, int SubLen, int TruncSize)
{
	if (!strPluginTitle.IsEmpty())
		strLocalTitle = strPluginTitle;
	else {
		if (!strTitle.IsEmpty())
			strLocalTitle = strTitle;
		else
			strLocalTitle = strFullFileName;
	}

	return strLocalTitle;
}

static const struct CharCodeFmtInfo
{
	const wchar_t *wide_fmt;
	size_t wide_width;
	const wchar_t *mb_fmt;
	size_t mb_width;
} s_CCFI[] = {
	{L"%05o", 8, L" (%o)", 6},
	{L"%5d", 7, L" (%d)", 6},
	{ L"%04Xh", 6, L" (%X)", 5}
};

void FileEditor::ShowStatus()
{
	if (m_editor->Locked() || !TitleBarVisible)
		return;

	SetFarColor(COL_EDITORSTATUS);
	GotoXY(X1, Y1);		//??
	FARString strLineStr;
	FARString strLocalTitle;
	GetTitle(strLocalTitle);

	strLineStr.Format(L"%d/%d", m_editor->NumLine + 1, m_editor->NumLastLine);

	FARString strCharCode;
	size_t CharCodeWidth = 5;
	{
		const bool UCP = IsUnicodeOrUtfCodePage(m_codepage);
		const wchar_t *Str = nullptr;
		int Length = 0;
		m_editor->CurLine->GetBinaryString(&Str, nullptr, Length);
		int CurPos = m_editor->CurLine->GetCurPos();
		const size_t CharCodeInfoIdx = (m_editor->EdOpt.CharCodeBase % ARRAYSIZE(s_CCFI));
		CharCodeWidth = s_CCFI[CharCodeInfoIdx].wide_width;
		if (!UCP) {
			CharCodeWidth+= s_CCFI[CharCodeInfoIdx].mb_width;
		}
		if (CurPos < Length) {
			/*
				$ 27.02.2001 SVS
				Показываем в зависимости от базы
			*/
			strCharCode.AppendFormat(s_CCFI[CharCodeInfoIdx].wide_fmt, (unsigned int)Str[CurPos]);
			if (!UCP) {
				char C = 0;
				BOOL UsedDefaultChar = FALSE;
				WINPORT(WideCharToMultiByte)
					(m_codepage, WC_NO_BEST_FIT_CHARS, &Str[CurPos], 1, &C, 1, 0, &UsedDefaultChar);

				if (C && !UsedDefaultChar && static_cast<wchar_t>(C) != Str[CurPos]) {
					strCharCode.AppendFormat(s_CCFI[CharCodeInfoIdx].mb_fmt, (unsigned int)(unsigned char)C);
				}
			}
		}
	}

	FARString strTabMode;
	strTabMode.Format(L"%c%d", m_editor->GetConvertTabs() ? 'S' : 'T', m_editor->GetTabSize());
	FARString str_codepage;
	ShortReadableCodepageName(m_codepage,str_codepage);
	FormatString FString;
	FString << fmt::Cells() << fmt::LeftAlign()
			<< (m_editor->Flags.Check(FEDITOR_MODIFIED) ? L'*' : L' ')
			<< (m_editor->Flags.Check(FEDITOR_LOCKMODE) ? L'-' : L' ')
			<< (m_editor->Flags.Check(FEDITOR_PROCESSCTRLQ) ? L'"' : L' ') << strTabMode << L' '
			<< fmt::Expand(5) << EOLName(m_editor->GlobalEOL) << L' ' << fmt::Expand(5) << str_codepage << L' '
			<< fmt::Expand(7) << Msg::EditStatusLine << L' '
			<< fmt::Expand(12) << strLineStr << L' ' //SizeLineStr
			<< fmt::Expand(5) << Msg::EditStatusCol << L' '
			<< fmt::LeftAlign() << fmt::Expand(4) << m_editor->CurLine->GetCellCurPos() + 1 << L' '
			<< AttrStr << (AttrStr.IsEmpty() ? L"" : L" ")
			<< fmt::Expand(CharCodeWidth) << strCharCode;
	int StatusWidth = ObjWidth - (Opt.ViewerEditorClock && Flags.Check(FFILEEDIT_FULLSCREEN) ? 6 : 0);

	if (StatusWidth < 0)
		StatusWidth = 0;

	FARString StrStatus = FString.strValue();
	int TitleCells = StatusWidth - StrStatus.CellsCount();
	if (TitleCells <= 5) {
		TitleCells = StatusWidth;
		StrStatus.Clear();
	}
	if (TitleCells > 0) {
		if (strLocalTitle.CellsCount() > size_t(TitleCells)) {
			TruncStr(strLocalTitle, TitleCells);
		}
		FS << fmt::LeftAlign() << fmt::Cells() << fmt::Expand(TitleCells) << strLocalTitle << StrStatus;
	}

	if (Opt.ViewerEditorClock && Flags.Check(FFILEEDIT_FULLSCREEN)) {
		if (X2 > 5) {
			Text(X2 - 5, Y1, FarColorToReal(COL_EDITORTEXT), L" ");
		}
		ShowTime(FALSE);
	}
}

/*
	$ 13.02.2001
	Узнаем атрибуты файла и заодно сформируем готовую строку атрибутов для
	статуса.
*/
DWORD FileEditor::EditorGetFileAttributes(const wchar_t *Name)
{
	SudoClientRegion sdc_rgn;
	DWORD FileAttributes = apiGetFileAttributes(Name);
	AttrStr.Clear();
	if (FileAttributes != INVALID_FILE_ATTRIBUTES) {
		if (FileAttributes & FILE_ATTRIBUTE_READONLY)
			AttrStr+= L'R';

		if (FileAttributes & FILE_ATTRIBUTE_SYSTEM)
			AttrStr+= L'S';

		if (FileAttributes & FILE_ATTRIBUTE_HIDDEN)
			AttrStr+= L'H';
	}
	return FileAttributes;
}

/*
	Return TRUE - панель обовили
*/
BOOL FileEditor::UpdateFileList()
{
	Panel *ActivePanel = CtrlObject->Cp()->ActivePanel;
	const wchar_t *FileName = PointToName(strFullFileName);
	FARString strFilePath, strPanelPath;
	strFilePath = strFullFileName;
	strFilePath.Truncate(FileName - strFullFileName.CPtr());
	ActivePanel->GetCurDir(strPanelPath);
	AddEndSlash(strPanelPath);
	AddEndSlash(strFilePath);

	if (!StrCmp(strPanelPath, strFilePath)) {
		ActivePanel->Update(UPDATE_KEEP_SELECTION | UPDATE_DRAW_MESSAGE);
		return TRUE;
	}

	return FALSE;
}

void FileEditor::GetEditorOptions(EditorOptions &EdOpt)
{
	EdOpt = m_editor->EdOpt;
	EdOpt.ShowTitleBar = TitleBarVisible;
	EdOpt.ShowKeyBar = KeyBarVisible;
}

void FileEditor::SetEditorOptions(EditorOptions &EdOpt)
{
	m_editor->SetTabSize(EdOpt.TabSize);
	m_editor->SetConvertTabs(EdOpt.ExpandTabs);
	m_editor->SetPersistentBlocks(EdOpt.PersistentBlocks);
	m_editor->SetDelRemovesBlocks(EdOpt.DelRemovesBlocks);
	m_editor->SetAutoIndent(EdOpt.AutoIndent);
	m_editor->SetAutoDetectCodePage(EdOpt.AutoDetectCodePage);
	m_editor->SetCursorBeyondEOL(EdOpt.CursorBeyondEOL);
	m_editor->SetCharCodeBase(EdOpt.CharCodeBase);
	m_editor->SetSavePosMode(EdOpt.SavePos, EdOpt.SaveShortPos);
	m_editor->SetReadOnlyLock(EdOpt.ReadOnlyLock);
	m_editor->SetShowScrollBar(EdOpt.ShowScrollBar);
	m_editor->SetShowWhiteSpace(EdOpt.ShowWhiteSpace);
	m_editor->SetSearchPickUpWord(EdOpt.SearchPickUpWord);
	TitleBarVisible = EdOpt.ShowTitleBar;
	KeyBarVisible = EdOpt.ShowKeyBar;
	// m_editor->SetBSLikeDel(EdOpt.BSLikeDel);
}

void FileEditor::OnChangeFocus(int focus)
{
	Frame::OnChangeFocus(focus);
	CtrlObject->Plugins.CurEditor = this;
	int FEditEditorID = m_editor->EditorID;
	CtrlObject->Plugins.ProcessEditorEvent(focus ? EE_GOTFOCUS : EE_KILLFOCUS, &FEditEditorID);
}

int FileEditor::EditorControl(int Command, void *Param)
{
#if defined(SYSLOG_KEYMACRO)
	_KEYMACRO(CleverSysLog SL(L"FileEditor::EditorControl()"));

	if (Command == ECTL_READINPUT || Command == ECTL_PROCESSINPUT) {
		_KEYMACRO(SysLog(L"(Command=%ls, Param=[%d/0x%08X]) Macro.IsExecuting()=%d", _ECTL_ToName(Command),
				(int)((DWORD_PTR)Param), (int)((DWORD_PTR)Param), CtrlObject->Macro.IsExecuting()));
	}

#else
	_ECTLLOG(CleverSysLog SL(L"FileEditor::EditorControl()"));
	_ECTLLOG(SysLog(L"(Command=%ls, Param=[%d/0x%08X])", _ECTL_ToName(Command), (int)Param, Param));
#endif

	if (m_bClosing && (Command != ECTL_GETINFO) && (Command != ECTL_GETBOOKMARKS)
			&& (Command != ECTL_GETFILENAME))
		return FALSE;

	switch (Command) {
		case ECTL_GETFILENAME: {
			if (Param) {
				wcscpy(reinterpret_cast<LPWSTR>(Param), strFullFileName);
			}

			return static_cast<int>(strFullFileName.GetLength() + 1);
		}
		case ECTL_GETBOOKMARKS: {
			if (!Flags.Check(FFILEEDIT_OPENFAILED) && Param) {
				EditorBookMarks *ebm = reinterpret_cast<EditorBookMarks *>(Param);
				for (size_t i = 0; i < POSCACHE_BOOKMARK_COUNT; i++) {
					if (ebm->Line) {
						ebm->Line[i] = static_cast<long>(m_editor->SavePos.Line[i]);
					}
					if (ebm->Cursor) {
						ebm->Cursor[i] = static_cast<long>(m_editor->SavePos.Cursor[i]);
					}
					if (ebm->ScreenLine) {
						ebm->ScreenLine[i] = static_cast<long>(m_editor->SavePos.ScreenLine[i]);
					}
					if (ebm->LeftPos) {
						ebm->LeftPos[i] = static_cast<long>(m_editor->SavePos.LeftPos[i]);
					}
				}
				return TRUE;
			}

			return FALSE;
		}
		case ECTL_ADDSTACKBOOKMARK: {
			return m_editor->AddStackBookmark();
		}
		case ECTL_PREVSTACKBOOKMARK: {
			return m_editor->PrevStackBookmark();
		}
		case ECTL_NEXTSTACKBOOKMARK: {
			return m_editor->NextStackBookmark();
		}
		case ECTL_CLEARSTACKBOOKMARKS: {
			return m_editor->ClearStackBookmarks();
		}
		case ECTL_DELETESTACKBOOKMARK: {
			return m_editor->DeleteStackBookmark(m_editor->PointerToStackBookmark((int)(INT_PTR)Param));
		}
		case ECTL_GETSTACKBOOKMARKS: {
			return m_editor->GetStackBookmarks((EditorBookMarks *)Param);
		}
		case ECTL_SETTITLE: {
			strPluginTitle = (const wchar_t *)Param;
			ShowStatus();
			ScrBuf.Flush();		//???
			return TRUE;
		}
		case ECTL_REDRAW: {
			FileEditor::DisplayObject();
			ScrBuf.Flush();
			return TRUE;
		}
		/*
			Функция установки Keybar Labels
			Param = nullptr - восстановить, пред. значение
			Param = -1 - обновить полосу (перерисовать)
			Param = KeyBarTitles
		*/
		case ECTL_SETKEYBAR: {
			KeyBarTitles *Kbt = (KeyBarTitles *)Param;

			if (!Kbt)	// восстановить изначальное
				InitKeyBar();
			else {
				if ((LONG_PTR)Param != (LONG_PTR)-1)	// не только перерисовать?
				{
					for (int I = 0; I < 12; ++I) {
						if (Kbt->Titles[I])
							EditKeyBar.Change(KBL_MAIN, Kbt->Titles[I], I);

						if (Kbt->CtrlTitles[I])
							EditKeyBar.Change(KBL_CTRL, Kbt->CtrlTitles[I], I);

						if (Kbt->AltTitles[I])
							EditKeyBar.Change(KBL_ALT, Kbt->AltTitles[I], I);

						if (Kbt->ShiftTitles[I])
							EditKeyBar.Change(KBL_SHIFT, Kbt->ShiftTitles[I], I);

						if (Kbt->CtrlShiftTitles[I])
							EditKeyBar.Change(KBL_CTRLSHIFT, Kbt->CtrlShiftTitles[I], I);

						if (Kbt->AltShiftTitles[I])
							EditKeyBar.Change(KBL_ALTSHIFT, Kbt->AltShiftTitles[I], I);

						if (Kbt->CtrlAltTitles[I])
							EditKeyBar.Change(KBL_CTRLALT, Kbt->CtrlAltTitles[I], I);
					}
				}

				EditKeyBar.Refresh(KeyBarVisible);
			}

			return TRUE;
		}
		case ECTL_SAVEFILE: {
			FARString strName = strFullFileName;
			int EOL = 0;
			UINT codepage = m_codepage;

			if (Param) {
				EditorSaveFile *esf = (EditorSaveFile *)Param;

				if (esf->FileName && *esf->FileName)
					strName = esf->FileName;

				if (esf->FileEOL) {
					if (!StrCmp(esf->FileEOL, DOS_EOL_fmt))
						EOL = 1;
					else if (!StrCmp(esf->FileEOL, UNIX_EOL_fmt))
						EOL = 2;
					else if (!StrCmp(esf->FileEOL, MAC_EOL_fmt))
						EOL = 3;
					else if (!StrCmp(esf->FileEOL, WIN_EOL_fmt))
						EOL = 4;
				}

				codepage = esf->CodePage;
			}

			{
				FARString strOldFullFileName = strFullFileName;

				if (SetFileName(strName)) {
					if (StrCmpI(strFullFileName, strOldFullFileName)) {
						if (!AskOverwrite(strName)) {
							SetFileName(strOldFullFileName);
							return FALSE;
						}
					}

					Flags.Set(FFILEEDIT_SAVEWQUESTIONS);
					// всегда записываем в режиме save as - иначе не сменить кодировку и концы линий.
					return SaveFile(strName, FALSE, true, EOL, codepage, DecideAboutSignature());
				}
			}

			return FALSE;
		}
		case ECTL_QUIT: {
			FrameManager->DeleteFrame(this);
			SetExitCode(SAVEFILE_ERROR);	// что-то меня терзают смутные сомнения ...???
			return TRUE;
		}
		case ECTL_READINPUT: {
			if (CtrlObject->Macro.IsRecording() == MACROMODE_RECORDING
					|| CtrlObject->Macro.IsExecuting() == MACROMODE_EXECUTING) {
				//				return FALSE;
			}

			if (Param) {
				INPUT_RECORD *rec = (INPUT_RECORD *)Param;
				DWORD Key;

				for (;;) {
					Key = GetInputRecord(rec);

					if ((!rec->EventType || rec->EventType == KEY_EVENT || rec->EventType == FARMACRO_KEY_EVENT)
							&& ((Key >= KEY_MACRO_BASE && Key <= KEY_MACRO_ENDBASE)
									|| (Key >= KEY_OP_BASE && Key <= KEY_OP_ENDBASE)))		// исключаем MACRO
						ReProcessKey(Key);
					else
						break;
				}

				// if(Key==KEY_CONSOLE_BUFFER_RESIZE)   //????
				// Show();                          //????
#if defined(SYSLOG_KEYMACRO)

				if (rec->EventType == KEY_EVENT) {
					SysLog(L"ECTL_READINPUT={%ls,{%d,%d,Vk=0x%04X,0x%08X}}",
							(rec->EventType == FARMACRO_KEY_EVENT ? "FARMACRO_KEY_EVENT" : "KEY_EVENT"),
							rec->Event.KeyEvent.bKeyDown, rec->Event.KeyEvent.wRepeatCount,
							rec->Event.KeyEvent.wVirtualKeyCode, rec->Event.KeyEvent.dwControlKeyState);
				}

#endif
				return TRUE;
			}

			return FALSE;
		}
		case ECTL_PROCESSINPUT: {
			if (Param) {
				INPUT_RECORD *rec = (INPUT_RECORD *)Param;

				if (ProcessEditorInput(rec))
					return TRUE;

				if (rec->EventType == MOUSE_EVENT)
					ProcessMouse(&rec->Event.MouseEvent);
				else {
#if defined(SYSLOG_KEYMACRO)

					if (!rec->EventType || rec->EventType == KEY_EVENT
							|| rec->EventType == FARMACRO_KEY_EVENT) {
						SysLog(L"ECTL_PROCESSINPUT={%ls,{%d,%d,Vk=0x%04X,0x%08X}}",
								(rec->EventType == FARMACRO_KEY_EVENT ? "FARMACRO_KEY_EVENT" : "KEY_EVENT"),
								rec->Event.KeyEvent.bKeyDown, rec->Event.KeyEvent.wRepeatCount,
								rec->Event.KeyEvent.wVirtualKeyCode, rec->Event.KeyEvent.dwControlKeyState);
					}

#endif
					FarKey Key = CalcKeyCode(rec, FALSE);
					ReProcessKey(Key);
				}

				return TRUE;
			}

			return FALSE;
		}
		case ECTL_PROCESSKEY: {
			ReProcessKey((int)(INT_PTR)Param);
			return TRUE;
		}
		case ECTL_SETPARAM: {
			if (Param) {
				EditorSetParameter *espar = (EditorSetParameter *)Param;
				if (ESPT_SETBOM == espar->Type) {
					if (IsUnicodeOrUtfCodePage(m_codepage)) {
						m_AddSignature = espar->Param.iParam ? FB_YES : FB_NO;
						return TRUE;
					}
					return FALSE;
				}
			}
			break;
		}
	}

	int result = m_editor->EditorControl(Command, Param);
	if (result && Param && ECTL_GETINFO == Command) {
		EditorInfo *Info = (EditorInfo *)Param;
		if (DecideAboutSignature())
			Info->Options|= EOPT_BOM;
	}
	return result;
}

bool FileEditor::DecideAboutSignature()
{
	return (m_AddSignature == FB_YES
			|| (m_AddSignature == FB_MAYBE && IsUnicodeOrUtfCodePage(m_codepage) && m_codepage != CP_UTF8));
}

bool FileEditor::LoadFromCache(EditorCacheParams *pp)
{
	memset(pp, 0, sizeof(EditorCacheParams));
	memset(&pp->SavePos, 0xff, sizeof(InternalEditorBookMark));

	FARString strCacheName;

	if (*GetPluginData()) {
		strCacheName = GetPluginData();
		strCacheName+= PointToName(strFullFileName);
	} else {
		strCacheName+= strFullFileName;
	}

	PosCache PosCache{};

	if (Opt.EdOpt.SaveShortPos) {
		PosCache.Position[0] = pp->SavePos.Line;
		PosCache.Position[1] = pp->SavePos.Cursor;
		PosCache.Position[2] = pp->SavePos.ScreenLine;
		PosCache.Position[3] = pp->SavePos.LeftPos;
	}

	if (CtrlObject->EditorPosCache->GetPosition(strCacheName, PosCache)) {
		pp->Line = static_cast<int>(PosCache.Param[0]);
		pp->ScreenLine = static_cast<int>(PosCache.Param[1]);
		pp->LinePos = static_cast<int>(PosCache.Param[2]);
		pp->LeftPos = static_cast<int>(PosCache.Param[3]);
		POSCACHE_EDIT_PARAM4_UNPACK(PosCache.Param[4], pp->CodePage, pp->ExpandTabs, pp->TabSize);

		if (pp->Line < 0)
			pp->Line = 0;

		if (pp->ScreenLine < 0)
			pp->ScreenLine = 0;

		if (pp->LinePos < 0)
			pp->LinePos = 0;

		if (pp->LeftPos < 0)
			pp->LeftPos = 0;

		if ((int)pp->CodePage < 0)
			pp->CodePage = 0;

		return true;
	}

	return false;
}

FARString FileEditor::ComposeCacheName()
{
	return strPluginData.IsEmpty() ? strFullFileName : strPluginData + PointToName(strFullFileName);
}

void FileEditor::SaveToCache()
{
	EditorCacheParams cp;
	m_editor->GetCacheParams(&cp);

	if (!Flags.Check(FFILEEDIT_OPENFAILED))		//????
	{
		PosCache poscache{};
		poscache.Param[0] = cp.Line;
		poscache.Param[1] = cp.ScreenLine;
		poscache.Param[2] = cp.LinePos;
		poscache.Param[3] = cp.LeftPos;
		const int codepage = Flags.Check(FFILEEDIT_CODEPAGECHANGEDBYUSER) ? m_codepage : 0;
		POSCACHE_EDIT_PARAM4_PACK(poscache.Param[4], codepage, cp.ExpandTabs, cp.TabSize);

		if (Opt.EdOpt.SaveShortPos) {
			// if no position saved these are nulls
			poscache.Position[0] = cp.SavePos.Line;
			poscache.Position[1] = cp.SavePos.Cursor;
			poscache.Position[2] = cp.SavePos.ScreenLine;
			poscache.Position[3] = cp.SavePos.LeftPos;
		}

		CtrlObject->EditorPosCache->AddPosition(ComposeCacheName(), poscache);
	}
}

void FileEditor::SetCodePage(UINT codepage)
{
	if (codepage != m_codepage) {
		m_codepage = codepage;

		if (m_editor) {
			if (!m_editor->SetCodePage(m_codepage)) {
				Message(MSG_WARNING, 1, Msg::Warning, Msg::EditorSwitchCPWarn1, Msg::EditorSwitchCPWarn2,
						Msg::EditorSaveNotRecommended, Msg::Ok);
				BadConversion = true;
			}

			ChangeEditKeyBar();		//???
		}
	}
}

bool FileEditor::AskOverwrite(const FARString &FileName)
{
	bool result = true;
	DWORD FNAttr = apiGetFileAttributes(FileName);

	if (FNAttr != INVALID_FILE_ATTRIBUTES) {
		if (Message(MSG_WARNING, 2, Msg::EditTitle, FileName, Msg::EditExists, Msg::EditOvr, Msg::Yes,
					Msg::No)) {
			result = false;
		} else {
			Flags.Set(FFILEEDIT_SAVEWQUESTIONS);
		}
	}

	return result;
}

void EditConsoleHistory(HANDLE con_hnd, bool modal)
{
	FARString histfile(CtrlObject->CmdLine->GetConsoleLog(con_hnd, false));
	if (histfile.IsEmpty())
		return;

	std::shared_ptr<TempFileHolder> tfh(std::make_shared<TempFileHolder>(histfile, false));

	DWORD EditorFlags = FFILEEDIT_DISABLEHISTORY | FFILEEDIT_NEW | FFILEEDIT_SAVETOSAVEAS;
	if (!modal)
		EditorFlags|= FFILEEDIT_ENABLEF6;

	FileEditor *ShellEditor = new (std::nothrow) FileEditor(
		tfh, CP_UTF8, EditorFlags, std::numeric_limits<int>::max());
	if (ShellEditor) {
		DWORD editorExitCode = ShellEditor->GetExitCode();
		if (editorExitCode != XC_LOADING_INTERRUPTED && editorExitCode != XC_OPEN_ERROR) {
			FrameManager->ExecuteModal();
		} else
			delete ShellEditor;
	}
}

//////////////////////////////////
