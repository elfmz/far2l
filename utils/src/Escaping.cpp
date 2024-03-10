#include "utils.h"
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

static struct EscapesLikeInC
{
	char raw;
	char encoded;
} s_escapes_like_in_c[] = {
	{ '\a', 'a'},
	{ '\b', 'b'},
	{ '\e', 'e'},
	{ '\f', 'f'},
	{ '\n', 'n'},
	{ '\r', 'r'},
	{ '\t', 't'},
	{ '\v', 'v'},
	{ '\\', '\\'},
	{ '\'', '\''},
	{ '\"', '\"'},
	{ '?', '?'}
};

std::string EscapeLikeInC(std::string str)
{
	for (size_t i = str.size(); i;) {
		--i;
		for (const auto &esc : s_escapes_like_in_c) if (str[i] == esc.raw) {
			char seq[] = {'\\', esc.encoded};
			str.replace(i, 1, seq, 2);
			break;
		}
	}
	return str;
}


template <class STRING_T>
	static STRING_T EscapeQuotesT(STRING_T str)
{
	for(size_t p = str.find('\"'); p!=std::string::npos; p = str.find('\"', p)) {
		str.insert(p, 1, '\\');
		p+= 2;
	}
	return str;
}

std::string EscapeQuotes(const std::string &str) {return EscapeQuotesT(str); }
std::wstring EscapeQuotes(const std::wstring &str) {return EscapeQuotesT(str); }

template <class CHAR_T>
	static std::basic_string<CHAR_T> EscapeCmdStrT(std::basic_string<CHAR_T> str, const CHAR_T *escaped_chars)
{
	for(size_t p = str.find_first_of(escaped_chars);
			p != std::basic_string<CHAR_T>::npos;
			p = str.find_first_of(escaped_chars, p)) {

		str.insert(p, 1, '\\');
		p+= 2;
	}
	return str;
}

std::string EscapeCmdStr(const std::string &str, const char *escaped_chars) {return EscapeCmdStrT<char>(str, escaped_chars); }
std::wstring EscapeCmdStr(const std::wstring &str, const wchar_t *escaped_chars) {return EscapeCmdStrT<wchar_t>(str, escaped_chars); }

std::string EscapeEscapes(std::string str)
{
	for (size_t p = 0; (p + 1) < str.size(); ) {
		if (str[p] == '\\' && (str[p + 1] == '\"' || str[p + 1] == '\\' || str[ p + 1] == '\t'|| str[ p + 1] == '`'|| str[ p + 1] == '$') ) {
			str.insert(p, 2, '\\');
			p+= 4;
		} else
			++p;
	}
	return str;
}

template <class STRING_T>
	static void QuoteCmdArgT(STRING_T &str)
{
	STRING_T tmp(1, '\"');
	tmp+= EscapeCmdStr(str);
	tmp+= '\"';
	str.swap(tmp);
}

void QuoteCmdArg(std::string &str) { QuoteCmdArgT(str); }
void QuoteCmdArg(std::wstring &str) { QuoteCmdArgT(str); }

void QuoteCmdArgIfNeed(std::string &str)
{
	if (str.find_first_of(" \\\"\'\r\n\t&|;,()`$") != std::string::npos) {
		QuoteCmdArg(str);
	}
}

void QuoteCmdArgIfNeed(std::wstring &str)
{
	if (str.find_first_of(L" \\\"\'\r\n\t&|;,()`$") != std::wstring::npos) {
		QuoteCmdArg(str);
	}
}

