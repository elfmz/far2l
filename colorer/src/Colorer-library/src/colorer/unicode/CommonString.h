#ifndef _COLORER_COMMONSTRING_H_
#define _COLORER_COMMONSTRING_H_

#include <wchar.h>

/// default unicode char definition
#ifdef WIN32
typedef wchar_t wchar;
#else
typedef char16_t wchar;
#endif
typedef char32_t w4char;
typedef unsigned char byte;

#define BAD_WCHAR ((wchar)0xFFFF)

#endif // _COLORER_COMMONSTRING_H_


