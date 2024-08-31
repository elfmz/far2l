#include <stdio.h>
#include <stdlib.h>
#include <wchar.h>
#include "utils.h"

extern "C"
{
char *itoa(int i, char *a, int radix)
{
	switch (radix) {
		case 10: sprintf(a, "%d", i); break;
		case 16: sprintf(a, "%x", i); break;
	}
	return a;
}

int _wtoi(const wchar_t *w)
{
	wchar_t *endptr = 0;
	return wcstol(w, &endptr, 10);
}

int64_t _wtoi64(const wchar_t *w)
{
	wchar_t *endptr = 0;
	return wcstoll(w, &endptr, 10);
}

char * _i64toa(int64_t i, char *a, int radix)
{
	switch (radix) {
		case 10: sprintf(a, "%lld", (long long)i); break;
		case 16: sprintf(a, "%llx", (long long)i); break;
	}
	return a;
}

wchar_t * _i64tow(int64_t i, wchar_t *w, int radix)
{
	int neg;
	if (i==0) {
		w[0] = '0';
		w[1] = 0;
		return w;
	}

	neg = i < 0;
	if (neg) {
		*w++ = '-';
		i = -i;
	}

	for (int64_t j = i; j; j/= radix) ++w;
	*w = 0;
	for (; i; i/= radix) {
		unsigned int d = i % radix;
		--w;
		if (d<=9) *w = d + '0';
		else *w = d - 10 + 'a';
	}

	return neg ? w-1 : w;
}


wchar_t * _itow(int i, wchar_t *w, int radix)
{
	int neg;
	if (i==0) {
		w[0] = '0';
		w[1] = 0;
		return w;
	}
	neg = i<0;
	if (neg) {
		*w++ = '-';
		i = -i;
	}

	for (int j = i; j; j/= radix) ++w;
	*w = 0;
	for (; i; i/= radix) {
		unsigned int d = i % radix;
		--w;
		if (d<=9) *w = d + '0';
		else *w = d - 10 + 'a';
	}

	return neg ? w-1 : w;
}

}

unsigned long HexToULong(const char *str, size_t maxlen, size_t *pos)
{
	unsigned long out = 0;
	size_t i;
	for (i = pos ? *pos : 0; i < maxlen; ++i) {
		unsigned char x = ParseHexDigit(str[i]);
		if (x == 0xff) {
			break;
		}
		out<<= 4;
		out|= (unsigned long)x;
	}
	if (pos) {
		*pos = i;
	}

	return out;
}

unsigned long DecToULong(const char *str, size_t maxlen, size_t *pos)
{
	unsigned long out = 0;
	size_t i;
	for (i = pos ? *pos : 0; i < maxlen; ++i) {
		const auto ch = str[i];
		if (ch < '0' || ch > '9') {
			break;
		}
		out*= 10;
		out+= ch - '0';
	}
	if (pos) {
		*pos = i;
	}
	return out;
}

long DecToLong(const char *str, size_t maxlen, size_t *pos)
{
	const bool minus = maxlen && *str == '-';
	long out = DecToULong(minus ? str + 1 : str, minus ? maxlen - 1 : maxlen, pos);
	if (minus) {
		if (pos) {
			++*pos;
		}
		out = -out;
	}
	return out;
}

bool IsHexaDecimalNumberStr(const char *str)
{
	if (!*str) {
		return false;
	}

	for (;*str; ++str) {
		if ((*str < '0' || *str > '9')
				&& (*str < 'a' || *str > 'f')
				&& (*str < 'A' || *str > 'F')) {
			return false;
		}
	}

	return true;
}

char MakeHexDigit(const unsigned char c)
{
	if (c <= 9) {
		return '0' + c;
	}

	if (c <= 0xf) {
		return 'a' + (c - 0xa);
	}

	return 0;
}


static void AppendHex(std::string &s, uint64_t v)
{
	if (v == 0) {
		s+= '0';
		return;
	}

	const size_t prev_sz = s.size();
	size_t i = 0;
	for (uint64_t tmp = v; tmp; tmp/= 16) {
		++i;
	}
	s.resize(prev_sz + i);
	for (uint64_t tmp = v; tmp; tmp/= 16) {
		--i;
		s[prev_sz + i] = MakeHexDigit((unsigned char)(tmp % 16));
	}
}

std::string ToHex(uint64_t v)
{
	std::string s;
	AppendHex(s, v);
	return s;
}

std::string ToPrefixedHex(uint64_t v, const char *prefix)
{
	std::string s(prefix);
	AppendHex(s, v);
	return s;
}
