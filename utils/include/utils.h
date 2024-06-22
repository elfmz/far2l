#pragma once
#include <string>
#include <vector>
#include <cstdarg>
#include <sys/types.h>
#include <string.h>
#include "cctweaks.h"
#include "BitTwiddle.hpp"
#include "MatchWildcard.hpp"
#include "WideMB.h"
#include "Escaping.h"
#include "Environment.h"
#include "ErrnoSaver.hpp"
#include "PlatformConstants.h"
#include "debug.h"
#include "IntStrConv.h"
#include "CharArray.hpp"

#define MAKE_STR(x) _MAKE_STR(x)
#define _MAKE_STR(x) #x

#ifdef __APPLE__
# define st_mtim st_mtimespec
# define st_ctim st_ctimespec
# define st_atim st_atimespec
#endif

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

template <class CharT>
	size_t StrEndsBy(const CharT *haystack, const CharT *needle)
{
	const size_t hl = tzlen(haystack);
	const size_t l = tzlen(needle);
	if (!l || hl < l)
		return 0;

	return memcmp(haystack + hl - l, needle, l * sizeof(CharT)) ? 0 : l;
}

template <class StrT>
	size_t StrEndsBy(const StrT &haystack, const typename StrT::value_type *needle)
{
	const size_t l = tzlen(needle);
	if (!l || haystack.size() < l)
		return 0;

	return memcmp(haystack.c_str() + haystack.size() - l, needle, l * sizeof(typename StrT::value_type)) ? 0 : l;
}

template <class StrT>
	size_t StrEndsBy(const StrT &haystack, const typename StrT::value_type needle)
{
	return !haystack.empty() && haystack.back() == needle;
}


const std::string &GetMyHome();

void InMyPathChanged(); // NOT thread safe, can be called only before any concurrent use of InMy...
std::string InMyConfig(const char *subpath = NULL, bool create_path = true);
std::string InMyCache(const char *subpath = NULL, bool create_path = true);
std::string InMyTemp(const char *subpath = NULL);
std::string FN_PRINTF_ARGS(1) InMyTempFmt(const char *subpath_fmt, ...);

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

void MakeFDBlocking(int fd);
void MakeFDNonBlocking(int fd);
void MakeFDCloexec(int fd);
void MakeFDNonCloexec(int fd);
void HintFDSequentialAccess(int fd);

size_t WriteAll(int fd, const void *data, size_t len, size_t chunk = (size_t)-1);
size_t ReadAll(int fd, void *data, size_t len);
ssize_t ReadWritePiece(int fd_src, int fd_dst);

bool ReadWholeFile(const char *path, std::string &result, size_t limit = (size_t)-1);
bool WriteWholeFile(const char *path, const void *content, size_t length, unsigned int mode = 0600);
bool WriteWholeFile(const char *path, const std::string &content, unsigned int mode = 0600);

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
	void StrTrimRight(std::basic_string<CharT> &str, const char *spaces = " \t")
{
	while (!str.empty() && unsigned(str.back()) <= 0x7f && strchr(spaces, str.back()) != NULL) {
		str.pop_back();
	}
}
template <class CharT>
	void StrTrimLeft(std::basic_string<CharT> &str, const char *spaces = " \t")
{
	while (!str.empty() && unsigned(str[0]) <= 0x7f && strchr(spaces, str[0]) != NULL) {
		str.erase(0, 1);
	}
}

template <class CharT>
	void StrTrim(std::basic_string<CharT> &str, const char *spaces = " \t")
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

bool POpen(std::string &result, const char *command);
bool POpen(std::vector<std::wstring> &result, const char *command);

bool IsCharFullWidth(wchar_t c);
bool IsCharPrefix(wchar_t c);
bool IsCharSuffix(wchar_t c);
bool IsCharXxxfix(wchar_t c);

void FN_NORETURN FN_PRINTF_ARGS(1) ThrowPrintf(const char *format, ...); // throws std::runtime_error with formatted .what()

