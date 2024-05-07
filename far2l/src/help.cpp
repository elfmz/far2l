/*
help.cpp

Помощь
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

#include "help.hpp"
#include "keyboard.hpp"
#include "lang.hpp"
#include "keys.hpp"
#include "colors.hpp"
#include "palette.hpp"
#include "scantree.hpp"
#include "savescr.hpp"
#include "manager.hpp"
#include "ctrlobj.hpp"
#include "macroopcode.hpp"
#include "syslog.hpp"
#include "interf.hpp"
#include "message.hpp"
#include "config.hpp"
#include "execute.hpp"
#include "pathmix.hpp"
#include "strmix.hpp"
#include "exitcode.hpp"
#include "filestr.hpp"
#include "stddlg.hpp"

// Стек возврата
class CallBackStack
{
private:
	struct ListNode
	{
		ListNode *Next;

		DWORD Flags;				// флаги
		int TopStr;					// номер верхней видимой строки темы
		int CurX, CurY;				// координаты (???)

		FARString strHelpTopic;		// текущий топик
		FARString strHelpPath;		// путь к хелпам
		FARString strSelTopic;		// текущее выделение
		FARString strHelpMask;		// маска

		ListNode(const StackHelpData *Data, ListNode *n = nullptr)
		{
			strHelpTopic = Data->strHelpTopic;
			strHelpPath = Data->strHelpPath;
			strSelTopic = Data->strSelTopic;
			strHelpMask = Data->strHelpMask;
			Flags = Data->Flags;
			TopStr = Data->TopStr;
			CurX = Data->CurX;
			CurY = Data->CurY;
			Next = n;
		}
		~ListNode() {}
	};

	ListNode *topOfStack;

public:
	CallBackStack() { topOfStack = nullptr; }
	~CallBackStack() { ClearStack(); }

public:
	void ClearStack();
	BOOL isEmpty() const { return !topOfStack; }

	void Push(const StackHelpData *Data);
	int Pop(StackHelpData *Data = nullptr);

	void PrintStack(const wchar_t *Title);
};

static const wchar_t *FoundContents = L"__FoundContents__";
static const wchar_t *PluginContents = L"__PluginContents__";
static const wchar_t *HelpOnHelpTopic = L":Help";
static const wchar_t *HelpContents = L"Contents";

void Help::Present(const wchar_t *Topic, const wchar_t *Mask, DWORD Flags)
{
	Help Hlp(Topic, Mask, Flags);
}

Help::Help(const wchar_t *Topic, const wchar_t *Mask, DWORD Flags)
	:
	CMM(MACRO_HELP),
	ErrorHelp(TRUE),
	IsNewTopic(TRUE),
	MouseDown(FALSE),
	CurColor(FarColorToReal(COL_HELPTEXT)),
	CtrlTabSize(8)
{
	CanLoseFocus = FALSE;
	KeyBarVisible = TRUE;
	/* $ OT По умолчанию все хелпы создаются статически*/
	SetDynamicallyBorn(FALSE);
	Stack = new CallBackStack;
	StackData.Clear();
	StackData.Flags = Flags;
	StackData.strHelpMask = Mask;	// сохраним маску файла
	TopScreen = new SaveScreen;
	StackData.strHelpTopic = Topic;

	if (Opt.FullScreenHelp)
		SetPosition(0, 0, ScrX, ScrY);
	else
		SetPosition(4, 2, ScrX - 4, ScrY - 2);

	if (!ReadHelp(StackData.strHelpMask) && (Flags & FHELP_USECONTENTS)) {
		StackData.strHelpTopic = Topic;

		if (StackData.strHelpTopic.At(0) == HelpBeginLink) {
			size_t pos;

			if (StackData.strHelpTopic.RPos(pos, HelpEndLink))
				StackData.strHelpTopic.Truncate(pos + 1);

			StackData.strHelpTopic+= HelpContents;
		}

		StackData.strHelpPath.Clear();
		ReadHelp(StackData.strHelpMask);
	}

	if (HelpList.getSize()) {
		ScreenObject::Flags.Clear(FHELPOBJ_ERRCANNOTOPENHELP);
		InitKeyBar();
		MacroMode = MACRO_HELP;
		MoveToReference(1, 1);
		FrameManager->ExecuteModal(this);	// OT
	} else {
		ErrorHelp = TRUE;

		if (!(Flags & FHELP_NOSHOWERROR)) {
			if (!ScreenObject::Flags.Check(FHELPOBJ_ERRCANNOTOPENHELP)) {
				Message(MSG_WARNING, 1, Msg::HelpTitle, Msg::HelpTopicNotFound, StackData.strHelpTopic,
						Msg::Ok);
			}

			ScreenObject::Flags.Clear(FHELPOBJ_ERRCANNOTOPENHELP);
		}
	}
}

Help::~Help()
{
	SetRestoreScreenMode(FALSE);

	if (Stack)
		delete Stack;

	if (TopScreen)
		delete TopScreen;
}

void Help::Hide()
{
	ScreenObject::Hide();
}

int Help::ReadHelp(const wchar_t *Mask)
{
	wchar_t *ReadStr;
	FARString strSplitLine;
	int Formatting = TRUE, RepeatLastLine, BreakProcess;
	const int MaxLength = X2 - X1 - 1;
	FARString strPath;

	if (StackData.strHelpTopic.At(0) == HelpBeginLink) {
		strPath = StackData.strHelpTopic.CPtr() + 1;
		size_t pos;

		if (!strPath.Pos(pos, HelpEndLink))
			return FALSE;

		StackData.strHelpTopic = strPath.CPtr() + pos + 1;
		strPath.Truncate(pos);
		DeleteEndSlash(strPath, true);
		AddEndSlash(strPath);
		StackData.strHelpPath = strPath;
	} else {
		strPath = !StackData.strHelpPath.IsEmpty() ? StackData.strHelpPath : g_strFarPath;
	}

	if (!StrCmp(StackData.strHelpTopic, PluginContents)) {
		strFullHelpPathName.Clear();
		ReadDocumentsHelp(HIDX_PLUGINS);
		return TRUE;
	}

	UINT nCodePage = CP_UTF8;
	FILE *HelpFile = OpenLangFile(strPath, (!*Mask ? HelpFileMask : Mask), Opt.strHelpLanguage,
			strFullHelpPathName, nCodePage);

	if (!HelpFile) {
		ErrorHelp = TRUE;

		if (!ScreenObject::Flags.Check(FHELPOBJ_ERRCANNOTOPENHELP)) {
			ScreenObject::Flags.Set(FHELPOBJ_ERRCANNOTOPENHELP);

			if (!(StackData.Flags & FHELP_NOSHOWERROR)) {
				Message(MSG_WARNING, 1, Msg::HelpTitle, Msg::CannotOpenHelp, Mask, Msg::Ok);
			}
		}

		return FALSE;
	}

	FARString strReadStr;

	if (GetOptionsParam(HelpFile, L"TabSize", strReadStr, nCodePage)) {
		CtrlTabSize = _wtoi(strReadStr);
	}

	if (CtrlTabSize < 0 || CtrlTabSize > 16)
		CtrlTabSize = Opt.HelpTabSize;

	if (GetOptionsParam(HelpFile, L"CtrlColorChar", strReadStr, nCodePage))
		strCtrlColorChar = strReadStr;
	else
		strCtrlColorChar.Clear();

	if (GetOptionsParam(HelpFile, L"CtrlStartPosChar", strReadStr, nCodePage))
		strCtrlStartPosChar = strReadStr;
	else
		strCtrlStartPosChar.Clear();

	/*
		$ 29.11.2001 DJ
		запомним, чего там написано в PluginContents
	*/
	if (!GetLangParam(HelpFile, L"PluginContents", &strCurPluginContents, nullptr, nCodePage))
		strCurPluginContents.Clear();

	HelpList.Free();

	if (!StrCmp(StackData.strHelpTopic, FoundContents)) {
		Search(HelpFile, nCodePage);
		fclose(HelpFile);
		return TRUE;
	}

	StrCount = 0;
	FixCount = 0;
	TopicFound = 0;
	RepeatLastLine = FALSE;
	BreakProcess = FALSE;
	int NearTopicFound = 0;
	wchar_t PrevSymbol = 0;

	StartPos = (DWORD)-1;
	LastStartPos = (DWORD)-1;
	int RealMaxLength;
	bool MacroProcess = false;
	int MI = 0;
	FARString strMacroArea;

	OldGetFileString GetStr(HelpFile);
	int nStrLength, GetCode;

	for (;;) {
		if (StartPos != (DWORD)-1)
			RealMaxLength = MaxLength - StartPos;
		else
			RealMaxLength = MaxLength;

		if (!MacroProcess && !RepeatLastLine && !BreakProcess) {
			if ((GetCode = GetStr.GetString(&ReadStr, nCodePage, nStrLength)) <= 0) {
				strReadStr = ReadStr;
				if (StringLen(strSplitLine) < MaxLength) {
					if (strSplitLine.At(0))
						AddLine(strSplitLine.CPtr());
				} else {
					strReadStr.Clear();
					RepeatLastLine = TRUE;
					continue;
				}

				break;
			} else {
				strReadStr = ReadStr;
			}
		}

		if (MacroProcess) {
			FARString strDescription;
			FARString strKeyName;
			FARString strOutTemp;

			if (CtrlObject->Macro.GetMacroKeyInfo(true, CtrlObject->Macro.GetSubKey(strMacroArea), MI,
						strKeyName, strDescription)
					== -1) {
				MacroProcess = false;
				MI = 0;
				continue;
			}

			if (strKeyName.At(0) == L'~') {
				MI++;
				continue;
			}

			ReplaceStrings(strKeyName, L"~", L"~~", -1);
			ReplaceStrings(strKeyName, L"#", L"##", -1);
			ReplaceStrings(strKeyName, L"@", L"@@", -1);
			int SizeKeyName = 20;

			if (wcschr(strKeyName, L'~'))	// корректировка размера
				SizeKeyName++;

			strOutTemp.Format(L" #%-*.*ls# ", SizeKeyName, SizeKeyName, strKeyName.CPtr());

			if (!strDescription.IsEmpty()) {
				ReplaceStrings(strDescription, L"#", L"##", -1);
				ReplaceStrings(strDescription, L"~", L"~~", -1);
				ReplaceStrings(strDescription, L"@", L"@@", -1);
				strOutTemp+= strCtrlStartPosChar;
				strOutTemp+= TruncStrFromEnd(strDescription, 300);
			}

			strReadStr = strOutTemp;
			MacroProcess = true;
			MI++;
		}

		RepeatLastLine = FALSE;

		// заменим табулятор по всем правилам
		ReplaceTabsBySpaces(strReadStr, CtrlTabSize);
		RemoveTrailingSpaces(strReadStr);

		if (!strCtrlStartPosChar.IsEmpty() && wcsstr(strReadStr, strCtrlStartPosChar)) {
			int Length = (int)(wcsstr(strReadStr, strCtrlStartPosChar) - strReadStr);
			FARString strLine = strReadStr;
			strLine.Truncate(Length);
			LastStartPos = StringLen(strLine);
			strReadStr = strReadStr.SubStr(0, Length)
					+ strReadStr.SubStr(Length + strCtrlStartPosChar.GetLength());
		}

		if (TopicFound) {
			HighlightsCorrection(strReadStr);
		}

		if (strReadStr.At(0) == L'@' && !BreakProcess) {
			if (TopicFound) {
				if (!StrCmp(strReadStr, L"@+")) {
					Formatting = TRUE;
					PrevSymbol = 0;
					continue;
				}

				if (!StrCmp(strReadStr, L"@-")) {
					Formatting = FALSE;
					PrevSymbol = 0;
					continue;
				}

				if (strSplitLine.At(0)) {
					BreakProcess = TRUE;
					strReadStr.Clear();
					PrevSymbol = 0;
					goto m1;
				}

				break;
			} else if (!StrCmpI(strReadStr.CPtr() + 1, StackData.strHelpTopic)) {
				TopicFound = 1;
				NearTopicFound = 1;
			}
		} else {
		m1:
			if (strReadStr.IsEmpty() && BreakProcess && strSplitLine.IsEmpty())
				break;

			if (TopicFound) {
				if (!StrCmpNI(strReadStr.CPtr(), L"<!Macro:", 8) && CtrlObject) {
					size_t PosEnd;
					if (!strReadStr.Pos(PosEnd, L'>') || strReadStr.At(PosEnd - 1) != L'!')
						continue;

					strMacroArea = strReadStr.SubStr(8, PosEnd - 1 - 8);	//???
					MacroProcess = true;
					MI = 0;
					continue;
				}

				/*
					$<text> в начале строки, определение темы
					Определяет не прокручиваемую область помощи
					Если идут несколько подряд сразу после строки обозначения темы...
				*/
				if (NearTopicFound) {
					StartPos = (DWORD)-1;
					LastStartPos = (DWORD)-1;
				}

				if (strReadStr.At(0) == L'$' && NearTopicFound
						&& (PrevSymbol == L'$' || PrevSymbol == L'@')) {
					AddLine(strReadStr.CPtr() + 1);
					FixCount++;
				} else {
					NearTopicFound = 0;

					if (!strReadStr.At(0) || !Formatting) {
						if (!strSplitLine.IsEmpty()) {
							if (StringLen(strSplitLine) < RealMaxLength) {
								AddLine(strSplitLine);
								strSplitLine.Clear();

								if (StringLen(strReadStr) < RealMaxLength) {
									AddLine(strReadStr);
									LastStartPos = (DWORD)-1;
									StartPos = (DWORD)-1;
									continue;
								}
							} else
								RepeatLastLine = TRUE;
						} else if (!strReadStr.IsEmpty()) {
							if (StringLen(strReadStr) < RealMaxLength) {
								AddLine(strReadStr);
								continue;
							}
						} else if (strReadStr.IsEmpty() && strSplitLine.IsEmpty()) {
							AddLine(L"");
							continue;
						}
					}

					if (!strReadStr.IsEmpty() && IsSpace(strReadStr.At(0)) && Formatting) {
						if (StringLen(strSplitLine) < RealMaxLength) {
							if (!strSplitLine.IsEmpty()) {
								AddLine(strSplitLine);
								StartPos = (DWORD)-1;
							}

							strSplitLine = strReadStr;
							strReadStr.Clear();
							continue;
						} else
							RepeatLastLine = TRUE;
					}

					if (!RepeatLastLine) {
						if (!strSplitLine.IsEmpty())
							strSplitLine+= L" ";

						strSplitLine+= strReadStr;
					}

					if (StringLen(strSplitLine) < RealMaxLength) {
						if (strReadStr.IsEmpty() && BreakProcess)
							goto m1;

						continue;
					}

					int Splitted = 0;

					for (int I = (int)strSplitLine.GetLength() - 1; I > 0; I--) {
						if (strSplitLine.At(I) == L'~' && strSplitLine.At(I - 1) == L'~') {
							I--;
							continue;
						}

						if (strSplitLine.At(I) == L'~' && strSplitLine.At(I - 1) != L'~') {
							do {
								I--;
							} while (I > 0 && strSplitLine.At(I) != L'~');

							continue;
						}

						if (strSplitLine.At(I) == L' ') {
							wchar_t *lpwszPtr = strSplitLine.GetBuffer();
							lpwszPtr[I] = 0;

							if (StringLen(lpwszPtr) < RealMaxLength) {
								AddLine(lpwszPtr);
								wmemmove(lpwszPtr + 1, lpwszPtr + I + 1, StrLength(lpwszPtr + I + 1) + 1);
								*lpwszPtr = L' ';
								strSplitLine.ReleaseBuffer();

								HighlightsCorrection(strSplitLine);
								Splitted = TRUE;
								break;
							} else {
								lpwszPtr[I] = L' ';
								strSplitLine.ReleaseBuffer();
							}
						}
					}

					if (!Splitted) {
						AddLine(strSplitLine);
						strSplitLine.Clear();
					} else {
						StartPos = LastStartPos;
					}
				}
			}

			if (BreakProcess) {
				if (!strSplitLine.IsEmpty())
					goto m1;

				break;
			}
		}

		PrevSymbol = strReadStr.At(0);
	}

	AddLine(L"");
	fclose(HelpFile);
	FixSize = FixCount ? FixCount + 1 : 0;
	ErrorHelp = FALSE;

	if (IsNewTopic) {
		StackData.CurX = StackData.CurY = 0;
		StackData.TopStr = 0;
	}

	return TopicFound;
}

void Help::AddLine(const wchar_t *Line)
{
	FARString strLine;

	if (StartPos != 0xFFFFFFFF) {
		DWORD StartPos0 = StartPos;
		if (*Line == L' ')
			StartPos0--;

		if (StartPos0 > 0) {
			strLine.Append(L' ', StartPos0);
		}
	}

	strLine+= Line;

	{
		HelpRecord AddRecord(strLine);
		HelpList.addItem(AddRecord);
	}

	StrCount++;
}

void Help::AddTitle(const wchar_t *Title)
{
	FARString strIndexHelpTitle;
	strIndexHelpTitle.Format(L"^ #%ls#", Title);
	AddLine(strIndexHelpTitle);
}

void Help::HighlightsCorrection(FARString &strStr)
{
	int I, Count;

	for (I = 0, Count = 0; strStr.At(I); I++)
		if (strStr.At(I) == L'#')
			Count++;

	if ((Count & 1) && strStr.At(0) != L'$')
		strStr.Insert(0, L'#');
}

void Help::DisplayObject()
{
	if (!TopScreen)
		TopScreen = new SaveScreen;

	if (!TopicFound) {
		if (!ErrorHelp)		// если это убрать, то при несуществующей ссылки
		{					// с нынешним манагером попадаем в бесконечный цикл.
			ErrorHelp = TRUE;

			if (!(StackData.Flags & FHELP_NOSHOWERROR)) {
				Message(MSG_WARNING, 1, Msg::HelpTitle, Msg::HelpTopicNotFound, StackData.strHelpTopic,
						Msg::Ok);
			}

			ProcessKey(KEY_ALTF1);
		}

		return;
	}

	SetCursorType(0, 10);

	if (StackData.strSelTopic.IsEmpty())
		MoveToReference(1, 1);

	FastShow();

	if (!Opt.FullScreenHelp) {
		HelpKeyBar.SetPosition(0, ScrY, ScrX, ScrY);
		HelpKeyBar.Refresh(Opt.ShowKeyBar);
	} else {
		HelpKeyBar.Refresh(false);
	}
}

void Help::FastShow()
{
	if (!Locked())
		DrawWindowFrame();

	CorrectPosition();
	StackData.strSelTopic.Clear();
	/*
		$ 01.09.2000 SVS
		Установим по умолчанию текущий цвет отрисовки...
		чтобы новая тема начиналась с нормальными атрибутами
	*/
	CurColor = FarColorToReal(COL_HELPTEXT);

	for (int i = 0; i < Y2 - Y1 - 1; i++) {
		int StrPos;

		if (i < FixCount) {
			StrPos = i;
		} else if (i == FixCount && FixCount > 0) {
			if (!Locked()) {
				GotoXY(X1, Y1 + i + 1);
				SetFarColor(COL_HELPBOX);
				ShowSeparator(X2 - X1 + 1, 1);
			}

			continue;
		} else {
			StrPos = i + StackData.TopStr;

			if (FixCount > 0)
				StrPos--;
		}

		if (StrPos < StrCount) {
			const HelpRecord *rec = GetHelpItem(StrPos);
			const wchar_t *OutStr = rec ? rec->HelpStr : nullptr;

			if (!OutStr)
				OutStr = L"";

			if (*OutStr == L'^') {
				int LeftShift;
				do {
					LeftShift = (X2 - X1 + 1 - StringLen(OutStr)) / 2;
					OutStr++;
				} while (LeftShift < 0 && *OutStr);
				GotoXY(X1 + LeftShift, Y1 + i + 1);
			} else {
				GotoXY(X1 + 1, Y1 + i + 1);
			}

			OutString(OutStr);
		}
	}

	if (!Locked()) {
		SetFarColor(COL_HELPSCROLLBAR);
		ScrollBarEx(X2, Y1 + FixSize + 1, Y2 - Y1 - FixSize - 1, StackData.TopStr, StrCount - FixCount);
	}
}

void Help::DrawWindowFrame()
{
	SetScreen(X1, Y1, X2, Y2, L' ', FarColorToReal(COL_HELPTEXT));
	Box(X1, Y1, X2, Y2, FarColorToReal(COL_HELPBOX), DOUBLE_BOX);
	SetFarColor(COL_HELPBOXTITLE);
	FARString strHelpTitleBuf;
	strHelpTitleBuf = Msg::HelpTitle;
	strHelpTitleBuf+= L" - ";

	if (!strCurPluginContents.IsEmpty())
		strHelpTitleBuf+= strCurPluginContents;
	else
		strHelpTitleBuf+= L"FAR2L";

	TruncStrFromEnd(strHelpTitleBuf, X2 - X1 - 3);
	GotoXY(X1 + (X2 - X1 + 1 - (int)strHelpTitleBuf.GetLength() - 2) / 2, Y1);
	FS << L" " << strHelpTitleBuf << L" ";

	// Bottom notification about BETA & windows legacy
	strHelpTitleBuf = L"(FAR2L Beta: some topics are not quite relevant and contain Windows legacy)";
	TruncStrFromEnd(strHelpTitleBuf, X2 - X1 - 3);
	GotoXY(X1 + (X2 - X1 + 1 - (int)strHelpTitleBuf.GetLength() - 2) / 2, Y2);
	FS << L" " << strHelpTitleBuf << L" ";
}

/*
	$ 01.09.2000 SVS
	Учтем символ CtrlColorChar & CurColor
*/
void Help::OutString(const wchar_t *Str)
{
	wchar_t OutStr[512];	// BUGBUG
	const wchar_t *StartTopic = nullptr;
	int OutPos = 0, Highlight = 0, Topic = 0;

	while (OutPos < (int)(ARRAYSIZE(OutStr) - 10)) {
		if ((Str[0] == L'~' && Str[1] == L'~') || (Str[0] == L'#' && Str[1] == L'#')
				|| (Str[0] == L'@' && Str[1] == L'@')
				|| (!strCtrlColorChar.IsEmpty() && Str[0] == strCtrlColorChar.At(0)
						&& Str[1] == strCtrlColorChar.At(0))) {
			OutStr[OutPos++] = *Str;
			Str+= 2;
			continue;
		}

		if (*Str == L'~'
				|| ((*Str == L'#' || *Str == strCtrlColorChar.At(0)) && !Topic) /*|| *Str==HelpBeginLink*/
				|| !*Str) {
			OutStr[OutPos] = 0;

			if (Topic) {
				int RealCurX, RealCurY;
				RealCurX = X1 + StackData.CurX + 1;
				RealCurY = Y1 + StackData.CurY + FixSize + 1;

				if (WhereY() == RealCurY && RealCurX >= WhereX()
						&& RealCurX < WhereX() + (Str - StartTopic) - 1) {
					SetFarColor(COL_HELPSELECTEDTOPIC);

					if (Str[1] == L'@') {
						StackData.strSelTopic = (Str + 2);
						/*
							$ 25.08.2000 SVS
							учтем, что может быть такой вариант: @@ или \@
							этот вариант только для URL!
						*/
						size_t pos;

						if (StackData.strSelTopic.Pos(pos, L'@')) {
							wchar_t *EndPtr =
									StackData.strSelTopic.GetBuffer(StackData.strSelTopic.GetLength() * 2)
									+ pos;

							if (*(EndPtr + 1) == L'@') {
								wmemmove(EndPtr, EndPtr + 1, StrLength(EndPtr) + 1);
								EndPtr++;
							}

							EndPtr = wcschr(EndPtr, L'@');

							if (EndPtr)
								*EndPtr = 0;

							StackData.strSelTopic.ReleaseBuffer();
						}
					}
				} else {
					SetFarColor(COL_HELPTOPIC);
				}
			} else {
				if (Highlight)
					SetFarColor(COL_HELPHIGHLIGHTTEXT);
				else
					SetColor(CurColor);
			}

			/*
				$ 24.09.2001 VVM
				! Обрежем длинные строки при показе. Такое будет только при длинных ссылках...
			*/
			if (static_cast<int>(StrLength(OutStr) + WhereX()) > X2)
				OutStr[X2 - WhereX()] = 0;

			if (Locked())
				GotoXY(WhereX() + StrLength(OutStr), WhereY());
			else
				Text(OutStr);

			OutPos = 0;
		}

		if (!*Str)
			break;

		if (*Str == L'~') {
			if (!Topic)
				StartTopic = Str;

			Topic = !Topic;
			Str++;
			continue;
		}

		if (*Str == L'@') {
			/*
				$ 25.08.2000 SVS
				учтем, что может быть такой вариант: @@
				этот вариант только для URL!
			*/
			while (*Str)
				if (*(++Str) == L'@' && *(Str - 1) != L'@')
					break;

			Str++;
			continue;
		}

		if (*Str == L'#') {
			Highlight = !Highlight;
			Str++;
			continue;
		}

		if (*Str == strCtrlColorChar.At(0)) {
			WORD Chr;
			Chr = (BYTE)Str[1];

			if (Chr == L'-')	// "\-" - установить дефолтовый цвет
			{
				Str+= 2;
				CurColor = FarColorToReal(COL_HELPTEXT);
				continue;
			}

			if (iswxdigit(Chr) && iswxdigit(Str[2])) {
				WORD Attr;

				if (Chr >= L'0' && Chr <= L'9')
					Chr-= L'0';
				else {
					Chr&= ~0x20;
					Chr = Chr - L'A' + 10;
				}

				Attr = (Chr << 4) & 0x00F0;
				// next char
				Chr = Str[2];

				if (Chr >= L'0' && Chr <= L'9')
					Chr-= L'0';
				else {
					Chr&= ~0x20;
					Chr = Chr - L'A' + 10;
				}

				Attr|= (Chr & 0x000F);
				CurColor = Attr;
				Str+= 3;
				continue;
			}
		}

		OutStr[OutPos++] = *(Str++);
	}

	if (!Locked() && WhereX() < X2) {
		SetColor(CurColor);
		FS << fmt::Cells() << fmt::Expand(X2 - WhereX()) << L"";
	}
}

int Help::StringLen(const wchar_t *Str)
{
	int Length = 0;

	while (*Str) {
		if ((Str[0] == L'~' && Str[1] == L'~') || (Str[0] == L'#' && Str[1] == L'#')
				|| (Str[0] == L'@' && Str[1] == L'@')
				|| (!strCtrlColorChar.IsEmpty() && Str[0] == strCtrlColorChar.At(0)
						&& Str[1] == strCtrlColorChar.At(0))) {
			Length++;
			Str+= 2;
			continue;
		}

		if (*Str == L'@') {
			/*
				$ 25.08.2000 SVS
				учтем, что может быть такой вариант: @@
				этот вариант только для URL!
			*/
			while (*Str)
				if (*(++Str) == L'@' && *(Str - 1) != L'@')
					break;

			Str++;
			continue;
		}

		/*
			$ 01.09.2000 SVS
			учтем наше нововведение \XX или \-
		*/
		if (*Str == strCtrlColorChar.At(0)) {
			if (Str[1] == L'-') {
				Str+= 2;
				continue;
			}

			if (iswxdigit(Str[1]) && iswxdigit(Str[2])) {
				Str+= 3;
				continue;
			}
		}

		if (*Str != L'#' && *Str != L'~')
			Length++;

		Str++;
	}

	return (Length);
}

void Help::CorrectPosition()
{
	if (StackData.CurX > X2 - X1 - 2)
		StackData.CurX = X2 - X1 - 2;

	if (StackData.CurX < 0)
		StackData.CurX = 0;

	if (StackData.CurY > Y2 - Y1 - 2 - FixSize) {
		StackData.TopStr+= StackData.CurY - (Y2 - Y1 - 2 - FixSize);
		StackData.CurY = Y2 - Y1 - 2 - FixSize;
	}

	if (StackData.CurY < 0) {
		StackData.TopStr+= StackData.CurY;
		StackData.CurY = 0;
	}

	if (StackData.TopStr > StrCount - FixCount - (Y2 - Y1 - 1 - FixSize))
		StackData.TopStr = StrCount - FixCount - (Y2 - Y1 - 1 - FixSize);

	if (StackData.TopStr < 0)
		StackData.TopStr = 0;
}

int64_t Help::VMProcess(MacroOpcode OpCode, void *vParam, int64_t iParam)
{
	switch (OpCode) {
		case MCODE_V_HELPFILENAME:							// Help.FileName
			*(FARString *)vParam = strFullHelpPathName;		// ???
			break;
		case MCODE_V_HELPTOPIC:								// Help.Topic
			*(FARString *)vParam = StackData.strHelpTopic;	// ???
			break;
		case MCODE_V_HELPSELTOPIC:							// Help.SELTopic
			*(FARString *)vParam = StackData.strSelTopic;	// ???
			break;
		default:
			return 0;
	}

	return 1;
}

int Help::ProcessKey(FarKey Key)
{
	if (StackData.strSelTopic.IsEmpty())
		StackData.CurX = StackData.CurY = 0;

	switch (Key) {
		case KEY_NONE:
		case KEY_IDLE: {
			break;
		}
		case KEY_F5: {
			Opt.FullScreenHelp = !Opt.FullScreenHelp;
			ResizeConsole();
			return TRUE;
		}
		case KEY_ESC:
		case KEY_F10: {
			FrameManager->DeleteFrame();
			SetExitCode(XC_QUIT);
			return TRUE;
		}
		case KEY_HOME:
		case KEY_NUMPAD7:
		case KEY_CTRLHOME:
		case KEY_CTRLNUMPAD7:
		case KEY_CTRLPGUP:
		case KEY_CTRLNUMPAD9: {
			StackData.CurX = StackData.CurY = 0;
			StackData.TopStr = 0;
			FastShow();

			if (StackData.strSelTopic.IsEmpty())
				MoveToReference(1, 1);

			return TRUE;
		}
		case KEY_END:
		case KEY_NUMPAD1:
		case KEY_CTRLEND:
		case KEY_CTRLNUMPAD1:
		case KEY_CTRLPGDN:
		case KEY_CTRLNUMPAD3: {
			StackData.CurX = StackData.CurY = 0;
			StackData.TopStr = StrCount;
			FastShow();

			if (StackData.strSelTopic.IsEmpty()) {
				StackData.CurX = 0;
				StackData.CurY = Y2 - Y1 - 2 - FixSize;
				MoveToReference(0, 1);
			}

			return TRUE;
		}
		case KEY_UP:
		case KEY_NUMPAD8: {
			if (StackData.TopStr > 0) {
				StackData.TopStr--;

				if (StackData.CurY < Y2 - Y1 - 2 - FixSize) {
					StackData.CurX = X2 - X1 - 2;
					StackData.CurY++;
				}

				FastShow();

				if (StackData.strSelTopic.IsEmpty())
					MoveToReference(0, 1);
			} else
				ProcessKey(KEY_SHIFTTAB);

			return TRUE;
		}
		case KEY_DOWN:
		case KEY_NUMPAD2: {
			if (StackData.TopStr < StrCount - FixCount - (Y2 - Y1 - 1 - FixSize)) {
				StackData.TopStr++;

				if (StackData.CurY > 0)
					StackData.CurY--;

				StackData.CurX = 0;
				FastShow();

				if (StackData.strSelTopic.IsEmpty())
					MoveToReference(1, 1);
			} else
				ProcessKey(KEY_TAB);

			return TRUE;
		}
		/*
			$ 26.07.2001 VVM
			+ С альтом скролим по 1
		*/
		case KEY_MSWHEEL_UP:
		case (KEY_MSWHEEL_UP | KEY_ALT): {
			if (StackData.TopStr > 0) {
				StackData.TopStr-= Key & KEY_ALT ? 1 : Opt.MsWheelDeltaHelp;
				FastShow();
			}
			return TRUE;
		}
		case KEY_MSWHEEL_DOWN:
		case (KEY_MSWHEEL_DOWN | KEY_ALT): {
			if (StackData.TopStr < StrCount - FixCount - (Y2 - Y1 - 1 - FixSize)) {
				StackData.TopStr+= Key & KEY_ALT ? 1 : Opt.MsWheelDeltaHelp;
				FastShow();
			}
			return TRUE;
		}
		case KEY_PGUP:
		case KEY_NUMPAD9: {
			StackData.CurX = StackData.CurY = 0;
			StackData.TopStr-= Y2 - Y1 - 2 - FixSize;
			FastShow();

			if (StackData.strSelTopic.IsEmpty()) {
				StackData.CurX = StackData.CurY = 0;
				MoveToReference(1, 1);
			}

			return TRUE;
		}
		case KEY_PGDN:
		case KEY_NUMPAD3: {
			{
				int PrevTopStr = StackData.TopStr;
				StackData.TopStr+= Y2 - Y1 - 2 - FixSize;
				FastShow();

				if (StackData.TopStr == PrevTopStr) {
					ProcessKey(KEY_CTRLPGDN);
					return TRUE;
				} else
					StackData.CurX = StackData.CurY = 0;

				MoveToReference(1, 1);
			}
			return TRUE;
		}
		case KEY_RIGHT:
		case KEY_NUMPAD6:
		case KEY_MSWHEEL_RIGHT:
		case KEY_TAB: {
			MoveToReference(1, 0);
			return TRUE;
		}
		case KEY_LEFT:
		case KEY_NUMPAD4:
		case KEY_MSWHEEL_LEFT:
		case KEY_SHIFTTAB: {
			MoveToReference(0, 0);
			return TRUE;
		}
		case KEY_F1: {
			// не поганим SelTopic, если и так в Help on Help
			if (StrCmpI(StackData.strHelpTopic, HelpOnHelpTopic)) {
				Stack->Push(&StackData);
				IsNewTopic = TRUE;
				JumpTopic(HelpOnHelpTopic);
				IsNewTopic = FALSE;
				ErrorHelp = FALSE;
			}

			return TRUE;
		}
		case KEY_SHIFTF1: {
			// не поганим SelTopic, если и так в теме Contents
			if (StrCmpI(StackData.strHelpTopic, HelpContents)) {
				Stack->Push(&StackData);
				IsNewTopic = TRUE;
				JumpTopic(HelpContents);
				ErrorHelp = FALSE;
				IsNewTopic = FALSE;
			}

			return TRUE;
		}
		case KEY_F7: {
			// не поганим SelTopic, если и так в FoundContents
			if (StrCmpI(StackData.strHelpTopic, FoundContents)) {
				FARString strLastSearchStr0 = strLastSearchStr;
				int Case = LastSearchCase;
				int WholeWords = LastSearchWholeWords;
				int Regexp = LastSearchRegexp;

				FARString strTempStr;
				// int RetCode = GetString(Msg::HelpSearchTitle,Msg::HelpSearchingFor,L"HelpSearch",strLastSearchStr,strLastSearchStr0);
				// Msg::HelpSearchTitle, Msg::HelpSearchingFor,
				int RetCode = GetSearchReplaceString(false, &strLastSearchStr0, &strTempStr, L"HelpSearch",
						L"", &Case, &WholeWords, nullptr, nullptr, &Regexp, nullptr);

				if (RetCode <= 0)
					return TRUE;

				strLastSearchStr = strLastSearchStr0;
				LastSearchCase = Case;
				LastSearchWholeWords = WholeWords;
				LastSearchRegexp = Regexp;

				Stack->Push(&StackData);
				IsNewTopic = TRUE;
				JumpTopic(FoundContents);
				ErrorHelp = FALSE;
				IsNewTopic = FALSE;
			}

			return TRUE;
		}
		case KEY_SHIFTF2: {
			// не поганим SelTopic, если и так в PluginContents
			if (StrCmpI(StackData.strHelpTopic, PluginContents)) {
				Stack->Push(&StackData);
				IsNewTopic = TRUE;
				JumpTopic(PluginContents);
				ErrorHelp = FALSE;
				IsNewTopic = FALSE;
			}

			return TRUE;
		}
		case KEY_ALTF1:
		case KEY_BS: {
			// Если стек возврата пуст - выходим их хелпа
			if (!Stack->isEmpty()) {
				Stack->Pop(&StackData);
				JumpTopic(StackData.strHelpTopic);
				ErrorHelp = FALSE;
				return TRUE;
			}

			return ProcessKey(KEY_ESC);
		}
		case KEY_NUMENTER:
		case KEY_ENTER: {
			if (!StackData.strSelTopic.IsEmpty() && StrCmpI(StackData.strHelpTopic, StackData.strSelTopic)) {
				Stack->Push(&StackData);
				IsNewTopic = TRUE;

				if (!JumpTopic()) {
					Stack->Pop(&StackData);
					ReadHelp(StackData.strHelpMask);	// вернем то, что отображали.
				}

				ErrorHelp = FALSE;
				IsNewTopic = FALSE;
			}

			return TRUE;
		}
	}

	return FALSE;
}

int Help::JumpTopic(const wchar_t *JumpTopic)
{
	FARString strNewTopic;
	size_t pos;
	Stack->PrintStack(JumpTopic);

	if (JumpTopic)
		StackData.strSelTopic = JumpTopic;

	/*
		$ 14.07.2002 IS
		При переходе по ссылкам используем всегда только абсолютные пути,
		если это возможно.
	*/

	// Если ссылка на другой файл, путь относительный и есть то, от чего можно
	// вычислить абсолютный путь, то сделаем это
	if (StackData.strSelTopic.At(0) == HelpBeginLink && StackData.strSelTopic.Pos(pos, HelpEndLink, 2)
			&& !IsAbsolutePath(StackData.strSelTopic.CPtr() + 1) && !StackData.strHelpPath.IsEmpty()) {
		FARString strFullPath;
		strNewTopic.Copy(StackData.strSelTopic.CPtr() + 1, pos);
		strFullPath = StackData.strHelpPath;
		// уберем _все_ конечные слеши и добавим один
		DeleteEndSlash(strFullPath, true);
		strFullPath+= L"/";
		strFullPath+= strNewTopic.CPtr() + (IsSlash(strNewTopic.At(0)) ? 1 : 0);
		bool addSlash = DeleteEndSlash(strFullPath);
		ConvertNameToFull(strFullPath, strNewTopic);
		strFullPath.Format(addSlash ? HelpFormatLink : HelpFormatLinkModule, strNewTopic.CPtr(),
				wcschr(StackData.strSelTopic.CPtr() + 2, HelpEndLink) + 1);
		StackData.strSelTopic = strFullPath;
	}

	//_SVS(SysLog(L"JumpTopic() = SelTopic=%ls",StackData.SelTopic));
	// URL активатор - это ведь так просто :-)))
	{
		strNewTopic = StackData.strSelTopic;
		if (strNewTopic.Begins("http:") || strNewTopic.Begins("https:")
				|| strNewTopic.Begins("mailto:")) {		// наверное подразумевается URL
			farExecuteA(strNewTopic.GetMB().c_str(), EF_NOWAIT | EF_HIDEOUT | EF_NOCMDPRINT | EF_OPEN);
			return FALSE;
		}
	}
	// а вот теперь попробуем...

	//_SVS(SysLog(L"JumpTopic() = SelTopic=%ls, StackData.HelpPath=%ls",StackData.SelTopic,StackData.HelpPath));
	if (!StackData.strHelpPath.IsEmpty() && StackData.strSelTopic.At(0) != HelpBeginLink
			&& StrCmp(StackData.strSelTopic, HelpOnHelpTopic)) {
		if (StackData.strSelTopic.At(0) == L':') {
			strNewTopic = StackData.strSelTopic.CPtr() + 1;
			StackData.Flags&= ~FHELP_CUSTOMFILE;
		} else if (StackData.Flags & FHELP_CUSTOMFILE)
			strNewTopic = StackData.strSelTopic;
		else
			strNewTopic.Format(HelpFormatLink, StackData.strHelpPath.CPtr(), StackData.strSelTopic.CPtr());
	} else {
		strNewTopic =
				StackData.strSelTopic.CPtr() + (!StrCmp(StackData.strSelTopic, HelpOnHelpTopic) ? 1 : 0);
	}

	// удалим ссылку на .DLL
	wchar_t *lpwszNewTopic = strNewTopic.GetBuffer();
	wchar_t *p = wcsrchr(lpwszNewTopic, HelpEndLink);

	if (p) {
		if (!IsSlash(*(p - 1))) {
			const wchar_t *p2 = p;

			while (p >= lpwszNewTopic) {
				if (IsSlash(*p)) {
					//++p;
					if (*p) {
						StackData.Flags|= FHELP_CUSTOMFILE;
						StackData.strHelpMask = p + 1;
						wchar_t *lpwszMask = StackData.strHelpMask.GetBuffer();
						*wcsrchr(lpwszMask, HelpEndLink) = 0;
						StackData.strHelpMask.ReleaseBuffer();
					}

					wmemmove(p, p2, StrLength(p2) + 1);
					const wchar_t *p3 = wcsrchr(StackData.strHelpMask, L'.');

					if (p3 && StrCmpI(p3, L".hlf"))
						StackData.strHelpMask.Clear();

					break;
				}

				--p;
			}
		} else {
			StackData.Flags&= ~FHELP_CUSTOMFILE;
			StackData.Flags|= FHELP_CUSTOMPATH;
		}
	}

	strNewTopic.ReleaseBuffer();

	//_SVS(SysLog(L"HelpMask=%ls NewTopic=%ls",StackData.HelpMask,NewTopic));
	if (StackData.strSelTopic.At(0) != L':'
			&& (StrCmpI(StackData.strSelTopic, PluginContents)
					|| StrCmpI(StackData.strSelTopic, FoundContents))) {
		if (!(StackData.Flags & FHELP_CUSTOMFILE) && wcsrchr(strNewTopic, HelpEndLink)) {
			StackData.strHelpMask.Clear();
		}
	} else {
		StackData.strHelpMask.Clear();
	}

	StackData.strHelpTopic = strNewTopic;

	if (!(StackData.Flags & FHELP_CUSTOMFILE))
		StackData.strHelpPath.Clear();

	if (!ReadHelp(StackData.strHelpMask)) {
		StackData.strHelpTopic = strNewTopic;

		if (StackData.strHelpTopic.At(0) == HelpBeginLink) {
			if (StackData.strHelpTopic.RPos(pos, HelpEndLink)) {
				StackData.strHelpTopic.Truncate(pos + 1);
				StackData.strHelpTopic+= HelpContents;
			}
		}

		StackData.strHelpPath.Clear();
		ReadHelp(StackData.strHelpMask);
	}

	ScreenObject::Flags.Clear(FHELPOBJ_ERRCANNOTOPENHELP);

	if (!HelpList.getSize()) {
		ErrorHelp = TRUE;

		if (!(StackData.Flags & FHELP_NOSHOWERROR)) {
			Message(MSG_WARNING, 1, Msg::HelpTitle, Msg::HelpTopicNotFound, StackData.strHelpTopic, Msg::Ok);
		}

		return FALSE;
	}

	// ResizeConsole();
	if (IsNewTopic
			|| !(StrCmpI(StackData.strSelTopic, PluginContents)
					|| StrCmpI(StackData.strSelTopic, FoundContents))		// Это неприятный костыль :-((
	)
		MoveToReference(1, 1);

	// FrameManager->ImmediateHide();
	FrameManager->RefreshFrame();
	return TRUE;
}

int Help::ProcessMouse(MOUSE_EVENT_RECORD *MouseEvent)
{
	if (HelpKeyBar.ProcessMouse(MouseEvent))
		return TRUE;

	if (MouseEvent->dwButtonState & FROM_LEFT_2ND_BUTTON_PRESSED && MouseEvent->dwEventFlags != MOUSE_MOVED) {
		ProcessKey(KEY_ENTER);
		return TRUE;
	}

	int MsX, MsY;
	MsX = MouseEvent->dwMousePosition.X;
	MsY = MouseEvent->dwMousePosition.Y;

	if ((MsX < X1 || MsY < Y1 || MsX > X2 || MsY > Y2) && MouseEventFlags != MOUSE_MOVED) {
		if (Flags.Check(HELPMODE_CLICKOUTSIDE)) {
			// Вываливаем если предыдущий эвент не был двойным кликом
			if (PreMouseEventFlags != DOUBLE_CLICK)
				ProcessKey(KEY_ESC);
		}

		if (MouseEvent->dwButtonState)
			Flags.Set(HELPMODE_CLICKOUTSIDE);

		return TRUE;
	}

	if (MouseX == X2 && (MouseEvent->dwButtonState & FROM_LEFT_1ST_BUTTON_PRESSED)) {
		int ScrollY = Y1 + FixSize + 1;
		int Height = Y2 - Y1 - FixSize - 1;

		if (MouseY == ScrollY) {
			while (IsMouseButtonPressed())
				ProcessKey(KEY_UP);

			return TRUE;
		}

		if (MouseY == ScrollY + Height - 1) {
			while (IsMouseButtonPressed())
				ProcessKey(KEY_DOWN);

			return TRUE;
		}
	}

	/*
		$ 15.03.2002 DJ
		обработаем щелчок в середине скроллбара
	*/
	if (MouseX == X2) {
		int ScrollY = Y1 + FixSize + 1;
		int Height = Y2 - Y1 - FixSize - 1;

		if (StrCount > Height) {
			while (IsMouseButtonPressed()) {
				if (MouseY > ScrollY && MouseY < ScrollY + Height + 1) {
					StackData.CurX = StackData.CurY = 0;
					StackData.TopStr =
							(MouseY - ScrollY - 1) * (StrCount - FixCount - Height + 1) / (Height - 2);
					FastShow();
				}
			}

			return TRUE;
		}
	}

	// DoubliClock - свернуть/развернуть хелп.
	if (MouseEvent->dwEventFlags == DOUBLE_CLICK && (MouseEvent->dwButtonState & FROM_LEFT_1ST_BUTTON_PRESSED)
			&& MouseEvent->dwMousePosition.Y < Y1 + 1 + FixSize) {
		ProcessKey(KEY_F5);
		return TRUE;
	}

	if (MouseEvent->dwMousePosition.Y < Y1 + 1 + FixSize) {
		while (IsMouseButtonPressed() && MouseY < Y1 + 1 + FixSize)
			ProcessKey(KEY_UP);

		return TRUE;
	}

	if (MouseEvent->dwMousePosition.Y >= Y2) {
		while (IsMouseButtonPressed() && MouseY >= Y2)
			ProcessKey(KEY_DOWN);

		return TRUE;
	}

	/*
		$ 26.11.2001 VVM
		+ Запомнить нажатие клавиши мышки и только в этом случае реагировать при отпускании
	*/
	if (!MouseEvent->dwEventFlags
			&& (MouseEvent->dwButtonState & (FROM_LEFT_1ST_BUTTON_PRESSED | RIGHTMOST_BUTTON_PRESSED)))
		MouseDown = TRUE;

	if (!MouseEvent->dwEventFlags
			&& !(MouseEvent->dwButtonState & (FROM_LEFT_1ST_BUTTON_PRESSED | RIGHTMOST_BUTTON_PRESSED))
			&& MouseDown && !StackData.strSelTopic.IsEmpty()) {
		MouseDown = FALSE;
		ProcessKey(KEY_ENTER);
	}

	if (MsX != PrevMouseX || MsY != PrevMouseY) {
		StackData.CurX = MsX - X1 - 1;
		StackData.CurY = MsY - Y1 - 1 - FixSize;
	}

	FastShow();
	WINPORT(Sleep)(10);

	return TRUE;
}

int Help::IsReferencePresent()
{
	CorrectPosition();
	int StrPos = FixCount + StackData.TopStr + StackData.CurY;

	if (StrPos >= StrCount) {
		return FALSE;
	}

	const HelpRecord *rec = GetHelpItem(StrPos);
	wchar_t *OutStr = rec ? rec->HelpStr : nullptr;
	return (OutStr && wcschr(OutStr, L'@') && wcschr(OutStr, L'~'));
}

const HelpRecord *Help::GetHelpItem(int Pos)
{
	if ((unsigned int)Pos < HelpList.getSize())
		return HelpList.getItem(Pos);
	return nullptr;
}

void Help::MoveToReference(int Forward, int CurScreen)
{
	int StartSelection = !StackData.strSelTopic.IsEmpty();
	int SaveCurX = StackData.CurX;
	int SaveCurY = StackData.CurY;
	int SaveTopStr = StackData.TopStr;
	BOOL ReferencePresent;
	StackData.strSelTopic.Clear();
	Lock();

	if (!ErrorHelp)
		while (StackData.strSelTopic.IsEmpty()) {
			ReferencePresent = IsReferencePresent();

			if (Forward) {
				if (!StackData.CurX && !ReferencePresent)
					StackData.CurX = X2 - X1 - 2;

				if (++StackData.CurX >= X2 - X1 - 2) {
					StartSelection = 0;
					StackData.CurX = 0;
					StackData.CurY++;

					if (StackData.TopStr + StackData.CurY >= StrCount - FixCount
							|| (CurScreen && StackData.CurY > Y2 - Y1 - 2 - FixSize))
						break;
				}
			} else {
				if (StackData.CurX == X2 - X1 - 2 && !ReferencePresent)
					StackData.CurX = 0;

				if (--StackData.CurX < 0) {
					StartSelection = 0;
					StackData.CurX = X2 - X1 - 2;
					StackData.CurY--;

					if (StackData.TopStr + StackData.CurY < 0 || (CurScreen && StackData.CurY < 0))
						break;
				}
			}

			FastShow();

			if (StackData.strSelTopic.IsEmpty())
				StartSelection = 0;
			else {
				// небольшая заплата, артефакты есть но уже меньше :-)
				if (ReferencePresent && CurScreen)
					StartSelection = 0;

				if (StartSelection)
					StackData.strSelTopic.Clear();
			}
		}

	Unlock();

	if (StackData.strSelTopic.IsEmpty()) {
		StackData.CurX = SaveCurX;
		StackData.CurY = SaveCurY;
		StackData.TopStr = SaveTopStr;
	}

	FastShow();
}

static int __cdecl CmpItems(const HelpRecord **el1, const HelpRecord **el2)
{
	if (el1 == el2)
		return 0;

	int result = StrCmpI((**el1).HelpStr, (**el2).HelpStr);
	if (!result)
		return 0;
	else if (result < 0)
		return -1;
	else
		return 1;
}

void Help::Search(FILE *HelpFile, uintptr_t nCodePage)
{
	StrCount = 0;
	FixCount = 1;
	FixSize = 2;
	StackData.TopStr = 0;
	TopicFound = TRUE;
	StackData.CurX = StackData.CurY = 0;
	strCtrlColorChar.Clear();

	FARString strTitleLine = strLastSearchStr;
	AddTitle(strTitleLine);

	bool TopicFound = false;
	OldGetFileString GetStr(HelpFile);
	int nStrLength;
	wchar_t *ReadStr;
	FARString strCurTopic, strEntryName, strReadStr, strLastSearchStrU = strLastSearchStr;

	strLastSearchStrU.Upper();

	for (;;) {
		if (GetStr.GetString(&ReadStr, nCodePage, nStrLength) <= 0) {
			break;
		}

		strReadStr = ReadStr;
		RemoveTrailingSpaces(strReadStr);

		if (strReadStr.At(0) == L'@' && !(strReadStr.At(1) == L'+' || strReadStr.At(1) == L'-')
				&& !strReadStr.Contains(L'='))		// && !TopicFound)
		{
			strEntryName = L"";
			strCurTopic = L"";
			RemoveExternalSpaces(strReadStr);
			if (StrCmpI(strReadStr.CPtr() + 1, HelpContents)) {
				strCurTopic = strReadStr;
				TopicFound = true;
			}
		} else if (TopicFound && strReadStr.At(0) == L'$' && strReadStr.At(1) && !strCurTopic.IsEmpty()) {
			strEntryName = strReadStr.CPtr() + 1;
			RemoveExternalSpaces(strEntryName);
			RemoveChar(strEntryName, L'#', false);
		}

		if (TopicFound && !strEntryName.IsEmpty()) {
			// !!!BUGBUG: необходимо "очистить" строку strReadStr от элементов разметки !!!

			FARString ReplaceStr;
			int CurPos = 0;
			int SearchLength;
			bool Result = SearchString(strReadStr, (int)strReadStr.GetLength(), strLastSearchStr, ReplaceStr,
					CurPos, 0, LastSearchCase, LastSearchWholeWords, false, LastSearchRegexp, &SearchLength);

			if (Result) {
				FARString strHelpLine;
				strHelpLine.Format(L"   ~%ls~%ls@", strEntryName.CPtr(), strCurTopic.CPtr());
				AddLine(strHelpLine);
				strCurTopic = L"";
				strEntryName = L"";
				TopicFound = false;
			}
		}
	}

	AddLine(L"");
	MoveToReference(1, 1);
}

void Help::ReadDocumentsHelp(int TypeIndex)
{
	HelpList.Free();

	strCurPluginContents.Clear();
	StrCount = 0;
	FixCount = 1;
	FixSize = 2;
	StackData.TopStr = 0;
	TopicFound = TRUE;
	StackData.CurX = StackData.CurY = 0;
	strCtrlColorChar.Clear();
	const wchar_t *PtrTitle = 0, *ContentsName = 0;
	FARString strPath, strFullFileName;

	switch (TypeIndex) {
		case HIDX_PLUGINS:
			PtrTitle = Msg::PluginsHelpTitle;
			ContentsName = L"PluginContents";
			break;
	}

	AddTitle(PtrTitle);
	/*
		TODO:
		1. Поиск (для "документов") не только в каталоге Documets, но
		и в плагинах
	*/
	int OldStrCount = StrCount;

	switch (TypeIndex) {
		case HIDX_PLUGINS: {
			for (int I = 0; I < CtrlObject->Plugins.GetPluginsCount(); I++) {
				strPath = CtrlObject->Plugins.GetPlugin(I)->GetModuleName();
				CutToSlash(strPath);
				UINT nCodePage = CP_UTF8;
				FILE *HelpFile =
						OpenLangFile(strPath, HelpFileMask, Opt.strHelpLanguage, strFullFileName, nCodePage);

				if (HelpFile) {
					FARString strEntryName, strHelpLine, strSecondParam;

					if (GetLangParam(HelpFile, ContentsName, &strEntryName, &strSecondParam, nCodePage)) {
						if (!strSecondParam.IsEmpty())
							strHelpLine.Format(L"   ~%ls,%ls~@" HelpFormatLink L"@", strEntryName.CPtr(),
									strSecondParam.CPtr(), strPath.CPtr(), HelpContents);
						else
							strHelpLine.Format(L"   ~%ls~@" HelpFormatLink L"@", strEntryName.CPtr(),
									strPath.CPtr(), HelpContents);

						AddLine(strHelpLine);
					}

					fclose(HelpFile);
				}
			}

			break;
		}
	}

	// сортируем по алфавиту
	HelpList.Sort(reinterpret_cast<TARRAYCMPFUNC>(CmpItems), OldStrCount);

	// $ 26.06.2000 IS - Устранение глюка с хелпом по f1, shift+f2, end (решение предложил IG)
	AddLine(L"");
}

// Формирование топика с учетом разных факторов
FARString &Help::MkTopic(INT_PTR PluginNumber, const wchar_t *HelpTopic, FARString &strTopic)
{
	strTopic.Clear();

	if (HelpTopic && *HelpTopic) {
		if (*HelpTopic == L':') {
			strTopic = (HelpTopic + 1);
		} else {
			Plugin *pPlugin = (Plugin *)PluginNumber;

			if (PluginNumber != -1 && pPlugin && *HelpTopic != HelpBeginLink) {
				strTopic.Format(HelpFormatLinkModule, pPlugin->GetModuleName().CPtr(), HelpTopic);
			} else {
				strTopic = HelpTopic;
			}

			if (strTopic.At(0) == HelpBeginLink) {
				wchar_t *Ptr, *Ptr2;
				wchar_t *lpwszTopic = strTopic.GetBuffer(strTopic.GetLength() * 2);		// BUGBUG

				if (!(Ptr = wcschr(lpwszTopic, HelpEndLink))) {
					*lpwszTopic = 0;
				} else {
					if (!Ptr[1])							// Вона как поперло то...
						wcscat(lpwszTopic, HelpContents);	// ... значит покажем основную тему. //BUGBUG

					/*
						А вот теперь разгребем...
						Формат может быть :
							"<FullPath>Topic" или "<FullModuleName>Topic"
						Для случая "FullPath" путь ДОЛЖЕН заканчиваться СЛЕШЕМ!
						Т.о. мы отличим ЧТО ЭТО - имя модуля или путь!
					*/
					Ptr2 = Ptr - 1;

					if (!IsSlash(*Ptr2))	// Это имя модуля?
					{
						// значит удалим это чертово имя :-)
						if (!(Ptr2 = const_cast<wchar_t *>(LastSlash(lpwszTopic))))		// ВО! Фигня какая-то :-(
							*lpwszTopic = 0;
					}

					if (*lpwszTopic) {
						/*
							$ 21.08.2001 KM
							- Неверно создавался топик с учётом нового правила,
							в котором путь для топика должен заканчиваться "/".
						*/
						wmemmove(Ptr2 + 1, Ptr, StrLength(Ptr) + 1);	//???
																		// А вот ЗДЕСЬ теперь все по правилам Help API!
					}
				}

				strTopic.ReleaseBuffer();
			}
		}
	}

	return strTopic;
}

void Help::SetScreenPosition()
{
	if (Opt.FullScreenHelp) {
		HelpKeyBar.Hide();
		SetPosition(0, 0, ScrX, ScrY);
	} else {
		SetPosition(4, 2, ScrX - 4, ScrY - 2);
	}

	Show();
}

void Help::InitKeyBar()
{
	HelpKeyBar.SetAllGroup(KBL_MAIN, Msg::HelpF1, 12);
	HelpKeyBar.SetAllGroup(KBL_SHIFT, Msg::HelpShiftF1, 12);
	HelpKeyBar.SetAllGroup(KBL_ALT, Msg::HelpAltF1, 12);
	HelpKeyBar.SetAllGroup(KBL_CTRL, Msg::HelpCtrlF1, 12);
	HelpKeyBar.SetAllGroup(KBL_CTRLSHIFT, Msg::HelpCtrlShiftF1, 12);
	HelpKeyBar.SetAllGroup(KBL_CTRLALT, Msg::HelpCtrlAltF1, 12);
	HelpKeyBar.SetAllGroup(KBL_ALTSHIFT, Msg::HelpAltShiftF1, 12);
	HelpKeyBar.SetAllGroup(KBL_CTRLALTSHIFT, Msg::HelpCtrlAltShiftF1, 12);
	// Уберем лишнее с глаз долой
	HelpKeyBar.Change(KBL_SHIFT, L"", 3 - 1);
	HelpKeyBar.Change(KBL_SHIFT, L"", 7 - 1);
	HelpKeyBar.ReadRegGroup(L"Help", Opt.strLanguage);
	HelpKeyBar.SetAllRegGroup();
	SetKeyBar(&HelpKeyBar);
}

void Help::OnChangeFocus(int Focus)
{
	if (Focus) {
		DisplayObject();
	}
}

void Help::ResizeConsole()
{
	int OldIsNewTopic = IsNewTopic;
	BOOL ErrCannotOpenHelp = ScreenObject::Flags.Check(FHELPOBJ_ERRCANNOTOPENHELP);
	ScreenObject::Flags.Set(FHELPOBJ_ERRCANNOTOPENHELP);
	IsNewTopic = FALSE;
	delete TopScreen;
	TopScreen = nullptr;
	Hide();

	if (Opt.FullScreenHelp) {
		HelpKeyBar.Hide();
		SetPosition(0, 0, ScrX, ScrY);
	} else
		SetPosition(4, 2, ScrX - 4, ScrY - 2);

	ReadHelp(StackData.strHelpMask);
	ErrorHelp = FALSE;
	// StackData.CurY--; // ЭТО ЕСМЬ КОСТЫЛЬ (пусть пока будет так!)
	StackData.CurX--;
	MoveToReference(1, 1);
	IsNewTopic = OldIsNewTopic;
	ScreenObject::Flags.Change(FHELPOBJ_ERRCANNOTOPENHELP, ErrCannotOpenHelp);
	FrameManager->ImmediateHide();
	FrameManager->RefreshFrame();
}

int Help::FastHide()
{
	return Opt.AllCtrlAltShiftRule & CASR_HELP;
}

int Help::GetTypeAndName(FARString &strType, FARString &strName)
{
	strType = Msg::HelpType;
	strName = strFullHelpPathName;
	return (MODALTYPE_HELP);
}

/* ------------------------------------------------------------------ */
void CallBackStack::ClearStack()
{
	while (!isEmpty())
		Pop();
}

int CallBackStack::Pop(StackHelpData *Dest)
{
	if (!isEmpty()) {
		ListNode *oldTop = topOfStack;
		topOfStack = topOfStack->Next;

		if (Dest) {
			Dest->strHelpTopic = oldTop->strHelpTopic;
			Dest->strHelpPath = oldTop->strHelpPath;
			Dest->strSelTopic = oldTop->strSelTopic;
			Dest->strHelpMask = oldTop->strHelpMask;
			Dest->Flags = oldTop->Flags;
			Dest->TopStr = oldTop->TopStr;
			Dest->CurX = oldTop->CurX;
			Dest->CurY = oldTop->CurY;
		}

		delete oldTop;
		return TRUE;
	}

	return FALSE;
}

void CallBackStack::Push(const StackHelpData *Data)
{
	topOfStack = new ListNode(Data, topOfStack);
}

void CallBackStack::PrintStack(const wchar_t *Title)
{
#if defined(SYSLOG)
	int I = 0;
	ListNode *Ptr = topOfStack;
	SysLog(L"Return Stack (%ls)", Title);
	SysLog(1);

	while (Ptr) {
		SysLog(L"%03d HelpTopic='%ls' HelpPath='%ls' HelpMask='%ls'", I++, Ptr->strHelpTopic.CPtr(),
				Ptr->strHelpPath.CPtr(), Ptr->strHelpMask.CPtr());
		Ptr = Ptr->Next;
	}

	SysLog(-1);
#endif
}
