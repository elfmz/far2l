#ifndef _COLORER_COMMONSTRING_H_
#define _COLORER_COMMONSTRING_H_

#include <wchar.h>

/// default unicode char definition
#ifndef WCHAR_MAX
#error wchar misconfig
#elif WCHAR_MAX == 0xFFFFFFFE/2
typedef unsigned short wchar;
#else
typedef wchar_t wchar;
#endif

typedef char32_t w4char;
typedef unsigned char byte;

#define BAD_WCHAR ((wchar)0xFFFF)

#endif // _COLORER_COMMONSTRING_H_


