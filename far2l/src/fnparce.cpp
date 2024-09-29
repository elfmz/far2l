/*
fnparce.cpp

Парсер файловых ассоциаций
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

#include "fnparce.hpp"
#include "panel.hpp"
#include "keys.hpp"
#include "ctrlobj.hpp"
#include "cmdline.hpp"
#include "filepanels.hpp"
#include "dialog.hpp"
#include "lang.hpp"
#include "message.hpp"
#include "pathmix.hpp"
#include "strmix.hpp"
#include "panelmix.hpp"
#include "mix.hpp"

struct TSubstData
{
	// параметры функции SubstFileName
	const wchar_t *Name;	// Длинное имя

	FARString *pListName;
	FARString *pAnotherListName;

	// локальные переменные
	FARString strAnotherName;
	FARString strNameOnly;
	FARString strAnotherNameOnly;
	FARString strCmdDir;
	int PreserveLFN;
	int PassivePanel;

	Panel *AnotherPanel;
	Panel *ActivePanel;
};

static int
IsReplaceVariable(const wchar_t *str, int *scr = nullptr, int *end = nullptr, int *beg_scr_break = nullptr,
		int *end_scr_break = nullptr, int *beg_txt_break = nullptr, int *end_txt_break = nullptr);

static int ReplaceVariables(FARString &strStr, TSubstData *PSubstData);

// Str=if exist !#!\!^!.! far:edit < diff -c -p "!#!\!^!.!" !\!.!

static const wchar_t *_SubstFileName(const wchar_t *CurStr, TSubstData *PSubstData, FARString &strOut)
{
	// рассмотрим переключатели активности/пассивности панели.
	if (!StrCmpN(CurStr, L"!#", 2)) {
		CurStr+= 2;
		PSubstData->PassivePanel = TRUE;
		return CurStr;
	}

	if (!StrCmpN(CurStr, L"!^", 2)) {
		CurStr+= 2;
		PSubstData->PassivePanel = FALSE;
		return CurStr;
	}

	// !! символ '!'
	if (!StrCmpN(CurStr, L"!!", 2) && CurStr[2] != L'?') {
		strOut+= L"!";
		CurStr+= 2;
		return CurStr;
	}

	// !.!      Длинное имя файла с расширением
	if (!StrCmpN(CurStr, L"!.!", 3) && CurStr[3] != L'?') {
		FARString filename;
		if (PSubstData->PassivePanel)
			filename = PSubstData->strAnotherName;
		else
			filename = PSubstData->Name;
		EscapeSpace(filename);
		strOut+= filename;

		CurStr+= 3;
		return CurStr;
	}

	// !~       Короткое имя файла без расширения
	/*if (!StrCmpN(CurStr,L"!~",2))
	{
		FARString filename = PSubstData->PassivePanel ? PSubstData->strAnothertNameOnly : PSubstData->strNameOnly;
		EscapeSpace(filename);
		strOut += filename;
		CurStr+=2;
		return CurStr;
	}*/

	// !`  Длинное расширение файла без имени
	if (!StrCmpN(CurStr, L"!`", 2)) {
		const wchar_t *Ext;

		{
			Ext = wcsrchr((PSubstData->PassivePanel ? PSubstData->strAnotherName.CPtr() : PSubstData->Name),
					L'.');
			CurStr+= 2;
		}

		if (Ext && *Ext)
			strOut+= ++Ext;

		return CurStr;
	}

	// !& !&~  список файлов разделенных пробелом.
	if (/*remove as windows legacy (!StrCmpN(CurStr, L"!&~", 3) && CurStr[3] != L'?')
			||*/
		 (!StrCmpN(CurStr, L"!&", 2) && CurStr[2] != L'?')) {
		FARString strFileNameL;
		Panel *WPanel = PSubstData->PassivePanel ? PSubstData->AnotherPanel : PSubstData->ActivePanel;
		DWORD FileAttrL;
		int CntSkip = 2;

		/*remove as windows legacy
		if (CurStr[2] == L'~') {
			CntSkip++;
		}*/

		WPanel->GetSelNameCompat(nullptr, FileAttrL);
		int First = TRUE;

		while (WPanel->GetSelNameCompat(&strFileNameL, FileAttrL)) {
			EscapeSpace(strFileNameL); //QuoteSpaceOnly(strFileNameL);

			// Вот здесь фиг его знает - нужно/ненужно...
			// если будет нужно - раскомментируем :-)
			//			if(FileAttrL & FILE_ATTRIBUTE_DIRECTORY)
			//				AddEndSlash(FileNameL);
			// А нужен ли нам пробел в самом начале?
			if (First)
				First = FALSE;
			else
				strOut+= L" ";

			strOut+= strFileNameL;
		}

		CurStr+= CntSkip;
		return CurStr;
	}

	// !@  Имя файла, содержащего имена помеченных файлов
	// !$!      Имя файла, содержащего короткие имена помеченных файлов
	// Ниже идет совмещение кода для разбора как !@! так и !$!
	// Вообще-то (по исторической справедливости как бы) - в !$! нужно выбрасывать модификаторы Q и A
	// Но нафиг нада:)
	if (!StrCmpN(CurStr, L"!@", 2) || !StrCmpN(CurStr, L"!$", 2)) {
		FARString *pListName;
		FARString *pAnotherListName;

		pListName = PSubstData->pListName;
		pAnotherListName = PSubstData->pAnotherListName;

		wchar_t Modifiers[32] = L"";
		const wchar_t *Ptr;

		if ((Ptr = wcschr(CurStr + 2, L'!'))) {
			if (Ptr[1] != L'?') {
				far_wcsncpy(Modifiers, CurStr + 2,
						Min(ARRAYSIZE(Modifiers), static_cast<size_t>(Ptr - (CurStr + 2) + 1)));

				if (pListName) {
					if (PSubstData->PassivePanel
							&& (!pAnotherListName->IsEmpty()
									|| PSubstData->AnotherPanel->MakeListFile(*pAnotherListName, Modifiers))) {
						strOut+= *pAnotherListName;
					}

					if (!PSubstData->PassivePanel
							&& (!pListName->IsEmpty()
									|| PSubstData->ActivePanel->MakeListFile(*pListName, Modifiers))) {
						strOut+= *pListName;
					}
				} else {
					strOut+= CurStr;
					strOut+= Modifiers;
					strOut+= L"!";
				}

				CurStr+= Ptr - CurStr + 1;
				return CurStr;
			}
		}
	}

	/* Remove !-!, !+!, !: as windows legacy
	// !-!      Короткое имя файла с расширением
	if (!StrCmpN(CurStr, L"!-!", 3) && CurStr[3] != L'?') {
		FARString filename;
		if (PSubstData->PassivePanel)
			filename = PSubstData->strAnotherName;
		else
			filename = PSubstData->Name;

		EscapeSpace(filename);
		strOut+= filename;
		CurStr+= 3;
		return CurStr;
	}

	// !+!      Аналогично !-!, но если длинное имя файла утеряно
	//          после выполнения команды, FAR восстановит его
	if (!StrCmpN(CurStr, L"!+!", 3) && CurStr[3] != L'?') {
		FARString filename;
		if (PSubstData->PassivePanel)
			filename = PSubstData->strAnotherName;
		else
			filename = PSubstData->Name;

		EscapeSpace(filename);
		strOut+= filename;

		CurStr+= 3;
		PSubstData->PreserveLFN = TRUE;
		return CurStr;
	}

	// !:       Текущий диск
	if (!StrCmpN(CurStr, L"!:", 2)) {
		fprintf(stderr, "%s: deprecated CurStr='%ls'\n", __FUNCTION__, CurStr);
		CurStr+= 2;
		return CurStr;
	}
	*/

	// !\       Текущий путь
	// !/       Короткое имя текущего пути
	// Ниже идет совмещение кода для разбора как !\ так и !/
	if (!StrCmpN(CurStr, L"!/", 2) || !StrCmpN(CurStr, L"!=/", 3)
		|| !StrCmpN(CurStr,L"!\\",2) || !StrCmpN(CurStr,L"!=\\",3))
	{
		FARString strCurDir;
		int RealPath = CurStr[1] == L'=' ? 1 : 0;

		if (PSubstData->PassivePanel)
			PSubstData->AnotherPanel->GetCurDir(strCurDir);
		else
			strCurDir = PSubstData->strCmdDir;

		if (RealPath) {
			_MakePath1(PSubstData->PassivePanel ? KEY_ALTSHIFTBACKBRACKET : KEY_ALTSHIFTBRACKET, strCurDir,
					L"");
			Unquote(strCurDir);
		}

		AddEndSlash(strCurDir);

		CurStr+= 2 + RealPath;

		if (*CurStr == L'!') {
			if (FindAnyOfChars(PSubstData->PassivePanel
								? PSubstData->strAnotherName.CPtr()
								: PSubstData->Name,
						"/:"))
				strCurDir.Clear();
		}
		EscapeSpace(strCurDir);

		strOut+= strCurDir;
		return CurStr;
	}

	// !?<title>?<init>!
	if (!StrCmpN(CurStr, L"!?", 2) && wcschr(CurStr + 2, L'!')) {
		int j;
		int i = IsReplaceVariable(CurStr);

		if (i == -1)	// if bad format string
		{				// skip 1 char
			j = 1;
		} else {
			j = i + 1;
		}

		strOut.Append(CurStr, j);
		CurStr+= j;
		return CurStr;
	}

	// !        Длинное имя файла без расширения
	if (*CurStr == L'!') {
		FARString filename = PointToName(
				PSubstData->PassivePanel ? PSubstData->strAnotherNameOnly : PSubstData->strNameOnly);
		EscapeSpace(filename);
		strOut+= filename;
		CurStr++;
	}

	return CurStr;
}

/*
	SubstFileName()
	Преобразование метасимволов ассоциации файлов в реальные значения

*/
int SubstFileName(FARString &strStr,	// результирующая строка
		const wchar_t *Name,			// Длинное имя
		FARString *pListName, FARString *pAnotherListName,
		int IgnoreInput,				// TRUE - не исполнять "!?<title>?<init>!"
		const wchar_t *CmdLineDir)		// Каталог исполнения
{
	if (pListName)
		pListName->Clear();

	if (pAnotherListName)
		pAnotherListName->Clear();

	/*
		$ 19.06.2001 SVS
		ВНИМАНИЕ! Для альтернативных метасимволов, не основанных на "!",
		нужно будет либо убрать эту проверку либо изменить условие (последнее
		предпочтительнее!)
	*/
	if (!wcschr(strStr, L'!'))
		return FALSE;

	TSubstData SubstData, *PSubstData = &SubstData;
	PSubstData->Name = Name;							// Длинное имя
	PSubstData->pListName = pListName;					// Длинное имя файла-списка
	PSubstData->pAnotherListName = pAnotherListName;	// Длинное имя файла-списка
	// Если имя текущего каталога не задано...
	if (CmdLineDir)
		PSubstData->strCmdDir = CmdLineDir;
	else	// ...спросим у ком.строки
		CtrlObject->CmdLine->GetCurDir(PSubstData->strCmdDir);

	size_t pos;
	// Предварительно получим некоторые "константы" :-)
	PSubstData->strNameOnly = Name;

	if (PSubstData->strNameOnly.RPos(pos, L'.'))
		PSubstData->strNameOnly.Truncate(pos);

	PSubstData->ActivePanel = CtrlObject->Cp()->ActivePanel;
	PSubstData->AnotherPanel = CtrlObject->Cp()->GetAnotherPanel(PSubstData->ActivePanel);
	PSubstData->AnotherPanel->GetCurName(PSubstData->strAnotherName);
	PSubstData->strAnotherNameOnly = PSubstData->strAnotherName;

	if (PSubstData->strAnotherNameOnly.RPos(pos, L'.'))
		PSubstData->strAnotherNameOnly.Truncate(pos);

	PSubstData->PreserveLFN = FALSE;
	PSubstData->PassivePanel = FALSE;	// первоначально речь идет про активную панель!
	FARString strTmp = strStr;

	if (!IgnoreInput)
		ReplaceVariables(strTmp,PSubstData);

	const wchar_t *CurStr = strTmp;
	FARString strOut;

	while (*CurStr) {
		if (*CurStr == L'!') {
			CurStr = _SubstFileName(CurStr, PSubstData, strOut);
		} else {
			strOut.Append(CurStr, 1);
			CurStr++;
		}
	}

	strStr = strOut;

	return (PSubstData->PreserveLFN);
}

int ReplaceVariables(FARString &strStr, TSubstData *PSubstData)
{
	const int MaxSize = 20;
	const wchar_t *Str = strStr;
	const wchar_t *const StartStr = Str;

	//	if (*Str==L'\"')
	//		while (*Str && *Str!=L'\"')
	//			Str++;

	DialogItemEx *DlgData = new DialogItemEx[MaxSize + 2];
	FARString HistoryName[MaxSize];
	int DlgSize = 0;
	int StrPos[128], StrEndPos[128], StrPosSize = 0;

	while (*Str && DlgSize < MaxSize) {
		if (*(Str++) != L'!')
			continue;

		if (!*Str)
			break;

		if (*(Str++) != L'?')
			continue;

		if (!*Str)
			break;

		// теперича все не просто
		// придется сразу определить наличие операторных скобок
		// запомнить их позицию
		int scr, end, beg_t, end_t, beg_s, end_s;
		scr = end = beg_t = end_t = beg_s = end_s = 0;
		int ii = IsReplaceVariable(Str - 2, &scr, &end, &beg_t, &end_t, &beg_s, &end_s);

		if (ii == -1) {
			delete[] DlgData;
			strStr.Clear();
			return 0;
		}

		StrEndPos[StrPosSize] = (int)(Str - StartStr - 2) + ii;		//+1
		StrPos[StrPosSize++] = (int)(Str - StartStr - 2);
		DlgData[DlgSize].Clear();
		DlgData[DlgSize].Type = DI_TEXT;
		DlgData[DlgSize].X1 = 5;
		DlgData[DlgSize].Y1 = DlgSize + 2;
		DlgData[DlgSize + 1].Clear();
		DlgData[DlgSize + 1].Type = DI_EDIT;
		DlgData[DlgSize + 1].X1 = 5;
		DlgData[DlgSize + 1].X2 = 70;
		DlgData[DlgSize + 1].Y1 = DlgSize + 3;
		DlgData[DlgSize + 1].Flags|= DIF_HISTORY | DIF_USELASTHISTORY;
		int HistoryNumber = DlgSize / 2;
		HistoryName[HistoryNumber].Format(L"UserVar%d", HistoryNumber);
		DlgData[DlgSize + 1].strHistory = HistoryName[HistoryNumber];

		if (!DlgSize) {
			DlgData[DlgSize + 1].DefaultButton = TRUE;
			DlgData[DlgSize + 1].Focus = TRUE;
		}

		FARString strTitle;

		if (scr > 2)	// if between !? and ? exist some
			strTitle.Append(Str, scr - 2);

		size_t hist_correct = 0;

		if (!strTitle.IsEmpty()) {
			if (strTitle[0] == L'$')	// begin of history name
			{
				const wchar_t *p = &strTitle[1];
				const wchar_t *p1 = wcschr(p, L'$');

				if (p1) {
					HistoryName[HistoryNumber].Copy(p, p1 - p);
					DlgData[DlgSize + 1].strHistory = HistoryName[HistoryNumber];
					strTitle = ++p1;
					hist_correct = p1 - p + 1;
				}
			}
		}

		if ((beg_t == -1) || (end_t == -1)) {
			//
		} else if ((end_t - beg_t) > 1)		// if between ( and ) exist some
		{
			FARString strTitle2;
			FARString strTitle3;
			strTitle2.Append(strTitle.CPtr() + (end_t - 2) + 1 - hist_correct, scr - end_t - 1);	// !?$zz$xxxx(fffff)ddddd
			//                  ^   ^
			strTitle3.Append(strTitle.CPtr() + (beg_t - 2) + 1 - hist_correct, end_t - beg_t - 1);	// !?$zz$xxxx(ffffff)ddddd
			//            ^    ^
			strTitle.Truncate(beg_t - 2 - hist_correct);	// !?$zz$xxxx(fffff)ddddd
			//       ^  ^
			FARString strTmp;
			const wchar_t *CurStr = strTitle3;

			while (*CurStr) {
				if (*CurStr == L'!') {
					CurStr = _SubstFileName(CurStr, PSubstData, strTmp);
				} else {
					strTmp.Append(CurStr, 1);
					CurStr++;
				}
			}

			strTitle+= strTmp;
			strTitle+= strTitle2;
		}

		// do it - типа здесь все уже раскрыто и преобразовано
		DlgData[DlgSize].strData = strTitle;
		// Заполняем поле ввода заданным шаблоном - если есть
		FARString strTxt;

		if ((end - scr) > 1)	// if between ? and ! exist some
			strTxt.Append((Str - 2) + scr + 1, (end - scr) - 1);

		if ((beg_s == -1) || (end_s == -1)) {
			//
		} else if ((end_s - beg_s) > 1)		// if between ( and ) exist some
		{
			FARString strTxt2;
			FARString strTxt3;
			strTxt2.Copy(strTxt.CPtr() + (end_s - scr), end - end_s - 1);	// !?$zz$xxxx(fffff)ddddd?rrrr(pppp)qqqqq!
			//                                  ^   ^
			strTxt3.Copy(strTxt.CPtr() + (beg_s - scr), end_s - beg_s - 1);		// !?$zz$xxxx(ffffff)ddddd?rrrr(pppp)qqqqq!
			//                              ^  ^
			strTxt.Truncate(beg_s - scr - 1);	// !?$zz$xxxx(fffff)ddddd?rrrr(pppp)qqqqq!
			//                        ^  ^
			FARString strTmp;
			const wchar_t *CurStr = strTxt3;

			while (*CurStr) {
				if (*CurStr == L'!') {
					CurStr = _SubstFileName(CurStr, PSubstData, strTmp);
				} else {
					strTmp.Append(CurStr, 1);
					CurStr++;
				}
			}

			strTxt+= strTmp;
			strTxt+= strTxt2;
		}

		DlgData[DlgSize + 1].strData = strTxt;
		apiExpandEnvironmentStrings(DlgData[DlgSize].strData, DlgData[DlgSize].strData);
		DlgSize+= 2;
	}

	if (!DlgSize) {
		delete[] DlgData;
		return 0;
	}

	DlgData[DlgSize].Clear();
	DlgData[DlgSize].Type = DI_DOUBLEBOX;
	DlgData[DlgSize].X1 = 3;
	DlgData[DlgSize].Y1 = 1;
	DlgData[DlgSize].X2 = 72;
	DlgData[DlgSize].Y2 = DlgSize + 2;
	DlgSize++;
	int ExitCode;
	{
		Dialog Dlg(DlgData, DlgSize);
		Dlg.SetPosition(-1, -1, 76, DlgSize + 3);
		Dlg.Process();
		ExitCode = Dlg.GetExitCode();
	}

	if (ExitCode == -1) {
		delete[] DlgData;
		strStr.Clear();
		return 0;
	}

	FARString strTmpStr;

	for (Str = StartStr; *Str; Str++) {
		int Replace = -1;
		int end_pos = 0;

		for (int I = 0; I < StrPosSize; I++) {
			if (Str - StartStr == StrPos[I]) {
				Replace = I;
				end_pos = StrEndPos[I];
				break;
			}
		}

		if (Replace != -1) {
			strTmpStr+= DlgData[Replace * 2 + 1].strData;
			Str = StartStr + end_pos;
		} else {
			strTmpStr.Append(Str, 1);
		}
	}

	strStr = strTmpStr;
	apiExpandEnvironmentStrings(strStr, strStr);
	delete[] DlgData;
	return 1;
}

bool Panel::MakeListFile(FARString &strListFileName, const wchar_t *Modifiers)
{
	bool Ret = false;

	if (FarMkTempEx(strListFileName)) {
		File ListFile;
		if (ListFile.Open(strListFileName, GENERIC_WRITE,
					FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, nullptr, CREATE_ALWAYS)) {
			UINT CodePage = CP_UTF8;
			LPCVOID Eol = NATIVE_EOL;
			DWORD EolSize = strlen(NATIVE_EOL);

			if (Modifiers && *Modifiers) {
				if (wcschr(Modifiers, L'A'))		// ANSI
				{
					CodePage = CP_ACP;
				} else {
					DWORD Signature = 0;
					int SignatureSize = 0;

					if (wcschr(Modifiers, L'W'))		// Wide
					{
						CodePage = CP_UTF16LE;
						Signature = SIGN_UTF16LE;
						SignatureSize = 2;
						Eol = NATIVE_EOLW;
						EolSize = sizeof(wchar_t) * wcslen(NATIVE_EOLW);
					} else {
						if (wcschr(Modifiers, L'U'))		// UTF8
						{
							CodePage = CP_UTF8;
							Signature = SIGN_UTF8;
							SignatureSize = 3;
						}
					}

					if (Signature && SignatureSize) {
						DWORD NumberOfBytesWritten;
						ListFile.Write(&Signature, SignatureSize, &NumberOfBytesWritten);
					}
				}
			}

			FARString strFileName;
			DWORD FileAttr;
			GetSelNameCompat(nullptr, FileAttr);

			while (GetSelNameCompat(&strFileName, FileAttr)) {
				if (Modifiers && *Modifiers) {
					if (wcschr(Modifiers, L'F') && PointToName(strFileName) == strFileName.CPtr())		// 'F' - использовать полный путь; //BUGBUG ?
					{
						FARString strTempFileName = strCurDir;

						AddEndSlash(strTempFileName);
						strTempFileName+= strFileName;	// BUGBUG ?
						strFileName = strTempFileName;
					}

					if (wcschr(Modifiers, L'Q'))		// 'Q' - заключать имена с пробелами в кавычки;
						QuoteSpaceOnly(strFileName);
				}

				LPCVOID Ptr = nullptr;
				LPSTR Buffer = nullptr;
				DWORD NumberOfBytesToWrite = 0, NumberOfBytesWritten = 0;

				if (CodePage == CP_WIDE_LE) {
					Ptr = strFileName.CPtr();
					NumberOfBytesToWrite = static_cast<DWORD>(strFileName.GetLength() * sizeof(WCHAR));
				} else {
					int Size = WINPORT(WideCharToMultiByte)(CodePage, 0, strFileName,
							static_cast<int>(strFileName.GetLength()), nullptr, 0, nullptr, nullptr);

					if (Size) {
						Buffer = static_cast<LPSTR>(malloc(Size));

						if (Buffer) {
							NumberOfBytesToWrite = WINPORT(WideCharToMultiByte)(CodePage, 0, strFileName,
									static_cast<int>(strFileName.GetLength()), Buffer, Size, nullptr,
									nullptr);
							Ptr = Buffer;
						}
					}
				}

				BOOL Written = ListFile.Write(Ptr, NumberOfBytesToWrite, &NumberOfBytesWritten);

				if (Buffer)
					free(Buffer);

				if (Written && NumberOfBytesWritten == NumberOfBytesToWrite) {
					if (ListFile.Write(Eol, EolSize, &NumberOfBytesWritten)
							&& NumberOfBytesWritten == EolSize) {
						Ret = true;
					}
				} else {
					Message(MSG_WARNING | MSG_ERRORTYPE, 1, Msg::Error, Msg::CannotCreateListFile,
							Msg::CannotCreateListWrite, Msg::Ok);
					apiDeleteFile(strListFileName);
					break;
				}
			}

			ListFile.Close();
		} else {
			Message(MSG_WARNING | MSG_ERRORTYPE, 1, Msg::Error, Msg::CannotCreateListFile,
					Msg::CannotCreateListTemp, Msg::Ok);
		}
	} else {
		Message(MSG_WARNING | MSG_ERRORTYPE, 1, Msg::Error, Msg::CannotCreateListFile,
				Msg::CannotCreateListTemp, Msg::Ok);
	}

	return Ret;
}

static int IsReplaceVariable(const wchar_t *str, int *scr, int *end, int *beg_scr_break, int *end_scr_break,
		int *beg_txt_break, int *end_txt_break)
// все очень сложно - посл-иe 4 указателя - это смещения от str
// начало скобок в строке описания, конец этих скобок, начало скобок в строке начального заполнения, ну и соотв конец.
// Вообще при простом вызове (который я собираюсь юзать) это выглядит просто:
// i = IsReplaceVariable(str) - ведь нам надо только проверять семантику скобок и всяких ?!
// где i - тот прыжок, который надо совершить, чтоб прыгнуть на конец ! структуры !??!
{
	const wchar_t *s = str;
	const wchar_t *scrtxt = str;
	int count_scob = 0;
	int second_count_scob = 0;
	bool was_quest = false;		//  ?
	bool was_asterics = false;	//  !
	bool in_firstpart_was_scob = false;
	const wchar_t *beg_firstpart_scob = nullptr;
	const wchar_t *end_firstpart_scob = nullptr;
	bool in_secondpart_was_scob = false;
	const wchar_t *beg_secondpart_scob = nullptr;
	const wchar_t *end_secondpart_scob = nullptr;

	if (!s)
		return -1;

	if (!StrCmpN(s, L"!?", 2))
		s = s + 2;
	else
		return -1;

	//
	for (;;)	// analyze from !? to ?
	{
		if (!*s)
			return -1;

		if (*s == L'(') {
			if (in_firstpart_was_scob) {
				// return -1;
			} else {
				in_firstpart_was_scob = true;
				beg_firstpart_scob = s;		// remember where is first break
			}

			count_scob+= 1;
		} else if (*s == L')') {
			count_scob-= 1;

			if (!count_scob) {
				if (!end_firstpart_scob)
					end_firstpart_scob = s;		// remember where is last break
			} else if (count_scob < 0)
				return -1;
		} else if ((*s == L'?')
				&& ((!beg_firstpart_scob && !end_firstpart_scob)
						|| (beg_firstpart_scob && end_firstpart_scob))) {
			was_quest = true;
		}

		s++;

		if (was_quest)
			break;
	}

	if (count_scob)
		return -1;

	scrtxt = s - 1;		// remember s for return

	for (;;)			// analyze from ? or !
	{
		if (!*s)
			return -1;

		if (*s == L'(') {
			if (in_secondpart_was_scob) {
				// return -1;
			} else {
				in_secondpart_was_scob = true;
				beg_secondpart_scob = s;	// remember where is first break
			}

			second_count_scob+= 1;
		} else if (*s == L')') {
			second_count_scob-= 1;

			if (!second_count_scob) {
				if (!end_secondpart_scob)
					end_secondpart_scob = s;	// remember where is last break
			} else if (second_count_scob < 0)
				return -1;
		} else if ((*s == L'!')
				&& ((!beg_secondpart_scob && !end_secondpart_scob)
						|| (beg_secondpart_scob && end_secondpart_scob))) {
			was_asterics = true;
		}

		s++;

		if (was_asterics)
			break;
	}

	if (second_count_scob)
		return -1;

	//
	if (scr)
		*scr = (int)(scrtxt - str);

	if (end)
		*end = (int)(s - str) - 1;

	if (in_firstpart_was_scob) {
		if (beg_scr_break)
			*beg_scr_break = (int)(beg_firstpart_scob - str);

		if (end_scr_break)
			*end_scr_break = (int)(end_firstpart_scob - str);
	} else {
		if (beg_scr_break)
			*beg_scr_break = -1;

		if (end_scr_break)
			*end_scr_break = -1;
	}

	if (in_secondpart_was_scob) {
		if (beg_txt_break)
			*beg_txt_break = (int)(beg_secondpart_scob - str);

		if (end_txt_break)
			*end_txt_break = (int)(end_secondpart_scob - str);
	} else {
		if (beg_txt_break)
			*beg_txt_break = -1;

		if (end_txt_break)
			*end_txt_break = -1;
	}

	return (int)((s - str) - 1);
}
