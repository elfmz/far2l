#pragma once
#include <string>
#include <sys/types.h>

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

std::string EscapeQuotas(std::string str);

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

struct ErrnoSaver
{
	int v;
	ErrnoSaver();
	~ErrnoSaver();
};


int pipe_cloexec(int pipedes[2]);

void PutZombieUnderControl(pid_t pid);

bool isCombinedUTF32(wchar_t c);
