#pragma once
#include "wineguts/unicode.h"

#ifdef __cplusplus
extern "C" {
#endif
int wine_cpsymbol_mbstowcs( const char *src, int srclen, WCHAR *dst, int dstlen);
int wine_cpsymbol_wcstombs( const WCHAR *src, int srclen, char *dst, int dstlen);
int wine_cp_mbstowcs( const union cptable *table, int flags,
                      const char *s, int srclen,
                      WCHAR *dst, int dstlen );
const union cptable *get_codepage_table( unsigned int codepage );

unsigned int wine_decompose( int flags, WCHAR ch, WCHAR *dst, unsigned int dstlen );

#ifdef __cplusplus
}
#endif
