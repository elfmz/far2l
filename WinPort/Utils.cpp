#include "stdafx.h"
#include <set>
#include <string>
#include <locale> 
#include <algorithm>
#include <iostream>
#include <fstream>
#include <mutex>
#include "WinCompat.h"
#include "WinPort.h"
#include "Utils.h"



///////////////////

unsigned char Hex2Digit(const char hex)
{
	if (hex>='0' && hex<='9')
		return hex - '0';
	if (hex>='a' && hex<='f')
		return 10 + hex - 'a';
	if (hex>='A' && hex<='F')
		return 10 + hex - 'A';

	return 0;
}

unsigned char Hex2Byte(const char *hex)
{
	unsigned char r = Hex2Digit(hex[0]);
	r<<=4;
	r+= Hex2Digit(hex[1]);
	return r;
}
/*
std::u16string ToUTF16(const char *pc)
{
	std::wstring_convert<std::codecvt_utf8_utf16<char16_t>,char16_t> convert; 
	return convert.from_bytes(pc); 
}

std::u16string ToUTF16(const std::string &s)
{
	std::wstring_convert<std::codecvt_utf8_utf16<char16_t>,char16_t> convert; 
	return convert.from_bytes(s.c_str()); 
}

std::wstring ToWide(const std::string &s)
{
	std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>,wchar_t> convert; 
	return convert.from_bytes(s.c_str()); 
}

std::string FromUTF16(const char16_t *s)
{
	std::wstring_convert<std::codecvt_utf8_utf16<char16_t>,char16_t> convert; 
	return convert.to_bytes(s); 
}

std::string FromUTF16(const wchar_t *pw)
{
	return FromUTF16((const char16_t *)pw);
}
*/

void RectifyPath(std::string &s)
{
	std::string tmp;
	bool prev_slash = false;
	for (auto c : s) {
		if (c==GOOD_SLASH) {
			if (prev_slash) continue;
			prev_slash = true;
		} else
			prev_slash = false;
		tmp+= c;
	}
	//std::transform(tmp.begin(), tmp.end(), tmp.begin(), ::tolower);
	s.swap(tmp);
	if (s[0]!=GOOD_SLASH && (s[0]!='.' || s[1]!=GOOD_SLASH)) {
		s.insert(s.begin(), GOOD_SLASH);
		char *dir = get_current_dir_name();
		if (dir) {
			s.insert(0, dir);
			free(dir);
		} else
			s.insert(s.begin(), '.');
	}
	
}

std::string ConsumeWinPath(const wchar_t *pw)
{
	std::string s = Wide2MB(pw);
	RectifyPath(s);
	return s;
}

void AppendAndRectifyPath(std::string &s, const char *div, LPCWSTR append)
{
	s+= div;
	if (append) {
		const std::string &sub = Wide2MB(append);
		for (size_t i = 0, j = 0, ii = sub.size(); i<=ii; ++i) {
			if (i==ii || sub[i]==GOOD_SLASH) {
				if (i > j) {
					{
						s+= div;
						s+= sub.substr(j, i - j);
					}
				}
				j = i + 1;
			}
		}
	}
	RectifyPath(s);
	std::string ddiv = div;
	ddiv+= div;
	for (;;) {
		size_t p = s.find(ddiv);
		if (p==std::string::npos) break;
		s.erase(p, ddiv.size() / 2);
	}
}

bool MatchWildcard(const char *string, const char *wild) {
	// originally written by Jack Handy
	const char *cp = NULL, *mp = NULL;
	while ((*string) && (*wild != '*')) {
		if ((*wild != *string) && (*wild != '?'))
			return false;
		
		wild++;
		string++;
	}

	while (*string) {
		if (*wild == '*') {
			if (!*++wild)
				return true;
			
			mp = wild;
			cp = string+1;
		} else if ((*wild == *string) || (*wild == '?')) {
			wild++;
			string++;
		} else {
			wild = mp;
			string = cp++;
		}
	}

	while (*wild == '*') wild++;
	return !*wild;
}


std::string SettingsPath(const char *subpath)
{
#ifdef _WIN32
	std::string path = "D:\\.far2l";
#else	
	const char *home = getenv("HOME");
	std::string path = home ? home : "/tmp";
	path+= "/.far2l";
#endif
	
	mkdir(path.c_str(), 0777);
	if (subpath) {
		if (*subpath != GOOD_SLASH) 
			path+= GOOD_SLASH;
		for (const char *p = subpath; *p; ++p) {
			if (*p == GOOD_SLASH)
				mkdir(path.c_str(), 0777);
			path+= *p;
		}
	}
	
	return path;
	
}



void WinPortInitWellKnownEnv()
{
	if (!getenv("TEMP")) {
		std::string temp = SettingsPath("tmp");
		mkdir(temp.c_str(), 0777);
		setenv("TEMP", temp.c_str(), 1);
	}
}

