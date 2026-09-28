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

#if defined(__386__) || defined(_WIN32) || defined(__NT__)
extern void memmovel(void *dest, const void *src, size_t count);
#if defined(__WATCOMC__)

#pragma aux memmovel parm[edi][esi][ecx] modify exact[ecx edi esi] =	/*"cld"*/                              \
		"rep movsb";
#endif																	/* __WATCOMC__ */
#else																	/* !__386__    */
#define memmovel memmove
#endif

#if defined(__386__) || defined(_WIN32) || defined(__NT__)
extern void zeromem(void *ptr, size_t nLen);
#if defined(__WATCOMC__)
#pragma aux zeromem parm[edi][ecx] modify exact[eax ecx edi] =	/*"cld"*/                                      \
		"xor eax, eax"                                                                                         \
		"rep stosb";
#endif	/* __WATCOMC__ */
#else	/* !__386__    */
#define zeromem(ptr, nLen) memset((ptr), 0, (nLen))
#endif

#if defined(_MSC_VER)
static __inline void setmem(void *pMem, int b, size_t nLength)
{
	_asm {
		mov edi, pMem
		mov ecx, nLength
		mov eax, b
		cld
		rep stosb
	}
}
#define memcpy(d, s, l) memmovel((d), (s), (l))
#else
#if defined(__386__) || defined(_WIN32) || defined(__NT__)
extern void setmem(void *ptr, int byte, size_t nLen);
#if defined(__WATCOMC__)
#pragma aux setmem parm[edi][eax][ecx] modify exact[eax ecx edi] = "rep stosb";
#endif	/* __WATCOMC__ */
#else	/* !__386__    */
#define setmem(ptr, byte, nLen) memset((ptr), byte, (nLen))
#endif
#endif

extern size_t utoar(unsigned int nNum, TCHAR *sBuff, size_t nBuffLen);
// returning index to first byte
#ifdef __WATCOMC__
#pragma aux utoar parm[eax][edi][ecx] value[ecx] modify exact[eax ebx ecx edx edi] =                           \
				"dec     edi"                                                                                  \
				"mov     ebx, 10"                                                                              \
				"mov     byte ptr [edi][ecx], 0"                                                               \
				"dec     ecx"                                                                                  \
				"Next:   xor     edx, edx"                                                                     \
				"div     ebx"                                                                                  \
				"add     edx, 48"                                                                              \
				"mov     byte ptr [edi][ecx], dl"                                                              \
				"dec     ecx"                                                                                  \
				"jz      Exit"                                                                                 \
				"test    eax, eax"                                                                             \
				"jz      Exit"                                                                                 \
				"jmp     short Next"                                                                           \
				"Exit:";
#endif

#if defined(__386__) || defined(_WIN32) || defined(__NT__)
extern size_t sstrnlen(const char *str, size_t count);
#if defined(__WATCOMC__)

#pragma aux sstrnlen parm[edi][ecx] modify exact[eax ecx edi] value[edx] =	/*"cld"*/                          \
		"mov    edx, ecx"                                                                                      \
		"xor    eax, eax"                                                                                      \
		"repne  scasb"                                                                                         \
		"jnz    Len"                                                                                           \
		"inc    ecx"                                                                                           \
		"Len:   sub    edx, ecx";
#endif	/* __WATCOMC__ */
#endif

#if defined(WINPORT_DIRECT)
extern size_t lstrnlen(const TCHAR *str, size_t count);
#endif
