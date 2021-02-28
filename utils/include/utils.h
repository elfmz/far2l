#pragma once
#include <string>
#include <vector>
#include <cstdarg>
#include <sys/types.h>
#include "luck.h"
#include "MatchWildcard.hpp"

template <class C> static size_t tzlen(const C *ptz)
{
	const C *etz;
	for (etz = ptz; *etz; ++etz);
	return (etz - ptz);
}


unsigned long htoul(const char *str, size_t maxlen = (size_t)-1);
unsigned long atoul(const char *str, size_t maxlen = (size_t)-1);

void Wide2MB(const wchar_t *src, std::string &dst);
void MB2Wide(const char *src, size_t src_len, std::wstring &dst);
void MB2Wide(const char *src, std::wstring &dst);
size_t MB2Wide_HonorIncomplete(const char *src, size_t src_len, std::wstring &dst);
std::string Wide2MB(const wchar_t *src);
std::wstring MB2Wide(const char *src);

void StrWide2MB(const std::wstring &src, std::string &dst);
std::string StrWide2MB(const std::wstring &src);

void StrMB2Wide(const std::string &src, std::wstring &dst);
std::wstring StrMB2Wide(const std::string &src);

// converts given value between 0x0 and 0xf to hex digit
// in case of error returns 0
char MakeHexDigit(const unsigned char c);

// converts given hex digit to value between 0x0 and 0xf
// in case of error returns 0xff
unsigned char ParseHexDigit(const char hex);

// converts given two hex digits to value between 0x0 and 0xff
// in case of error returns 0
unsigned char ParseHexByte(const char *hex);

size_t StrStartsFrom(const std::string &haystack, const char *needle);
size_t StrEndsBy(const std::string &haystack, const char *needle);

std::string EscapeEscapes(std::string str);
std::string EscapeQuotas(const std::string &str);
std::wstring EscapeQuotas(const std::wstring &str);
std::string EscapeCmdStr(const std::string &str);
std::wstring EscapeCmdStr(const std::wstring &str);

void QuoteCmdArg(std::string &str);
void QuoteCmdArg(std::wstring &str);
void QuoteCmdArgIfNeed(std::string &str);
void QuoteCmdArgIfNeed(std::wstring &str);

std::string GetMyHome();
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

// converts /some/path/to/filename.extension into form "filename@HASH"
// where HASH produced from path and extension and also filename has
// some special for ini files chars replaced by '_' and affected HASH
void FilePathHashSuffix(std::string &pathname);

void CheckedCloseFD(int &fd);
void CheckedCloseFDPair(int *fd);

size_t WriteAll(int fd, const void *data, size_t len, size_t chunk = (size_t)-1);
size_t ReadAll(int fd, void *data, size_t len);
ssize_t ReadWritePiece(int fd_src, int fd_dst);


struct ErrnoSaver
{
	int v;
	ErrnoSaver();
	~ErrnoSaver();
};


int pipe_cloexec(int pipedes[2]);

void PutZombieUnderControl(pid_t pid);

size_t GetMallocSize(void *p);


void AbbreviateString(std::string &path, size_t needed_length);

const wchar_t *FileSizeToFractionAndUnits(unsigned long long &value);
std::wstring FileSizeString(unsigned long long value);
std::wstring ThousandSeparatedString(unsigned long long value);

std::string StrPrintfV(const char *format, va_list args);
std::string StrPrintf(const char *format, ...);

template <class CharT>
	std::basic_string<CharT> EnsureNoSlashAtEnd(std::basic_string<CharT> str, CharT slash = '/')
{
	for (size_t p = str.size(); p && str[p - 1] == slash; )  {
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

bool CaseIgnoreEngStrMatch(const char *str1, const char *str2, size_t len);
const char *CaseIgnoreEngStrChr(const char c, const char *str, size_t len);

#define APP_BASENAME "far2l"
