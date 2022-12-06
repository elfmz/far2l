/*
farrtl.cpp

Переопределение различных CRT функций
*/

#include "headers.hpp"

// dest и src НЕ ДОЛЖНЫ пересекаться
char * __cdecl far_strncpy(char * dest,const char * src,size_t DestSize)
{
	char *tmpsrc = dest;

	while (DestSize>1 && (*dest++ = *src++))
	{
		DestSize--;
	}

	*dest = 0;
	return tmpsrc;
}

wchar_t * __cdecl far_wcsncpy(wchar_t * dest,const wchar_t * src,size_t DestSize)
{
	wchar_t *tmpsrc = dest;

	while (DestSize>1 && (*dest++ = *src++))
		DestSize--;

	*dest = 0;
	return tmpsrc;
}

#if defined(__MUSL__)
struct QSortExAdapterArg
{
	int (*__cdecl comp)(const void *, const void *, void *);
	void *ctx;
};

thread_local QSortExAdapterArg *g_qse_aa;

static int QSortExAdapter(const void *left, const void *right)
{
	return g_qse_aa->comp(left, right, g_qse_aa->ctx);
}

void __cdecl far_qsortex(void *base, size_t num, size_t width,
	int (*__cdecl comp)(const void *, const void *, void *), void *ctx)
{
	struct QSortExAdapterArg aa = { comp, ctx };
	g_qse_aa = &aa;
	qsort(base, num, width, QSortExAdapter);
}

#elif defined(__APPLE__) || defined(__FreeBSD__)
struct QSortExAdapterArg
{
	int (*__cdecl comp)(const void *, const void *, void *);
	void *ctx;
};

static int QSortExAdapter(void *a, const void *left, const void *right)
{
	struct QSortExAdapterArg *aa = (struct QSortExAdapterArg *)a;
	return aa->comp(left, right, aa->ctx);
}

void __cdecl far_qsortex(void *base, size_t num, size_t width,
	int (*__cdecl comp)(const void *, const void *, void *), void *ctx)
{
	struct QSortExAdapterArg aa = { comp, ctx };
	qsort_r(base, num, width, &aa, QSortExAdapter);
}

#else

void __cdecl far_qsortex(void *base, size_t num, size_t width,
	int (*__cdecl comp)(const void *, const void *, void *), void *ctx)
{
	qsort_r(base, num, width, comp, ctx);
}

#endif

void __cdecl far_qsort(void *base, size_t num, size_t width,
    int (*__cdecl comp)(const void *, const void *)
)
{
	qsort(base, num, width, comp);
}
