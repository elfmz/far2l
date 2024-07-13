/*
processname.cpp

Обработать имя файла: сравнить с маской, масками, сгенерировать по маске
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

#include "processname.hpp"
#include "strmix.hpp"
#include "pathmix.hpp"
#include "CFileMask.hpp"

// обработать имя файла: сравнить с маской, масками, сгенерировать по маске
int WINAPI ProcessName(const wchar_t *param1, wchar_t *param2, DWORD size, DWORD flags)
{
	bool skippath = (flags & PN_SKIPPATH) != 0;

	flags&= ~PN_SKIPPATH;

	if (flags == PN_CMPNAME)
		return CmpName(param1, param2, skippath);

	if (flags == PN_CMPNAMELIST) {
		CFileMask Masks;
		return Masks.Set(param1, FMF_SILENT) && Masks.Compare(skippath ? PointToName(param2) : param2);
	}

	if (flags & PN_GENERATENAME) {
		FARString strResult = param2;
		int nResult = ConvertWildcards(param1, strResult, (flags & 0xFFFF) | (skippath ? PN_SKIPPATH : 0));
		far_wcsncpy(param2, strResult, size);
		return nResult;
	}

	return FALSE;
}

/*
	$ 09.10.2000 IS
	Генерация нового имени по маске
	(взял из ShellCopy::ShellCopyConvertWildcards)
*/
// На основе имени файла (Src) и маски (Dest) генерируем новое имя
// SelectedFolderNameLength - длина каталога. Например, есть
// каталог dir1, а в нем файл file1. Нужно сгенерировать имя по маске для dir1.
// Параметры могут быть следующими: Src="dir1", SelectedFolderNameLength=0
// или Src="dir1/file1", а SelectedFolderNameLength=4 (длина "dir1")
int ConvertWildcards(const wchar_t *SrcName, FARString &strDest, int SelectedFolderNameLength)
{
	FARString strPartAfterFolderName;
	FARString strSrc = SrcName;
	wchar_t *DestName = strDest.GetBuffer(strDest.GetLength() + strSrc.GetLength() + 1);	//???
	wchar_t *DestNamePtr = (wchar_t *)PointToName(DestName);
	FARString strWildName = DestNamePtr;

	if (!wcschr(strWildName, L'*') && !wcschr(strWildName, L'?')) {
		// strDest.ReleaseBuffer (); не надо так как строка не поменялась
		return FALSE;
	}

	if (SelectedFolderNameLength) {
		strPartAfterFolderName = ((const wchar_t *)strSrc + SelectedFolderNameLength);
		strSrc.Truncate(SelectedFolderNameLength);
	}

	const wchar_t *Src = strSrc;

	const wchar_t *SrcNamePtr = PointToName(Src);

	int BeforeNameLength = DestNamePtr == DestName ? (int)(SrcNamePtr - Src) : 0;

	wchar_t *PartBeforeName = (wchar_t *)malloc((BeforeNameLength + 1) * sizeof(wchar_t));

	far_wcsncpy(PartBeforeName, Src, BeforeNameLength + 1);

	const wchar_t *SrcNameDot = wcsrchr(SrcNamePtr, L'.');

	const wchar_t *CurWildPtr = strWildName;

	while (*CurWildPtr) {
		switch (*CurWildPtr) {
			case L'?':
				CurWildPtr++;

				if (*SrcNamePtr)
					*(DestNamePtr++) = *(SrcNamePtr++);

				break;
			case L'*':
				CurWildPtr++;

				while (*SrcNamePtr) {
					if (*CurWildPtr == L'.' && SrcNameDot && !wcschr(CurWildPtr + 1, L'.')) {
						if (SrcNamePtr == SrcNameDot)
							break;
					} else if (*SrcNamePtr == *CurWildPtr) {
						break;
					}

					*(DestNamePtr++) = *(SrcNamePtr++);
				}

				break;
			case L'.':
				CurWildPtr++;
				*(DestNamePtr++) = L'.';

				if (FindAnyOfChars(CurWildPtr, "*?"))
					while (*SrcNamePtr)
						if (*(SrcNamePtr++) == L'.')
							break;

				break;
			default:
				*(DestNamePtr++) = *(CurWildPtr++);

				if (*SrcNamePtr && *SrcNamePtr != L'.')
					SrcNamePtr++;

				break;
		}
	}

	*DestNamePtr = 0;

	if (DestNamePtr != DestName && *(DestNamePtr - 1) == L'.')
		*(DestNamePtr - 1) = 0;

	strDest.ReleaseBuffer();

	if (*PartBeforeName)
		strDest = PartBeforeName + strDest;

	if (SelectedFolderNameLength)
		strDest+= strPartAfterFolderName;	// BUGBUG???, was src in 1.7x

	free(PartBeforeName);
	return TRUE;
}

// IS: это реальное тело функции сравнения с маской, но использовать
// IS: "снаружи" нужно не эту функцию, а CmpName (ее тело расположено
// IS: после CmpName_Body)
static bool CmpName_Body(const wchar_t *pattern, const wchar_t *str, bool ignorecase)
{
	for (;; ++str) {
		/*
			$ 01.05.2001 DJ
			используем инлайновые версии
		*/
		wchar_t rangec;
		bool match;
		wchar_t stringc = *str;
		wchar_t patternc = *pattern++;

		switch (patternc) {
			case 0:
				return !stringc;

			case L'?':

				if (!stringc)
					return false;

				break;

			case L'*':

				if (!*pattern)
					return true;

				if (!FindAnyOfChars(pattern, "*?[")) {
					const size_t pattern_len = wcslen(pattern);
					const size_t str_len = wcslen(str);

					bool result = (str_len >= pattern_len);
					if (result) {
						if (ignorecase) {
							for (size_t i = 0; i < pattern_len; ++i) {
								if (Upper(pattern[i]) != Upper(str[str_len - pattern_len + i])) {
									result = false;
									break;
								}
							}
						} else {
							result = (wmemcmp(pattern, str + str_len - pattern_len, pattern_len) == 0);
						}
					}
					return result;
				}

				do {
					if (CmpName_Body(pattern, str, ignorecase))
						return true;
				} while (*str++);

				return false;

			case L'[':

				if (!wcschr(pattern, L']')) {
					if (patternc != stringc)
						return false;

					break;
				}

				if (*pattern && *(pattern + 1) == L']') {
					if (ignorecase) {
						if (Upper(*pattern) != Upper(*str))
							return false;
					} else {
						if (*pattern != *str)
							return false;
					}

					pattern+= 2;
					break;
				}

				match = false;

				while ((rangec = *pattern++) != 0) {
					if (rangec == L']') {
						if (match)
							break;
						else
							return false;
					}

					if (match)
						continue;

					if (rangec == L'-' && *(pattern - 2) != L'[' && *pattern != L']') {
						match = ignorecase
							? (Upper(stringc) <= Upper(*pattern) && Upper(*(pattern - 2)) <= Upper(stringc))
							: (stringc <= *pattern && *(pattern - 2) <= stringc);
						pattern++;
					} else
						match = ignorecase
							? (Upper(stringc) == Upper(rangec))
							: (stringc == rangec);
				}

				if (!rangec)
					return false;

				break;

			default:
				if (ignorecase) {
					if (Upper(patternc) != stringc && Lower(patternc) != stringc)
						return false;
				} else {
					if (patternc != stringc)
						return false;
				}
				break;
		}
	}
}

// IS: функция для внешнего мира, использовать ее
bool CmpName(const wchar_t *pattern, const wchar_t *str, bool skippath, bool ignorecase)
{
	if (!pattern || !str)
		return false;

	if (skippath)
		str = PointToName(str);

	return CmpName_Body(pattern, str, ignorecase);
}
