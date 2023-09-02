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

bool IsHexaDecimalNumberStr(const char *str);

#endif

