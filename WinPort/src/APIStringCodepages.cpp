#include <set>
#include <string>
#include <locale>
#include <algorithm>
#include <iostream>
#include <fstream>
#include <mutex>

#include <stdlib.h>
#include <locale.h>
#include <utils.h>
#include <StackHeapArray.hpp>

#include "WinCompat.h"
#include "WinPort.h"
#include "wineguts.h"
#include "PathHelpers.h"
#include "UtfConvert.hpp"


template <class SRC_T, class DST_T>
	int TranscodeUTF(int flags, const SRC_T *src, size_t srclen, DST_T *dst, size_t dstlen)
{
	const bool fail_on_illformed = ((flags & MB_ERR_INVALID_CHARS) != 0);

	if (dstlen == 0) {
		DummyPushBack<DST_T> pb;
		try {
			const unsigned ucr = UtfConvert(src, srclen, pb, fail_on_illformed);
			if (ucr & (CONV_ILLFORMED_CHARS | CONV_NEED_MORE_SRC)) {
				WINPORT(SetLastError)( ERROR_NO_UNICODE_TRANSLATION );
			}

		} catch (std::exception &e) {
			fprintf(stderr, "%s: %s\n", __FUNCTION__, e.what());
			WINPORT(SetLastError)( ERROR_NO_UNICODE_TRANSLATION );
		}
		return (int)pb.size();
	}

	ArrayPushBack<DST_T> pb(dst, dst + dstlen);
	try {
			const unsigned ucr = UtfConvert(src, srclen, pb, fail_on_illformed);
			if (ucr & (CONV_ILLFORMED_CHARS | CONV_NEED_MORE_SRC)) {
				WINPORT(SetLastError)( ERROR_NO_UNICODE_TRANSLATION );

			} else if (ucr & CONV_NEED_MORE_DST) {
				WINPORT(SetLastError)( ERROR_INSUFFICIENT_BUFFER );
				return 0;
			}

	} catch (ArrayPushBackOverflow &e) {
		(void)e;
		WINPORT(SetLastError)( ERROR_INSUFFICIENT_BUFFER );
		return 0;

	} catch (std::exception &e) {
		fprintf(stderr, "%s: %s\n", __FUNCTION__, e.what());
		WINPORT(SetLastError)( ERROR_NO_UNICODE_TRANSLATION );
	}

	return (int)pb.size();
}

template <class BYTES_TYPE, bool BYTEREV>
	static int Wide2Bytes( int flags, const WCHAR *src, int srclen, void *dst, int dstlen)
{
	if (srclen < 0) { // per MSDN - conversion should include terminating NUL char
		srclen = tzlen(src) + 1;
	}

	if (dstlen > 0) {
		dstlen/= sizeof(BYTES_TYPE);
		// if dstlen specified was nonzero but obviousy too small then fail fast
		if (dstlen == 0 || (sizeof(BYTES_TYPE) == sizeof(WCHAR) && dstlen < srclen)) {
			WINPORT(SetLastError)( ERROR_INSUFFICIENT_BUFFER );
			return 0;
		}

	} else {
		dstlen = 0;
	}

	int r;
	if (sizeof(BYTES_TYPE) == sizeof(WCHAR)) { // noop shortcut
		if (dstlen) {
			memcpy(dst, src, srclen * sizeof(WCHAR));
		}
		r = srclen;

	} else {
		r = TranscodeUTF(flags, src, srclen, (BYTES_TYPE *)dst, dstlen);
	}

	if (r > 0) {
		if (BYTEREV && dstlen) {
			RevBytes((BYTES_TYPE *)dst, r);
		}
		r*= sizeof(BYTES_TYPE);
	}
	return r;
}

template <class BYTES_TYPE, bool BYTEREV>
	static int Bytes2Wide( int flags, const void *src, int srclen, WCHAR *dst, int dstlen)
{
	if (srclen < 0) { // per MSDN - conversion should include terminating NUL char
		srclen = tzlen((const BYTES_TYPE*)src) + 1;
	} else {
		srclen/= sizeof(BYTES_TYPE);
	}
	if (dstlen < 0) {
		dstlen = 0;
	}

	if (sizeof(BYTES_TYPE) == sizeof(WCHAR)) { // noop shortcut
		if (dstlen != 0) {
			if (dstlen < srclen) {
				WINPORT(SetLastError)( ERROR_INSUFFICIENT_BUFFER );
				return 0;
			}
			memcpy(dst, src, srclen * sizeof(WCHAR));
		}
		return srclen;
	}

	if (BYTEREV && srclen > 0) {
		StackHeapArray<BYTES_TYPE> reversed_src(srclen);
		RevBytes(reversed_src.Get(), (const BYTES_TYPE *)src, reversed_src.Count());
		return TranscodeUTF(flags, reversed_src.Get(), reversed_src.Count(), dst, dstlen);
	}

	return TranscodeUTF(flags, (const BYTES_TYPE *)src, srclen, dst, dstlen);
}


#define IsLocaleMatches(current, wanted_literal) \
	( strncmp((current), wanted_literal, sizeof(wanted_literal) - 1) == 0 && \
	( (current)[sizeof(wanted_literal) - 1] == 0 || (current)[sizeof(wanted_literal) - 1] == '.') )

struct Codepages
{
	int oem;
	int ansi;
};

static Codepages DeduceCodepages()
{
	// deduce oem/ansi cp from system locale
	std::ifstream is;
	is.open(InMyConfig("cp").c_str());
	if (is.is_open()) {
		Codepages out {866, 1251};
		std::string str;
		getline (is, str);
		if (!str.empty()) {
			out.oem = atoi(str.c_str());
		}
		getline (is, str);
		if (!str.empty()) {
			out.ansi = atoi(str.c_str());
		}
		return out;
	}

	const char *lc = setlocale(LC_CTYPE, NULL);
	if (!lc) {
		fprintf(stderr, "DeduceCodepages: setlocale returned NULL\n");
		return Codepages{866, 1251};
	}

	if (IsLocaleMatches(lc, "af_ZA")) { return Codepages{850, 1252}; }
	if (IsLocaleMatches(lc, "ar_SA")) { return Codepages{720, 1256}; }
	if (IsLocaleMatches(lc, "ar_LB")) { return Codepages{720, 1256}; }
	if (IsLocaleMatches(lc, "ar_EG")) { return Codepages{720, 1256}; }
	if (IsLocaleMatches(lc, "ar_DZ")) { return Codepages{720, 1256}; }
	if (IsLocaleMatches(lc, "ar_BH")) { return Codepages{720, 1256}; }
	if (IsLocaleMatches(lc, "ar_IQ")) { return Codepages{720, 1256}; }
	if (IsLocaleMatches(lc, "ar_JO")) { return Codepages{720, 1256}; }
	if (IsLocaleMatches(lc, "ar_KW")) { return Codepages{720, 1256}; }
	if (IsLocaleMatches(lc, "ar_LY")) { return Codepages{720, 1256}; }
	if (IsLocaleMatches(lc, "ar_MA")) { return Codepages{720, 1256}; }
	if (IsLocaleMatches(lc, "ar_OM")) { return Codepages{720, 1256}; }
	if (IsLocaleMatches(lc, "ar_QA")) { return Codepages{720, 1256}; }
	if (IsLocaleMatches(lc, "ar_SY")) { return Codepages{720, 1256}; }
	if (IsLocaleMatches(lc, "ar_TN")) { return Codepages{720, 1256}; }
	if (IsLocaleMatches(lc, "ar_AE")) { return Codepages{720, 1256}; }
	if (IsLocaleMatches(lc, "ar_YE")) { return Codepages{720, 1256}; }
	if (IsLocaleMatches(lc, "ast_ES")) { return Codepages{850, 1252}; }
	if (IsLocaleMatches(lc, "az_AZ")) { return Codepages{866, 1251}; }
	if (IsLocaleMatches(lc, "az_AZ")) { return Codepages{857, 1254}; }
	if (IsLocaleMatches(lc, "be_BY")) { return Codepages{866, 1251}; }
	if (IsLocaleMatches(lc, "bg_BG")) { return Codepages{866, 1251}; }
	if (IsLocaleMatches(lc, "br_FR")) { return Codepages{850, 1252}; }
	if (IsLocaleMatches(lc, "ca_ES")) { return Codepages{850, 1252}; }
	if (IsLocaleMatches(lc, "zh_CN")) { return Codepages{936, 936}; }
	if (IsLocaleMatches(lc, "zh_TW")) { return Codepages{950, 950}; }
	if (IsLocaleMatches(lc, "kw_GB")) { return Codepages{850, 1252}; }
	if (IsLocaleMatches(lc, "cs_CZ")) { return Codepages{852, 1250}; }
	if (IsLocaleMatches(lc, "cy_GB")) { return Codepages{850, 28604}; }
	if (IsLocaleMatches(lc, "da_DK")) { return Codepages{850, 1252}; }
	if (IsLocaleMatches(lc, "de_AT")) { return Codepages{850, 1252}; }
	if (IsLocaleMatches(lc, "de_LI")) { return Codepages{850, 1252}; }
	if (IsLocaleMatches(lc, "de_LU")) { return Codepages{850, 1252}; }
	if (IsLocaleMatches(lc, "de_CH")) { return Codepages{850, 1252}; }
	if (IsLocaleMatches(lc, "de_DE")) { return Codepages{850, 1252}; }
	if (IsLocaleMatches(lc, "el_GR")) { return Codepages{737, 1253}; }
	if (IsLocaleMatches(lc, "en_AU")) { return Codepages{850, 1252}; }
	if (IsLocaleMatches(lc, "en_CA")) { return Codepages{850, 1252}; }
	if (IsLocaleMatches(lc, "en_GB")) { return Codepages{850, 1252}; }
	if (IsLocaleMatches(lc, "en_IE")) { return Codepages{850, 1252}; }
	if (IsLocaleMatches(lc, "en_JM")) { return Codepages{850, 1252}; }
	if (IsLocaleMatches(lc, "en_BZ")) { return Codepages{850, 1252}; }
	if (IsLocaleMatches(lc, "en_PH")) { return Codepages{437, 1252}; }
	if (IsLocaleMatches(lc, "en_ZA")) { return Codepages{437, 1252}; }
	if (IsLocaleMatches(lc, "en_TT")) { return Codepages{850, 1252}; }
	if (IsLocaleMatches(lc, "en_US")) { return Codepages{437, 1252}; }
	if (IsLocaleMatches(lc, "en_ZW")) { return Codepages{437, 1252}; }
	if (IsLocaleMatches(lc, "en_NZ")) { return Codepages{850, 1252}; }
	if (IsLocaleMatches(lc, "es_PA")) { return Codepages{850, 1252}; }
	if (IsLocaleMatches(lc, "es_BO")) { return Codepages{850, 1252}; }
	if (IsLocaleMatches(lc, "es_CR")) { return Codepages{850, 1252}; }
	if (IsLocaleMatches(lc, "es_DO")) { return Codepages{850, 1252}; }
	if (IsLocaleMatches(lc, "es_SV")) { return Codepages{850, 1252}; }
	if (IsLocaleMatches(lc, "es_EC")) { return Codepages{850, 1252}; }
	if (IsLocaleMatches(lc, "es_GT")) { return Codepages{850, 1252}; }
	if (IsLocaleMatches(lc, "es_HN")) { return Codepages{850, 1252}; }
	if (IsLocaleMatches(lc, "es_NI")) { return Codepages{850, 1252}; }
	if (IsLocaleMatches(lc, "es_CL")) { return Codepages{850, 1252}; }
	if (IsLocaleMatches(lc, "es_MX")) { return Codepages{850, 1252}; }
	if (IsLocaleMatches(lc, "es_ES")) { return Codepages{850, 1252}; }
	if (IsLocaleMatches(lc, "es_CO")) { return Codepages{850, 1252}; }
	if (IsLocaleMatches(lc, "es_ES")) { return Codepages{850, 1252}; }
	if (IsLocaleMatches(lc, "es_PE")) { return Codepages{850, 1252}; }
	if (IsLocaleMatches(lc, "es_AR")) { return Codepages{850, 1252}; }
	if (IsLocaleMatches(lc, "es_PR")) { return Codepages{850, 1252}; }
	if (IsLocaleMatches(lc, "es_VE")) { return Codepages{850, 1252}; }
	if (IsLocaleMatches(lc, "es_UY")) { return Codepages{850, 1252}; }
	if (IsLocaleMatches(lc, "es_PY")) { return Codepages{850, 1252}; }
	if (IsLocaleMatches(lc, "et_EE")) { return Codepages{775, 1257}; }
	if (IsLocaleMatches(lc, "eu_ES")) { return Codepages{850, 1252}; }
	if (IsLocaleMatches(lc, "fa_IR")) { return Codepages{720, 1256}; }
	if (IsLocaleMatches(lc, "fi_FI")) { return Codepages{850, 1252}; }
	if (IsLocaleMatches(lc, "fo_FO")) { return Codepages{850, 1252}; }
	if (IsLocaleMatches(lc, "fr_FR")) { return Codepages{850, 1252}; }
	if (IsLocaleMatches(lc, "fr_BE")) { return Codepages{850, 1252}; }
	if (IsLocaleMatches(lc, "fr_CA")) { return Codepages{850, 1252}; }
	if (IsLocaleMatches(lc, "fr_LU")) { return Codepages{850, 1252}; }
	if (IsLocaleMatches(lc, "fr_MC")) { return Codepages{850, 1252}; }
	if (IsLocaleMatches(lc, "fr_CH")) { return Codepages{850, 1252}; }
	if (IsLocaleMatches(lc, "ga_IE")) { return Codepages{437, 1252}; }
	if (IsLocaleMatches(lc, "gd_GB")) { return Codepages{850, 1252}; }
	if (IsLocaleMatches(lc, "gv_IM")) { return Codepages{850, 1252}; }
	if (IsLocaleMatches(lc, "gl_ES")) { return Codepages{850, 1252}; }
	if (IsLocaleMatches(lc, "he_IL")) { return Codepages{862, 1255}; }
	if (IsLocaleMatches(lc, "hr_HR")) { return Codepages{852, 1250}; }
	if (IsLocaleMatches(lc, "hu_HU")) { return Codepages{852, 1250}; }
	if (IsLocaleMatches(lc, "id_ID")) { return Codepages{850, 1252}; }
	if (IsLocaleMatches(lc, "is_IS")) { return Codepages{850, 1252}; }
	if (IsLocaleMatches(lc, "it_IT")) { return Codepages{850, 1252}; }
	if (IsLocaleMatches(lc, "it_CH")) { return Codepages{850, 1252}; }
	if (IsLocaleMatches(lc, "iv_IV")) { return Codepages{437, 1252}; }
	if (IsLocaleMatches(lc, "ja_JP")) { return Codepages{932, 932}; }
	if (IsLocaleMatches(lc, "kk_KZ")) { return Codepages{866, 1251}; }
	if (IsLocaleMatches(lc, "ko_KR")) { return Codepages{949, 949}; }
	if (IsLocaleMatches(lc, "ky_KG")) { return Codepages{866, 1251}; }
	if (IsLocaleMatches(lc, "lt_LT")) { return Codepages{775, 1251}; }
	if (IsLocaleMatches(lc, "lv_LV")) { return Codepages{775, 1257}; }
	if (IsLocaleMatches(lc, "mk_MK")) { return Codepages{866, 1251}; }
	if (IsLocaleMatches(lc, "mn_MN")) { return Codepages{866, 1251}; }
	if (IsLocaleMatches(lc, "ms_BN")) { return Codepages{850, 1252}; }
	if (IsLocaleMatches(lc, "ms_MY")) { return Codepages{850, 1252}; }
	if (IsLocaleMatches(lc, "nl_BE")) { return Codepages{850, 1252}; }
	if (IsLocaleMatches(lc, "nl_NL")) { return Codepages{850, 1252}; }
	if (IsLocaleMatches(lc, "nl_SR")) { return Codepages{850, 1252}; }
	if (IsLocaleMatches(lc, "nn_NO")) { return Codepages{850, 1252}; }
	if (IsLocaleMatches(lc, "nb_NO")) { return Codepages{850, 1252}; }
	if (IsLocaleMatches(lc, "pl_PL")) { return Codepages{852, 1250}; }
	if (IsLocaleMatches(lc, "pt_BR")) { return Codepages{850, 1252}; }
	if (IsLocaleMatches(lc, "pt_PT")) { return Codepages{850, 1252}; }
	if (IsLocaleMatches(lc, "rm_CH")) { return Codepages{850, 1252}; }
	if (IsLocaleMatches(lc, "ro_RO")) { return Codepages{852, 1250}; }
	if (IsLocaleMatches(lc, "ru_RU")) { return Codepages{866, 1251}; }
	if (IsLocaleMatches(lc, "sk_SK")) { return Codepages{852, 1250}; }
	if (IsLocaleMatches(lc, "sl_SI")) { return Codepages{852, 1250}; }
	if (IsLocaleMatches(lc, "sq_AL")) { return Codepages{852, 1250}; }
	if (IsLocaleMatches(lc, "sr_RS")) { return Codepages{855, 1251}; }
	if (IsLocaleMatches(lc, "sr_RS")) { return Codepages{852, 1250}; }
	if (IsLocaleMatches(lc, "sv_SE")) { return Codepages{850, 1252}; }
	if (IsLocaleMatches(lc, "sv_FI")) { return Codepages{850, 1252}; }
	if (IsLocaleMatches(lc, "sw_KE")) { return Codepages{437, 1252}; }
	if (IsLocaleMatches(lc, "th_TH")) { return Codepages{874, 874}; }
	if (IsLocaleMatches(lc, "tr_TR")) { return Codepages{857, 1254}; }
	if (IsLocaleMatches(lc, "tt_RU")) { return Codepages{866, 1251}; }
	if (IsLocaleMatches(lc, "uk_UA")) { return Codepages{866, 1251}; }
	if (IsLocaleMatches(lc, "ur_PK")) { return Codepages{720, 1256}; }
	if (IsLocaleMatches(lc, "uz_UZ")) { return Codepages{866, 1251}; }
	if (IsLocaleMatches(lc, "uz_UZ")) { return Codepages{857, 1254}; }
	if (IsLocaleMatches(lc, "vi_VN")) { return Codepages{1258, 1258}; }
	if (IsLocaleMatches(lc, "wa_BE")) { return Codepages{850, 1252}; }
	if (IsLocaleMatches(lc, "zh_HK")) { return Codepages{950, 950}; }
	if (IsLocaleMatches(lc, "zh_SG")) { return Codepages{936, 936}; }
	if (IsLocaleMatches(lc, "zh_MO")) { return Codepages{950, 1252}; }

	if (!IsLocaleMatches(lc, "C") && !IsLocaleMatches(lc, "UTF8")) {
		fprintf(stderr, "DeduceCodepages: unknown locale '%s'\n", lc);
	}

	return Codepages{866, 1251};
}

static UINT TranslateCodepage(UINT codepage)
{
	static Codepages s_cp = DeduceCodepages();
	switch (codepage) {
		case CP_ACP: return s_cp.ansi;
		case CP_OEMCP: return s_cp.oem;
		default:
			return codepage;
	}
}

extern "C" {
	/***********************************************************************
	*              utf7_write_w
	*
	* Helper for utf7_mbstowcs
	*
	* RETURNS
	*   TRUE on success, FALSE on error
	*/
	static inline BOOL utf7_write_w(WCHAR *dst, int dstlen, int *index, WCHAR character)
	{
		if (dstlen > 0)
		{
			if (*index >= dstlen)
				return FALSE;

			dst[*index] = character;
		}

		(*index)++;

		return TRUE;
	}

	/***********************************************************************
	*              utf7_mbstowcs
	*
	* UTF-7 to UTF-16 string conversion, helper for MultiByteToWideChar
	*
	* RETURNS
	*   On success, the number of characters written
	*   On dst buffer overflow, -1
	*/
	static int utf7_mbstowcs(const char *src, int srclen, WCHAR *dst, int dstlen)
	{
		static const signed char base64_decoding_table[] =
		{
			-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, /* 0x00-0x0F */
			-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, /* 0x10-0x1F */
			-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 62, -1, -1, -1, 63, /* 0x20-0x2F */
			52, 53, 54, 55, 56, 57, 58, 59, 60, 61, -1, -1, -1, -1, -1, -1, /* 0x30-0x3F */
			-1,  0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14, /* 0x40-0x4F */
			15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, -1, -1, -1, -1, -1, /* 0x50-0x5F */
			-1, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40, /* 0x60-0x6F */
			41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, -1, -1, -1, -1, -1  /* 0x70-0x7F */
		};

		const char *source_end = src + srclen;
		int dest_index = 0;

		DWORD byte_pair = 0;
		short offset = 0;

		while (src < source_end)
		{
			if (*src == '+')
			{
				src++;
				if (src >= source_end)
					break;

				if (*src == '-')
				{
					/* just a plus sign escaped as +- */
					if (!utf7_write_w(dst, dstlen, &dest_index, '+'))
						return -1;
					src++;
					continue;
				}

				do
				{
					signed char sextet = *src;
					if (sextet == '-')
					{
						/* skip over the dash and end base64 decoding
						* the current, unfinished byte pair is discarded */
						src++;
						offset = 0;
						break;
					}
					if (sextet < 0)
					{
						/* the next character of src is < 0 and therefore not part of a base64 sequence
						* the current, unfinished byte pair is NOT discarded in this case
						* this is probably a bug in Windows */
						break;
					}

					sextet = base64_decoding_table[sextet];
					if (sextet == -1)
					{
						/* -1 means that the next character of src is not part of a base64 sequence
						* in other words, all sextets in this base64 sequence have been processed
						* the current, unfinished byte pair is discarded */
						offset = 0;
						break;
					}

					byte_pair = (byte_pair << 6) | sextet;
					offset += 6;

					if (offset >= 16)
					{
						/* this byte pair is done */
						if (!utf7_write_w(dst, dstlen, &dest_index, (byte_pair >> (offset - 16)) & 0xFFFF))
							return -1;
						offset -= 16;
					}

					src++;
				}
				while (src < source_end);
			}
			else
			{
				/* we have to convert to unsigned char in case *src < 0 */
				if (!utf7_write_w(dst, dstlen, &dest_index, (unsigned char)*src))
					return -1;
				src++;
			}
		}

		return dest_index;
	}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


	/***********************************************************************
	*              MultiByteToWideChar   (KERNEL32.@)
	*
	* Convert a multibyte character string into a Unicode string.
	*
	* PARAMS
	*   page   [I] Codepage character set to convert from
	*   flags  [I] Character mapping flags
	*   src    [I] Source string buffer
	*   srclen [I] Length of src (in bytes), or -1 if src is NUL terminated
	*   dst    [O] Destination buffer
	*   dstlen [I] Length of dst (in WCHARs), or 0 to compute the required length
	*
	* RETURNS
	*   Success: If dstlen > 0, the number of characters written to dst.
	*            If dstlen == 0, the number of characters needed to perform the
	*            conversion. In both cases the count includes the terminating NUL.
	*   Failure: 0. Use GetLastError() to determine the cause. Possible errors are
	*            ERROR_INSUFFICIENT_BUFFER, if not enough space is available in dst
	*            and dstlen != 0; ERROR_INVALID_PARAMETER,  if an invalid parameter
	*            is passed, and ERROR_NO_UNICODE_TRANSLATION if no translation is
	*            possible for src.
	*/
	WINPORT_DECL(MultiByteToWideChar, int, ( UINT page, DWORD flags,
		LPCSTR src, int srclen, LPWSTR dst, int dstlen))
	{
		//return ::MultiByteToWideChar(page, flags, src, srclen, dst, dstlen);
		//fprintf(stderr, "MultiByteToWideChar\n");
		const union cptable *table;
		int ret;
		page = TranslateCodepage(page);

		if (!src || !srclen || (!dst && dstlen) || dstlen < 0)
		{
			WINPORT(SetLastError)( ERROR_INVALID_PARAMETER );
			return 0;
		}

		if (srclen < 0) srclen = strlen(src) + 1;

		WINPORT(SetLastError)( ERROR_SUCCESS );
		switch(page)
		{
		case CP_SYMBOL:
			if (flags)
			{
				WINPORT(SetLastError)( ERROR_INVALID_FLAGS );
				return 0;
			}
			ret = wine_cpsymbol_mbstowcs( src, srclen, dst, dstlen );
			break;
		case CP_UTF7:
			if (flags)
			{
				WINPORT(SetLastError)( ERROR_INVALID_FLAGS );
				return 0;
			}
			ret = utf7_mbstowcs( src, srclen, dst, dstlen );
			break;

		case CP_UTF16LE:
			ret = Bytes2Wide<uint16_t, false>( flags, src, srclen, dst, dstlen );
			break;

		case CP_UTF16BE:
			ret = Bytes2Wide<uint16_t, true>( flags, src, srclen, dst, dstlen );
			break;

		case CP_UTF32LE:
			ret = Bytes2Wide<uint32_t, false>( flags, src, srclen, dst, dstlen );
			break;

		case CP_UTF32BE:
			ret = Bytes2Wide<uint32_t, true>( flags, src, srclen, dst, dstlen );
			break;

		case CP_UTF8:
			ret = Bytes2Wide<uint8_t, false>( flags, src, srclen, dst, dstlen);
			break;

		default:
			if (!(table = get_codepage_table( page )))
			{
				fprintf(stderr, "MultiByteToWideChar: bad codepage %x\n", page);
				WINPORT(SetLastError)( ERROR_INVALID_PARAMETER );
				return 0;
			}
			ret = wine_cp_mbstowcs( table, flags, src, srclen, dst, dstlen );
			break;
		}

		if (ret <= 0)
		{
			switch(ret)
			{
			case -1: WINPORT(SetLastError)( ERROR_INSUFFICIENT_BUFFER ); break;
			case -2: WINPORT(SetLastError)( ERROR_NO_UNICODE_TRANSLATION ); break;
			}
			ret = 0;
		}
		return ret;
	}


	/***********************************************************************
	*              utf7_can_directly_encode
	*
	* Helper for utf7_wcstombs
	*/
	static inline BOOL utf7_can_directly_encode(WCHAR codepoint)
	{
		static const BOOL directly_encodable_table[] =
		{
			1, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 1, 0, 0, /* 0x00 - 0x0F */
			0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, /* 0x10 - 0x1F */
			1, 0, 0, 0, 0, 0, 0, 1, 1, 1, 0, 1, 1, 1, 1, 1, /* 0x20 - 0x2F */
			1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 1, /* 0x30 - 0x3F */
			0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, /* 0x40 - 0x4F */
			1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, /* 0x50 - 0x5F */
			0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, /* 0x60 - 0x6F */
			1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1                 /* 0x70 - 0x7A */
		};

		return codepoint <= 0x7A ? directly_encodable_table[codepoint] : FALSE;
	}

	/***********************************************************************
	*              utf7_write_c
	*
	* Helper for utf7_wcstombs
	*
	* RETURNS
	*   TRUE on success, FALSE on error
	*/
	static inline BOOL utf7_write_c(char *dst, int dstlen, int *index, char character)
	{
		if (dstlen > 0)
		{
			if (*index >= dstlen)
				return FALSE;

			dst[*index] = character;
		}

		(*index)++;

		return TRUE;
	}

	/***********************************************************************
	*              utf7_wcstombs
	*
	* UTF-16 to UTF-7 string conversion, helper for WideCharToMultiByte
	*
	* RETURNS
	*   On success, the number of characters written
	*   On dst buffer overflow, -1
	*/
	static int utf7_wcstombs(const WCHAR *src, int srclen, char *dst, int dstlen)
	{
		static const char base64_encoding_table[] =
			"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

		const WCHAR *source_end = src + srclen;
		int dest_index = 0;

		while (src < source_end)
		{
			if (*src == '+')
			{
				if (!utf7_write_c(dst, dstlen, &dest_index, '+'))
					return -1;
				if (!utf7_write_c(dst, dstlen, &dest_index, '-'))
					return -1;
				src++;
			}
			else if (utf7_can_directly_encode(*src))
			{
				if (!utf7_write_c(dst, dstlen, &dest_index, *src))
					return -1;
				src++;
			}
			else
			{
				unsigned int offset = 0;
				DWORD byte_pair = 0;

				if (!utf7_write_c(dst, dstlen, &dest_index, '+'))
					return -1;

				while (src < source_end && !utf7_can_directly_encode(*src))
				{
					byte_pair = (byte_pair << 16) | *src;
					offset += 16;
					while (offset >= 6)
					{
						if (!utf7_write_c(dst, dstlen, &dest_index, base64_encoding_table[(byte_pair >> (offset - 6)) & 0x3F]))
							return -1;
						offset -= 6;
					}
					src++;
				}

				if (offset)
				{
					/* Windows won't create a padded base64 character if there's no room for the - sign
					* as well ; this is probably a bug in Windows */
					if (dstlen > 0 && dest_index + 1 >= dstlen)
						return -1;

					byte_pair <<= (6 - offset);
					if (!utf7_write_c(dst, dstlen, &dest_index, base64_encoding_table[byte_pair & 0x3F]))
						return -1;
				}

				/* Windows always explicitly terminates the base64 sequence
				even though RFC 2152 (page 3, rule 2) does not require this */
				if (!utf7_write_c(dst, dstlen, &dest_index, '-'))
					return -1;
			}
		}

		return dest_index;
	}

	/***********************************************************************
	*              WideCharToMultiByte   (KERNEL32.@)
	*
	* Convert a Unicode character string into a multibyte string.
	*
	* PARAMS
	*   page    [I] Code page character set to convert to
	*   flags   [I] Mapping Flags (MB_ constants from "winnls.h").
	*   src     [I] Source string buffer
	*   srclen  [I] Length of src (in WCHARs), or -1 if src is NUL terminated
	*   dst     [O] Destination buffer
	*   dstlen  [I] Length of dst (in bytes), or 0 to compute the required length
	*   defchar [I] Default character to use for conversion if no exact
	*		    conversion can be made
	*   used    [O] Set if default character was used in the conversion
	*
	* RETURNS
	*   Success: If dstlen > 0, the number of characters written to dst.
	*            If dstlen == 0, number of characters needed to perform the
	*            conversion. In both cases the count includes the terminating NUL.
	*   Failure: 0. Use GetLastError() to determine the cause. Possible errors are
	*            ERROR_INSUFFICIENT_BUFFER, if not enough space is available in dst
	*            and dstlen != 0, and ERROR_INVALID_PARAMETER, if an invalid
	*            parameter was given.
	*/
	WINPORT_DECL(WideCharToMultiByte, int, ( UINT page, DWORD flags, LPCWSTR src,
		int srclen, LPSTR dst, int dstlen, LPCSTR defchar, LPBOOL used))
	{
		//return ::WideCharToMultiByte(page, flags, src, srclen, dst, dstlen, defchar, used);
		//fprintf(stderr, "WideCharToMultiByte\n");
		const union cptable *table;
		int ret, used_tmp;
		page = TranslateCodepage(page);

		if (!src || !srclen || (!dst && dstlen) || dstlen < 0)
		{
			WINPORT(SetLastError)( ERROR_INVALID_PARAMETER );
			return 0;
		}

		if (srclen < 0) srclen = strlenW(src) + 1;

		WINPORT(SetLastError)( ERROR_SUCCESS );
		switch(page)
		{
		case CP_SYMBOL:
			/* when using CP_SYMBOL, ERROR_INVALID_FLAGS takes precedence */
			if (flags)
			{
				WINPORT(SetLastError)( ERROR_INVALID_FLAGS );
				return 0;
			}
			if (defchar || used)
			{
				WINPORT(SetLastError)( ERROR_INVALID_PARAMETER );
				return 0;
			}
			ret = wine_cpsymbol_wcstombs( src, srclen, dst, dstlen );
			break;
		case CP_UTF7:
			/* when using CP_UTF7, ERROR_INVALID_PARAMETER takes precedence */
			if (defchar || used)
			{
				WINPORT(SetLastError)( ERROR_INVALID_PARAMETER );
				return 0;
			}
			if (flags)
			{
				WINPORT(SetLastError)( ERROR_INVALID_FLAGS );
				return 0;
			}
			ret = utf7_wcstombs( src, srclen, dst, dstlen );
			break;

		case CP_UTF16LE:
			ret = Wide2Bytes<uint16_t, false>( flags, src, srclen, dst, dstlen );
			break;

		case CP_UTF16BE:
			ret = Wide2Bytes<uint16_t, true>( flags, src, srclen, dst, dstlen );
			break;

		case CP_UTF32LE:
			ret = Wide2Bytes<uint32_t, false>( flags, src, srclen, dst, dstlen );
			break;

		case CP_UTF32BE:
			ret = Wide2Bytes<uint32_t, true>( flags, src, srclen, dst, dstlen );
			break;

		case CP_UTF8:
			if (defchar || used)
			{
				fprintf(stderr, "WideCharToMultiByte: 'defchar' and 'used' can't be used with UTF8\n");
				WINPORT(SetLastError)( ERROR_INVALID_PARAMETER );
				return 0;
			}
			ret = Wide2Bytes<uint8_t, false>( flags, src, srclen, dst, dstlen );
			break;

		default:
			if (!(table = get_codepage_table( page )))
			{
				fprintf(stderr, "WideCharToMultiByte: bad codepage %x\n", page);
				WINPORT(SetLastError)( ERROR_INVALID_PARAMETER );
				return 0;
			}
			ret = wine_cp_wcstombs( table, flags, src, srclen, dst, dstlen,
				defchar, used ? &used_tmp : NULL );
			if (used) *used = used_tmp;
			break;
		}

		if (ret < 0) {
			switch (ret) {
				case -1: WINPORT(SetLastError)( ERROR_INSUFFICIENT_BUFFER ); break;
				case -2: WINPORT(SetLastError)( ERROR_NO_UNICODE_TRANSLATION ); break;
			}
			ret = 0;
		}
		return ret;
	}

	WINPORT_DECL(GetOEMCP, UINT, ())
	{
		return TranslateCodepage(CP_OEMCP);//get_codepage_table( 866 )->info.codepage;//866
	}

	WINPORT_DECL(GetACP, UINT, ())
	{
		return TranslateCodepage(CP_ACP);//get_codepage_table( 1251 )->info.codepage;//1251
	}

	WINPORT_DECL(GetCPInfo, BOOL, (UINT codepage, LPCPINFO cpinfo))
	{
		const union cptable *table;

		if (!cpinfo)
		{
			WINPORT(SetLastError)( ERROR_INVALID_PARAMETER );
			return FALSE;
		}
		memset(cpinfo, 0, sizeof(*cpinfo));

		codepage = TranslateCodepage(codepage);

		if (!(table = get_codepage_table( codepage )))
		{
			switch(codepage) {
			case CP_UTF7:
			case CP_UTF8:
				cpinfo->DefaultChar[0] = 0x3f;
				cpinfo->MaxCharSize = (codepage == CP_UTF7) ? 5 : 4;
				return TRUE;

			case CP_UTF16LE:
				cpinfo->DefaultChar[0] = 0x3f;
				cpinfo->MaxCharSize = 2;
				return TRUE;

			case CP_UTF16BE:
				cpinfo->DefaultChar[1] = 0x3f;
				cpinfo->MaxCharSize = 2;
				return TRUE;

			case CP_UTF32LE:
				cpinfo->DefaultChar[0] = 0x3f;
				cpinfo->MaxCharSize = 4;
				return TRUE;

			case CP_UTF32BE:
				cpinfo->DefaultChar[3] = 0x3f;
				cpinfo->MaxCharSize = 4;
				return TRUE;
			}
			fprintf(stderr, "GetCPInfo: bad codepage %u\n", codepage);
			WINPORT(SetLastError)( ERROR_INVALID_PARAMETER );
			return FALSE;
		}
		if (table->info.def_char & 0xff00)
		{
			cpinfo->DefaultChar[0] = (table->info.def_char & 0xff00) >> 8;
			cpinfo->DefaultChar[1] = table->info.def_char & 0x00ff;
		}
		else
		{
			cpinfo->DefaultChar[0] = table->info.def_char & 0xff;
			cpinfo->DefaultChar[1] = 0;
		}
		if ((cpinfo->MaxCharSize = table->info.char_size) == 2)
			memcpy( cpinfo->LeadByte, table->dbcs.lead_bytes, sizeof(cpinfo->LeadByte) );
		else
			cpinfo->LeadByte[0] = cpinfo->LeadByte[1] = 0;

		return TRUE;
	}

	WINPORT_DECL(GetCPInfoEx, BOOL, (UINT codepage, DWORD dwFlags, LPCPINFOEX cpinfo))
	{
		if (!WINPORT(GetCPInfo)( codepage, (LPCPINFO)cpinfo ))
			return FALSE;

		switch(codepage) {
			case CP_UTF7:
			{
				static const WCHAR utf7[] = {'U','n','i','c','o','d','e',' ','(','U','T','F','-','7',')',0};

				cpinfo->CodePage = CP_UTF7;
				cpinfo->UnicodeDefaultChar = 0x3f;
				strcpyW(cpinfo->CodePageName, utf7);
				break;
			}

			case CP_UTF8:
			{
				static const WCHAR utf8[] = {'U','n','i','c','o','d','e',' ','(','U','T','F','-','8',')',0};

				cpinfo->CodePage = CP_UTF8;
				cpinfo->UnicodeDefaultChar = 0x3f;
				strcpyW(cpinfo->CodePageName, utf8);
				break;
			}

			case CP_UTF16LE:
			{
				cpinfo->CodePage = CP_UTF16LE;
				cpinfo->UnicodeDefaultChar = 0x3f;
				wcscpy(cpinfo->CodePageName, L"Unicode (UTF-16 LE)");
				break;
			}

			case CP_UTF16BE:
			{
				cpinfo->CodePage = CP_UTF16BE;
				cpinfo->UnicodeDefaultChar = 0x003f;
				wcscpy(cpinfo->CodePageName, L"Unicode (UTF-16 BE)");
				break;
			}

			case CP_UTF32LE:
			{
				cpinfo->CodePage = CP_UTF32LE;
				cpinfo->UnicodeDefaultChar = 0x3f;
				wcscpy(cpinfo->CodePageName, L"Unicode (UTF-32 LE)");
				break;
			}

			case CP_UTF32BE:
			{
				cpinfo->CodePage = CP_UTF32BE;
				cpinfo->UnicodeDefaultChar = 0x0000003f;
				wcscpy(cpinfo->CodePageName, L"Unicode (UTF-32 BE)");
				break;
			}

			default:
			{
				const union cptable *table = get_codepage_table( codepage );
				if (!table)
					return FALSE;

				cpinfo->CodePage = table->info.codepage;
				cpinfo->UnicodeDefaultChar = table->info.def_unicode_char;
				WINPORT(MultiByteToWideChar)(CP_ACP, 0, table->info.name, -1, cpinfo->CodePageName,
					sizeof(cpinfo->CodePageName)/sizeof(WCHAR));
				break;
			}
		}
		return TRUE;
	}

	WINPORT_DECL(EnumSystemCodePages, BOOL, (CODEPAGE_ENUMPROCW lpfnCodePageEnum, DWORD flags))
	{
		const union cptable *table;
		WCHAR buffer[10], *p;
		int page, index = 0;
		for (;;)
		{
			if (!(table = wine_cp_enum_table( index++ ))) break;
			p = buffer + sizeof(buffer)/sizeof(WCHAR);
			*--p = 0;
			page = table->info.codepage;
			do {
				*--p = '0' + (page % 10);
				page /= 10;
			} while( page );
			if (!lpfnCodePageEnum( p )) break;
		}
		return TRUE;
	}
}

