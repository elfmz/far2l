#include <set>
#include <string>
#include <locale> 
#include <algorithm>
#include <iostream>
#include <fstream>
#include <mutex>

#include <locale.h>

#if !defined(__APPLE__) && !defined(__FreeBSD__)
# include <alloca.h>
#endif

#include "WinCompat.h"
#include "WinPort.h"
#include "wineguts.h"
#include "PathHelpers.h"
#include "ConvertUTF.h"



template <class SRC_T, class DST_T>
	int utf_translation( 
			ConversionResult (* fnCalcSpace) (int *out, const SRC_T** src, const SRC_T* src_end, ConversionFlags flags),
			ConversionResult (* fnConvert) (const SRC_T** src, const SRC_T* src_end, DST_T** dst, DST_T* dst_end, ConversionFlags flags),
			int flags, const SRC_T *src, int srclen, DST_T *dst, int dstlen)
{
	int ret;
	const ConversionFlags cf = ((flags&MB_ERR_INVALID_CHARS)!=0) ? strictConversion : lenientConversion;
	const SRC_T *source = (const SRC_T *)src, *source_end = (const SRC_T *)src;
	if (srclen==-1) {
		for(;*source_end;++source_end);
	} else {
		for(;srclen;++source_end, --srclen);
	}

	if (dstlen==0) {
		if (fnCalcSpace (&ret, &source, source_end, cf)!=conversionOK) {
			WINPORT(SetLastError)( ERROR_NO_UNICODE_TRANSLATION ); 
		}
		
	} else {
		DST_T *target = (DST_T *)dst;
		DST_T *target_end = target + dstlen;
		
		ConversionResult cr = fnConvert(&source, source_end, &target, target_end, cf);
		if (cr==targetExhausted) {
			ret = 0;
			WINPORT(SetLastError)( ERROR_INSUFFICIENT_BUFFER );
		} else {
			ret = target - (DST_T *)dst;
			if (cr!=conversionOK) {
				WINPORT(SetLastError)( ERROR_NO_UNICODE_TRANSLATION ); 
			}
		}
	}
	return ret;		
}

static int utf32_utf8_wcstombs( int flags, const WCHAR *src, int srclen, char *dst, int dstlen)
{
	return utf_translation<UTF32, UTF8>( CalcSpaceUTF32toUTF8, ConvertUTF32toUTF8,
		flags, (const UTF32 *)src, srclen, (UTF8 *)dst, dstlen);
}
	
static int utf32_utf8_mbstowcs( int flags, const char *src, int srclen, WCHAR *dst, int dstlen)
{
	return utf_translation<UTF8, UTF32>( CalcSpaceUTF8toUTF32, ConvertUTF8toUTF32,
		flags, (const UTF8 *)src, srclen, (UTF32 *)dst, dstlen);
}
	
static int wide_cvtstub( int flags, const wchar_t *src, int srclen, wchar_t *dst, int dstlen)
{
	if (srclen==-1)
		srclen = wcslen(src) + 1;
		
	if (dstlen != 0) {
		if (dstlen < srclen)
			return -1;
			
		memcpy(dst, src, srclen * sizeof(WCHAR));
	}
	return srclen;
}
	
static int wide_utf16_wcstombs( int flags, const wchar_t *src, int srclen, char *dst, int dstlen, bool reverse)
{
	int ret;
	
	if (dstlen > 0) dstlen/= sizeof(UTF16);
	
	if (sizeof(WCHAR)==4) {
		ret = utf_translation<UTF32, UTF16>( CalcSpaceUTF32toUTF16, ConvertUTF32toUTF16,
									flags, (const UTF32 *)src, srclen, (UTF16 *)dst, dstlen);
	} else
		ret = wide_cvtstub( flags, src, srclen, (wchar_t *)dst, dstlen);
	
	if (ret > 0)  {
		if (reverse && dstlen > 0) {
			for (int i = 0; i < ret; ++i) {
				std::swap(dst[i * 2], dst[i * 2 + 1]);
			}
		}		
		ret*= sizeof(UTF16);
	}
	
	return ret;
}

static int wide_utf32_wcstombs( int flags, const wchar_t *src, int srclen, char *dst, int dstlen, bool reverse)
{
	dstlen/= sizeof(wchar_t);
	int ret = wide_cvtstub( flags, src, srclen, (wchar_t *)dst, dstlen);
	if (ret > 0)  {
		if (reverse && dstlen > 0) {
			for (int i = 0; i < ret; ++i) {
				std::swap(dst[i * 4], dst[i * 4 + 3]);
				std::swap(dst[i * 4 + 1], dst[i * 4 + 2]);
			}
		}
		ret*= sizeof(wchar_t);
	}
	return ret;
}

static int wide_utf32_mbstowcs( int flags, const char *src, int srclen, wchar_t *dst, int dstlen, bool reverse)
{
	if (srclen > 0) srclen/= sizeof(wchar_t);
	int ret = wide_cvtstub( flags, (const wchar_t *)src, srclen, dst, dstlen);
	if (ret > 0)  {
		if (reverse && dstlen > 0) {
			char *dst_as_chars = (char *)dst;
			for (int i = 0; i < ret; ++i) {				
				std::swap(dst_as_chars[i * 4], dst_as_chars[i * 4 + 3]);
				std::swap(dst_as_chars[i * 4 + 1], dst_as_chars[i * 4 + 2]);
			}
		}
	}
	return ret;
}


static int wide_utf16_mbstowcs( int flags, const char *src, int srclen, WCHAR *dst, int dstlen, bool reverse)
{
	int ret;
	
	if (srclen > 0) srclen/= sizeof(UTF16);
	
	char *tmp = NULL;
	if (reverse) {
		if (srclen==-1) srclen = wcslen((const wchar_t *)src) + 1;
		
		const bool onstack = (srclen < 0x10000);
		tmp = (char *) (onstack ? alloca(srclen * sizeof(UTF16)) : malloc(srclen * sizeof(UTF16)));
			
		if (!tmp) 
			return -2;
			
		for (int i = 0; i < srclen; ++i) {
			tmp[2 * i] = src[2 * i + 1];
			tmp[2 * i + 1] = src[2 * i];
		}
		src = tmp;
		if (onstack) tmp = NULL;
	}
	
	if (sizeof(WCHAR)==4) {
		
		ret = utf_translation<UTF16, UTF32>( CalcSpaceUTF16toUTF32, ConvertUTF16toUTF32,
			flags, (const UTF16 *)src, srclen, (UTF32 *)dst, dstlen);
	} else
		ret = wide_cvtstub( flags, (const wchar_t *)src, srclen, dst, dstlen);
		
	free(tmp);
	return ret;
}

	
extern "C" {
	#define IsLocaleMatches(current, wanted_literal) \
		( strncmp((current), wanted_literal, sizeof(wanted_literal) - 1) == 0 && \
		( (current)[sizeof(wanted_literal) - 1] == 0 || (current)[sizeof(wanted_literal) - 1] == '.') )

	static int DetectOemCP()
	{
		// detect oem cp from system locale
		int oem_cp = 866; // default

		const char *lc = setlocale(LC_CTYPE, NULL);
		if (!lc) { fprintf(stderr, "DetectOemCP: setlocale returned NULL\n"); }
		else if (IsLocaleMatches(lc, "af_ZA")) { oem_cp = 850; }
		else if (IsLocaleMatches(lc, "ar_SA")) { oem_cp = 720; }
		else if (IsLocaleMatches(lc, "ar_LB")) { oem_cp = 720; }
		else if (IsLocaleMatches(lc, "ar_EG")) { oem_cp = 720; }
		else if (IsLocaleMatches(lc, "ar_DZ")) { oem_cp = 720; }
		else if (IsLocaleMatches(lc, "ar_BH")) { oem_cp = 720; }
		else if (IsLocaleMatches(lc, "ar_IQ")) { oem_cp = 720; }
		else if (IsLocaleMatches(lc, "ar_JO")) { oem_cp = 720; }
		else if (IsLocaleMatches(lc, "ar_KW")) { oem_cp = 720; }
		else if (IsLocaleMatches(lc, "ar_LY")) { oem_cp = 720; }
		else if (IsLocaleMatches(lc, "ar_MA")) { oem_cp = 720; }
		else if (IsLocaleMatches(lc, "ar_OM")) { oem_cp = 720; }
		else if (IsLocaleMatches(lc, "ar_QA")) { oem_cp = 720; }
		else if (IsLocaleMatches(lc, "ar_SY")) { oem_cp = 720; }
		else if (IsLocaleMatches(lc, "ar_TN")) { oem_cp = 720; }
		else if (IsLocaleMatches(lc, "ar_AE")) { oem_cp = 720; }
		else if (IsLocaleMatches(lc, "ar_YE")) { oem_cp = 720; }
		else if (IsLocaleMatches(lc, "ast_ES")) { oem_cp = 850; }
		else if (IsLocaleMatches(lc, "az_AZ")) { oem_cp = 866; }
		else if (IsLocaleMatches(lc, "az_AZ")) { oem_cp = 857; }
		else if (IsLocaleMatches(lc, "be_BY")) { oem_cp = 866; }
		else if (IsLocaleMatches(lc, "bg_BG")) { oem_cp = 866; }
		else if (IsLocaleMatches(lc, "br_FR")) { oem_cp = 850; }
		else if (IsLocaleMatches(lc, "ca_ES")) { oem_cp = 850; }
		else if (IsLocaleMatches(lc, "zh_CN")) { oem_cp = 936; }
		else if (IsLocaleMatches(lc, "zh_TW")) { oem_cp = 950; }
		else if (IsLocaleMatches(lc, "kw_GB")) { oem_cp = 850; }
		else if (IsLocaleMatches(lc, "cs_CZ")) { oem_cp = 852; }
		else if (IsLocaleMatches(lc, "cy_GB")) { oem_cp = 850; }
		else if (IsLocaleMatches(lc, "da_DK")) { oem_cp = 850; }
		else if (IsLocaleMatches(lc, "de_AT")) { oem_cp = 850; }
		else if (IsLocaleMatches(lc, "de_LI")) { oem_cp = 850; }
		else if (IsLocaleMatches(lc, "de_LU")) { oem_cp = 850; }
		else if (IsLocaleMatches(lc, "de_CH")) { oem_cp = 850; }
		else if (IsLocaleMatches(lc, "de_DE")) { oem_cp = 850; }
		else if (IsLocaleMatches(lc, "el_GR")) { oem_cp = 737; }
		else if (IsLocaleMatches(lc, "en_AU")) { oem_cp = 850; }
		else if (IsLocaleMatches(lc, "en_CA")) { oem_cp = 850; }
		else if (IsLocaleMatches(lc, "en_GB")) { oem_cp = 850; }
		else if (IsLocaleMatches(lc, "en_IE")) { oem_cp = 850; }
		else if (IsLocaleMatches(lc, "en_JM")) { oem_cp = 850; }
		else if (IsLocaleMatches(lc, "en_BZ")) { oem_cp = 850; }
		else if (IsLocaleMatches(lc, "en_PH")) { oem_cp = 437; }
		else if (IsLocaleMatches(lc, "en_ZA")) { oem_cp = 437; }
		else if (IsLocaleMatches(lc, "en_TT")) { oem_cp = 850; }
		else if (IsLocaleMatches(lc, "en_US")) { oem_cp = 437; }
		else if (IsLocaleMatches(lc, "en_ZW")) { oem_cp = 437; }
		else if (IsLocaleMatches(lc, "en_NZ")) { oem_cp = 850; }
		else if (IsLocaleMatches(lc, "es_PA")) { oem_cp = 850; }
		else if (IsLocaleMatches(lc, "es_BO")) { oem_cp = 850; }
		else if (IsLocaleMatches(lc, "es_CR")) { oem_cp = 850; }
		else if (IsLocaleMatches(lc, "es_DO")) { oem_cp = 850; }
		else if (IsLocaleMatches(lc, "es_SV")) { oem_cp = 850; }
		else if (IsLocaleMatches(lc, "es_EC")) { oem_cp = 850; }
		else if (IsLocaleMatches(lc, "es_GT")) { oem_cp = 850; }
		else if (IsLocaleMatches(lc, "es_HN")) { oem_cp = 850; }
		else if (IsLocaleMatches(lc, "es_NI")) { oem_cp = 850; }
		else if (IsLocaleMatches(lc, "es_CL")) { oem_cp = 850; }
		else if (IsLocaleMatches(lc, "es_MX")) { oem_cp = 850; }
		else if (IsLocaleMatches(lc, "es_ES")) { oem_cp = 850; }
		else if (IsLocaleMatches(lc, "es_CO")) { oem_cp = 850; }
		else if (IsLocaleMatches(lc, "es_ES")) { oem_cp = 850; }
		else if (IsLocaleMatches(lc, "es_PE")) { oem_cp = 850; }
		else if (IsLocaleMatches(lc, "es_AR")) { oem_cp = 850; }
		else if (IsLocaleMatches(lc, "es_PR")) { oem_cp = 850; }
		else if (IsLocaleMatches(lc, "es_VE")) { oem_cp = 850; }
		else if (IsLocaleMatches(lc, "es_UY")) { oem_cp = 850; }
		else if (IsLocaleMatches(lc, "es_PY")) { oem_cp = 850; }
		else if (IsLocaleMatches(lc, "et_EE")) { oem_cp = 775; }
		else if (IsLocaleMatches(lc, "eu_ES")) { oem_cp = 850; }
		else if (IsLocaleMatches(lc, "fa_IR")) { oem_cp = 720; }
		else if (IsLocaleMatches(lc, "fi_FI")) { oem_cp = 850; }
		else if (IsLocaleMatches(lc, "fo_FO")) { oem_cp = 850; }
		else if (IsLocaleMatches(lc, "fr_FR")) { oem_cp = 850; }
		else if (IsLocaleMatches(lc, "fr_BE")) { oem_cp = 850; }
		else if (IsLocaleMatches(lc, "fr_CA")) { oem_cp = 850; }
		else if (IsLocaleMatches(lc, "fr_LU")) { oem_cp = 850; }
		else if (IsLocaleMatches(lc, "fr_MC")) { oem_cp = 850; }
		else if (IsLocaleMatches(lc, "fr_CH")) { oem_cp = 850; }
		else if (IsLocaleMatches(lc, "ga_IE")) { oem_cp = 437; }
		else if (IsLocaleMatches(lc, "gd_GB")) { oem_cp = 850; }
		else if (IsLocaleMatches(lc, "gv_IM")) { oem_cp = 850; }
		else if (IsLocaleMatches(lc, "gl_ES")) { oem_cp = 850; }
		else if (IsLocaleMatches(lc, "he_IL")) { oem_cp = 862; }
		else if (IsLocaleMatches(lc, "hr_HR")) { oem_cp = 852; }
		else if (IsLocaleMatches(lc, "hu_HU")) { oem_cp = 852; }
		else if (IsLocaleMatches(lc, "id_ID")) { oem_cp = 850; }
		else if (IsLocaleMatches(lc, "is_IS")) { oem_cp = 850; }
		else if (IsLocaleMatches(lc, "it_IT")) { oem_cp = 850; }
		else if (IsLocaleMatches(lc, "it_CH")) { oem_cp = 850; }
		else if (IsLocaleMatches(lc, "iv_IV")) { oem_cp = 437; }
		else if (IsLocaleMatches(lc, "ja_JP")) { oem_cp = 932; }
		else if (IsLocaleMatches(lc, "kk_KZ")) { oem_cp = 866; }
		else if (IsLocaleMatches(lc, "ko_KR")) { oem_cp = 949; }
		else if (IsLocaleMatches(lc, "ky_KG")) { oem_cp = 866; }
		else if (IsLocaleMatches(lc, "lt_LT")) { oem_cp = 775; }
		else if (IsLocaleMatches(lc, "lv_LV")) { oem_cp = 775; }
		else if (IsLocaleMatches(lc, "mk_MK")) { oem_cp = 866; }
		else if (IsLocaleMatches(lc, "mn_MN")) { oem_cp = 866; }
		else if (IsLocaleMatches(lc, "ms_BN")) { oem_cp = 850; }
		else if (IsLocaleMatches(lc, "ms_MY")) { oem_cp = 850; }
		else if (IsLocaleMatches(lc, "nl_BE")) { oem_cp = 850; }
		else if (IsLocaleMatches(lc, "nl_NL")) { oem_cp = 850; }
		else if (IsLocaleMatches(lc, "nl_SR")) { oem_cp = 850; }
		else if (IsLocaleMatches(lc, "nn_NO")) { oem_cp = 850; }
		else if (IsLocaleMatches(lc, "nb_NO")) { oem_cp = 850; }
		else if (IsLocaleMatches(lc, "pl_PL")) { oem_cp = 852; }
		else if (IsLocaleMatches(lc, "pt_BR")) { oem_cp = 850; }
		else if (IsLocaleMatches(lc, "pt_PT")) { oem_cp = 850; }
		else if (IsLocaleMatches(lc, "rm_CH")) { oem_cp = 850; }
		else if (IsLocaleMatches(lc, "ro_RO")) { oem_cp = 852; }
		else if (IsLocaleMatches(lc, "ru_RU")) { oem_cp = 866; }
		else if (IsLocaleMatches(lc, "sk_SK")) { oem_cp = 852; }
		else if (IsLocaleMatches(lc, "sl_SI")) { oem_cp = 852; }
		else if (IsLocaleMatches(lc, "sq_AL")) { oem_cp = 852; }
		else if (IsLocaleMatches(lc, "sr_RS")) { oem_cp = 855; }
		else if (IsLocaleMatches(lc, "sr_RS")) { oem_cp = 852; }
		else if (IsLocaleMatches(lc, "sv_SE")) { oem_cp = 850; }
		else if (IsLocaleMatches(lc, "sv_FI")) { oem_cp = 850; }
		else if (IsLocaleMatches(lc, "sw_KE")) { oem_cp = 437; }
		else if (IsLocaleMatches(lc, "th_TH")) { oem_cp = 874; }
		else if (IsLocaleMatches(lc, "tr_TR")) { oem_cp = 857; }
		else if (IsLocaleMatches(lc, "tt_RU")) { oem_cp = 866; }
		else if (IsLocaleMatches(lc, "uk_UA")) { oem_cp = 866; }
		else if (IsLocaleMatches(lc, "ur_PK")) { oem_cp = 720; }
		else if (IsLocaleMatches(lc, "uz_UZ")) { oem_cp = 866; }
		else if (IsLocaleMatches(lc, "uz_UZ")) { oem_cp = 857; }
		else if (IsLocaleMatches(lc, "vi_VN")) { oem_cp = 1258; }
		else if (IsLocaleMatches(lc, "wa_BE")) { oem_cp = 850; }
		else if (IsLocaleMatches(lc, "zh_HK")) { oem_cp = 950; }
		else if (IsLocaleMatches(lc, "zh_SG")) { oem_cp = 936; }
		else if (IsLocaleMatches(lc, "zh_MO")) { oem_cp = 950; }
		else {
			fprintf(stderr, "DetectOemCP: setlocale returned unexpected '%s'\n", lc);
		}

		fprintf(stderr, "DetectOemCP: returning %u\n", oem_cp);

		return oem_cp;
	}

	static UINT TranslateCodepage(UINT codepage)
	{
		switch (codepage) {
			case CP_ACP: return 1251; // TODO: detect system locale specific ACP in same way as OEMCP
			case CP_OEMCP: {
				static int s_oemcp = DetectOemCP();
				return s_oemcp;
			}
		}
		return codepage;
	}

	WINPORT_DECL(IsTextUnicode, BOOL, (CONST VOID* buf, int len, LPINT pf))
	{//borrowed from wine
		static const WCHAR std_control_chars[] = {'\r','\n','\t',' ',0x3000,0};
		static const WCHAR byterev_control_chars[] = {0x0d00,0x0a00,0x0900,0x2000,0};
		const WCHAR *s = (const WCHAR *)buf;
		int i;
		unsigned int flags = ~0U, out_flags = 0;

		if (len < (int)sizeof(WCHAR))
		{
			/* FIXME: MSDN documents IS_TEXT_UNICODE_BUFFER_TOO_SMALL but there is no such thing... */
			if (pf) *pf = 0;
			return FALSE;
		}
		if (pf)
			flags = *pf;
		/*
		* Apply various tests to the text string. According to the
		* docs, each test "passed" sets the corresponding flag in
		* the output flags. But some of the tests are mutually
		* exclusive, so I don't see how you could pass all tests ...
		*/

		/* Check for an odd length ... pass if even. */
		if (len & 1) out_flags |= IS_TEXT_UNICODE_ODD_LENGTH;

		if (((const char *)buf)[len - 1] == 0)
			len--;  /* Windows seems to do something like that to avoid e.g. false IS_TEXT_UNICODE_NULL_BYTES  */

		len /= sizeof(WCHAR);
		/* Windows only checks the first 256 characters */
		if (len > 256) len = 256;

		/* Check for the special byte order unicode marks. */
		if (*s == 0xFEFF) out_flags |= IS_TEXT_UNICODE_SIGNATURE;
		if (*s == 0xFFFE) out_flags |= IS_TEXT_UNICODE_REVERSE_SIGNATURE;

		/* apply some statistical analysis */
		if (flags & IS_TEXT_UNICODE_STATISTICS)
		{
			int stats = 0;
			/* FIXME: checks only for ASCII characters in the unicode stream */
			for (i = 0; i < len; i++)
			{
				if (s[i] <= 255) stats++;
			}
			if (stats > len / 2)
				out_flags |= IS_TEXT_UNICODE_STATISTICS;
		}

		/* Check for unicode NULL chars */
		if (flags & IS_TEXT_UNICODE_NULL_BYTES)
		{
			for (i = 0; i < len; i++)
			{
				if (!(s[i] & 0xff) || !(s[i] >> 8))
				{
					out_flags |= IS_TEXT_UNICODE_NULL_BYTES;
					break;
				}
			}
		}

		if (flags & IS_TEXT_UNICODE_CONTROLS)
		{
			for (i = 0; i < len; i++)
			{
				if (wcschr(std_control_chars, s[i]))
				{
					out_flags |= IS_TEXT_UNICODE_CONTROLS;
					break;
				}
			}
		}

		if (flags & IS_TEXT_UNICODE_REVERSE_CONTROLS)
		{
			for (i = 0; i < len; i++)
			{
				if (wcschr(byterev_control_chars, s[i]))
				{
					out_flags |= IS_TEXT_UNICODE_REVERSE_CONTROLS;
					break;
				}
			}
		}

		if (pf)
		{
			out_flags &= *pf;
			*pf = out_flags;
		}
		/* check for flags that indicate it's definitely not valid Unicode */
		if (out_flags & (IS_TEXT_UNICODE_REVERSE_MASK | IS_TEXT_UNICODE_NOT_UNICODE_MASK)) return FALSE;
		/* now check for invalid ASCII, and assume Unicode if so */
		if (out_flags & IS_TEXT_UNICODE_NOT_ASCII_MASK) return TRUE;
		/* now check for Unicode flags */
		if (out_flags & IS_TEXT_UNICODE_UNICODE_MASK) return TRUE;
		/* no flags set */
		return FALSE;
	}




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
			ret = wide_utf16_mbstowcs( flags, src, srclen, dst, dstlen, false );
			break;

		case CP_UTF16BE:
			ret = wide_utf16_mbstowcs( flags, src, srclen, dst, dstlen, true );
			break;

		case CP_UTF32LE:
			ret = wide_utf32_mbstowcs( flags, src, srclen, dst, dstlen, false );
			break;

		case CP_UTF32BE:
			ret = wide_utf32_mbstowcs( flags, src, srclen, dst, dstlen, true );
			break;

		case CP_UTF8:
			if (sizeof(wchar_t)==4) {
				ret = utf32_utf8_mbstowcs( flags, src, srclen, dst, dstlen);
			} else {
				ret = wine_utf8_mbstowcs( flags, src, srclen, dst, dstlen );
			}
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
			ret = wide_utf16_wcstombs( flags, src, srclen, dst, dstlen, false );
			break;

		case CP_UTF16BE:
			ret = wide_utf16_wcstombs( flags, src, srclen, dst, dstlen, true );
			break;

		case CP_UTF32LE:
			ret = wide_utf32_wcstombs( flags, src, srclen, dst, dstlen, false );
			break;

		case CP_UTF32BE:
			ret = wide_utf32_wcstombs( flags, src, srclen, dst, dstlen, true );
			break;

		case CP_UTF8:
			if (defchar || used)
			{
				//abort();
				fprintf(stderr, "WideCharToMultiByte: 'defchar' and 'used' can't be used with UTF8\n");
				WINPORT(SetLastError)( ERROR_INVALID_PARAMETER );
				return 0;
			}
			
			if (sizeof(wchar_t)==4) {
				ret = utf32_utf8_wcstombs( flags, src, srclen, dst, dstlen );
			} else {
				ret = wine_utf8_wcstombs( flags, src, srclen, dst, dstlen );
			}
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
				WINPORT(MultiByteToWideChar)( CP_ACP, 0, table->info.name, -1, cpinfo->CodePageName,
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

