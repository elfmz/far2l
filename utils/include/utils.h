#pragma once
#include <string>
#include <sys/types.h>

void Wide2MB(const wchar_t *src, std::string &dst);
void MB2Wide(const char *src, std::wstring &dst);
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
