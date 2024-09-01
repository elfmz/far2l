#pragma once
#include <stdint.h>
#include <wchar.h>

#ifdef __cplusplus
extern "C" {
#endif

char *itoa(int i, char *a, int radix);
int _wtoi(const wchar_t *w);
int64_t _wtoi64(const wchar_t *w);
char * _i64toa(int64_t i, char *a, int radix);
wchar_t * _i64tow(int64_t i, wchar_t *w, int radix);
wchar_t * _itow(int i, wchar_t *w, int radix);

#ifdef __cplusplus
}

// if pos is not nullptr then value being parsed starting from *pos value
// and upon completion *pos is set to the end of parsed substring
unsigned long HexToULong(const char *str, size_t maxlen, size_t *pos = nullptr);
unsigned long DecToULong(const char *str, size_t maxlen, size_t *pos = nullptr);
long DecToLong(const char *str, size_t maxlen, size_t *pos = nullptr);

bool IsHexaDecimalNumberStr(const char *str);

#include <string>

// converts given hex digit to value between 0x0 and 0xf
// in case of error returns 0xff
template <class CHAR_T>
	unsigned char ParseHexDigit(const CHAR_T hex)
{
	if (hex >= (CHAR_T)'0' && hex <= (CHAR_T)'9')
		return hex - (CHAR_T)'0';
	if (hex >= (CHAR_T)'a' && hex <= (CHAR_T)'f')
		return 10 + hex - (CHAR_T)'a';
	if (hex >= (CHAR_T)'A' && hex <= (CHAR_T)'F')
		return 10 + hex - (CHAR_T)'A';

	return 0xff;
}

// converts given two hex digits to value between 0x0 and 0xff
// in case of error returns 0
template <class CHAR_T>
	unsigned char ParseHexByte(const CHAR_T *hex)
{
	const unsigned char rh = ParseHexDigit(hex[0]);
	const unsigned char rl = ParseHexDigit(hex[1]);
	if (rh == 0xff || rl == 0xff) {
		return 0;
	}
	return ((rh << 4) | rl);
}


// converts given value between 0x0 and 0xf to lowercased hex digit
// in case of error returns 0
char MakeHexDigit(const unsigned char c);

std::string ToHex(uint64_t v);
std::string ToPrefixedHex(uint64_t v, const char *prefix = "0x");
template <class V> std::string ToDec(V v)
{
	return std::to_string(v);
}

#endif

