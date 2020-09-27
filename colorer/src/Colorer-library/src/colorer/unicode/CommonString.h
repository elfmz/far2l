#ifndef _COLORER_COMMONSTRING_H_
#define _COLORER_COMMONSTRING_H_

#include <wchar.h>
#include <stdint.h>
#include <xercesc/util/XMLChar.hpp>

/// default unicode char definition
typedef XMLCh wchar;

typedef char32_t w4char;
typedef unsigned char byte;

#define BAD_WCHAR ((wchar)0xFFFF)

#endif // _COLORER_COMMONSTRING_H_


