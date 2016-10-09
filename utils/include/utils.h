#pragma once
#include <string>


void Wide2MB(const wchar_t *src, std::string &dst);
void MB2Wide(const char *src, std::wstring &dst);
std::string Wide2MB(const wchar_t *src);
std::wstring MB2Wide(const char *src);
std::string StrWide2MB(const std::wstring &src);
std::wstring StrMB2Wide(const std::string &src);

unsigned char Hex2Digit(const char hex);
unsigned char Hex2Byte(const char *hex);

std::string EscapeQuotas(std::string str);

std::string InMyProfile(const char *subpath = NULL);


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
