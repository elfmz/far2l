/*
farrtl.cpp

Переопределение различных CRT функций
*/

#include "headers.hpp"

#if !defined(__APPLE__) && !defined(__FreeBSD__)
# include <alloca.h>
#endif

#include "savefpos.hpp"
#include "console.hpp"

bool InsufficientMemoryHandler()
{
	Console.SetTextAttributes(FOREGROUND_RED|FOREGROUND_INTENSITY);
	COORD OldPos,Pos={};
	Console.GetCursorPosition(OldPos);
	Console.SetCursorPosition(Pos);
	static WCHAR ErrorMessage[] = L"Not enough memory is available to complete this operation.\nPress Enter to retry or Esc to continue...";
	Console.Write(ErrorMessage, ARRAYSIZE(ErrorMessage));
	Console.SetCursorPosition(OldPos);
	INPUT_RECORD ir={};
	do
	{
		Console.ReadInput(ir);
	}
	while(!(ir.EventType == KEY_EVENT && !ir.Event.KeyEvent.bKeyDown && (ir.Event.KeyEvent.wVirtualKeyCode == VK_RETURN || ir.Event.KeyEvent.wVirtualKeyCode == VK_RETURN || ir.Event.KeyEvent.wVirtualKeyCode == VK_ESCAPE)));
	return ir.Event.KeyEvent.wVirtualKeyCode == VK_RETURN;
}

#ifdef SYSLOG
#define MEMORY_CHECK
#endif

#ifdef MEMORY_CHECK
enum ALLOCATION_TYPE
{
	AT_C,
	AT_CPP,
	AT_CPPARRAY,
};

struct MEMINFO
{
	ALLOCATION_TYPE AllocationType;
};
#endif

void *__cdecl xf_malloc(size_t size)
{
#ifdef MEMORY_CHECK
	size+=sizeof(MEMINFO);
#endif

	void *Ptr = nullptr;
	do
	{
		Ptr = malloc(size);
	}
	while (!Ptr && InsufficientMemoryHandler());

#ifdef MEMORY_CHECK
	MEMINFO* Info = reinterpret_cast<MEMINFO*>(Ptr);
	Info->AllocationType = AT_C;
	Ptr=reinterpret_cast<LPBYTE>(Ptr)+sizeof(MEMINFO);
#endif

#if defined(SYSLOG)
	CallMallocFree++;
#endif

	return Ptr;
}

void *__cdecl xf_realloc_nomove(void * block, size_t size)
{
	if (!block)
	{
		return xf_malloc(size);
	}
	else
	{
		void *Ptr=xf_malloc(size);

		if (Ptr)
			xf_free(block);

		return Ptr;
	}
}

void *__cdecl xf_realloc(void * block, size_t size)
{
#ifdef MEMORY_CHECK
	if(block)
	{
		block=reinterpret_cast<LPBYTE>(block)-sizeof(MEMINFO);
		MEMINFO* Info = reinterpret_cast<MEMINFO*>(block);
		assert(Info->AllocationType == AT_C);
	}
	size+=sizeof(MEMINFO);
#endif

	void *Ptr = nullptr;
	do
	{
		Ptr = realloc(block, size);
	}
	while (size && !Ptr && InsufficientMemoryHandler());

#ifdef MEMORY_CHECK
	if (!block)
	{
		MEMINFO* Info = reinterpret_cast<MEMINFO*>(Ptr);
		Info->AllocationType = AT_C;
	}
	Ptr=reinterpret_cast<LPBYTE>(Ptr)+sizeof(MEMINFO);
#endif

#if defined(SYSLOG)
	if (!block)
	{
		CallMallocFree++;
	}
#endif

	return Ptr;
}

void __cdecl xf_free(void * block)
{
#ifdef MEMORY_CHECK
	if(block)
	{
		block=reinterpret_cast<LPBYTE>(block)-sizeof(MEMINFO);
		MEMINFO* Info = reinterpret_cast<MEMINFO*>(block);

		assert(Info->AllocationType == AT_C);
	}
#endif

#if defined(SYSLOG)
	CallMallocFree--;
#endif

	free(block);
}

int64_t ftell64(FILE *fp)
{
#if defined(__APPLE__) || defined(__FreeBSD__) || defined(__CYGWIN__)
	return ftello(fp);
#elif defined(__GNUC__)
	return ftello64(fp);
#else
	return _ftelli64(fp);
#endif
}

int fseek64(FILE *fp, int64_t offset, int whence)
{
#if defined(__APPLE__) || defined(__FreeBSD__) || defined(__CYGWIN__)
	return fseeko(fp,offset,whence);
#elif defined(__GNUC__)
	return fseeko64(fp,offset,whence);
#else
	return _fseeki64(fp,offset,whence);
#endif
}

long filelen(FILE *FPtr)
{
	SaveFilePos SavePos(FPtr);
	fseek(FPtr,0,SEEK_END);
	return(ftell(FPtr));
}

int64_t filelen64(FILE *FPtr)
{
	SaveFilePos SavePos(FPtr);
	fseek64(FPtr,0,SEEK_END);
	return(ftell64(FPtr));
}

char * __cdecl xf_strdup(const char * string)
{
	if (string)
	{
		char *memory;

		if ((memory = (char *)xf_malloc(strlen(string) + 1)) )
			return strcpy(memory,string);
	}

	return nullptr;
}

wchar_t * __cdecl xf_wcsdup(const wchar_t * string)
{
	if (string)
	{
		wchar_t *memory;

		if ((memory = (wchar_t *)xf_malloc((wcslen(string)+1)*sizeof(wchar_t))) )
			return wcscpy(memory,string);
	}

	return nullptr;
}

// dest и src НЕ ДОЛЖНЫ пересекаться
char * __cdecl xstrncpy(char * dest,const char * src,size_t DestSize)
{
	char *tmpsrc = dest;

	while (DestSize>1 && (*dest++ = *src++))
	{
		DestSize--;
	}

	*dest = 0;
	return tmpsrc;
}

wchar_t * __cdecl xwcsncpy(wchar_t * dest,const wchar_t * src,size_t DestSize)
{
	wchar_t *tmpsrc = dest;

	while (DestSize>1 && (*dest++ = *src++))
		DestSize--;

	*dest = 0;
	return tmpsrc;
}

char * __cdecl xstrncat(char * dest,const char * src, size_t DestSize)
{
	char * start=dest;

	while (*dest)
	{
		dest++;
		DestSize--;
	}

	while (DestSize-->1)
		if (!(*dest++=*src++))
			return start;

	*dest=0;
	return start;
}

wchar_t * __cdecl xwcsncat(wchar_t * dest,const wchar_t * src, size_t DestSize)
{
	wchar_t * start=dest;

	while (*dest)
	{
		dest++;
		DestSize--;
	}

	while (DestSize-->1)
		if (!(*dest++=*src++))
			return start;

	*dest=0;
	return start;
}

#if defined(__APPLE__) || defined(__FreeBSD__)
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
