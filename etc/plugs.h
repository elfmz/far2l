#pragma once
#include "plugin.hpp"
#include <windows.h>


#ifdef UNICODE
#define EXP_NAME(p) _export p ## W
#define _tmemset(t,c,s) wmemset(t,c,s)
#define _tmemcpy(t,s,c) wmemcpy(t,s,c)
#define _tmemchr(b,c,n) wmemchr(b,c,n)
#define _tmemmove(b,c,n) wmemmove(b,c,n)
#else
#define EXP_NAME(p) _export p ## A
#define _tmemset(t,c,s) memset(t,c,s)
#define _tmemcpy(t,s,c) memcpy(t,s,c)
#define _tmemchr(b,c,n) memchr(b,c,n)
#define _tmemmove(b,c,n) memmove(b,c,n)
#endif
