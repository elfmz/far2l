#pragma once

/*
codepage.cpp

Работа с кодовыми страницами
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

// Тип выбранной таблицы символов
enum CPSelectType
{
	// "Любимая" таблица символов
	CPST_FAVORITE = 1,
	// Таблица символов участвующая в поиске по всем таблицам символов
	CPST_FIND = 2
};

extern const wchar_t *FavoriteCodePagesKey;

inline bool IsUTF7(UINT CP) {return (CP==CP_UTF7); };
inline bool IsUTF8(UINT CP) {return (CP==CP_UTF8); };
inline bool IsUTF16(UINT CP) {return (CP==CP_UTF16LE || CP==CP_UTF16BE); };
inline bool IsUTF32(UINT CP) {return (CP==CP_UTF32LE || CP==CP_UTF32BE); };
inline bool IsFixedSingleCharCodePage(UINT CP) {return (!IsUTF7(CP) && !IsUTF8(CP) && !IsUTF16(CP) && !IsUTF32(CP)  ); };
inline bool IsStandard8BitCodePage(UINT CP) {return ((CP==WINPORT(GetACP)()) || CP==CP_KOI8R || (CP==WINPORT(GetOEMCP)())); };

#if (__WCHAR_MAX__ > 0xffff)
const int StandardCPCount = 3 /* DOS, ANSI, KOI */ + 2 /* UTF-32 LE, UTF-32 BE */ + 2 /* UTF-16 LE, UTF-16 BE */ + 2 /* UTF-7, UTF-8 */;
inline bool IsStandardCodePage(UINT CP) { return (CP==CP_UTF8 || CP==CP_UTF16LE || CP==CP_UTF16BE || CP==CP_UTF32LE || CP==CP_UTF32BE || IsStandard8BitCodePage(CP)); }
inline bool IsFullWideCodePage(UINT CP) { return (CP==CP_UTF32LE || CP==CP_UTF32BE); }
inline bool IsUnicodeOrUtfCodePage(UINT CP) { return (CP==CP_UTF8 || CP==CP_UTF16LE || CP==CP_UTF16BE || CP==CP_UTF32LE || CP==CP_UTF32BE); }
inline static wchar_t WideReverse(wchar_t w)
{
	union {
		uint8_t b[4];
		wchar_t w;
	} u;
	u.w = w;
	std::swap(u.b[0], u.b[3]);
	std::swap(u.b[1], u.b[2]);
	return u.w;
}
#else
const int StandardCPCount = 3 /* DOS, ANSI, KOI */ + 2 /* UTF-16 LE, UTF-16 BE */ + 2 /* UTF-7, UTF-8 */;
inline bool IsStandardCodePage(UINT CP) { return (CP==CP_UTF8 || CP==CP_UTF16LE || CP==CP_UTF16BE || IsStandard8BitCodePage(CP)); }
inline bool IsFullWideCodePage(UINT CP) { return (CP==CP_UTF16LE || CP==CP_UTF16BE); }
inline bool IsUnicodeOrUtfCodePage(UINT CP) { return (CP==CP_UTF8 || CP==CP_UTF16LE || CP==CP_UTF16BE); }
inline static wchar_t WideReverse(wchar_t w)
{
	union {
		uint8_t b[2];
		wchar_t w;
	} u;
	u.w = w;
	std::swap(u.b[0], u.b[1]);
	return u.w;
}

#endif

inline static void WideReverse(const wchar_t *src, wchar_t *dst, size_t l) {
	for ( ; l; --l, ++src, ++dst) {
		*dst = WideReverse(*src);
	}
}

bool IsCodePageSupported(UINT CodePage);

UINT SelectCodePage(UINT nCurrent, bool bShowUnicode, bool bShowUTF, bool bShowUTF7 = false);

UINT FillCodePagesList(HANDLE dialogHandle, UINT controlId, UINT codePage, bool allowAuto, bool allowAll);

wchar_t *FormatCodePageName(UINT CodePage, wchar_t *CodePageName, size_t Length);

//#define CP_DBG

#ifdef __cplusplus
extern "C" {
#endif

void cp_logger(int level, const char *cname, const char *msg, ...);

#ifdef __cplusplus
}
#endif

