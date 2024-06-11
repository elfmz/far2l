/*
message.cpp

Вывод MessageBox
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

#include "message.hpp"
#include "ctrlobj.hpp"
#include "lang.hpp"
#include "colors.hpp"
#include "dialog.hpp"
#include "scrbuf.hpp"
#include "keys.hpp"
#include "interf.hpp"
#include "palette.hpp"
#include "config.hpp"
#include "mix.hpp"
#include "InterThreadCall.hpp"

static int MessageX1, MessageY1, MessageX2, MessageY2;
static FARString strMsgHelpTopic;
static int FirstButtonIndex, LastButtonIndex;
static BOOL IsWarningStyle;

LONG_PTR WINAPI MsgDlgProc(HANDLE hDlg, int Msg, int Param1, LONG_PTR Param2)
{
	switch (Msg) {
		case DN_INITDIALOG: {
			FarDialogItem di;

			for (int i = 0; SendDlgMessage(hDlg, DM_GETDLGITEMSHORT, i, (LONG_PTR)&di); i++) {
				if (di.Type == DI_EDIT) {
					COORD pos = {0, 0};
					SendDlgMessage(hDlg, DM_SETCURSORPOS, i, (LONG_PTR)&pos);
				}
			}
		} break;
		case DN_CTLCOLORDLGITEM: {
			FarDialogItem di;
			SendDlgMessage(hDlg, DM_GETDLGITEMSHORT, Param1, (LONG_PTR)&di);

			if (di.Type == DI_EDIT) {
				uint64_t *ItemColor = reinterpret_cast<uint64_t *>(Param2);
				uint64_t color = FarColorToReal(IsWarningStyle ? COL_WARNDIALOGTEXT : COL_DIALOGTEXT);
				ItemColor[0] = color;
				ItemColor[2] = color;
				return 1;
			}
		} break;
		case DN_KEY: {
			if (Param1 == FirstButtonIndex
					&& (Param2 == KEY_LEFT || Param2 == KEY_NUMPAD4 || Param2 == KEY_SHIFTTAB)) {
				SendDlgMessage(hDlg, DM_SETFOCUS, LastButtonIndex, 0);
				return TRUE;
			} else if (Param1 == LastButtonIndex
					&& (Param2 == KEY_RIGHT || Param2 == KEY_NUMPAD6 || Param2 == KEY_TAB)) {
				SendDlgMessage(hDlg, DM_SETFOCUS, FirstButtonIndex, 0);
				return TRUE;
			}
		} break;
	}

	return DefDlgProc(hDlg, Msg, Param1, Param2);
}

static int ShowMessageSynched(DWORD Flags, int Buttons, const wchar_t *Title, const wchar_t *const *Items,
		int ItemsNumber, INT_PTR PluginNumber)
{
	FARString strTempStr;
	int X1, Y1, X2, Y2;
	int Length, BtnLength, J;
	DWORD I, MaxLength, StrCount;
	BOOL ErrorSets = FALSE;
	wchar_t *PtrStr;
	const wchar_t *CPtrStr = nullptr;
	FARString strErrStr;

	if (Flags & MSG_ERRORTYPE)
		ErrorSets = GetErrorString(strErrStr);

	if (ItemsNumber < 0 || Buttons < 0 || ItemsNumber < Buttons)
		return -1;

	for (;;) {	// #1248
		StrCount = ItemsNumber - Buttons;
		if (int(ScrY) - int(StrCount) > 2 || ItemsNumber < Buttons + 1)
			break;

		--ItemsNumber;
		++Items;
	}

	// выделим память под рабочий массив указателей на строки (+запас 16)
	const wchar_t **Str =
			(const wchar_t **)malloc((ItemsNumber + ADDSPACEFORPSTRFORMESSAGE) * sizeof(wchar_t *));

	if (!Str)
		return -1;

	// предварительный обсчет максимального размера.
	for (BtnLength = 0, I = 0; I < static_cast<DWORD>(Buttons); I++)	//??
	{
		BtnLength+= HiStrCellsCount(Items[I + StrCount]) + 2 + 2 + 1;	// "[ ", " ]", " "
	}
	if (BtnLength) {
		BtnLength--;
	}

	for (MaxLength = BtnLength, I = 0; I < StrCount; I++) {
		if (static_cast<DWORD>(Length = StrLength(Items[I])) > MaxLength)
			MaxLength = Length;
	}

	// учтем так же размер заголовка
	if (Title && *Title) {
		I = (DWORD)StrLength(Title) + 2;

		if (MaxLength < I)
			MaxLength = I;
	}

	// первая коррекция максимального размера
	if (MaxLength > MAX_WIDTH_MESSAGE)
		MaxLength = MAX_WIDTH_MESSAGE;

	// теперь обработаем MSG_ERRORTYPE
	DWORD CountErrorLine = 0;

	if ((Flags & MSG_DISPLAYNOTIFY) != 0) {
		FARString strTitle(APP_BASENAME);
		if (Title && *Title)
			strTitle = Title;
		DisplayNotification(strTitle.CPtr(), ItemsNumber ? Items[0] : L"???");
	}

	if ((Flags & MSG_ERRORTYPE) && ErrorSets) {
		// подсчет количества строк во врапенном сообщении
		++CountErrorLine;
		// InsertQuote(ErrStr); // оквочим
		//  вычисление "красивого" размера
		DWORD LenErrStr = (DWORD)strErrStr.GetLength();

		if (LenErrStr > MAX_WIDTH_MESSAGE) {
			// половина меньше?
			if (LenErrStr / 2 < MAX_WIDTH_MESSAGE) {
				// а половина + 1/3?
				if ((LenErrStr + LenErrStr / 3) / 2 < MAX_WIDTH_MESSAGE)
					LenErrStr = (LenErrStr + LenErrStr / 3) / 2;
				else
					LenErrStr/= 2;
			} else
				LenErrStr = MAX_WIDTH_MESSAGE;
		} else if (LenErrStr < MaxLength)
			LenErrStr = MaxLength;

		if (MaxLength > LenErrStr && MaxLength >= MAX_WIDTH_MESSAGE)
			MaxLength = LenErrStr;

		if (MaxLength < LenErrStr && LenErrStr <= MAX_WIDTH_MESSAGE)
			MaxLength = LenErrStr;

		// а теперь проврапим
		// PtrStr=FarFormatText(ErrStr,MaxLength-(MaxLength > MAX_WIDTH_MESSAGE/2?1:0),ErrStr,sizeof(ErrStr),"\n",0); //?? MaxLength ??
		FarFormatText(strErrStr, LenErrStr, strErrStr, L"\n", 0);	//?? MaxLength ??
		PtrStr = strErrStr.GetBuffer();

		// BUGBUG: FARString не предназначен для хранения строк разделённых \0
		while ((PtrStr = wcschr(PtrStr, L'\n'))) {
			*PtrStr++ = 0;

			if (*PtrStr)
				CountErrorLine++;
		}

		strErrStr.ReleaseBuffer();

		if (CountErrorLine > ADDSPACEFORPSTRFORMESSAGE)
			CountErrorLine = ADDSPACEFORPSTRFORMESSAGE;		//??
	}

	// BUGBUG: FARString не предназначен для хранения строк разделённых \0
	//  заполняем массив...
	CPtrStr = strErrStr;

	for (I = 0; I < CountErrorLine; I++) {
		Str[I] = CPtrStr;
		CPtrStr+= StrLength(CPtrStr) + 1;

		if (!*CPtrStr)		// два идущих подряд нуля - "хандец" всему
		{
			++I;
			break;
		}
	}

	bool EmptyText = false;
	if (ItemsNumber == Buttons && !I) {
		EmptyText = true;
		Str[I] = L"";
		I++;
		StrCount++;
		ItemsNumber++;
	}

	for (J = 0; J < ItemsNumber - (EmptyText ? 1 : 0); ++J, ++I) {
		Str[I] = Items[J];
	}

	StrCount+= CountErrorLine;
	MessageX1 = X1 = (ScrX - MaxLength) / 2 - 4;
	MessageX2 = X2 = X1 + MaxLength + 9;
	Y1 = (int(ScrY) - int(StrCount)) / 2 - 2;

	if (Y1 < 0)
		Y1 = 0;

	MessageY1 = Y1;
	MessageY2 = Y2 = Y1 + int(StrCount) + 3;
	FARString strHelpTopic(strMsgHelpTopic);
	strMsgHelpTopic.Clear();
	// *** Вариант с Диалогом ***

	if (Buttons > 0) {
		DWORD ItemCount = StrCount + Buttons + 1;
		DialogItemEx *PtrMsgDlg;
		DialogItemEx *MsgDlg = new (std::nothrow) DialogItemEx[ItemCount + 1];

		if (!MsgDlg) {
			free(Str);
			return -1;
		}

		for (DWORD i = 0; i < ItemCount + 1; i++)
			MsgDlg[i].Clear();

		int RetCode;
		MessageY2 = ++Y2;
		MsgDlg[0].Type = DI_DOUBLEBOX;
		MsgDlg[0].X1 = 3;
		MsgDlg[0].Y1 = 1;
		MsgDlg[0].X2 = X2 - X1 - 3;
		MsgDlg[0].Y2 = Y2 - Y1 - 1;

		if (Title && *Title)
			MsgDlg[0].strData = Title;

		int TypeItem = DI_TEXT;
		DWORD FlagsItem = DIF_SHOWAMPERSAND;
		BOOL IsButton = FALSE;
		int CurItem = 0;
		bool StrSeparator = false;
		bool Separator = false;
		for (PtrMsgDlg = MsgDlg + 1, I = 1; I < ItemCount; ++I, ++PtrMsgDlg, ++CurItem) {
			if (I == StrCount + 1 && !StrSeparator && !Separator) {
				PtrMsgDlg->Type = DI_TEXT;
				PtrMsgDlg->Flags = DIF_SEPARATOR;
				PtrMsgDlg->Y1 = PtrMsgDlg->Y2 = I + 1;
				CurItem--;
				I--;
				Separator = true;
				continue;
			}
			if (I == StrCount + 1) {
				PtrMsgDlg->DefaultButton = TRUE;
				PtrMsgDlg->Focus = TRUE;
				TypeItem = DI_BUTTON;
				FlagsItem = DIF_CENTERGROUP;
				IsButton = TRUE;
				FirstButtonIndex = CurItem + 1;
				LastButtonIndex = CurItem;
			}

			PtrMsgDlg->Type = TypeItem;
			PtrMsgDlg->Flags|= FlagsItem;
			CPtrStr = Str[CurItem];

			if (IsButton) {
				PtrMsgDlg->Y1 = Y2 - Y1 - 2 + (Separator ? 1 : 0);
				PtrMsgDlg->strData+= CPtrStr;
				LastButtonIndex++;
			} else {
				PtrMsgDlg->X1 = (Flags & MSG_LEFTALIGN) ? 5 : -1;
				PtrMsgDlg->Y1 = I + 1;
				wchar_t Chr = *CPtrStr;

				if (Chr == L'\1' || Chr == L'\2') {
					CPtrStr++;
					PtrMsgDlg->Flags|= (Chr == 2 ? DIF_SEPARATOR2 : DIF_SEPARATOR);
					if (I == StrCount) {
						StrSeparator = true;
					}
				} else if (StrLength(CPtrStr) > X2 - X1 - 9) {
					PtrMsgDlg->Type = DI_EDIT;
					PtrMsgDlg->Flags|= DIF_READONLY | DIF_BTNNOCLOSE | DIF_SELECTONENTRY;
					PtrMsgDlg->X1 = 5;
					PtrMsgDlg->X2 = X2 - X1 - 5;
					PtrMsgDlg->strData = CPtrStr;
					continue;
				}

				PtrMsgDlg->strData = CPtrStr;	// BUGBUG, wrong len
			}
		}

		{
			if (Separator) {
				FirstButtonIndex++;
				LastButtonIndex++;
				MessageY2++;
				Y2++;
				MsgDlg[0].Y2++;
				ItemCount++;
			}
			IsWarningStyle = Flags & MSG_WARNING;
			Dialog Dlg(MsgDlg, ItemCount, MsgDlgProc);
			Dlg.SetPosition(X1, Y1, X2, Y2);

			if (!strHelpTopic.IsEmpty())
				Dlg.SetHelp(strHelpTopic);

			Dlg.SetPluginNumber(PluginNumber);	// Запомним номер плагина

			if (IsWarningStyle) {
				Dlg.SetDialogMode(DMODE_WARNINGSTYLE);
			}

			Dlg.SetDialogMode(DMODE_MSGINTERNAL);
			if (!WinPortTesting())
				FlushInputBuffer();

			if (Flags & MSG_KILLSAVESCREEN)
				SendDlgMessage((HANDLE)&Dlg, DM_KILLSAVESCREEN, 0, 0);

			Dlg.Process();
			RetCode = Dlg.GetExitCode();
		}

		delete[] MsgDlg;
		free(Str);
		return (RetCode < 0 ? RetCode : RetCode - StrCount - 1 - (Separator ? 1 : 0));
	}

	// *** Без Диалога! ***
	SetCursorType(0, 0);

	if (!(Flags & MSG_KEEPBACKGROUND)) {
		SetScreen(X1, Y1, X2, Y2, L' ', FarColorToReal((Flags & MSG_WARNING) ? COL_WARNDIALOGTEXT : COL_DIALOGTEXT));
		MakeShadow(X1 + 2, Y2 + 1, X2, Y2 + 1);
		MakeShadow(X2 + 1, Y1 + 1, X2 + 2, Y2 + 1);

		Box(X1 + 3, Y1 + 1, X2 - 3, Y2 - 1, FarColorToReal((Flags & MSG_WARNING) ? COL_WARNDIALOGBOX : COL_DIALOGBOX),
				DOUBLE_BOX);
	}

	SetFarColor((Flags & MSG_WARNING) ? COL_WARNDIALOGTEXT : COL_DIALOGTEXT);

	if (Title && *Title) {
		FARString strTempTitle = Title;

		if (strTempTitle.GetLength() > MaxLength)
			strTempTitle.Truncate(MaxLength);

		GotoXY(X1 + (X2 - X1 - 1 - (int)strTempTitle.GetLength()) / 2, Y1 + 1);
		FS << L" " << strTempTitle << L" ";
	}

	for (I = 0; I < StrCount; I++) {
		int PosX;
		CPtrStr = Str[I];
		wchar_t Chr = *CPtrStr;

		if (Chr == 1 || Chr == 2) {
			int Length = X2 - X1 - 5;

			if (Length > 1) {
				SetFarColor((Flags & MSG_WARNING) ? COL_WARNDIALOGBOX : COL_DIALOGBOX);
				GotoXY(X1 + 3, Y1 + I + 2);
				DrawLine(Length, (Chr == 2 ? 3 : 1));
				CPtrStr++;
				int TextLength = StrLength(CPtrStr);

				if (TextLength < Length) {
					GotoXY(X1 + 3 + (Length - TextLength) / 2, Y1 + I + 2);
					Text(CPtrStr);
				}

				SetFarColor((Flags & MSG_WARNING) ? COL_WARNDIALOGBOX : COL_DIALOGTEXT);
			}

			continue;
		}

		if ((Length = StrLength(CPtrStr)) > ScrX - 15)
			Length = ScrX - 15;

		int Width = X2 - X1 + 1;
		wchar_t *lpwszTemp = nullptr;

		if (Flags & MSG_LEFTALIGN) {
			lpwszTemp = (wchar_t *)malloc((Width - 10 + 1) * sizeof(wchar_t));
			swprintf(lpwszTemp, Width - 10 + 1, L"%.*ls", Width - 10, CPtrStr);
			GotoXY(X1 + 5, Y1 + I + 2);
		} else {
			PosX = X1 + (Width - Length) / 2;
			lpwszTemp = (wchar_t *)malloc(
					(PosX - X1 - 4 + Length + X2 - PosX - Length - 3 + 1) * sizeof(wchar_t));
			swprintf(lpwszTemp, PosX - X1 - 4 + Length + X2 - PosX - Length - 3 + 1, L"%*ls%.*ls%*ls",
					PosX - X1 - 4, L"", Length, CPtrStr, X2 - PosX - Length - 3, L"");
			GotoXY(X1 + 4, Y1 + I + 2);
		}

		Text(lpwszTemp);
		free(lpwszTemp);
	}

	/*
		$ 13.01.2003 IS
		- Принудительно уберем запрет отрисовки экрана, если количество кнопок
		в сообщении равно нулю и макрос закончил выполняться. Это необходимо,
		чтобы заработал прогресс-бар от плагина, который был запущен при помощи
		макроса запретом отрисовки (bugz#533).
	*/
	free(Str);

	if (!Buttons) {
		if (ScrBuf.GetLockCount() > 0 && !CtrlObject->Macro.PeekKey())
			ScrBuf.SetLockCount(0);

		ScrBuf.Flush();
	}

	return 0;
}

static int ShowMessage(DWORD Flags, int Buttons, const wchar_t *Title, const wchar_t *const *Items,
		int ItemsNumber, INT_PTR PluginNumber)
{
	return InterThreadCall<int, 0>(
			std::bind(ShowMessageSynched, Flags, Buttons, Title, Items, ItemsNumber, PluginNumber));
}

FN_NOINLINE Messager::Messager(FarLangMsg title)
{
	Add(title);
}

FN_NOINLINE Messager::Messager(const wchar_t *title)
{
	Add(title);
}

FN_NOINLINE Messager::Messager() {}

FN_NOINLINE Messager::~Messager() {}

Messager &FN_NOINLINE Messager::Add(FarLangMsg v)
{
	return Add(v.CPtr());
}

Messager &FN_NOINLINE Messager::Add(const wchar_t *v)
{
	emplace_back(v);
	return *this;
}

int FN_NOINLINE Messager::Show(DWORD Flags, int Buttons, INT_PTR PluginNumber)
{
	// ignore trailing nullptr-s
	while (!empty() && !back())
		pop_back();

	if (empty())
		return -1;

	return ShowMessage(Flags, Buttons, front(), data() + 1, (int)(size() - 1), PluginNumber);
}

int FN_NOINLINE Messager::Show(DWORD Flags, int Buttons)
{
	return Show(Flags, Buttons, -1);
}

int FN_NOINLINE Messager::Show(int Buttons)
{
	return Show(0, Buttons, -1);
}

///////////////////////////////////

ExMessager::ExMessager(FarLangMsg title) : Messager(title)
{
}

ExMessager::ExMessager(const wchar_t *title) : Messager(title)
{
}

ExMessager::ExMessager()
{
}

ExMessager::~ExMessager()
{
}

Messager &FN_NOINLINE ExMessager::AddFormat(FarLangMsg fmt, ...)
{
	va_list args;
	va_start(args, fmt);
	AddFormatV(fmt.CPtr(), args);
	va_end(args);
	return *this;
}

Messager &FN_NOINLINE ExMessager::AddFormat(const wchar_t *fmt, ...)
{
	va_list args;
	va_start(args, fmt);
	AddFormatV(fmt, args);
	va_end(args);
	return *this;
}

Messager &FN_NOINLINE ExMessager::AddFormatV(const wchar_t *fmt, va_list args)
{
	_owneds.emplace_back();
	FARStringFmtV(_owneds.back(), false, fmt, args);
	Add(_owneds.back().CPtr());
	return *this;
}

Messager &FN_NOINLINE ExMessager::AddDup(const wchar_t *v)
{
	_owneds.emplace_back(v);
	Add(_owneds.back().CPtr());
	return *this;
}

///////////////////////////////////

void GetMessagePosition(int &X1, int &Y1, int &X2, int &Y2)
{
	X1 = MessageX1;
	Y1 = MessageY1;
	X2 = MessageX2;
	Y2 = MessageY2;
}

static FARString s_ErrorString;

bool GetErrorString(FARString &strErrStr)
{
	auto err = errno;
	if (err == -1 && !s_ErrorString.IsEmpty()) {
		strErrStr = s_ErrorString;
		return true;
	}

	const char *str = strerror(err);
	if (str) {
		strErrStr.Format(L"%s (%u)", str, err);
	} else {
		strErrStr.Format(L"Error %u", err);
	}
	return true;
}

void SetErrorString(const FARString &strErrStr)
{
	s_ErrorString = strErrStr;
	errno = -1;
}

void SetMessageHelp(const wchar_t *Topic)
{
	strMsgHelpTopic = Topic;
}

/*
	$ 12.03.2002 VVM
	Новая функция - пользователь попытался прервать операцию.
	Зададим вопрос.
	Возвращает:
		FALSE - продолжить операцию
		TRUE  - прервать операцию
*/
bool AbortMessage()
{
	int Res = Message(MSG_WARNING | MSG_KILLSAVESCREEN, 2, Msg::KeyESCWasPressed,
			((Opt.Confirm.EscTwiceToInterrupt) ? Msg::DoYouWantToStopWork2 : Msg::DoYouWantToStopWork),
			Msg::Yes, Msg::No);

	if (Res == -1)	// Set "ESC" equal to "NO" button
		Res = 1;

	return ((Opt.Confirm.EscTwiceToInterrupt && Res) || (!Opt.Confirm.EscTwiceToInterrupt && !Res));
}
