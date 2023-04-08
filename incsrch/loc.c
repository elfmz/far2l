/*
FAR manager incremental search plugin, search as you type in editor.
Copyright (C) 1999-2019, Stanislav V. Mekhanoshin

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/
#include "incsrch.h"

char aUpcaseTable[256];

void InitUpcaseTable(int nTableNum, BOOL bAnsiMode)
{
#if !defined(WINPORT_DIRECT)
	if (nTableNum == -1) {	/* OEM text */
		int i;
		for (i = 0; i < sizeof(aUpcaseTable); i++)
			aUpcaseTable[i] = (char)i;
		if (!bAnsiMode)
			OemToCharBuff(aUpcaseTable, aUpcaseTable, sizeof(aUpcaseTable));
		CharUpperBuff(aUpcaseTable, sizeof(aUpcaseTable));
		if (!bAnsiMode)
			CharToOemBuff(aUpcaseTable, aUpcaseTable, sizeof(aUpcaseTable));
	} else {
		struct CharTableSet cts;

		apiCharTable(nTableNum, (char *)&cts, sizeof(struct CharTableSet));
		memcpy(aUpcaseTable, cts.UpperTable, sizeof(aUpcaseTable));
	}
#endif
}

#if !defined(__WATCOMC__) && !defined(WINPORT_DIRECT)
void UpperCase(char *sDest, const char *sSrc, size_t nLength)
{
	/* while(nLength--)sDest[nLength]=aUpcaseTable[sSrc[nLenhth]]; */
	_asm {
		mov     ecx, nLength
		jecxz   Exit
		mov     esi, sSrc
		mov     edi, sDest

		dec     esi
		dec     edi

		mov     ebx, offset aUpcaseTable
Next:	mov     al,  [esi][ecx]
		xlat
		mov     [edi][ecx], al
		loop    Next
Exit:
	}
}
#endif

#if defined(WINPORT_DIRECT)
void UpperCase(TCHAR *sDest, const TCHAR *sSrc, size_t nLen)
{
	if (sDest != sSrc)
		memcpy(sDest, sSrc, nLen * sizeof(sDest[0]));
	WINPORT(CharUpperBuff)(sDest, nLen);
}
#endif

#ifdef __WATCOMC__
extern char ToUpper(char cByte);
#pragma aux ToUpper parm[al] value[al] modify exact[eax ebx] nomemory =                                        \
				"mov     ebx, offset aUpcaseTable"                                                             \
				"xlat"
#elif !defined(WINPORT_DIRECT)
static __inline char ToUpper(char cByte)
{
	/* return aUpcaseTable[(unsigned char)cByte]; */
	_asm {
		movzx   eax, cByte
		mov     ebx, offset aUpcaseTable
		xlat
	}
}
#else
static __inline TCHAR ToUpper(TCHAR cByte)
{
#if defined(WINPORT_DIRECT)
	return Upper(cByte);
#else
	return aUpcaseTable[(unsigned char)cByte];
#endif
}
#endif

/* Pattern matching */

#if defined(WINPORT_DIRECT)
#define CHAR_MIN_T unsigned short
#define CHAR_MAX_V USHRT_MAX
#else
#define CHAR_MIN_T unsigned char
#define CHAR_MAX_V UCHAR_MAX
#endif

static const TCHAR *pattern;
static size_t skip[CHAR_MAX_V + 1];

static void PreInitSkipTable(const TCHAR *sPattern)
{
	size_t *s_p = skip + CHAR_MAX_V;

	pattern = sPattern;

	do {
		*s_p = nLen;
	} while (s_p-- > skip);
}

void SetSubstringPatternL(const TCHAR *sPattern)
{
	size_t i;

	PreInitSkipTable(sPattern);
	for (i = 0; i < (size_t)nLen; i++)
		skip[(CHAR_MIN_T)pattern[i]] = nLen - 1 - i;
}

void SetSubstringPatternR(const TCHAR *sPattern)
{
	int i;

	PreInitSkipTable(sPattern);
	for (i = nLen; (i--);)
		skip[(CHAR_MIN_T)pattern[i]] = i;
}

TCHAR *SubStringL(const TCHAR *text, size_t N)
{
	const TCHAR *p_p;
	const TCHAR *t_p;

	if (nLen == 0)
		return (TCHAR *)text;

	if ((size_t)nLen > N)	/* If pattern is longer than the text string. */
		return 0;

	p_p = pattern + nLen - 1;
	t_p = text + nLen - 1;

	for (;;) {
		TCHAR c;

		c = *t_p;
		if (!bCaseSensitive)
			c = ToUpper(c);
		if (c == *p_p) {
			if (p_p - pattern == 0)
				return (TCHAR *)t_p;

			t_p--;
			p_p--;
		} else {
			size_t step = nLen - (p_p - pattern);

			if (step < skip[(CHAR_MIN_T)c])
				step = skip[(CHAR_MIN_T)c];

			/* If we have run out of text to search in. */
			/* Need cast for case of large strings with 16 bit size_t... */
			if ((unsigned long)(t_p - text) + step >= (unsigned long)N)
				return 0;

			t_p+= step;

			p_p = pattern + nLen - 1;
		}
	}
}

TCHAR *SubStringR(const TCHAR *text, size_t N)
{
	const TCHAR *p_p;
	const TCHAR *t_p;

	if (nLen == 0)
		return (TCHAR *)text;

	if ((size_t)nLen > N)	/* If pattern is longer than the text string. */
		return 0;

	p_p = pattern;
	t_p = text + N - nLen;

	for (;;) {
		TCHAR c;

		c = *t_p;
		if (!bCaseSensitive)
			c = ToUpper(c);
		if (c == *p_p) {
			if ((int)(p_p - pattern) == (int)(nLen - 1))
				return (TCHAR *)t_p - nLen + 1;

			t_p++;
			p_p++;
		} else {
			size_t step = (p_p - pattern) + 1;

			if (step < skip[(CHAR_MIN_T)c])
				step = skip[(CHAR_MIN_T)c];

			/* If we have run out of text to search in. */
			if (t_p < text + step)
				return 0;

			t_p-= step;

			p_p = pattern;
		}
	}
}
