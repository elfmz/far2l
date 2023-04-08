#pragma once

/*
farrtl.cpp

Переопределение различных CRT функций
*/

#include <WinCompat.h>

char *__cdecl far_strncpy(char *dest, const char *src, size_t DestSize);
wchar_t *__cdecl far_wcsncpy(wchar_t *dest, const wchar_t *src, size_t DestSize);

void __cdecl far_qsortex(void *base, size_t num, size_t width,
		int(__cdecl *comp_fp)(const void *, const void *, void *), void *ctx);

void __cdecl far_qsort(void *base, size_t num, size_t width, int(__cdecl *comp)(const void *, const void *));
