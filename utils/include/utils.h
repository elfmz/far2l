#pragma once
#include <string>
#include <vector>
#include <cstdarg>
#include <sys/types.h>
#include "cctweaks.h"
#include "MatchWildcard.hpp"
#include "WideMB.h"
#include "Escaping.h"
#include "Environment.h"
#include "ErrnoSaver.hpp"
#include "PlatformConstants.h"

#define MAKE_STR(x) _MAKE_STR(x)
#define _MAKE_STR(x) #x

template <class C> static size_t tzlen(const C *ptz)
{
	const C *etz;
	for (etz = ptz; *etz; ++etz);
	return (etz - ptz);
}

template <class C> static size_t tnzlen(const C *ptz, size_t n)
{
	size_t i;
	for (i = 0; i < n && ptz[i]; ++i);
	return i;
}


unsigned long htoul(const char *str, size_t maxlen = (size_t)-1);
unsigned long atoul(const char *str, size_t maxlen = (size_t)-1);

// converts given hex digit to value between 0x0 and 0xf
// in case of error returns 0xff
template <class CHAR_T>
	unsigned char ParseHexDigit(const CHAR_T hex)
{
	if (hex >= (CHAR_T)'0' && hex <= (CHAR_T)'9')
		return hex - (CHAR_T)'0';
	if (hex >= (CHAR_T)'a' && hex <= (CHAR_T)'f')
		return 10 + hex - (CHAR_T)'a';
	if (hex >= (CHAR_T)'A' && hex <= (CHAR_T)'F')
		return 10 + hex - (CHAR_T)'A';

	return 0xff;
}

// converts given two hex digits to value between 0x0 and 0xff
// in case of error returns 0
template <class CHAR_T>
	unsigned char ParseHexByte(const CHAR_T *hex)
{
	const unsigned char rh = ParseHexDigit(hex[0]);
	const unsigned char rl = ParseHexDigit(hex[1]);
	if (rh == 0xff || rl == 0xff) {
		return 0;
	}
	return ((rh << 4) | rl);
}


// converts given value between 0x0 and 0xf to lowercased hex digit
// in case of error returns 0
char MakeHexDigit(const unsigned char c);

template <class StrT>
	size_t StrStartsFrom(const StrT &haystack, const typename StrT::value_type needle)
{
	return (!haystack.empty() && haystack.front() == needle) ? 1 : 0;
}

template <class CharT>
	size_t StrStartsFrom(const CharT *haystack, const CharT *needle)
{
	size_t i;
	for (i = 0; needle[i]; ++i) {
		if (haystack[i] != needle[i])
			return 0;
	}
	return i;
}

template <class StrT>
	size_t StrStartsFrom(const StrT &haystack, const typename StrT::value_type *needle)
{
	size_t i;
	for (i = 0; needle[i]; ++i) {
		if (i >= haystack.size() || haystack[i] != needle[i])
			return 0;
	}
	return i;
}

template <class StrT>
	size_t StrEndsBy(const StrT &haystack, const typename StrT::value_type *needle)
{
	const size_t l = tzlen(needle);
	if (!l || haystack.size() < l)
		return 0;

	return memcmp(haystack.c_str() + haystack.size() - l, needle, l * sizeof(typename StrT::value_type)) ? 0 : l;
}


const std::string &GetMyHome();

void InMyPathChanged(); // NOT thread safe, can be called only before any concurrent use of InMy...
std::string InMyConfig(const char *subpath = NULL, bool create_path = true);
std::string InMyCache(const char *subpath = NULL, bool create_path = true);
std::string InMyTemp(const char *subpath = NULL);

bool IsPathIn(const wchar_t *path, const wchar_t *root);

bool TranslateInstallPath_Bin2Share(std::wstring &path);
bool TranslateInstallPath_Bin2Share(std::string &path);
bool TranslateInstallPath_Lib2Share(std::wstring &path);
bool TranslateInstallPath_Lib2Share(std::string &path);
bool TranslateInstallPath_Share2Lib(std::wstring &path);
bool TranslateInstallPath_Share2Lib(std::string &path);
bool TranslateInstallPath_Bin2Lib(std::string &path);


// converts /some/path/to/filename.extension into form "filename@HASH"
// where HASH produced from path and extension and also filename has
// some special for ini files chars replaced by '_' and affected HASH
void FilePathHashSuffix(std::string &pathname);

void CheckedCloseFD(int &fd);
void CheckedCloseFDPair(int *fd);

size_t WriteAll(int fd, const void *data, size_t len, size_t chunk = (size_t)-1);
size_t ReadAll(int fd, void *data, size_t len);
ssize_t ReadWritePiece(int fd_src, int fd_dst);


int pipe_cloexec(int pipedes[2]);

void PutZombieUnderControl(pid_t pid);

void AbbreviateString(std::string &path, size_t needed_length);

const wchar_t *FileSizeToFractionAndUnits(unsigned long long &value);
std::wstring FileSizeString(unsigned long long value);
std::wstring ThousandSeparatedString(unsigned long long value);

std::string StrPrintfV(const char *format, va_list args);
std::string FN_PRINTF_ARGS(1) StrPrintf(const char *format, ...);

template <class CharT>
	std::basic_string<CharT> EnsureNoSlashAtEnd(std::basic_string<CharT> str, CharT slash = '/')
{
	for (size_t p = str.size(); p && str[p - 1] == slash; )  {
		str.resize(--p);
	}
	return str;
}

template <class CharT>
	std::basic_string<CharT> EnsureNoSlashAtNestedEnd(std::basic_string<CharT> str, CharT slash = '/')
{
	for (size_t p = str.size(); p > 1 && str[p - 1] == slash; )  {
		str.resize(--p);
	}
	return str;
}


template <class CharT>
	std::basic_string<CharT> EnsureSlashAtEnd(std::basic_string<CharT> str, CharT slash = '/')
{
	const size_t p = str.size();
	if (!p || str[p - 1] != slash) {
		str+= slash;
	}
	return str;
}

template <class CharT>
	std::basic_string<CharT> ExtractFilePath(std::basic_string<CharT> str, CharT slash = '/')
{
	const size_t p = str.rfind(slash);
	str.resize( (p != std::string::npos) ? p : 0);
	return str;
}


template <class CharT>
	std::basic_string<CharT> ExtractFileName(std::basic_string<CharT> str, CharT slash = '/')
{
	const size_t p = str.rfind(slash);
	return (p != std::string::npos) ? str.substr( p + 1, str.size() - (p + 1) ) : str;
}


template <class CharT>
	bool CutToSlash(std::basic_string<CharT> &str, bool include = false)
{
	size_t p = str.rfind('/');
	if (p == std::string::npos)
		return false;

	str.resize(include ? p + 1 : p);
	return true;
}

template <class CharT>
	void ReplaceFileNamePart(std::basic_string<CharT> &str, const CharT *replacement)
{
	if (CutToSlash(str, true)) {
		str+= replacement;
	} else {
		str = replacement;
	}
}


template <class CharT>
	void StrExplode(std::vector<std::basic_string<CharT> > &out, const std::basic_string<CharT> &str, const CharT *divs)
{
	for (size_t i = 0, j = 0; i <= str.size(); ++i) {
		const CharT *d = divs;
		if (i != str.size()) {
			for (; *d && *d != str[i]; ++d);
		}
		if (*d) {
			if (i > j) {
				out.emplace_back(str.substr(j, i - j));
			}
			j = i + 1;
		}
	}
}

template <class CharT>
	void StrTrimRight(std::basic_string<CharT> &str, const CharT *spaces = " \t")
{
	while (!str.empty() && strchr(spaces, str[str.size() - 1]) != NULL) {
		str.resize(str.size() - 1);
	}
}
template <class CharT>
	void StrTrimLeft(std::basic_string<CharT> &str, const CharT *spaces = " \t")
{
	while (!str.empty() && strchr(spaces, str[0]) != NULL) {
		str.erase(0, 1);
	}
}

template <class CharT>
	void StrTrim(std::basic_string<CharT> &str, const CharT *spaces = " \t")
{
	StrTrimRight(str, spaces);
	StrTrimLeft(str, spaces);
}


template <typename HaystackT, typename NeedlesT>
	static const HaystackT *FindAnyOfChars(const HaystackT *haystack, const NeedlesT *needles)
{
	for(; *haystack; ++haystack)
	{
		for(size_t i = 0; needles[i]; ++i)
		{
			if (*haystack == (HaystackT)needles[i])
				return haystack;
		}
	}
	return nullptr;
}

template <typename HaystackIT, typename NeedlesT>
	static HaystackIT FindAnyOfChars(HaystackIT haystack, const HaystackIT haystack_end, const NeedlesT *needles)
{
	for(; haystack != haystack_end; ++haystack)
	{
		for(size_t i = 0; needles[i]; ++i)
		{
			if (*haystack == (decltype(*haystack))needles[i])
				return haystack;
		}
	}
	return nullptr;
}

bool CaseIgnoreEngStrMatch(const std::string &str1, const std::string &str2);
bool CaseIgnoreEngStrMatch(const char *str1, const char *str2, size_t len);
const char *CaseIgnoreEngStrChr(const char c, const char *str, size_t len);


template <class POD_T>
	void ZeroFill(POD_T &dst)
{
	static_assert ( std::is_pod<POD_T>::value, "ZeroFill should be used with POD types only");
	static_assert ( sizeof(dst) != sizeof(void *), "ZeroFill should not be used with pointers");
	memset(&dst, 0, sizeof(dst));
}

template <class STRING_T, typename ARRAY_T>
	void StrAssignArray(STRING_T &s, const ARRAY_T &a)
{
	static_assert ( sizeof(a) != sizeof(void *), "StrAssignArray should be used with arrays but not pointers");
	s.assign(a, tnzlen(a, ARRAYSIZE(a)));
}

template <class STRING_T, typename ARRAY_T>
	void StrAppendArray(STRING_T &s, const ARRAY_T &a)
{
	static_assert ( sizeof(a) != sizeof(void *), "StrAppendArray should be used with arrays but not pointers");
	s.append(a, tnzlen(a, ARRAYSIZE(a)));
}


template <class STRING_T, typename ARRAY_T>
	bool StrMatchArray(STRING_T &s, const ARRAY_T &a)
{
	static_assert ( sizeof(a) != sizeof(void *), "StrMatchArray should be used with arrays but not pointers");
	const size_t l = tnzlen(a, ARRAYSIZE(a));
	return s.size() == l && s.compare(0, std::string::npos, a, l) == 0;
}

template <typename ARRAY_T, class CHAR_T>
	void ArrayCpyZ(ARRAY_T &dst, const CHAR_T *src)
{
	static_assert ( sizeof(dst) != sizeof(void *), "ArrayCpyZ should be used with arrays but not pointers");
	size_t i;
	for (i = 0; src[i] && i + 1 < ARRAYSIZE(dst); ++i) {
		dst[i] = src[i];
	}
	dst[i] = 0;
}

bool POpen(std::string &result, const char *command);
bool POpen(std::vector<std::wstring> &result, const char *command);

#define DBGLINE fprintf(stderr, "%d %d @%s\n", getpid(), __LINE__, __FILE__)
