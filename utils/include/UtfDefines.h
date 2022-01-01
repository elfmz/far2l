#pragma once
enum
{
	CONV_NEED_MORE_SRC   = 0x00001,
	CONV_NEED_MORE_DST   = 0x00002,
	CONV_ILLFORMED_CHARS = 0x00004
};

// maximum number of single-byte characters that can be required to be translated into single wide character
#define MAX_MB_CHARS_PER_WCHAR	6

// U+0000..U+D7FF, U+E000..U+10FFFF
#define WCHAR_IS_VALID(c)    ( (((unsigned int)(c)) <= 0xd7ff) || (((unsigned int)(c)) >=0xe000 && ((unsigned int)(c)) <= 0x10ffff ) )


/*
The following blocks are dedicated specifically to combining characters:
Combining Diacritical Marks (0300 - 036F), since version 1.0, with modifications in subsequent versions down to 4.1
Combining Diacritical Marks Extended (1AB0 - 1AFF), version 7.0
Combining Diacritical Marks Supplement (1DC0 - 1DFF), versions 4.1 to 5.2
Combining Diacritical Marks for Symbols (20D0 - 20FF), since version 1.0, with modifications in subsequent versions down to 5.1
*/
#define WCHAR_IS_COMBINING(c)  ( (((unsigned int)(c)) >= 0x0300 && ((unsigned int)(c)) <= 0x036f) || \
				(((unsigned int)(c)) >= 0x1ab0 && ((unsigned int)(c)) <= 0x1aff) || \
				(((unsigned int)(c)) >= 0x1dc0 && ((unsigned int)(c)) <= 0x1dff) || \
				(((unsigned int)(c)) >= 0x20d0 && ((unsigned int)(c)) <= 0x20ff) || \
				(((unsigned int)(c)) >= 0xFE20 && ((unsigned int)(c)) <= 0xFE2F) )

#define WCHAR_IS_PSEUDOGRAPHIC(c)  ( (((unsigned int)(c)) >= 0x2500 && ((unsigned int)(c)) <= 0x259f))

/*
 Wide2MB uses this to represent untranslateble wchar_t
*/
#define CHAR_REPLACEMENT		0x07 //0x0000FFFD
#define WCHAR_REPLACEMENT		((wchar_t)CHAR_REPLACEMENT)

/*
 MB2Wide prepends by this hex value of unprocessable bytes
 Wide2MB reverts such escaped sequences to original values
*/
#define WCHAR_ESCAPING ((wchar_t)0xE5CA) /* 0xE5CA belongs to UTF Private Area */


//#define ESCAPING_CHAR 0x1a
