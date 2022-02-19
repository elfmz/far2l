#include <strings.h>

#include <WinCompat.h>
#include "../WinPort/WinPort.h"

#include "DetectCodepage.h"

#ifdef USEUCD
# include <uchardet.h>

static bool IsDecimalNumber(const char *s)
{
	for (;*s;++s) {
		if (*s < '0' || *s > '9') {
			return false;
		}
	}
	return true;
}

static int TranslateUDCharset(const char *cs)
{
	if (strncasecmp(cs, "windows-", 8) == 0) {
		if (IsDecimalNumber(cs + 8)) {
			return atoi(cs + 8);
		}
		if (strcasecmp(cs + 8, "31j") == 0) {
				return 932;
		}
	}

	if (strncasecmp(cs, "CP", 2) == 0 && IsDecimalNumber(cs + 2)) {
		return atoi(cs + 2);
	}

	if (strncasecmp(cs, "IBM", 3) == 0 && IsDecimalNumber(cs + 3)) {
		return atoi(cs + 3);
	}

	if (!strcasecmp(cs, "UTF16-LE") || !strcasecmp(cs, "UTF16"))
		return CP_UTF16LE;
	if (!strcasecmp(cs, "UTF16-BE"))
		return CP_UTF16BE;
	if (!strcasecmp(cs, "UTF32-LE") || !strcasecmp(cs, "UTF32"))
		return CP_UTF32LE;
	if (!strcasecmp(cs, "UTF32-BE"))
		return CP_UTF32BE;
	if (!strcasecmp(cs, "UTF-8"))
		return CP_UTF8;
	if (!strcasecmp(cs, "UTF-7"))
		return CP_UTF7;
//	if (!strcasecmp(cs, "IBM855"))
//		return 855;
//	if (!strcasecmp(cs, "IBM866"))
//		return 866;
	if (!strcasecmp(cs, "KOI8-R"))
		return 20866;
	if (!strcasecmp(cs, "KOI8-U"))
		return 21866;
	if (!strcasecmp(cs, "x-mac-hebrew") || !strcasecmp(cs, "MS-MAC-HEBREW"))
		return 10005;
	if (!strcasecmp(cs, "x-mac-cyrillic") || !strcasecmp(cs, "MS-MAC-CYRILLIC"))
		return 10007;
	if (!strcasecmp(cs, "ISO-8859-2"))
		return 28592;
	if (!strcasecmp(cs, "ISO-8859-5"))
		return 28595;
	if (!strcasecmp(cs, "ISO-8859-7"))
		return 28597;
	if (!strcasecmp(cs, "ISO-8859-8"))
		return 28598;
	if (!strcasecmp(cs, "ISO-8859-8-I"))
		return 38598;
	if (!strcasecmp(cs, "EUC-JP"))
		return 20932;

	fprintf(stderr, "TranslateUDCharset: unknown charset '%s'\n", cs);

	/*
		and the rest:
		"Shift_JIS"
		"gb18030"
		"x-euc-tw"
		"EUC-KR"
		"EUC-JP"
		"Big5"
		"X-ISO-10646-UCS-4-3412" - UCS-4, unusual octet order BOM (3412)
		"UTF-32BE"
		"X-ISO-10646-UCS-4-2143" - UCS-4, unusual octet order BOM (2143)
		"UTF-32LE"
		ISO-2022-CN
		ISO-2022-JP
		ISO-2022-KR
		"TIS-620"
		*/
	return -1;
}

int DetectCodePage(const char *data, size_t len)
{
	uchardet_t ud = uchardet_new();
	uchardet_handle_data(ud, data, len);
	uchardet_data_end(ud);
	const char *cs = uchardet_get_charset(ud);
	int out = cs ? TranslateUDCharset(cs) : -1;
//	fprintf(stderr, "DetectCodePage: '%s' -> %d\n", cs, out);
	uchardet_delete(ud);
	return out;
}

#else
int DetectCodePage(const char *data, size_t len)
{
	return -1;
}
#endif
