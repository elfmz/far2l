#include "stdafx.h"
#include <set>
#include <string>
#include <locale> 
#include <algorithm>
#include <iostream>
#include <fstream>
#include <codecvt> 
#include <mutex>
#include "WinCompat.h"
#include "WinPort.h"
#include "Utils.h"


std::string UTF16to8(const wchar_t *src)
{
	size_t src_len = wcslen(src);
	std::string dst;
	dst.resize(src_len + 8);
	for (;; ) {
		int r = WINPORT(WideCharToMultiByte)(CP_UTF8, 0, src, src_len, &dst[0], dst.size(), NULL, NULL);
		if (r<=dst.size()) {
			dst.resize(r);
			break;
		}
		if (r==0 && WINPORT(GetLastError)()==ERROR_INSUFFICIENT_BUFFER) {
			dst.resize(dst.size() + 8 + dst.size()/2);
		} else {
			fprintf(stderr, "UTF16to8('" WS_FMT "') - failed\n", src);
			dst.clear();
			break;
		}
	}
	return dst;
}

std::wstring UTF8to16(const char *src)
{
	size_t src_len = strlen(src);
	std::wstring dst;
	dst.resize(src_len + 8);
	for (;; ) {
		int r = WINPORT(MultiByteToWideChar)(CP_UTF8, 0, src, src_len, &dst[0], dst.size());
		if (r<=dst.size()) {
			dst.resize(r);
			break;
		}
		if (r==0 && WINPORT(GetLastError)()==ERROR_INSUFFICIENT_BUFFER) {
			dst.resize(dst.size() + 8 + dst.size()/2);
		} else {
			fprintf(stderr, "UTF8to16('%s') - failed\n", src);
			dst.clear();
			break;
		}
	}
	return dst;
}

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
		if (c==BAD_SLASH) c = GOOD_SLASH;
		if (c==GOOD_SLASH) {
			if (prev_slash) continue;
			prev_slash = true;
		} else
			prev_slash = false;
		tmp+= c;
	}
	//std::transform(tmp.begin(), tmp.end(), tmp.begin(), ::tolower);
	s.swap(tmp);
	if (s[0]!=GOOD_SLASH) {
		s.insert(s.begin(), GOOD_SLASH);
		s.insert(s.begin(), '.');
	}
	
}

std::string ConsumeWinPath(const wchar_t *pw)
{
	if (pw[0]=='\\' && (pw[1]=='\\' || pw[1]=='?') && pw[2]=='?'  && pw[3]=='\\')
		pw+= 4;
	else if (pw[0]=='/' && (pw[1]=='/' || pw[1]=='?') && pw[2]=='?'  && pw[3]=='/')
		pw+= 4;
	if (pw[0]=='U' && pw[1]=='N' && pw[2]=='C')
		pw+=3; 
	std::string s = UTF16to8(pw);
	RectifyPath(s);
	return s;
}

void AppendAndRectifyPath(std::string &s, const char *div, LPCWSTR append)
{
	s+= div;
	if (append) {
		const std::string &sub = UTF16to8(append);
		for (size_t i = 0, j = 0, ii = sub.size(); i<=ii; ++i) {
			if (i==ii || sub[i]==GOOD_SLASH|| sub[i]==BAD_SLASH) {
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
