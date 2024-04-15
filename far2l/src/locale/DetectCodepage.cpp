#include <strings.h>
#include <cstring>
#include <map>

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

static int CheckForEncodedInName(const char *cs)
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
		cs+= 2;
	} else if (strncasecmp(cs, "IBM", 3) == 0 && IsDecimalNumber(cs + 3)) {
		cs+= 3;
	} else {
		return -1;
	}

	int r = atoi(cs);
	if (r == 878) {		// IBM KOI8-R
		return 20866;	// MS KOI8-R
	}

	return r;
}

static int CheckForHardcodedByName(const char *cs)
{
	struct cmp_str
	{
		bool operator()(char const *a, char const *b) const
		{
			return std::strcmp(a, b) < 0;
		}
	};

	std::map<const char*, int, cmp_str> encodings
		{
			{"UTF-16",CP_UTF16LE},
			{"UTF-32",CP_UTF32LE},
			{"UTF-8",CP_UTF8},
			{"ASCII",CP_UTF8},				// treat ASCII as UTF-8 (better in the editor)
			{"ISO-8859-1",28591},			// Latin 1; Western European
			{"ISO-8859-2",28592},			// Latin 2; Central European
			{"ISO-8859-3",28593},			// Latin 3; South European
			{"ISO-8859-4",28594},			// Latin 4; Baltic
			{"ISO-8859-5",28595},			// Cyrillic
			{"ISO-8859-6",28596},			// Arabic
			{"ISO-8859-7",28597},			// Greek
			{"ISO-8859-8",28598},			// Hebrew
			{"ISO-8859-9",28599},			// Latin-5; Turkish
			{"ISO-8859-10",28600},			// Latin-6; Nordic
			{"ISO-8859-11",874},			// Thai (in fact, it's 28601 but it's not supported by far2l)
			{"ISO-8859-13",28603},			// Latin-7; Baltic Rim (Estonian)
			{"ISO-8859-14",28604},			// Latin-8; iso-celtic
			{"ISO-8859-15",28605},			// Latin-9; Western European
			{"ISO-8859-16",28606},			// Latin-10; South-Eastern European
			{"TIS-620",874},				// Thai
			{"MAC-CYRILLIC",10007},			// Cyrillic (Mac)
			{"MAC-CENTRALEUROPE",10029},	// Mac OS Central European
			{"KOI8-R",20866},				// Cyrillic
			{"EUC-JP",20932},				// Japanese
			{"ISO-2022-JP",50220},			// Japanese
			{"ISO-2022-CN",50227},			// Chinese Simplified
			{"Johab",1361},					// Korean
			{"SHIFT_JIS",932},				// Japanese
			{"EUC-KR",51949},				// Korean
			{"UHC",949},					// Korean
			{"ISO-2022-KR",50225},			// Korean
			{"BIG5",950},					// Traditional Chinese
			{"GB18030",54936}				// Chinese Simplified
		};

	// the rest:
	// EUC-TW, GEORGIAN-ACADEMY, GEORGIAN-PS, HZ-GB-2312, VISCII

	auto r= encodings.find(cs);
	return r==encodings.end() ? -1 : r->second;
}

static int TranslateUDCharset(const char *cs)
{
	int r = CheckForEncodedInName(cs);
	if (r == -1)
		r = CheckForHardcodedByName(cs);
	if (r == -1)
		fprintf(stderr, "TranslateUDCharset: unknown charset '%s'\n", cs);

	return r;
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
