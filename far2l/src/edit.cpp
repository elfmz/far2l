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
#include "palette.hpp"
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

static int Recurse = 0;

enum
{
	EOL_NONE,
	EOL_CR,
	EOL_LF,
	EOL_CRLF,
	EOL_CRCRLF
};
static const wchar_t *EOL_TYPE_CHARS[] = {L"", L"\r", L"\n", L"\r\n", L"\r\r\n"};

#define EDMASK_ANY    L'X'		// позволяет вводить в строку ввода любой символ;
#define EDMASK_DSS    L'#'		// позволяет вводить в строку ввода цифры, пробел и знак минуса;
#define EDMASK_DIGIT  L'9'		// позволяет вводить в строку ввода только цифры;
#define EDMASK_DIGITS L'N'		// позволяет вводить в строку ввода только цифры и пробелы;
#define EDMASK_ALPHA  L'A'		// позволяет вводить в строку ввода только буквы.
#define EDMASK_HEX    L'H'		// позволяет вводить в строку ввода шестнадцатиричные символы.

class DisableCallback
{
	bool OldState;
	bool *CurState;

public:
	DisableCallback(bool &State)
	{
		OldState = State;
		CurState = &State;
		State = false;
	}
	void Restore() { *CurState = OldState; }
	~DisableCallback() { Restore(); }
};

Edit::Edit(ScreenObject *pOwner, Callback *aCallback, bool bAllocateData)
	:
	m_next(nullptr),
	m_prev(nullptr),
	Str(bAllocateData ? reinterpret_cast<wchar_t *>(malloc(sizeof(wchar_t))) : nullptr),
	StrSize(0),
	MaxLength(-1),
	Mask(nullptr),
	LeftPos(0),
	CurPos(0),
	PrevCurPos(0),
	MSelStart(-1),
	SelStart(-1),
	SelEnd(0),
	CursorSize(-1),
	CursorPos(0)
{
	m_Callback.Active = true;
	m_Callback.m_Callback = nullptr;
	m_Callback.m_Param = nullptr;

	if (aCallback)
		m_Callback = *aCallback;

	SetOwner(pOwner);
	SetWordDiv(Opt.strWordDiv);

	if (bAllocateData)
		*Str = 0;

	Flags.Set(FEDITLINE_EDITBEYONDEND);
	Color = F_LIGHTGRAY | B_BLACK;
	SelColor = F_WHITE | B_BLACK;
	ColorUnChanged = FarColorToReal(COL_DIALOGEDITUNCHANGED);
	EndType = EOL_NONE;
	TabSize = Opt.EdOpt.TabSize;
	TabExpandMode = EXPAND_NOTABS;
	Flags.Change(FEDITLINE_DELREMOVESBLOCKS, Opt.EdOpt.DelRemovesBlocks);
	Flags.Change(FEDITLINE_PERSISTENTBLOCKS, Opt.EdOpt.PersistentBlocks);
	Flags.Change(FEDITLINE_SHOWWHITESPACE, Opt.EdOpt.ShowWhiteSpace);
	m_codepage = 0;		// BUGBUG
}

Edit::~Edit()
{
	if (Mask)
		free(Mask);

	if (Str)
		free(Str);
}

DWORD Edit::SetCodePage(UINT codepage)
{
	DWORD Ret = SETCP_NOERROR;
	DWORD wc2mbFlags = WC_NO_BEST_FIT_CHARS;
	BOOL UsedDefaultChar = FALSE;
	LPBOOL lpUsedDefaultChar = &UsedDefaultChar;

	if (m_codepage == CP_UTF7 || m_codepage == CP_UTF8 || m_codepage == CP_UTF16LE
			|| m_codepage == CP_UTF16BE)	// BUGBUG: CP_SYMBOL, 50xxx, 57xxx too
	{
		wc2mbFlags = 0;
		lpUsedDefaultChar = nullptr;
	}

	DWORD mb2wcFlags = MB_ERR_INVALID_CHARS;

	if (codepage == CP_UTF7)	// BUGBUG: CP_SYMBOL, 50xxx, 57xxx too
	{
		mb2wcFlags = 0;
	}

	if (codepage != m_codepage) {
		if (Str && *Str) {
			// m_codepage = codepage;
			int length = WINPORT(WideCharToMultiByte)(m_codepage, wc2mbFlags, Str, StrSize, nullptr, 0,
					nullptr, lpUsedDefaultChar);

			if (UsedDefaultChar)
				Ret|= SETCP_WC2MBERROR;

			char *decoded = (char *)malloc(length);

			if (!decoded) {
				Ret|= SETCP_OTHERERROR;
				return Ret;
			}

			WINPORT(WideCharToMultiByte)(m_codepage, 0, Str, StrSize, decoded, length, nullptr, nullptr);
			int length2 = WINPORT(MultiByteToWideChar)(codepage, mb2wcFlags, decoded, length, nullptr, 0);

			if (!length2 && WINPORT(GetLastError)() == ERROR_NO_UNICODE_TRANSLATION) {
				Ret|= SETCP_MB2WCERROR;
				length2 = WINPORT(MultiByteToWideChar)(codepage, 0, decoded, length, nullptr, 0);
			}

			wchar_t *encoded = (wchar_t *)malloc((length2 + 1) * sizeof(wchar_t));

			if (!encoded) {
				free(decoded);
				Ret|= SETCP_OTHERERROR;
				return Ret;
			}

			length2 = WINPORT(MultiByteToWideChar)(codepage, 0, decoded, length, encoded, length2);
			encoded[length2] = L'\0';
			free(decoded);
			free(Str);
			Str = encoded;
			StrSize = length2;
		}

		m_codepage = codepage;
		Changed();
	}

	return Ret;
}

UINT Edit::GetCodePage()
{
	return m_codepage;
}

void Edit::DisplayObject()
{
	if (Flags.Check(FEDITLINE_DROPDOWNBOX)) {
		Flags.Clear(FEDITLINE_CLEARFLAG);	// при дроп-даун нам не нужно никакого unchanged text
		SelStart = 0;
		SelEnd = StrSize;					// а также считаем что все выделено -
											// надо же отличаться от обычных Edit
	}

	// Вычисление нового положения курсора в строке с учётом Mask.
	int Value = (PrevCurPos > CurPos) ? -1 : 1;
	CurPos = GetNextCursorPos(CurPos, Value);
	FastShow();

	/*
		$ 26.07.2000 tran
		при DropDownBox курсор выключаем
		не знаю даже - попробовал но не очень красиво вышло
	*/
	if (Flags.Check(FEDITLINE_DROPDOWNBOX))
		::SetCursorType(0, 10);
	else {
		if (Flags.Check(FEDITLINE_OVERTYPE)) {
			int NewCursorSize = (Opt.CursorSize[2] ? Opt.CursorSize[2] : 99);
			::SetCursorType(1, CursorSize == -1 ? NewCursorSize : CursorSize);
		} else {
			int NewCursorSize = (Opt.CursorSize[0] ? Opt.CursorSize[0] : 10);
			::SetCursorType(1, CursorSize == -1 ? NewCursorSize : CursorSize);
		}
	}

	MoveCursor(X1 + CursorPos - LeftPos, Y1);
}

void Edit::SetCursorType(bool Visible, DWORD Size)
{
	Flags.Change(FEDITLINE_CURSORVISIBLE, Visible);
	CursorSize = Size;
	::SetCursorType(Visible, Size);
}

void Edit::GetCursorType(bool &Visible, DWORD &Size)
{
	Visible = Flags.Check(FEDITLINE_CURSORVISIBLE) != FALSE;
	Size = CursorSize;
}

// Вычисление нового положения курсора в строке с учётом Mask.
int Edit::GetNextCursorPos(int Position, int Where)
{
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
	int EditLength = ObjWidth;

	if (!Flags.Check(FEDITLINE_EDITBEYONDEND) && CurPos > StrSize && StrSize >= 0)
		CurPos = StrSize;

	if (MaxLength != -1) {
		if (StrSize > MaxLength) {
			Str[MaxLength] = 0;
			StrSize = MaxLength;
		}

		if (CurPos > MaxLength - 1)
			CurPos = MaxLength > 0 ? (MaxLength - 1) : 0;
	}

	int CellCurPos = GetCellCurPos();

	/*
		$ 31.07.2001 KM
		! Для комбобокса сделаем отображение строки
		с первой позиции.
	*/
	int RealLeftPos = -1;
	if (!Flags.Check(FEDITLINE_DROPDOWNBOX)) {
		if (CellCurPos - LeftPos > EditLength - 1) {
			/*
				tricky left pos shifting to
				- avoid LeftPos pointing into middle of full-width char cells pair
				- ensure RealLeftPos really shifted in case string starts by some long character
			*/
			for (int ShiftBy = 1; ShiftBy <= std::max(TabSize, 2); ++ShiftBy) {
				RealLeftPos = CellPosToReal(CellCurPos - EditLength + ShiftBy);
				int NewLeftPos = RealPosToCell(RealLeftPos);
				if (LeftPos != NewLeftPos) {
					LeftPos = NewLeftPos;
					break;
				}
			}
		}

		if (CellCurPos < LeftPos)
			LeftPos = CellCurPos;
	}

	if (RealLeftPos == -1)
		RealLeftPos = CellPosToReal(LeftPos);

	GotoXY(X1, Y1);
	int CellSelStart = (SelStart == -1) ? -1 : RealPosToCell(SelStart);
	int CellSelEnd = (SelEnd < 0) ? -1 : RealPosToCell(SelEnd);

	/*
		$ 17.08.2000 KM
		Если есть маска, сделаем подготовку строки, то есть
		все "постоянные" символы в маске, не являющиеся шаблонными
		должны постоянно присутствовать в Str
	*/
	if (Mask && *Mask)
		RefreshStrByMask();

	CursorPos = CellCurPos;

	OutStr.clear();
	size_t OutStrCells = 0;
	for (int i = RealLeftPos; i < StrSize && int(OutStrCells) < EditLength; ++i) {
		auto wc = Str[i];
		if (wc == L' ' && Flags.Check(FEDITLINE_SHOWWHITESPACE) && Flags.Check(FEDITLINE_EDITORMODE)) {
			wc = L'\xB7';
		}

		if (wc == L'\t') {
			for (int j = 0, S = TabSize - ((LeftPos + OutStrCells) % TabSize);
					j < S && int(OutStrCells) < EditLength; ++j, ++OutStrCells) {
				OutStr.emplace_back(
						(Flags.Check(FEDITLINE_SHOWWHITESPACE) && Flags.Check(FEDITLINE_EDITORMODE) && !j)
								? L'\x2192'
								: L' ');
			}
		} else {
			if (IsCharFullWidth(wc)) {
				if (int(OutStrCells + 2) > EditLength) {
					OutStr.emplace_back(L' ');
					OutStrCells++;
					break;
				}
				OutStrCells+= 2;
			} else if (!IsCharXxxfix(wc) || i == RealLeftPos)
				OutStrCells++;

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
		if ((CellSelStart-= LeftPos) < 0)
			CellSelStart = 0;

		int AllString = (CellSelEnd == -1);

		if (AllString)
			CellSelEnd = EditLength;
		else if ((CellSelEnd-= LeftPos) < 0)
			CellSelEnd = 0;

		for (; int(OutStrCells) < EditLength; ++OutStrCells) {
			OutStr.emplace(OutStr.begin() + OutStr.size() - 1, L' ');
		}

		/*
			$ 24.08.2000 SVS
			! У DropDowList`а выделение по полной программе - на всю видимую длину
			ДАЖЕ ЕСЛИ ПУСТАЯ СТРОКА
		*/
		if (CellSelStart >= EditLength	/*|| !AllString && CellSelStart>=StrSize*/
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
			LeftPos = 0;
			SetString(L"");
		}

		if (PrevSelStart != -1) {
			SelStart = PrevSelStart;
			SelEnd = PrevSelEnd;
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
	switch (OpCode) {
		case MCODE_C_EMPTY:
			return (int64_t)!GetLength();
		case MCODE_C_SELECTED:
			return (int64_t)(SelStart != -1 && SelStart < SelEnd);
		case MCODE_C_EOF:
			return (int64_t)(CurPos >= StrSize);
		case MCODE_C_BOF:
			return (int64_t)!CurPos;
		case MCODE_V_ITEMCOUNT:
			return (int64_t)StrSize;
		case MCODE_V_CURPOS:
			return (int64_t)(CurPos + 1);
		case MCODE_F_EDITOR_SEL: {
			int Action = (int)((INT_PTR)vParam);

			switch (Action) {
				case 0:		// Get Param
				{
					switch (iParam) {
						case 0:		// return FirstLine
						case 2:		// return LastLine
							return IsSelection() ? 1 : 0;
						case 1:		// return FirstPos
							return IsSelection() ? SelStart + 1 : 0;
						case 3:		// return LastPos
							return IsSelection() ? SelEnd : 0;
						case 4:		// return block type (0=nothing 1=stream, 2=column)
							return IsSelection() ? 1 : 0;
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
								SetCellCurPos(iParam ? SelEnd : SelStart);
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
							MSelStart = GetCurPos();
							return 1;
						}
						case 1:		// selection finish
						{
							if (MSelStart != -1) {
								if (MSelStart != GetCurPos())
									Select(MSelStart, GetCurPos());
								else
									Select(-1, 0);

								Show();
								MSelStart = -1;
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
					MSelStart = -1;
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
	int TrimmedStrSize = StrSize;
	while (TrimmedStrSize > 0 && (IsSpace(Str[TrimmedStrSize - 1]) || IsEol(Str[TrimmedStrSize - 1]))) {
		--TrimmedStrSize;
	}
	return TrimmedStrSize;
}

int Edit::CalcPosFwdTo(int Pos, int LimitPos) const
{
	if (LimitPos != -1) {
		if (Pos < LimitPos)
			do {
				Pos++;
			} while (Pos < LimitPos && Pos < StrSize && IsCharXxxfix(Str[Pos]));
	} else
		do {
			Pos++;
		} while (Pos < StrSize && IsCharXxxfix(Str[Pos]));

	return Pos;
}

int Edit::CalcPosBwdTo(int Pos) const
{
	if (Pos <= 0)
		return 0;

	do {
		--Pos;
	} while (Pos > 0 && IsCharXxxfix(Str[Pos]));

	return Pos;
}

int Edit::ProcessKey(FarKey Key)
{
	switch (Key) {
		case KEY_ADD:
			Key = L'+';
			break;
		case KEY_SUBTRACT:
			Key = L'-';
			break;
		case KEY_MULTIPLY:
			Key = L'*';
			break;
		case KEY_DIVIDE:
			Key = L'/';
			break;
		case KEY_DECIMAL:
			Key = L'.';
			break;
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
			&& !Flags.Check(FEDITLINE_EDITORMODE) && SelStart != -1 && SelStart < SelEnd) {
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
			if (SelStart != -1 || SelEnd) {
				PrevSelStart = SelStart;
				PrevSelEnd = SelEnd;
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
			&& Flags.Check(FEDITLINE_CLEARFLAG) && CurPos >= StrSize)
		Key = KEY_CTRLY;

	/*
		$ 15.09.2000 SVS
		Bug - Выделяем кусочек строки -> Shift-Del удяляет всю строку
		Так должно быть только для UnChanged состояния
	*/
	if ((Key == KEY_SHIFTDEL || Key == KEY_SHIFTNUMDEL || Key == KEY_SHIFTDECIMAL)
			&& Flags.Check(FEDITLINE_CLEARFLAG) && CurPos >= StrSize && SelStart == -1) {
		SelStart = 0;
		SelEnd = StrSize;
	}

	if (Flags.Check(FEDITLINE_CLEARFLAG)
			&& ((Key <= 0xFFFF && Key != KEY_BS) || Key == KEY_CTRLBRACKET || Key == KEY_CTRLBACKBRACKET
					|| Key == KEY_CTRLSHIFTBRACKET || Key == KEY_CTRLSHIFTBACKBRACKET || Key == KEY_SHIFTENTER
					|| Key == KEY_SHIFTNUMENTER)) {
		LeftPos = 0;
		SetString(L"");
		Show();
	}

	// Здесь - вызов функции вставки путей/файлов
	if (ProcessInsPath(Key, PrevSelStart, PrevSelEnd)) {
		Show();
		return TRUE;
	}

	if (Key != KEY_NONE && Key != KEY_IDLE && Key != KEY_SHIFTINS && Key != KEY_SHIFTNUMPAD0 && Key != KEY_CTRLINS
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
		case KEY_SHIFTLEFT:
		case KEY_SHIFTNUMPAD4: {
			if (CurPos > 0) {
				RecurseProcessKey(KEY_LEFT);

				if (!Flags.Check(FEDITLINE_MARKINGBLOCK)) {
					Select(-1, 0);
					Flags.Set(FEDITLINE_MARKINGBLOCK);
				}

				if (SelStart != -1 && SelStart <= CurPos)
					Select(SelStart, CurPos);
				else {
					int EndPos = CalcPosFwd((Mask && *Mask) ? CalcRTrimmedStrSize() : -1);
					int NewStartPos = CurPos;

					if (EndPos > StrSize)
						EndPos = StrSize;

					if (NewStartPos > StrSize)
						NewStartPos = StrSize;

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

			if ((SelStart != -1 && SelEnd == -1) || SelEnd > CurPos) {
				if (CalcPosFwd() == SelEnd)
					Select(-1, 0);
				else
					Select(CalcPosFwd(), SelEnd);
			} else
				AddSelect(CurPos, CalcPosFwd());

			RecurseProcessKey(KEY_RIGHT);
			return TRUE;
		}
		case KEY_CTRLSHIFTLEFT:
		case KEY_CTRLSHIFTNUMPAD4: {
			if (CurPos > StrSize) {
				PrevCurPos = CurPos;
				CurPos = StrSize;
			}

			if (CurPos > 0)
				RecurseProcessKey(KEY_SHIFTLEFT);

			while (CurPos > 0
					&& !(!IsWordDiv(WordDiv(), Str[CurPos]) && IsWordDiv(WordDiv(), Str[CurPos - 1])
							&& !IsSpace(Str[CurPos]))) {
				if (!IsSpace(Str[CurPos])
						&& (IsSpace(Str[CurPos - 1]) || IsWordDiv(WordDiv(), Str[CurPos - 1])))
					break;

				RecurseProcessKey(KEY_SHIFTLEFT);
			}

			Show();
			return TRUE;
		}
		case KEY_CTRLSHIFTRIGHT:
		case KEY_CTRLSHIFTNUMPAD6: {
			if (CurPos >= StrSize)
				return FALSE;

			RecurseProcessKey(KEY_SHIFTRIGHT);

			while (CurPos < StrSize
					&& !(IsWordDiv(WordDiv(), Str[CurPos]) && !IsWordDiv(WordDiv(), Str[CurPos - 1]))) {
				if (!IsSpace(Str[CurPos])
						&& (IsSpace(Str[CurPos - 1]) || IsWordDiv(WordDiv(), Str[CurPos - 1])))
					break;

				RecurseProcessKey(KEY_SHIFTRIGHT);

				if (MaxLength != -1 && CurPos == MaxLength - 1)
					break;
			}

			Show();
			return TRUE;
		}
		case KEY_SHIFTHOME:
		case KEY_SHIFTNUMPAD7: {
			Lock();

			while (CurPos > 0)
				RecurseProcessKey(KEY_SHIFTLEFT);

			Unlock();
			Show();
			return TRUE;
		}
		case KEY_SHIFTEND:
		case KEY_SHIFTNUMPAD1: {
			Lock();
			int Len = (Mask && *Mask) ? CalcRTrimmedStrSize() : StrSize;

			int LastCurPos = CurPos;

			while (CurPos < Len /*StrSize*/) {
				RecurseProcessKey(KEY_SHIFTRIGHT);

				if (LastCurPos == CurPos)
					break;

				LastCurPos = CurPos;
			}

			Unlock();
			Show();
			return TRUE;
		}
		case KEY_BS: {
			if (CurPos <= 0)
				return FALSE;

			PrevCurPos = CurPos;
			CurPos = CalcPosBwd();

			while (LeftPos > 0 && RealPosToCell(CurPos) <= LeftPos) {
				LeftPos-= 15;
				if (LeftPos > 0)
					LeftPos = RealPosToCell(CellPosToReal(LeftPos));
				else
					LeftPos = 0;
			}

			if (!RecurseProcessKey(KEY_DEL))
				Show();

			return TRUE;
		}
		case KEY_CTRLSHIFTBS: {
			DisableCallback DC(m_Callback.Active);

			// BUGBUG
			for (int i = CurPos; i >= 0; i--) {
				RecurseProcessKey(KEY_BS);
			}
			DC.Restore();
			Changed(true);
			Show();
			return TRUE;
		}
		case KEY_CTRLBS: {
			if (CurPos > StrSize) {
				PrevCurPos = CurPos;
				CurPos = StrSize;
			}

			Lock();

			DisableCallback DC(m_Callback.Active);

			// BUGBUG
			for (;;) {
				int StopDelete = FALSE;

				if (CurPos > 1 && IsSpace(Str[CurPos - 1]) != IsSpace(Str[CurPos - 2]))
					StopDelete = TRUE;

				RecurseProcessKey(KEY_BS);

				if (!CurPos || StopDelete)
					break;

				if (IsWordDiv(WordDiv(), Str[CurPos - 1]))
					break;
			}

			Unlock();
			DC.Restore();
			Changed(true);
			Show();
			return TRUE;
		}
		case KEY_CTRLQ: {
			Lock();

			if (!Flags.Check(FEDITLINE_PERSISTENTBLOCKS)
					&& (SelStart != -1 || Flags.Check(FEDITLINE_CLEARFLAG)))
				RecurseProcessKey(KEY_DEL);

			ProcessCtrlQ();
			Unlock();
			Show();
			return TRUE;
		}
		case KEY_OP_SELWORD: {
			int OldCurPos = CurPos;
			PrevSelStart = SelStart;
			PrevSelEnd = SelEnd;
#if defined(MOUSEKEY)

			if (CurPos >= SelStart && CurPos <= SelEnd) {	// выделяем ВСЮ строку при повторном двойном клике
				Select(0, StrSize);
			} else
#endif
			{
				int SStart, SEnd;

				if (CalcWordFromString(Str, CurPos, &SStart, &SEnd, WordDiv()))
					Select(SStart, SEnd + (SEnd < StrSize ? 1 : 0));
			}

			CurPos = OldCurPos;		// возвращаем обратно
			Show();
			return TRUE;
		}
		case KEY_OP_PLAINTEXT: {
			if (!Flags.Check(FEDITLINE_PERSISTENTBLOCKS)) {
				if (SelStart != -1 || Flags.Check(FEDITLINE_CLEARFLAG))		// BugZ#1053 - Неточности в $Text
					RecurseProcessKey(KEY_DEL);
			}

			const wchar_t *S = eStackAsString();

			ProcessInsPlainText(S);

			Show();
			return TRUE;
		}
		case KEY_CTRLT:
		case KEY_CTRLDEL:
		case KEY_CTRLNUMDEL:
		case KEY_CTRLDECIMAL: {
			if (CurPos >= StrSize)
				return FALSE;

			Lock();
			DisableCallback DC(m_Callback.Active);
			if (Mask && *Mask) {
				int MaskLen = StrLength(Mask);
				int ptr = CurPos;

				while (ptr < MaskLen) {
					ptr++;

					if (!CheckCharMask(Mask[ptr]) || (IsSpace(Str[ptr]) && !IsSpace(Str[ptr + 1]))
							|| (IsWordDiv(WordDiv(), Str[ptr])))
						break;
				}

				// BUGBUG
				for (int i = 0; i < ptr - CurPos; i++)
					RecurseProcessKey(KEY_DEL);
			} else {
				for (;;) {
					int StopDelete = FALSE;

					if (CurPos < StrSize - 1 && IsSpace(Str[CurPos]) && !IsSpace(Str[CurPos + 1]))
						StopDelete = TRUE;

					RecurseProcessKey(KEY_DEL);

					if (CurPos >= StrSize || StopDelete)
						break;

					if (IsWordDiv(WordDiv(), Str[CurPos]))
						break;
				}
			}

			Unlock();
			DC.Restore();
			Changed(true);
			Show();
			return TRUE;
		}
		case KEY_CTRLY: {
			if (Flags.Check(FEDITLINE_READONLY | FEDITLINE_DROPDOWNBOX))
				return (TRUE);

			PrevCurPos = CurPos;
			LeftPos = CurPos = 0;
			*Str = 0;
			StrSize = 0;
			Str = (wchar_t *)realloc(Str, 1 * sizeof(wchar_t));
			Select(-1, 0);
			Changed();
			Show();
			return TRUE;
		}
		case KEY_CTRLK: {
			if (Flags.Check(FEDITLINE_READONLY | FEDITLINE_DROPDOWNBOX))
				return (TRUE);

			if (CurPos >= StrSize)
				return FALSE;

			if (!Flags.Check(FEDITLINE_EDITBEYONDEND)) {
				if (CurPos < SelEnd)
					SelEnd = CurPos;

				if (SelEnd < SelStart && SelEnd != -1) {
					SelEnd = 0;
					SelStart = -1;
				}
			}

			Str[CurPos] = 0;
			StrSize = CurPos;
			Str = (wchar_t *)realloc(Str, (StrSize + 1) * sizeof(wchar_t));
			Changed();
			Show();
			return TRUE;
		}
		case KEY_HOME:
		case KEY_NUMPAD7:
		case KEY_CTRLHOME:
		case KEY_CTRLNUMPAD7: {
			PrevCurPos = CurPos;
			CurPos = 0;
			Show();
			return TRUE;
		}
		case KEY_END:
		case KEY_NUMPAD1:
		case KEY_CTRLEND:
		case KEY_CTRLNUMPAD1:
		case KEY_CTRLSHIFTEND:
		case KEY_CTRLSHIFTNUMPAD1: {
			PrevCurPos = CurPos;
			CurPos = (Mask && *Mask) ? CalcRTrimmedStrSize() : StrSize;
			Show();
			return TRUE;
		}
		case KEY_LEFT:
		case KEY_NUMPAD4:
		case KEY_MSWHEEL_LEFT:
		case KEY_CTRLS: {
			if (CurPos > 0) {
				PrevCurPos = CurPos;
				CurPos = CalcPosBwd();
				Show();
			}

			return TRUE;
		}
		case KEY_RIGHT:
		case KEY_NUMPAD6:
		case KEY_MSWHEEL_RIGHT:
		case KEY_CTRLD: {
			PrevCurPos = CurPos;
			CurPos = CalcPosFwd((Mask && *Mask) ? CalcRTrimmedStrSize() : -1);
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

			if (CurPos >= StrSize)
				return FALSE;

			if (SelStart != -1) {
				if (SelEnd != -1 && CurPos < SelEnd)
					SelEnd--;

				if (CurPos < SelStart)
					SelStart--;

				if (SelEnd != -1 && SelEnd <= SelStart) {
					SelStart = -1;
					SelEnd = 0;
				}
			}

			if (Mask && *Mask) {
				Str[CurPos] = L' ';
			} else {
				auto NextPos = CalcPosFwd();
				if (NextPos > CurPos) {
					wmemmove(Str + CurPos, Str + NextPos, (StrSize - NextPos) + 1);
					StrSize-= (NextPos - CurPos);
					wchar_t *NewStr = (wchar_t *)realloc(Str, (StrSize + 1) * sizeof(wchar_t));
					if (NewStr) {
						Str = NewStr;
					}
				}
			}

			Changed(true);
			Show();
			return TRUE;
		}
		case KEY_CTRLLEFT:
		case KEY_CTRLNUMPAD4: {
			PrevCurPos = CurPos;

			if (CurPos > StrSize)
				CurPos = StrSize;

			CurPos = CalcPosBwd();

			while (CurPos > 0
					&& !(!IsWordDiv(WordDiv(), Str[CurPos]) && IsWordDiv(WordDiv(), Str[CurPos - 1])
							&& !IsSpace(Str[CurPos]))) {
				if (!IsSpace(Str[CurPos]) && IsSpace(Str[CurPos - 1]))
					break;

				CurPos--;
			}

			Show();
			return TRUE;
		}
		case KEY_CTRLRIGHT:
		case KEY_CTRLNUMPAD6: {
			if (CurPos >= StrSize)
				return FALSE;

			PrevCurPos = CurPos;
			int Len;

			if (Mask && *Mask) {
				Len = CalcRTrimmedStrSize();
				CurPos = CalcPosFwd(Len);
			} else {
				Len = StrSize;
				CurPos = CalcPosFwd();
			}

			while (CurPos < Len /*StrSize*/
					&& !(IsWordDiv(WordDiv(), Str[CurPos]) && !IsWordDiv(WordDiv(), Str[CurPos - 1]))) {
				if (!IsSpace(Str[CurPos]) && IsSpace(Str[CurPos - 1]))
					break;

				CurPos++;
			}

			Show();
			return TRUE;
		}
		case KEY_SHIFTNUMDEL:
		case KEY_SHIFTDECIMAL:
		case KEY_SHIFTDEL: {
			if (SelStart == -1 || SelStart >= SelEnd)
				return FALSE;

			RecurseProcessKey(KEY_CTRLINS);
			DeleteBlock();
			Show();
			return TRUE;
		}
		case KEY_CTRLINS:
		case KEY_CTRLNUMPAD0: {
			if (!Flags.Check(FEDITLINE_PASSWORDMODE)) {
				if (SelStart == -1 || SelStart >= SelEnd) {
					if (Mask && *Mask) {
						std::wstring TrimmedStr(Str, CalcRTrimmedStrSize());
						CopyToClipboard(TrimmedStr.c_str());
					} else {
						CopyToClipboard(Str);
					}
				} else if (SelEnd <= StrSize)		// TODO: если в начало условия добавить "StrSize &&", то пропадет баг "Ctrl-Ins в пустой строке очищает клипборд"
				{
					int Ch = Str[SelEnd];
					Str[SelEnd] = 0;
					CopyToClipboard(Str + SelStart);
					Str[SelEnd] = Ch;
				}
			}

			return TRUE;
		}
		case KEY_SHIFTINS:
		case KEY_SHIFTNUMPAD0: {
			wchar_t *ClipText = PasteFromClipboardEx(MaxLength);

			if (!ClipText)
				return TRUE;

			if (!Flags.Check(FEDITLINE_PERSISTENTBLOCKS)) {
				DisableCallback DC(m_Callback.Active);
				DeleteBlock();
			}

			for (int i = StrLength(Str) - 1; i >= 0 && IsEol(Str[i]); i--)
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
				LeftPos = 0;
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
			PrevCurPos = CurPos;
			CursorPos-= (CursorPos - 1) % TabSize + 1;

			if (CursorPos < 0)
				CursorPos = 0;	// CursorPos=0,TabSize=1 case

			SetCellCurPos(CursorPos);
			Show();
			return TRUE;
		}
		case KEY_SHIFTSPACE:
			Key = KEY_SPACE;
		default: {
			//			_D(SysLog(L"Key=0x%08X",Key));
			if (Key == KEY_ENTER || !IS_KEY_NORMAL(Key))	// KEY_NUMENTER,KEY_IDLE,KEY_NONE covered by !IS_KEY_NORMAL
				break;

			if (!Flags.Check(FEDITLINE_PERSISTENTBLOCKS)) {
				if (PrevSelStart != -1) {
					SelStart = PrevSelStart;
					SelEnd = PrevSelEnd;
				}
				DisableCallback DC(m_Callback.Active);
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
	return InsertKey(rec.Event.KeyEvent.uChar.AsciiChar);
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
	bool changed = false;
	wchar_t *NewStr;

	if (Flags.Check(FEDITLINE_READONLY | FEDITLINE_DROPDOWNBOX))
		return (TRUE);

	if (Key == KEY_TAB && Flags.Check(FEDITLINE_OVERTYPE)) {
		PrevCurPos = CurPos;
		CursorPos+= TabSize - (CursorPos % TabSize);
		SetCellCurPos(CursorPos);
		return TRUE;
	}

	if (Mask && *Mask) {
		int MaskLen = StrLength(Mask);

		if (CurPos < MaskLen) {
			if (KeyMatchedMask(Key)) {
				if (!Flags.Check(FEDITLINE_OVERTYPE)) {
					int i = MaskLen - 1;

					while (i > CurPos && !CheckCharMask(Mask[i]))
						i--;

					for (int j = i; i > CurPos; i--) {
						if (CheckCharMask(Mask[i])) {
							while (!CheckCharMask(Mask[j - 1])) {
								if (j <= CurPos)
									break;

								j--;
							}

							Str[i] = Str[j - 1];
							j--;
						}
					}
				}

				PrevCurPos = CurPos;
				Str[CurPos++] = Key;
				changed = true;
			} else {
				// Здесь вариант для "ввели символ из маски", например для SetAttr - ввесли '.'
				;	// char *Ptr=strchr(Mask+CurPos,Key);
			}
		} else if (CurPos < StrSize) {
			PrevCurPos = CurPos;
			Str[CurPos++] = Key;
			changed = true;
		}
	} else {
		if (MaxLength == -1 || StrSize < MaxLength) {
			if (CurPos >= StrSize) {
				if (!(NewStr = (wchar_t *)realloc(Str, (CurPos + 2) * sizeof(wchar_t))))
					return FALSE;

				Str = NewStr;
				wmemset(&Str[StrSize], L' ', CurPos + 1 - StrSize);
				Str[CurPos + 1] = 0;
				StrSize = CurPos + 1;

			} else if (!Flags.Check(FEDITLINE_OVERTYPE))
				StrSize++;

			if (Key == KEY_TAB && (TabExpandMode == EXPAND_NEWTABS || TabExpandMode == EXPAND_ALLTABS)) {
				StrSize--;
				InsertTab();
				return TRUE;
			}

			if (!(NewStr = (wchar_t *)realloc(Str, (StrSize + 1) * sizeof(wchar_t))))
				return TRUE;

			Str = NewStr;

			if (!Flags.Check(FEDITLINE_OVERTYPE)) {
				wmemmove(Str + CurPos + 1, Str + CurPos, StrSize - CurPos);

				if (SelStart != -1) {
					if (SelEnd != -1 && CurPos < SelEnd)
						SelEnd++;

					if (CurPos < SelStart)
						SelStart++;
				}
			}

			PrevCurPos = CurPos;
			Str[CurPos++] = Key;
			changed = true;
		} else if (Flags.Check(FEDITLINE_OVERTYPE)) {
			if (CurPos < StrSize) {
				PrevCurPos = CurPos;
				Str[CurPos++] = Key;
				changed = true;
			}
		}
		/*else
			MessageBeep(MB_ICONHAND);*/
	}

	Str[StrSize] = 0;

	if (changed)
		Changed();

	return TRUE;
}

void Edit::SetObjectColor(uint64_t Color, uint64_t SelColor, uint64_t ColorUnChanged)
{
	this->Color = Color;
	this->SelColor = SelColor;
	this->ColorUnChanged = ColorUnChanged;
}

void Edit::GetString(wchar_t *Str, int MaxSize)
{
	// far_wcsncpy(Str, this->Str,MaxSize);
	wmemmove(Str, this->Str, Min(StrSize, MaxSize - 1));
	Str[Min(StrSize, MaxSize - 1)] = 0;
	Str[MaxSize - 1] = 0;
}

void Edit::GetString(FARString &strStr)
{
	strStr = Str;
}

const wchar_t *Edit::GetStringAddr()
{
	return Str;
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

void Edit::SetEOL(const wchar_t *EOL)
{
	EndType = EOL_NONE;

	if (EOL && *EOL) {
		if (EOL[0] == L'\r')
			if (EOL[1] == L'\n')
				EndType = EOL_CRLF;
			else if (EOL[1] == L'\r' && EOL[2] == L'\n')
				EndType = EOL_CRCRLF;
			else
				EndType = EOL_CR;
		else if (EOL[0] == L'\n')
			EndType = EOL_LF;
	}
}

const wchar_t *Edit::GetEOL()
{
	return EOL_TYPE_CHARS[EndType];
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
	if (Flags.Check(FEDITLINE_READONLY))
		return;

	// коррекция вставляемого размера, если определен MaxLength
	if (MaxLength != -1 && Length > MaxLength) {
		Length = MaxLength;		// ??
	}

	if (Length > 0 && !Flags.Check(FEDITLINE_PARENT_SINGLELINE)) {
		if (Str[Length - 1] == L'\r') {
			EndType = EOL_CR;
			Length--;
		} else {
			if (Str[Length - 1] == L'\n') {
				Length--;

				if (Length > 0 && Str[Length - 1] == L'\r') {
					Length--;

					if (Length > 0 && Str[Length - 1] == L'\r') {
						Length--;
						EndType = EOL_CRCRLF;
					} else
						EndType = EOL_CRLF;
				} else
					EndType = EOL_LF;
			} else
				EndType = EOL_NONE;
		}
	}

	CurPos = 0;

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
				PrevCurPos = CurPos;
				CurPos++;
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
		wchar_t *NewStr = (wchar_t *)malloc((Length + 1) * sizeof(wchar_t));

		if (!NewStr)
			return;

		free(this->Str);

		this->Str = NewStr;
		StrSize = Length;
		wmemcpy(this->Str, Str, Length);
		this->Str[Length] = 0;

		if (TabExpandMode == EXPAND_ALLTABS)
			ExpandTabs();

		PrevCurPos = CurPos;
		CurPos = StrSize;
	}

	Changed();
}

void Edit::GetBinaryString(const wchar_t **Str, const wchar_t **EOL, int &Length)
{
	*Str = this->Str;

	if (EOL)
		*EOL = EOL_TYPE_CHARS[EndType];

	Length = StrSize;	//???
}

int Edit::GetSelString(wchar_t *Str, int MaxSize)
{
	if (SelStart == -1 || (SelEnd != -1 && SelEnd <= SelStart) || SelStart >= StrSize) {
		*Str = 0;
		return FALSE;
	}

	int CopyLength;

	if (SelEnd == -1)
		CopyLength = MaxSize;
	else
		CopyLength = Min(MaxSize, SelEnd - SelStart + 1);

	far_wcsncpy(Str, this->Str + SelStart, CopyLength);
	return TRUE;
}

int Edit::GetSelString(FARString &strStr)
{
	if (SelStart == -1 || (SelEnd != -1 && SelEnd <= SelStart) || SelStart >= StrSize) {
		strStr.Clear();
		return FALSE;
	}

	strStr.Copy(this->Str + SelStart, SelEnd - SelStart + 1);
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
	wchar_t *NewStr;

	if (Flags.Check(FEDITLINE_READONLY | FEDITLINE_DROPDOWNBOX))
		return;

	Flags.Clear(FEDITLINE_CLEARFLAG);

	if (Mask && *Mask) {
		int Pos = CurPos;
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
					PrevCurPos = CurPos;
					CurPos++;
				}

				i++;
			}
		}

		RefreshStrByMask();
		//_SVS(SysLog(L"InsertBinaryString ==> this->Str='%ls'",this->Str));
	} else {
		if (MaxLength != -1 && StrSize + Length > MaxLength) {
			// коррекция вставляемого размера, если определен MaxLength
			if (StrSize < MaxLength) {
				Length = MaxLength - StrSize;
			}
		}

		if (MaxLength == -1 || StrSize + Length <= MaxLength) {
			if (CurPos > StrSize) {
				if (!(NewStr = (wchar_t *)realloc(this->Str, (CurPos + 1) * sizeof(wchar_t))))
					return;

				this->Str = NewStr;
				wmemset(&this->Str[StrSize], L' ', CurPos - StrSize);
				this->Str[CurPos] = 0;
				StrSize = CurPos;
			}

			int TmpSize = StrSize - CurPos;
			wchar_t *TmpStr = new (std::nothrow) wchar_t[TmpSize + 16];

			if (!TmpStr)
				return;

			wmemcpy(TmpStr, &this->Str[CurPos], TmpSize);
			StrSize+= Length;

			if (!(NewStr = (wchar_t *)realloc(this->Str, (StrSize + 1) * sizeof(wchar_t)))) {
				delete[] TmpStr;
				return;
			}

			this->Str = NewStr;
			wmemcpy(&this->Str[CurPos], Str, Length);
			PrevCurPos = CurPos;
			CurPos+= Length;
			wmemcpy(this->Str + CurPos, TmpStr, TmpSize);
			this->Str[StrSize] = 0;
			delete[] TmpStr;

			if (TabExpandMode == EXPAND_ALLTABS)
				ExpandTabs();

			Changed();
		}
		/*else
			MessageBeep(MB_ICONHAND);*/
	}
}

int Edit::GetLength()
{
	return (StrSize);
}

// Функция установки маски ввода в объект Edit
void Edit::SetInputMask(const wchar_t *InputMask)
{
	if (Mask)
		free(Mask);

	if (InputMask && *InputMask) {
		if (!(Mask = wcsdup(InputMask)))
			return;

		RefreshStrByMask(TRUE);
	} else
		Mask = nullptr;
}

// Функция обновления состояния строки ввода по содержимому Mask
void Edit::RefreshStrByMask(int InitMode)
{
	if (Mask && *Mask) {
		int MaskLen = StrLength(Mask);

		if (StrSize != MaskLen) {
			wchar_t *NewStr = (wchar_t *)realloc(Str, (MaskLen + 1) * sizeof(wchar_t));

			if (!NewStr)
				return;

			Str = NewStr;

			for (int i = StrSize; i < MaskLen; i++)
				Str[i] = L' ';

			StrSize = MaxLength = MaskLen;
			Str[StrSize] = 0;
		}

		for (int i = 0; i < MaskLen; i++) {
			if (InitMode)
				Str[i] = L' ';

			if (!CheckCharMask(Mask[i]))
				Str[i] = Mask[i];
		}
	}
}

int Edit::ProcessMouse(MOUSE_EVENT_RECORD *MouseEvent)
{
	if (!(MouseEvent->dwButtonState & 3))
		return FALSE;

	if (MouseEvent->dwMousePosition.X < X1 || MouseEvent->dwMousePosition.X > X2
			|| MouseEvent->dwMousePosition.Y != Y1)
		return FALSE;

	// SetClearFlag(0); // пусть едитор сам заботится о снятии клеар-текста?
	SetCellCurPos(MouseEvent->dwMousePosition.X - X1 + LeftPos);

	if (!Flags.Check(FEDITLINE_PERSISTENTBLOCKS))
		Select(-1, 0);

	if (MouseEvent->dwButtonState & FROM_LEFT_1ST_BUTTON_PRESSED) {
		static int PrevDoubleClick = 0;
		static COORD PrevPosition = {0, 0};

		if (WINPORT(GetTickCount)() - PrevDoubleClick <= WINPORT(GetDoubleClickTime)()
				&& MouseEvent->dwEventFlags != MOUSE_MOVED && PrevPosition.X == MouseEvent->dwMousePosition.X
				&& PrevPosition.Y == MouseEvent->dwMousePosition.Y) {
			Select(0, StrSize);
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
int Edit::Search(const FARString &Str, FARString &ReplaceStr, int Position, int Case, int WholeWords,
		int Reverse, int Regexp, int *SearchLength)
{
	return SearchString(this->Str, this->StrSize, Str, ReplaceStr, CurPos, Position, Case, WholeWords,
			Reverse, Regexp, SearchLength, WordDiv());
}

void Edit::InsertTab()
{
	wchar_t *TabPtr;
	int Pos, S;

	if (Flags.Check(FEDITLINE_READONLY))
		return;

	Pos = CurPos;
	S = TabSize - (Pos % TabSize);

	if (SelStart != -1) {
		if (Pos <= SelStart) {
			SelStart+= S - (Pos == SelStart ? 0 : 1);
		}

		if (SelEnd != -1 && Pos < SelEnd) {
			SelEnd+= S;
		}
	}

	int PrevStrSize = StrSize;
	StrSize+= S;
	CurPos+= S;
	Str = (wchar_t *)realloc(Str, (StrSize + 1) * sizeof(wchar_t));
	TabPtr = Str + Pos;
	wmemmove(TabPtr + S, TabPtr, PrevStrSize - Pos);
	wmemset(TabPtr, L' ', S);
	Str[StrSize] = 0;
	Changed();
}

void Edit::ExpandTabs()
{
	wchar_t *TabPtr;
	int Pos = 0, S;

	if (Flags.Check(FEDITLINE_READONLY))
		return;

	bool changed = false;

	while ((TabPtr = (wchar_t *)wmemchr(Str + Pos, L'\t', StrSize - Pos))) {
		changed = true;
		Pos = (int)(TabPtr - Str);
		S = TabSize - ((int)(TabPtr - Str) % TabSize);

		if (SelStart != -1) {
			if (Pos <= SelStart) {
				SelStart+= S - (Pos == SelStart ? 0 : 1);
			}

			if (SelEnd != -1 && Pos < SelEnd) {
				SelEnd+= S - 1;
			}
		}

		int PrevStrSize = StrSize;
		StrSize+= S - 1;

		if (CurPos > Pos)
			CurPos+= S - 1;

		Str = (wchar_t *)realloc(Str, (StrSize + 1) * sizeof(wchar_t));
		TabPtr = Str + Pos;
		wmemmove(TabPtr + S, TabPtr + 1, PrevStrSize - Pos);
		wmemset(TabPtr, L' ', S);
		Str[StrSize] = 0;
	}

	if (changed)
		Changed();
}

int Edit::GetCellCurPos()
{
	return (RealPosToCell(CurPos));
}

void Edit::SetCellCurPos(int NewPos)
{
	if (Mask && *Mask) {
		int NewPosLimit = CalcRTrimmedStrSize();
		if (NewPos > NewPosLimit)
			NewPos = NewPosLimit;
	}

	CurPos = CellPosToReal(NewPos);
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
	if (PrevPos >= StrSize)
		TabPos+= Pos - PrevPos;
	else {
		// Начинаем вычисление с предыдущей позиции
		int Index = PrevPos;

		// Проходим по всем символам до позиции поиска, если она ещё в пределах строки,
		// либо до конца строки, если позиция поиска за пределами строки
		for (; Index < Min(Pos, StrSize); Index++)

			// Обрабатываем табы
			if (Str[Index] == L'\t' && TabExpandMode != EXPAND_ALLTABS) {
				// Если есть необходимость делать корректировку табов и эта коректировка
				// ещё не проводилась, то увеличиваем длину обрабатываемой строки на еденицу
				if (bCorrectPos) {
					++Pos;
					*CorrectPos = 1;
					bCorrectPos = false;
				}

				// Расчитываем длину таба с учётом настроек и текущей позиции в строке
				TabPos+= TabSize - (TabPos % TabSize);
			}
			// Обрабатываем все остальные символы
			else if (IsCharFullWidth(Str[Index])) {
				TabPos+= 2;
			} else if (!IsCharXxxfix(Str[Index])) {
				TabPos++;
			}

		// Если позиция находится за пределами строки, то там точно нет табов и всё просто
		if (Pos >= StrSize)
			TabPos+= Pos - Index;
	}

	return TabPos;
}

int Edit::CellPosToReal(int Pos)
{
	int Index = 0;
	for (int CellPos = 0; CellPos < Pos; Index++) {
		if (Index >= StrSize) {
			Index+= Pos - CellPos;
			break;
		}

		if (Str[Index] == L'\t' && TabExpandMode != EXPAND_ALLTABS) {
			int NewCellPos = CellPos + TabSize - (CellPos % TabSize);

			if (NewCellPos > Pos)
				break;

			CellPos = NewCellPos;
		} else {
			CellPos+= IsCharFullWidth(Str[Index]) ? 2 : 1;
			while (Index + 1 < StrSize && IsCharXxxfix(Str[Index + 1])) {
				Index++;
			}
		}
	}

	return Index;
}

void Edit::SanitizeSelectionRange()
{
	if (SelEnd >= SelStart && SelStart >= 0) {
		while (SelStart > 0 && IsCharXxxfix(Str[SelStart]))
			--SelStart;

		while (SelEnd < StrSize && IsCharXxxfix(Str[SelEnd]))
			++SelEnd;
	}

	/*
		$ 24.06.2002 SKV
		Если начало выделения за концом строки, надо выделение снять.
		17.09.2002 возвращаю обратно. Глюкодром.
	*/
	if (SelEnd < SelStart && SelEnd != -1) {
		SelStart = -1;
		SelEnd = 0;
	}

	if (SelStart == -1 && SelEnd == -1) {
		SelStart = -1;
		SelEnd = 0;
	}
}

void Edit::Select(int Start, int End)
{
	SelStart = Start;
	SelEnd = End;

	SanitizeSelectionRange();
}

void Edit::AddSelect(int Start, int End)
{
	if (Start < SelStart || SelStart == -1)
		SelStart = Start;

	if (End == -1 || (End > SelEnd && SelEnd != -1))
		SelEnd = End;

	if (SelEnd > StrSize)
		SelEnd = StrSize;

	SanitizeSelectionRange();
}

void Edit::GetSelection(int &Start, int &End)
{
	/*
		$ 17.09.2002 SKV
		Мало того, что это нарушение правил OO design'а,
		так это еще и источние багов.
	*/
	/*
	if (SelEnd>StrSize+1)
		SelEnd=StrSize+1;
	if (SelStart>StrSize+1)
		SelStart=StrSize+1;
	*/
	/* SKV $ */
	Start = SelStart;
	End = SelEnd;

	if (End > StrSize)
		End = -1;	// StrSize;

	if (Start > StrSize)
		Start = StrSize;
}

void Edit::GetRealSelection(int &Start, int &End)
{
	Start = SelStart;
	End = SelEnd;
}

void Edit::DeleteBlock()
{
	if (Flags.Check(FEDITLINE_READONLY | FEDITLINE_DROPDOWNBOX))
		return;

	if (SelStart == -1 || SelStart >= SelEnd)
		return;

	PrevCurPos = CurPos;

	if (Mask && *Mask) {
		for (int i = SelStart; i < SelEnd; i++) {
			if (CheckCharMask(Mask[i]))
				Str[i] = L' ';
		}

		CurPos = SelStart;
	} else {
		int From = SelStart, To = SelEnd;

		if (From > StrSize)
			From = StrSize;

		if (To > StrSize)
			To = StrSize;

		wmemmove(Str + From, Str + To, StrSize - To + 1);
		StrSize-= To - From;

		if (CurPos > From) {
			if (CurPos < To)
				CurPos = From;
			else
				CurPos-= To - From;
		}

		Str = (wchar_t *)realloc(Str, (StrSize + 1) * sizeof(wchar_t));
	}

	SelStart = -1;
	SelEnd = 0;
	Flags.Clear(FEDITLINE_MARKINGBLOCK);

	// OT: Проверка на корректность поведения строки при удалении и вставки
	if (Flags.Check((FEDITLINE_PARENT_SINGLELINE | FEDITLINE_PARENT_MULTILINE))) {
		if (LeftPos > CurPos)
			LeftPos = CurPos;
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
	if (Item >= (int)ColorList.size())
		return false;

	*col = ColorList[Item];
	return true;
}

void Edit::ApplyColor()
{
	// Для оптимизации сохраняем вычисленные позиции между итерациями цикла
	int Pos = INT_MIN, TabPos = INT_MIN, TabEditorPos = INT_MIN;

	// Обрабатываем элементы ракраски
	for (auto &CurItem : ColorList) {
		// Пропускаем элементы у которых начало больше конца
		if (CurItem.StartPos > CurItem.EndPos)
			continue;

		// Отсекаем элементы заведомо не попадающие на экран
		if (CurItem.StartPos - LeftPos > X2 && CurItem.EndPos - LeftPos < X1)
			continue;

		DWORD64 Attr = CurItem.Color;
		int Length = CurItem.EndPos - CurItem.StartPos + 1;

		if (CurItem.StartPos + Length >= StrSize)
			Length = StrSize - CurItem.StartPos;

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
			Start = RealStart - LeftPos;
		}
		// Для оптимизации делаем вычисление относительно предыдущей позиции
		else {
			RealStart = RealPosToCell(TabPos, Pos, CurItem.StartPos, nullptr);
			Start = RealStart - LeftPos;
		}

		// Запоминаем вычисленные значения для их дальнейшего повторного использования
		Pos = CurItem.StartPos;
		TabPos = RealStart;
		TabEditorPos = Start;

		// Пропускаем элементы раскраски у которых начальная позиция за экраном
		if (Start > X2)
			continue;

		// Корректировка относительно табов (отключается, если присутвует флаг ECF_TAB1)
		int CorrectPos = Attr & ECF_TAB1 ? 0 : 1;

		if (!CorrectPos)
			Attr&= ~ECF_TAB1;

		// Получаем конечную позицию
		int EndPos = CurItem.EndPos;
		int RealEnd, End;

		/*
			Обрабатываем случай, когда предыдущая позиция равна текущей, то есть
			длина раскрашиваемой строкии равна 1
		*/
		if (Pos == EndPos) {
			/*
				Если необходимо делать корректироку относительно табов и единственный
				символ строки -- это таб, то делаем расчёт с учтом корректировки,
				иначе ничего не вычисялем и берём старые значения
			*/
			if (CorrectPos && EndPos < StrSize && Str[EndPos] == L'\t') {
				RealEnd = RealPosToCell(TabPos, Pos, ++EndPos, nullptr);
				End = RealEnd - LeftPos;
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
		else if (EndPos < Pos) {
			RealEnd = RealPosToCell(0, 0, EndPos, &CorrectPos);
			EndPos+= CorrectPos;
			End = RealEnd - LeftPos;
		}
		/*
			Для оптимизации делаем вычисление относительно предыдущей позиции (с учётом
			корректировки относительно табов)
		*/
		else {
			RealEnd = RealPosToCell(TabPos, Pos, EndPos, &CorrectPos);
			EndPos+= CorrectPos;
			End = RealEnd - LeftPos;
		}

		// Запоминаем вычисленные значения для их дальнейшего повторного использования
		Pos = EndPos;
		TabPos = RealEnd;
		TabEditorPos = End;

		// Пропускаем элементы раскраски у которых конечная позиция меньше левой границы экрана
		if (End < X1)
			continue;

		// Обрезаем раскраску элемента по экрану
		if (Start < X1)
			Start = X1;

		if (End > X2)
			End = X2;

		// Устанавливаем длину раскрашиваемого элемента
		Length = End - Start + 1;

		if (Length < X2)
			Length-= CorrectPos;

		// Раскрашиваем элемент, если есть что раскрашивать
		if (Length > 0) {
			ScrBuf.ApplyColor(Start, Y1, Start + Length - 1, Y1, Attr, SelColor );
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
	// Для CmdLine - если нет выделения, преобразуем всю строку
	if (All && SelStart == -1 && !SelEnd) {
		::Xlat(Str, 0, StrLength(Str), Opt.XLat.Flags);
		Changed();
		Show();
		return;
	}

	if (SelStart != -1 && SelStart != SelEnd) {
		if (SelEnd == -1)
			SelEnd = StrLength(Str);

		::Xlat(Str, SelStart, SelEnd, Opt.XLat.Flags);
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
		int start = CurPos, end, StrSize = StrLength(Str);
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

			while (end < StrSize && !IsWordDiv(Opt.XLat.strWordDivForXlat, Str[end]))
				end++;

			::Xlat(Str, start, end, Opt.XLat.Flags);
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
	int Inserted = FALSE;

	if (Mask[CurPos] == EDMASK_ANY)
		Inserted = TRUE;
	else if (Mask[CurPos] == EDMASK_DSS && (std::iswdigit(Key) || Key == L' ' || Key == L'-'))
		Inserted = TRUE;
	else if (Mask[CurPos] == EDMASK_DIGITS && (std::iswdigit(Key) || Key == L' '))
		Inserted = TRUE;
	else if (Mask[CurPos] == EDMASK_DIGIT && (std::iswdigit(Key)))
		Inserted = TRUE;
	else if (Mask[CurPos] == EDMASK_ALPHA && IsAlpha(Key))
		Inserted = TRUE;
	else if (Mask[CurPos] == EDMASK_HEX && std::iswxdigit(Key))
		Inserted = TRUE;

	return Inserted;
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
	if (m_Callback.Active && m_Callback.m_Callback) {
		m_Callback.m_Callback(m_Callback.m_Param);
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

EditControl::EditControl(ScreenObject *pOwner, Callback *aCallback, bool bAllocateData, History *iHistory,
		FarList *iList, DWORD iFlags)
	:
	Edit(pOwner, aCallback, bAllocateData),
	pCustomCompletionList(nullptr),
	pHistory(iHistory),
	pList(iList),
	Selection(false),
	SelectionStart(-1),
	ECFlags(iFlags)
{
	ACState = ECFlags.Check(EC_ENABLEAUTOCOMPLETE) != FALSE;
}

void EditControl::Show()
{
	if (X2 - X1 + 1 > StrSize) {
		SetLeftPos(0);
	}
	Edit::Show();
}

void EditControl::Changed(bool DelBlock)
{
	if (m_Callback.Active) {
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
	VMenu ComplMenu(nullptr, nullptr, 0, 0);
	FARString strTemp = Str;
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
				StrSize--;
				SelStart--;
				CurPos--;
			}

			InsertString(ComplMenu.GetItemPtr(0)->strName.SubStr(SelStart));
			Select(SelStart, GetLength());
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
										StrSize--;
										SelStart--;
										CurPos--;
									}

									DisableCallback DC(m_Callback.Active);
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

	if (ECFlags.Check(EC_ENABLEAUTOCOMPLETE) && *Str && !Reenter
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

int EditControl::ProcessMouse(MOUSE_EVENT_RECORD *MouseEvent)
{
	if (Edit::ProcessMouse(MouseEvent)) {
		while (IsMouseButtonPressed() == FROM_LEFT_1ST_BUTTON_PRESSED) {
			Flags.Clear(FEDITLINE_CLEARFLAG);
			SetCellCurPos(MouseX - X1 + LeftPos);
			if (MouseEventFlags & MOUSE_MOVED) {
				if (!Selection) {
					Selection = true;
					SelectionStart = -1;
					Select(SelectionStart, 0);
				} else {
					if (SelectionStart == -1) {
						SelectionStart = CurPos;
					}
					Select(Min(SelectionStart, CurPos), Min(StrSize, Max(SelectionStart, CurPos)));
					Show();
				}
			}
		}
		Selection = false;
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
