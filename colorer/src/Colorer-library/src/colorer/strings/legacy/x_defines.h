/*
    This file was automatically generated from
    UNICODE 3.2.0 UnicodeData.txt base.
    Do not edit it with hands.
*/

// macroses to access char properties
#define CHAR_PROP(c) (arr_CharInfo[ (arr_idxCharInfo[(c)>>4]<<4) + ((c) & (0xFFFF>>(16-4)))])
#define CHAR_PROP2(c) (arr_CharInfo2[ (arr_idxCharInfo2[(c)>>4]<<4) + ((c) & (0xFFFF>>(16-4)))])
#define TITLE_CASE(c) ((c) & (1 << 15))
#define CHAR_CATEGORY(c) ((c) & 0x1F)
#define MIRRORED(c) ((c) & (1 << 14))
#define NUMBER(c) ((c) & (1 << 13))
#define COMBINING_CLASS(c) (((c) >> 5) & 0xFF)




