#pragma once

/*
strmix.hpp

Куча разных вспомогательных функций по работе со строками
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
#include <string>
#include <WinCompat.h>
#include "FARString.hpp"

enum
{
	COLUMN_MARK           = 0x80000000,
	COLUMN_NAMEONLY       = 0x40000000,
	COLUMN_RIGHTALIGN     = 0x20000000,
	COLUMN_FORMATTED      = 0x10000000,
	COLUMN_COMMAS         = 0x08000000,
	COLUMN_THOUSAND       = 0x04000000,
	COLUMN_BRIEF          = 0x02000000,
	COLUMN_MONTH          = 0x01000000,
	COLUMN_FLOATSIZE      = 0x00800000,
	COLUMN_ECONOMIC       = 0x00400000,
	COLUMN_MINSIZEINDEX   = 0x00200000,
	COLUMN_SHOWBYTESINDEX = 0x00100000,
	COLUMN_FULLOWNER      = 0x00080000,

	// MINSIZEINDEX может быть только 0, 1, 2 или 3 (K,M,G,T)
	COLUMN_MINSIZEINDEX_MASK = 0x00000003,
};

std::string EscapeUnprintable(const std::string &str);
std::string UnescapeUnprintable(const std::string &str);

FARString &EscapeSpace(FARString &strStr);
FARString &UnEscapeSpace(FARString &strStr);
wchar_t *WINAPI InsertQuote(wchar_t *Str);
FARString &InsertQuote(FARString &strStr);
void WINAPI Unquote(FARString &strStr);
void WINAPI Unquote(wchar_t *Str);
wchar_t *WINAPI InsertRegexpQuote(wchar_t *Str);
FARString &InsertRegexpQuote(FARString &strStr);
void UnquoteExternal(FARString &strStr);
wchar_t *WINAPI RemoveLeadingSpaces(wchar_t *Str);
FARString &WINAPI RemoveLeadingSpaces(FARString &strStr);
wchar_t *WINAPI RemoveTrailingSpaces(wchar_t *Str);
FARString &WINAPI RemoveTrailingSpaces(FARString &strStr);
wchar_t *WINAPI RemoveExternalSpaces(wchar_t *Str);
FARString &WINAPI RemoveExternalSpaces(FARString &strStr);
FARString &WINAPI RemoveUnprintableCharacters(FARString &strStr);
wchar_t *WINAPI QuoteSpaceOnly(wchar_t *Str);
FARString &WINAPI QuoteSpaceOnly(FARString &strStr);

FARString &RemoveChar(FARString &strStr, wchar_t Target, BOOL Dup = TRUE);
wchar_t *InsertString(wchar_t *Str, int Pos, const wchar_t *InsStr, int InsSize = 0);
int ReplaceStrings(FARString &strStr, const wchar_t *FindStr, const wchar_t *ReplStr, int Count = -1,
		BOOL IgnoreCase = FALSE);
int ReplaceChars(FARString &strStr, wchar_t FindCh, wchar_t ReplCh);
int ReplaceTabsBySpaces(FARString &strStr, size_t TabSize = 1);

const wchar_t *GetCommaWord(const wchar_t *Src, FARString &strWord, wchar_t Separator = L',');

FARString &WINAPI
FarFormatText(const wchar_t *SrcText, int Width, FARString &strDestText, const wchar_t *Break, DWORD Flags);

void PrepareUnitStr();
FARString &WINAPI
FileSizeToStr(FARString &strDestStr, uint64_t Size, int Width = -1, int ViewFlags = COLUMN_COMMAS);
bool CheckFileSizeStringFormat(const wchar_t *FileSizeStr);
uint64_t ConvertFileSizeString(const wchar_t *FileSizeStr);
FARString &FormatNumber(const wchar_t *Src, FARString &strDest, int NumDigits = 0);
FARString &InsertCommas(uint64_t li, FARString &strDest);

inline bool IsWordDiv(const wchar_t *WordDiv, wchar_t Chr) noexcept
{
	return wcschr(WordDiv, Chr) != nullptr;
}

inline bool IsWordDivSTNR(const wchar_t *WordDiv, wchar_t Chr) noexcept
{
	return wcschr(WordDiv, Chr) != nullptr || IsSpace(Chr) || IsEol(Chr);
}

// WordDiv  - набор разделителей слова в кодировке OEM
// возвращает указатель на начало слова
const wchar_t *
CalcWordFromString(const wchar_t *Str, int CurPos, int *Start, int *End, const wchar_t *WordDiv);

wchar_t *WINAPI TruncStr(wchar_t *Str, int MaxLength);
FARString &WINAPI TruncStr(FARString &strStr, int MaxLength);
wchar_t *WINAPI TruncStrFromEnd(wchar_t *Str, int MaxLength);
FARString &WINAPI TruncStrFromEnd(FARString &strStr, int MaxLength);
wchar_t *TruncStrFromCenter(wchar_t *Str, int MaxLength);
FARString &TruncStrFromCenter(FARString &strStr, int MaxLength);
wchar_t *WINAPI TruncPathStr(wchar_t *Str, int MaxLength);
FARString &WINAPI TruncPathStr(FARString &strStr, int MaxLength);

BOOL IsCaseMixed(const FARString &strStr);
BOOL IsCaseLower(const FARString &strStr);

FARString &CenterStr(const wchar_t *Src, FARString &strDest, int Length);
FARString FixedSizeStr(FARString str, size_t Cells, bool RAlign, bool TruncateCenter);

void Transform(FARString &strBuffer, const wchar_t *ConvStr, wchar_t TransformType);

wchar_t GetDecimalSeparator();
inline const wchar_t GetDecimalSeparatorDefault() { return L'.'; };
inline const wchar_t* GetDecimalSeparatorDefaultStr() { return L"."; };

FARString
ReplaceBrackets(const wchar_t *SearchStr, const FARString &ReplaceStr, RegExpMatch *Match, int Count);

bool SearchString(const wchar_t *Source, int StrSize, const FARString &Str, FARString &ReplaceStr,
		int &CurPos, int Position, int Case, int WholeWords, int Reverse, int Regexp, int *SearchLength,
		const wchar_t *WordDiv = nullptr);
