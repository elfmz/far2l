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

// #define PW_to_PWZ(src,dst,lendst)    WINPORT(MultiByteToWideChar)(CP_UTF8,0, (src),-1,(dst),(int)(lendst))

static int PWZ_to_PZ(const wchar_t *src, char *dst, int lendst)
{
	ErrnoSaver ErSr;
	return WINPORT(WideCharToMultiByte)(CP_UTF8, 0, (src), -1, (dst), (int)(lendst), nullptr, nullptr);
}

static int PZ_to_PWZ(const char *src, wchar_t *dst, int lendst)
{
	ErrnoSaver ErSr;
	return WINPORT(MultiByteToWideChar)(CP_UTF8, 0, (src), -1, (dst), (int)(lendst));
}

static const char *FirstSlashA(const char *String)
{
	do {
		if (IsSlashA(*String))
			return String;
	} while (*String++);

	return nullptr;
}

bool FirstSlashA(const char *String, size_t &pos)
{
	bool Ret = false;
	const char *Ptr = FirstSlashA(String);

	if (Ptr) {
		pos = Ptr - String;
		Ret = true;
	}

	return Ret;
}

static const char *LastSlashA(const char *String)
{
	const char *Start = String;

	while (*String++)
		;

	while (--String != Start && !IsSlashA(*String))
		;

	return IsSlashA(*String) ? String : nullptr;
}

static bool LastSlashA(const char *String, size_t &pos)
{
	bool Ret = false;
	const char *Ptr = LastSlashA(String);

	if (Ptr) {
		pos = Ptr - String;
		Ret = true;
	}

	return Ret;
}

static void AnsiToUnicodeBin(const char *lpszAnsiString, wchar_t *lpwszUnicodeString, int nLength,
		UINT CodePage = CP_UTF8)
{
	if (lpszAnsiString && lpwszUnicodeString && nLength) {
		ErrnoSaver ErSr;
		wmemset(lpwszUnicodeString, 0, nLength);
		WINPORT(MultiByteToWideChar)(CodePage, 0, lpszAnsiString, nLength, lpwszUnicodeString, nLength);
	}
}

static wchar_t *AnsiToUnicodeBin(const char *lpszAnsiString, int nLength, UINT CodePage = CP_UTF8)
{
	wchar_t *lpResult = (wchar_t *)malloc(nLength * sizeof(wchar_t));
	AnsiToUnicodeBin(lpszAnsiString, lpResult, nLength, CodePage);
	return lpResult;
}

static wchar_t *AnsiToUnicode(const char *lpszAnsiString, int nMaxLength = -1, UINT CodePage = CP_UTF8)
{
	if (!lpszAnsiString)
		return nullptr;

	int nLength = (nMaxLength == -1) ? strlen(lpszAnsiString) : (int)strnlen(lpszAnsiString, nMaxLength);

	wchar_t *out = AnsiToUnicodeBin(lpszAnsiString, nLength + 1, CodePage);
	out[nLength] = 0;
	return out;
}

static char *UnicodeToAnsiBin(const wchar_t *lpwszUnicodeString, int nLength, UINT CodePage = CP_UTF8)
{
	/*
		$ 06.01.2008 TS
		! Увеличил размер выделяемой под строку памяти на 1 байт для нормальной
		работы старых плагинов, которые не знали, что надо смотреть на длину,
		а не на завершающий ноль (например в EditorGetString.StringText).
	*/
	if (!lpwszUnicodeString || (nLength < 0))
		return nullptr;

	ErrnoSaver ErSr;
	int dst_length =
			WINPORT(WideCharToMultiByte)(CodePage, 0, lpwszUnicodeString, nLength, NULL, 0, nullptr, nullptr);
	if (dst_length <= 0)
		dst_length = nLength + 1;
	else
		++dst_length;
	char *lpResult = (char *)malloc(dst_length);
	if (!dst_length)
		return NULL;
	memset(lpResult, 0, dst_length);

	if (nLength) {
		WINPORT(WideCharToMultiByte)
		(CodePage, 0, lpwszUnicodeString, nLength, lpResult, dst_length, nullptr, nullptr);
	}

	return lpResult;
}

static char *UnicodeToAnsi(const wchar_t *lpwszUnicodeString, UINT CodePage = CP_UTF8)
{
	if (!lpwszUnicodeString)
		return nullptr;

	return UnicodeToAnsiBin(lpwszUnicodeString, StrLength(lpwszUnicodeString) + 1, CodePage);
}

static wchar_t **ArrayAnsiToUnicode(char **lpaszAnsiString, int iCount)
{
	wchar_t **lpaResult = nullptr;

	if (lpaszAnsiString) {
		lpaResult = (wchar_t **)malloc((iCount + 1) * sizeof(wchar_t *));

		if (lpaResult) {
			for (int i = 0; i < iCount; i++) {
				lpaResult[i] = (lpaszAnsiString[i]) ? AnsiToUnicode(lpaszAnsiString[i]) : nullptr;
			}

			lpaResult[iCount] = (wchar_t *)(LONG_PTR)1;		// Array end mark
		}
	}

	return lpaResult;
}

static void FreeArrayUnicode(wchar_t **lpawszUnicodeString)
{
	if (lpawszUnicodeString) {
		for (int i = 0; (LONG_PTR)lpawszUnicodeString[i] != 1; i++)		// Until end mark
		{
			if (lpawszUnicodeString[i])
				free(lpawszUnicodeString[i]);
		}

		free(lpawszUnicodeString);
	}
}

static DWORD OldKeyToKey(DWORD dOldKey)
{
	if (dOldKey & 0x100) {
		dOldKey = (dOldKey ^ 0x100) + EXTENDED_KEY_BASE;
	} else if (dOldKey & 0x200) {
		dOldKey = (dOldKey ^ 0x200) + INTERNAL_KEY_BASE;
	} else {
		DWORD CleanKey = dOldKey & ~KEY_CTRLMASK;

		if (CleanKey > 0x80 && CleanKey < 0x100) {
			ErrnoSaver ErSr;
			char OemChar = static_cast<char>(CleanKey);
			wchar_t WideChar = 0;
			WINPORT(MultiByteToWideChar)(CP_UTF8, 0, &OemChar, 1, &WideChar, 1);
			dOldKey = (dOldKey ^ CleanKey) | WideChar;
		}
	}

	return dOldKey;
}

DWORD KeyToOldKey(DWORD dKey)
{
	if (IS_KEY_EXTENDED(dKey)) {
		dKey = (dKey - EXTENDED_KEY_BASE) | 0x100;
	} else if (IS_KEY_INTERNAL(dKey)) {
		dKey = (dKey - INTERNAL_KEY_BASE) | 0x200;
	} else {
		DWORD CleanKey = dKey & ~KEY_CTRLMASK;

		if (CleanKey > 0x80 && CleanKey < 0x10000) {
			ErrnoSaver ErSr;
			wchar_t WideChar = static_cast<wchar_t>(CleanKey);
			char OemChar = 0;
			WINPORT(WideCharToMultiByte)(CP_UTF8, 0, &WideChar, 1, &OemChar, 1, 0, nullptr);
			dKey = (dKey ^ CleanKey) | OemChar;
		}
	}

	return dKey;
}

void ConvertInfoPanelLinesA(const oldfar::InfoPanelLine *iplA, InfoPanelLine **piplW, int iCount)
{
	if (iplA && piplW && (iCount > 0)) {
		InfoPanelLine *iplW = (InfoPanelLine *)malloc(iCount * sizeof(InfoPanelLine));

		if (iplW) {
			for (int i = 0; i < iCount; i++) {
				iplW[i].Text = AnsiToUnicodeBin(iplA[i].Text, 80);	// BUGBUG
				iplW[i].Data = AnsiToUnicodeBin(iplA[i].Data, 80);	// BUGBUG
				iplW[i].Separator = iplA[i].Separator;
			}
		}

		*piplW = iplW;
	}
}

void FreeUnicodeInfoPanelLines(InfoPanelLine *iplW, int InfoLinesNumber)
{
	for (int i = 0; i < InfoLinesNumber; i++) {
		if (iplW[i].Text)
			free((void *)iplW[i].Text);

		if (iplW[i].Data)
			free((void *)iplW[i].Data);
	}

	if (iplW)
		free((void *)iplW);
}

void ConvertPanelModesA(const oldfar::PanelMode *pnmA, PanelMode **ppnmW, int iCount)
{
	if (pnmA && ppnmW && (iCount > 0)) {
		PanelMode *pnmW = new (std::nothrow) PanelMode[iCount]();
		if (pnmW) {
			for (int i = 0; i < iCount; i++) {
				int iColumnCount = 0;

				if (pnmA[i].ColumnTypes) {
					char *lpTypes = strdup(pnmA[i].ColumnTypes);
					const char *lpToken = strtok(lpTypes, ",");

					while (lpToken && *lpToken) {
						iColumnCount++;
						lpToken = strtok(nullptr, ",");
					}

					free(lpTypes);
				}

				pnmW[i].ColumnTypes = (pnmA[i].ColumnTypes) ? AnsiToUnicode(pnmA[i].ColumnTypes) : nullptr;
				pnmW[i].ColumnWidths = (pnmA[i].ColumnWidths) ? AnsiToUnicode(pnmA[i].ColumnWidths) : nullptr;
				pnmW[i].ColumnTitles = (pnmA[i].ColumnTitles && (iColumnCount > 0))
						? ArrayAnsiToUnicode(pnmA[i].ColumnTitles, iColumnCount)
						: nullptr;
				pnmW[i].FullScreen = pnmA[i].FullScreen;
				pnmW[i].DetailedStatus = pnmA[i].DetailedStatus;
				pnmW[i].AlignExtensions = pnmA[i].AlignExtensions;
				pnmW[i].CaseConversion = pnmA[i].CaseConversion;
				pnmW[i].StatusColumnTypes =
						(pnmA[i].StatusColumnTypes) ? AnsiToUnicode(pnmA[i].StatusColumnTypes) : nullptr;
				pnmW[i].StatusColumnWidths =
						(pnmA[i].StatusColumnWidths) ? AnsiToUnicode(pnmA[i].StatusColumnWidths) : nullptr;
			}
		}

		*ppnmW = pnmW;
	}
}

void FreeUnicodePanelModes(PanelMode *pnmW, int iCount)
{
	if (pnmW) {
		for (int i = 0; i < iCount; i++) {
			if (pnmW[i].ColumnTypes)
				free((void *)pnmW[i].ColumnTypes);

			if (pnmW[i].ColumnWidths)
				free((void *)pnmW[i].ColumnWidths);

			if (pnmW[i].ColumnTitles)
				FreeArrayUnicode((wchar_t **)pnmW[i].ColumnTitles);

			if (pnmW[i].StatusColumnTypes)
				free((void *)pnmW[i].StatusColumnTypes);

			if (pnmW[i].StatusColumnWidths)
				free((void *)pnmW[i].StatusColumnWidths);
		}

		delete[] pnmW;
	}
}

void ConvertKeyBarTitlesA(const oldfar::KeyBarTitles *kbtA, KeyBarTitles *kbtW, bool FullStruct = true)
{
	if (kbtA && kbtW) {
		for (int i = 0; i < 12; i++) {
			kbtW->Titles[i] = kbtA->Titles[i] ? AnsiToUnicode(kbtA->Titles[i]) : nullptr;
			kbtW->CtrlTitles[i] = kbtA->CtrlTitles[i] ? AnsiToUnicode(kbtA->CtrlTitles[i]) : nullptr;
			kbtW->AltTitles[i] = kbtA->AltTitles[i] ? AnsiToUnicode(kbtA->AltTitles[i]) : nullptr;
			kbtW->ShiftTitles[i] = kbtA->ShiftTitles[i] ? AnsiToUnicode(kbtA->ShiftTitles[i]) : nullptr;
			kbtW->CtrlShiftTitles[i] = FullStruct && kbtA->CtrlShiftTitles[i]
					? AnsiToUnicode(kbtA->CtrlShiftTitles[i])
					: nullptr;
			kbtW->AltShiftTitles[i] =
					FullStruct && kbtA->AltShiftTitles[i] ? AnsiToUnicode(kbtA->AltShiftTitles[i]) : nullptr;
			kbtW->CtrlAltTitles[i] =
					FullStruct && kbtA->CtrlAltTitles[i] ? AnsiToUnicode(kbtA->CtrlAltTitles[i]) : nullptr;
		}
	}
}

void FreeUnicodeKeyBarTitles(KeyBarTitles *kbtW)
{
	if (kbtW) {
		for (int i = 0; i < 12; i++) {
			if (kbtW->Titles[i])
				free(kbtW->Titles[i]);

			if (kbtW->CtrlTitles[i])
				free(kbtW->CtrlTitles[i]);

			if (kbtW->AltTitles[i])
				free(kbtW->AltTitles[i]);

			if (kbtW->ShiftTitles[i])
				free(kbtW->ShiftTitles[i]);

			if (kbtW->CtrlShiftTitles[i])
				free(kbtW->CtrlShiftTitles[i]);

			if (kbtW->AltShiftTitles[i])
				free(kbtW->AltShiftTitles[i]);

			if (kbtW->CtrlAltTitles[i])
				free(kbtW->CtrlAltTitles[i]);
		}
	}
}

void ConvertPanelItemA(const oldfar::PluginPanelItem *PanelItemA, PluginPanelItem **PanelItemW,
		int ItemsNumber)
{
	*PanelItemW = (PluginPanelItem *)malloc(ItemsNumber * sizeof(PluginPanelItem));
	memset(*PanelItemW, 0, ItemsNumber * sizeof(PluginPanelItem));

	for (int i = 0; i < ItemsNumber; i++) {
		(*PanelItemW)[i].FindData.ftCreationTime = PanelItemA[i].FindData.ftCreationTime;
		(*PanelItemW)[i].FindData.ftLastAccessTime = PanelItemA[i].FindData.ftLastAccessTime;
		(*PanelItemW)[i].FindData.ftLastWriteTime = PanelItemA[i].FindData.ftLastWriteTime;
		(*PanelItemW)[i].FindData.nPhysicalSize = PanelItemA[i].FindData.nPhysicalSize;
		(*PanelItemW)[i].FindData.nFileSize = PanelItemA[i].FindData.nFileSize;
		(*PanelItemW)[i].FindData.dwFileAttributes = PanelItemA[i].FindData.dwFileAttributes;
		(*PanelItemW)[i].FindData.dwUnixMode = PanelItemA[i].FindData.dwUnixMode;
		(*PanelItemW)[i].UserData = PanelItemA[i].UserData;
		(*PanelItemW)[i].Flags = PanelItemA[i].Flags;
		(*PanelItemW)[i].NumberOfLinks = PanelItemA[i].NumberOfLinks;
		(*PanelItemW)[i].CRC32 = PanelItemA[i].CRC32;
		(*PanelItemW)[i].FindData.lpwszFileName =
				AnsiToUnicode(PanelItemA[i].FindData.cFileName, ARRAYSIZE(PanelItemA[i].FindData.cFileName));

		if (PanelItemA[i].Description)
			(*PanelItemW)[i].Description = AnsiToUnicode(PanelItemA[i].Description);

		if (PanelItemA[i].Owner)
			(*PanelItemW)[i].Owner = AnsiToUnicode(PanelItemA[i].Owner);

		if (PanelItemA[i].Group)
			(*PanelItemW)[i].Group = AnsiToUnicode(PanelItemA[i].Group);

		if (PanelItemA[i].CustomColumnNumber) {
			(*PanelItemW)[i].CustomColumnNumber = PanelItemA[i].CustomColumnNumber;
			(*PanelItemW)[i].CustomColumnData =
					ArrayAnsiToUnicode(PanelItemA[i].CustomColumnData, PanelItemA[i].CustomColumnNumber);
		}
	}
}

void ConvertPanelItemToAnsi(const PluginPanelItem &PanelItem, oldfar::PluginPanelItem &PanelItemA)
{
	PanelItemA.Flags = PanelItem.Flags;
	PanelItemA.NumberOfLinks = PanelItem.NumberOfLinks;

	if (PanelItem.Description)
		PanelItemA.Description = UnicodeToAnsi(PanelItem.Description);

	if (PanelItem.Owner)
		PanelItemA.Owner = UnicodeToAnsi(PanelItem.Owner);

	if (PanelItem.Group)
		PanelItemA.Group = UnicodeToAnsi(PanelItem.Group);

	if (PanelItem.CustomColumnNumber) {
		PanelItemA.CustomColumnNumber = PanelItem.CustomColumnNumber;
		PanelItemA.CustomColumnData = (char **)malloc(PanelItem.CustomColumnNumber * sizeof(char *));

		for (int j = 0; j < PanelItem.CustomColumnNumber; j++)
			PanelItemA.CustomColumnData[j] = UnicodeToAnsi(PanelItem.CustomColumnData[j]);
	}

	if (PanelItem.UserData && PanelItem.Flags & PPIF_USERDATA) {
		DWORD Size = *(DWORD *)PanelItem.UserData;
		PanelItemA.UserData = (DWORD_PTR)malloc(Size);
		memcpy((void *)PanelItemA.UserData, (void *)PanelItem.UserData, Size);
	} else
		PanelItemA.UserData = PanelItem.UserData;

	PanelItemA.CRC32 = PanelItem.CRC32;
	PanelItemA.FindData.dwFileAttributes = PanelItem.FindData.dwFileAttributes;
	PanelItemA.FindData.ftCreationTime = PanelItem.FindData.ftCreationTime;
	PanelItemA.FindData.ftLastAccessTime = PanelItem.FindData.ftLastAccessTime;
	PanelItemA.FindData.ftLastWriteTime = PanelItem.FindData.ftLastWriteTime;
	PanelItemA.FindData.nFileSize = PanelItem.FindData.nFileSize;
	PanelItemA.FindData.nPhysicalSize = PanelItem.FindData.nPhysicalSize;
	PWZ_to_PZ(PanelItem.FindData.lpwszFileName, PanelItemA.FindData.cFileName,
			ARRAYSIZE(PanelItemA.FindData.cFileName));
}

void ConvertPanelItemsArrayToAnsi(const PluginPanelItem *PanelItemW, oldfar::PluginPanelItem *&PanelItemA,
		int ItemsNumber)
{
	PanelItemA = (oldfar::PluginPanelItem *)malloc(ItemsNumber * sizeof(oldfar::PluginPanelItem));
	memset(PanelItemA, 0, ItemsNumber * sizeof(oldfar::PluginPanelItem));

	for (int i = 0; i < ItemsNumber; i++) {
		ConvertPanelItemToAnsi(PanelItemW[i], PanelItemA[i]);
	}
}

void FreeUnicodePanelItem(PluginPanelItem *PanelItem, int ItemsNumber)
{
	for (int i = 0; i < ItemsNumber; i++) {
		if (PanelItem[i].Description)
			free((void *)PanelItem[i].Description);

		if (PanelItem[i].Owner)
			free((void *)PanelItem[i].Owner);

		if (PanelItem[i].Group)
			free((void *)PanelItem[i].Group);

		if (PanelItem[i].CustomColumnNumber) {
			for (int j = 0; j < PanelItem[i].CustomColumnNumber; j++)
				free((void *)PanelItem[i].CustomColumnData[j]);

			free((void *)PanelItem[i].CustomColumnData);
		}

		apiFreeFindData(&PanelItem[i].FindData);
	}

	free(PanelItem);
}

void FreePanelItemA(oldfar::PluginPanelItem *PanelItem, int ItemsNumber, bool bFreeArray = true)
{
	for (int i = 0; i < ItemsNumber; i++) {
		if (PanelItem[i].Description)
			free(PanelItem[i].Description);

		if (PanelItem[i].Owner)
			free(PanelItem[i].Owner);

		if (PanelItem[i].Group)
			free(PanelItem[i].Group);

		if (PanelItem[i].CustomColumnNumber) {
			for (int j = 0; j < PanelItem[i].CustomColumnNumber; j++)
				free(PanelItem[i].CustomColumnData[j]);

			free(PanelItem[i].CustomColumnData);
		}

		if (PanelItem[i].UserData && PanelItem[i].Flags & oldfar::PPIF_USERDATA) {
			free((PVOID)PanelItem[i].UserData);
		}
	}

	if (bFreeArray)
		free(PanelItem);
}

char *WINAPI RemoveTrailingSpacesA(char *Str)
{
	if (!Str)
		return nullptr;

	if (*Str == '\0')
		return Str;

	char *ChPtr;
	size_t I;

	for (ChPtr = Str + (I = strlen(Str)) - 1; I > 0; I--, ChPtr--) {
		if (IsSpace(*ChPtr) || IsEol(*ChPtr))
			*ChPtr = 0;
		else
			break;
	}

	return Str;
}

char *WINAPI FarItoaA(int value, char *string, int radix)
{
	if (string)
		return _itoa(value, string, radix);

	return nullptr;
}

char *WINAPI FarItoa64A(int64_t value, char *string, int radix)
{
	if (string)
		return _i64toa(value, string, radix);

	return nullptr;
}

int WINAPI FarAtoiA(const char *s)
{
	if (s)
		return atoi(s);

	return 0;
}

int64_t WINAPI FarAtoi64A(const char *s)
{
	return s ? atoll(s) : 0;
}

char *WINAPI PointToNameA(char *Path)
{
	if (!Path)
		return nullptr;

	char *NamePtr = Path;

	while (*Path) {
		if (IsSlashA(*Path))
			NamePtr = Path + 1;

		Path++;
	}

	return (NamePtr);
}

void WINAPI UnquoteA(char *Str)
{
	if (!Str)
		return;

	char *Dst = Str;

	while (*Str) {
		if (*Str != '\"')
			*Dst++ = *Str;

		Str++;
	}

	*Dst = 0;
}

char *WINAPI RemoveLeadingSpacesA(char *Str)
{
	char *ChPtr;

	if (!(ChPtr = Str))
		return nullptr;

	for (; IsSpace(*ChPtr); ChPtr++)
		;

	if (ChPtr != Str)
		memmove(Str, ChPtr, strlen(ChPtr) + 1);

	return Str;
}

char *WINAPI RemoveExternalSpacesA(char *Str)
{
	return RemoveTrailingSpacesA(RemoveLeadingSpacesA(Str));
}

char *WINAPI TruncStrA(char *Str, int MaxLength)
{
	if (Str) {
		int Length;

		if (MaxLength < 0)
			MaxLength = 0;

		if ((Length = (int)strlen(Str)) > MaxLength) {
			if (MaxLength > 3) {
				char *MovePos = Str + Length - MaxLength + 3;
				memmove(Str + 3, MovePos, strlen(MovePos) + 1);
				memcpy(Str, "...", 3);
			}

			Str[MaxLength] = 0;
		}
	}

	return (Str);
}

char *WINAPI TruncPathStrA(char *Str, int MaxLength)
{
	if (Str) {
		int nLength = (int)strlen(Str);

		if (nLength > MaxLength) {
			char *lpStart = nullptr;

			/*			if (*Str && (Str[1] == ':') && IsSlash(Str[2]))
							lpStart = Str+3;
						else*/
			{
				if ((Str[0] == GOOD_SLASH) && (Str[1] == GOOD_SLASH)) {
					if ((lpStart = const_cast<char *>(FirstSlashA(Str + 2))))
						if ((lpStart = const_cast<char *>(FirstSlashA(lpStart + 1))))
							lpStart++;
				}
			}

			if (!lpStart || (lpStart - Str > MaxLength - 5))
				return TruncStrA(Str, MaxLength);

			char *lpInPos = lpStart + 3 + (nLength - MaxLength);
			memmove(lpStart + 3, lpInPos, strlen(lpInPos) + 1);
			memcpy(lpStart, "...", 3);
		}
	}

	return Str;
}

char *InsertQuoteA(char *Str)
{
	size_t l = strlen(Str);

	if (*Str != '"') {
		memmove(Str + 1, Str, ++l);
		*Str = '"';
	}

	if (Str[l - 1] != '"') {
		Str[l++] = '\"';
		Str[l] = 0;
	}

	return Str;
}

char *WINAPI QuoteSpaceOnlyA(char *Str)
{
	if (Str && strchr(Str, ' '))
		InsertQuoteA(Str);

	return (Str);
}

BOOL AddEndSlashA(char *Path, char TypeSlash)
{
	BOOL Result = FALSE;

	if (Path) {
		/*
			$ 06.12.2000 IS
			! Теперь функция работает с обоими видами слешей, также происходит
			изменение уже существующего конечного слеша на такой, который
			встречается чаще.
		*/
		char *end = Path + strlen(Path);
		int Length = (int)(end - Path);
		char c = (TypeSlash == '\\') ? '\\' : '/';
		Result = TRUE;

		if (!Length) {
			*end = c;
			end[1] = 0;
		} else {
			end--;

			if (!IsSlashA(*end)) {
				end[1] = c;
				end[2] = 0;
			} else
				*end = c;
		}

		/* IS $ */
	}

	return Result;
}

BOOL WINAPI AddEndSlashA(char *Path)
{
	return AddEndSlashA(Path, 0);
}

void WINAPI GetPathRootA(const char *Path, char *Root)
{
	fprintf(stderr, "DEPRECATED: %s('%s')\n", __FUNCTION__, Path);
	Root[0] = GOOD_SLASH;
	Root[1] = 0;
}

int WINAPI CopyToClipboardA(const char *Data)
{
	wchar_t *p = Data ? AnsiToUnicode(Data) : nullptr;
	int ret = CopyToClipboard(p);

	if (p)
		free(p);

	return ret;
}

char *WINAPI PasteFromClipboardA()
{
	wchar_t *p = PasteFromClipboard();

	if (p) {
		char *pa = UnicodeToAnsi(p);
		free(p);
		return pa;
	}

	return nullptr;
}

void WINAPI DeleteBufferA(void *Buffer)
{
	if (Buffer)
		free(Buffer);
}

int WINAPI ProcessNameA(const char *Param1, char *Param2, DWORD Flags)
{
	FARString strP1(Param1), strP2(Param2);
	int size = (int)(strP1.GetLength() + strP2.GetLength() + oldfar::NM) + 1;	// а хрен ещё как угадать скока там этот Param2 для PN_GENERATENAME
	wchar_t *p = (wchar_t *)malloc(size * sizeof(wchar_t));
	wcscpy(p, strP2);
	int newFlags = 0;

	if (Flags & oldfar::PN_SKIPPATH) {
		newFlags|= PN_SKIPPATH;
		Flags&= ~oldfar::PN_SKIPPATH;
	}

	if (Flags == oldfar::PN_CMPNAME) {
		newFlags|= PN_CMPNAME;
	} else if (Flags == oldfar::PN_CMPNAMELIST) {
		newFlags|= PN_CMPNAMELIST;
	} else if (Flags & oldfar::PN_GENERATENAME) {
		newFlags|= PN_GENERATENAME | (Flags & 0xFF);
	}

	int ret = ProcessName(strP1, p, size, newFlags);

	if (newFlags & PN_GENERATENAME)
		PWZ_to_PZ(p, Param2, size);

	free(p);
	return ret;
}

unsigned int WINAPI KeyNameToKeyA(const char *Name)
{
	FARString strN(Name);
	return KeyToOldKey(KeyNameToKey(strN));
}

BOOL WINAPI FarKeyToNameA(FarKey Key, char *KeyText, int Size)
{
	FARString strKT;
	int ret = KeyToText(OldKeyToKey(Key), strKT);

	if (ret)
		strKT.GetCharString(KeyText, Size > 0 ? Size + 1 : 32);

	return ret;
}

unsigned int WINAPI InputRecordToKeyA(const INPUT_RECORD *r)
{
	return KeyToOldKey(InputRecordToKey(r));
}

char *WINAPI FarMkTempA(char *Dest, const char *Prefix)
{
	FARString strP(Prefix);
	wchar_t D[oldfar::NM] = {0};
	FarMkTemp(D, ARRAYSIZE(D), strP);
	PWZ_to_PZ(D, Dest, sizeof(D));
	return Dest;
}

int WINAPI FarMkLinkA(const char *Src, const char *Dest, DWORD Flags)
{
	FARString s(Src), d(Dest);
	int flg = 0;

	switch (Flags & 0xf) {
		case oldfar::FLINK_HARDLINK:
			flg = FLINK_HARDLINK;
			break;
		case oldfar::FLINK_JUNCTION:
			flg = FLINK_JUNCTION;
			break;
		case oldfar::FLINK_VOLMOUNT:
			flg = FLINK_VOLMOUNT;
			break;
		case oldfar::FLINK_SYMLINKFILE:
			flg = FLINK_SYMLINKFILE;
			break;
		case oldfar::FLINK_SYMLINKDIR:
			flg = FLINK_SYMLINKDIR;
			break;
	}

	if (Flags & oldfar::FLINK_SHOWERRMSG)
		flg|= FLINK_SHOWERRMSG;

	if (Flags & oldfar::FLINK_DONOTUPDATEPANEL)
		flg|= FLINK_DONOTUPDATEPANEL;

	return FarMkLink(s, d, flg);
}

int WINAPI GetNumberOfLinksA(const char *Name)
{
	fprintf(stderr, "TODO: GetNumberOfLinksA(%s)\n", Name);
	return 1;
	// FARString n(Name);
	// return GetNumberOfLinks(n);
}

int WINAPI ConvertNameToRealA(const char *Src, char *Dest, int DestSize)
{
	FARString strSrc(Src), strDest;
	ConvertNameToReal(strSrc, strDest);

	if (!Dest)
		return (int)strDest.GetLength();
	else
		strDest.GetCharString(Dest, DestSize);

	return Min((int)strDest.GetLength(), DestSize);
}

int WINAPI FarGetReparsePointInfoA(const char *Src, char *Dest, int DestSize)
{

	/*if (Src && *Src)
	{
		FARString strSrc(Src);
		FARString strDest;
		DWORD Size=GetReparsePointInfo(strSrc,strDest,nullptr);

		if (DestSize && Dest)
			strDest.GetCharString(Dest,DestSize);

		return Size;
	}*/

	return 0;
}

struct FAR_SEARCH_A_CALLBACK_PARAM
{
	oldfar::FRSUSERFUNC Func;
	void *Param;
};

static int WINAPI
FarRecursiveSearchA_Callback(const FAR_FIND_DATA *FData, const wchar_t *FullName, void *param)
{
	FAR_SEARCH_A_CALLBACK_PARAM *pCallbackParam = static_cast<FAR_SEARCH_A_CALLBACK_PARAM *>(param);
	WIN32_FIND_DATAA FindData{};
	FindData.dwFileAttributes = FData->dwFileAttributes;
	FindData.ftCreationTime = FData->ftCreationTime;
	FindData.ftLastAccessTime = FData->ftLastAccessTime;
	FindData.ftLastWriteTime = FData->ftLastWriteTime;
	FindData.nFileSize = (DWORD)FData->nFileSize;
	PWZ_to_PZ(FData->lpwszFileName, FindData.cFileName, ARRAYSIZE(FindData.cFileName));
	char FullNameA[oldfar::NM];
	PWZ_to_PZ(FullName, FullNameA, sizeof(FullNameA));
	return pCallbackParam->Func(&FindData, FullNameA, pCallbackParam->Param);
}

void WINAPI
FarRecursiveSearchA(const char *InitDir, const char *Mask, oldfar::FRSUSERFUNC Func, DWORD Flags, void *Param)
{
	FARString strInitDir(InitDir);
	FARString strMask(Mask);
	FAR_SEARCH_A_CALLBACK_PARAM CallbackParam;
	CallbackParam.Func = Func;
	CallbackParam.Param = Param;
	int newFlags = 0;

	if (Flags & oldfar::FRS_RETUPDIR)
		newFlags|= FRS_RETUPDIR;

	if (Flags & oldfar::FRS_RECUR)
		newFlags|= FRS_RECUR;

	if (Flags & oldfar::FRS_SCANSYMLINK)
		newFlags|= FRS_SCANSYMLINK;

	FarRecursiveSearch(static_cast<const wchar_t *>(strInitDir), static_cast<const wchar_t *>(strMask),
			FarRecursiveSearchA_Callback, newFlags, static_cast<void *>(&CallbackParam));
}

DWORD WINAPI ExpandEnvironmentStrA(const char *src, char *dest, size_t size)
{
	FARString strS(src), strD;
	apiExpandEnvironmentStrings(strS, strD);
	DWORD len = strD.GetCharString(dest, size);
	return std::min((DWORD)size, len ? len - 1 : 0);
}

int WINAPI FarViewerA(const char *FileName, const char *Title, int X1, int Y1, int X2, int Y2, DWORD Flags)
{
	FARString strFN(FileName), strT(Title);
	return FarViewer(strFN, strT, X1, Y1, X2, Y2, Flags, CP_AUTODETECT);
}

int WINAPI FarEditorA(const char *FileName, const char *Title, int X1, int Y1, int X2, int Y2, DWORD Flags,
		int StartLine, int StartChar)
{
	FARString strFN(FileName), strT(Title);
	return FarEditor(strFN, strT, X1, Y1, X2, Y2, Flags, StartLine, StartChar, CP_AUTODETECT);
}

int WINAPI FarCmpNameA(const char *pattern, const char *str, int skippath)
{
	FARString strP(pattern), strS(str);
	return FarCmpName(strP, strS, skippath);
}

void WINAPI FarTextA(int X, int Y, int Color, const char *Str)
{
	if (!Str)
		return FarText(X, Y, Color, nullptr);

	FARString strS(Str);
	return FarText(X, Y, Color, strS);
}

BOOL WINAPI FarShowHelpA(const char *ModuleName, const char *HelpTopic, DWORD Flags)
{
	FARString strMN(ModuleName), strHT(HelpTopic);
	return FarShowHelp(strMN, (HelpTopic ? strHT.CPtr() : nullptr), Flags);
}

int WINAPI FarInputBoxA(const char *Title, const char *Prompt, const char *HistoryName, const char *SrcText,
		char *DestText, int DestLength, const char *HelpTopic, DWORD Flags)
{
	FARString strT(Title), strP(Prompt), strHN(HistoryName), strST(SrcText), strD, strHT(HelpTopic);
	wchar_t *D = strD.GetBuffer(DestLength);
	int ret = FarInputBox((Title ? strT.CPtr() : nullptr), (Prompt ? strP.CPtr() : nullptr),
			(HistoryName ? strHN.CPtr() : nullptr), (SrcText ? strST.CPtr() : nullptr), D, DestLength,
			(HelpTopic ? strHT.CPtr() : nullptr), Flags);
	strD.ReleaseBuffer();

	if (ret && DestText)
		strD.GetCharString(DestText, DestLength + 1);

	return ret;
}

int WINAPI FarMessageFnA(INT_PTR PluginNumber, DWORD Flags, const char *HelpTopic, const char *const *Items,
		int ItemsNumber, int ButtonsNumber)
{
	FARString strHT(HelpTopic);
	wchar_t **p;
	int c = 0;

	Flags&= ~oldfar::FMSG_DOWN;

	if (Flags & oldfar::FMSG_ALLINONE) {
		fprintf(stderr, "FMSG_ALLINONE\n");
		p = (wchar_t **)AnsiToUnicode((const char *)Items);
	} else {
		c = ItemsNumber;
		p = (wchar_t **)malloc(c * sizeof(wchar_t *));

		for (int i = 0; i < c; i++)
			p[i] = AnsiToUnicode(Items[i]);
	}

	int ret = FarMessageFn(PluginNumber, Flags, (HelpTopic ? strHT.CPtr() : nullptr), p, ItemsNumber,
			ButtonsNumber);

	for (int i = 0; i < c; i++)
		free(p[i]);

	free(p);
	return ret;
}

static CriticalSection s_get_msga_cs;
const char *WINAPI FarGetMsgFnA(INT_PTR PluginHandle, FarLangMsgID MsgId)
{
	// BUGBUG, надо проверять, что PluginHandle - плагин
	PluginA *pPlugin = (PluginA *)PluginHandle;

	std::wstring strPath = pPlugin->GetModuleName().CPtr();
	CutToSlash(strPath);

	CriticalSectionLock lock(s_get_msga_cs);
	//	fprintf(stderr,"FarGetMsgFnA: strPath=%ls\n", strPath.CPtr());

	if (!pPlugin->InitLang(strPath.c_str())) {
		return "";
	}

	return pPlugin->GetMsgA(MsgId);
}

int WINAPI FarMenuFnA(INT_PTR PluginNumber, int X, int Y, int MaxHeight, DWORD Flags, const char *Title,
		const char *Bottom, const char *HelpTopic, const int *BreakKeys, int *BreakCode,
		const oldfar::FarMenuItem *Item, int ItemsNumber)
{
	FARString strT(Title), strB(Bottom), strHT(HelpTopic);
	const wchar_t *wszT = Title ? strT.CPtr() : nullptr;
	const wchar_t *wszB = Bottom ? strB.CPtr() : nullptr;
	const wchar_t *wszHT = HelpTopic ? strHT.CPtr() : nullptr;

	if (!Item || !ItemsNumber)
		return FarMenuFn(PluginNumber, X, Y, MaxHeight, Flags, wszT, wszB, wszHT, BreakKeys, BreakCode,
				nullptr, 0);

	FarMenuItemEx *mi = (FarMenuItemEx *)malloc(ItemsNumber * sizeof(*mi));

	if (Flags & FMENU_USEEXT) {
		oldfar::FarMenuItemEx *p = (oldfar::FarMenuItemEx *)Item;

		for (int i = 0; i < ItemsNumber; i++) {
			mi[i].Flags = p[i].Flags & ~oldfar::MIF_USETEXTPTR;
			mi[i].Text =
					AnsiToUnicode(p[i].Flags & oldfar::MIF_USETEXTPTR ? p[i].Text.TextPtr : p[i].Text.Text);
			mi[i].AccelKey = OldKeyToKey(p[i].AccelKey);
			mi[i].Reserved = p[i].Reserved;
			mi[i].UserData = p[i].UserData;
		}
	} else {
		for (int i = 0; i < ItemsNumber; i++) {
			mi[i].Flags = 0;

			if (Item[i].Selected)
				mi[i].Flags|= MIF_SELECTED;

			if (Item[i].Checked) {
				mi[i].Flags|= MIF_CHECKED;

				if (Item[i].Checked > 1)
					AnsiToUnicodeBin((const char *)&Item[i].Checked, (wchar_t *)&mi[i].Flags, 1);
			}

			if (Item[i].Separator) {
				mi[i].Flags|= MIF_SEPARATOR;
				mi[i].Text = 0;
			} else
				mi[i].Text = AnsiToUnicode(Item[i].Text);

			mi[i].AccelKey = 0;
			mi[i].Reserved = 0;
			mi[i].UserData = 0;
		}
	}

	int ret = FarMenuFn(PluginNumber, X, Y, MaxHeight, Flags | FMENU_USEEXT, wszT, wszB, wszHT, BreakKeys,
			BreakCode, (FarMenuItem *)mi, ItemsNumber);

	for (int i = 0; i < ItemsNumber; i++)
		if (mi[i].Text)
			free((wchar_t *)mi[i].Text);

	if (mi)
		free(mi);

	return ret;
}

typedef struct DialogData
{
	FARWINDOWPROC DlgProc;
	HANDLE hDlg;
	oldfar::FarDialogItem *diA;
	FarDialogItem *di;
	FarList *l;
} *PDialogData;

DList<PDialogData> DialogList;

oldfar::FarDialogItem *OneDialogItem = nullptr;

PDialogData FindCurrentDialogData(HANDLE hDlg)
{
	PDialogData Result = nullptr;
	for (PDialogData *i = DialogList.Last(); i; i = DialogList.Prev(i)) {
		if ((*i)->hDlg == hDlg) {
			Result = *i;
			break;
		}
	}
	return Result;
}

oldfar::FarDialogItem *CurrentDialogItemA(HANDLE hDlg, int ItemNumber)
{
	PDialogData Data = FindCurrentDialogData(hDlg);
	return Data ? &Data->diA[ItemNumber] : nullptr;
}

FarDialogItem *CurrentDialogItem(HANDLE hDlg, int ItemNumber)
{
	PDialogData Data = FindCurrentDialogData(hDlg);
	return Data ? &Data->di[ItemNumber] : nullptr;
}

FarList *CurrentList(HANDLE hDlg, int ItemNumber)
{
	PDialogData Data = FindCurrentDialogData(hDlg);
	return Data ? &Data->l[ItemNumber] : nullptr;
}

LONG_PTR WINAPI CurrentDlgProc(HANDLE hDlg, int Msg, int Param1, LONG_PTR Param2)
{
	LONG_PTR Ret = 0;
	PDialogData Data = FindCurrentDialogData(hDlg);

	if (Data && Data->DlgProc)
		Ret = Data->DlgProc(Data->hDlg, Msg, Param1, Param2);

	return Ret;
}

void UnicodeListItemToAnsi(FarListItem *li, oldfar::FarListItem *liA)
{
	PWZ_to_PZ(li->Text, liA->Text, sizeof(liA->Text) - 1);
	liA->Flags = 0;

	if (li->Flags & LIF_SELECTED)
		liA->Flags|= oldfar::LIF_SELECTED;

	if (li->Flags & LIF_CHECKED)
		liA->Flags|= oldfar::LIF_CHECKED;

	if (li->Flags & LIF_SEPARATOR)
		liA->Flags|= oldfar::LIF_SEPARATOR;

	if (li->Flags & LIF_DISABLE)
		liA->Flags|= oldfar::LIF_DISABLE;

	if (li->Flags & LIF_GRAYED)
		liA->Flags|= oldfar::LIF_GRAYED;

	if (li->Flags & LIF_HIDDEN)
		liA->Flags|= oldfar::LIF_HIDDEN;

	if (li->Flags & LIF_DELETEUSERDATA)
		liA->Flags|= oldfar::LIF_DELETEUSERDATA;
}

size_t GetAnsiVBufSize(oldfar::FarDialogItem &diA)
{
	return size_t(diA.X2 - diA.X1 + 1) * (diA.Y2 - diA.Y1 + 1);
}

PCHAR_INFO GetAnsiVBufPtr(PCHAR_INFO VBuf, size_t Size)
{
	PCHAR_INFO VBufA = nullptr;
	if (VBuf) {
		VBufA = *reinterpret_cast<PCHAR_INFO *>(&VBuf[Size]);
	}
	return VBufA;
}

void SetAnsiVBufPtr(PCHAR_INFO VBuf, PCHAR_INFO VBufA, size_t Size)
{
	if (VBuf) {
		*reinterpret_cast<PCHAR_INFO *>(&VBuf[Size]) = VBufA;
	}
}

void AnsiVBufToUnicode(PCHAR_INFO VBufA, PCHAR_INFO VBuf, size_t Size, bool NoCvt)
{
	if (VBuf && VBufA) {
		for (size_t i = 0; i < Size; i++) {
			if (NoCvt) {
				VBuf[i].Char.UnicodeChar = VBufA[i].Char.UnicodeChar;
			} else {
				WCHAR wc{};
				AnsiToUnicodeBin(&VBufA[i].Char.AsciiChar, &wc, 1);
				CI_SET_WCHAR(VBuf[i], wc);
			}

			CI_SET_ATTR(VBuf[i], VBufA[i].Attributes);
		}
	}
}

PCHAR_INFO AnsiVBufToUnicode(oldfar::FarDialogItem &diA)
{
	PCHAR_INFO VBuf = nullptr;

	if (diA.Param.VBuf) {
		size_t Size = GetAnsiVBufSize(diA);
		// +sizeof(PCHAR_INFO) потому что там храним поинтер на анси vbuf.
		VBuf = reinterpret_cast<PCHAR_INFO>(malloc(Size * sizeof(CHAR_INFO) + sizeof(PCHAR_INFO)));

		if (VBuf) {
			AnsiVBufToUnicode(diA.Param.VBuf, VBuf, Size,
					(diA.Flags & oldfar::DIF_NOTCVTUSERCONTROL) == oldfar::DIF_NOTCVTUSERCONTROL);
			SetAnsiVBufPtr(VBuf, diA.Param.VBuf, Size);
		}
	}

	return VBuf;
}

void AnsiListItemToUnicode(oldfar::FarListItem *liA, FarListItem *li)
{
	wchar_t *ListItemText = (wchar_t *)malloc(ARRAYSIZE(liA->Text) * sizeof(wchar_t));
	PZ_to_PWZ(liA->Text, ListItemText, sizeof(liA->Text) - 1);
	ListItemText[ARRAYSIZE(liA->Text) - 1] = 0;
	li->Text = ListItemText;
	li->Flags = 0;

	if (liA->Flags & oldfar::LIF_SELECTED)
		li->Flags|= LIF_SELECTED;

	if (liA->Flags & oldfar::LIF_CHECKED)
		li->Flags|= LIF_CHECKED;

	if (liA->Flags & oldfar::LIF_SEPARATOR)
		li->Flags|= LIF_SEPARATOR;

	if (liA->Flags & oldfar::LIF_DISABLE)
		li->Flags|= LIF_DISABLE;

	if (liA->Flags & oldfar::LIF_GRAYED)
		li->Flags|= LIF_GRAYED;

	if (liA->Flags & oldfar::LIF_HIDDEN)
		li->Flags|= LIF_HIDDEN;

	if (liA->Flags & oldfar::LIF_DELETEUSERDATA)
		li->Flags|= LIF_DELETEUSERDATA;
}

void AnsiDialogItemToUnicodeSafe(oldfar::FarDialogItem &diA, FarDialogItem &di)
{
	switch (diA.Type) {
		case oldfar::DI_TEXT:
			di.Type = DI_TEXT;
			break;
		case oldfar::DI_VTEXT:
			di.Type = DI_VTEXT;
			break;
		case oldfar::DI_SINGLEBOX:
			di.Type = DI_SINGLEBOX;
			break;
		case oldfar::DI_DOUBLEBOX:
			di.Type = DI_DOUBLEBOX;
			break;
		case oldfar::DI_EDIT:
			di.Type = DI_EDIT;
			break;
		case oldfar::DI_PSWEDIT:
			di.Type = DI_PSWEDIT;
			break;
		case oldfar::DI_FIXEDIT:
			di.Type = DI_FIXEDIT;
			break;
		case oldfar::DI_BUTTON:
			di.Type = DI_BUTTON;
			di.Param.Selected = diA.Param.Selected;
			break;
		case oldfar::DI_CHECKBOX:
			di.Type = DI_CHECKBOX;
			di.Param.Selected = diA.Param.Selected;
			break;
		case oldfar::DI_RADIOBUTTON:
			di.Type = DI_RADIOBUTTON;
			di.Param.Selected = diA.Param.Selected;
			break;
		case oldfar::DI_COMBOBOX:
			di.Type = DI_COMBOBOX;
			di.Param.ListPos = diA.Param.ListPos;
			break;
		case oldfar::DI_LISTBOX:
			di.Type = DI_LISTBOX;
			di.Param.ListPos = diA.Param.ListPos;
			break;
		case oldfar::DI_MEMOEDIT:
			di.Type = DI_MEMOEDIT;
			break;
		case oldfar::DI_USERCONTROL:
			di.Type = DI_USERCONTROL;
			break;
	}

	di.X1 = diA.X1;
	di.Y1 = diA.Y1;
	di.X2 = diA.X2;
	di.Y2 = diA.Y2;
	di.Focus = diA.Focus;
	di.Flags = 0;

	if (diA.Flags) {
		if (diA.Flags & oldfar::DIF_SETCOLOR)
			di.Flags|= DIF_SETCOLOR | (diA.Flags & oldfar::DIF_COLORMASK);

		if (diA.Flags & oldfar::DIF_BOXCOLOR)
			di.Flags|= DIF_BOXCOLOR;

		if (diA.Flags & oldfar::DIF_GROUP)
			di.Flags|= DIF_GROUP;

		if (diA.Flags & oldfar::DIF_LEFTTEXT)
			di.Flags|= DIF_LEFTTEXT;

		if (diA.Flags & oldfar::DIF_MOVESELECT)
			di.Flags|= DIF_MOVESELECT;

		if (diA.Flags & oldfar::DIF_SHOWAMPERSAND)
			di.Flags|= DIF_SHOWAMPERSAND;

		if (diA.Flags & oldfar::DIF_CENTERGROUP)
			di.Flags|= DIF_CENTERGROUP;

		if (diA.Flags & oldfar::DIF_NOBRACKETS)
			di.Flags|= DIF_NOBRACKETS;

		if (diA.Flags & oldfar::DIF_MANUALADDHISTORY)
			di.Flags|= DIF_MANUALADDHISTORY;

		if (diA.Flags & oldfar::DIF_SEPARATOR)
			di.Flags|= DIF_SEPARATOR;

		if (diA.Flags & oldfar::DIF_SEPARATOR2)
			di.Flags|= DIF_SEPARATOR2;

		if (diA.Flags & oldfar::DIF_EDITOR)
			di.Flags|= DIF_EDITOR;

		if (diA.Flags & oldfar::DIF_LISTNOAMPERSAND)
			di.Flags|= DIF_LISTNOAMPERSAND;

		if (diA.Flags & oldfar::DIF_LISTNOBOX)
			di.Flags|= DIF_LISTNOBOX;

		if (diA.Flags & oldfar::DIF_HISTORY)
			di.Flags|= DIF_HISTORY;

		if (diA.Flags & oldfar::DIF_BTNNOCLOSE)
			di.Flags|= DIF_BTNNOCLOSE;

		if (diA.Flags & oldfar::DIF_CENTERTEXT)
			di.Flags|= DIF_CENTERTEXT;

		if (diA.Flags & oldfar::DIF_SEPARATORUSER)
			di.Flags|= DIF_SEPARATORUSER;

		if (diA.Flags & oldfar::DIF_EDITEXPAND)
			di.Flags|= DIF_EDITEXPAND;

		if (diA.Flags & oldfar::DIF_DROPDOWNLIST)
			di.Flags|= DIF_DROPDOWNLIST;

		if (diA.Flags & oldfar::DIF_USELASTHISTORY)
			di.Flags|= DIF_USELASTHISTORY;

		if (diA.Flags & oldfar::DIF_MASKEDIT)
			di.Flags|= DIF_MASKEDIT;

		if (diA.Flags & oldfar::DIF_SELECTONENTRY)
			di.Flags|= DIF_SELECTONENTRY;

		if (diA.Flags & oldfar::DIF_3STATE)
			di.Flags|= DIF_3STATE;

		if (diA.Flags & oldfar::DIF_EDITPATH)
			di.Flags|= DIF_EDITPATH;

		if (diA.Flags & oldfar::DIF_LISTWRAPMODE)
			di.Flags|= DIF_LISTWRAPMODE;

		if (diA.Flags & oldfar::DIF_LISTAUTOHIGHLIGHT)
			di.Flags|= DIF_LISTAUTOHIGHLIGHT;

		if (diA.Flags & oldfar::DIF_AUTOMATION)
			di.Flags|= DIF_AUTOMATION;

		if (diA.Flags & oldfar::DIF_HIDDEN)
			di.Flags|= DIF_HIDDEN;

		if (diA.Flags & oldfar::DIF_READONLY)
			di.Flags|= DIF_READONLY;

		if (diA.Flags & oldfar::DIF_NOFOCUS)
			di.Flags|= DIF_NOFOCUS;

		if (diA.Flags & oldfar::DIF_DISABLE)
			di.Flags|= DIF_DISABLE;
	}

	di.DefaultButton = diA.DefaultButton;
}

void AnsiDialogItemToUnicode(oldfar::FarDialogItem &diA, FarDialogItem &di, FarList &l)
{
	memset(&di, 0, sizeof(FarDialogItem));
	AnsiDialogItemToUnicodeSafe(diA, di);

	switch (di.Type) {
		case DI_LISTBOX:
		case DI_COMBOBOX: {
			if (diA.Param.ListItems && IsPtr(diA.Param.ListItems)) {
				l.Items = (FarListItem *)malloc(diA.Param.ListItems->ItemsNumber * sizeof(FarListItem));
				l.ItemsNumber = diA.Param.ListItems->ItemsNumber;

				for (int j = 0; j < diA.Param.ListItems->ItemsNumber; j++) {
					AnsiListItemToUnicode(&diA.Param.ListItems->Items[j], &l.Items[j]);
				}
				di.Param.ListItems = &l;
			}

			break;
		}
		case DI_USERCONTROL:
			di.Param.VBuf = AnsiVBufToUnicode(diA);
			break;
		case DI_EDIT:
		case DI_FIXEDIT: {
			if (diA.Flags & oldfar::DIF_HISTORY && diA.Param.History)
				di.Param.History = AnsiToUnicode(diA.Param.History);
			else if (diA.Flags & oldfar::DIF_MASKEDIT && diA.Param.Mask)
				di.Param.Mask = AnsiToUnicode(diA.Param.Mask);

			break;
		}
	}

	if (diA.Type == oldfar::DI_USERCONTROL) {
		di.PtrData = (wchar_t *)malloc(sizeof(diA.Data.Data));

		if (di.PtrData)
			memcpy((char *)di.PtrData, diA.Data.Data, sizeof(diA.Data.Data));

		di.MaxLen = 0;
	} else if ((diA.Type == oldfar::DI_EDIT || diA.Type == oldfar::DI_COMBOBOX)
			&& diA.Flags & oldfar::DIF_VAREDIT)
		di.PtrData = AnsiToUnicode(diA.Data.Ptr.PtrData);
	else
		di.PtrData = AnsiToUnicode(diA.Data.Data);

	// BUGBUG тут надо придумать как сделать лучше: maxlen=513 например и также подумать что делать для DIF_VAREDIT
	// di->MaxLen = 0;
}

void FreeUnicodeDialogItem(FarDialogItem &di)
{
	switch (di.Type) {
		case DI_EDIT:
		case DI_FIXEDIT:

			if ((di.Flags & DIF_HISTORY) && di.Param.History)
				free((void *)di.Param.History);
			else if ((di.Flags & DIF_MASKEDIT) && di.Param.Mask)
				free((void *)di.Param.Mask);

			break;
		case DI_LISTBOX:
		case DI_COMBOBOX:

			if (di.Param.ListItems) {
				if (di.Param.ListItems->Items) {
					for (int i = 0; i < di.Param.ListItems->ItemsNumber; i++) {
						if (di.Param.ListItems->Items[i].Text)
							free((void *)di.Param.ListItems->Items[i].Text);
					}

					free(di.Param.ListItems->Items);
					di.Param.ListItems->Items = nullptr;
				}
			}

			break;
		case DI_USERCONTROL:

			if (di.Param.VBuf)
				free(di.Param.VBuf);

			break;
	}

	if (di.PtrData)
		free((void *)di.PtrData);
}

void FreeAnsiDialogItem(oldfar::FarDialogItem &diA)
{
	if ((diA.Type == oldfar::DI_EDIT || diA.Type == oldfar::DI_FIXEDIT)
			&& (diA.Flags & oldfar::DIF_HISTORY || diA.Flags & oldfar::DIF_MASKEDIT) && diA.Param.History)
		free((void *)diA.Param.History);

	if ((diA.Type == oldfar::DI_EDIT || diA.Type == oldfar::DI_COMBOBOX) && diA.Flags & oldfar::DIF_VAREDIT
			&& diA.Data.Ptr.PtrData)
		free(diA.Data.Ptr.PtrData);

	memset(&diA, 0, sizeof(oldfar::FarDialogItem));
}

void UnicodeDialogItemToAnsiSafe(FarDialogItem &di, oldfar::FarDialogItem &diA)
{
	switch (di.Type) {
		case DI_TEXT:
			diA.Type = oldfar::DI_TEXT;
			break;
		case DI_VTEXT:
			diA.Type = oldfar::DI_VTEXT;
			break;
		case DI_SINGLEBOX:
			diA.Type = oldfar::DI_SINGLEBOX;
			break;
		case DI_DOUBLEBOX:
			diA.Type = oldfar::DI_DOUBLEBOX;
			break;
		case DI_EDIT:
			diA.Type = oldfar::DI_EDIT;
			break;
		case DI_PSWEDIT:
			diA.Type = oldfar::DI_PSWEDIT;
			break;
		case DI_FIXEDIT:
			diA.Type = oldfar::DI_FIXEDIT;
			break;
		case DI_BUTTON:
			diA.Type = oldfar::DI_BUTTON;
			diA.Param.Selected = di.Param.Selected;
			break;
		case DI_CHECKBOX:
			diA.Type = oldfar::DI_CHECKBOX;
			diA.Param.Selected = di.Param.Selected;
			break;
		case DI_RADIOBUTTON:
			diA.Type = oldfar::DI_RADIOBUTTON;
			diA.Param.Selected = di.Param.Selected;
			break;
		case DI_COMBOBOX:
			diA.Type = oldfar::DI_COMBOBOX;
			diA.Param.ListPos = di.Param.ListPos;
			break;
		case DI_LISTBOX:
			diA.Type = oldfar::DI_LISTBOX;
			diA.Param.ListPos = di.Param.ListPos;
			break;
		case DI_MEMOEDIT:
			diA.Type = oldfar::DI_MEMOEDIT;
			break;
		case DI_USERCONTROL:
			diA.Type = oldfar::DI_USERCONTROL;
			break;
	}

	diA.X1 = di.X1;
	diA.Y1 = di.Y1;
	diA.X2 = di.X2;
	diA.Y2 = di.Y2;
	diA.Focus = di.Focus;
	diA.Flags = 0;

	if (di.Flags) {
		if (di.Flags & DIF_SETCOLOR)
			diA.Flags|= oldfar::DIF_SETCOLOR | (di.Flags & DIF_COLORMASK);

		if (di.Flags & DIF_BOXCOLOR)
			diA.Flags|= oldfar::DIF_BOXCOLOR;

		if (di.Flags & DIF_GROUP)
			diA.Flags|= oldfar::DIF_GROUP;

		if (di.Flags & DIF_LEFTTEXT)
			diA.Flags|= oldfar::DIF_LEFTTEXT;

		if (di.Flags & DIF_MOVESELECT)
			diA.Flags|= oldfar::DIF_MOVESELECT;

		if (di.Flags & DIF_SHOWAMPERSAND)
			diA.Flags|= oldfar::DIF_SHOWAMPERSAND;

		if (di.Flags & DIF_CENTERGROUP)
			diA.Flags|= oldfar::DIF_CENTERGROUP;

		if (di.Flags & DIF_NOBRACKETS)
			diA.Flags|= oldfar::DIF_NOBRACKETS;

		if (di.Flags & DIF_MANUALADDHISTORY)
			diA.Flags|= oldfar::DIF_MANUALADDHISTORY;

		if (di.Flags & DIF_SEPARATOR)
			diA.Flags|= oldfar::DIF_SEPARATOR;

		if (di.Flags & DIF_SEPARATOR2)
			diA.Flags|= oldfar::DIF_SEPARATOR2;

		if (di.Flags & DIF_EDITOR)
			diA.Flags|= oldfar::DIF_EDITOR;

		if (di.Flags & DIF_LISTNOAMPERSAND)
			diA.Flags|= oldfar::DIF_LISTNOAMPERSAND;

		if (di.Flags & DIF_LISTNOBOX)
			diA.Flags|= oldfar::DIF_LISTNOBOX;

		if (di.Flags & DIF_HISTORY)
			diA.Flags|= oldfar::DIF_HISTORY;

		if (di.Flags & DIF_BTNNOCLOSE)
			diA.Flags|= oldfar::DIF_BTNNOCLOSE;

		if (di.Flags & DIF_CENTERTEXT)
			diA.Flags|= oldfar::DIF_CENTERTEXT;

		if (di.Flags & DIF_SEPARATORUSER)
			diA.Flags|= oldfar::DIF_SEPARATORUSER;

		if (di.Flags & DIF_EDITEXPAND)
			diA.Flags|= oldfar::DIF_EDITEXPAND;

		if (di.Flags & DIF_DROPDOWNLIST)
			diA.Flags|= oldfar::DIF_DROPDOWNLIST;

		if (di.Flags & DIF_USELASTHISTORY)
			diA.Flags|= oldfar::DIF_USELASTHISTORY;

		if (di.Flags & DIF_MASKEDIT)
			diA.Flags|= oldfar::DIF_MASKEDIT;

		if (di.Flags & DIF_SELECTONENTRY)
			diA.Flags|= oldfar::DIF_SELECTONENTRY;

		if (di.Flags & DIF_3STATE)
			diA.Flags|= oldfar::DIF_3STATE;

		if (di.Flags & DIF_EDITPATH)
			diA.Flags|= oldfar::DIF_EDITPATH;

		if (di.Flags & DIF_LISTWRAPMODE)
			diA.Flags|= oldfar::DIF_LISTWRAPMODE;

		if (di.Flags & DIF_LISTAUTOHIGHLIGHT)
			diA.Flags|= oldfar::DIF_LISTAUTOHIGHLIGHT;

		if (di.Flags & DIF_AUTOMATION)
			diA.Flags|= oldfar::DIF_AUTOMATION;

		if (di.Flags & DIF_HIDDEN)
			diA.Flags|= oldfar::DIF_HIDDEN;

		if (di.Flags & DIF_READONLY)
			diA.Flags|= oldfar::DIF_READONLY;

		if (di.Flags & DIF_NOFOCUS)
			diA.Flags|= oldfar::DIF_NOFOCUS;

		if (di.Flags & DIF_DISABLE)
			diA.Flags|= oldfar::DIF_DISABLE;
	}

	diA.DefaultButton = di.DefaultButton;
}

oldfar::FarDialogItem *UnicodeDialogItemToAnsi(FarDialogItem &di, HANDLE hDlg, int ItemNumber)
{
	oldfar::FarDialogItem *diA = CurrentDialogItemA(hDlg, ItemNumber);

	if (!diA) {
		if (OneDialogItem)
			free(OneDialogItem);

		OneDialogItem = (oldfar::FarDialogItem *)malloc(sizeof(oldfar::FarDialogItem));
		memset(OneDialogItem, 0, sizeof(oldfar::FarDialogItem));
		diA = OneDialogItem;
	}

	FreeAnsiDialogItem(*diA);
	UnicodeDialogItemToAnsiSafe(di, *diA);

	switch (diA->Type) {
		case oldfar::DI_USERCONTROL:
			diA->Param.VBuf = GetAnsiVBufPtr(di.Param.VBuf, GetAnsiVBufSize(*diA));
			break;
		case oldfar::DI_EDIT:
		case oldfar::DI_FIXEDIT: {
			if (di.Flags & DIF_HISTORY)
				diA->Param.History = UnicodeToAnsi(di.Param.History);
			else if (di.Flags & DIF_MASKEDIT)
				diA->Param.Mask = UnicodeToAnsi(di.Param.Mask);
		} break;
	}

	if (diA->Type == oldfar::DI_USERCONTROL) {
		if (di.PtrData)
			memcpy(diA->Data.Data, (char *)di.PtrData, sizeof(diA->Data.Data));
	} else if ((diA->Type == oldfar::DI_EDIT || diA->Type == oldfar::DI_COMBOBOX)
			&& diA->Flags & oldfar::DIF_VAREDIT) {
		diA->Data.Ptr.PtrLength = StrLength(di.PtrData);
		diA->Data.Ptr.PtrData = (char *)malloc(4 * (diA->Data.Ptr.PtrLength + 1));
		PWZ_to_PZ(di.PtrData, diA->Data.Ptr.PtrData, 4 * (diA->Data.Ptr.PtrLength + 1));
	} else
		PWZ_to_PZ(di.PtrData, diA->Data.Data, sizeof(diA->Data.Data));

	return diA;
}

LONG_PTR WINAPI DlgProcA(HANDLE hDlg, int Msg, int Param1, LONG_PTR Param2)
{
	static wchar_t *HelpTopic = nullptr;

	switch (Msg) {
		case DN_LISTHOTKEY:
			Msg = oldfar::DN_LISTHOTKEY;
			break;
		case DN_BTNCLICK:
			Msg = oldfar::DN_BTNCLICK;
			break;
		case DN_CTLCOLORDIALOG:
			Msg = oldfar::DN_CTLCOLORDIALOG;
			break;
		case DN_CTLCOLORDLGITEM:
			Msg = oldfar::DN_CTLCOLORDLGITEM;
			break;
		case DN_CTLCOLORDLGLIST:
			Msg = oldfar::DN_CTLCOLORDLGLIST;
			break;
		case DN_DRAWDIALOG:
			Msg = oldfar::DN_DRAWDIALOG;
			break;
		case DN_DRAWDLGITEM: {
			Msg = oldfar::DN_DRAWDLGITEM;
			FarDialogItem *di = (FarDialogItem *)Param2;
			oldfar::FarDialogItem *FarDiA = UnicodeDialogItemToAnsi(*di, hDlg, Param1);
			LONG_PTR ret = CurrentDlgProc(hDlg, Msg, Param1, (LONG_PTR)FarDiA);

			if (ret && (di->Type == DI_USERCONTROL) && (di->Param.VBuf)) {
				AnsiVBufToUnicode(FarDiA->Param.VBuf, di->Param.VBuf, GetAnsiVBufSize(*FarDiA),
						(FarDiA->Flags & oldfar::DIF_NOTCVTUSERCONTROL) == oldfar::DIF_NOTCVTUSERCONTROL);
			}

			return ret;
		}
		case DN_EDITCHANGE:
			Msg = oldfar::DN_EDITCHANGE;
			return Param2 ? CurrentDlgProc(hDlg, Msg, Param1,
						(LONG_PTR)UnicodeDialogItemToAnsi(*((FarDialogItem *)Param2), hDlg, Param1))
						: FALSE;
		case DN_ENTERIDLE:
			Msg = oldfar::DN_ENTERIDLE;
			break;
		case DN_GOTFOCUS:
			Msg = oldfar::DN_GOTFOCUS;
			break;
		case DN_HELP: {
			char *HelpTopicA = UnicodeToAnsi((const wchar_t *)Param2);
			LONG_PTR ret = CurrentDlgProc(hDlg, oldfar::DN_HELP, Param1, (LONG_PTR)HelpTopicA);

			if (ret) {
				if (HelpTopic)
					free(HelpTopic);

				HelpTopic = AnsiToUnicode((const char *)ret);
				ret = (LONG_PTR)HelpTopic;
			}

			free(HelpTopicA);
			return ret;
		}
		case DN_HOTKEY:
			Msg = oldfar::DN_HOTKEY;
			break;
		case DN_INITDIALOG:
			Msg = oldfar::DN_INITDIALOG;
			break;
		case DN_KILLFOCUS:
			Msg = oldfar::DN_KILLFOCUS;
			break;
		case DN_LISTCHANGE:
			Msg = oldfar::DN_LISTCHANGE;
			break;
		case DN_MOUSECLICK:
			Msg = oldfar::DN_MOUSECLICK;
			break;
		case DN_DRAGGED:
			Msg = oldfar::DN_DRAGGED;
			break;
		case DN_RESIZECONSOLE:
			Msg = oldfar::DN_RESIZECONSOLE;
			break;
		case DN_MOUSEEVENT:
			Msg = oldfar::DN_MOUSEEVENT;
			break;
		case DN_DRAWDIALOGDONE:
			Msg = oldfar::DN_DRAWDIALOGDONE;
			break;
		case DM_KILLSAVESCREEN:
			Msg = oldfar::DM_KILLSAVESCREEN;
			break;
		case DM_ALLKEYMODE:
			Msg = oldfar::DM_ALLKEYMODE;
			break;
		case DN_ACTIVATEAPP:
			Msg = oldfar::DN_ACTIVATEAPP;
			break;
		case DN_KEY:
			Msg = oldfar::DN_KEY;
			Param2 = KeyToOldKey((DWORD)Param2);
			break;
	}

	return CurrentDlgProc(hDlg, Msg, Param1, Param2);
}

LONG_PTR WINAPI FarDefDlgProcA(HANDLE hDlg, int Msg, int Param1, LONG_PTR Param2)
{
	if (Msg == DN_DRAWDLGITEM || Msg == DN_EDITCHANGE) {
		FarDialogItem *di = CurrentDialogItem(hDlg, Param1);
		if (di)
			Param2 = (LONG_PTR)di;
	}
	return FarDefDlgProc(hDlg, Msg, Param1, Param2);
}

LONG_PTR WINAPI FarSendDlgMessageA(HANDLE hDlg, int Msg, int Param1, LONG_PTR Param2)
{
	switch (Msg) {
		case oldfar::DM_CLOSE:
			Msg = DM_CLOSE;
			break;
		case oldfar::DM_ENABLE:
			Msg = DM_ENABLE;
			break;
		case oldfar::DM_ENABLEREDRAW:
			Msg = DM_ENABLEREDRAW;
			break;
		case oldfar::DM_GETDLGDATA:
			Msg = DM_GETDLGDATA;
			break;
		case oldfar::DM_GETDLGITEM: {
			size_t item_size = FarSendDlgMessage(hDlg, DM_GETDLGITEM, Param1, 0);

			if (item_size) {
				FarDialogItem *di = (FarDialogItem *)malloc(item_size);

				if (di) {
					FarSendDlgMessage(hDlg, DM_GETDLGITEM, Param1, (LONG_PTR)di);
					oldfar::FarDialogItem *FarDiA = UnicodeDialogItemToAnsi(*di, hDlg, Param1);
					free(di);
					*reinterpret_cast<oldfar::FarDialogItem *>(Param2) = *FarDiA;
					return TRUE;
				}
			}

			return FALSE;
		}
		case oldfar::DM_GETDLGRECT:
			Msg = DM_GETDLGRECT;
			break;
		case oldfar::DM_GETTEXT: {
			if (!Param2)
				return FarSendDlgMessage(hDlg, DM_GETTEXT, Param1, 0);

			oldfar::FarDialogItemData *didA = (oldfar::FarDialogItemData *)Param2;
			if (!didA->PtrLength)	// вот такой хреновый API!!!
				didA->PtrLength = static_cast<int>(FarSendDlgMessage(hDlg, DM_GETTEXT, Param1, 0));
			wchar_t *text = (wchar_t *)malloc((didA->PtrLength + 1) * sizeof(wchar_t));
			// BUGBUG: если didA->PtrLength=0, то вернётся с учётом '\0', в Энц написано, что без, хз как правильно.
			FarDialogItemData did = {(size_t)didA->PtrLength, text};
			LONG_PTR ret = FarSendDlgMessage(hDlg, DM_GETTEXT, Param1, (LONG_PTR)&did);
			didA->PtrLength = (unsigned)did.PtrLength;
			PWZ_to_PZ(text, didA->PtrData, didA->PtrLength + 1);
			free(text);
			return ret;
		}
		case oldfar::DM_GETTEXTLENGTH:
			Msg = DM_GETTEXTLENGTH;
			break;
		case oldfar::DM_KEY: {
			if (!Param1 || !Param2)
				return FALSE;

			int Count = (int)Param1;
			DWORD *KeysA = (DWORD *)Param2;
			DWORD *KeysW = (DWORD *)malloc(Count * sizeof(DWORD));

			for (int i = 0; i < Count; i++) {
				KeysW[i] = OldKeyToKey(KeysA[i]);
			}

			LONG_PTR ret = FarSendDlgMessage(hDlg, DM_KEY, Param1, (LONG_PTR)KeysW);
			free(KeysW);
			return ret;
		}
		case oldfar::DM_MOVEDIALOG:
			Msg = DM_MOVEDIALOG;
			break;
		case oldfar::DM_SETDLGDATA:
			Msg = DM_SETDLGDATA;
			break;
		case oldfar::DM_SETDLGITEM: {
			if (!Param2)
				return FALSE;

			FarDialogItem *di = CurrentDialogItem(hDlg, Param1);

			if (di->Type == DI_LISTBOX || di->Type == DI_COMBOBOX)
				di->Param.ListItems = CurrentList(hDlg, Param1);

			FreeUnicodeDialogItem(*di);
			oldfar::FarDialogItem *diA = (oldfar::FarDialogItem *)Param2;
			AnsiDialogItemToUnicode(*diA, *di, *di->Param.ListItems);
			return FarSendDlgMessage(hDlg, DM_SETDLGITEM, Param1, (LONG_PTR)di);
		}
		case oldfar::DM_SETFOCUS:
			Msg = DM_SETFOCUS;
			break;
		case oldfar::DM_REDRAW:
			Msg = DM_REDRAW;
			break;
		case oldfar::DM_SETTEXT: {
			if (!Param2)
				return 0;

			oldfar::FarDialogItemData *didA = (oldfar::FarDialogItemData *)Param2;

			if (!didA->PtrData)
				return 0;

			wchar_t *text = AnsiToUnicode(didA->PtrData);
			// BUGBUG - PtrLength ни на что не влияет.
			FarDialogItemData di = {(size_t)didA->PtrLength, text};
			LONG_PTR ret = FarSendDlgMessage(hDlg, DM_SETTEXT, Param1, (LONG_PTR)&di);
			free(text);
			return ret;
		}
		case oldfar::DM_SETMAXTEXTLENGTH:
			Msg = DM_SETMAXTEXTLENGTH;
			break;
		case oldfar::DM_SHOWDIALOG:
			Msg = DM_SHOWDIALOG;
			break;
		case oldfar::DM_GETFOCUS:
			Msg = DM_GETFOCUS;
			break;
		case oldfar::DM_GETCURSORPOS:
			Msg = DM_GETCURSORPOS;
			break;
		case oldfar::DM_SETCURSORPOS:
			Msg = DM_SETCURSORPOS;
			break;
		case oldfar::DM_GETTEXTPTR: {
			LONG_PTR length = FarSendDlgMessage(hDlg, DM_GETTEXTPTR, Param1, 0);

			// if (!Param2) return length;

			wchar_t *text = (wchar_t *)malloc((length + 1) * sizeof(wchar_t));
			length = FarSendDlgMessage(hDlg, DM_GETTEXTPTR, Param1, (LONG_PTR)text);
			length = PWZ_to_PZ(text, (char *)Param2, Param2 ? length + 1 : 0);
			free(text);
			return length + 1;
		}
		case oldfar::DM_SETTEXTPTR: {
			if (!Param2)
				return FALSE;

			wchar_t *text = AnsiToUnicode((char *)Param2);
			LONG_PTR ret = FarSendDlgMessage(hDlg, DM_SETTEXTPTR, Param1, (LONG_PTR)text);
			free(text);
			return ret;
		}
		case oldfar::DM_SHOWITEM:
			Msg = DM_SHOWITEM;
			break;
		case oldfar::DM_ADDHISTORY: {
			if (!Param2)
				return FALSE;

			wchar_t *history = AnsiToUnicode((char *)Param2);
			LONG_PTR ret = FarSendDlgMessage(hDlg, DM_ADDHISTORY, Param1, (LONG_PTR)history);
			free(history);
			return ret;
		}
		case oldfar::DM_GETCHECK: {
			LONG_PTR ret = FarSendDlgMessage(hDlg, DM_GETCHECK, Param1, 0);
			LONG_PTR state = 0;

			if (ret == oldfar::BSTATE_UNCHECKED)
				state = BSTATE_UNCHECKED;
			else if (ret == oldfar::BSTATE_CHECKED)
				state = BSTATE_CHECKED;
			else if (ret == oldfar::BSTATE_3STATE)
				state = BSTATE_3STATE;

			return state;
		}
		case oldfar::DM_SETCHECK: {
			LONG_PTR state = 0;

			if (Param2 == oldfar::BSTATE_UNCHECKED)
				state = BSTATE_UNCHECKED;
			else if (Param2 == oldfar::BSTATE_CHECKED)
				state = BSTATE_CHECKED;
			else if (Param2 == oldfar::BSTATE_3STATE)
				state = BSTATE_3STATE;
			else if (Param2 == oldfar::BSTATE_TOGGLE)
				state = BSTATE_TOGGLE;

			return FarSendDlgMessage(hDlg, DM_SETCHECK, Param1, state);
		}
		case oldfar::DM_SET3STATE:
			Msg = DM_SET3STATE;
			break;
		case oldfar::DM_LISTSORT:
			Msg = DM_LISTSORT;
			break;
		case oldfar::DM_LISTGETITEM:	// BUGBUG, недоделано в фаре
		{
			if (!Param2)
				return FALSE;

			oldfar::FarListGetItem *lgiA = (oldfar::FarListGetItem *)Param2;
			FarListGetItem lgi = {lgiA->ItemIndex};
			LONG_PTR ret = FarSendDlgMessage(hDlg, DM_LISTGETITEM, Param1, (LONG_PTR)&lgi);
			UnicodeListItemToAnsi(&lgi.Item, &lgiA->Item);
			return ret;
		}
		case oldfar::DM_LISTGETCURPOS:

			if (Param2) {
				FarListPos lp;
				LONG_PTR ret = FarSendDlgMessage(hDlg, DM_LISTGETCURPOS, Param1, (LONG_PTR)&lp);
				oldfar::FarListPos *lpA = (oldfar::FarListPos *)Param2;
				lpA->SelectPos = lp.SelectPos;
				lpA->TopPos = lp.TopPos;
				return ret;
			} else
				return FarSendDlgMessage(hDlg, DM_LISTGETCURPOS, Param1, 0);

		case oldfar::DM_LISTSETCURPOS: {
			if (!Param2)
				return FALSE;

			oldfar::FarListPos *lpA = (oldfar::FarListPos *)Param2;
			FarListPos lp = {lpA->SelectPos, lpA->TopPos};
			return FarSendDlgMessage(hDlg, DM_LISTSETCURPOS, Param1, (LONG_PTR)&lp);
		}
		case oldfar::DM_LISTDELETE: {
			oldfar::FarListDelete *ldA = (oldfar::FarListDelete *)Param2;
			FarListDelete ld;

			if (Param2) {
				ld.Count = ldA->Count;
				ld.StartIndex = ldA->StartIndex;
			}

			return FarSendDlgMessage(hDlg, DM_LISTDELETE, Param1, Param2 ? (LONG_PTR)&ld : 0);
		}
		case oldfar::DM_LISTADD: {
			FarList newlist = {0, 0};

			if (Param2) {
				oldfar::FarList *oldlist = (oldfar::FarList *)Param2;
				newlist.ItemsNumber = oldlist->ItemsNumber;

				if (newlist.ItemsNumber) {
					newlist.Items = (FarListItem *)malloc(newlist.ItemsNumber * sizeof(FarListItem));

					if (newlist.Items) {
						for (int i = 0; i < newlist.ItemsNumber; i++)
							AnsiListItemToUnicode(&oldlist->Items[i], &newlist.Items[i]);
					}
				}
			}

			LONG_PTR ret = FarSendDlgMessage(hDlg, DM_LISTADD, Param1, Param2 ? (LONG_PTR)&newlist : 0);

			if (newlist.Items) {
				for (int i = 0; i < newlist.ItemsNumber; i++)
					if (newlist.Items[i].Text)
						free((void *)newlist.Items[i].Text);

				free(newlist.Items);
			}

			return ret;
		}
		case oldfar::DM_LISTADDSTR: {
			wchar_t *newstr = nullptr;

			if (Param2) {
				newstr = AnsiToUnicode((char *)Param2);
			}

			LONG_PTR ret = FarSendDlgMessage(hDlg, DM_LISTADDSTR, Param1, (LONG_PTR)newstr);

			if (newstr)
				free(newstr);

			return ret;
		}
		case oldfar::DM_LISTUPDATE: {
			FarListUpdate newui{};

			if (Param2) {
				oldfar::FarListUpdate *oldui = (oldfar::FarListUpdate *)Param2;
				newui.Index = oldui->Index;
				AnsiListItemToUnicode(&oldui->Item, &newui.Item);
			}

			LONG_PTR ret = FarSendDlgMessage(hDlg, DM_LISTUPDATE, Param1, Param2 ? (LONG_PTR)&newui : 0);

			if (newui.Item.Text)
				free((void *)newui.Item.Text);

			return ret;
		}
		case oldfar::DM_LISTINSERT: {
			FarListInsert newli{};

			if (Param2) {
				oldfar::FarListInsert *oldli = (oldfar::FarListInsert *)Param2;
				newli.Index = oldli->Index;
				AnsiListItemToUnicode(&oldli->Item, &newli.Item);
			}

			LONG_PTR ret = FarSendDlgMessage(hDlg, DM_LISTINSERT, Param1, Param2 ? (LONG_PTR)&newli : 0);

			if (newli.Item.Text)
				free((void *)newli.Item.Text);

			return ret;
		}
		case oldfar::DM_LISTFINDSTRING: {
			FarListFind newlf = {0, 0, 0, 0};

			if (Param2) {
				oldfar::FarListFind *oldlf = (oldfar::FarListFind *)Param2;
				newlf.StartIndex = oldlf->StartIndex;
				newlf.Pattern = (oldlf->Pattern) ? AnsiToUnicode(oldlf->Pattern) : nullptr;

				if (oldlf->Flags & oldfar::LIFIND_EXACTMATCH)
					newlf.Flags|= LIFIND_EXACTMATCH;
			}

			LONG_PTR ret = FarSendDlgMessage(hDlg, DM_LISTFINDSTRING, Param1, Param2 ? (LONG_PTR)&newlf : 0);

			if (newlf.Pattern)
				free((void *)newlf.Pattern);

			return ret;
		}
		case oldfar::DM_LISTINFO: {
			if (!Param2)
				return FALSE;

			oldfar::FarListInfo *liA = (oldfar::FarListInfo *)Param2;
			FarListInfo li = {0, liA->ItemsNumber, liA->SelectPos, liA->TopPos, liA->MaxHeight,
					liA->MaxLength};

			if (liA->Flags & oldfar::LINFO_SHOWNOBOX)
				li.Flags|= LINFO_SHOWNOBOX;

			if (liA->Flags & oldfar::LINFO_AUTOHIGHLIGHT)
				li.Flags|= LINFO_AUTOHIGHLIGHT;

			if (liA->Flags & oldfar::LINFO_REVERSEHIGHLIGHT)
				li.Flags|= LINFO_REVERSEHIGHLIGHT;

			if (liA->Flags & oldfar::LINFO_WRAPMODE)
				li.Flags|= LINFO_WRAPMODE;

			if (liA->Flags & oldfar::LINFO_SHOWAMPERSAND)
				li.Flags|= LINFO_SHOWAMPERSAND;

			return FarSendDlgMessage(hDlg, DM_LISTINFO, Param1, Param2);
		}
		case oldfar::DM_LISTGETDATA:
			Msg = DM_LISTGETDATA;
			break;
		case oldfar::DM_LISTSETDATA: {
			FarListItemData newlid = {0, 0, 0, 0};

			if (Param2) {
				oldfar::FarListItemData *oldlid = (oldfar::FarListItemData *)Param2;
				newlid.Index = oldlid->Index;
				newlid.DataSize = oldlid->DataSize;
				newlid.Data = oldlid->Data;
			}

			LONG_PTR ret = FarSendDlgMessage(hDlg, DM_LISTSETDATA, Param1, Param2 ? (LONG_PTR)&newlid : 0);
			return ret;
		}
		case oldfar::DM_LISTSETTITLES: {
			if (!Param2)
				return FALSE;

			oldfar::FarListTitles *ltA = (oldfar::FarListTitles *)Param2;
			FarListTitles lt = {0, ltA->Title ? AnsiToUnicode(ltA->Title) : nullptr, 0,
					ltA->Bottom ? AnsiToUnicode(ltA->Bottom) : nullptr};
			Param2 = (LONG_PTR)&lt;
			LONG_PTR ret = FarSendDlgMessage(hDlg, DM_LISTSETTITLES, Param1, Param2);

			if (lt.Bottom)
				free((wchar_t *)lt.Bottom);

			if (lt.Title)
				free((wchar_t *)lt.Title);

			return ret;
		}
		case oldfar::DM_LISTGETTITLES: {
			if (Param2) {
				oldfar::FarListTitles *OldListTitle = (oldfar::FarListTitles *)Param2;
				FarListTitles ListTitle = {0, nullptr, 0, nullptr};

				if (OldListTitle->Title) {
					ListTitle.TitleLen = OldListTitle->TitleLen;
					ListTitle.Title = (wchar_t *)malloc(sizeof(wchar_t) * ListTitle.TitleLen);
				}

				if (OldListTitle->BottomLen) {
					ListTitle.BottomLen = OldListTitle->BottomLen;
					ListTitle.Bottom = (wchar_t *)malloc(sizeof(wchar_t) * ListTitle.BottomLen);
				}

				LONG_PTR Ret = FarSendDlgMessage(hDlg, DM_LISTGETTITLES, Param1, (LONG_PTR)&ListTitle);

				if (Ret) {
					PWZ_to_PZ(ListTitle.Title, OldListTitle->Title, OldListTitle->TitleLen);
					PWZ_to_PZ(ListTitle.Bottom, OldListTitle->Bottom, OldListTitle->BottomLen);
				}

				if (ListTitle.Title)
					free((wchar_t *)ListTitle.Title);

				if (ListTitle.Bottom)
					free((wchar_t *)ListTitle.Bottom);

				return Ret;
			}

			return FALSE;
		}
		case oldfar::DM_RESIZEDIALOG:
			Msg = DM_RESIZEDIALOG;
			break;
		case oldfar::DM_SETITEMPOSITION:
			Msg = DM_SETITEMPOSITION;
			break;
		case oldfar::DM_GETDROPDOWNOPENED:
			Msg = DM_GETDROPDOWNOPENED;
			break;
		case oldfar::DM_SETDROPDOWNOPENED:
			Msg = DM_SETDROPDOWNOPENED;
			break;
		case oldfar::DM_SETHISTORY:

			if (!Param2)
				return FarSendDlgMessage(hDlg, DM_SETHISTORY, Param1, 0);
			else {
				FarDialogItem *di = CurrentDialogItem(hDlg, Param1);
				free((void *)di->Param.History);
				di->Param.History = AnsiToUnicode((const char *)Param2);
				return FarSendDlgMessage(hDlg, DM_SETHISTORY, Param1, (LONG_PTR)di->Param.History);
			}

		case oldfar::DM_GETITEMPOSITION:
			Msg = DM_GETITEMPOSITION;
			break;
		case oldfar::DM_SETMOUSEEVENTNOTIFY:
			Msg = DM_SETMOUSEEVENTNOTIFY;
			break;
		case oldfar::DM_EDITUNCHANGEDFLAG:
			Msg = DM_EDITUNCHANGEDFLAG;
			break;
		case oldfar::DM_GETITEMDATA:
			Msg = DM_GETITEMDATA;
			break;
		case oldfar::DM_SETITEMDATA:
			Msg = DM_SETITEMDATA;
			break;
		case oldfar::DM_LISTSET: {
			FarList newlist = {0, 0};

			if (Param2) {
				oldfar::FarList *oldlist = (oldfar::FarList *)Param2;
				newlist.ItemsNumber = oldlist->ItemsNumber;

				if (newlist.ItemsNumber) {
					newlist.Items = (FarListItem *)malloc(newlist.ItemsNumber * sizeof(FarListItem));

					if (newlist.Items) {
						for (int i = 0; i < newlist.ItemsNumber; i++)
							AnsiListItemToUnicode(&oldlist->Items[i], &newlist.Items[i]);
					}
				}
			}

			LONG_PTR ret = FarSendDlgMessage(hDlg, DM_LISTSET, Param1, Param2 ? (LONG_PTR)&newlist : 0);

			if (newlist.Items) {
				for (int i = 0; i < newlist.ItemsNumber; i++)
					if (newlist.Items[i].Text)
						free((void *)newlist.Items[i].Text);

				free(newlist.Items);
			}

			return ret;
		}
		case oldfar::DM_LISTSETMOUSEREACTION: {
			LONG_PTR type = 0;

			if (Param2 == oldfar::LMRT_ONLYFOCUS)
				type = LMRT_ONLYFOCUS;
			else if (Param2 == oldfar::LMRT_ALWAYS)
				type = LMRT_ALWAYS;
			else if (Param2 == oldfar::LMRT_NEVER)
				type = LMRT_NEVER;

			return FarSendDlgMessage(hDlg, DM_LISTSETMOUSEREACTION, Param1, type);
		}
		case oldfar::DM_GETCURSORSIZE:
			Msg = DM_GETCURSORSIZE;
			break;
		case oldfar::DM_SETCURSORSIZE:
			Msg = DM_SETCURSORSIZE;
			break;
		case oldfar::DM_LISTGETDATASIZE:
			Msg = DM_LISTGETDATASIZE;
			break;
		case oldfar::DM_GETSELECTION: {
			if (!Param2)
				return FALSE;

			EditorSelect es;
			LONG_PTR ret = FarSendDlgMessage(hDlg, DM_GETSELECTION, Param1, (LONG_PTR)&es);
			oldfar::EditorSelect *esA = (oldfar::EditorSelect *)Param2;
			esA->BlockType = es.BlockType;
			esA->BlockStartLine = es.BlockStartLine;
			esA->BlockStartPos = es.BlockStartPos;
			esA->BlockWidth = es.BlockWidth;
			esA->BlockHeight = es.BlockHeight;
			return ret;
		}
		case oldfar::DM_SETSELECTION: {
			if (!Param2)
				return FALSE;

			oldfar::EditorSelect *esA = (oldfar::EditorSelect *)Param2;
			EditorSelect es;
			es.BlockType = esA->BlockType;
			es.BlockStartLine = esA->BlockStartLine;
			es.BlockStartPos = esA->BlockStartPos;
			es.BlockWidth = esA->BlockWidth;
			es.BlockHeight = esA->BlockHeight;
			return FarSendDlgMessage(hDlg, DM_SETSELECTION, Param1, (LONG_PTR)&es);
		}
		case oldfar::DM_GETEDITPOSITION:
			Msg = DM_GETEDITPOSITION;
			break;
		case oldfar::DM_SETEDITPOSITION:
			Msg = DM_SETEDITPOSITION;
			break;
		case oldfar::DM_SETCOMBOBOXEVENT:
			Msg = DM_SETCOMBOBOXEVENT;
			break;
		case oldfar::DM_GETCOMBOBOXEVENT:
			Msg = DM_GETCOMBOBOXEVENT;
			break;
		case oldfar::DM_KILLSAVESCREEN:
		case oldfar::DM_ALLKEYMODE:
		case oldfar::DN_ACTIVATEAPP:
			break;
	}

	return FarSendDlgMessage(hDlg, Msg, Param1, Param2);
}

int WINAPI FarDialogExA(INT_PTR PluginNumber, int X1, int Y1, int X2, int Y2, const char *HelpTopic,
		oldfar::FarDialogItem *Item, int ItemsNumber, DWORD Reserved, DWORD Flags,
		oldfar::FARWINDOWPROC DlgProc, LONG_PTR Param)
{
	FARString strHT(HelpTopic);

	if (!Item || !ItemsNumber)
		return -1;

	oldfar::FarDialogItem *diA = new oldfar::FarDialogItem[ItemsNumber]();
	FarDialogItem *di = new FarDialogItem[ItemsNumber]();
	FarList *l = new FarList[ItemsNumber]();

	for (int i = 0; i < ItemsNumber; i++) {
		AnsiDialogItemToUnicode(Item[i], di[i], l[i]);
	}

	DWORD DlgFlags = 0;

	if (Flags & oldfar::FDLG_WARNING)
		DlgFlags|= FDLG_WARNING;

	if (Flags & oldfar::FDLG_SMALLDIALOG)
		DlgFlags|= FDLG_SMALLDIALOG;

	if (Flags & oldfar::FDLG_NODRAWSHADOW)
		DlgFlags|= FDLG_NODRAWSHADOW;

	if (Flags & oldfar::FDLG_NODRAWPANEL)
		DlgFlags|= FDLG_NODRAWPANEL;

	if (Flags & oldfar::FDLG_NONMODAL)
		DlgFlags|= FDLG_NONMODAL;

	if (Flags & oldfar::FDLG_KEEPCONSOLETITLE)
		DlgFlags|= FDLG_KEEPCONSOLETITLE;

	if (Flags & oldfar::FDLG_REGULARIDLE)
		DlgFlags|= FDLG_REGULARIDLE;

	int ret = -1;
	HANDLE hDlg = FarDialogInit(PluginNumber, X1, Y1, X2, Y2, (HelpTopic ? strHT.CPtr() : nullptr),
			(FarDialogItem *)di, ItemsNumber, 0, DlgFlags, DlgProc ? DlgProcA : 0, Param);
	PDialogData NewDialogData = new DialogData;
	NewDialogData->DlgProc = DlgProc;
	NewDialogData->hDlg = hDlg;
	NewDialogData->diA = diA;
	NewDialogData->di = di;
	NewDialogData->l = l;
	DialogList.Push(&NewDialogData);

	if (hDlg != INVALID_HANDLE_VALUE) {
		ret = FarDialogRun(hDlg);

		for (int i = 0; i < ItemsNumber; i++) {
			FarDialogItem *pdi = (FarDialogItem *)malloc(FarSendDlgMessage(hDlg, DM_GETDLGITEM, i, 0));

			if (pdi) {
				FarSendDlgMessage(hDlg, DM_GETDLGITEM, i, (LONG_PTR)pdi);
				UnicodeDialogItemToAnsiSafe(*pdi, Item[i]);
				const wchar_t *res = pdi->PtrData;

				if (!res)
					res = L"";

				if ((di[i].Type == DI_EDIT || di[i].Type == DI_COMBOBOX)
						&& Item[i].Flags & oldfar::DIF_VAREDIT)
					PWZ_to_PZ(res, Item[i].Data.Ptr.PtrData, Item[i].Data.Ptr.PtrLength + 1);
				else
					PWZ_to_PZ(res, Item[i].Data.Data, sizeof(Item[i].Data.Data));

				if (pdi->Type == DI_USERCONTROL) {
					di[i].Param.VBuf = pdi->Param.VBuf;
					Item[i].Param.VBuf = GetAnsiVBufPtr(pdi->Param.VBuf, GetAnsiVBufSize(Item[i]));
				}

				free(pdi);
			}

			FreeAnsiDialogItem(diA[i]);
		}

		FarDialogFree(hDlg);

		for (int i = 0; i < ItemsNumber; i++) {
			if (di[i].Type == DI_LISTBOX || di[i].Type == DI_COMBOBOX)
				di[i].Param.ListItems = CurrentList(hDlg, i);
		}
	}

	for (int i = 0; i < ItemsNumber; i++) {
		FreeUnicodeDialogItem(di[i]);
	}

	for (PDialogData *i = DialogList.Last(); i; i = DialogList.Prev(i)) {
		if ((*i)->hDlg == hDlg) {
			delete *i;
			DialogList.Delete(i);
			break;
		}
	}

	delete[] diA;
	delete[] di;
	delete[] l;

	return ret;
}

int WINAPI FarDialogFnA(INT_PTR PluginNumber, int X1, int Y1, int X2, int Y2, const char *HelpTopic,
		oldfar::FarDialogItem *Item, int ItemsNumber)
{
	return FarDialogExA(PluginNumber, X1, Y1, X2, Y2, HelpTopic, Item, ItemsNumber, 0, 0, 0, 0);
}

void ConvertUnicodePanelInfoToAnsi(PanelInfo *PIW, oldfar::PanelInfo *PIA)
{
	PIA->PanelType = 0;

	switch (PIW->PanelType) {
		case PTYPE_FILEPANEL:
			PIA->PanelType = oldfar::PTYPE_FILEPANEL;
			break;
		case PTYPE_TREEPANEL:
			PIA->PanelType = oldfar::PTYPE_TREEPANEL;
			break;
		case PTYPE_QVIEWPANEL:
			PIA->PanelType = oldfar::PTYPE_QVIEWPANEL;
			break;
		case PTYPE_INFOPANEL:
			PIA->PanelType = oldfar::PTYPE_INFOPANEL;
			break;
	}

	PIA->Plugin = PIW->Plugin;
	PIA->PanelRect.left = PIW->PanelRect.left;
	PIA->PanelRect.top = PIW->PanelRect.top;
	PIA->PanelRect.right = PIW->PanelRect.right;
	PIA->PanelRect.bottom = PIW->PanelRect.bottom;
	PIA->ItemsNumber = PIW->ItemsNumber;
	PIA->SelectedItemsNumber = PIW->SelectedItemsNumber;
	PIA->PanelItems = nullptr;
	PIA->SelectedItems = nullptr;
	PIA->CurrentItem = PIW->CurrentItem;
	PIA->TopPanelItem = PIW->TopPanelItem;
	PIA->Visible = PIW->Visible;
	PIA->Focus = PIW->Focus;
	PIA->ViewMode = PIW->ViewMode;
	PIA->SortMode = 0;

	switch (PIW->SortMode) {
		case SM_DEFAULT:
			PIA->SortMode = oldfar::SM_DEFAULT;
			break;
		case SM_UNSORTED:
			PIA->SortMode = oldfar::SM_UNSORTED;
			break;
		case SM_NAME:
			PIA->SortMode = oldfar::SM_NAME;
			break;
		case SM_EXT:
			PIA->SortMode = oldfar::SM_EXT;
			break;
		case SM_MTIME:
			PIA->SortMode = oldfar::SM_MTIME;
			break;
		case SM_CTIME:
			PIA->SortMode = oldfar::SM_CTIME;
			break;
		case SM_ATIME:
			PIA->SortMode = oldfar::SM_ATIME;
			break;
		case SM_SIZE:
			PIA->SortMode = oldfar::SM_SIZE;
			break;
		case SM_DESCR:
			PIA->SortMode = oldfar::SM_DESCR;
			break;
		case SM_OWNER:
			PIA->SortMode = oldfar::SM_OWNER;
			break;
		case SM_COMPRESSEDSIZE:
			PIA->SortMode = oldfar::SM_COMPRESSEDSIZE;
			break;
		case SM_NUMLINKS:
			PIA->SortMode = oldfar::SM_NUMLINKS;
			break;
	}

	PIA->Flags = 0;

	if (PIW->Flags & PFLAGS_SHOWHIDDEN)
		PIA->Flags|= oldfar::PFLAGS_SHOWHIDDEN;

	if (PIW->Flags & PFLAGS_HIGHLIGHT)
		PIA->Flags|= oldfar::PFLAGS_HIGHLIGHT;

	if (PIW->Flags & PFLAGS_REVERSESORTORDER)
		PIA->Flags|= oldfar::PFLAGS_REVERSESORTORDER;

	if (PIW->Flags & PFLAGS_USESORTGROUPS)
		PIA->Flags|= oldfar::PFLAGS_USESORTGROUPS;

	if (PIW->Flags & PFLAGS_SELECTEDFIRST)
		PIA->Flags|= oldfar::PFLAGS_SELECTEDFIRST;

	if (PIW->Flags & PFLAGS_REALNAMES)
		PIA->Flags|= oldfar::PFLAGS_REALNAMES;

	if (PIW->Flags & PFLAGS_NUMERICSORT)
		PIA->Flags|= oldfar::PFLAGS_NUMERICSORT;

	if (PIW->Flags & PFLAGS_PANELLEFT)
		PIA->Flags|= oldfar::PFLAGS_PANELLEFT;

	PIA->Reserved = PIW->Reserved;
}

void FreeAnsiPanelInfo(oldfar::PanelInfo *PIA)
{
	if (PIA->PanelItems)
		FreePanelItemA(PIA->PanelItems, PIA->ItemsNumber);

	if (PIA->SelectedItems)
		FreePanelItemA(PIA->SelectedItems, PIA->SelectedItemsNumber);

	memset(PIA, 0, sizeof(oldfar::PanelInfo));
}

int WINAPI FarControlA(HANDLE hPlugin, int Command, void *Param)
{
	static oldfar::PanelInfo PanelInfoA{}, AnotherPanelInfoA{};
	static int Reenter = 0;

	if (hPlugin == INVALID_HANDLE_VALUE)
		hPlugin = PANEL_ACTIVE;

	switch (Command) {
		case oldfar::FCTL_CHECKPANELSEXIST:
			return FarControl(hPlugin, FCTL_CHECKPANELSEXIST, 0, (LONG_PTR)Param);
		case oldfar::FCTL_CLOSEPLUGIN: {
			wchar_t *ParamW = nullptr;

			if (Param)
				ParamW = AnsiToUnicode((const char *)Param);

			int ret = FarControl(hPlugin, FCTL_CLOSEPLUGIN, 0, (LONG_PTR)ParamW);

			if (ParamW)
				free(ParamW);

			return ret;
		}
		case oldfar::FCTL_GETANOTHERPANELINFO:
		case oldfar::FCTL_GETPANELINFO: {
			if (!Param)
				return FALSE;

			bool Passive = Command == oldfar::FCTL_GETANOTHERPANELINFO;

			if (Reenter) {
				// Попытка борьбы с рекурсией (вызов GET*PANELINFO из GetOpenPluginInfo).
				// Так как у нас всё статик то должно сработать нормально в 99% случаев
				*(oldfar::PanelInfo *)Param = Passive ? AnotherPanelInfoA : PanelInfoA;
				return TRUE;
			}

			Reenter++;

			if (Passive)
				hPlugin = PANEL_PASSIVE;

			oldfar::PanelInfo *OldPI = Passive ? &AnotherPanelInfoA : &PanelInfoA;
			PanelInfo PI;
			int ret = FarControl(hPlugin, FCTL_GETPANELINFO, 0, (LONG_PTR)&PI);
			FreeAnsiPanelInfo(OldPI);

			if (ret) {
				ConvertUnicodePanelInfoToAnsi(&PI, OldPI);

				if (PI.ItemsNumber) {
					OldPI->PanelItems = (oldfar::PluginPanelItem *)malloc(
							PI.ItemsNumber * sizeof(oldfar::PluginPanelItem));

					if (OldPI->PanelItems) {
						memset(OldPI->PanelItems, 0, PI.ItemsNumber * sizeof(oldfar::PluginPanelItem));
						PluginPanelItem *PPI = nullptr;
						int PPISize = 0;

						for (int i = 0; i < PI.ItemsNumber; i++) {
							int NewPPISize = FarControl(hPlugin, FCTL_GETPANELITEM, i, 0);

							if (NewPPISize > PPISize) {
								PluginPanelItem *NewPPI = (PluginPanelItem *)realloc(PPI, NewPPISize);

								if (NewPPI) {
									PPI = NewPPI;
									PPISize = NewPPISize;
								} else
									break;
							}

							FarControl(hPlugin, FCTL_GETPANELITEM, i, (LONG_PTR)PPI);
							if (PPI) {
								ConvertPanelItemToAnsi(*PPI, OldPI->PanelItems[i]);
							}
						}

						if (PPI)
							free(PPI);
					}
				}

				if (PI.SelectedItemsNumber) {
					OldPI->SelectedItems = (oldfar::PluginPanelItem *)malloc(
							PI.SelectedItemsNumber * sizeof(oldfar::PluginPanelItem));

					if (OldPI->SelectedItems) {
						memset(OldPI->SelectedItems, 0,
								PI.SelectedItemsNumber * sizeof(oldfar::PluginPanelItem));
						PluginPanelItem *PPI = nullptr;
						int PPISize = 0;

						for (int i = 0; i < PI.SelectedItemsNumber; i++) {
							int NewPPISize = FarControl(hPlugin, FCTL_GETSELECTEDPANELITEM, i, 0);

							if (NewPPISize > PPISize) {
								PluginPanelItem *NewPPI = (PluginPanelItem *)realloc(PPI, NewPPISize);

								if (NewPPI) {
									PPI = NewPPI;
									PPISize = NewPPISize;
								} else
									break;
							}

							FarControl(hPlugin, FCTL_GETSELECTEDPANELITEM, i, (LONG_PTR)PPI);
							if (PPI) {
								ConvertPanelItemToAnsi(*PPI, OldPI->SelectedItems[i]);
							}
						}

						if (PPI)
							free(PPI);
					}
				}

				wchar_t CurDir[sizeof(OldPI->CurDir)];
				FarControl(hPlugin, FCTL_GETPANELDIR, sizeof(OldPI->CurDir), (LONG_PTR)CurDir);
				PWZ_to_PZ(CurDir, OldPI->CurDir, sizeof(OldPI->CurDir));
				wchar_t ColumnTypes[sizeof(OldPI->ColumnTypes)];
				FarControl(hPlugin, FCTL_GETCOLUMNTYPES, sizeof(OldPI->ColumnTypes), (LONG_PTR)ColumnTypes);
				PWZ_to_PZ(ColumnTypes, OldPI->ColumnTypes, sizeof(OldPI->ColumnTypes));
				wchar_t ColumnWidths[sizeof(OldPI->ColumnWidths)];
				FarControl(hPlugin, FCTL_GETCOLUMNWIDTHS, sizeof(OldPI->ColumnWidths),
						(LONG_PTR)ColumnWidths);
				PWZ_to_PZ(ColumnWidths, OldPI->ColumnWidths, sizeof(OldPI->ColumnWidths));
				*(oldfar::PanelInfo *)Param = *OldPI;
			} else {
				memset((oldfar::PanelInfo *)Param, 0, sizeof(oldfar::PanelInfo));
			}

			Reenter--;
			return ret;
		}
		case oldfar::FCTL_GETANOTHERPANELSHORTINFO:
		case oldfar::FCTL_GETPANELSHORTINFO: {
			if (!Param)
				return FALSE;

			oldfar::PanelInfo *OldPI = (oldfar::PanelInfo *)Param;
			memset(OldPI, 0, sizeof(oldfar::PanelInfo));

			if (Command == oldfar::FCTL_GETANOTHERPANELSHORTINFO)
				hPlugin = PANEL_PASSIVE;

			PanelInfo PI;
			int ret = FarControl(hPlugin, FCTL_GETPANELINFO, 0, (LONG_PTR)&PI);

			if (ret) {
				ConvertUnicodePanelInfoToAnsi(&PI, OldPI);
				wchar_t CurDir[sizeof(OldPI->CurDir)];
				FarControl(hPlugin, FCTL_GETPANELDIR, sizeof(OldPI->CurDir), (LONG_PTR)CurDir);
				PWZ_to_PZ(CurDir, OldPI->CurDir, sizeof(OldPI->CurDir));
				wchar_t ColumnTypes[sizeof(OldPI->ColumnTypes)];
				FarControl(hPlugin, FCTL_GETCOLUMNTYPES, sizeof(OldPI->ColumnTypes), (LONG_PTR)ColumnTypes);
				PWZ_to_PZ(ColumnTypes, OldPI->ColumnTypes, sizeof(OldPI->ColumnTypes));
				wchar_t ColumnWidths[sizeof(OldPI->ColumnWidths)];
				FarControl(hPlugin, FCTL_GETCOLUMNWIDTHS, sizeof(OldPI->ColumnWidths),
						(LONG_PTR)ColumnWidths);
				PWZ_to_PZ(ColumnWidths, OldPI->ColumnWidths, sizeof(OldPI->ColumnWidths));
			}

			return ret;
		}
		case oldfar::FCTL_SETANOTHERSELECTION:
			hPlugin = PANEL_PASSIVE;
		case oldfar::FCTL_SETSELECTION: {
			if (!Param)
				return FALSE;

			oldfar::PanelInfo *OldPI = (oldfar::PanelInfo *)Param;
			FarControl(hPlugin, FCTL_BEGINSELECTION, 0, 0);

			for (int i = 0; i < OldPI->ItemsNumber; i++) {
				FarControl(hPlugin, FCTL_SETSELECTION, i, OldPI->PanelItems[i].Flags & oldfar::PPIF_SELECTED);
			}

			FarControl(hPlugin, FCTL_ENDSELECTION, 0, 0);
			return TRUE;
		}
		case oldfar::FCTL_REDRAWANOTHERPANEL:
			hPlugin = PANEL_PASSIVE;
		case oldfar::FCTL_REDRAWPANEL: {
			if (!Param)
				return FarControl(hPlugin, FCTL_REDRAWPANEL, 0, 0);

			oldfar::PanelRedrawInfo *priA = (oldfar::PanelRedrawInfo *)Param;
			PanelRedrawInfo pri = {priA->CurrentItem, priA->TopPanelItem};
			return FarControl(hPlugin, FCTL_REDRAWPANEL, 0, (LONG_PTR)&pri);
		}
		case oldfar::FCTL_SETANOTHERNUMERICSORT:
			hPlugin = PANEL_PASSIVE;
		case oldfar::FCTL_SETNUMERICSORT:
			return FarControl(hPlugin, FCTL_SETNUMERICSORT, (Param && (*(int *)Param)) ? 1 : 0, 0);
		case oldfar::FCTL_SETANOTHERPANELDIR:
			hPlugin = PANEL_PASSIVE;
		case oldfar::FCTL_SETPANELDIR: {
			if (!Param)
				return FALSE;

			wchar_t *Dir = AnsiToUnicode((char *)Param);
			int ret = FarControl(hPlugin, FCTL_SETPANELDIR, 0, (LONG_PTR)Dir);
			free(Dir);
			return ret;
		}
		case oldfar::FCTL_SETANOTHERSORTMODE:
			hPlugin = PANEL_PASSIVE;
		case oldfar::FCTL_SETSORTMODE:

			if (!Param)
				return FALSE;

			return FarControl(hPlugin, FCTL_SETSORTMODE, *(int *)Param, 0);
		case oldfar::FCTL_SETANOTHERSORTORDER:
			hPlugin = PANEL_PASSIVE;
		case oldfar::FCTL_SETSORTORDER:
			return FarControl(hPlugin, FCTL_SETSORTORDER, (Param && (*(int *)Param)) ? TRUE : FALSE, 0);
		case oldfar::FCTL_SETANOTHERVIEWMODE:
			hPlugin = PANEL_PASSIVE;
		case oldfar::FCTL_SETVIEWMODE:
			return FarControl(hPlugin, FCTL_SETVIEWMODE, (Param ? *(int *)Param : 0), 0);
		case oldfar::FCTL_UPDATEANOTHERPANEL:
			hPlugin = PANEL_PASSIVE;
		case oldfar::FCTL_UPDATEPANEL:
			return FarControl(hPlugin, FCTL_UPDATEPANEL, Param ? 1 : 0, 0);
		case oldfar::FCTL_GETCMDLINE:
		case oldfar::FCTL_GETCMDLINESELECTEDTEXT: {
			if (Param) {
				int CmdW =
						(Command == oldfar::FCTL_GETCMDLINE) ? FCTL_GETCMDLINE : FCTL_GETCMDLINESELECTEDTEXT;
				wchar_t s[1024];
				FarControl(hPlugin, CmdW, ARRAYSIZE(s), (LONG_PTR)s);
				PWZ_to_PZ(s, (char *)Param, ARRAYSIZE(s));
				return TRUE;
			}

			return FALSE;
		}
		case oldfar::FCTL_GETCMDLINEPOS:

			if (!Param)
				return FALSE;

			return FarControl(hPlugin, FCTL_GETCMDLINEPOS, 0, (LONG_PTR)Param);
		case oldfar::FCTL_GETCMDLINESELECTION: {
			if (!Param)
				return FALSE;

			CmdLineSelect cls;
			int ret = FarControl(hPlugin, FCTL_GETCMDLINESELECTION, 0, (LONG_PTR)&cls);

			if (ret) {
				oldfar::CmdLineSelect *clsA = (oldfar::CmdLineSelect *)Param;
				clsA->SelStart = cls.SelStart;
				clsA->SelEnd = cls.SelEnd;
			}

			return ret;
		}
		case oldfar::FCTL_INSERTCMDLINE: {
			if (!Param)
				return FALSE;

			wchar_t *s = AnsiToUnicode((const char *)Param);
			int ret = FarControl(hPlugin, FCTL_INSERTCMDLINE, 0, (LONG_PTR)s);
			free(s);
			return ret;
		}
		case oldfar::FCTL_SETCMDLINE: {
			if (!Param)
				return FALSE;

			wchar_t *s = AnsiToUnicode((const char *)Param);
			int ret = FarControl(hPlugin, FCTL_SETCMDLINE, 0, (LONG_PTR)s);
			free(s);
			return ret;
		}
		case oldfar::FCTL_SETCMDLINEPOS:

			if (!Param)
				return FALSE;

			return FarControl(hPlugin, FCTL_SETCMDLINEPOS, *(int *)Param, 0);
		case oldfar::FCTL_SETCMDLINESELECTION: {
			if (!Param)
				return FALSE;

			oldfar::CmdLineSelect *clsA = (oldfar::CmdLineSelect *)Param;
			CmdLineSelect cls = {clsA->SelStart, clsA->SelEnd};
			return FarControl(hPlugin, FCTL_SETCMDLINESELECTION, 0, (LONG_PTR)&cls);
		}
		case oldfar::FCTL_GETUSERSCREEN:
			return FarControl(hPlugin, FCTL_GETUSERSCREEN, 0, 0);
		case oldfar::FCTL_SETUSERSCREEN:
			return FarControl(hPlugin, FCTL_SETUSERSCREEN, 0, 0);
	}

	return FALSE;
}

int WINAPI FarGetDirListA(const char *Dir, oldfar::PluginPanelItem **pPanelItem, int *pItemsNumber)
{
	if (!Dir || !*Dir || !pPanelItem || !pItemsNumber)
		return FALSE;

	*pPanelItem = nullptr;
	*pItemsNumber = 0;
	FARString strDir(Dir);
	DeleteEndSlash(strDir, true);

	FAR_FIND_DATA *pItems;
	int ItemsNumber;
	int ret = FarGetDirList(strDir, &pItems, &ItemsNumber);

	size_t PathOffset = ExtractFilePath(strDir).GetLength() + 1;

	if (ret && ItemsNumber) {
		//+sizeof(int) чтоб хранить ItemsNumber ибо в FarFreeDirListA как то надо знать
		*pPanelItem = (oldfar::PluginPanelItem *)malloc(
				ItemsNumber * sizeof(oldfar::PluginPanelItem) + sizeof(int));

		if (*pPanelItem) {
			*pItemsNumber = ItemsNumber;
			**((int **)pPanelItem) = ItemsNumber;
			(*((int **)pPanelItem))++;
			memset(*pPanelItem, 0, ItemsNumber * sizeof(oldfar::PluginPanelItem));

			for (int i = 0; i < ItemsNumber; i++) {
				(*pPanelItem)[i].FindData.dwFileAttributes = pItems[i].dwFileAttributes;
				(*pPanelItem)[i].FindData.ftCreationTime = pItems[i].ftCreationTime;
				(*pPanelItem)[i].FindData.ftLastAccessTime = pItems[i].ftLastAccessTime;
				(*pPanelItem)[i].FindData.ftLastWriteTime = pItems[i].ftLastWriteTime;
				(*pPanelItem)[i].FindData.nFileSize = (DWORD)pItems[i].nFileSize;
				PWZ_to_PZ(pItems[i].lpwszFileName + PathOffset, (*pPanelItem)[i].FindData.cFileName,
						ARRAYSIZE((*pPanelItem)[i].FindData.cFileName));
			}
		} else {
			ret = FALSE;
		}

		FarFreeDirList(pItems, ItemsNumber);
	}

	return ret;
}

int WINAPI FarGetPluginDirListA(INT_PTR PluginNumber, HANDLE hPlugin, const char *Dir,
		oldfar::PluginPanelItem **pPanelItem, int *pItemsNumber)
{
	if (!Dir || !*Dir || !pPanelItem || !pItemsNumber)
		return FALSE;

	*pPanelItem = nullptr;
	*pItemsNumber = 0;
	FARString strDir(Dir);

	PluginPanelItem *pPanelItemW;
	int ItemsNumber;
	int ret = FarGetPluginDirList(PluginNumber, hPlugin, strDir, &pPanelItemW, &ItemsNumber);

	if (ret && ItemsNumber) {
		//+sizeof(int) чтоб хранить ItemsNumber ибо в FarFreeDirListA как то надо знать
		*pPanelItem = (oldfar::PluginPanelItem *)malloc(
				ItemsNumber * sizeof(oldfar::PluginPanelItem) + sizeof(int));

		if (*pPanelItem) {
			*pItemsNumber = ItemsNumber;
			**((int **)pPanelItem) = ItemsNumber;
			(*((int **)pPanelItem))++;
			memset(*pPanelItem, 0, ItemsNumber * sizeof(oldfar::PluginPanelItem));

			for (int i = 0; i < ItemsNumber; i++) {
				ConvertPanelItemToAnsi(pPanelItemW[i], (*pPanelItem)[i]);
			}
		} else {
			ret = FALSE;
		}

		FarFreePluginDirList(pPanelItemW, ItemsNumber);
	}

	return ret;
}

void WINAPI FarFreeDirListA(const oldfar::PluginPanelItem *PanelItem)
{
	if (!PanelItem)
		return;

	// Тут хранится ItemsNumber полученный в FarGetDirListA или FarGetPluginDirListA
	int *base = ((int *)PanelItem) - 1;
	FreePanelItemA((oldfar::PluginPanelItem *)PanelItem, *base, false);
	free(base);
}

INT_PTR WINAPI FarAdvControlA(INT_PTR ModuleNumber, int Command, void *Param)
{
	static char *ErrMsg1 = nullptr;
	static char *ErrMsg2 = nullptr;
	static char *ErrMsg3 = nullptr;

	switch (Command) {
		case oldfar::ACTL_GETFARVERSION: {
			DWORD FarVer = (DWORD)FarAdvControl(ModuleNumber, ACTL_GETFARVERSION, nullptr, nullptr);

			if (Param)
				*(DWORD *)Param = FarVer;

			return FarVer;
		}
		case oldfar::ACTL_CONSOLEMODE:
			return FALSE;

		case oldfar::ACTL_GETSYSWORDDIV: {
			INT_PTR Length = FarAdvControl(ModuleNumber, ACTL_GETSYSWORDDIV, nullptr, nullptr);

			if (Param) {
				wchar_t *SysWordDiv = (wchar_t *)malloc((Length + 1) * sizeof(wchar_t));
				FarAdvControl(ModuleNumber, ACTL_GETSYSWORDDIV, SysWordDiv, nullptr);
				PWZ_to_PZ(SysWordDiv, (char *)Param, oldfar::NM);
				free(SysWordDiv);
			}

			return Length;
		}
		case oldfar::ACTL_WAITKEY:
			return FarAdvControl(ModuleNumber, ACTL_WAITKEY, Param, nullptr);
		case oldfar::ACTL_GETCOLOR: {
			uint64_t color;
				FarAdvControl(ModuleNumber, ACTL_GETCOLOR, Param, &color);
			return (INT_PTR)(color & 0xFFFF);
		}
		case oldfar::ACTL_GETARRAYCOLOR:
			return FarAdvControl(ModuleNumber, ACTL_GETARRAYCOLOR, Param, nullptr);
		case oldfar::ACTL_EJECTMEDIA:
			return FarAdvControl(ModuleNumber, ACTL_EJECTMEDIA, Param, nullptr);
		case oldfar::ACTL_KEYMACRO: {
			if (!Param)
				return FALSE;

			ActlKeyMacro km{};
			oldfar::ActlKeyMacro *kmA = (oldfar::ActlKeyMacro *)Param;

			switch (kmA->Command) {
				case oldfar::MCMD_LOADALL:
					km.Command = MCMD_LOADALL;
					break;
				case oldfar::MCMD_SAVEALL:
					km.Command = MCMD_SAVEALL;
					break;
				case oldfar::MCMD_POSTMACROSTRING:
					km.Command = MCMD_POSTMACROSTRING;
					km.Param.PlainText.SequenceText = AnsiToUnicode(kmA->Param.PlainText.SequenceText);

					if (kmA->Param.PlainText.Flags & oldfar::KSFLAGS_DISABLEOUTPUT)
						km.Param.PlainText.Flags|= KSFLAGS_DISABLEOUTPUT;

					if (kmA->Param.PlainText.Flags & oldfar::KSFLAGS_NOSENDKEYSTOPLUGINS)
						km.Param.PlainText.Flags|= KSFLAGS_NOSENDKEYSTOPLUGINS;

					break;
				case oldfar::MCMD_GETSTATE:
					km.Command = MCMD_GETSTATE;
					break;
					/*
					case oldfar::MCMD_COMPILEMACRO:
					km.Command=MCMD_COMPILEMACRO;
					km.Param.Compile.Count = kmA->Param.Compile.Count;
					km.Param.Compile.Flags = kmA->Param.Compile.Flags;
					km.Param.Compile.Sequence = AnsiToUnicode(kmA->Param.Compile.Sequence);
					break;
					*/
				case oldfar::MCMD_CHECKMACRO:
					km.Command = MCMD_CHECKMACRO;
					km.Param.PlainText.SequenceText = AnsiToUnicode(kmA->Param.PlainText.SequenceText);
					break;
			}

			INT_PTR res = FarAdvControl(ModuleNumber, ACTL_KEYMACRO, &km, nullptr);

			switch (km.Command) {
				case MCMD_CHECKMACRO: {

					if (ErrMsg1)
						free(ErrMsg1);

					if (ErrMsg2)
						free(ErrMsg2);

					if (ErrMsg3)
						free(ErrMsg3);

					FARString ErrMessage[3];

					CtrlObject->Macro.GetMacroParseError(&ErrMessage[0], &ErrMessage[1], &ErrMessage[2],
							nullptr);

					kmA->Param.MacroResult.ErrMsg1 = ErrMsg1 = UnicodeToAnsi(ErrMessage[0]);
					kmA->Param.MacroResult.ErrMsg2 = ErrMsg2 = UnicodeToAnsi(ErrMessage[1]);
					kmA->Param.MacroResult.ErrMsg3 = ErrMsg3 = UnicodeToAnsi(ErrMessage[2]);

					if (km.Param.PlainText.SequenceText)
						free((void *)km.Param.PlainText.SequenceText);

					break;
				}

				case MCMD_COMPILEMACRO:

					if (km.Param.Compile.Sequence)
						free((void *)km.Param.Compile.Sequence);

					break;
				case MCMD_POSTMACROSTRING:

					if (km.Param.PlainText.SequenceText)
						free((void *)km.Param.PlainText.SequenceText);

					break;
			}

			return res;
		}
		case oldfar::ACTL_POSTKEYSEQUENCE: {
			if (!Param)
				return FALSE;

			KeySequence ks{};
			oldfar::KeySequence *ksA = (oldfar::KeySequence *)Param;

			if (!ksA->Count || !ksA->Sequence)
				return FALSE;

			ks.Count = ksA->Count;

			if (ksA->Flags & oldfar::KSFLAGS_DISABLEOUTPUT)
				ks.Flags|= KSFLAGS_DISABLEOUTPUT;

			if (ksA->Flags & oldfar::KSFLAGS_NOSENDKEYSTOPLUGINS)
				ks.Flags|= KSFLAGS_NOSENDKEYSTOPLUGINS;

			DWORD *Sequence = (DWORD *)malloc(ks.Count * sizeof(DWORD));
			for (int i = 0; i < ks.Count; i++) {
				Sequence[i] = OldKeyToKey(ksA->Sequence[i]);
			}
			ks.Sequence = Sequence;

			LONG_PTR ret = FarAdvControl(ModuleNumber, ACTL_POSTKEYSEQUENCE, &ks, nullptr);
			free(Sequence);
			return ret;
		}
		case oldfar::ACTL_GETSHORTWINDOWINFO:
		case oldfar::ACTL_GETWINDOWINFO: {
			if (!Param)
				return FALSE;

			int cmd = (Command == oldfar::ACTL_GETWINDOWINFO) ? ACTL_GETWINDOWINFO : ACTL_GETSHORTWINDOWINFO;
			oldfar::WindowInfo *wiA = (oldfar::WindowInfo *)Param;
			WindowInfo wi{};
			wi.Pos = wiA->Pos;
			INT_PTR ret = FarAdvControl(ModuleNumber, cmd, &wi, nullptr);

			if (ret) {
				switch (wi.Type) {
					case WTYPE_PANELS:
						wiA->Type = oldfar::WTYPE_PANELS;
						break;
					case WTYPE_VIEWER:
						wiA->Type = oldfar::WTYPE_VIEWER;
						break;
					case WTYPE_EDITOR:
						wiA->Type = oldfar::WTYPE_EDITOR;
						break;
					case WTYPE_DIALOG:
						wiA->Type = oldfar::WTYPE_DIALOG;
						break;
					case WTYPE_VMENU:
						wiA->Type = oldfar::WTYPE_VMENU;
						break;
					case WTYPE_HELP:
						wiA->Type = oldfar::WTYPE_HELP;
						break;
				}

				wiA->Modified = wi.Modified;
				wiA->Current = wi.Current;

				if (cmd == ACTL_GETWINDOWINFO) {
					if (wi.TypeNameSize) {
						wi.TypeName = new wchar_t[wi.TypeNameSize];
					}

					if (wi.NameSize) {
						wi.Name = new wchar_t[wi.NameSize];
					}

					if (wi.TypeName && wi.Name) {
						FarAdvControl(ModuleNumber, ACTL_GETWINDOWINFO, &wi, nullptr);
						PWZ_to_PZ(wi.TypeName, wiA->TypeName, sizeof(wiA->TypeName));
						PWZ_to_PZ(wi.Name, wiA->Name, sizeof(wiA->Name));
					}

					if (wi.TypeName) {
						delete[] wi.TypeName;
					}

					if (wi.Name) {
						delete[] wi.Name;
					}
				} else {
					*wiA->TypeName = 0;
					*wiA->Name = 0;
				}
			}

			return ret;
		}
		case oldfar::ACTL_GETWINDOWCOUNT:
			return FarAdvControl(ModuleNumber, ACTL_GETWINDOWCOUNT, nullptr, nullptr);
		case oldfar::ACTL_SETCURRENTWINDOW:
			return FarAdvControl(ModuleNumber, ACTL_SETCURRENTWINDOW, Param, nullptr);
		case oldfar::ACTL_COMMIT:
			return FarAdvControl(ModuleNumber, ACTL_COMMIT, nullptr, nullptr);
		case oldfar::ACTL_GETFARHWND:
			return FarAdvControl(ModuleNumber, ACTL_GETFARHWND, nullptr, nullptr);
		case oldfar::ACTL_GETSYSTEMSETTINGS: {
			INT_PTR ss = FarAdvControl(ModuleNumber, ACTL_GETSYSTEMSETTINGS, nullptr, nullptr);
			INT_PTR ret = 0;

			if (ss & oldfar::FSS_DELETETORECYCLEBIN)
				ret|= FSS_DELETETORECYCLEBIN;

			if (ss & oldfar::FSS_WRITETHROUGH)
				ret|= FSS_WRITETHROUGH;

			if (ss & oldfar::FSS_SAVECOMMANDSHISTORY)
				ret|= FSS_SAVECOMMANDSHISTORY;

			if (ss & oldfar::FSS_SAVEFOLDERSHISTORY)
				ret|= FSS_SAVEFOLDERSHISTORY;

			if (ss & oldfar::FSS_SAVEVIEWANDEDITHISTORY)
				ret|= FSS_SAVEVIEWANDEDITHISTORY;

			if (ss & oldfar::FSS_USEWINDOWSREGISTEREDTYPES)
				ret|= FSS_USEWINDOWSREGISTEREDTYPES;

			if (ss & oldfar::FSS_AUTOSAVESETUP)
				ret|= FSS_AUTOSAVESETUP;

			if (ss & oldfar::FSS_SCANSYMLINK)
				ret|= FSS_SCANSYMLINK;

			return ret;
		}
		case oldfar::ACTL_GETPANELSETTINGS: {
			INT_PTR ps = FarAdvControl(ModuleNumber, ACTL_GETPANELSETTINGS, nullptr,  nullptr);
			INT_PTR ret = 0;

			if (ps & oldfar::FPS_SHOWHIDDENANDSYSTEMFILES)
				ret|= FPS_SHOWHIDDENANDSYSTEMFILES;

			if (ps & oldfar::FPS_HIGHLIGHTFILES)
				ret|= FPS_HIGHLIGHTFILES;

			if (ps & oldfar::FPS_AUTOCHANGEFOLDER)
				ret|= FPS_AUTOCHANGEFOLDER;

			if (ps & oldfar::FPS_SELECTFOLDERS)
				ret|= FPS_SELECTFOLDERS;

			if (ps & oldfar::FPS_ALLOWREVERSESORTMODES)
				ret|= FPS_ALLOWREVERSESORTMODES;

			if (ps & oldfar::FPS_SHOWCOLUMNTITLES)
				ret|= FPS_SHOWCOLUMNTITLES;

			if (ps & oldfar::FPS_SHOWSTATUSLINE)
				ret|= FPS_SHOWSTATUSLINE;

			if (ps & oldfar::FPS_SHOWFILESTOTALINFORMATION)
				ret|= FPS_SHOWFILESTOTALINFORMATION;

			if (ps & oldfar::FPS_SHOWFREESIZE)
				ret|= FPS_SHOWFREESIZE;

			if (ps & oldfar::FPS_SHOWSCROLLBAR)
				ret|= FPS_SHOWSCROLLBAR;

			if (ps & oldfar::FPS_SHOWBACKGROUNDSCREENSNUMBER)
				ret|= FPS_SHOWBACKGROUNDSCREENSNUMBER;

			if (ps & oldfar::FPS_SHOWSORTMODELETTER)
				ret|= FPS_SHOWSORTMODELETTER;

			return ret;
		}
		case oldfar::ACTL_GETINTERFACESETTINGS: {
			INT_PTR is = FarAdvControl(ModuleNumber, ACTL_GETINTERFACESETTINGS, nullptr, nullptr);
			INT_PTR ret = 0;

			if (is & oldfar::FIS_CLOCKINPANELS)
				ret|= FIS_CLOCKINPANELS;

			if (is & oldfar::FIS_CLOCKINVIEWERANDEDITOR)
				ret|= FIS_CLOCKINVIEWERANDEDITOR;

			if (is & oldfar::FIS_MOUSE)
				ret|= FIS_MOUSE;

			if (is & oldfar::FIS_SHOWKEYBAR)
				ret|= FIS_SHOWKEYBAR;

			if (is & oldfar::FIS_ALWAYSSHOWMENUBAR)
				ret|= FIS_ALWAYSSHOWMENUBAR;

			if (is & oldfar::FIS_SHOWTOTALCOPYPROGRESSINDICATOR)
				ret|= FIS_SHOWTOTALCOPYPROGRESSINDICATOR;

			if (is & oldfar::FIS_SHOWCOPYINGTIMEINFO)
				ret|= FIS_SHOWCOPYINGTIMEINFO;

			if (is & oldfar::FIS_USECTRLPGUPTOCHANGEDRIVE)
				ret|= FIS_USECTRLPGUPTOCHANGEDRIVE;

			return ret;
		}
		case oldfar::ACTL_GETCONFIRMATIONS: {
			INT_PTR cs = FarAdvControl(ModuleNumber, ACTL_GETCONFIRMATIONS, nullptr, nullptr);
			INT_PTR ret = 0;

			if (cs & oldfar::FCS_COPYOVERWRITE)
				ret|= FCS_COPYOVERWRITE;

			if (cs & oldfar::FCS_MOVEOVERWRITE)
				ret|= FCS_MOVEOVERWRITE;

			if (cs & oldfar::FCS_DRAGANDDROP)
				ret|= FCS_DRAGANDDROP;

			if (cs & oldfar::FCS_DELETE)
				ret|= FCS_DELETE;

			if (cs & oldfar::FCS_DELETENONEMPTYFOLDERS)
				ret|= FCS_DELETENONEMPTYFOLDERS;

			if (cs & oldfar::FCS_INTERRUPTOPERATION)
				ret|= FCS_INTERRUPTOPERATION;

			if (cs & oldfar::FCS_DISCONNECTNETWORKDRIVE)
				ret|= FCS_DISCONNECTNETWORKDRIVE;

			if (cs & oldfar::FCS_RELOADEDITEDFILE)
				ret|= FCS_RELOADEDITEDFILE;

			if (cs & oldfar::FCS_CLEARHISTORYLIST)
				ret|= FCS_CLEARHISTORYLIST;

			if (cs & oldfar::FCS_EXIT)
				ret|= FCS_EXIT;

			return ret;
		}
		case oldfar::ACTL_GETDESCSETTINGS: {
			INT_PTR ds = FarAdvControl(ModuleNumber, ACTL_GETDESCSETTINGS, nullptr, nullptr);
			INT_PTR ret = 0;

			if (ds & oldfar::FDS_UPDATEALWAYS)
				ret|= FDS_UPDATEALWAYS;

			if (ds & oldfar::FDS_UPDATEIFDISPLAYED)
				ret|= FDS_UPDATEIFDISPLAYED;

			if (ds & oldfar::FDS_SETHIDDEN)
				ret|= FDS_SETHIDDEN;

			if (ds & oldfar::FDS_UPDATEREADONLY)
				ret|= FDS_UPDATEREADONLY;

			return ret;
		}
		case oldfar::ACTL_SETARRAYCOLOR: {
			return FALSE;

//			if (!Param)
//				return FALSE;

//			oldfar::FarSetColors *scA = (oldfar::FarSetColors *)Param;
//			FarSetColors sc = {0, scA->StartIndex, scA->ColorCount, scA->Colors};

//			if (scA->Flags & oldfar::FCLR_REDRAW)
//				sc.Flags|= FCLR_REDRAW;

//			return FarAdvControl(ModuleNumber, ACTL_SETARRAYCOLOR, &sc);
		}
		case oldfar::ACTL_GETWCHARMODE:
			return TRUE;
		case oldfar::ACTL_GETPLUGINMAXREADDATA:
			return FarAdvControl(ModuleNumber, ACTL_GETPLUGINMAXREADDATA, nullptr, nullptr);
		case oldfar::ACTL_GETDIALOGSETTINGS: {
			INT_PTR ds = FarAdvControl(ModuleNumber, ACTL_GETDIALOGSETTINGS, nullptr, nullptr);
			INT_PTR ret = 0;

			if (ds & oldfar::FDIS_HISTORYINDIALOGEDITCONTROLS)
				ret|= FDIS_HISTORYINDIALOGEDITCONTROLS;

			if (ds & oldfar::FDIS_AUTOCOMPLETEININPUTLINES)
				ret|= FDIS_AUTOCOMPLETEININPUTLINES;

			if (ds & oldfar::FDIS_PERSISTENTBLOCKSINEDITCONTROLS)
				ret|= FDIS_PERSISTENTBLOCKSINEDITCONTROLS;

			if (ds & oldfar::FDIS_BSDELETEUNCHANGEDTEXT)
				ret|= FDIS_BSDELETEUNCHANGEDTEXT;

			return ret;
		}
		case oldfar::ACTL_REMOVEMEDIA:
		case oldfar::ACTL_GETMEDIATYPE:
		case oldfar::ACTL_GETPOLICIES:
			return FALSE;
		case oldfar::ACTL_REDRAWALL:
			return FarAdvControl(ModuleNumber, ACTL_REDRAWALL, nullptr, nullptr);
	}

	return FALSE;
}

UINT GetEditorCodePageA()
{
	EditorInfo info{};
	FarEditorControl(ECTL_GETINFO, &info);
	UINT CodePage = info.CodePage;
	CPINFO cpi;

	if (!WINPORT(GetCPInfo)(CodePage, &cpi) || cpi.MaxCharSize > 1)
		CodePage = WINPORT(GetACP)();

	return CodePage;
}

int GetEditorCodePageFavA()
{
	UINT CodePage = GetEditorCodePageA();
	DWORD FavIndex = 2;
	int result = -((int)CodePage + 2);

	if (WINPORT(GetOEMCP)() == CodePage) {
		result = 0;
	} else if (WINPORT(GetACP)() == CodePage) {
		result = 1;
	} else {
		ConfigReader cfg_reader;
		const auto &codepages = cfg_reader.EnumKeys();
		for (const auto &cp : codepages) {
			int selectType = cfg_reader.GetInt(cp, 0);
			if (!(selectType & CPST_FAVORITE))
				continue;

			UINT nCP = atoi(cp.c_str());

			if (nCP == CodePage) {
				result = FavIndex;
				break;
			}

			FavIndex++;
		}
	}

	return result;
}

void MultiByteRecode(UINT nCPin, UINT nCPout, char *szBuffer, int nLength)
{
	if (szBuffer && nLength > 0) {
		wchar_t *wszTempTable = (wchar_t *)malloc(nLength * sizeof(wchar_t));

		if (wszTempTable) {
			ErrnoSaver ErSr;
			WINPORT(MultiByteToWideChar)(nCPin, 0, szBuffer, nLength, wszTempTable, nLength);
			WINPORT(WideCharToMultiByte)
			(nCPout, 0, wszTempTable, nLength, szBuffer, nLength, nullptr, nullptr);
			free(wszTempTable);
		}
	}
};

static UINT ConvertCharTableToCodePage(int Command)
{
	UINT nCP = CP_AUTODETECT;

	if (Command < 0) {
		nCP = -(Command + 2);
	} else {
		switch (Command) {
			case 0 /* OEM */:
				nCP = WINPORT(GetOEMCP)();
				break;
			case 1 /* ANSI */:
				nCP = WINPORT(GetACP)();
				break;
			default: {
				int FavIndex = 2;
				ConfigReader cfg_reader(FavoriteCodePagesKey);
				const auto &codepages = cfg_reader.EnumKeys();
				for (const auto &cp : codepages) {
					int selectType = cfg_reader.GetInt(cp, 0);
					if (!(selectType & CPST_FAVORITE))
						continue;

					if (FavIndex == Command) {
						nCP = atoi(cp.c_str());
						break;
					}

					FavIndex++;
				}
			}
		}
	}

	return nCP;
}

int WINAPI FarEditorControlA(int Command, void *Param)
{
	static char *gt = nullptr;
	static char *geol = nullptr;
	static char *fn = nullptr;

	switch (Command) {
		case oldfar::ECTL_ADDCOLOR:
			Command = ECTL_ADDCOLOR;
			break;
		case oldfar::ECTL_ADDTRUECOLOR:
			Command = ECTL_ADDTRUECOLOR;
			break;
		case oldfar::ECTL_DELETEBLOCK:
			Command = ECTL_DELETEBLOCK;
			break;
		case oldfar::ECTL_DELETECHAR:
			Command = ECTL_DELETECHAR;
			break;
		case oldfar::ECTL_DELETESTRING:
			Command = ECTL_DELETESTRING;
			break;
		case oldfar::ECTL_EXPANDTABS:
			Command = ECTL_EXPANDTABS;
			break;
		case oldfar::ECTL_GETCOLOR:
			Command = ECTL_GETCOLOR;
			break;
		case oldfar::ECTL_GETTRUECOLOR:
			Command = ECTL_GETTRUECOLOR;
			break;
		case oldfar::ECTL_GETBOOKMARKS:
			Command = ECTL_GETBOOKMARKS;
			break;
		case oldfar::ECTL_INSERTSTRING:
			Command = ECTL_INSERTSTRING;
			break;
		case oldfar::ECTL_QUIT:
			Command = ECTL_QUIT;
			break;
		case oldfar::ECTL_REALTOTAB:
			Command = ECTL_REALTOTAB;
			break;
		case oldfar::ECTL_REDRAW:
			Command = ECTL_REDRAW;
			break;
		case oldfar::ECTL_SELECT:
			Command = ECTL_SELECT;
			break;
		case oldfar::ECTL_SETPOSITION:
			Command = ECTL_SETPOSITION;
			break;
		case oldfar::ECTL_TABTOREAL:
			Command = ECTL_TABTOREAL;
			break;
		case oldfar::ECTL_TURNOFFMARKINGBLOCK:
			Command = ECTL_TURNOFFMARKINGBLOCK;
			break;
		case oldfar::ECTL_ADDSTACKBOOKMARK:
			Command = ECTL_ADDSTACKBOOKMARK;
			break;
		case oldfar::ECTL_PREVSTACKBOOKMARK:
			Command = ECTL_PREVSTACKBOOKMARK;
			break;
		case oldfar::ECTL_NEXTSTACKBOOKMARK:
			Command = ECTL_NEXTSTACKBOOKMARK;
			break;
		case oldfar::ECTL_CLEARSTACKBOOKMARKS:
			Command = ECTL_CLEARSTACKBOOKMARKS;
			break;
		case oldfar::ECTL_DELETESTACKBOOKMARK:
			Command = ECTL_DELETESTACKBOOKMARK;
			break;
		case oldfar::ECTL_GETSTACKBOOKMARKS:
			Command = ECTL_GETSTACKBOOKMARKS;
			break;
		case oldfar::ECTL_GETSTRING: {
			EditorGetString egs;
			oldfar::EditorGetString *oegs = (oldfar::EditorGetString *)Param;

			if (!oegs)
				return FALSE;

			egs.StringNumber = oegs->StringNumber;
			int ret = FarEditorControl(ECTL_GETSTRING, &egs);

			if (ret) {
				oegs->StringNumber = egs.StringNumber;
				oegs->StringLength = egs.StringLength;
				oegs->SelStart = egs.SelStart;
				oegs->SelEnd = egs.SelEnd;

				if (gt)
					free(gt);

				if (geol)
					free(geol);

				UINT CodePage = GetEditorCodePageA();
				gt = UnicodeToAnsiBin(egs.StringText, egs.StringLength, CodePage);
				geol = UnicodeToAnsi(egs.StringEOL, CodePage);
				oegs->StringText = gt;
				oegs->StringEOL = geol;
				return TRUE;
			}

			return FALSE;
		}
		case oldfar::ECTL_INSERTTEXT: {
			const char *p = (const char *)Param;

			if (!p)
				return FALSE;

			FARString strP(p);
			return FarEditorControl(ECTL_INSERTTEXT, (void *)strP.CPtr());
		}
		case oldfar::ECTL_GETINFO: {
			EditorInfo ei{};
			oldfar::EditorInfo *oei = (oldfar::EditorInfo *)Param;

			if (!oei)
				return FALSE;

			int ret = FarEditorControl(ECTL_GETINFO, &ei);

			if (ret) {
				if (fn)
					free(fn);

				memset(oei, 0, sizeof(*oei));
				size_t FileNameSize = FarEditorControl(ECTL_GETFILENAME, nullptr);

				if (FileNameSize) {
					LPWSTR FileName = new wchar_t[FileNameSize];
					FarEditorControl(ECTL_GETFILENAME, FileName);
					fn = UnicodeToAnsi(FileName);
					oei->FileName = fn;
					delete[] FileName;
				}

				oei->EditorID = ei.EditorID;
				oei->WindowSizeX = ei.WindowSizeX;
				oei->WindowSizeY = ei.WindowSizeY;
				oei->TotalLines = ei.TotalLines;
				oei->CurLine = ei.CurLine;
				oei->CurPos = ei.CurPos;
				oei->CurTabPos = ei.CurTabPos;
				oei->TopScreenLine = ei.TopScreenLine;
				oei->LeftPos = ei.LeftPos;
				oei->Overtype = ei.Overtype;
				oei->BlockType = ei.BlockType;
				oei->BlockStartLine = ei.BlockStartLine;
				oei->AnsiMode = 0;
				oei->TableNum = GetEditorCodePageFavA();
				oei->Options = ei.Options;
				oei->TabSize = ei.TabSize;
				oei->BookMarkCount = ei.BookMarkCount;
				oei->CurState = ei.CurState;
				return TRUE;
			}

			return FALSE;
		}
		case oldfar::ECTL_EDITORTOOEM:
		case oldfar::ECTL_OEMTOEDITOR: {
			if (!Param)
				return FALSE;

			oldfar::EditorConvertText *ect = (oldfar::EditorConvertText *)Param;
			UINT CodePage = GetEditorCodePageA();
			MultiByteRecode(Command == oldfar::ECTL_OEMTOEDITOR ? CP_UTF8 : CodePage,
					Command == oldfar::ECTL_OEMTOEDITOR ? CodePage : CP_OEMCP, ect->Text, ect->TextLength);
			return TRUE;
		}
		case oldfar::ECTL_SAVEFILE: {
			EditorSaveFile newsf{};

			if (Param) {
				oldfar::EditorSaveFile *oldsf = (oldfar::EditorSaveFile *)Param;
				newsf.FileName = AnsiToUnicode(oldsf->FileName);
				newsf.FileEOL = (oldsf->FileEOL) ? AnsiToUnicode(oldsf->FileEOL) : nullptr;
			}

			int ret = FarEditorControl(ECTL_SAVEFILE, Param ? (void *)&newsf : 0);

			if (newsf.FileName)
				free((void *)newsf.FileName);

			if (newsf.FileEOL)
				free((void *)newsf.FileEOL);

			return ret;
		}
		case oldfar::ECTL_PROCESSINPUT:		// BUGBUG?
		{
			if (Param) {
				INPUT_RECORD *pIR = (INPUT_RECORD *)Param;

				switch (pIR->EventType) {
					case KEY_EVENT:
					case FARMACRO_KEY_EVENT: {
						wchar_t res;
						ErrnoSaver ErSr;
						WINPORT(MultiByteToWideChar)
						(CP_OEMCP, 0, &pIR->Event.KeyEvent.uChar.AsciiChar, 1, &res, 1);
						pIR->Event.KeyEvent.uChar.UnicodeChar = res;
					}
				}
			}

			return FarEditorControl(ECTL_PROCESSINPUT, Param);
		}
		case oldfar::ECTL_PROCESSKEY: {
			return FarEditorControl(ECTL_PROCESSKEY, (void *)(DWORD_PTR)OldKeyToKey((DWORD)(DWORD_PTR)Param));
		}
		case oldfar::ECTL_READINPUT:	// BUGBUG?
		{
			int ret = FarEditorControl(ECTL_READINPUT, Param);

			if (Param) {
				INPUT_RECORD *pIR = (INPUT_RECORD *)Param;

				switch (pIR->EventType) {
					case KEY_EVENT:
					case FARMACRO_KEY_EVENT: {
						char res;
						ErrnoSaver ErSr;
						WINPORT(WideCharToMultiByte)
						(CP_OEMCP, 0, &pIR->Event.KeyEvent.uChar.UnicodeChar, 1, &res, 1, nullptr, nullptr);
						pIR->Event.KeyEvent.uChar.UnicodeChar = res;
					}
				}
			}

			return ret;
		}
		case oldfar::ECTL_SETKEYBAR: {
			switch ((LONG_PTR)Param) {
				case 0:
				case -1:
					return FarEditorControl(ECTL_SETKEYBAR, Param);
				default:
					oldfar::KeyBarTitles *oldkbt = (oldfar::KeyBarTitles *)Param;
					KeyBarTitles newkbt;
					ConvertKeyBarTitlesA(oldkbt, &newkbt);
					int ret = FarEditorControl(ECTL_SETKEYBAR, &newkbt);
					FreeUnicodeKeyBarTitles(&newkbt);
					return ret;
			}
		}
		case oldfar::ECTL_SETPARAM: {
			EditorSetParameter newsp{};

			if (Param) {
				oldfar::EditorSetParameter *oldsp = (oldfar::EditorSetParameter *)Param;
				newsp.Param.iParam = oldsp->Param.iParam;

				switch (oldsp->Type) {
					case oldfar::ESPT_AUTOINDENT:
						newsp.Type = ESPT_AUTOINDENT;
						break;
					case oldfar::ESPT_CHARCODEBASE:
						newsp.Type = ESPT_CHARCODEBASE;
						break;
					case oldfar::ESPT_CURSORBEYONDEOL:
						newsp.Type = ESPT_CURSORBEYONDEOL;
						break;
					case oldfar::ESPT_LOCKMODE:
						newsp.Type = ESPT_LOCKMODE;
						break;
					case oldfar::ESPT_SAVEFILEPOSITION:
						newsp.Type = ESPT_SAVEFILEPOSITION;
						break;
					case oldfar::ESPT_TABSIZE:
						newsp.Type = ESPT_TABSIZE;
						break;
					case oldfar::ESPT_CHARTABLE:	// BUGBUG, недоделано в фаре
					{
						if (!oldsp->Param.iParam)
							return FALSE;

						newsp.Type = ESPT_CODEPAGE;

						switch (oldsp->Param.iParam) {
							case 1:
								newsp.Param.iParam = WINPORT(GetOEMCP)();
								break;
							case 2:
								newsp.Param.iParam = WINPORT(GetACP)();
								break;
							default:
								newsp.Param.iParam = oldsp->Param.iParam;

								if (newsp.Param.iParam > 0)
									newsp.Param.iParam-= 3;

								newsp.Param.iParam = ConvertCharTableToCodePage(newsp.Param.iParam);

								if ((UINT)newsp.Param.iParam == CP_AUTODETECT)
									return FALSE;

								break;
						}

						break;
					}
					case oldfar::ESPT_EXPANDTABS: {
						newsp.Type = ESPT_EXPANDTABS;

						switch (oldsp->Param.iParam) {
							case oldfar::EXPAND_NOTABS:
								newsp.Param.iParam = EXPAND_NOTABS;
								break;
							case oldfar::EXPAND_ALLTABS:
								newsp.Param.iParam = EXPAND_ALLTABS;
								break;
							case oldfar::EXPAND_NEWTABS:
								newsp.Param.iParam = EXPAND_NEWTABS;
								break;
							default:
								return FALSE;
						}

						break;
					}
					case oldfar::ESPT_SETWORDDIV: {
						newsp.Type = ESPT_SETWORDDIV;
						newsp.Param.wszParam =
								(oldsp->Param.cParam) ? AnsiToUnicode(oldsp->Param.cParam) : nullptr;
						int ret = FarEditorControl(ECTL_SETPARAM, (void *)&newsp);

						if (newsp.Param.wszParam)
							free(newsp.Param.wszParam);

						return ret;
					}
					case oldfar::ESPT_GETWORDDIV: {
						if (!oldsp->Param.cParam)
							return FALSE;

						*oldsp->Param.cParam = 0;
						newsp.Type = ESPT_GETWORDDIV;
						newsp.Param.wszParam = nullptr;
						newsp.Size = 0;
						newsp.Size = FarEditorControl(ECTL_SETPARAM, (void *)&newsp);
						newsp.Param.wszParam = (wchar_t *)malloc(newsp.Size * sizeof(wchar_t));

						if (newsp.Param.wszParam) {
							int ret = FarEditorControl(ECTL_SETPARAM, (void *)&newsp);
							char *olddiv = UnicodeToAnsi(newsp.Param.wszParam);

							if (olddiv) {
								far_strncpy(oldsp->Param.cParam, olddiv, 0x100);
								free(olddiv);
							}

							free(newsp.Param.wszParam);
							return ret;
						}

						return FALSE;
					}
					default:
						return FALSE;
				}
			}

			return FarEditorControl(ECTL_SETPARAM, Param ? (void *)&newsp : 0);
		}
		case oldfar::ECTL_SETSTRING: {
			EditorSetString newss = {0, 0, 0, 0};

			if (Param) {
				oldfar::EditorSetString *oldss = (oldfar::EditorSetString *)Param;
				newss.StringNumber = oldss->StringNumber;
				UINT CodePage = GetEditorCodePageA();
				newss.StringText = (oldss->StringText)
						? AnsiToUnicodeBin(oldss->StringText, oldss->StringLength, CodePage)
						: nullptr;
				newss.StringEOL = (oldss->StringEOL) ? AnsiToUnicode(oldss->StringEOL, CodePage) : nullptr;
				newss.StringLength = oldss->StringLength;
			}

			int ret = FarEditorControl(ECTL_SETSTRING, Param ? (void *)&newss : 0);

			if (newss.StringText)
				free((void *)newss.StringText);

			if (newss.StringEOL)
				free((void *)newss.StringEOL);

			return ret;
		}
		case oldfar::ECTL_SETTITLE: {
			wchar_t *newtit = nullptr;

			if (Param) {
				newtit = AnsiToUnicode((char *)Param);
			}

			int ret = FarEditorControl(ECTL_SETTITLE, (void *)newtit);

			if (newtit)
				free(newtit);

			return ret;
		}
		default:
			return FALSE;
	}

	return FarEditorControl(Command, Param);
}

int WINAPI FarViewerControlA(int Command, void *Param)
{
	static char *filename = nullptr;

	switch (Command) {
		case oldfar::VCTL_GETINFO: {
			if (!Param)
				return FALSE;

			oldfar::ViewerInfo *viA = (oldfar::ViewerInfo *)Param;

			if (!viA->StructSize)
				return FALSE;

			ViewerInfo viW;
			viW.StructSize = sizeof(ViewerInfo);	// BUGBUG?

			if (FarViewerControl(VCTL_GETINFO, &viW) == FALSE)
				return FALSE;

			viA->ViewerID = viW.ViewerID;

			if (filename)
				free(filename);

			filename = UnicodeToAnsi(viW.FileName);
			viA->FileName = filename;
			viA->FileSize = viW.FileSize;
			viA->FilePos = viW.FilePos;
			viA->WindowSizeX = viW.WindowSizeX;
			viA->WindowSizeY = viW.WindowSizeY;
			viA->Options = 0;

			if (viW.Options & VOPT_SAVEFILEPOSITION)
				viA->Options|= oldfar::VOPT_SAVEFILEPOSITION;

			if (viW.Options & VOPT_AUTODETECTCODEPAGE)
				viA->Options|= oldfar::VOPT_AUTODETECTTABLE;

			viA->TabSize = viW.TabSize;
			viA->CurMode.UseDecodeTable = 0;
			viA->CurMode.TableNum = 0;
			viA->CurMode.AnsiMode = viW.CurMode.CodePage == WINPORT(GetACP)();
			viA->CurMode.Unicode = IsFullWideCodePage(viW.CurMode.CodePage);
			viA->CurMode.Wrap = viW.CurMode.Wrap;
			viA->CurMode.WordWrap = viW.CurMode.WordWrap;
			viA->CurMode.Processed = viW.CurMode.Processed;
			viA->CurMode.Hex = viW.CurMode.Hex;
			viA->LeftPos = (int)viW.LeftPos;
			viA->Reserved3 = 0;
			break;
		}
		case oldfar::VCTL_QUIT:
			return FarViewerControl(VCTL_QUIT, nullptr);
		case oldfar::VCTL_REDRAW:
			return FarViewerControl(VCTL_REDRAW, nullptr);
		case oldfar::VCTL_SETKEYBAR: {
			switch ((LONG_PTR)Param) {
				case 0:
				case -1:
					return FarViewerControl(VCTL_SETKEYBAR, Param);
				default:
					oldfar::KeyBarTitles *kbtA = (oldfar::KeyBarTitles *)Param;
					KeyBarTitles kbt;
					ConvertKeyBarTitlesA(kbtA, &kbt);
					int ret = FarViewerControl(VCTL_SETKEYBAR, &kbt);
					FreeUnicodeKeyBarTitles(&kbt);
					return ret;
			}
		}
		case oldfar::VCTL_SETPOSITION: {
			if (!Param)
				return FALSE;

			oldfar::ViewerSetPosition *vspA = (oldfar::ViewerSetPosition *)Param;
			ViewerSetPosition vsp;
			vsp.Flags = 0;

			if (vspA->Flags & oldfar::VSP_NOREDRAW)
				vsp.Flags|= VSP_NOREDRAW;

			if (vspA->Flags & oldfar::VSP_PERCENT)
				vsp.Flags|= VSP_PERCENT;

			if (vspA->Flags & oldfar::VSP_RELATIVE)
				vsp.Flags|= VSP_RELATIVE;

			if (vspA->Flags & oldfar::VSP_NORETNEWPOS)
				vsp.Flags|= VSP_NORETNEWPOS;

			vsp.StartPos = vspA->StartPos;
			vsp.LeftPos = vspA->LeftPos;
			int ret = FarViewerControl(VCTL_SETPOSITION, &vsp);
			vspA->StartPos = vsp.StartPos;
			return ret;
		}
		case oldfar::VCTL_SELECT: {
			if (!Param)
				return FarViewerControl(VCTL_SELECT, nullptr);

			oldfar::ViewerSelect *vsA = (oldfar::ViewerSelect *)Param;
			ViewerSelect vs = {vsA->BlockStartPos, vsA->BlockLen};
			return FarViewerControl(VCTL_SELECT, &vs);
		}
		case oldfar::VCTL_SETMODE: {
			if (!Param)
				return FALSE;

			oldfar::ViewerSetMode *vsmA = (oldfar::ViewerSetMode *)Param;
			ViewerSetMode vsm;
			vsm.Type = 0;

			switch (vsmA->Type) {
				case oldfar::VSMT_HEX:
					vsm.Type = VSMT_HEX;
					break;
				case oldfar::VSMT_WRAP:
					vsm.Type = VSMT_WRAP;
					break;
				case oldfar::VSMT_WORDWRAP:
					vsm.Type = VSMT_WORDWRAP;
					break;
			}

			vsm.Param.iParam = vsmA->Param.iParam;
			vsm.Flags = 0;

			if (vsmA->Flags & oldfar::VSMFL_REDRAW)
				vsm.Flags|= VSMFL_REDRAW;

			vsm.Reserved = 0;
			return FarViewerControl(VCTL_SETMODE, &vsm);
		}
	}

	return TRUE;
}

int WINAPI FarCharTableA(int Command, char *Buffer, int BufferSize)
{
	if (Command != oldfar::FCT_DETECT) {
		if (BufferSize != (int)sizeof(oldfar::CharTableSet))
			return -1;

		oldfar::CharTableSet *TableSet = (oldfar::CharTableSet *)Buffer;
		// Preset. Also if Command != FCT_DETECT and failed, buffer must be filled by OEM data.
		strcpy(TableSet->TableName, "<failed>");

		for (unsigned int i = 0; i < 256; ++i) {
			TableSet->EncodeTable[i] = TableSet->DecodeTable[i] = i;
			TableSet->UpperTable[i] = toupper(i);
			TableSet->LowerTable[i] = tolower(i);
		}

		FormatString sTableName;
		UINT nCP = ConvertCharTableToCodePage(Command);

		if (nCP == CP_AUTODETECT)
			return -1;

		// CPINFOEX cpiex;
		// if (!GetCPInfoEx(nCP, 0, &cpiex)) {
		CPINFO cpi;

		if (!WINPORT(GetCPInfo)(nCP, &cpi))
			return -1;

		// cpiex.MaxCharSize = cpi.MaxCharSize;
		// cpiex.CodePageName[0] = L'\0';
		//}

		// if (cpiex.MaxCharSize != 1)
		if (cpi.MaxCharSize != 1)
			return -1;

		ErrnoSaver ErSr;
		const wchar_t *codePageName = L"";	// FormatCodePageName(nCP, cpiex.CodePageName, sizeof(cpiex.CodePageName)/sizeof(wchar_t));
		sTableName << fmt::Expand(5) << nCP << BoxSymbols[BS_V1] << L" " << codePageName;
		sTableName.strValue().GetCharString(TableSet->TableName, sizeof(TableSet->TableName) - 1, CP_OEMCP);
		wchar_t *us = AnsiToUnicodeBin((char *)TableSet->DecodeTable, sizeof(TableSet->DecodeTable), nCP);
		WINPORT(CharLowerBuff)(us, sizeof(TableSet->DecodeTable));
		WINPORT(WideCharToMultiByte)
		(nCP, 0, us, sizeof(TableSet->DecodeTable), (char *)TableSet->LowerTable,
				sizeof(TableSet->DecodeTable), nullptr, nullptr);
		WINPORT(CharUpperBuff)(us, sizeof(TableSet->DecodeTable));
		WINPORT(WideCharToMultiByte)
		(nCP, 0, us, sizeof(TableSet->DecodeTable), (char *)TableSet->UpperTable,
				sizeof(TableSet->DecodeTable), nullptr, nullptr);
		free(us);
		MultiByteRecode(nCP, CP_OEMCP, (char *)TableSet->DecodeTable, sizeof(TableSet->DecodeTable));
		MultiByteRecode(CP_OEMCP, nCP, (char *)TableSet->EncodeTable, sizeof(TableSet->EncodeTable));
		return Command;
	}

	return -1;
}

char *WINAPI XlatA(char *Line,						// исходная строка
		int StartPos,								// начало переконвертирования
		int EndPos,									// конец переконвертирования
		const oldfar::CharTableSet *TableSet,		// перекодировочная таблица (может быть nullptr)
		DWORD Flags)								// флаги (см. enum XLATMODE)
{
	FARString strLine(Line);
	DWORD NewFlags = 0;

	if (Flags & oldfar::XLAT_SWITCHKEYBLAYOUT)
		NewFlags|= XLAT_SWITCHKEYBLAYOUT;

	if (Flags & oldfar::XLAT_SWITCHKEYBBEEP)
		NewFlags|= XLAT_SWITCHKEYBBEEP;

	if (Flags & oldfar::XLAT_USEKEYBLAYOUTNAME)
		NewFlags|= XLAT_USEKEYBLAYOUTNAME;

	if (Flags & oldfar::XLAT_CONVERTALLCMDLINE)
		NewFlags|= XLAT_CONVERTALLCMDLINE;

	Xlat(strLine.GetBuffer(), StartPos, EndPos, NewFlags);
	strLine.ReleaseBuffer();
	strLine.GetCharString(Line, strLine.GetLength() + 1);
	return Line;
}

int WINAPI GetFileOwnerA(const char *Computer, const char *Name, char *Owner)
{
	FARString strComputer(Computer), strName(Name), strOwner;
	int Ret = GetFileOwner(strComputer, strName, strOwner);
	strOwner.GetCharString(Owner, oldfar::NM);
	return Ret;
}
