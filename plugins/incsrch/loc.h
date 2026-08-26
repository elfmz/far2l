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
#pragma once

#ifdef __cplusplus
extern "C" {
#endif

extern char aUpcaseTable[256];

extern void SetSubstringPatternL(const TCHAR *sPattern);
extern TCHAR *SubStringL(const TCHAR *text, size_t N);
extern void SetSubstringPatternR(const TCHAR *sPattern);
extern TCHAR *SubStringR(const TCHAR *text, size_t N);

extern void InitUpcaseTable(int nTableNum, BOOL bAnsiMode);
extern void UpperCase(TCHAR *sDest, const TCHAR *sSrc, size_t nLen);
#ifdef __WATCOMC__
#pragma aux UpperCase parm[edi][esi][ecx] modify exact[eax ebx ecx edi esi] =                                  \
				"jecxz   Exit"                                                                                 \
				"dec     esi"                                                                                  \
				"dec     edi"                                                                                  \
				"lea     ebx, aUpcaseTable"                                                                    \
				"Next:   mov     al,  [esi][ecx]"                                                              \
				"xlat"                                                                                         \
				"mov     [edi][ecx], al"                                                                       \
				"loop    Next"                                                                                 \
				"Exit:"
#endif

#ifdef __cplusplus
}
#endif
