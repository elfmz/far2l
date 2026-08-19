/*
edit.cpp

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

#include "headers.hpp"

#include "edit.hpp"
#include "keyboard.hpp"
#include "macroopcode.hpp"
#include "lang.hpp"
#include "keys.hpp"
#include "editor.hpp"
#include "ctrlobj.hpp"
#include "filepanels.hpp"
#include "filelist.hpp"
#include "panel.hpp"
#include "scrbuf.hpp"
#include "interf.hpp"
#include "farcolors.hpp"
#include "clipboard.hpp"
#include "xlat.hpp"
#include "datetime.hpp"
#include "Bookmarks.hpp"
#include "pathmix.hpp"
#include "strmix.hpp"
#include "panelmix.hpp"
#include "RegExp.hpp"
#include "history.hpp"
#include "vmenu.hpp"
#include "chgmmode.hpp"
#include <cwctype>
#include <unordered_map>

static int Recurse = 0;

struct LocalSettings
{
	struct MaskDeleter
	{
		void operator()(wchar_t *p) const { free(p); }
	};
	std::unique_ptr<wchar_t, MaskDeleter> Mask;
	IEditListener *Listener{nullptr};
	uint64_t Color{F_LIGHTGRAY | B_BLACK};
	uint64_t SelColor{F_WHITE | B_BLACK};
	uint64_t ColorUnChanged{FarColorToReal(COL_DIALOGEDITUNCHANGED)};
	int TabSize{Opt.EdOpt.TabSize};
	int TabExpandMode{EXPAND_NOTABS};
	int MaxLength{-1};
	int CursorSize{-1};
	UINT codepage{0};
};

static class Edit2Settings
{
	std::unordered_map<const Edit *, LocalSettings> _m;

public:
	void Dismiss(const Edit *edit)
	{
		if (edit->Flags.Check(FEDITLINE_LOCAL_SETTINGS)) {
			if (!_m.erase(edit)) {
				fprintf(stderr, "Could not dismiss local edit settings\n");
			}
		}
	}

	const LocalSettings *Get(const Edit *edit) const
	{
		if (edit->Flags.Check(FEDITLINE_LOCAL_SETTINGS)) {
			if (auto it = _m.find(edit); it != _m.end()) {
				return &it->second;
			}
		}
		return nullptr;
	}

	LocalSettings *Get(Edit *edit, bool ensure = false)
	{
		if (edit->Flags.Check(FEDITLINE_LOCAL_SETTINGS) || ensure) {
			auto ir = _m.emplace(edit, LocalSettings());
			if (ir.second) {
				edit->Flags.Set(FEDITLINE_LOCAL_SETTINGS);
				if (_m.size() < 0x10 || _m.size() == 0x80 || _m.size() == 0x100 || (_m.size() & 0xffff) == 0x1000) {
					fprintf(stderr, "Local edit settings count %lu\n", (unsigned long)_m.size());
				}
			}
			return &ir.first->second;
		}
		return nullptr;
	}

} s_e2s;


class PauseEditListener
{
	Edit *_edit;
	IEditListener *_saved_listener;
public:
	PauseEditListener(Edit &edit) : _edit(&edit), _saved_listener(nullptr)
	{
		if (auto *s = s_e2s.Get(_edit)) {
			_saved_listener = s->Listener;
			s->Listener = nullptr;
		}
	}
	~PauseEditListener()
	{
		Resume();
	}
	void Resume()
	{
		if (_saved_listener) {
			if (auto *s = s_e2s.Get(_edit); s && !s->Listener) {
				s->Listener = _saved_listener;
			}
		}
	}
};

Edit::Fields Edit::Fields::Default;

static std::vector<wchar_t> s_render_buffer;

enum
{
	EOL_NONE,
	EOL_CR,
	EOL_LF,
	EOL_CRLF,
	EOL_CRCRLF
};
static const wchar_t *EOL_TYPE_CHARS[] = {L"", L"\r", L"\n", L"\r\n", L"\r\r\n"};

bool TranslateInsertKey(FarKey &Key)
{
	switch (Key) {
		case KEY_ADD:
			Key = L'+';
			return true;
		case KEY_SUBTRACT:
			Key = L'-';
			return true;
		case KEY_MULTIPLY:
			Key = L'*';
			return true;
		case KEY_DIVIDE:
			Key = L'/';
			return true;
		case KEY_DECIMAL:
			Key = L'.';
			return true;
		case KEY_SHIFTSPACE:
			Key = L' ';
			return true;
		case KEY_TAB:
			return true;
		default:
			return Key >= L' ' && WCHAR_IS_VALID(Key);
	}
}

#define EDMASK_ANY    L'X'		// позволяет вводить в строку ввода любой символ;
#define EDMASK_DSS    L'#'		// позволяет вводить в строку ввода цифры, пробел и знак минуса;
#define EDMASK_DIGIT  L'9'		// позволяет вводить в строку ввода только цифры;
#define EDMASK_DIGITS L'N'		// позволяет вводить в строку ввода только цифры и пробелы;
#define EDMASK_ALPHA  L'A'		// позволяет вводить в строку ввода только буквы.
#define EDMASK_HEX    L'H'		// позволяет вводить в строку ввода шестнадцатиричные символы.

Edit::Edit(ScreenObject *pOwner)
	:
	m_next(nullptr),
	m_prev(nullptr)
{
	SetOwner(pOwner);
	Flags.Set(FEDITLINE_EDITBEYONDEND);
	Flags.Set(FEDITLINE_CURSORVISIBLE);
	SetEndType(EOL_NONE);
	Flags.Change(FEDITLINE_DELREMOVESBLOCKS, Opt.EdOpt.DelRemovesBlocks);
	Flags.Change(FEDITLINE_PERSISTENTBLOCKS, Opt.EdOpt.PersistentBlocks);
	Flags.Change(FEDITLINE_SHOWWHITESPACE, Opt.EdOpt.ShowWhiteSpace);
}

Edit::~Edit()
{
	s_e2s.Dismiss(this);
}

void Edit::SetListener(IEditListener *Listener)
{
	if (auto *s = s_e2s.Get(this, Listener != nullptr)) {
		s->Listener = Listener;
	}
}

IEditListener *Edit::GetListener()
{
	if (auto *s = s_e2s.Get(this)) {
		return s->Listener;
	}
	return nullptr;
}


Editor *Edit::GetEditorOwner()
{
	if (!Flags.Check(FEDITLINE_PARENT_EDITOR))
		return nullptr;

	return static_cast<Editor *>(GetOwner());
}

void Edit::SetTabSize(int NewSize)
{
	if (auto *editor = GetEditorOwner())
		editor->SetTabSize(NewSize);
	else if (auto *s = s_e2s.Get(this, true))
		s->TabSize = NewSize;
}

int Edit::GetTabSize()
{
	if (auto *editor = GetEditorOwner())
		return editor->GetTabSize();

	if (auto *s = s_e2s.Get(this))
		return s->TabSize;

	return Opt.EdOpt.TabSize;
}

void Edit::SetMaxLength(int Length)
{
	if (auto *s = s_e2s.Get(this, Length != -1))
		s->MaxLength = Length;
}

int Edit::GetMaxLength() const
{
	if (auto *s = s_e2s.Get(this))
		return s->MaxLength;

	return -1;
}

void Edit::SetConvertTabs(int Mode)
{
	if (auto *editor = GetEditorOwner())
		editor->SetConvertTabs(Mode);
	else if (auto *s = s_e2s.Get(this, true))
		s->TabExpandMode = Mode;
}

int Edit::GetConvertTabs()
{
	if (auto *editor = GetEditorOwner())
		return editor->GetConvertTabs();

	if (auto *s = s_e2s.Get(this))
		return s->TabExpandMode;

	return EXPAND_NOTABS;
}

const wchar_t *Edit::WordDiv()
{
	if (auto *editor = GetEditorOwner())
		return editor->GetWordDiv();

	return Opt.strWordDiv.CPtr();
}

void Edit::GetObjectColors(uint64_t &Color, uint64_t &SelColor, uint64_t &ColorUnChanged)
{
	if (auto *editor = GetEditorOwner()) {
		Color = editor->m_Color;
		SelColor = editor->m_SelColor;
		ColorUnChanged = editor->m_ColorUnChanged;
		return;
	}

	if (auto *s = s_e2s.Get(this)) {
		Color = s->Color;
		SelColor = s->SelColor;
		ColorUnChanged = s->ColorUnChanged;
		return;
	}

	Color = F_LIGHTGRAY | B_BLACK;
	SelColor = F_WHITE | B_BLACK;
	ColorUnChanged = FarColorToReal(COL_DIALOGEDITUNCHANGED);
}

DWORD Edit::SetCodePage(UINT codepage)
{
	if (auto *editor = GetEditorOwner())
		return editor->SetCodePage(codepage) ? SETCP_NOERROR : SETCP_OTHERERROR;

	const UINT oldCodepage = GetCodePage();
	const DWORD result = TranscodeCodePage(oldCodepage, codepage);
	if (!(result & SETCP_OTHERERROR)) {
		if (auto *s = s_e2s.Get(this, true)) {
			s->codepage = codepage;
		}
	}

	return result;
}

DWORD Edit::TranscodeCodePage(UINT oldCodepage, UINT codepage)
{
	if (codepage == oldCodepage || Str.Size() == 0) {
		return SETCP_NOERROR;
	}
	auto was_specwidth = Flags.Check(FEDITLINE_HASSPECIALWIDTHCHARS);
	const auto out = Str.Transcode(oldCodepage, codepage);
	if (was_specwidth) {
		Flags.Clear(FEDITLINE_HASSPECIALWIDTHCHARS);
		CheckForSpecialWidthChars();
	}
	Changed();
	return out;
}

UINT Edit::GetCodePage()
{
	if (auto *editor = GetEditorOwner())
		return editor->GetCodePage();

	if (auto *s = s_e2s.Get(this))
		return s->codepage;

	return 0;
}

void Edit::DisplayObject()
{
	MyEcoLazy::Use my(fields);
	if (Flags.Check(FEDITLINE_DROPDOWNBOX)) {
		Flags.Clear(FEDITLINE_CLEARFLAG);	// при дроп-даун нам не нужно никакого unchanged text
		my->SelStart = 0;
		my->SelEnd = Str.Size();			// а также считаем что все выделено -
											// надо же отличаться от обычных Edit
	}

	// Вычисление нового положения курсора в строке с учётом Mask.
	int Value = (my->PrevCurPos > my->CurPos) ? -1 : 1;
	my->CurPos = GetNextCursorPos(my->CurPos, Value);
	FastShow();

	/*
		$ 26.07.2000 tran
		при DropDownBox курсор выключаем
		не знаю даже - попробовал но не очень красиво вышло
	*/
	if (Flags.Check(FEDITLINE_CURSORVISIBLE)) {
		if (Flags.Check(FEDITLINE_DROPDOWNBOX))
			::SetCursorType(0, 10);
		else {
			int CursorSize = -1;
			if (auto *s = s_e2s.Get(this))
				CursorSize = s->CursorSize;

			if (CursorSize == -1) {
				CursorSize = Flags.Check(FEDITLINE_OVERTYPE)
					? (Opt.CursorSize[2] ? Opt.CursorSize[2] : 99)
					: (Opt.CursorSize[0] ? Opt.CursorSize[0] : 10);
			}
			::SetCursorType(1, CursorSize);
		}

		MoveCursor(X1 + my->CursorPos - my->LeftPos, Y1);
	}
}

void Edit::SetCursorType(bool Visible, DWORD Size)
{
	Flags.Change(FEDITLINE_CURSORVISIBLE, Visible);
	if (auto *s = s_e2s.Get(this, true))
		s->CursorSize = Size;
	::SetCursorType(Visible, Size);
}

void Edit::GetCursorType(bool &Visible, DWORD &Size)
{
	Visible = Flags.Check(FEDITLINE_CURSORVISIBLE) != FALSE;
	if (auto *s = s_e2s.Get(this))
		Size = s->CursorSize;
	else
		Size = -1;
}

// Вычисление нового положения курсора в строке с учётом Mask.
int Edit::GetNextCursorPos(int Position, int Where)
{
	const wchar_t *Mask = GetInputMask();
	int Result = Position;

	if (Mask && *Mask && (Where == -1 || Where == 1)) {
		int PosChanged = FALSE;
		int MaskLen = StrLength(Mask);

		for (int i = Position; i < MaskLen && i >= 0; i+= Where) {
			if (CheckCharMask(Mask[i])) {
				Result = i;
				PosChanged = TRUE;
				break;
			}
		}

		if (!PosChanged) {
			for (int i = Position; i >= 0; i--) {
				if (CheckCharMask(Mask[i])) {
					Result = i;
					PosChanged = TRUE;
					break;
				}
			}
		}

		if (!PosChanged) {
			for (int i = Position; i < MaskLen; i++) {
				if (CheckCharMask(Mask[i])) {
					Result = i;
					break;
				}
			}
		}
	}

	return Result;
}

void Edit::FastShow()
{
	MyEcoLazy::Use my(fields);
	auto &OutStr = s_render_buffer;
	const wchar_t *Mask = GetInputMask();
	uint64_t Color, SelColor, ColorUnChanged;
	GetObjectColors(Color, SelColor, ColorUnChanged);
	int EditLength = ObjWidth();

	if (!Flags.Check(FEDITLINE_EDITBEYONDEND) && my->CurPos > Str.Size() && Str.Size() >= 0)
		my->CurPos = Str.Size();

	const int MaxLength = GetMaxLength();
	if (MaxLength != -1) {
		if (Str.Size() > MaxLength) {
			Str.Truncate(MaxLength);
		}

		if (my->CurPos > MaxLength - 1)
			my->CurPos = MaxLength > 0 ? (MaxLength - 1) : 0;
	}

	int CellCurPos = GetCellCurPos();

	/*
		$ 31.07.2001 KM
		! Для комбобокса сделаем отображение строки
		с первой позиции.
	*/
	int RealLeftPos = -1;
	if (!Flags.Check(FEDITLINE_DROPDOWNBOX)) {
		if (CellCurPos - my->LeftPos > EditLength - 1) {
			/*
				tricky left pos shifting to
				- avoid LeftPos pointing into middle of full-width char cells pair
				- ensure RealLeftPos really shifted in case string starts by some long character
			*/
			for (int ShiftBy = 1; ShiftBy <= std::max(GetTabSize(), 2); ++ShiftBy) {
				RealLeftPos = CellPosToReal(CellCurPos - EditLength + ShiftBy);
				int NewLeftPos = RealPosToCell(RealLeftPos);
				if (my->LeftPos != NewLeftPos) {
					my->LeftPos = NewLeftPos;
					break;
				}
			}
		}

		if (CellCurPos < my->LeftPos)
			my->LeftPos = CellCurPos;
	}

	if (RealLeftPos == -1)
		RealLeftPos = CellPosToReal(my->LeftPos);

	GotoXY(X1, Y1);
	int CellSelStart = (my->SelStart == -1) ? -1 : RealPosToCell(my->SelStart);
	int CellSelEnd = (my->SelEnd < 0) ? -1 : RealPosToCell(my->SelEnd);

	int iTrailingSpacesPos = Str.Size(); // for Visual show trailing spaces/tabs in dialog editlines

	/*
		$ 17.08.2000 KM
		Если есть маска, сделаем подготовку строки, то есть
		все "постоянные" символы в маске, не являющиеся шаблонными
		должны постоянно присутствовать в Str
	*/
	if (Mask && *Mask)
		RefreshStrByMask();
	// for Visual show trailing spaces/tabs in dialog editlines (not in masked)
	else if (Flags.Check(FEDITLINE_PARENT_SINGLELINE | FEDITLINE_PARENT_MULTILINE)) {
		for (iTrailingSpacesPos = Str.Size(); iTrailingSpacesPos > 0; iTrailingSpacesPos--)
			if (!std::iswblank(Str[iTrailingSpacesPos-1]))
				break;
	}

	my->CursorPos = CellCurPos;

	OutStr.clear();
	size_t OutStrCells = 0;
	bool joining = false;
	for (int i = RealLeftPos; i < Str.Size() && int(OutStrCells) < EditLength; ++i) {
		wchar_t wc = Str[i];
		auto showSymbols = (Flags.Check(FEDITLINE_SHOWWHITESPACE) && Flags.Check(FEDITLINE_EDITORMODE)) || i >= iTrailingSpacesPos;
		if (showSymbols) {
			switch(wc) {
				case 0x0020: //space
					wc = L'\xB7'; // ·
					break;
				case 0x00A0: //no-break space
					wc = L'\xB0'; // °
					break;
				case 0x00AD: //soft hyphen
					wc = L'\xAC'; // ¬
					break;
				case 0x2028: //line separator
					wc = L'\x2424'; // ␤
					break;
				case 0x2029: //paragraph separator
					wc = L'\xB6'; // ¶
					break;
				case 0x2000 ... 0x200A: //other spaces
				case 0x202F: case 0x205F:
				case 0x180E: case 0x3000:
					wc = L'\x2420'; // ␠
					break;
				case 0x200B ... 0x200D: //zero-width
				case 0x2060: case 0xFEFF:
					wc = L'\x2422'; // ␢
					break;
				case 0x200E ... 0x200F: //text direction marks and shaping controls
				case 0x202A ... 0x202E:
				case 0x2066 ... 0x206F:
					wc = L'\x2194'; // ↔
					break;
				case 0x000A: //line feed
					wc = L'\x240A'; // ␊
					break;
				case 0x000D: //carriage return
					wc = L'\x240D'; // ␍
					break;
			}
		} else {
			if (wc == L'\n')
				wc = L'\x21B5'; // ↵
			else if (wc == L'\r')
				wc = L'\x240D'; // ␍
		}

		if (wc == L'\t') {
			for (int j = 0, S = GetTabSize() - ((my->LeftPos + OutStrCells) % GetTabSize());
					j < S && int(OutStrCells) < EditLength; ++j, ++OutStrCells) {
				OutStr.emplace_back(
						(showSymbols && !j)
								? L'\x2192' // →
								: L' ');
			}
		} else {
			if (wc == CharClasses::ZERO_WIDTH_JOINER) {
				joining = true;
			} else if (Str.IsFullWidth(i)) {
				if (int(OutStrCells + 2) > EditLength) {
					OutStr.emplace_back(L' ');
					OutStrCells++;
					break;
				}
				if (!joining) OutStrCells+= 2;
				joining = false;
			} else if (!CharClasses::IsXxxfix(wc)) {
				if (!joining) OutStrCells++;
				joining = false;
			}

			OutStr.emplace_back(wc ? wc : L' ');
		}
	}

	if (Flags.Check(FEDITLINE_PASSWORDMODE)) {
		OutStr.resize(OutStrCells);
		std::fill(OutStr.begin(), OutStr.end(), L'*');
	}

	OutStr.emplace_back(0);
	SetColor(Color);

	if (CellSelStart == -1) {
		if (Flags.Check(FEDITLINE_CLEARFLAG)) {
			SetColor(ColorUnChanged);

			if (Mask && *Mask) {
				RemoveTrailingSpaces(OutStr.data());
				OutStr.resize(wcslen(OutStr.data()));
				OutStrCells = StrCellsCount(OutStr.data(), OutStr.size());
				OutStr.emplace_back(0);
			}

			FS << fmt::Cells() << fmt::LeftAlign() << OutStr.data();
			SetColor(Color);
			int BlankLength = EditLength - (int)OutStrCells;

			if (BlankLength > 0) {
				FS << fmt::Cells() << fmt::Expand(BlankLength) << L"";
			}
		} else {
			FS << fmt::LeftAlign() << fmt::Cells() << fmt::Size(EditLength) << OutStr.data();
		}
	} else {
		if ((CellSelStart-= my->LeftPos) < 0)
			CellSelStart = 0;

		int AllString = (CellSelEnd == -1);

		if (AllString)
			CellSelEnd = EditLength;
		else if ((CellSelEnd-= my->LeftPos) < 0)
			CellSelEnd = 0;

		for (; int(OutStrCells) < EditLength; ++OutStrCells) {
			OutStr.emplace(OutStr.begin() + OutStr.size() - 1, L' ');
		}

		/*
			$ 24.08.2000 SVS
			! У DropDowList`а выделение по полной программе - на всю видимую длину
			ДАЖЕ ЕСЛИ ПУСТАЯ СТРОКА
		*/
		if (CellSelStart >= EditLength	/*|| !AllString && CellSelStart>=Str.Size()*/
				|| CellSelEnd < CellSelStart) {
			if (Flags.Check(FEDITLINE_DROPDOWNBOX)) {
				SetColor(SelColor);
				FS << fmt::Cells() << fmt::Expand(X2 - X1 + 1) << OutStr.data();
			} else
				Text(OutStr.data());
		} else {
			FS << fmt::Cells() << fmt::Truncate(CellSelStart) << OutStr.data();
			SetColor(SelColor);

			if (!Flags.Check(FEDITLINE_DROPDOWNBOX)) {
				FS << fmt::Cells() << fmt::Skip(CellSelStart) << fmt::Truncate(CellSelEnd - CellSelStart)
					<< OutStr.data();

				if (CellSelEnd < EditLength) {
					// SetColor(Flags.Check(FEDITLINE_CLEARFLAG) ? SelColor:Color);
					SetColor(Color);
					FS << fmt::Cells() << fmt::Skip(CellSelEnd) << OutStr.data();
				}
			} else {
				FS << fmt::Cells() << fmt::Expand(X2 - X1 + 1) << OutStr.data();
			}
		}
	}

	/*
		$ 26.07.2000 tran
		при дроп-даун цвета нам не нужны
	*/
	if (!Flags.Check(FEDITLINE_DROPDOWNBOX))
		ApplyColor();
}

int Edit::RecurseProcessKey(FarKey Key)
{
	Recurse++;
	int RetCode = ProcessKey(Key);
	Recurse--;
	return (RetCode);
}

// Функция вставки всякой хреновени - от шорткатов до имен файлов
int Edit::ProcessInsPath(FarKey Key, int PrevSelStart, int PrevSelEnd)
{
	MyEcoLazy::Use my(fields);
	int RetCode = FALSE;
	FARString strPathName;

	if (Key >= KEY_RCTRL0 && Key <= KEY_RCTRL9)		// шорткаты?
	{
		FARString strPluginModule, strPluginFile, strPluginData;

		if (Bookmarks().Get(Key - KEY_RCTRL0, &strPathName, &strPluginModule, &strPluginFile, &strPluginData))
			RetCode = TRUE;
	} else		// Пути/имена?
	{
		RetCode = _MakePath1(Key, strPathName, L"", 0); // 0 - always not escaping path names
	}

	// Если что-нить получилось, именно его и вставим (PathName)
	if (RetCode) {
		if (Flags.Check(FEDITLINE_CLEARFLAG)) {
			my->LeftPos = 0;
			SetString(L"");
		}

		if (PrevSelStart != -1) {
			my->SelStart = PrevSelStart;
			my->SelEnd = PrevSelEnd;
		}

		if (!Flags.Check(FEDITLINE_PERSISTENTBLOCKS))
			DeleteBlock();

		InsertString(strPathName);
		Flags.Clear(FEDITLINE_CLEARFLAG);
	}

	return RetCode;
}

int64_t Edit::VMProcess(MacroOpcode OpCode, void *vParam, int64_t iParam)
{
	MyEcoLazy::Use my(fields);
	switch (OpCode) {
		case MCODE_C_EMPTY:
			return (int64_t)!GetLength();
		case MCODE_C_SELECTED:
			return (int64_t)(my->SelStart != -1 && my->SelStart < my->SelEnd);
		case MCODE_C_EOF:
			return (int64_t)(my->CurPos >= Str.Size());
		case MCODE_C_BOF:
			return (int64_t)!my->CurPos;
		case MCODE_V_ITEMCOUNT:
			return (int64_t)Str.Size();
		case MCODE_V_CURPOS:
			return (int64_t)(my->CurPos + 1);
		case MCODE_F_EDITOR_SEL: {
			int Action = (int)((INT_PTR)vParam);

			switch (Action) {
				case 0:		// Get Param
				{
					switch (iParam) {
						case 0:		// return FirstLine
						case 2:		// return LastLine
						case 4:		// return block type (0=nothing 1=stream, 2=column)
							return IsSelection() ? 1 : 0;
						case 1:		// return FirstPos
							return IsSelection() ? my->SelStart + 1 : 0;
						case 3:		// return LastPos
							return IsSelection() ? my->SelEnd : 0;
					}

					break;
				}
				case 1:		// Set Pos
				{
					if (IsSelection()) {
						switch (iParam) {
							case 0:		// begin block (FirstLine & FirstPos)
							case 1:		// end block (LastLine & LastPos)
							{
								SetCellCurPos(iParam ? my->SelEnd : my->SelStart);
								Show();
								return 1;
							}
						}
					}

					break;
				}
				case 2:		// Set Stream Selection Edge
				case 3:		// Set Column Selection Edge
				{
					switch (iParam) {
						case 0:		// selection start
						{
							my->MSelStart = GetCurPos();
							return 1;
						}
						case 1:		// selection finish
						{
							if (my->MSelStart != -1) {
								if (my->MSelStart != GetCurPos())
									Select(my->MSelStart, GetCurPos());
								else
									Select(-1, 0);

								Show();
								my->MSelStart = -1;
								return 1;
							}

							return 0;
						}
					}

					break;
				}
				case 4:		// UnMark sel block
				{
					Select(-1, 0);
					my->MSelStart = -1;
					Show();
					return 1;
				}
			}

			break;
		}
	}

	return 0;
}

int Edit::CalcRTrimmedStrSize() const
{
	int TrimmedStrSize = Str.Size();
	while (TrimmedStrSize > 0 && (IsSpace(Str[TrimmedStrSize - 1]) || IsEol(Str[TrimmedStrSize - 1]))) {
		--TrimmedStrSize;
	}
	return TrimmedStrSize;
}

int Edit::CalcPosFwdTo(int Pos, int LimitPos) const
{
	bool joining = false;
	if (LimitPos != -1) {
		if (Pos < LimitPos) {
			++Pos;
			// Skip combining marks and ZWJ sequences
			for ( ; Pos < LimitPos && Pos < Str.Size(); ++Pos) {
				if (Str[Pos] == CharClasses::ZERO_WIDTH_JOINER) {
					joining = true;
				} else if (Str.IsXxxfix(Pos)) {
					continue;
				} else if (joining) {
					joining = false;
				} else {
					break;
				}
			}
		}
	} else {
		++Pos;
		for ( ; Pos < Str.Size(); ++Pos) {
			if (Str[Pos] == CharClasses::ZERO_WIDTH_JOINER) {
				joining = true;
			} else if (Str.IsXxxfix(Pos)) {
				continue;
			} else if (joining) {
				joining = false;
			} else {
				break;
			}
		}
	}

	return Pos;
}

int Edit::CalcPosBwdTo(int Pos) const
{
	if (Pos <= 0)
		return 0;

	--Pos;
	for ( ; Pos > 0 && Pos < Str.Size(); --Pos) {
		if (Str[Pos] == CharClasses::ZERO_WIDTH_JOINER) {
			continue;
		} else if (Str.IsXxxfix(Pos)) {
			continue;
		} else if (Str[Pos - 1] == CharClasses::ZERO_WIDTH_JOINER) {
			continue;
		} else {
			break;
		}
	}

	return Pos;
}

int Edit::CalcPosFwd(int LimitPos) const
{
	MyEcoLazy::See my(fields);
	return CalcPosFwdTo(my->CurPos, LimitPos);
}

int Edit::CalcPosBwd() const
{
	MyEcoLazy::See my(fields);
	return CalcPosBwdTo(my->CurPos);
}

int Edit::ProcessKey(FarKey Key)
{
	MyEcoLazy::Use my(fields);
	const wchar_t *Mask = GetInputMask();
	TranslateInsertKey(Key);

	switch (Key) {
		case KEY_CTRLC:
			Key = KEY_CTRLINS;
			break;
		case KEY_CTRLV:
			Key = KEY_SHIFTINS;
			break;
		case KEY_CTRLX:
			Key = KEY_SHIFTDEL;
			break;
	}

	int PrevSelStart = -1, PrevSelEnd = 0;

	if (!Flags.Check(FEDITLINE_DROPDOWNBOX) && Key == KEY_CTRLL) {
		Flags.Swap(FEDITLINE_READONLY);
	}

	/*
		$ 26.07.2000 SVS
		Bugs #??
		В строках ввода при выделенном блоке нажимаем BS и вместо
		ожидаемого удаления блока (как в редакторе) получаем:
		- символ перед курсором удален
		- выделение блока снято
	*/
	if ((((Key == KEY_BS || Key == KEY_DEL || Key == KEY_NUMDEL) && Flags.Check(FEDITLINE_DELREMOVESBLOCKS))
				|| Key == KEY_CTRLD)
			&& !Flags.Check(FEDITLINE_EDITORMODE) && my->SelStart != -1 && my->SelStart < my->SelEnd) {
		DeleteBlock();
		Show();
		return TRUE;
	}

	int _Macro_IsExecuting = CtrlObject->Macro.IsExecuting();

	// $ 04.07.2000 IG - добавлена проврерка на запуск макроса (00025.edit.cpp.txt)
	if (!ShiftPressed && (!_Macro_IsExecuting || (IsNavKey(Key) && _Macro_IsExecuting)) && !IsShiftKey(Key)
			&& !Recurse && Key != KEY_SHIFT && Key != KEY_CTRL && Key != KEY_ALT && Key != KEY_RCTRL
			&& Key != KEY_RALT && Key != KEY_NONE && Key != KEY_INS && Key != KEY_KILLFOCUS
			&& Key != KEY_GOTFOCUS
			&& ((Key & (~KEY_CTRLMASK)) != KEY_LWIN && (Key & (~KEY_CTRLMASK)) != KEY_RWIN
					&& (Key & (~KEY_CTRLMASK)) != KEY_APPS)) {
		Flags.Clear(FEDITLINE_MARKINGBLOCK);	// хмм... а это здесь должно быть?

		if (!Flags.Check(FEDITLINE_PERSISTENTBLOCKS) && !(Key == KEY_CTRLINS || Key == KEY_CTRLNUMPAD0)
				&& !(Key == KEY_SHIFTDEL || Key == KEY_SHIFTNUMDEL || Key == KEY_SHIFTDECIMAL) && !Flags.Check(FEDITLINE_EDITORMODE)
				&& Key != KEY_CTRLQ && !(Key == KEY_SHIFTINS || Key == KEY_SHIFTNUMPAD0))		// Key != KEY_SHIFTINS) //??
		{
			/*
				$ 12.11.2002 DJ
				зачем рисоваться, если ничего не изменилось?
			*/
			if (my->SelStart != -1 || my->SelEnd) {
				PrevSelStart = my->SelStart;
				PrevSelEnd = my->SelEnd;
				Select(-1, 0);
				Show();
			}
		}
	}

	/*
		$ 11.09.2000 SVS
		если Opt.DlgEULBsClear = 1, то BS в диалогах для UnChanged строки
		удаляет такую строку также, как и Del
	*/
	if (((Opt.Dialogs.EULBsClear && Key == KEY_BS) || Key == KEY_DEL || Key == KEY_NUMDEL)
			&& Flags.Check(FEDITLINE_CLEARFLAG) && my->CurPos >= Str.Size())
		Key = KEY_CTRLY;

	/*
		$ 15.09.2000 SVS
		Bug - Выделяем кусочек строки -> Shift-Del удяляет всю строку
		Так должно быть только для UnChanged состояния
	*/
	if ((Key == KEY_SHIFTDEL || Key == KEY_SHIFTNUMDEL || Key == KEY_SHIFTDECIMAL)
			&& Flags.Check(FEDITLINE_CLEARFLAG) && my->CurPos >= Str.Size() && my->SelStart == -1) {
		my->SelStart = 0;
		my->SelEnd = Str.Size();
	}

	if (Flags.Check(FEDITLINE_CLEARFLAG)
			&& ((Key <= 0xFFFF && Key != KEY_BS) || Key == KEY_CTRLBRACKET || Key == KEY_CTRLBACKBRACKET
					|| Key == KEY_CTRLSHIFTBRACKET || Key == KEY_CTRLSHIFTBACKBRACKET || Key == KEY_SHIFTENTER
					|| Key == KEY_SHIFTNUMENTER)) {
		my->LeftPos = 0;
		SetString(L"");
		Show();
	}

	// Здесь - вызов функции вставки путей/файлов
	if (ProcessInsPath(Key, PrevSelStart, PrevSelEnd)) {
		Show();
		return TRUE;
	}

	if (Flags.Check(FEDITLINE_CLEARFLAG)
			&& Key != KEY_NONE && Key != KEY_IDLE && Key != KEY_SHIFTINS && Key != KEY_SHIFTNUMPAD0
			&& Key != KEY_CTRLINS
			&& ((unsigned int)Key < KEY_F1 || (unsigned int)Key > KEY_F12) && Key != KEY_ALT && Key != KEY_SHIFT
			&& Key != KEY_CTRL && Key != KEY_RALT && Key != KEY_RCTRL && (Key < KEY_ALT_BASE || Key > KEY_ALT_BASE + 0xFFFF)
			&& !( Key & (KEY_ALT | KEY_RALT) )
			&&		// ???? 256 ???
			!(((unsigned int)Key >= KEY_MACRO_BASE && (unsigned int)Key <= KEY_MACRO_ENDBASE)
					|| ((unsigned int)Key >= KEY_OP_BASE && (unsigned int)Key <= KEY_OP_ENDBASE))
			&& Key != KEY_CTRLQ) {
		Flags.Clear(FEDITLINE_CLEARFLAG);
		Show();
	}

	switch (Key) {
		case KEY_CTRLA:
			Select(0, Str.Size());
			Show();
			return FALSE;

		case KEY_CTRLU:
			SetClearFlag(0);
			Select(-1, 0);
			Show();
			return FALSE;

		case KEY_SHIFTLEFT:
		case KEY_SHIFTNUMPAD4: {
			if (my->CurPos > 0) {
				RecurseProcessKey(KEY_LEFT);

				if (!Flags.Check(FEDITLINE_MARKINGBLOCK)) {
					Select(-1, 0);
					Flags.Set(FEDITLINE_MARKINGBLOCK);
				}

				if (my->SelStart != -1 && my->SelStart <= my->CurPos)
					Select(my->SelStart, my->CurPos);
				else {
					int EndPos = CalcPosFwd((Mask && *Mask) ? CalcRTrimmedStrSize() : -1);
					int NewStartPos = my->CurPos;

					if (EndPos > Str.Size())
						EndPos = Str.Size();

					if (NewStartPos > Str.Size())
						NewStartPos = Str.Size();

					AddSelect(NewStartPos, EndPos);
				}

				Show();
			}

			return TRUE;
		}
		case KEY_SHIFTRIGHT:
		case KEY_SHIFTNUMPAD6: {
			if (!Flags.Check(FEDITLINE_MARKINGBLOCK)) {
				Select(-1, 0);
				Flags.Set(FEDITLINE_MARKINGBLOCK);
			}

			if ((my->SelStart != -1 && my->SelEnd == -1) || my->SelEnd > my->CurPos) {
				if (CalcPosFwd() == my->SelEnd)
					Select(-1, 0);
				else
					Select(CalcPosFwd(), my->SelEnd);
			} else
				AddSelect(my->CurPos, CalcPosFwd());

			RecurseProcessKey(KEY_RIGHT);
			return TRUE;
		}
		case KEY_CTRLSHIFTLEFT:
		case KEY_CTRLSHIFTNUMPAD4: {
			if (my->CurPos > Str.Size()) {
				my->PrevCurPos = my->CurPos;
				my->CurPos = Str.Size();
			}

			if (my->CurPos > 0)
				RecurseProcessKey(KEY_SHIFTLEFT);

			while (my->CurPos > 0
					&& !(!IsWordDiv(WordDiv(), Str[my->CurPos]) && IsWordDiv(WordDiv(), Str[my->CurPos - 1])
							&& !IsSpace(Str[my->CurPos]))) {
				if (!IsSpace(Str[my->CurPos])
						&& (IsSpace(Str[my->CurPos - 1]) || IsWordDiv(WordDiv(), Str[my->CurPos - 1])))
					break;

				RecurseProcessKey(KEY_SHIFTLEFT);
			}

			Show();
			return TRUE;
		}
		case KEY_CTRLSHIFTRIGHT:
		case KEY_CTRLSHIFTNUMPAD6: {
			if (my->CurPos >= Str.Size())
				return FALSE;

			const int MaxLength = GetMaxLength();
			RecurseProcessKey(KEY_SHIFTRIGHT);

			while (my->CurPos < Str.Size() && !(IsWordDiv(WordDiv(), Str[my->CurPos]) && !IsWordDiv(WordDiv(), Str[my->CurPos - 1]))) {
				if (!IsSpace(Str[my->CurPos]) && (IsSpace(Str[my->CurPos - 1]) || IsWordDiv(WordDiv(), Str[my->CurPos - 1])))
					break;

				RecurseProcessKey(KEY_SHIFTRIGHT);

				if (MaxLength != -1 && my->CurPos == MaxLength - 1)
					break;
			}

			Show();
			return TRUE;
		}
		case KEY_SHIFTHOME:
		case KEY_SHIFTNUMPAD7: {
			LockThinObject l(*this);
			while (my->CurPos > 0)
				RecurseProcessKey(KEY_SHIFTLEFT);
			l.Unlock();
			Show();
			return TRUE;
		}
		case KEY_SHIFTEND:
		case KEY_SHIFTNUMPAD1: {
			LockThinObject l(*this);
			int Len = (Mask && *Mask) ? CalcRTrimmedStrSize() : Str.Size();

			int LastCurPos = my->CurPos;

			while (my->CurPos < Len /*Str.Size()*/) {
				RecurseProcessKey(KEY_SHIFTRIGHT);

				if (LastCurPos == my->CurPos)
					break;

				LastCurPos = my->CurPos;
			}

			l.Unlock();
			Show();
			return TRUE;
		}
		case KEY_BS: {
			if (my->CurPos <= 0)
				return FALSE;

			my->PrevCurPos = my->CurPos;
			my->CurPos = CalcPosBwd();
			if (Mask && *Mask)
				my->CurPos = GetNextCursorPos(my->CurPos, -1);

			while (my->LeftPos > 0 && RealPosToCell(my->CurPos) <= my->LeftPos) {
				my->LeftPos-= 15;
				if (my->LeftPos > 0)
					my->LeftPos = RealPosToCell(CellPosToReal(my->LeftPos));
				else
					my->LeftPos = 0;
			}

			if (!RecurseProcessKey(KEY_DEL))
				Show();

			return TRUE;
		}
		case KEY_CTRLSHIFTBS: {
			PauseEditListener pel(*this);

			// BUGBUG
			for (int i = my->CurPos; i >= 0; i--) {
				RecurseProcessKey(KEY_BS);
			}
			pel.Resume();
			Changed(true);
			Show();
			return TRUE;
		}
		case KEY_CTRLBS: {
			if (my->CurPos > Str.Size()) {
				my->PrevCurPos = my->CurPos;
				my->CurPos = Str.Size();
			}

			LockThinObject l(*this);
			PauseEditListener pel(*this);

			// BUGBUG
			for (;;) {
				int StopDelete = FALSE;

				if (my->CurPos > 1 && IsSpace(Str[my->CurPos - 1]) != IsSpace(Str[my->CurPos - 2]))
					StopDelete = TRUE;

				RecurseProcessKey(KEY_BS);

				if (!my->CurPos || StopDelete)
					break;

				if (IsWordDiv(WordDiv(), Str[my->CurPos - 1]))
					break;
			}

			l.Unlock();
			pel.Resume();
			Changed(true);
			Show();
			return TRUE;
		}
		case KEY_CTRLQ: {
			LockThinObject l(*this);

			if (!Flags.Check(FEDITLINE_PERSISTENTBLOCKS) && (my->SelStart != -1 || Flags.Check(FEDITLINE_CLEARFLAG)))
				RecurseProcessKey(KEY_DEL);

			ProcessCtrlQ();
			l.Unlock();
			Show();
			return TRUE;
		}
		case KEY_OP_SELWORD: {
			int OldCurPos = my->CurPos;
			PrevSelStart = my->SelStart;
			PrevSelEnd = my->SelEnd;
#if defined(MOUSEKEY)

			if (my->CurPos >= my->SelStart && my->CurPos <= my->SelEnd) {	// выделяем ВСЮ строку при повторном двойном клике
				Select(0, Str.Size());
			} else
#endif
			{
				int SStart, SEnd;

				if (CalcWordFromString(Str.CPtr(), my->CurPos, &SStart, &SEnd, WordDiv()))
					Select(SStart, SEnd + (SEnd < Str.Size() ? 1 : 0));
			}

			my->CurPos = OldCurPos;		// возвращаем обратно
			Show();
			return TRUE;
		}
		case KEY_OP_PLAINTEXT: {
			if (!Flags.Check(FEDITLINE_PERSISTENTBLOCKS)) {
				if (my->SelStart != -1 || Flags.Check(FEDITLINE_CLEARFLAG))
					RecurseProcessKey(KEY_DEL);
			}

			FARString strPastedText;
			if (!GPastedText.IsEmpty()) {
				strPastedText = GPastedText;
				GPastedText.Clear();
			} else {
				strPastedText = eStackAsString();
			}

			// For single-line edit controls, replace EOL sequences with spaces.
			ReplaceStrings(strPastedText, L"\r\n", L" ");
			ReplaceStrings(strPastedText, L"\r", L" ");
			ReplaceStrings(strPastedText, L"\n", L" ");

			InsertString(strPastedText);

			Show();
			return TRUE;
		}
		case KEY_CTRLT:
		case KEY_CTRLDEL:
		case KEY_CTRLNUMDEL:
		case KEY_CTRLDECIMAL: {
			if (my->CurPos >= Str.Size())
				return FALSE;

			LockThinObject l(*this);
			PauseEditListener pel(*this);
			if (Mask && *Mask) {
				int MaskLen = StrLength(Mask);
				int ptr = my->CurPos;

				while (ptr < MaskLen) {
					ptr++;

					if (!CheckCharMask(Mask[ptr]) || (IsSpace(Str[ptr]) && !IsSpace(Str[ptr + 1])) || (IsWordDiv(WordDiv(), Str[ptr])))
						break;
				}

				// BUGBUG
				for (int i = 0; i < ptr - my->CurPos; i++)
					RecurseProcessKey(KEY_DEL);
			} else {
				for (;;) {
					int StopDelete = FALSE;

					if (my->CurPos < Str.Size() - 1 && IsSpace(Str[my->CurPos]) && !IsSpace(Str[my->CurPos + 1]))
						StopDelete = TRUE;

					RecurseProcessKey(KEY_DEL);

					if (my->CurPos >= Str.Size() || StopDelete)
						break;

					if (IsWordDiv(WordDiv(), Str[my->CurPos]))
						break;
				}
			}

			l.Unlock();
			pel.Resume();
			Changed(true);
			Show();
			return TRUE;
		}
		case KEY_CTRLY: {
			if (Flags.Check(FEDITLINE_READONLY | FEDITLINE_DROPDOWNBOX))
				return (TRUE);

			my->PrevCurPos = my->CurPos;
			my->LeftPos = my->CurPos = 0;
			Str.Truncate();
			Select(-1, 0);
			Changed();
			Show();
			return TRUE;
		}
		case KEY_CTRLK: {
			if (Flags.Check(FEDITLINE_READONLY | FEDITLINE_DROPDOWNBOX))
				return (TRUE);

			if (my->CurPos >= Str.Size())
				return FALSE;

			if (!Flags.Check(FEDITLINE_EDITBEYONDEND)) {
				if (my->CurPos < my->SelEnd)
					my->SelEnd = my->CurPos;

				if (my->SelEnd < my->SelStart && my->SelEnd != -1) {
					my->SelEnd = 0;
					my->SelStart = -1;
				}
			}

			Str.Truncate(my->CurPos);
			Changed();
			Show();
			return TRUE;
		}
		case KEY_HOME:
		case KEY_NUMPAD7:
		case KEY_CTRLHOME:
		case KEY_CTRLNUMPAD7: {
			my->PrevCurPos = my->CurPos;
			my->CurPos = 0;
			Show();
			return TRUE;
		}
		case KEY_END:
		case KEY_NUMPAD1:
		case KEY_CTRLEND:
		case KEY_CTRLNUMPAD1:
		case KEY_CTRLSHIFTEND:
		case KEY_CTRLSHIFTNUMPAD1: {
			my->PrevCurPos = my->CurPos;
			my->CurPos = (Mask && *Mask) ? CalcRTrimmedStrSize() : Str.Size();
			Show();
			return TRUE;
		}
		case KEY_LEFT:
		case KEY_NUMPAD4:
		case KEY_MSWHEEL_LEFT:
		case KEY_CTRLS: {
			if (my->CurPos > 0) {
				my->PrevCurPos = my->CurPos;
				my->CurPos = CalcPosBwd();
				Show();
			}

			return TRUE;
		}
		case KEY_RIGHT:
		case KEY_NUMPAD6:
		case KEY_MSWHEEL_RIGHT:
		case KEY_CTRLD: {
			my->PrevCurPos = my->CurPos;
			my->CurPos = CalcPosFwd((Mask && *Mask) ? CalcRTrimmedStrSize() : -1);
			Show();
			return TRUE;
		}
		case KEY_INS:
		case KEY_NUMPAD0: {
			Flags.Swap(FEDITLINE_OVERTYPE);
			Show();
			return TRUE;
		}
		case KEY_NUMDEL:
		case KEY_DEL: {
			if (Flags.Check(FEDITLINE_READONLY | FEDITLINE_DROPDOWNBOX))
				return (TRUE);

			if (my->CurPos >= Str.Size())
				return FALSE;

			if (my->SelStart != -1) {
				if (my->SelEnd != -1 && my->CurPos < my->SelEnd)
					my->SelEnd--;

				if (my->CurPos < my->SelStart)
					my->SelStart--;

				if (my->SelEnd != -1 && my->SelEnd <= my->SelStart) {
					my->SelStart = -1;
					my->SelEnd = 0;
				}
			}

			if (Mask && *Mask) {
				Str[my->CurPos] = L' ';
			} else {
				auto NextPos = CalcPosFwd();
				if (NextPos > my->CurPos) {
					Str.Remove(my->CurPos, NextPos - my->CurPos);
				}
			}

			if (GetWordWrap())
			{
				RecalculateWordWrap(ObjWidth(), GetTabSize());
			}
			Changed(true);
			Show();
			return TRUE;
		}
		case KEY_CTRLLEFT:
		case KEY_CTRLNUMPAD4: {
			my->PrevCurPos = my->CurPos;

			if (my->CurPos > Str.Size())
				my->CurPos = Str.Size();

			my->CurPos = CalcPosBwd();

			while (my->CurPos > 0
					&& !(!IsWordDiv(WordDiv(), Str[my->CurPos]) && IsWordDiv(WordDiv(), Str[my->CurPos - 1]) && !IsSpace(Str[my->CurPos]))) {
				if (!IsSpace(Str[my->CurPos]) && IsSpace(Str[my->CurPos - 1]))
					break;

				my->CurPos--;
			}

			Show();
			return TRUE;
		}
		case KEY_CTRLRIGHT:
		case KEY_CTRLNUMPAD6: {
			if (my->CurPos >= Str.Size())
				return FALSE;

			my->PrevCurPos = my->CurPos;
			int Len;

			if (Mask && *Mask) {
				Len = CalcRTrimmedStrSize();
				my->CurPos = CalcPosFwd(Len);
			} else {
				Len = Str.Size();
				my->CurPos = CalcPosFwd();
			}

			while (my->CurPos < Len && !(IsWordDiv(WordDiv(), Str[my->CurPos]) && !IsWordDiv(WordDiv(), Str[my->CurPos - 1]))) {
				if (!IsSpace(Str[my->CurPos]) && IsSpace(Str[my->CurPos - 1]))
					break;

				my->CurPos++;
			}

			Show();
			return TRUE;
		}
		case KEY_SHIFTNUMDEL:
		case KEY_SHIFTDECIMAL:
		case KEY_SHIFTDEL: {
			if (my->SelStart == -1 || my->SelStart >= my->SelEnd)
				return FALSE;

			RecurseProcessKey(KEY_CTRLINS);
			DeleteBlock();
			Show();
			return TRUE;
		}
		case KEY_CTRLINS:
		case KEY_CTRLNUMPAD0: {
			if (!Flags.Check(FEDITLINE_PASSWORDMODE)) {
				if (my->SelStart == -1 || my->SelStart >= my->SelEnd) {
					if (Mask && *Mask) {
						std::wstring TrimmedStr(Str.CPtr(), CalcRTrimmedStrSize());
						CopyToClipboard(TrimmedStr.c_str());
					} else {
						CopyToClipboard(Str.CPtr());
					}
				} else if (my->SelEnd <= Str.Size()) // TODO: если в начало условия добавить "Str.Size() &&", то пропадет баг "Ctrl-Ins в пустой строке очищает клипборд"
				{
					int Ch = Str[my->SelEnd];
					Str[my->SelEnd] = 0;
					CopyToClipboard(Str.CPtr() + my->SelStart);
					Str[my->SelEnd] = Ch;
				}
			}

			return TRUE;
		}
		case KEY_SHIFTINS:
		case KEY_SHIFTNUMPAD0: {
			wchar_t *ClipText = PasteFromClipboardEx(GetMaxLength());

			if (!ClipText)
				return TRUE;

			if (!Flags.Check(FEDITLINE_PERSISTENTBLOCKS)) {
				PauseEditListener pel(*this);
				DeleteBlock();
			}

			for (int i = StrLength(Str.CPtr()) - 1; i >= 0 && IsEol(Str[i]); i--)
				Str[i] = 0;

			for (int i = 0; ClipText[i]; i++) {
				if (IsEol(ClipText[i])) {
					if (IsEol(ClipText[i + 1]))
						wmemmove(&ClipText[i], &ClipText[i + 1], StrLength(&ClipText[i + 1]) + 1);

					if (!ClipText[i + 1])
						ClipText[i] = 0;
					else
						ClipText[i] = L' ';
				}
			}

			if (Flags.Check(FEDITLINE_CLEARFLAG)) {
				my->LeftPos = 0;
				Flags.Clear(FEDITLINE_CLEARFLAG);
				SetString(ClipText);
			} else {
				InsertString(ClipText);
			}

			if (ClipText)
				free(ClipText);

			Show();
			return TRUE;
		}
		case KEY_SHIFTTAB: {
			my->PrevCurPos = my->CurPos;
			my->CursorPos-= (my->CursorPos - 1) % GetTabSize() + 1;

			if (my->CursorPos < 0)
				my->CursorPos = 0;	// CursorPos=0,TabSize=1 case

			SetCellCurPos(my->CursorPos);
			Show();
			return TRUE;
		}
		default: {
			//			_D(SysLog(L"Key=0x%08X",Key));
			if (Key == KEY_ENTER || !IS_KEY_NORMAL(Key))	// KEY_NUMENTER,KEY_IDLE,KEY_NONE covered by !IS_KEY_NORMAL
				break;

			if (!Flags.Check(FEDITLINE_PERSISTENTBLOCKS)) {
				if (PrevSelStart != -1) {
					my->SelStart = PrevSelStart;
					my->SelEnd = PrevSelEnd;
				}
				PauseEditListener pel(*this);
				DeleteBlock();
			}

			if (InsertKey(Key))
				Show();

			return TRUE;
		}
	}

	return FALSE;
}

// обработка Ctrl-Q
int Edit::ProcessCtrlQ()
{
	INPUT_RECORD rec;
	DWORD Key;

	for (;;) {
		Key = GetInputRecord(&rec);

		if (Key != KEY_NONE && Key != KEY_IDLE && rec.Event.KeyEvent.uChar.AsciiChar)
			break;

		if (Key == KEY_CONSOLE_BUFFER_RESIZE) {
			//			int Dis=EditOutDisabled;
			//			EditOutDisabled=0;
			Show();
			//			EditOutDisabled=Dis;
		}
	}

	/*
	EditOutDisabled++;
	if (!Flags.Check(FEDITLINE_PERSISTENTBLOCKS))
	{
		DeleteBlock();
	}
	else
		Flags.Clear(FEDITLINE_CLEARFLAG);
	EditOutDisabled--;
	*/
	CHAR ch = rec.Event.KeyEvent.uChar.UnicodeChar;
	if( rec.Event.KeyEvent.dwControlKeyState & (LEFT_CTRL_PRESSED |RIGHT_CTRL_PRESSED ) && ch >= 'A' && ch <= 'Z'  )
		ch -= ('A' - 1); // convert to binary
	return InsertKey(ch);
}

int Edit::ProcessInsPlainText(const wchar_t *str)
{
	if (*str) {
		InsertString(str);
		return TRUE;
	}

	return FALSE;
}

int Edit::InsertKey(FarKey Key)
{
	const wchar_t *Mask = GetInputMask();
	if (Flags.Check(FEDITLINE_READONLY | FEDITLINE_DROPDOWNBOX))
		return (TRUE);

	MyEcoLazy::Use my(fields);

	if (Key == KEY_TAB && Flags.Check(FEDITLINE_OVERTYPE)) {
		my->PrevCurPos = my->CurPos;
		my->CursorPos+= GetTabSize() - (my->CursorPos % GetTabSize());
		SetCellCurPos(my->CursorPos);
		return TRUE;
	}

	if (Mask && *Mask) {
		int MaskLen = StrLength(Mask);

		if (my->CurPos < MaskLen) {
			if (KeyMatchedMask(Key)) {
				if (!Flags.Check(FEDITLINE_OVERTYPE)) {
					int i = MaskLen - 1;

					while (i > my->CurPos && !CheckCharMask(Mask[i]))
						i--;

					for (int j = i; i > my->CurPos; i--) {
						if (CheckCharMask(Mask[i])) {
							while (!CheckCharMask(Mask[j - 1])) {
								if (j <= my->CurPos)
									break;

								j--;
							}

							Str[i] = Str[j - 1];
							j--;
						}
					}
				}

				my->PrevCurPos = my->CurPos;
				Str[my->CurPos++] = Key;
				Changed();
			} else {
				// Здесь вариант для "ввели символ из маски", например для SetAttr - ввесли '.'
				;	// char *Ptr=strchr(Mask+CurPos,Key);
			}
		} else if (my->CurPos < Str.Size()) {
			my->PrevCurPos = my->CurPos;
			Str[my->CurPos++] = Key;
			Changed();
		}
	} else {
		const int MaxLength = GetMaxLength();
		if (MaxLength == -1 || Str.Size() < MaxLength) {
			if (my->CurPos > Str.Size() && !Str.Expand(my->CurPos, ' ')) {
				fprintf(stderr, "Edit::InsertKey: failed to expand to %d\n", my->CurPos);
				return FALSE;
			}

			wchar_t ch = static_cast<wchar_t>(Key);
			my->PrevCurPos = my->CurPos;
			if (Key == KEY_TAB && (GetConvertTabs() == EXPAND_NEWTABS || GetConvertTabs() == EXPAND_ALLTABS)) {
				auto S = GetTabSize() - (my->CurPos % GetTabSize());
				if (!Str.Insert(my->CurPos, L' ', S)) {
					fprintf(stderr, "Edit::InsertKey: failed to insert %d tab spaces at %d\n", S, my->CurPos);
					return FALSE;
				}
				my->CurPos+= S;
				if (Flags.Check(FEDITLINE_OVERTYPE)) {
					Str.Remove(my->CurPos, 1);
				}

			} else if (Flags.Check(FEDITLINE_OVERTYPE) && my->CurPos < Str.Size()) {
				Str[my->CurPos] = ch;
				my->CurPos++;
			} else if (Str.Insert(my->CurPos, ch)) {
				my->CurPos++;
			} else {
				fprintf(stderr, "Edit::InsertKey: failed to insert char at %d\n", my->CurPos);
				return FALSE;
			}

			if (!Flags.Check(FEDITLINE_OVERTYPE) && my->SelStart != -1) {
				if (my->SelEnd != -1 && my->PrevCurPos < my->SelEnd)
					my->SelEnd+= my->CurPos - my->PrevCurPos;

				if (my->PrevCurPos <= my->SelStart)
					my->SelStart+= (my->CurPos - my->PrevCurPos);
			}

			CheckForSpecialWidthChars(&ch, 1);
			Changed();

		} else if (Flags.Check(FEDITLINE_OVERTYPE)) {
			if (my->CurPos < Str.Size()) {
				my->PrevCurPos = my->CurPos;
				Str[my->CurPos++] = Key;
				Changed();
			}
		}
		/*else
			MessageBeep(MB_ICONHAND);*/
	}

	return TRUE;
}

int Edit::GetVisualLineCount() const
{
	if (!GetWordWrap())
		return 1;

	MyEcoLazy::See my(fields);
	return my->WrapBreaks.empty() ? 1 : my->WrapBreaks.size();
}

int Edit::FindVisualLine(int Pos) const
{
	if (Pos <= 0 || !GetWordWrap())
		return 0;

	MyEcoLazy::See my(fields);
	if (my->WrapBreaks.empty())
		return 0;

	const auto it = std::upper_bound(my->WrapBreaks.begin(), my->WrapBreaks.end(), Pos);
	return std::max(0, static_cast<int>(it - my->WrapBreaks.begin()) - 1);
}

void Edit::GetVisualLine(int line, int& start, int& end) const
{
	start = 0;
	end = Str.Size();
	if (line < 0 || !GetWordWrap())
		return;

	MyEcoLazy::See my(fields);
	if (my->WrapBreaks.empty())
		return;

	if (static_cast<size_t>(line) < my->WrapBreaks.size()) {
		start = my->WrapBreaks[line];
		if (static_cast<size_t>(line + 1) < my->WrapBreaks.size())
			end = my->WrapBreaks[line + 1];
		else
			end = Str.Size();
	} else {
		start = end = Str.Size();
	}
}

void Edit::RecalculateWordWrap(int Width, int TabSize)
{
	MyEcoLazy::Use my(fields);
    Width--; // save last column for cursor

	if (!GetWordWrap() || Width <= 1)
	{
		my->WrapBreaks.clear();
		return;
	}

	int CurrentStart = 0;
	bool HasWrap = false;
	while (CurrentStart < Str.Size())
	{
		int CurrentPos = CurrentStart;
		int CurrentX = 0;
		int LastBreakPos = -1; // Position *after* a space, where the new line would start.

		int ForceBreakPos = -1;

		while (CurrentPos < Str.Size())
		{
			int CharWidth = 1;
			if (Str[CurrentPos] == L'\t') {
				CharWidth = TabSize - (CurrentX % TabSize);
			} else if (Str.IsFullWidth(CurrentPos)) {
				CharWidth = 2;
			}

			if (CurrentX + CharWidth > Width)
			{
				ForceBreakPos = (CurrentPos > CurrentStart) ? CurrentPos : CurrentStart + 1;
				break;
			}

			CurrentX += CharWidth;

			if (Str[CurrentPos] == L' ') {
				LastBreakPos = CurrentPos + 1;
			}

			CurrentPos++;
		}

		if (ForceBreakPos == -1) // Didn't exceed width, so we are done with this line
		{
			break;
		}

		int NextStart = (LastBreakPos != -1) ? LastBreakPos : ForceBreakPos;

		if (!HasWrap) {
			my->WrapBreaks.clear();
			my->WrapBreaks.emplace_back(0);
			HasWrap = true;
		}

		my->WrapBreaks.emplace_back(NextStart);
		CurrentStart = NextStart;
	}

	if (!HasWrap)
		my->WrapBreaks.clear();
}

void Edit::SetObjectColor(uint64_t Color, uint64_t SelColor, uint64_t ColorUnChanged)
{
	if (auto *editor = GetEditorOwner()) {
		editor->SetObjectColor(Color, SelColor, ColorUnChanged);
		return;
	}

	if (auto *s = s_e2s.Get(this, true)) {
		s->Color = Color;
		s->SelColor = SelColor;
		s->ColorUnChanged = ColorUnChanged;
	}
}

long Edit::GetObjectColor()
{
	uint64_t Color, SelColor, ColorUnChanged;
	GetObjectColors(Color, SelColor, ColorUnChanged);
	return MAKELONG(Color, SelColor);
}

int Edit::GetObjectColorUnChanged()
{
	uint64_t Color, SelColor, ColorUnChanged;
	GetObjectColors(Color, SelColor, ColorUnChanged);
	return ColorUnChanged;
}

void Edit::SetHiString(const wchar_t *Str)
{
	if (Flags.Check(FEDITLINE_READONLY))
		return;

	FARString NewStr;
	HiText2Str(NewStr, Str);
	Select(-1, 0);
	SetBinaryString(NewStr, StrLength(NewStr));
}

void Edit::SetString(const wchar_t *Str, int Length)
{
	if (Flags.Check(FEDITLINE_READONLY))
		return;

	Select(-1, 0);
	SetBinaryString(Str, Length == -1 ? (int)StrLength(Str) : Length);
}


template <class CHAR_T>
	int TypeOfEOL(const CHAR_T *EOL)
{
	if (EOL && *EOL) {
		if (EOL[0] == L'\r')
			if (EOL[1] == L'\n')
				return EOL_CRLF;
			else if (EOL[1] == L'\r' && EOL[2] == L'\n')
				return EOL_CRCRLF;
			else
				return EOL_CR;
		else if (EOL[0] == L'\n')
			return EOL_LF;
	}
	return EOL_NONE;
}

void Edit::SetEOL(const wchar_t *EOL)
{
	SetEndType(TypeOfEOL(EOL));
}

void Edit::SetEOL(const char *EOL)
{
	SetEndType(TypeOfEOL(EOL));
}

const wchar_t *Edit::GetEOL()
{
	return EOL_TYPE_CHARS[GetEndType()];
}

void Edit::CheckForSpecialWidthChars(const wchar_t *CheckStr, int Length)
{
	if (Flags.Check(FEDITLINE_HASSPECIALWIDTHCHARS)) return;

	bool AndTabs = true;
	if (!CheckStr) {
		CheckStr = Str.CPtr();
		Length = Str.Size();
	} else if (GetConvertTabs() == EXPAND_ALLTABS) {
		AndTabs = false; // this is a string to be inserted and its tabs gonna be expanded to spaces, so ignore them
	}
	for (int i = 0; i < Length; ++i) {
		if ( (AndTabs && CheckStr[i] == L'\t') || Str.IsFullWidth(i) || Str.IsXxxfix(i) ) {
			Flags.Set(FEDITLINE_HASSPECIALWIDTHCHARS);
			return;
		}
	}
}

/*
	$ 25.07.2000 tran
	примечание:
	в этом методе DropDownBox не обрабатывается
	ибо он вызывается только из SetString и из класса Editor
	в Dialog он нигде не вызывается
*/
void Edit::SetBinaryString(const wchar_t *Str, int Length)
{
	const wchar_t *Mask = GetInputMask();
	if (Flags.Check(FEDITLINE_READONLY))
		return;

	MyEcoLazy::Use my(fields);

	const int MaxLength = GetMaxLength();
	// коррекция вставляемого размера, если определен MaxLength
	if (MaxLength != -1 && Length > MaxLength) {
		Length = MaxLength;		// ??
	}

	if (Length > 0 && !Flags.Check(FEDITLINE_PARENT_SINGLELINE)) {
		if (Str[Length - 1] == L'\r') {
			SetEndType(EOL_CR);
			Length--;
		} else {
			if (Str[Length - 1] == L'\n') {
				Length--;

				if (Length > 0 && Str[Length - 1] == L'\r') {
					Length--;

					if (Length > 0 && Str[Length - 1] == L'\r') {
						Length--;
						SetEndType(EOL_CRCRLF);
					} else
						SetEndType(EOL_CRLF);
				} else
					SetEndType(EOL_LF);
			} else
				SetEndType(EOL_NONE);
		}
	}

	my->CurPos = 0;

	if (Mask && *Mask) {
		RefreshStrByMask(TRUE);
		int maskLen = StrLength(Mask);

		for (int i = 0, j = 0; j < maskLen && j < Length;) {
			if (CheckCharMask(Mask[i])) {
				int goLoop = FALSE;

				if (KeyMatchedMask(Str[j]))
					InsertKey(Str[j]);
				else
					goLoop = TRUE;

				j++;

				if (goLoop)
					continue;
			} else {
				my->PrevCurPos = my->CurPos;
				my->CurPos++;
			}

			i++;
		}

		/*
			Здесь необходимо условие (!*Str), т.к. для очистки строки
			обычно вводится нечто вроде SetBinaryString("",0)
			Т.е. таким образом мы добиваемся "инициализации" строки с маской
		*/
		RefreshStrByMask(!*Str);
	} else {
		if (!this->Str.Assign(Str, Length)) {
			fprintf(stderr, "Edit::SetBinaryString: failed to assign to length of %d\n", Length);
			return;
		}
		if (GetConvertTabs() == EXPAND_ALLTABS)
			ExpandTabs();

		my->PrevCurPos = my->CurPos;
		my->CurPos = this->Str.Size();

		Flags.Clear(FEDITLINE_HASSPECIALWIDTHCHARS);
		CheckForSpecialWidthChars(Str, Length);
	}

	if (GetWordWrap()) {
		int Width = ObjWidth();
		if (Flags.Check(FEDITLINE_EDITORMODE)) { // Corresponds to editor.cpp's EdOpt.ShowScrollBar
			// This logic is a bit of a guess, assuming FEDITLINE_EDITORMODE is a good proxy.
			// In editor.cpp, XX2 is calculated based on NumLastLine > Y2-Y1+1. We don't have that here.
			// Let's assume for now if it's in editor mode, scrollbar might be there.
			// A better solution would be to pass this info down.
			// For now, let's just use ObjWidth as it is passed down correctly.
		}
		RecalculateWordWrap(Width, GetTabSize());
	}
	Changed();
}

void Edit::GetString(int Offset, wchar_t *Data, int MaxSize)
{
	if (LIKELY(MaxSize > 0)) {
		if (Offset < Str.Size()) {
			const auto l = std::min(Str.Size() - Offset, MaxSize);
			Str.CopyTo(Data, Offset, l);
			if (l < MaxSize) {
				Data[l] = 0;
			}
		} else {
			Data[0] = 0;
		}
	}
}

int Edit::GetLength(const wchar_t **EOL)
{
	if (EOL)
		*EOL = EOL_TYPE_CHARS[GetEndType()];

	return Str.Size();
}

const wchar_t *Edit::GetStringAddr()
{
	const wchar_t *out = Str.CPtr();
	return LIKELY(out) ? out : L"";
}

const wchar_t *Edit::GetStringAddr(int &Length, const wchar_t **EOL)
{
	if (EOL)
		*EOL = EOL_TYPE_CHARS[GetEndType()];

	const wchar_t *out = Str.CPtr();
	if (LIKELY(out)) {
		Length = Str.Size();	//???
		return out;
	}

	Length = 0;
	return L"";
}

int Edit::GetSelString(wchar_t *Data, int MaxSize)
{
	MyEcoLazy::Use my(fields);
	if (my->SelStart == -1 || (my->SelEnd != -1 && my->SelEnd <= my->SelStart) || my->SelStart >= Str.Size()) {
		*Data = 0;
		return FALSE;
	}

	int CopyLength;

	if (my->SelEnd == -1)
		CopyLength = MaxSize;
	else
		CopyLength = Min(MaxSize, my->SelEnd - my->SelStart + 1);

	far_wcsncpy(Data, Str.CPtr() + my->SelStart, CopyLength);
	return TRUE;
}

int Edit::GetSelString(FARString &strStr)
{
	MyEcoLazy::Use my(fields);
	if (my->SelStart == -1 || (my->SelEnd != -1 && my->SelEnd <= my->SelStart) || my->SelStart >= Str.Size()) {
		strStr.Clear();
		return FALSE;
	}

	strStr.Copy(this->Str.CPtr() + my->SelStart, my->SelEnd - my->SelStart + 1);
	return TRUE;
}

void Edit::InsertString(const wchar_t *Str)
{
	if (Flags.Check(FEDITLINE_READONLY | FEDITLINE_DROPDOWNBOX))
		return;

	if (!Flags.Check(FEDITLINE_PERSISTENTBLOCKS))
		DeleteBlock();

	InsertBinaryString(Str, StrLength(Str));
}

void Edit::InsertBinaryString(const wchar_t *Str, int Length)
{
	const wchar_t *Mask = GetInputMask();
	if (Flags.Check(FEDITLINE_READONLY | FEDITLINE_DROPDOWNBOX))
		return;

	Flags.Clear(FEDITLINE_CLEARFLAG);
	MyEcoLazy::Use my(fields);

	if (Mask && *Mask) {
		int Pos = my->CurPos;
		int MaskLen = StrLength(Mask);

		if (Pos < MaskLen) {
			//_SVS(SysLog(L"InsertBinaryString ==> Str='%ls' (Length=%d) Mask='%ls'",Str,Length,Mask+Pos));
			int StrLen = (MaskLen - Pos > Length) ? Length : MaskLen - Pos;

			/*
				$ 15.11.2000 KM
				Внесены исправления для правильной работы PasteFromClipboard
				в строке с маской
			*/
			for (int i = Pos, j = 0; j < StrLen + Pos;) {
				if (CheckCharMask(Mask[i])) {
					int goLoop = FALSE;

					if (j < Length && KeyMatchedMask(Str[j])) {
						InsertKey(Str[j]);
						//_SVS(SysLog(L"InsertBinaryString ==> InsertKey(Str[%d]='%c');",j,Str[j]));
					} else
						goLoop = TRUE;

					j++;

					if (goLoop)
						continue;
				} else {
					if (Mask[j] == Str[j]) {
						j++;
					}
					my->PrevCurPos = my->CurPos;
					my->CurPos++;
				}

				i++;
			}
		}

		RefreshStrByMask();
		//_SVS(SysLog(L"InsertBinaryString ==> this->Str='%ls'",this->Str));
	} else {
		const int MaxLength = GetMaxLength();
		this->Str.Expand(my->CurPos);
		if (!this->Str.Insert(my->CurPos, Str, Length)) {
			fprintf(stderr, "Edit::InsertBinaryString: failed to insert %d chars\n", Length);
			return;
		}
		
		my->CurPos+= Length;

		if (MaxLength != -1 && this->Str.Size() > MaxLength) {
			this->Str.Truncate(MaxLength);
			if (my->CurPos > MaxLength) {
				my->CurPos = MaxLength;
			}
		}

		if (GetConvertTabs() == EXPAND_ALLTABS) {
			ExpandTabs();
		}

		if (GetWordWrap()) {
			RecalculateWordWrap(ObjWidth(), GetTabSize());
		}

		CheckForSpecialWidthChars(Str, Length);
		Changed();
		/*else
			MessageBeep(MB_ICONHAND);*/
	}
}

// Функция установки маски ввода в объект Edit
void Edit::SetInputMask(const wchar_t *InputMask)
{
	if (!InputMask || !*InputMask) {
		if (auto *s = s_e2s.Get(this))
			s->Mask.reset();
		return;
	}

	if (auto *s = s_e2s.Get(this, true)) {
		s->Mask.reset(wcsdup(InputMask));
		if (s->Mask) {
			RefreshStrByMask(TRUE);
		}
	}
}

const wchar_t *Edit::GetInputMask() const
{
	if (const auto *s = s_e2s.Get(this)) {
		return s->Mask.get();
	}
	return nullptr;
}

// Функция обновления состояния строки ввода по содержимому Mask
void Edit::RefreshStrByMask(int InitMode)
{
	const wchar_t *Mask = GetInputMask();
	if (Mask && *Mask) {
		int MaskLen = StrLength(Mask);

		if (Str.Size() != MaskLen && !Str.Resize(MaskLen, L' ')) {
			fprintf(stderr, "Edit::RefreshStrByMask: failed to resize to %d\n", MaskLen);
			return;
		}

		for (int i = 0; i < MaskLen; i++) {
			if (!CheckCharMask(Mask[i]))
				Str[i] = Mask[i];
			else if (InitMode)
				Str[i] = L' ';
		}
	}
}

int Edit::ProcessMouse(MOUSE_EVENT_RECORD *MouseEvent)
{
	if (!(MouseEvent->dwButtonState & 3))
		return FALSE;

	if (MouseEvent->dwMousePosition.X < X1 || MouseEvent->dwMousePosition.X > X2 || MouseEvent->dwMousePosition.Y != Y1)
		return FALSE;

	MyEcoLazy::Use my(fields);
	// SetClearFlag(0); // пусть едитор сам заботится о снятии клеар-текста?
	SetCellCurPos(MouseEvent->dwMousePosition.X - X1 + my->LeftPos);

	if (!Flags.Check(FEDITLINE_PERSISTENTBLOCKS))
		Select(-1, 0);

	if (MouseEvent->dwButtonState & FROM_LEFT_1ST_BUTTON_PRESSED) {
		static int PrevDoubleClick = 0;
		static COORD PrevPosition = {0, 0};

		if (WINPORT(GetTickCount)() - PrevDoubleClick <= WINPORT(GetDoubleClickTime)()
				&& MouseEvent->dwEventFlags != MOUSE_MOVED && PrevPosition.X == MouseEvent->dwMousePosition.X
				&& PrevPosition.Y == MouseEvent->dwMousePosition.Y) {
			Select(0, Str.Size());
			PrevDoubleClick = 0;
			PrevPosition.X = 0;
			PrevPosition.Y = 0;
		}

		if (MouseEvent->dwEventFlags == DOUBLE_CLICK) {
			ProcessKey(KEY_OP_SELWORD);
			PrevDoubleClick = WINPORT(GetTickCount)();
			PrevPosition = MouseEvent->dwMousePosition;
		} else {
			PrevDoubleClick = 0;
			PrevPosition.X = 0;
			PrevPosition.Y = 0;
		}
	}

	Show();
	return TRUE;
}

/*
	$ 03.08.2000 KM
	Немного изменён алгоритм из-за необходимости
	добавления поиска целых слов.
*/
int Edit::Search(const FARString &What, FARString &ReplaceStr, int Position, int Case, int WholeWords,
		int Reverse, int Regexp, int *SearchLength)
{
	MyEcoLazy::Use my(fields);
	return SearchString(Str.CPtr(), Str.Size(), What.CPtr(), ReplaceStr, my->CurPos, Position, Case, WholeWords,
			Reverse, Regexp, SearchLength, WordDiv());
}

void Edit::ExpandTabs()
{
	if (Flags.Check(FEDITLINE_READONLY))
		return;

	bool changed = false;

	MyEcoLazy::Use my(fields);
	for (int Pos = Str.Find(L'\t'); Pos != -1; Pos = Str.Find(L'\t', Pos + 1)) {
		auto S = GetTabSize() - (Pos % GetTabSize());

		if (!Str.Replace(Pos, 1, L' ', S)) {
			fprintf(stderr, "Edit::ExpandTabs: failed to replace\n");
			break;
		}

		if (my->SelStart != -1) {
			if (Pos <= my->SelStart) {
				my->SelStart+= S - (Pos == my->SelStart ? 0 : 1);
			}

			if (my->SelEnd != -1 && Pos < my->SelEnd) {
				my->SelEnd+= S - 1;
			}
		}

		if (my->CurPos > Pos) {
			my->CurPos+= S - 1;
		}

		changed = true;
	}

	if (changed)
		Changed();
}

void Edit::SetCurPos(int NewPos)
{
	MyEcoLazy::Use my(fields);
	my->CurPos = NewPos;
	my->PrevCurPos = NewPos;
}

int Edit::GetLeftPos()
{
	MyEcoLazy::See my(fields);
	return (my->LeftPos);
}

void Edit::SetLeftPos(int NewPos)
{
	MyEcoLazy::Use my(fields);
	my->LeftPos = NewPos;
}

int Edit::GetCurPos()
{
	MyEcoLazy::See my(fields);
	return (my->CurPos);
}

int Edit::GetCellCurPos()
{
	MyEcoLazy::See my(fields);
	return (RealPosToCell(my->CurPos));
}

void Edit::SetCellCurPos(int NewPos)
{
	MyEcoLazy::Use my(fields);
	const wchar_t *Mask = GetInputMask();
	if (Mask && *Mask) {
		int NewPosLimit = CalcRTrimmedStrSize();
		if (NewPos > NewPosLimit)
			NewPos = NewPosLimit;
	}

	my->CurPos = CellPosToReal(NewPos);
}

int Edit::RealPosToCell(int Pos)
{
	return RealPosToCell(0, 0, Pos, nullptr);
}

int Edit::RealPosToCell(int PrevLength, int PrevPos, int Pos, int *CorrectPos)
{
	// Корректировка табов
	bool bCorrectPos = CorrectPos && *CorrectPos;
	if (CorrectPos)
		*CorrectPos = 0;

	// Инциализируем результирующую длину предыдущим значением
	int TabPos = PrevLength;

	// Если предыдущая позиция за концом строки, то табов там точно нет и
	// вычислять особо ничего не надо, иначе производим вычисление
	if (PrevPos >= Str.Size() || !Flags.Check(FEDITLINE_HASSPECIALWIDTHCHARS))
		TabPos+= Pos - PrevPos;
	else {
		// Начинаем вычисление с предыдущей позиции
		int Index = PrevPos;
		bool joining = false;
		// Проходим по всем символам до позиции поиска, если она ещё в пределах строки,
		// либо до конца строки, если позиция поиска за пределами строки
		for (; Index < Min(Pos, Str.Size()); Index++)

			// Обрабатываем табы
			if (Str[Index] == L'\t' && GetConvertTabs() != EXPAND_ALLTABS) {
				// Если есть необходимость делать корректировку табов и эта корректировка
				// ещё не проводилась, то увеличиваем длину обрабатываемой строки на единицу
				if (bCorrectPos) {
					++Pos;
					*CorrectPos = 1;
					bCorrectPos = false;
				}

				// Расчитываем длину таба с учётом настроек и текущей позиции в строке
				TabPos+= GetTabSize() - (TabPos % GetTabSize());
				joining = false;
			}
			// Обрабатываем все остальные символы
			else {
				if (Str[Index] == CharClasses::ZERO_WIDTH_JOINER)
				{
					joining = true;
					continue;
				}
				if (Str.IsXxxfix(Index))
					continue;
				if (joining)
				{
					joining = false;
					continue;
				}

				TabPos += Str.IsFullWidth(Index) ? 2 : 1;
			}

		// Если позиция находится за пределами строки, то там точно нет табов и всё просто
		if (Pos >= Str.Size())
			TabPos+= Pos - Index;
	}
	return TabPos;
}

int Edit::CellPosToReal(int Pos)
{
	if (Pos < 0) return 0;
	if (!Flags.Check(FEDITLINE_HASSPECIALWIDTHCHARS)) return Pos;
	int Index = 0;
	bool joining = false;
	for (int CellPos = 0; CellPos < Pos || joining; Index++) {
		if (Index >= Str.Size()) {
			Index+= Pos - CellPos;
			break;
		}

		if (Str[Index] == L'\t' && GetConvertTabs() != EXPAND_ALLTABS) {
			int NewCellPos = CellPos + GetTabSize() - (CellPos % GetTabSize());

			if (NewCellPos > Pos)
				break;

			CellPos = NewCellPos;
			joining = false;
		} else {
			if (Str[Index] == CharClasses::ZERO_WIDTH_JOINER)
			{
				joining = true;
				continue;
			}

			if (Str.IsXxxfix(Index))
				continue;

			if (!joining)
				CellPos += Str.IsFullWidth(Index) ? 2 : 1;

			joining = false;
			while (Index + 1 < Str.Size() && Str.IsXxxfix(Index + 1)) {
				if (Str[Index + 1] == CharClasses::ZERO_WIDTH_JOINER)
					joining = true;
				Index++;
			}
		}
	}
	return Index;
}

void Edit::SanitizeSelectionRange()
{
	MyEcoLazy::Use my(fields);
	if (Flags.Check(FEDITLINE_HASSPECIALWIDTHCHARS) && my->SelEnd >= my->SelStart && my->SelStart >= 0) {
		bool joining = false;
		if (my->SelStart >= Str.Size()) {
			fprintf(stderr, "%s: SelStart{%d} >= StrSize{%d} - FIXME!!!\n", __FUNCTION__, my->SelStart, Str.Size());
			my->SelStart = std::max(Str.Size() - 1, 0);
		}
		for ( ; my->SelStart > 0; my->SelStart--) {
			if (Str[my->SelStart] == CharClasses::ZERO_WIDTH_JOINER) {
				joining = true;
			} else if (Str.IsXxxfix(my->SelStart)) {
				continue;
			} else if (joining) {
				joining = false;
			} else {
				break;
			}
		}

		joining = false;
		for ( ; my->SelEnd < Str.Size(); my->SelEnd++) {
			if (Str[my->SelEnd] == CharClasses::ZERO_WIDTH_JOINER) {
				joining = true;
			} else if (Str.IsXxxfix(my->SelEnd)) {
				continue;
			} else if (joining) {
				joining = false;
			} else {
				break;
			}
		}
	}

	/*
		$ 24.06.2002 SKV
		Если начало выделения за концом строки, надо выделение снять.
		17.09.2002 возвращаю обратно. Глюкодром.
	*/
	if (my->SelEnd < my->SelStart && my->SelEnd != -1) {
		my->SelStart = -1;
		my->SelEnd = 0;
	}

	if (my->SelStart == -1 && my->SelEnd == -1) {
		my->SelStart = -1;
		my->SelEnd = 0;
	}
}

void Edit::Select(int Start, int End)
{
	MyEcoLazy::Use my(fields);
	my->SelStart = Start;
	my->SelEnd = End;

	if (Start != -1 || End != 0) {
		SanitizeSelectionRange();
	}
}

void Edit::AddSelect(int Start, int End)
{
	MyEcoLazy::Use my(fields);
	if (Start < my->SelStart || my->SelStart == -1)
		my->SelStart = Start;

	if (End == -1 || (End > my->SelEnd && my->SelEnd != -1))
		my->SelEnd = End;

	if (my->SelEnd > Str.Size())
		my->SelEnd = Str.Size();

	SanitizeSelectionRange();
}

bool Edit::IsSelection()
{
	MyEcoLazy::See my(fields);
	return my->SelStart != -1 || my->SelEnd != 0;
}

Edit::Selection Edit::GetSelection()
{
	/*
		$ 17.09.2002 SKV
		Мало того, что это нарушение правил OO design'а,
		так это еще и источние багов.
	*/
	/*
	if (SelEnd>Str.Size()+1)
		SelEnd=Str.Size()+1;
	if (SelStart>Str.Size()+1)
		SelStart=Str.Size()+1;
	*/
	/* SKV $ */
	Selection Sel = GetRealSelection();

	if (Sel.End > Str.Size())
		Sel.End = -1;	// StrSize;

	if (Sel.Start > Str.Size())
		Sel.Start = Str.Size();

	return Sel;
}

Edit::Selection Edit::GetRealSelection()
{
	MyEcoLazy::See my(fields);
	return {my->SelStart, my->SelEnd};
}

void Edit::DeleteBlock()
{
	const wchar_t *Mask = GetInputMask();
	if (Flags.Check(FEDITLINE_READONLY | FEDITLINE_DROPDOWNBOX))
		return;

	MyEcoLazy::Use my(fields);
	if (my->SelStart == -1 || my->SelStart >= my->SelEnd)
		return;

	my->PrevCurPos = my->CurPos;

	if (Mask && *Mask) {
		for (int i = my->SelStart; i < my->SelEnd; i++) {
			if (CheckCharMask(Mask[i]))
				Str[i] = L' ';
		}

		my->CurPos = my->SelStart;
	} else {
		const auto From = std::min(my->SelStart, Str.Size());
		const auto To = std::min(my->SelEnd, Str.Size());

		if (To < From || !Str.Remove(From, To - From)) {
			fprintf(stderr, "Edit::DeleteBlock: remove [%d, %d) failed\n", From, To);
			return;
		}

		if (my->CurPos > From) {
			if (my->CurPos < To)
				my->CurPos = From;
			else
				my->CurPos-= To - From;
		}
	}

	my->SelStart = -1;
	my->SelEnd = 0;
	Flags.Clear(FEDITLINE_MARKINGBLOCK);

	// OT: Проверка на корректность поведения строки при удалении и вставки
	if (Flags.Check((FEDITLINE_PARENT_SINGLELINE | FEDITLINE_PARENT_MULTILINE))) {
		if (my->LeftPos > my->CurPos)
			my->LeftPos = my->CurPos;
	}

	Changed(true);
}

void Edit::AddColor(const ColorItem *col)
{
	ColorList.emplace_back(*col);
}

size_t Edit::DeleteColor(int ColorPos)
{
	if (ColorList.empty())
		return 0;

	size_t Dest, Src;

	for (Src = Dest = 0; Src < ColorList.size(); ++Src)
		if (ColorPos != -1 && ColorList[Src].StartPos != ColorPos) {
			if (Dest != Src)
				ColorList[Dest] = ColorList[Src];

			++Dest;
		}

	const size_t DelCount = ColorList.size() - Dest;
	ColorList.resize(Dest);
	return DelCount;
}

bool Edit::GetColor(ColorItem *col, int Item)
{
	if ((size_t)Item >= ColorList.size())
		return false;

	*col = ColorList[Item];
	return true;
}

void Edit::ApplyColor()
{
	if (ColorList.empty())
		return;

	uint64_t Color, SelColor, ColorUnChanged;
	GetObjectColors(Color, SelColor, ColorUnChanged);

	// Для оптимизации сохраняем вычисленные позиции между итерациями цикла
	int Pos = INT_MIN, TabPos = INT_MIN, TabEditorPos = INT_MIN;

	MyEcoLazy::See my(fields);
	// Обрабатываем элементы ракраски
	for (auto &CurItem : ColorList) {

		// Пропускаем элементы у которых начало больше конца
		if (CurItem.StartPos > CurItem.EndPos)
			continue;
		// Отсекаем элементы заведомо не попадающие на экран
		/*if (CurItem.StartPos - LeftPos > X2 && CurItem.EndPos - LeftPos < X1)
			continue;*/
		/* ^^^ закомментировано, т.к. при текущем && условие никогда не выполняется - лишняя проверка в цикле.
		       Замена на || приводит к некорректной логике,
		       если в строке за пределами видимой части есть \t или многобайтовые.*/

		int Length = CurItem.EndPos - CurItem.StartPos + 1;

		if (CurItem.StartPos + Length >= Str.Size())
			Length = Str.Size() - CurItem.StartPos;

		// Получаем начальную позицию
		int RealStart, Start;

		/*
			Если предыдущая позиция равна текущей, то ничего не вычисляем
			и сразу берём ранее вычисленное значение
		*/
		if (Pos == CurItem.StartPos) {
			RealStart = TabPos;
			Start = TabEditorPos;
		}
		/*
			Если вычисление идёт первый раз или предыдущая позиция больше текущей,
			то производим вычисление с начала строки
		*/
		else if (Pos == INT_MIN || CurItem.StartPos < Pos) {
			RealStart = RealPosToCell(CurItem.StartPos);
			Start = RealStart - my->LeftPos;
		}
		// Для оптимизации делаем вычисление относительно предыдущей позиции
		else {
			RealStart = RealPosToCell(TabPos, Pos, CurItem.StartPos, nullptr);
			Start = RealStart - my->LeftPos;
		}

		// Запоминаем вычисленные значения для их дальнейшего повторного использования
		Pos = CurItem.StartPos;
		TabPos = RealStart;
		TabEditorPos = Start;

		// Пропускаем элементы раскраски у которых начальная позиция за экраном
		if (Start > ObjWidth() - 1)
			continue;

		// Корректировка относительно табов (отключается, если присутвует флаг ECF_TAB1)
		DWORD64 Attr = CurItem.Color;
		int CorrectPos = Attr & ECF_TAB1 ? 0 : 1;

		if (!CorrectPos)
			Attr&= ~ECF_TAB1;

		// Получаем конечную позицию
		int EndPos = CurItem.EndPos;
		int RealEnd, End;

		/*
			Обрабатываем случай, когда предыдущая позиция равна текущей, то есть
			длина раскрашиваемой строки равна 1
		*/
		if (Pos == EndPos) {
			/*
				Если необходимо делать корректироку относительно табов и единственный
				символ строки -- это таб, то делаем расчёт с учтом корректировки,
				иначе ничего не вычисялем и берём старые значения
			*/
			if (CorrectPos && EndPos < Str.Size() && Str[EndPos] == L'\t') {
				RealEnd = RealPosToCell(TabPos, Pos, ++EndPos, nullptr);
				End = RealEnd - my->LeftPos;
			} else {
				RealEnd = TabPos;
				CorrectPos = 0;
				End = TabEditorPos;
			}
		}
		/*
			Если предыдущая позиция больше текущей, то производим вычисление
			с начала строки (с учётом корректировки относительно табов)
		*/
		/*else if (EndPos < Pos) {
			RealEnd = RealPosToCell(0, 0, EndPos, &CorrectPos);
			EndPos+= CorrectPos;
			End = RealEnd - LeftPos;
		}*/
		// ^^^ закомментировано, т.к. данное условие всегда ложно - лишняя проверка в цикле.

		/*
			Для оптимизации делаем вычисление относительно предыдущей позиции (с учётом
			корректировки относительно табов)
		*/
		else {
			RealEnd = RealPosToCell(TabPos, Pos, EndPos, &CorrectPos);
			EndPos+= CorrectPos;
			End = RealEnd - my->LeftPos;
		}

		// Запоминаем вычисленные значения для их дальнейшего повторного использования
		Pos = EndPos;
		TabPos = RealEnd;
		TabEditorPos = End;

		if (Start < 0)
			Start = 0;

		if (End > ObjWidth() - 1)
			End = ObjWidth() - 1;

		// Устанавливаем длину раскрашиваемого элемента
		Length = End - Start + 1;

		if (Length < X2)
			Length-= CorrectPos;

		if (Length > 0) {
			ScrBuf.ApplyColor(X1 + Start, Y1, X1 + Start + Length - 1, Y1, Attr, SelColor );
					// Не раскрашиваем выделение
//					SelColor >= COL_FIRSTPALETTECOLOR ? Palette[SelColor - COL_FIRSTPALETTECOLOR] : SelColor);
		}
	}
}

/*
	$ 24.09.2000 SVS $
	Функция Xlat - перекодировка по принципу QWERTY <-> ЙЦУКЕН
*/
void Edit::Xlat(bool All)
{
	MyEcoLazy::Use my(fields);
	// Для CmdLine - если нет выделения, преобразуем всю строку
	if (All && my->SelStart == -1 && !my->SelEnd) {
		::Xlat(Str.Ptr(), 0, Str.Size(), Opt.XLat.Flags);
		Changed();
		Show();
		return;
	}

	if (my->SelStart != -1 && my->SelStart != my->SelEnd) {
		if (my->SelEnd == -1)
			my->SelEnd = Str.Size();

		::Xlat(Str.Ptr(), my->SelStart, my->SelEnd, Opt.XLat.Flags);
		Changed();
		Show();
	}
	/*
		$ 25.11.2000 IS
		Если нет выделения, то обработаем текущее слово. Слово определяется на
		основе специальной группы разделителей.
	*/
	else {
		/*
			$ 10.12.2000 IS
			Обрабатываем только то слово, на котором стоит курсор, или то слово, что
			находится левее позиции курсора на 1 символ
		*/
		int start = my->CurPos, end, len = Str.Size();
		bool DoXlat = true;

		if (IsWordDiv(Opt.XLat.strWordDivForXlat, Str[start])) {
			if (start)
				start--;

			DoXlat = (!IsWordDiv(Opt.XLat.strWordDivForXlat, Str[start]));
		}

		if (DoXlat) {
			while (start >= 0 && !IsWordDiv(Opt.XLat.strWordDivForXlat, Str[start]))
				start--;

			start++;
			end = start + 1;

			while (end < len && !IsWordDiv(Opt.XLat.strWordDivForXlat, Str[end]))
				end++;

			::Xlat(Str.Ptr(), start, end, Opt.XLat.Flags);
			Changed();
			Show();
		}
	}
}

/*
	$ 15.11.2000 KM
	Проверяет: попадает ли символ в разрешённый
	диапазон символов, пропускаемых маской
*/
int Edit::KeyMatchedMask(FarKey Key)
{
	MyEcoLazy::See my(fields);
	const wchar_t *Mask = GetInputMask();
	switch (Mask[my->CurPos]) {
		case EDMASK_DSS:
			return (std::iswdigit(Key) || Key == L' ' || Key == L'-');
		case EDMASK_DIGITS:
			return (std::iswdigit(Key) || Key == L' ');
		case EDMASK_DIGIT:
			return std::iswdigit(Key);
		case EDMASK_ALPHA:
			return IsAlpha(Key);
		case EDMASK_HEX:
			return std::iswxdigit(Key);
		case EDMASK_ANY:
			return true;
		default:
			return false;
	}
}

int Edit::CheckCharMask(wchar_t Chr)
{
	return (Chr == EDMASK_ANY || Chr == EDMASK_DIGIT || Chr == EDMASK_DIGITS || Chr == EDMASK_DSS
				|| Chr == EDMASK_ALPHA || Chr == EDMASK_HEX)
			? TRUE
			: FALSE;
}

void Edit::SetDialogParent(DWORD Sets)
{
	if ((Sets & (FEDITLINE_PARENT_SINGLELINE | FEDITLINE_PARENT_MULTILINE))
					== (FEDITLINE_PARENT_SINGLELINE | FEDITLINE_PARENT_MULTILINE)
			|| !(Sets & (FEDITLINE_PARENT_SINGLELINE | FEDITLINE_PARENT_MULTILINE)))
		Flags.Clear(FEDITLINE_PARENT_SINGLELINE | FEDITLINE_PARENT_MULTILINE);
	else if (Sets & FEDITLINE_PARENT_SINGLELINE) {
		Flags.Clear(FEDITLINE_PARENT_MULTILINE);
		Flags.Set(FEDITLINE_PARENT_SINGLELINE);
	} else if (Sets & FEDITLINE_PARENT_MULTILINE) {
		Flags.Clear(FEDITLINE_PARENT_SINGLELINE);
		Flags.Set(FEDITLINE_PARENT_MULTILINE);
	}
}

void Edit::Changed(bool DelBlock)
{
	if (auto *s = s_e2s.Get(this); s && s->Listener) {
		s->Listener->OnEditChanged(this);
	}
}

/*
SystemCPEncoder::SystemCPEncoder(int nCodePage)
{
	m_nCodePage = nCodePage;
	m_nRefCount = 1;
	m_strName.Format(L"codepage - %d", m_nCodePage);
}

SystemCPEncoder::~SystemCPEncoder()
{
}

int __stdcall SystemCPEncoder::AddRef()
{
	return ++m_nRefCount;
}

int __stdcall SystemCPEncoder::Release()
{
	if (!(--m_nRefCount))
	{
		delete this;
		return 0;
	}

	return m_nRefCount;
}

const wchar_t* __stdcall SystemCPEncoder::GetName()
{
	return (const wchar_t*)m_strName;
}

int __stdcall SystemCPEncoder::Encode(
	const char *lpString,
	int nLength,
	wchar_t *lpwszResult,
	int nResultLength
)
{
	int length = MultiByteToWideChar(m_nCodePage, 0, lpString, nLength, nullptr, 0);

	if (lpwszResult)
		length = MultiByteToWideChar(m_nCodePage, 0, lpString, nLength, lpwszResult, nResultLength);

	return length;
}

int __stdcall SystemCPEncoder::Decode(
	const wchar_t *lpwszString,
	int nLength,
	char *lpResult,
	int nResultLength
)
{
	int length = WideCharToMultiByte(m_nCodePage, 0, lpwszString, nLength, nullptr, 0, nullptr, nullptr);

	if (lpResult)
		length = WideCharToMultiByte(m_nCodePage, 0, lpwszString, nLength, lpResult, nResultLength, nullptr, nullptr);

	return length;
}

int __stdcall SystemCPEncoder::Transcode(
	const wchar_t *lpwszString,
	int nLength,
	ICPEncoder *pFrom,
	wchar_t *lpwszResult,
	int nResultLength
)
{
	int length = pFrom->Decode(lpwszString, nLength, nullptr, 0);
	char *lpDecoded = (char *)malloc(length);

	if (lpDecoded)
	{
		pFrom->Decode(lpwszString, nLength, lpDecoded, length);
		length = Encode(lpDecoded, length, nullptr, 0);

		if (lpwszResult)
			length = Encode(lpDecoded, length, lpwszResult, nResultLength);

		free(lpDecoded);
		return length;
	}

	return -1;
}
*/

////
static struct DummyEditListener : IEditListener
{
	virtual void OnEditChanged(Edit *edit) {}
} sDummyListener;

EditControl::EditControl(ScreenObject *pOwner, History *iHistory, FarList *iList, DWORD iFlags)
	:
	Edit(pOwner),
	pCustomCompletionList(nullptr),
	pHistory(iHistory),
	pList(iList),
	Selection(false),
	SelectionStart(-1),
	OverflowArrowsColor(0),
	ECFlags(iFlags)
{
	ACState = ECFlags.Check(EC_ENABLEAUTOCOMPLETE) != FALSE;
	SetListener(&sDummyListener);
}

void EditControl::ShowArrows()
{
	if (OverflowArrowsColor > 0) {
		MyEcoLazy::See my(fields);
		if (RealPosToCell(Str.Size()) > my->LeftPos + X2 - X1 && RealPosToCell(my->CurPos) != my->LeftPos + X2 - X1) {
			GotoXY(X2, Y1);
			SetColor(OverflowArrowsColor);
			BoxText(0xbb);
		}

		if (my->LeftPos > 0 && my->CurPos != my->LeftPos) {
			GotoXY(X1, Y1);
			SetColor(OverflowArrowsColor);
			BoxText(0xab);
		}
	}
}

void EditControl::Show()
{
	if (X2 - X1 + 1 > Str.Size()) {
		Edit::SetLeftPos(0);
	}

	Edit::Show();
	ShowArrows();
}

void EditControl::FastShow()
{
	MyEcoLazy::Use my(fields);
	if ( OverflowArrowsColor > 0 &&  RealPosToCell(Str.Size()) > my->LeftPos + X2 - X1 ) {
		//avoid right overflow arrow disappearance on dialog redraw resetting left position to 0
		Edit::SetLeftPos(std::max(my->LeftPos, RealPosToCell(my->CurPos) - X2 + X1 + 1));
	}
	Edit::FastShow();
	ShowArrows();
}

void EditControl::Changed(bool DelBlock)
{
	if (Edit::GetListener()) {
		Edit::Changed();
		AutoComplete(false, DelBlock);
	}
}

void EditControl::SetMenuPos(VMenu &menu)
{
	if (ScrY - Y1 < Min(Opt.Dialogs.CBoxMaxHeight, menu.GetItemCount()) + 2 && Y1 > ScrY / 2) {
		menu.SetPosition(X1, Max(0, Y1 - 1 - Min(Opt.Dialogs.CBoxMaxHeight, menu.GetItemCount()) - 1),
				Min(ScrX - 2, X2), Y1 - 1);
	} else {
		menu.SetPosition(X1, Y1 + 1, X2, 0);
	}
}

static void FilteredAddToMenu(VMenu &menu, const FARString &filter, const FARString &text)
{
	if (!StrCmpNI(text, filter, static_cast<int>(filter.GetLength())) && StrCmp(text, filter)) {
		menu.AddItem(text);
	}
}

void EditControl::PopulateCompletionMenu(VMenu &ComplMenu, const FARString &strFilter)
{
	SudoSilentQueryRegion ssqr;
	if (pCustomCompletionList) {
		for (const auto &possibility : *pCustomCompletionList)
			FilteredAddToMenu(ComplMenu, strFilter, FARString(possibility));

		if (ComplMenu.GetItemCount() < 10)
			ComplMenu.AssignHighlights(0);
	} else {
		if (pHistory) {
			pHistory->GetAllSimilar(ComplMenu, strFilter);
		} else if (pList) {
			for (int i = 0; i < pList->ItemsNumber; i++)
				FilteredAddToMenu(ComplMenu, strFilter, pList->Items[i].Text);
		}
		if (ECFlags.Check(EC_ENABLEFNCOMPLETE)) {
			if (!m_pSuggestor)
				m_pSuggestor.reset(new MenuFilesSuggestor);

			m_pSuggestor->Suggest(strFilter, ComplMenu, ECFlags.Check(EC_ENABLEFNCOMPLETE_ESCAPED));
		}
	}
}

void EditControl::RemoveSelectedCompletionMenuItem(VMenu &ComplMenu)
{
	int CurPos = ComplMenu.GetSelectPos();
	if (CurPos >= 0 && !pCustomCompletionList && pHistory) {
		FARString strName = ComplMenu.GetItemPtr(CurPos)->strName;
		if (pHistory->DeleteMatching(strName)) {
			ComplMenu.DeleteItem(CurPos, 1);
			ComplMenu.FastShow();
		}
	}
}

void EditControl::AutoCompleteProcMenu(bool &Result, bool Manual, bool DelBlock, FarKey &BackKey)
{
	MyEcoLazy::Use my(fields);
	VMenu ComplMenu(nullptr, nullptr, 0, 0);
	FARString strTemp = Str.CPtr();
	PopulateCompletionMenu(ComplMenu, strTemp);
	ComplMenu.SetBottomTitle(((!pCustomCompletionList && pHistory)
					? Msg::EditControlHistoryFooter
					: Msg::EditControlHistoryFooterNoDel));

	if (ComplMenu.GetItemCount() > 1
			|| (ComplMenu.GetItemCount() == 1 && StrCmpI(strTemp, ComplMenu.GetItemPtr(0)->strName))) {
		ComplMenu.SetFlags(VMENU_WRAPMODE | VMENU_NOTCENTER | VMENU_SHOWAMPERSAND);

		if (!DelBlock && Opt.AutoComplete.AppendCompletion
				&& (!Flags.Check(FEDITLINE_PERSISTENTBLOCKS) || Opt.AutoComplete.ShowList)) {
			int SelStart = GetLength();

			// magic
			if (IsSlash(Str[SelStart - 1]) && Str[SelStart - 2] == L'"'
					&& IsSlash(ComplMenu.GetItemPtr(0)->strName.At(SelStart - 2))) {
				Str[SelStart - 2] = Str[SelStart - 1];
				Str.Truncate(Str.Size() - 1);// StrSize--; NB: originally NUL char wasnt enforced after truncation
				my->SelStart--;
				my->CurPos--;
			}

			InsertString(ComplMenu.GetItemPtr(0)->strName.SubStr(my->SelStart));
			Select(my->SelStart, GetLength());
			Show();
		}
		if (Opt.AutoComplete.ShowList) {
			ChangeMacroMode MacroMode(MACRO_AUTOCOMPLETION);
			MenuItemEx EmptyItem;
			ComplMenu.AddItem(&EmptyItem, 0);
			SetMenuPos(ComplMenu);
			ComplMenu.SetSelectPos(0, 0);
			ComplMenu.SetBoxType(SHORT_SINGLE_BOX);
			ComplMenu.ClearDone();
			ComplMenu.Show();
			Show();
			int PrevPos = 0;

			while (!ComplMenu.Done()) {
				INPUT_RECORD ir;
				ComplMenu.ReadInput(&ir);
				if (!Opt.AutoComplete.ModalList) {
					int CurPos = ComplMenu.GetSelectPos();
					if (CurPos >= 0 && PrevPos != CurPos) {
						PrevPos = CurPos;
						SetString(CurPos ? ComplMenu.GetItemPtr(CurPos)->strName : strTemp);
						Show();
					}
				}
				if (ir.EventType == WINDOW_BUFFER_SIZE_EVENT) {
					SetMenuPos(ComplMenu);
					ComplMenu.Show();
				} else if (ir.EventType == KEY_EVENT || ir.EventType == FARMACRO_KEY_EVENT) {
					FarKey MenuKey = InputRecordToKey(&ir);

					// ввод
					if ((MenuKey >= FarKey(L' ') && MenuKey <= MAX_VKEY_CODE) || MenuKey == KEY_BS
							|| MenuKey == KEY_DEL || MenuKey == KEY_NUMDEL) {
						FARString strPrev;
						GetString(strPrev);
						DeleteBlock();
						ProcessKey(MenuKey);
						GetString(strTemp);
						if (StrCmp(strPrev, strTemp)) {
							ComplMenu.DeleteItems();
							PrevPos = 0;
							if (!strTemp.IsEmpty()) {
								PopulateCompletionMenu(ComplMenu, strTemp);
							}
							if (ComplMenu.GetItemCount() > 1
									|| (ComplMenu.GetItemCount() == 1
											&& StrCmpI(strTemp, ComplMenu.GetItemPtr(0)->strName))) {
								if (MenuKey != KEY_BS && MenuKey != KEY_DEL && MenuKey != KEY_NUMDEL
										&& Opt.AutoComplete.AppendCompletion) {
									int SelStart = GetLength();

									// magic
									if (IsSlash(Str[SelStart - 1]) && Str[SelStart - 2] == L'"'
											&& IsSlash(ComplMenu.GetItemPtr(0)->strName.At(SelStart - 2))) {
										Str[SelStart - 2] = Str[SelStart - 1];
										Str.Truncate(Str.Size() - 1);// StrSize--; NB: originally NUL char wasnt enforced after truncation
										SelStart--;
										my->CurPos--;
									}

									PauseEditListener pel(*this);
									InsertString(ComplMenu.GetItemPtr(0)->strName.SubStr(SelStart));
									if (X2 - X1 > GetLength())
										SetLeftPos(0);
									Select(SelStart, GetLength());
								}
								ComplMenu.AddItem(&EmptyItem, 0);
								SetMenuPos(ComplMenu);
								ComplMenu.SetSelectPos(0, 0);
								ComplMenu.Redraw();
							} else {
								ComplMenu.SetExitCode(-1);
							}
							Show();
						}
					} else {
						switch (MenuKey) {
							// "классический" перебор
							case KEY_CTRLEND: {
								ComplMenu.ProcessKey(KEY_DOWN);
								break;
							}

							// навигация по строке ввода
							case KEY_LEFT:
							case KEY_NUMPAD4:
							case KEY_CTRLS:
							case KEY_RIGHT:
							case KEY_NUMPAD6:
							case KEY_CTRLD:
							case KEY_CTRLLEFT:
							case KEY_CTRLRIGHT:
							case KEY_CTRLHOME: {
								if (MenuKey == KEY_LEFT || MenuKey == KEY_NUMPAD4) {
									MenuKey = KEY_CTRLS;
								} else if (MenuKey == KEY_RIGHT || MenuKey == KEY_NUMPAD6) {
									MenuKey = KEY_CTRLD;
								}
								pOwner->ProcessKey(MenuKey);
								break;
							}

							// навигация по списку
							case KEY_HOME:
							case KEY_NUMPAD7:
							case KEY_END:
							case KEY_NUMPAD1:
							case KEY_IDLE:
							case KEY_NONE:
							case KEY_ESC:
							case KEY_F10:
							case KEY_ALTF9:
							case KEY_UP:
							case KEY_NUMPAD8:
							case KEY_DOWN:
							case KEY_NUMPAD2:
							case KEY_PGUP:
							case KEY_NUMPAD9:
							case KEY_PGDN:
							case KEY_NUMPAD3:
							case KEY_ALTLEFT:
							case KEY_ALTRIGHT:
							case KEY_ALTHOME:
							case KEY_ALTEND:
							case KEY_MSWHEEL_UP:
							case KEY_MSWHEEL_DOWN:
							case KEY_MSWHEEL_LEFT:
							case KEY_MSWHEEL_RIGHT: {
								ComplMenu.ProcessInput();
								break;
							}

							case KEY_SHIFTNUMDEL:
							case KEY_SHIFTDEL: {
								RemoveSelectedCompletionMenuItem(ComplMenu);
								PrevPos = -1;	// force edit's content update on next iteration
								break;
							}

							case KEY_ENTER:
							case KEY_NUMENTER: {
								if (Opt.AutoComplete.ModalList) {
									ComplMenu.ProcessInput();
									break;
								}
								[[fallthrough]];
							}

							// всё остальное закрывает список и идёт владельцу
							default: {
								ComplMenu.Hide();
								ComplMenu.SetExitCode(-1);
								BackKey = MenuKey;
								Result = true;
							}
						}
					}
				} else {
					ComplMenu.ProcessInput();
				}
			}
			if (Opt.AutoComplete.ModalList) {
				int ExitCode = ComplMenu.GetExitCode();
				if (ExitCode > 0) {
					SetString(ComplMenu.GetItemPtr(ExitCode)->strName);
				}
			}
		}
	}
}

bool EditControl::AutoCompleteProc(bool Manual, bool DelBlock, FarKey &BackKey)
{
	bool Result = false;
	static int Reenter = 0;

	if (ECFlags.Check(EC_ENABLEAUTOCOMPLETE) && Str.Size() > 0 && !Reenter
			&& (CtrlObject->Macro.GetCurRecord(nullptr, nullptr) == MACROMODE_NOMACRO || Manual)) {
		Reenter++;
		AutoCompleteProcMenu(Result, Manual, DelBlock, BackKey);
		Reenter--;
	}
	return Result;
}

void EditControl::AutoComplete(bool Manual, bool DelBlock)
{
	FarKey Key = 0;
	if (AutoCompleteProc(Manual, DelBlock, Key)) {
		// BUGBUG, hack
		const auto Wait = WaitInMainLoop;
		WaitInMainLoop = 1;
		if (!CtrlObject->Macro.ProcessKey(Key))
			pOwner->ProcessKey(Key);
		WaitInMainLoop = Wait;
		Show();
	}
}

int EditControl::ProcessKey(FarKey Key)
{
	MyEcoLazy::Use my(fields);
	int ret_code = Edit::ProcessKey(Key);
	if ( ret_code && OverflowArrowsColor > 0 && !Recurse) {
		if (RealPosToCell(Str.Size()) > my->LeftPos + X2 - X1 && RealPosToCell(my->CurPos) == my->LeftPos + X2 - X1) {
			my->CurPos = CalcPosFwd();
			Edit::ProcessKey(KEY_LEFT);
		}

		if (my->LeftPos > 0 && my->CurPos == my->LeftPos) {
			my->CurPos = CalcPosBwd();
			Edit::ProcessKey(KEY_RIGHT);
		}
	}
	return ret_code;
}
int EditControl::ProcessMouse(MOUSE_EVENT_RECORD *MouseEvent)
{
	MyEcoLazy::Use my(fields);
	if (Edit::ProcessMouse(MouseEvent)) {
		while (IsMouseButtonPressed() == FROM_LEFT_1ST_BUTTON_PRESSED) {
			Flags.Clear(FEDITLINE_CLEARFLAG);
			SetCellCurPos(MouseX - X1 + my->LeftPos);
			if (MouseEventFlags & MOUSE_MOVED) {
				if (!Selection) {
					Selection = true;
					SelectionStart = -1;
					Select(SelectionStart, 0);
				} else {
					if (SelectionStart == -1) {
						SelectionStart = my->CurPos;
					}
					Select(Min(SelectionStart, my->CurPos), Min(Str.Size(), Max(SelectionStart, my->CurPos)));
					Show();
				}
			}
		}
		Selection = false;

		if (OverflowArrowsColor > 0) {
			if (RealPosToCell(Str.Size()) > my->LeftPos + X2 - X1 && RealPosToCell(my->CurPos) == my->LeftPos + X2 - X1) {
				ProcessKey(KEY_RIGHT);
			}

			if (my->LeftPos > 0 && my->CurPos == my->LeftPos) {
				ProcessKey(KEY_LEFT);
			}
		}
		return TRUE;
	}
	return FALSE;
}

void EditControl::EnableAC(bool Permanent)
{
	ACState = Permanent ? true : ECFlags.Check(EC_ENABLEAUTOCOMPLETE) != FALSE;
	ECFlags.Set(EC_ENABLEAUTOCOMPLETE);
}

void EditControl::DisableAC(bool Permanent)
{
	ACState = Permanent ? false : ECFlags.Check(EC_ENABLEAUTOCOMPLETE) != FALSE;
	ECFlags.Clear(EC_ENABLEAUTOCOMPLETE);
}

void EditControl::ShowCustomCompletionList(const std::vector<std::string> &list)
{
	pCustomCompletionList = &list;
	AutoComplete(true, false);
	pCustomCompletionList = nullptr;
}
