#pragma once

#ifdef __cplusplus
extern "C" {
#endif

char *itoa(int i, char *a, int radix);
int _wtoi(const wchar_t *w);
int64_t _wtoi64(const wchar_t *w);
char * _i64toa(int64_t i, char *a, int radix);
wchar_t * _i64tow(int64_t i, wchar_t *w, int radix);
wchar_t * _itow(int i, wchar_t *w, int radix);
unsigned long htoul(const char *str, size_t maxlen);
unsigned long atoul(const char *str, size_t maxlen);

#ifdef __cplusplus
}
#endif
