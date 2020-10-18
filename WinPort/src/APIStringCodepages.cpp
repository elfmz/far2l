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

#include <iconv.h>

#define UTF8_INTERNAL_IMPLEMENTATION

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

template <class CODEUNIT_SRC, class CODEUNIT_DST>
	static int iconv_do(iconv_t cd, unsigned int flags,
		const CODEUNIT_SRC *src_cu, size_t srclen, CODEUNIT_DST *dst_cu, size_t dstlen,
		CODEUNIT_DST replacement_char = (CODEUNIT_DST)UNI_REPLACEMENT_CHAR, LPBOOL replacement_used = nullptr)
{
	char *src = (char *)src_cu;
	char *dst = (char *)dst_cu;

	std::unique_ptr<char[]> tmp_dst;
	if (!dst && srclen) {
		dstlen = srclen;
		if (sizeof(CODEUNIT_DST) < sizeof(CODEUNIT_SRC)) {
			dstlen*= 6;
		}
		tmp_dst.reset( new char[dstlen * sizeof(CODEUNIT_DST)]);
		dst = tmp_dst.get();
	}

	srclen*= sizeof(CODEUNIT_SRC);
	dstlen*= sizeof(CODEUNIT_DST);

	char *dst_orig = dst;
	int res = iconv(cd, &src, &srclen, &dst, &dstlen);
	if (res >= 0) {
		return (dst - dst_orig) / sizeof(CODEUNIT_DST);
	}

	if (errno == E2BIG) {
		return (srclen == 0) ? 0 : -1;
	}

	if (flags & MB_ERR_INVALID_CHARS) {
		return -2;
	}

	if (replacement_used) {
		*replacement_used = TRUE;
	}

	do {
		if (srclen < sizeof(CODEUNIT_SRC)) {
			break;
		}
		if (dstlen < sizeof(CODEUNIT_DST)) {
			return -1;
		}

		memcpy(dst, &replacement_char, sizeof(CODEUNIT_DST));
		src++;
		dst+= sizeof(CODEUNIT_DST);
		srclen-= sizeof(CODEUNIT_SRC);
		dstlen-= sizeof(CODEUNIT_DST);

		if (srclen < sizeof(CODEUNIT_SRC)) {
			break;
		}
		if (dstlen < sizeof(CODEUNIT_DST)) {
			return -1;
		}

		res = iconv(cd, &src, &srclen, &dst, &dstlen);
	} while (res < 0);
		
	return (dst - dst_orig) / sizeof(CODEUNIT_DST);
}


#if (__WCHAR_MAX__ > 0xffff)
# define CP_ICONV_WCHAR "UTF-32LE" // //IGNORE
#else
# define CP_ICONV_WCHAR "UTF-15LE" // //IGNORE
#endif

thread_local char g_iconv_codepage_name[20];

static const char *iconv_codepage_name(unsigned int page)
{
	switch (page) {
       		case CP_KOI8R: return "CP_KOI8R-7";
       		case CP_UTF8:  return "UTF-8";
       		case CP_UTF7:  return "UTF-7";
		default: {
			sprintf(g_iconv_codepage_name, "CP%d", page);
			return g_iconv_codepage_name;
		}
	}
}

static int iconv_cp_mbstowcs(unsigned int page, unsigned int flags,
	const char *src, size_t srclen, wchar_t *dst, size_t dstlen )
{
	// iconv init
	iconv_t cd = iconv_open(CP_ICONV_WCHAR, iconv_codepage_name(page));

	if (cd == (iconv_t)-1) {
		fprintf(stderr, "iconv_cp_mbstowcs(%u): error %d\n", page, errno);
		WINPORT(SetLastError)( ERROR_INVALID_PARAMETER );
		return 0;
	}

	int ir = iconv_do<char, wchar_t>(cd, flags, src, srclen, dst, dstlen);

	iconv_close(cd);

	return ir;
}

static int iconv_cp_wcstombs(unsigned int page, unsigned int flags,
	const wchar_t *src, size_t srclen,  char *dst, size_t dstlen,
	char replacement_char, LPBOOL replacement_used)
{
	iconv_t cd = iconv_open(iconv_codepage_name(page), CP_ICONV_WCHAR);

	if (cd == (iconv_t)-1) {
		fprintf(stderr, "iconv_cp_wcstombs(%u): error %d\n", page, errno);
		WINPORT(SetLastError)( ERROR_INVALID_PARAMETER );
		return 0;
	}

	int ir = iconv_do<wchar_t, char>(cd, flags, src, srclen, dst, dstlen, replacement_char, replacement_used);

	iconv_close(cd);

	return ir;
}

///////////////////////////////////

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

	fprintf(stderr, "DeduceCodepages: unknown locale '%s'\n", lc);

	return Codepages{866, 1251};
}

static UINT TranslateCodepage(UINT codepage)
{
	static Codepages s_cp = DeduceCodepages();
	switch (codepage) {
		case CP_ACP: case CP_THREAD_ACP: return s_cp.ansi;
		case CP_OEMCP: return s_cp.oem;
		default:
			return codepage;
	}
}


extern "C" {
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
		int ret;
		page = TranslateCodepage(page);

		if (!src || !srclen || (!dst && dstlen) || dstlen < 0)
		{
			WINPORT(SetLastError)( ERROR_INVALID_PARAMETER );
			return 0;
		}

		if (srclen < 0)
			srclen = strlen(src) + 1;

		WINPORT(SetLastError)( ERROR_SUCCESS );
		switch(page)
		{
		case CP_SYMBOL: case CP_MACCP:
			WINPORT(SetLastError)( ERROR_INVALID_PARAMETER );
			return 0;

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

#if defined(UTF8_INTERNAL_IMPLEMENTATION) && (__WCHAR_MAX__ > 0xffff)
		case CP_UTF8:
			ret = utf32_utf8_mbstowcs( flags, src, srclen, dst, dstlen);
			break;
#endif
		default:
			ret = iconv_cp_mbstowcs( page, flags, src, srclen, dst, dstlen );
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

    	//fprintf(stderr, "MultiByteToWideChar, %d --> wchar_t\n", trcp);

        // calculating source length
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
		int ret;
		page = TranslateCodepage(page);

		if (!src || !srclen || (!dst && dstlen) || dstlen < 0)
		{
			WINPORT(SetLastError)( ERROR_INVALID_PARAMETER );
			return 0;
		}
		
		if (srclen < 0) srclen = wcslen(src) + 1;

		WINPORT(SetLastError)( ERROR_SUCCESS );
		switch(page)
		{
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

#if defined(UTF8_INTERNAL_IMPLEMENTATION) && (__WCHAR_MAX__ > 0xffff)
		case CP_UTF8:
			if (defchar || used)
			{
				//abort();
				fprintf(stderr, "WideCharToMultiByte: 'defchar' and 'used' can't be used with UTF8\n");
				WINPORT(SetLastError)( ERROR_INVALID_PARAMETER );
				return 0;
			}

			ret = utf32_utf8_wcstombs( flags, src, srclen, dst, dstlen );
			break;
#endif
		default:
			ret = iconv_cp_wcstombs( page, flags, src, srclen,
				dst, dstlen, defchar ? *defchar : UNI_REPLACEMENT_CHAR, used);
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
		if (!cpinfo)
		{
			WINPORT(SetLastError)( ERROR_INVALID_PARAMETER );
			return FALSE;
		}
		memset(cpinfo, 0, sizeof(*cpinfo));

		codepage = TranslateCodepage(codepage);

		switch(codepage) {
			case CP_UTF7:
			case CP_UTF8:
				cpinfo->DefaultChar[0] = UNI_REPLACEMENT_CHAR;
				cpinfo->MaxCharSize = (codepage == CP_UTF7) ? 5 : 4;
				return TRUE;

			case CP_UTF16LE: 
				cpinfo->DefaultChar[0] = UNI_REPLACEMENT_CHAR;
				cpinfo->MaxCharSize = 2;
				return TRUE;

			case CP_UTF16BE:
				cpinfo->DefaultChar[1] = UNI_REPLACEMENT_CHAR;
				cpinfo->MaxCharSize = 2;
				return TRUE;
				
			case CP_UTF32LE: 
				cpinfo->DefaultChar[0] = UNI_REPLACEMENT_CHAR;
				cpinfo->MaxCharSize = 4;
				return TRUE;

			case CP_UTF32BE:
				cpinfo->DefaultChar[3] = UNI_REPLACEMENT_CHAR;
				cpinfo->MaxCharSize = 4;
				return TRUE;
		}

		cpinfo->DefaultChar[0] = UNI_REPLACEMENT_CHAR;
		cpinfo->MaxCharSize = 1;
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
				cpinfo->UnicodeDefaultChar = UNI_REPLACEMENT_CHAR;
				strcpyW(cpinfo->CodePageName, utf7);
				break;
			}

			case CP_UTF8:
			{
				static const WCHAR utf8[] = {'U','n','i','c','o','d','e',' ','(','U','T','F','-','8',')',0};

				cpinfo->CodePage = CP_UTF8;
				cpinfo->UnicodeDefaultChar = UNI_REPLACEMENT_CHAR;
				strcpyW(cpinfo->CodePageName, utf8);
				break;
			}

			case CP_UTF16LE:
			{
				cpinfo->CodePage = CP_UTF16LE;
				cpinfo->UnicodeDefaultChar = UNI_REPLACEMENT_CHAR;
				wcscpy(cpinfo->CodePageName, L"Unicode (UTF-16 LE)");
				break;
			}

			case CP_UTF16BE:
			{
				cpinfo->CodePage = CP_UTF16BE;
				cpinfo->UnicodeDefaultChar = UNI_REPLACEMENT_CHAR;
				wcscpy(cpinfo->CodePageName, L"Unicode (UTF-16 BE)");
				break;
			}

			case CP_UTF32LE:
			{
				cpinfo->CodePage = CP_UTF32LE;
				cpinfo->UnicodeDefaultChar = UNI_REPLACEMENT_CHAR;
				wcscpy(cpinfo->CodePageName, L"Unicode (UTF-32 LE)");
				break;
			}

			case CP_UTF32BE:
			{
				cpinfo->CodePage = CP_UTF32BE;
				cpinfo->UnicodeDefaultChar = UNI_REPLACEMENT_CHAR;
				wcscpy(cpinfo->CodePageName, L"Unicode (UTF-32 BE)");
				break;
			}

			default:
			{
                return TRUE;
				/*
                const union cptable *table = get_codepage_table( codepage );
				if (!table)
					return FALSE;

				cpinfo->CodePage = table->info.codepage;
				cpinfo->UnicodeDefaultChar = table->info.def_unicode_char;
				WINPORT(MultiByteToWideChar)( CP_ACP, 0, table->info.name, -1, cpinfo->CodePageName,
                                 sizeof(cpinfo->CodePageName)/sizeof(WCHAR));
                */
				break;
			}
		}
		return TRUE;
	}

	WINPORT_DECL(EnumSystemCodePages, BOOL, (CODEPAGE_ENUMPROCW lpfnCodePageEnum, DWORD flags))
	{
        return TRUE; //FIXME

	    const union cptable *table;
	    WCHAR buffer[10], *p;
	    int page, index = 0;
	    for (;;)
	    {
        	//if (!(table = wine_cp_enum_table( index++ ))) break;
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

