#pragma once

/*
edit.hpp

Реализация одиночной строки редактирования
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

#include "scrobj.hpp"
#include "colors.hpp"
#include "farcolors.hpp"
#include "bitflags.hpp"
#include "FilesSuggestor.hpp"
#include "EcoString.hpp"
#include "EcoVector.hpp"
#include "EcoLazy.hpp"
#include <memory>
#include <vector>
#include <vector>

// Младший байт (маска 0xFF) юзается классом ScreenObject!!!
enum FLAGS_CLASS_EDITLINE
{
	FEDITLINE_MARKINGBLOCK     = 0x00000100,
	FEDITLINE_DROPDOWNBOX      = 0x00000200,
	FEDITLINE_CLEARFLAG        = 0x00000400,
	FEDITLINE_PASSWORDMODE     = 0x00000800,
	FEDITLINE_EDITBEYONDEND    = 0x00001000,
	FEDITLINE_EDITORMODE       = 0x00002000,
	FEDITLINE_OVERTYPE         = 0x00004000,
	FEDITLINE_DELREMOVESBLOCKS = 0x00008000,	// Del удаляет блоки (Opt.EditorDelRemovesBlocks)
	FEDITLINE_PERSISTENTBLOCKS = 0x00010000,	// Постоянные блоки (Opt.EditorPersistentBlocks)
	FEDITLINE_SHOWWHITESPACE = 0x00020000,
	FEDITLINE_READONLY       = 0x00040000,
	FEDITLINE_CURSORVISIBLE  = 0x00080000,
	// Если ни один из FEDITLINE_PARENT_ не указан (или указаны оба), то Edit
	// явно не в диалоге юзается.
	FEDITLINE_PARENT_SINGLELINE = 0x00100000,		// обычная строка ввода в диалоге
	FEDITLINE_PARENT_MULTILINE  = 0x00200000,		// для будущего Memo-Edit (DI_EDITOR или DIF_MULTILINE)
	FEDITLINE_PARENT_EDITOR     = 0x00400000,		// "вверху" обычный редактор
	FEDITLINE_LOCAL_SETTINGS    = 0x00800000,		// has own instance of settings
	FEDITLINE_HASSPECIALWIDTHCHARS = 0x01000000,
	FEDITLINE_WORDWRAP          = 0x02000000,
	FEDITLINE_EOLTYPE_MASK      = 0x1c000000,
	FEDITLINE_EOLTYPE_SHIFT     = 26,
};

struct ColorItem
{
	int StartPos;
	int EndPos;
	DWORD64 Color;
};

/*
interface ICPEncoder
{

	virtual int __stdcall AddRef() = 0;
	virtual int __stdcall Release() = 0;

	virtual const wchar_t* __stdcall GetName() = 0;
	virtual int __stdcall Encode(const char *lpString, int nLength, wchar_t *lpwszResult, int nResultLength) = 0;
	virtual int __stdcall Decode(const wchar_t *lpwszString, int nLength, char *lpResult, int nResultLength) = 0;
	virtual int __stdcall Transcode(const wchar_t *lpwszString, int nLength, ICPEncoder *pFrom, wchar_t *lpwszResult, int nResultLength) = 0;
};

class SystemCPEncoder : public ICPEncoder
{

	public:

		int m_nRefCount;
		int m_nCodePage; //system single-byte codepage

		FARString m_strName;

	public:

		SystemCPEncoder(int nCodePage);
		virtual ~SystemCPEncoder();

		virtual int __stdcall AddRef();
		virtual int __stdcall Release();

		virtual const wchar_t* __stdcall GetName();
		virtual int __stdcall Encode(const char *lpString, int nLength, wchar_t *lpwszResult, int nResultLength);
		virtual int __stdcall Decode(const wchar_t *lpwszString, int nLength, char *lpResult, int nResultLength);
		virtual int __stdcall Transcode(const wchar_t *lpwszString, int nLength, ICPEncoder *pFrom, wchar_t *lpwszResult, int nResultLength);
};
*/
class Dialog;
class Editor;

bool TranslateInsertKey(FarKey &Key);

class Edit;

struct IEditListener
{
	virtual void OnEditChanged(Edit *edit) = 0;
};

class Edit : public ThinScreenObject
{
	friend class DlgEdit;
	friend class Editor;
	friend class CommandLine;
	friend class EditControl;
	friend class PauseEditListener;
	friend class Edit2Settings;

public:
	Edit *m_next;
	Edit *m_prev;


private:
	struct Fields // lazily instantiated and accessed by EcoLazy
	{
		EcoVector<int> WrapBreaks;

		int LeftPos{0};
		int CurPos{0};
		int PrevCurPos{0};		// 12.08.2000 KM - предыдущее положение курсора
		int CursorPos{0};
		int MSelStart{-1};
		int SelStart{-1};
		int SelEnd{0};

		// things below needed for EcoLazy
		static Fields Default;

		bool IsDefault() const
		{
			return LeftPos == 0 && CurPos == 0 && PrevCurPos == 0 && CursorPos == 0
				&& MSelStart == -1 && SelStart == -1 && SelEnd == 0 && WrapBreaks.empty();
		}
	};
	struct MyEcoLazy : EcoLazy<Fields> {} fields;
	EcoVector<ColorItem> ColorList; // all colors will be fullfilled by colorer as its fast now
	EcoString Str;

private:
	virtual void DisplayObject();
	int InsertKey(FarKey Key);
	int RecurseProcessKey(FarKey Key);
	void DeleteBlock();
	void ApplyColor();
	int GetNextCursorPos(int Position, int Where);
	void RefreshStrByMask(int InitMode = FALSE);
	int KeyMatchedMask(FarKey Key);

	int ProcessCtrlQ();
	void RecalculateWordWrap(int Width, int TabSize);
	int ProcessInsDate(const wchar_t *Str);
	int ProcessInsPlainText(const wchar_t *Str);

	int CheckCharMask(wchar_t Chr);
	int ProcessInsPath(FarKey Key, int PrevSelStart = -1, int PrevSelEnd = 0);

	int RealPosToCell(int PrevLength, int PrevPos, int Pos, int *CorrectPos);
	void SanitizeSelectionRange();
	Editor *GetEditorOwner();
	DWORD TranscodeCodePage(UINT oldCodepage, UINT codepage);
	const wchar_t *WordDiv();
	void GetObjectColors(uint64_t &Color, uint64_t &SelColor, uint64_t &ColorUnChanged);
	int GetEndType() const { return (Flags.Flags & FEDITLINE_EOLTYPE_MASK) >> FEDITLINE_EOLTYPE_SHIFT; }
	void SetEndType(int Type) { Flags.Flags = (Flags.Flags & ~FEDITLINE_EOLTYPE_MASK) | (static_cast<DWORD>(Type) << FEDITLINE_EOLTYPE_SHIFT); }
	void CheckForSpecialWidthChars(const wchar_t *CheckStr = nullptr, int Length = 0);

protected:
	int CalcRTrimmedStrSize() const;

	int CalcPosFwdTo(int Pos, int LimitPos = -1) const;
	int CalcPosBwdTo(int Pos) const;

	inline int CalcPosFwd(int LimitPos = -1) const;
	inline int CalcPosBwd() const;

	int FindVisualLine(int Pos) const;
	int GetVisualLineCount() const;
	void GetVisualLine(int line, int& start, int& end) const;
public:
	Edit(ScreenObject *pOwner = nullptr);
	virtual ~Edit();

	void SetListener(IEditListener *Listener = nullptr);
	IEditListener *GetListener();

	void Compact() { Str.Compact(); }
	bool IsCompact() const { return Str.IsCompact(); }

	DWORD SetCodePage(UINT codepage);	// BUGBUG
	UINT GetCodePage();					// BUGBUG

	virtual void FastShow();
	virtual int ProcessKey(FarKey Key);
	virtual int ProcessMouse(MOUSE_EVENT_RECORD *MouseEvent);
	virtual int64_t VMProcess(MacroOpcode OpCode, void *vParam = nullptr, int64_t iParam = 0);

	// ! Функция установки текущих Color,SelColor и ColorUnChanged!
	void SetObjectColor(uint64_t Color, uint64_t SelColor = 0xf, uint64_t ColorUnChanged = FarColorToReal(COL_DIALOGEDITUNCHANGED));
	// + Функция получения текущих Color,SelColor
	long GetObjectColor();
	int GetObjectColorUnChanged();

	void SetTabSize(int NewSize);
	int GetTabSize();

	void SetDelRemovesBlocks(int Mode) { Flags.Change(FEDITLINE_DELREMOVESBLOCKS, Mode); }
	int GetDelRemovesBlocks() { return Flags.Check(FEDITLINE_DELREMOVESBLOCKS); }

	void SetPersistentBlocks(int Mode) { Flags.Change(FEDITLINE_PERSISTENTBLOCKS, Mode); }
	int GetPersistentBlocks() { return Flags.Check(FEDITLINE_PERSISTENTBLOCKS); }

	void SetShowWhiteSpace(int Mode) { Flags.Change(FEDITLINE_SHOWWHITESPACE, Mode); }

	void GetString(int Offset, wchar_t *Data, int MaxSize);
	inline void GetString(wchar_t *Data, int MaxSize)
	{
		GetString(0, Data, MaxSize);
	}

	template <class DST_T>
		void GetString(DST_T &dst, const wchar_t **EOL = nullptr)
	{
		if (EOL)
			*EOL = GetEOL();

		Str.CopyTo(dst);
	}

	template <class CMP_T>
		bool EqualTo(CMP_T &to)
	{
		return Str.EqualTo(to);
	}

	std::wstring GetString()
	{
		std::wstring out;
		Str.CopyTo(out);
		return out;
	}

	int GetLength(const wchar_t **EOL = nullptr);

	// NB: GetStringAddr functions have implicit memory overhead due to they forcing uncompacting of underlying string
	// so prefer use GetString()/GetLength() if need to massive-query multiple lines, or use Compact() afterwards
	// to avoid memory usage surge
	const wchar_t *GetStringAddr(int &Length, const wchar_t **EOL = nullptr);
	const wchar_t *GetStringAddr();

	inline const wchar_t GetChar(int Pos) // similar to but faster than GetStringAddr()[Pos]
	{
		return Str.At(Pos);
	}

	void SetHiString(const wchar_t *Str);
	void SetString(const wchar_t *Str, int Length = -1);

	void SetBinaryString(const wchar_t *Str, int Length);

	void SetEOL(const wchar_t *EOL);
	void SetEOL(const char *EOL);
	const wchar_t *GetEOL();

	int GetSelString(wchar_t *Data, int MaxSize);
	int GetSelString(FARString &strStr);

	void InsertString(const wchar_t *Str);
	void InsertBinaryString(const wchar_t *Str, int Length);

	int Search(const FARString &What, FARString &ReplaceStr, int Position, int Case, int WholeWords,
			int Reverse, int Regexp, int *SearchLength);

	void SetClearFlag(int Flag) { Flags.Change(FEDITLINE_CLEARFLAG, Flag); }
	int GetClearFlag() { return Flags.Check(FEDITLINE_CLEARFLAG); }
	void SetCurPos(int NewPos);
	int GetCurPos();
	int GetCellCurPos();
	void SetCellCurPos(int NewPos);
	int GetLeftPos();
	void SetLeftPos(int NewPos);
	void SetPasswordMode(int Mode) { Flags.Change(FEDITLINE_PASSWORDMODE, Mode); };
	void SetMaxLength(int Length);

	// Получение максимального значения строки для потребностей Dialod API
	int GetMaxLength() const;

	void SetInputMask(const wchar_t *InputMask);
	const wchar_t *GetInputMask() const;

	void SetOvertypeMode(int Mode) { Flags.Change(FEDITLINE_OVERTYPE, Mode); };
	int GetOvertypeMode() { return Flags.Check(FEDITLINE_OVERTYPE); };

	void SetConvertTabs(int Mode);
	int GetConvertTabs();

	int RealPosToCell(int Pos);
	int CellPosToReal(int Pos);
	void Select(int Start, int End);
	void AddSelect(int Start, int End);

	bool IsSelection();

	struct Selection { int Start, End; };

	Selection GetSelection();
	void GetSelection(int &Start, int &End)
	{
		const auto &Sel = GetSelection();
		Start = Sel.Start;
		End = Sel.End;
	}

	Selection GetRealSelection();
	void GetRealSelection(int &Start, int &End)
	{
		const auto &Sel = GetRealSelection();
		Start = Sel.Start;
		End = Sel.End;
	}

	void SetEditBeyondEnd(int Mode) { Flags.Change(FEDITLINE_EDITBEYONDEND, Mode); };
	void SetWordWrap(int Wrap) { Flags.Change(FEDITLINE_WORDWRAP, Wrap != 0); }
	bool GetWordWrap() const { return Flags.Check(FEDITLINE_WORDWRAP); }
	void SetEditorMode(int Mode) { Flags.Change(FEDITLINE_EDITORMODE, Mode); };
	void SetEditorParent(int Mode) { Flags.Change(FEDITLINE_PARENT_EDITOR, Mode); };
	void ExpandTabs();

	void AddColor(const ColorItem *col);
	size_t DeleteColor(int ColorPos);
	bool GetColor(ColorItem *col, int Item);

	void Xlat(bool All = false);

	void SetDialogParent(DWORD Sets);
	void SetCursorType(bool Visible, DWORD Size);
	void GetCursorType(bool &Visible, DWORD &Size);
	void SetCursorVisibleFlag(bool Visible) { Flags.Change(FEDITLINE_CURSORVISIBLE, Visible); }
	int GetReadOnly() { return Flags.Check(FEDITLINE_READONLY); }
	void SetReadOnly(int NewReadOnly) { Flags.Change(FEDITLINE_READONLY, NewReadOnly); }
	int GetDropDownBox() { return Flags.Check(FEDITLINE_DROPDOWNBOX); }
	void SetDropDownBox(int NewDropDownBox) { Flags.Change(FEDITLINE_DROPDOWNBOX, NewDropDownBox); }
	virtual void Changed(bool DelBlock = false);
};

class History;
class VMenu;
// Надстройка над Edit.
// Одиночная строка ввода для диалогов и комстроки (не для редактора)

class EditControl : public Edit
{
	friend class DlgEdit;

	std::unique_ptr<MenuFilesSuggestor> m_pSuggestor;
	const std::vector<std::string> *pCustomCompletionList;
	History *pHistory;
	FarList *pList;
	bool Selection;
	int SelectionStart;
	uint64_t OverflowArrowsColor;
	BitFlags ECFlags;
	bool ACState;

	void SetMenuPos(VMenu &menu);
	void AutoCompleteProcMenu(bool &Result, bool Manual, bool DelBlock, FarKey &BackKey);
	bool AutoCompleteProc(bool Manual, bool DelBlock, FarKey &BackKey);
	void PopulateCompletionMenu(VMenu &ComplMenu, const FARString &strFilter);
	void RemoveSelectedCompletionMenuItem(VMenu &ComplMenu);
	virtual void ShowArrows();

public:
	enum ECFLAGS
	{
		EC_ENABLEAUTOCOMPLETE       = 0x1,
		EC_ENABLEFNCOMPLETE         = 0x2,
		EC_ENABLEFNCOMPLETE_ESCAPED = 0x4,
	};

	EditControl(ScreenObject *pOwner = nullptr, History *iHistory = 0, FarList *iList = 0, DWORD iFlags = 0);

	virtual int ProcessMouse(MOUSE_EVENT_RECORD *MouseEvent);
	virtual int ProcessKey(FarKey Key);
	virtual void FastShow();
	virtual void Show();
	virtual void Changed(bool DelBlock = false);

	void AutoComplete(bool Manual, bool DelBlock);
	void EnableAC(bool Permanent = false);
	void DisableAC(bool Permanent = false);
	void RevertAC() { ACState ? EnableAC() : DisableAC(); }
	void SetFNComplete(bool Enable)
	{
		if (Enable)
			ECFlags.Set(EC_ENABLEFNCOMPLETE);
		else
			ECFlags.Clear(EC_ENABLEFNCOMPLETE);
	}
	void ShowCustomCompletionList(const std::vector<std::string> &list);
	void SetOverflowArrowsColor(uint64_t Color) { OverflowArrowsColor = Color; }
};
