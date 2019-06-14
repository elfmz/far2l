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

unsigned char Hex2Digit(const char hex);
unsigned char Hex2Byte(const char *hex);

size_t StrStartsFrom(const std::string &haystack, const char *needle);

std::string EscapeEscapes(std::string str);
std::string EscapeQuotas(std::string str);

void QuoteCmdArg(std::string &str);
void QuoteCmdArgIfNeed(std::string &str);

std::string GetMyHome();
std::string InMyConfig(const char *subpath = NULL, bool create_path = true);
std::string InMyTemp(const char *subpath = NULL);

bool IsPathIn(const wchar_t *path, const wchar_t *root);

bool TranslateInstallPath_Bin2Share(std::wstring &path);
bool TranslateInstallPath_Bin2Share(std::string &path);
bool TranslateInstallPath_Lib2Share(std::wstring &path);
bool TranslateInstallPath_Lib2Share(std::string &path);
bool TranslateInstallPath_Share2Lib(std::wstring &path);
bool TranslateInstallPath_Share2Lib(std::string &path);



void CheckedCloseFD(int &fd);
void CheckedCloseFDPair(int *fd);

size_t WriteAll(int fd, const void *data, size_t len, size_t chunk = (size_t)-1);
size_t ReadAll(int fd, void *data, size_t len);


struct ErrnoSaver
{
	int v;
	ErrnoSaver();
	~ErrnoSaver();
};


int pipe_cloexec(int pipedes[2]);

void PutZombieUnderControl(pid_t pid);

bool isCombinedUTF32(wchar_t c);

size_t GetMallocSize(void *p);


void AbbreviateString(std::string &path, size_t needed_length);

const char *FileSizeToFractionAndUnits(unsigned long long &value);
std::string FileSizeString(unsigned long long value);

std::string StrPrintfV(const char *format, va_list args);
std::string StrPrintf(const char *format, ...);

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
