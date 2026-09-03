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

#if !defined(WINPORT_DIRECT)

#if !defined(__WATCOMC__) && (defined(__386__) || defined(_WIN32) || defined(__NT__))
void memmovel(void *dest, void *src, size_t count)
{
	_asm {
		mov edi, dest
		mov esi, src
		mov ecx, count
		cld
		rep movsb
	}
}
#endif

#if !defined(__WATCOMC__) && (defined(__386__) || defined(_WIN32) || defined(__NT__))
void zeromem(void *pMem, size_t nLength)
{
	_asm {
		mov edi, pMem
		mov ecx, nLength
		cld
		xor eax, eax
		rep stosb
	}
}
#endif

#ifndef __WATCOMC__
size_t utoar(unsigned int nNum, char *sBuff, size_t nBuffLen)
// returning index to first byte
{
	_asm {
		mov     eax, nNum
		mov     edi, sBuff
		mov     ecx, nBuffLen
		dec     edi
		mov     ebx, 10

		mov     byte ptr [edi][ecx], 0  ; terminating null
		dec     ecx

Next:	xor     edx, edx
		div     ebx
		add     edx, 48                 ; to ascii
		mov     byte ptr [edi][ecx], dl
		dec     ecx
		jz      Exit
		test    eax, eax
		jz      Exit
		jmp     short Next
Exit:	mov     eax, ecx
	}
}
#endif

#ifndef __WATCOMC__
size_t sstrnlen(const char *sString, size_t count)
{
	_asm {
		mov    edi, sString
		mov    ecx, count
		cld
		mov    edx, ecx
		xor    eax, eax
		repne  scasb
		jnz    Len
		inc    ecx
Len:	sub    edx, ecx
		mov    eax, edx
	}
}
#endif

BOOL WaitInput(BOOL Infinite)
{
	return WaitForSingleObject(hInputHandle, Infinite ? INFINITE : 0) == WAIT_OBJECT_0;
}
#else	// WINPORT_DIRECT
size_t utoar(unsigned int nNum, TCHAR *sBuff, size_t nBuffLen)
// returning index to first byte
{
	apiSnprintf(sBuff, nBuffLen, _T("%*ud"), nBuffLen - 1, nNum);
	sBuff[nBuffLen - 1] = 0;
	size_t pos = 0;
	for (; pos < nBuffLen; ++pos)
		if (sBuff[pos] != _T(' '))
			break;
	return pos;
}

size_t lstrnlen(const TCHAR *str, size_t count)
{
	size_t i = 0;
	for (i = 0; i < count; i++)
		if (!str[i])
			break;
	return i;
}

BOOL WaitInput(BOOL Infinite)
{
	INPUT_RECORD rec;
	DWORD ReadCount;

	do {
		WINPORT(PeekConsoleInput)(NULL, &rec, 1, &ReadCount);
		if (Infinite)
			WINPORT(Sleep(20));
	} while (Infinite && ReadCount == 0);

	return ReadCount != 0;
}
#endif	// WINPORT_DIRECT
