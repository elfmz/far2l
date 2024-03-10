#include <set>
#include <string>
#include <locale> 
#include <algorithm>
#include <iostream>
#include <fstream>
#include <mutex>
#include <vector>

#include <cwctype>

#include "WinCompat.h"
#include "WinPort.h"
#include "wineguts.h"
#include "PathHelpers.h"

/*************************************************************************
*           LCMapStringEx   (KERNEL32.@)
*
* Map characters in a locale sensitive string.
*
* PARAMS
*  name     [I] Locale name for the conversion.
*  flags    [I] Flags controlling the mapping (LCMAP_ constants from "winnls.h")
*  src      [I] String to map
*  srclen   [I] Length of src in chars, or -1 if src is NUL terminated
*  dst      [O] Destination for mapped string
*  dstlen   [I] Length of dst in characters
*  version  [I] reserved, must be NULL
*  reserved [I] reserved, must be NULL
*  lparam   [I] reserved, must be 0
*
* RETURNS
*  Success: The length of the mapped string in dst, including the NUL terminator.
*  Failure: 0. Use GetLastError() to determine the cause.
*/
WINPORT_DECL(LCMapStringEx, INT, (LPCWSTR name, DWORD flags, LPCWSTR src, INT srclen, LPWSTR dst, INT dstlen,
	LPNLSVERSIONINFO version, LPVOID reserved, LPARAM lparam))
{
	LPWSTR dst_ptr;


	if (!src || !srclen || dstlen < 0)
	{
		WINPORT(SetLastError)(ERROR_INVALID_PARAMETER);
		return 0;
	}

	/* mutually exclusive flags */
	if ((flags & (LCMAP_LOWERCASE | LCMAP_UPPERCASE)) == (LCMAP_LOWERCASE | LCMAP_UPPERCASE) ||
		(flags & (LCMAP_HIRAGANA | LCMAP_KATAKANA)) == (LCMAP_HIRAGANA | LCMAP_KATAKANA) ||
		(flags & (LCMAP_HALFWIDTH | LCMAP_FULLWIDTH)) == (LCMAP_HALFWIDTH | LCMAP_FULLWIDTH) ||
		(flags & (LCMAP_TRADITIONAL_CHINESE | LCMAP_SIMPLIFIED_CHINESE)) == (LCMAP_TRADITIONAL_CHINESE | LCMAP_SIMPLIFIED_CHINESE))
	{
		WINPORT(SetLastError)(ERROR_INVALID_FLAGS);
		return 0;
	}

	if (!dstlen) dst = NULL;

	if (flags & LCMAP_SORTKEY)
	{
		INT ret;
		if (src == dst)
		{
			WINPORT(SetLastError)(ERROR_INVALID_FLAGS);
			return 0;
		}

		if (srclen < 0) srclen = wcslen(src);

		ret = wine_get_sortkey(flags, src, srclen, (char *)dst, dstlen);
		if (ret == 0)
			WINPORT(SetLastError)(ERROR_INSUFFICIENT_BUFFER);
		else
			ret++;
		return ret;
	}

	/* SORT_STRINGSORT must be used exclusively with LCMAP_SORTKEY */
	if (flags & SORT_STRINGSORT)
	{
		WINPORT(SetLastError)(ERROR_INVALID_FLAGS);
		return 0;
	}

	if (srclen < 0) srclen = wcslen(src) + 1;


	if (!dst) /* return required string length */
	{
		INT len;

		for (len = 0; srclen; src++, srclen--)
		{
			WCHAR wch = *src;
			/* tests show that win2k just ignores NORM_IGNORENONSPACE,
			* and skips white space and punctuation characters for
			* NORM_IGNORESYMBOLS.
			*/
			if (flags & NORM_IGNORESYMBOLS) {
				if (uint64_t(wch) <= 0xffff) {
					if (get_char_typeW(wch) & (C1_PUNCT | C1_SPACE))
						continue;
				} else if (iswspace(wch) || iswpunct(wch)) {
					continue;
				}
			}
			len++;
		}
		return len;
	}

	if (flags & LCMAP_UPPERCASE)
	{
		for (dst_ptr = dst; srclen && dstlen; src++, srclen--)
		{
			WCHAR wch = *src;
			if (uint64_t(wch) <= 0xffff) {
				if ((flags & NORM_IGNORESYMBOLS) && (get_char_typeW(wch) & (C1_PUNCT | C1_SPACE)))
					continue;
				*dst_ptr++ = toupperW(wch);
			} else {
				if ((flags & NORM_IGNORESYMBOLS) && (iswspace(wch) || iswpunct(wch)))
					continue;
				*dst_ptr++ = towupper(wch);
			}
			dstlen--;
		}
	}
	else if (flags & LCMAP_LOWERCASE)
	{
		for (dst_ptr = dst; srclen && dstlen; src++, srclen--)
		{
			WCHAR wch = *src;
			if (uint64_t(wch) <= 0xffff) {
				if ((flags & NORM_IGNORESYMBOLS) && (get_char_typeW(wch) & (C1_PUNCT | C1_SPACE)))
					continue;
				*dst_ptr++ = tolowerW(wch);

			} else {
				if ((flags & NORM_IGNORESYMBOLS) && (iswspace(wch) || iswpunct(wch)))
					continue;
				*dst_ptr++ = towlower(wch);
			}

			dstlen--;
		}
	}
	else
	{
		if (src == dst)
		{
			WINPORT(SetLastError)(ERROR_INVALID_FLAGS);
			return 0;
		}
		for (dst_ptr = dst; srclen && dstlen; src++, srclen--)
		{
			WCHAR wch = *src;
			if (uint64_t(wch) <= 0xffff) {
				if ((flags & NORM_IGNORESYMBOLS) && (get_char_typeW(wch) & (C1_PUNCT | C1_SPACE)))
					continue;
			} else if ((flags & NORM_IGNORESYMBOLS) && (iswspace(wch) || iswpunct(wch))) {
				continue;
			}
			*dst_ptr++ = wch;
			dstlen--;
		}
	}

	if (srclen)
	{
		WINPORT(SetLastError)(ERROR_INSUFFICIENT_BUFFER);
		return 0;
	}

	return dst_ptr - dst;
}


/******************************************************************************
*           CompareStringEx    (KERNEL32.@)
*/
INT WINAPI WINPORT(CompareStringEx)(LPCWSTR locale, DWORD flags, LPCWSTR str1, INT len1,
									LPCWSTR str2, INT len2, LPNLSVERSIONINFO version, LPVOID reserved, LPARAM lParam)
{
	DWORD supported_flags = NORM_IGNORECASE|NORM_IGNORENONSPACE|NORM_IGNORESYMBOLS|SORT_STRINGSORT
		|NORM_IGNOREKANATYPE|NORM_IGNOREWIDTH|LOCALE_USE_CP_ACP;
	DWORD semistub_flags = NORM_LINGUISTIC_CASING|LINGUISTIC_IGNORECASE|0x10000000;
	/* 0x10000000 is related to diacritics in Arabic, Japanese, and Hebrew */
	INT ret;

	if (!str1 || !str2)
	{
		WINPORT(SetLastError)(ERROR_INVALID_PARAMETER);
		return 0;
	}

	if (flags & ~(supported_flags|semistub_flags))
	{
		WINPORT(SetLastError)(ERROR_INVALID_FLAGS);
		return 0;
	}

	if (len1 < 0) len1 = wcslen(str1);
	if (len2 < 0) len2 = wcslen(str2);

	ret = wine_compare_string(flags, str1, len1, str2, len2);
	if (ret) /* need to translate result */
		return (ret < 0) ? CSTR_LESS_THAN : CSTR_GREATER_THAN;
	return CSTR_EQUAL;
}

WINPORT_DECL(CompareStringA, int, ( LCID Locale, DWORD dwCmpFlags, 
		LPCSTR lpString1, int cchCount1, LPCSTR lpString2, int cchCount2))
{
	if (cchCount1==-1) cchCount1 = lpString1 ? strlen(lpString1) : 0;
	if (cchCount2==-1) cchCount2 = lpString2 ? strlen(lpString2) : 0;
	std::vector<WCHAR> wstr1(cchCount1 * 2 + 1);
	std::vector<WCHAR> wstr2(cchCount2 * 2 + 1);
	int l1 = WINPORT(MultiByteToWideChar)(CP_ACP, 0, lpString1, cchCount1, &wstr1[0], wstr1.size() - 1 );
	int l2 = WINPORT(MultiByteToWideChar)(CP_ACP, 0, lpString2, cchCount2, &wstr2[0], wstr2.size() - 1);
	return WINPORT(CompareString)( Locale, dwCmpFlags, &wstr1[0], l1, &wstr2[0], l2);
}

static BOOL WINPORT(GetStringType)( DWORD type, LPCWSTR src, INT count, LPWORD chartype )
{
	static const unsigned char type2_map[16] =
	{
		C2_NOTAPPLICABLE,      /* unassigned */
		C2_LEFTTORIGHT,        /* L */
		C2_RIGHTTOLEFT,        /* R */
		C2_EUROPENUMBER,       /* EN */
		C2_EUROPESEPARATOR,    /* ES */
		C2_EUROPETERMINATOR,   /* ET */
		C2_ARABICNUMBER,       /* AN */
		C2_COMMONSEPARATOR,    /* CS */
		C2_BLOCKSEPARATOR,     /* B */
		C2_SEGMENTSEPARATOR,   /* S */
		C2_WHITESPACE,         /* WS */
		C2_OTHERNEUTRAL,       /* ON */
		C2_RIGHTTOLEFT,        /* AL */
		C2_NOTAPPLICABLE,      /* NSM */
		C2_NOTAPPLICABLE,      /* BN */
		C2_OTHERNEUTRAL        /* LRE, LRO, RLE, RLO, PDF */
	};

	if (!src)
	{
		WINPORT(SetLastError)( ERROR_INVALID_PARAMETER );
		return FALSE;
	}

	if (count == -1) count = wcslen(src) + 1;
	switch(type)
	{
	case CT_CTYPE1:
		while (count--) *chartype++ = get_char_typeW( *src++ ) & 0xfff;
		break;
	case CT_CTYPE2:
		while (count--) *chartype++ = type2_map[get_char_typeW( *src++ ) >> 12];
		break;
	case CT_CTYPE3:
		{
			while (count--)
			{
				int c = *src;
				WORD type1, type3 = 0; /* C3_NOTAPPLICABLE */

				type1 = get_char_typeW( *src++ ) & 0xfff;
				/* try to construct type3 from type1 */
				if(type1 & C1_SPACE) type3 |= C3_SYMBOL;
				if(type1 & C1_ALPHA) type3 |= C3_ALPHA;
				if ((c>=0x30A0)&&(c<=0x30FF)) type3 |= C3_KATAKANA;
				if ((c>=0x3040)&&(c<=0x309F)) type3 |= C3_HIRAGANA;
				if ((c>=0x4E00)&&(c<=0x9FAF)) type3 |= C3_IDEOGRAPH;
				if (c == 0x0640) type3 |= C3_KASHIDA;
				if ((c>=0x3000)&&(c<=0x303F)) type3 |= C3_SYMBOL;

				if ((c>=0xD800)&&(c<=0xDBFF)) type3 |= C3_HIGHSURROGATE;
				if ((c>=0xDC00)&&(c<=0xDFFF)) type3 |= C3_LOWSURROGATE;

				if ((c>=0xFF00)&&(c<=0xFF60)) type3 |= C3_FULLWIDTH;
				if ((c>=0xFF00)&&(c<=0xFF20)) type3 |= C3_SYMBOL;
				if ((c>=0xFF3B)&&(c<=0xFF40)) type3 |= C3_SYMBOL;
				if ((c>=0xFF5B)&&(c<=0xFF60)) type3 |= C3_SYMBOL;
				if ((c>=0xFF21)&&(c<=0xFF3A)) type3 |= C3_ALPHA;
				if ((c>=0xFF41)&&(c<=0xFF5A)) type3 |= C3_ALPHA;
				if ((c>=0xFFE0)&&(c<=0xFFE6)) type3 |= C3_FULLWIDTH;
				if ((c>=0xFFE0)&&(c<=0xFFE6)) type3 |= C3_SYMBOL;

				if ((c>=0xFF61)&&(c<=0xFFDC)) type3 |= C3_HALFWIDTH;
				if ((c>=0xFF61)&&(c<=0xFF64)) type3 |= C3_SYMBOL;
				if ((c>=0xFF65)&&(c<=0xFF9F)) type3 |= C3_KATAKANA;
				if ((c>=0xFF65)&&(c<=0xFF9F)) type3 |= C3_ALPHA;
				if ((c>=0xFFE8)&&(c<=0xFFEE)) type3 |= C3_HALFWIDTH;
				if ((c>=0xFFE8)&&(c<=0xFFEE)) type3 |= C3_SYMBOL;
				*chartype++ = type3;
			}
			break;
		}
	default:
		WINPORT(SetLastError)( ERROR_INVALID_PARAMETER );
		return FALSE;
	}
	return TRUE;
}

//////////////////////////
extern "C" {
	WINPORT_DECL(LCMapString, INT, (LCID lcid, DWORD flags, LPCWSTR src, INT srclen, LPWSTR dst, INT dstlen))
	{
		return WINPORT(LCMapStringEx)(NULL, flags, src, srclen, dst, dstlen, NULL, NULL, 0);
	}


	WINPORT_DECL(CharLowerBuff, DWORD , ( LPWSTR str, DWORD len ))
	{
		if (!str) return 0; /* YES */
		return WINPORT(LCMapString)( LOCALE_USER_DEFAULT, LCMAP_LOWERCASE, str, len, str, len );
	}

	WINPORT_DECL(CharUpperBuff, DWORD , ( LPWSTR str, DWORD len ))
	{
		if (!str) return 0; /* YES */
		return WINPORT(LCMapString)( LOCALE_USER_DEFAULT, LCMAP_UPPERCASE, str, len, str, len );
	}

	WINPORT_DECL(IsCharLower, BOOL, (WCHAR ch))
	{
		if (uint64_t(ch) <= 0xffff) {
			WORD type;
			return WINPORT(GetStringType)( CT_CTYPE1, &ch, 1, &type ) && (type & C1_LOWER);
		}
		return iswlower(ch);
	}

	WINPORT_DECL(IsCharUpper, BOOL, (WCHAR ch))
	{
		if (uint64_t(ch) <= 0xffff) {
			WORD type;
			return WINPORT(GetStringType)( CT_CTYPE1, &ch, 1, &type ) && (type & C1_UPPER);
		}
		return iswupper(ch);
	}

	WINPORT_DECL(IsCharAlphaNumeric, BOOL, (WCHAR ch))
	{
		if (uint64_t(ch) <= 0xffff) {
			WORD type;
			return WINPORT(GetStringType)( CT_CTYPE1, &ch, 1, &type ) && (type & (C1_ALPHA|C1_DIGIT));
		}
		return iswalnum(ch);
	}

	WINPORT_DECL(IsCharAlpha, BOOL, (WCHAR ch))
	{
		if (uint64_t(ch) <= 0xffff) {
			WORD type;
			return WINPORT(GetStringType)( CT_CTYPE1, &ch, 1, &type ) && (type & C1_ALPHA);
		}
		return iswalpha(ch);
	}


	WINPORT_DECL(CompareString, INT, (LCID lcid, DWORD flags, LPCWSTR str1, INT len1, LPCWSTR str2, INT len2))
	{
		return WINPORT(CompareStringEx)(NULL, flags, str1, len1, str2, len2, NULL, NULL, 0);
	}

	WINPORT_DECL(CharLower, LPWSTR, (LPWSTR str))
	{
		if (!IS_INTRESOURCE( str ))
		{
			WINPORT(CharLowerBuff)( str, wcslen( str ));
			return str;
		}
		else
		{
			WCHAR ch = LOWORD( str );
			WINPORT(CharLowerBuff)( &ch, 1 );
			return (LPWSTR)(UINT_PTR)ch;
		}
	}

	WINPORT_DECL(CharUpper, LPWSTR, (LPWSTR str))
	{
		if (!IS_INTRESOURCE( str ))
		{
			WINPORT(CharUpperBuff)( str, wcslen( str ));
			return str;
		}
		else
		{
			WCHAR ch = LOWORD( str );
			WINPORT(CharUpperBuff)( &ch, 1 );
			return (LPWSTR)(UINT_PTR)ch;
		}
	}

}

