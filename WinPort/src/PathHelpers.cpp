#include <set>
#include <string>
#include <list>
#include <locale>
#include <algorithm>
#include <iostream>
#include <fstream>
#include <mutex>
#include <vector>
#include "WinCompat.h"
#include "WinPort.h"
#include "sudo.h"
#include "PathHelpers.h"
#include <utils.h>



///////////////////

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
		char buf[MAX_PATH + 1];
		char *dir = sdc_getcwd(buf, ARRAYSIZE(buf) - 1);
		buf[ARRAYSIZE(buf) - 1] = 0;

		if (dir)
			s.insert(0, dir);
		else
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

void WinPortInitWellKnownEnv()
{
#ifdef __APPLE__
	std::list<std::string> pathfiles;
	pathfiles.emplace_back("/etc/paths");
	DIR *dir = opendir("/etc/paths.d");
	std::string str;
	if (dir) {
		for (;;) {
			struct dirent *de = readdir(dir);
			if (!de) break;
			if (strcmp(de->d_name, ".") == 0 || strcmp(de->d_name, "..") == 0)
				continue;

			str = "/etc/paths.d/";
			str+= de->d_name;
			pathfiles.emplace_back(str);
		}
		closedir(dir);
	}

	const char *psz = getenv("PATH");
	str = psz ? psz : "";
	bool changed = false;
	for (const auto &pathfile : pathfiles) {
		FILE *f = fopen(pathfile.c_str(), "r");
		if (f) {
			char line[MAX_PATH + 1] = {};
			while (fgets(line, sizeof(line) - 1, f)) {
				char *crlf = strpbrk(line, "\r\n");
				if (crlf)
					*crlf = 0;

				size_t l = strlen(line);
				if (!l)
					continue;

				bool dup = false;
				for (size_t p = str.find(line); p != std::string::npos; p = str.find(line, p + l)) {
					if ( (p == 0 || str[p - 1] == ':') && ( p + l == str.size() || str[p + l] == ':')) {
						dup = true;
						break;
					}
				}

				if (dup)
					continue;

				if (!str.empty() && str[str.size() - 1] != ':')
					str+= ':';
				str.append(line, l);
				changed = true;
			}
			fclose(f);
		}
	}

	if (changed) {
		fprintf(stderr, "PATH:= '%s'\n", str.c_str());
		setenv("PATH", str.c_str(), 1);
	}
#endif
}


static std::wstring g_path_translation_prefix;
static std::string g_path_translation_prefix_a;

SHAREDSYMBOL void SetPathTranslationPrefix(const wchar_t *prefix)
{
	g_path_translation_prefix = prefix;
	g_path_translation_prefix_a = Wide2MB(prefix);
}

SHAREDSYMBOL const wchar_t *GetPathTranslationPrefix()
{
	return g_path_translation_prefix.c_str();
}

SHAREDSYMBOL const char *GetPathTranslationPrefixA()
{
	return g_path_translation_prefix_a.c_str();
}

