/*
dlgedit.cpp

Одиночная строка редактирования для диалога (как наследник класса Edit)
Мультиредактор
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

#include "dlgedit.hpp"
#include "dialog.hpp"
#include "history.hpp"
#include "ctrlobj.hpp"

namespace
{
class DialogEditorPluginScope
{
public:
	DialogEditorPluginScope(Editor *editor)
		: m_prev(CtrlObject ? CtrlObject->Plugins.CurDialogEditor : nullptr)
	{
		if (CtrlObject)
			CtrlObject->Plugins.CurDialogEditor = editor;
	}
	~DialogEditorPluginScope()
	{
		if (CtrlObject)
			CtrlObject->Plugins.CurDialogEditor = m_prev;
	}

private:
	Editor *m_prev;
};

void NotifyDialogEditorFocus(Editor *editor, bool &opened, bool &has_focus, bool focus)
{
	if (!editor || !CtrlObject)
		return;

	DialogEditorPluginScope scope(editor);
	if (!opened) {
		CtrlObject->Plugins.ProcessEditorEvent(EE_READ, nullptr);
		opened = true;
	}

	if (has_focus == focus)
		return;

	int id = editor->GetEditorID();
	CtrlObject->Plugins.ProcessEditorEvent(focus ? EE_GOTFOCUS : EE_KILLFOCUS, &id);
	has_focus = focus;
}
}

DlgEdit::DlgEdit(Dialog *pOwner, unsigned Index, DLGEDITTYPE Type)
	:
	LastPartLength(-1),
	m_Dialog(pOwner),
	m_Index(Index),
	Type(Type),
	iHistory(nullptr),
	lineEdit(nullptr)
	,
	multiEdit(nullptr),
	m_dialogHasFocus(false),
	m_dialogEditorOpened(false)
{
	switch (Type) {
		case DLGEDIT_MULTILINE:
			multiEdit = new Editor(pOwner, true);	// ??? (pOwner) ?
			multiEdit->SetShowScrollBar(1);
			if (pOwner) {
				DialogItemEx *CurItem = pOwner->Item[Index];
				if (CurItem && IsPtr(CurItem->UserData)) {
					multiEdit->SetVirtualFileName(reinterpret_cast<const wchar_t *>(CurItem->UserData));
				}
			}
			break;
		case DLGEDIT_SINGLELINE: {
			Edit::Callback callback = {true, EditChange, this};

			iHistory = 0;
			FarList *iList = 0;
			DWORD iFlags = 0;
			if (pOwner) {
				DialogItemEx *CurItem = pOwner->Item[Index];
				if (Opt.Dialogs.AutoComplete && CurItem->Flags & (DIF_HISTORY | DIF_EDITPATH)
						&& !(CurItem->Flags & DIF_DROPDOWNLIST) && !(CurItem->Flags & DIF_NOAUTOCOMPLETE)) {
					iFlags = EditControl::EC_ENABLEAUTOCOMPLETE;
				}
				if (CurItem->Flags & DIF_HISTORY && !CurItem->strHistory.IsEmpty()) {
					FARString strHistory = fmtSavedDialogHistory;
					strHistory+= CurItem->strHistory;
					iHistory = new History(HISTORYTYPE_DIALOG, Opt.DialogsHistoryCount, strHistory.GetMB(),
							&Opt.Dialogs.EditHistory, false);
				}
				if (CurItem->Type == DI_COMBOBOX) {
					iList = CurItem->ListItems;
				}
				if (CurItem->Flags & DIF_EDITPATH) {
					iFlags|= EditControl::EC_ENABLEFNCOMPLETE;
				}
			}
			lineEdit = new EditControl(pOwner, &callback, true, iHistory, iList, iFlags);
		} break;
	}
}

DlgEdit::~DlgEdit()
{
	if (iHistory) {
		delete iHistory;
	}

	if (lineEdit)
		delete lineEdit;


	if (multiEdit) {
		if (m_dialogEditorOpened && CtrlObject) {
			DialogEditorPluginScope scope(multiEdit);
			int id = multiEdit->GetEditorID();
			CtrlObject->Plugins.ProcessEditorEvent(EE_CLOSE, &id);
		}
		delete multiEdit;
	}

}

int DlgEdit::ProcessKey(FarKey Key)
{

	if (Type == DLGEDIT_MULTILINE) {
		DialogEditorPluginScope scope(multiEdit);
		return multiEdit->ProcessKey(Key);
	} else
		return lineEdit->ProcessKey(Key);
}

int DlgEdit::ProcessMouse(MOUSE_EVENT_RECORD *MouseEvent)
{

	if (Type == DLGEDIT_MULTILINE) {
		DialogEditorPluginScope scope(multiEdit);
		return multiEdit->ProcessMouse(MouseEvent);
	} else
		return lineEdit->ProcessMouse(MouseEvent);
}

void DlgEdit::DisplayObject()
{

	if (Type == DLGEDIT_MULTILINE) {
		if (m_Dialog && m_Index < m_Dialog->ItemCount) {
			DialogItemEx *CurItem = m_Dialog->Item[m_Index];
			if (CurItem) {
				multiEdit->SetShowCursor(CurItem->Focus != 0);
			}
		}
		NotifyDialogEditorFocus(multiEdit, m_dialogEditorOpened, m_dialogHasFocus, true);
		DialogEditorPluginScope scope(multiEdit);
		multiEdit->DisplayObject();
	} else
		lineEdit->DisplayObject();
}

void DlgEdit::SetPosition(int X1, int Y1, int X2, int Y2)
{

	if (Type == DLGEDIT_MULTILINE)
		multiEdit->SetPosition(X1, Y1, X2, Y2);
	else
		lineEdit->SetPosition(X1, Y1, X2, Y2);
}

void DlgEdit::Show()
{

	if (Type == DLGEDIT_MULTILINE) {
		if (m_Dialog && m_Index < m_Dialog->ItemCount) {
			DialogItemEx *CurItem = m_Dialog->Item[m_Index];
			if (CurItem) {
				multiEdit->SetShowCursor(CurItem->Focus != 0);
			}
		}
		NotifyDialogEditorFocus(multiEdit, m_dialogEditorOpened, m_dialogHasFocus, true);
		DialogEditorPluginScope scope(multiEdit);
		multiEdit->Show();
	} else
		lineEdit->Show();
}

void DlgEdit::GetPosition(int &X1, int &Y1, int &X2, int &Y2)
{

	if (Type == DLGEDIT_MULTILINE)
		multiEdit->GetPosition(X1, Y1, X2, Y2);
	else
		lineEdit->GetPosition(X1, Y1, X2, Y2);
}

void DlgEdit::SetDialogParent(DWORD Sets)
{

	if (Type == DLGEDIT_MULTILINE)
		multiEdit->SetDialogParent(Sets);
	else
		lineEdit->SetDialogParent(Sets);
}

void DlgEdit::SetDropDownBox(int NewDropDownBox)
{
	if (Type == DLGEDIT_SINGLELINE)
		lineEdit->SetDropDownBox(NewDropDownBox);
}

int DlgEdit::GetMaxLength()
{
	if (Type == DLGEDIT_SINGLELINE)
		return lineEdit->GetMaxLength();

	return 0;
}

void DlgEdit::SetMaxLength(int Length)
{
	if (Type == DLGEDIT_SINGLELINE)
		lineEdit->SetMaxLength(Length);
}

void DlgEdit::SetPasswordMode(int Mode)
{
	if (Type == DLGEDIT_SINGLELINE)
		lineEdit->SetPasswordMode(Mode);
}

void DlgEdit::SetOvertypeMode(int Mode)
{

	if (Type == DLGEDIT_MULTILINE)
		multiEdit->SetOvertypeMode(Mode);
	else
		lineEdit->SetOvertypeMode(Mode);
}

int DlgEdit::GetOvertypeMode()
{

	if (Type == DLGEDIT_MULTILINE)
		return multiEdit->GetOvertypeMode();
	else
		return lineEdit->GetOvertypeMode();
}

void DlgEdit::SetInputMask(const wchar_t *InputMask)
{
	if (Type == DLGEDIT_SINGLELINE)
		lineEdit->SetInputMask(InputMask);
}

const wchar_t *DlgEdit::GetInputMask()
{
	if (Type == DLGEDIT_SINGLELINE)
		return lineEdit->GetInputMask();

	return L"";		//???
}

void DlgEdit::SetEditBeyondEnd(int Mode)
{

	if (Type == DLGEDIT_MULTILINE)
		multiEdit->SetEditBeyondEnd(Mode);
	else
		lineEdit->SetEditBeyondEnd(Mode);
}

void DlgEdit::SetClearFlag(int Flag)
{

	if (Type == DLGEDIT_MULTILINE)
		multiEdit->SetClearFlag(Flag);
	else
		lineEdit->SetClearFlag(Flag);
}

int DlgEdit::GetClearFlag()
{

	if (Type == DLGEDIT_MULTILINE)
		return multiEdit->GetClearFlag();
	else
		return lineEdit->GetClearFlag();
}

const wchar_t *DlgEdit::GetStringAddr()
{

	if (Type == DLGEDIT_MULTILINE) {
		return nullptr;
	} else
		return lineEdit->GetStringAddr();
}

void DlgEdit::SetHiString(const wchar_t *Str)
{

	if (Type == DLGEDIT_MULTILINE) {
		// Not supported for multiline editor control.
	} else
		lineEdit->SetHiString(Str);
}

void DlgEdit::SetString(const wchar_t *Str)
{

	if (Type == DLGEDIT_MULTILINE) {
		if (multiEdit)
			multiEdit->SetRawData(Str, -1, 0);
	} else
		lineEdit->SetString(Str);
}

void DlgEdit::InsertString(const wchar_t *Str)
{
	if (Type == DLGEDIT_MULTILINE) {
		if (multiEdit)
			multiEdit->SetRawData(Str, -1, 0);
	} else
		lineEdit->InsertString(Str);
}

void DlgEdit::GetString(wchar_t *Str, int MaxSize, int Row)
{

	if (Type == DLGEDIT_MULTILINE) {
		if (!multiEdit || !Str || MaxSize <= 0)
			return;
		if (Row >= 0) {
			Edit *line = multiEdit->GetStringByNumber(Row);
			if (!line) {
				*Str = 0;
				return;
			}
			line->GetString(Str, MaxSize);
		} else {
			wchar_t *buf = nullptr;
			int size = 0;
			if (!multiEdit->GetRawData(&buf, size, 1) || !buf) {
				*Str = 0;
				return;
			}
			int copy_len = Min(size, MaxSize - 1);
			wmemcpy(Str, buf, copy_len);
			Str[copy_len] = 0;
			free(buf);
		}
	} else
		lineEdit->GetString(Str, MaxSize);
}

void DlgEdit::GetString(FARString &strStr, int Row)
{

	if (Type == DLGEDIT_MULTILINE) {
		if (!multiEdit) {
			strStr.Clear();
			return;
		}
		if (Row >= 0) {
			Edit *line = multiEdit->GetStringByNumber(Row);
			if (!line) {
				strStr.Clear();
				return;
			}
			line->GetString(strStr);
		} else {
			wchar_t *buf = nullptr;
			int size = 0;
			if (!multiEdit->GetRawData(&buf, size, 1) || !buf) {
				strStr.Clear();
				return;
			}
			strStr.Copy(buf, size);
			free(buf);
		}
	} else
		lineEdit->GetString(strStr);
}

void DlgEdit::SetCurPos(int NewCol, int NewRow)		// Row==-1 - current line
{

	if (Type == DLGEDIT_MULTILINE)
		multiEdit->SetCurPos(NewCol, NewRow);
	else
		lineEdit->SetCurPos(NewCol);
}

int DlgEdit::GetCurPos()
{

	if (Type == DLGEDIT_MULTILINE)
		return multiEdit->GetCurPos();	// GetCurCol???
	else
		return lineEdit->GetCurPos();
}

int DlgEdit::GetCurRow()
{

	if (Type == DLGEDIT_MULTILINE)
		return multiEdit->GetCurRow();
	else
		return 0;
}

int DlgEdit::GetCellCurPos()
{

	if (Type == DLGEDIT_MULTILINE)
		return multiEdit->GetCurPos();	// GetCurCol???
	else
		return lineEdit->GetCellCurPos();
}

void DlgEdit::SetCellCurPos(int NewPos)
{

	if (Type == DLGEDIT_MULTILINE)
		multiEdit->SetCurPos(NewPos, multiEdit->GetCurRow());	//???
	else
		lineEdit->SetCellCurPos(NewPos);
}

void DlgEdit::SetPersistentBlocks(int Mode)
{

	if (Type == DLGEDIT_MULTILINE)
		multiEdit->SetPersistentBlocks(Mode);
	else
		lineEdit->SetPersistentBlocks(Mode);
}

int DlgEdit::GetPersistentBlocks()
{

	if (Type == DLGEDIT_MULTILINE)
		return multiEdit->GetPersistentBlocks();
	else
		return lineEdit->GetPersistentBlocks();
}

void DlgEdit::SetDelRemovesBlocks(int Mode)
{

	if (Type == DLGEDIT_MULTILINE)
		multiEdit->SetDelRemovesBlocks(Mode);
	else
		lineEdit->SetDelRemovesBlocks(Mode);
}

int DlgEdit::GetDelRemovesBlocks()
{

	if (Type == DLGEDIT_MULTILINE)
		return multiEdit->GetDelRemovesBlocks();
	else
		return lineEdit->GetDelRemovesBlocks();
}

void DlgEdit::SetObjectColor(uint64_t Color, uint64_t SelColor, uint64_t ColorUnChanged)
{

	if (Type == DLGEDIT_MULTILINE)
		multiEdit->SetObjectColor(Color, SelColor, ColorUnChanged);
	else
		lineEdit->SetObjectColor(Color, SelColor, ColorUnChanged);
}

long DlgEdit::GetObjectColor()
{

	if (Type == DLGEDIT_MULTILINE)
		return 0;	// multiEdit->GetObjectColor();
	else
		return lineEdit->GetObjectColor();
}

int DlgEdit::GetObjectColorUnChanged()
{

	if (Type == DLGEDIT_MULTILINE)
		return 0;	// multiEdit->GetObjectColorUnChanged();
	else
		return lineEdit->GetObjectColorUnChanged();
}

void DlgEdit::SetOverflowArrowsColor(uint64_t Color)
{

	if (Type == DLGEDIT_MULTILINE)
		return;
	else
		lineEdit->SetOverflowArrowsColor(Color);
}

void DlgEdit::FastShow()
{

	if (Type == DLGEDIT_MULTILINE) {
		if (m_Dialog && m_Index < m_Dialog->ItemCount) {
			DialogItemEx *CurItem = m_Dialog->Item[m_Index];
			if (CurItem) {
				multiEdit->SetShowCursor(CurItem->Focus != 0);
			}
		}
		NotifyDialogEditorFocus(multiEdit, m_dialogEditorOpened, m_dialogHasFocus, false);
		DialogEditorPluginScope scope(multiEdit);
		multiEdit->Show();
	} else
		lineEdit->FastShow();
}

int DlgEdit::GetLeftPos()
{

	if (Type == DLGEDIT_MULTILINE)
		return 0;	// multiEdit->GetLeftPos();
	else
		return lineEdit->GetLeftPos();
}

void DlgEdit::SetLeftPos(int NewPos, int Row)	// Row==-1 - current line
{

	if (Type == DLGEDIT_MULTILINE)
		;	// multiEdit->SetLeftPos(NewPos,Row);
	else
		lineEdit->SetLeftPos(NewPos);
}

void DlgEdit::DeleteBlock()
{

	if (Type == DLGEDIT_MULTILINE)
		multiEdit->DeleteBlock();
	else
		lineEdit->DeleteBlock();
}

int DlgEdit::GetLength()
{

	if (Type == DLGEDIT_MULTILINE) {
		if (!multiEdit)
			return 0;
		wchar_t *buf = nullptr;
		int size = 0;
		if (!multiEdit->GetRawData(&buf, size, 1) || !buf) {
			fprintf(stderr, "DlgEdit::GetLength multiline getraw failed\n");
			return 0;
		}
		free(buf);
		return size;
	} else
		return lineEdit->GetLength();
}

void DlgEdit::Select(int Start, int End)
{

	if (Type == DLGEDIT_MULTILINE)
		;	// multiEdit->Select(Start,End);
	else
		lineEdit->Select(Start, End);
}

void DlgEdit::GetSelection(int &Start, int &End)
{

	if (Type == DLGEDIT_MULTILINE)
		;	// multiEdit->GetSelection();
	else
		lineEdit->GetSelection(Start, End);
}

void DlgEdit::Xlat(bool All)
{

	if (Type == DLGEDIT_MULTILINE)
		multiEdit->Xlat();
	else
		lineEdit->Xlat(All);
}

int DlgEdit::GetStrSize(int Row)
{

	if (Type == DLGEDIT_MULTILINE)
		return 0;	// multiEdit->
	else
		return lineEdit->StrSize;
}

void DlgEdit::SetCursorType(bool Visible, DWORD Size)
{

	if (Type == DLGEDIT_MULTILINE)
		multiEdit->SetCursorType(Visible, Size);
	else
		lineEdit->SetCursorType(Visible, Size);
}

void DlgEdit::GetCursorType(bool &Visible, DWORD &Size)
{

	if (Type == DLGEDIT_MULTILINE)
		multiEdit->GetCursorType(Visible, Size);
	else
		lineEdit->GetCursorType(Visible, Size);
}

void DlgEdit::ToggleShowWhiteSpace()
{
	if (Type == DLGEDIT_MULTILINE && multiEdit) {
		multiEdit->SetShowWhiteSpace(!multiEdit->GetShowWhiteSpace());
		multiEdit->Show();
	}
}

void DlgEdit::ToggleShowLineNumbers()
{
	if (Type == DLGEDIT_MULTILINE && multiEdit) {
		multiEdit->SetShowLineNumbers(!multiEdit->GetShowLineNumbers());
		multiEdit->Show();
	}
}

void DlgEdit::ToggleWordWrap()
{
	if (Type == DLGEDIT_MULTILINE && multiEdit) {
		multiEdit->SetWordWrap(!multiEdit->GetWordWrap());
		multiEdit->Show();
	}
}

int DlgEdit::GetReadOnly()
{

	if (Type == DLGEDIT_MULTILINE)
		return multiEdit->GetReadOnly();
	else
		return lineEdit->GetReadOnly();
}

void DlgEdit::SetReadOnly(int NewReadOnly)
{

	if (Type == DLGEDIT_MULTILINE)
		multiEdit->SetReadOnly(NewReadOnly);
	else
		lineEdit->SetReadOnly(NewReadOnly);
}

BitFlags &DlgEdit::Flags()
{

	if (Type == DLGEDIT_MULTILINE)
		return multiEdit->Flags;
	else
		return lineEdit->Flags;
}

void DlgEdit::Hide()
{

	if (Type == DLGEDIT_MULTILINE)
		multiEdit->Hide();
	else
		lineEdit->Hide();
}

void DlgEdit::Hide0()
{

	if (Type == DLGEDIT_MULTILINE)
		multiEdit->Hide0();
	else
		lineEdit->Hide0();
}

void DlgEdit::ShowConsoleTitle()
{

	if (Type == DLGEDIT_MULTILINE)
		multiEdit->ShowConsoleTitle();
	else
		lineEdit->ShowConsoleTitle();
}

void DlgEdit::SetScreenPosition()
{

	if (Type == DLGEDIT_MULTILINE)
		multiEdit->SetScreenPosition();
	else
		lineEdit->SetScreenPosition();
}

void DlgEdit::ResizeConsole()
{

	if (Type == DLGEDIT_MULTILINE)
		multiEdit->ResizeConsole();
	else
		lineEdit->ResizeConsole();
}

int64_t DlgEdit::VMProcess(MacroOpcode OpCode, void *vParam, int64_t iParam)
{

	if (Type == DLGEDIT_MULTILINE)
		return multiEdit->VMProcess(OpCode, vParam, iParam);
	else
		return lineEdit->VMProcess(OpCode, vParam, iParam);
}

void DlgEdit::EditChange(void *aParam)
{
	static_cast<DlgEdit *>(aParam)->DoEditChange();
}

void DlgEdit::DoEditChange()
{
	if (m_Dialog->IsInited()) {
		SendDlgMessage((HANDLE)m_Dialog, DN_EDITCHANGE, m_Index, 0);
	}
}

bool DlgEdit::HistoryGetSimilar(FARString &strStr, int LastCmdPartLength, bool bAppend)
{
	return iHistory ? iHistory->GetSimilar(strStr, LastCmdPartLength, bAppend) : false;
}
