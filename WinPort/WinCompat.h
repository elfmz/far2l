#pragma once
#ifndef FAR_PYTHON_GEN
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <memory.h>
#include <wctype.h>
#include <limits.h>

#ifdef __linux__
// for PATH_MAX
# include <linux/limits.h>
#endif
#endif /* FAR_PYTHON_GEN */

#define ELFMZ_WINPORT

#ifdef _WIN32
typedef __int64 int64_t;
typedef unsigned __int64 uint64_t;

#define FOPEN_READ  "rb"
#define FOPEN_WRITE "wb"

#else
#ifndef FAR_PYTHON_GEN
# include <limits.h>
# include <sys/types.h>
# include <sys/stat.h>
# include <unistd.h>
# include <wchar.h>
# ifdef __cplusplus
#  define SHAREDSYMBOL extern "C" __attribute__ ((visibility("default")))
# else
#  define SHAREDSYMBOL __attribute__ ((visibility("default")))
# endif
#endif /* FAR_PYTHON_GEN */

# define FAR
# define FARPROC
# define OUT
# define IN
# define OPTIONAL
# define _In_
# define _Out_
# define _Inout_
# define _Out_opt_
# define _Inout_opt_
# define _Reserved_
# define _countof(x) (sizeof(x)/ sizeof(x[0]))

#define _wcsdup wcsdup
#define _strdup strdup
#define _close sdc_close
#define _mkdir(dir) sdc_mkdir(dir, 0775)
#define _rmdir sdc_rmdir
#define _open sdc_open
#define _getcwd sdc_getcwd
#define _chdir sdc_chdir
#define _write sdc_write
#define _read sdc_read
#define _lseek sdc_lseek
#define _chsize sdc_ftruncate
#define _itoa itoa

#ifndef __HAIKU__
#define __cdecl
#define __stdcall
#endif
#define _export
#define _cdecl

#ifdef __DragonFly__
#define ENODATA 87		/* Attribute not found */
/* This is the same as the MacOS X definition of UF_HIDDEN. */
#define UF_HIDDEN	0x00008000	/* file is hidden */
#endif

#define _UI64_MAX    0xffffffffffffffff

#define FOPEN_READ  "r"
#define FOPEN_WRITE "w"
#endif


#ifndef OCCASIONAL_WINDOWS_H
typedef uint32_t ULONG;
typedef unsigned int UINT;
typedef unsigned short USHORT;
typedef int LONG;
typedef int INT, *PINT, *LPINT;
typedef short SHORT;
typedef long long LONGLONG;
typedef unsigned long long ULONGLONG;
typedef void VOID;

typedef size_t SIZE_T;

typedef char CCHAR;          // winnt
typedef short CSHORT;
typedef ULONG CLONG;

typedef CCHAR *PCCHAR;
typedef CSHORT *PCSHORT;
typedef CLONG *PCLONG;

typedef ULONG DWORD;
typedef USHORT WORD;

typedef unsigned long long DWORD64, *PDWORD64;
typedef int64_t INT64, *PINT64;
typedef uint64_t UINT64, *PUINT64;
typedef int64_t LONG64, *PLONG64;
typedef uint64_t ULONG64, *PULONG64;

#if defined(__LP64__) || defined(_LP64)
typedef INT64 INT_PTR;
typedef UINT64 UINT_PTR;
typedef DWORD64 DWORD_PTR;
typedef LONG64 LONG_PTR;
typedef ULONG64 ULONG_PTR;
#else
typedef INT INT_PTR;
typedef UINT UINT_PTR;
typedef DWORD DWORD_PTR;
typedef LONG LONG_PTR;
typedef ULONG ULONG_PTR;
#endif

typedef const char *LPCSTR;
typedef char *LPSTR;
typedef char *PSTR;

typedef const wchar_t *LPCWSTR;
typedef wchar_t *LPWSTR;
typedef wchar_t *PWSTR;

#define LPCTSTR LPCWSTR
#define LPTSTR LPWSTR

typedef wchar_t WCHAR;
typedef char CHAR;
typedef unsigned char UCHAR;
typedef unsigned char BYTE;
typedef BYTE *PBYTE;
typedef BYTE *LPBYTE;

typedef DWORD *LPDWORD;
typedef DWORD *PDWORD;
typedef ULONG *PULONG;
typedef LONG *PLONG;
typedef WORD *LPWORD;
typedef WORD *PWORD;
typedef USHORT *PUSHORT;
typedef SHORT *PSHORT;

typedef LONGLONG *PLONGLONG;
typedef ULONGLONG *PULONGLONG;

#define TCHAR WCHAR 

#define CONST const

typedef int BOOL;
typedef UCHAR BOOLEAN;
typedef BOOL *LPBOOL, *PBOOL;

typedef void *HANDLE;
typedef void *PVOID;
typedef void *LPVOID;
typedef const void *LPCVOID;

typedef ULONG LCID;         // winnt
typedef PULONG PLCID;       // winnt
typedef USHORT LANGID;      // winnt


typedef HANDLE HKEY;
typedef struct _OVERLAPPED *LPOVERLAPPED;
typedef HKEY *PHKEY;

typedef DWORD ACCESS_MASK;
typedef ACCESS_MASK *PACCESS_MASK;
typedef ACCESS_MASK REGSAM;

typedef int HRESULT;


#ifndef _T
# define _T(x) L##x
#endif

#ifndef TEXT
# define TEXT(x) L##x
#endif

#define SEVERITY_SUCCESS    0
#define SEVERITY_ERROR      1

#define FACILITY_ITF        4


#define HRESULT_SEVERITY(hr)  (((hr) >> 31) & 0x1)
#define SCODE_SEVERITY(sc)    (((sc) >> 31) & 0x1)

#define HRESULT_FACILITY(hr)  (((hr) >> 16) & 0x1fff)
#define SCODE_FACILITY(sc)    (((sc) >> 16) & 0x1fff)

#define HRESULT_CODE(hr)    ((hr) & 0xFFFF)
#define SCODE_CODE(sc)      ((sc) & 0xFFFF)

#define IS_ERROR(Status) (((unsigned long)(Status)) >> 31 == SEVERITY_ERROR)

#define FAILED(hr) (((HRESULT)(hr)) < 0)

#define SUCCEEDED(hr) (((HRESULT)(hr)) >= 0)


#define MAKE_HRESULT(sev,fac,code) \
    ((HRESULT) (((unsigned long)(sev)<<31) | ((unsigned long)(fac)<<16) | ((unsigned long)(code))) )
#define MAKE_SCODE(sev,fac,code) \
    ((SCODE) (((unsigned long)(sev)<<31) | ((unsigned long)(fac)<<16) | ((unsigned long)(code))) )


#ifdef PATH_MAX
# define MAX_PATH PATH_MAX
#else
# define MAX_PATH 0x1000
# warning "PATH_MAX not defined"
#endif

#ifdef NAME_MAX
# define MAX_NAME NAME_MAX
#else
# define MAX_NAME 255
# warning "NAME_MAX not defined"
#endif

typedef struct _FILETIME {
#if defined(__BYTE_ORDER__) && (__BYTE_ORDER__ == __ORDER_BIG_ENDIAN__)
    DWORD dwHighDateTime;
    DWORD dwLowDateTime;
#else
    DWORD dwLowDateTime;
    DWORD dwHighDateTime;
#endif
} FILETIME, *PFILETIME, *LPFILETIME;

typedef union _LARGE_INTEGER {
#if defined(__BYTE_ORDER__) && (__BYTE_ORDER__ == __ORDER_BIG_ENDIAN__)
    struct {
        LONG HighPart;
        DWORD LowPart;
    } u;
    struct {
        LONG HighPart;
        DWORD LowPart;
    };
#else
    struct {
        DWORD LowPart;
        LONG HighPart;
    } u;
    struct {
        DWORD LowPart;
        LONG HighPart;
    };
#endif
    LONGLONG QuadPart;
} LARGE_INTEGER;
typedef LARGE_INTEGER *PLARGE_INTEGER;

typedef union _ULARGE_INTEGER {
#if defined(__BYTE_ORDER__) && (__BYTE_ORDER__ == __ORDER_BIG_ENDIAN__)
    struct {
        DWORD HighPart;
        DWORD LowPart;
    } u;
    struct {
        DWORD HighPart;
        DWORD LowPart;
    };
#else
    struct {
        DWORD LowPart;
        DWORD HighPart;
    } u;
    struct {
        DWORD LowPart;
        DWORD HighPart;
    };
#endif
    ULONGLONG QuadPart;
} ULARGE_INTEGER;
typedef ULARGE_INTEGER *PULARGE_INTEGER;

typedef struct _SECURITY_ATTRIBUTES *LPSECURITY_ATTRIBUTES;


typedef struct _SYSTEMTIME {
    WORD wYear;
    WORD wMonth;
    WORD wDayOfWeek;
    WORD wDay;
    WORD wHour;
    WORD wMinute;
    WORD wSecond;
    WORD wMilliseconds;
} SYSTEMTIME, *PSYSTEMTIME, *LPSYSTEMTIME;


typedef struct _WIN32_FIND_DATAA {
    FILETIME ftCreationTime;
    FILETIME ftLastAccessTime;
    FILETIME ftLastWriteTime;
    uid_t UnixOwner;
    gid_t UnixGroup;
    DWORD64 UnixDevice;
    DWORD64 UnixNode;
    DWORD64 nPhysicalSize;
    DWORD64 nFileSize;
    DWORD dwFileAttributes;
    DWORD dwUnixMode;
    DWORD nHardLinks;
    CHAR  cFileName[ MAX_NAME ];
} WIN32_FIND_DATAA, *PWIN32_FIND_DATAA, *LPWIN32_FIND_DATAA;

typedef struct _WIN32_FIND_DATAW {
    FILETIME ftCreationTime;
    FILETIME ftLastAccessTime;
    FILETIME ftLastWriteTime;
    uid_t UnixOwner;
    gid_t UnixGroup;
    DWORD64 UnixDevice;
    DWORD64 UnixNode;
    DWORD64 nPhysicalSize;
    DWORD64 nFileSize;
    DWORD dwFileAttributes;
    DWORD dwUnixMode;
    DWORD nHardLinks;
    DWORD nBlockSize;
    WCHAR cFileName[ MAX_NAME ];
} WIN32_FIND_DATAW, *PWIN32_FIND_DATAW, *LPWIN32_FIND_DATAW, WIN32_FIND_DATA, *PWIN32_FIND_DATA, *LPWIN32_FIND_DATA;


//////////////////

#define CONSOLE_FULLSCREEN_MODE 1
#define CONSOLE_WINDOWED_MODE 2

typedef struct _COORD {
    SHORT X;
    SHORT Y;
} COORD, *PCOORD;

typedef struct _CONSOLE_FONT_INFO {
    DWORD  nFont;
    COORD  dwFontSize;
} CONSOLE_FONT_INFO, *PCONSOLE_FONT_INFO;

typedef struct _SMALL_RECT {
    SHORT Left;
    SHORT Top;
    SHORT Right;
    SHORT Bottom;
} SMALL_RECT, *PSMALL_RECT;

typedef struct _CONSOLE_SCREEN_BUFFER_INFO {
    COORD dwSize;
    COORD dwCursorPosition;
    DWORD64  wAttributes;
    SMALL_RECT srWindow;
    COORD dwMaximumWindowSize;
} CONSOLE_SCREEN_BUFFER_INFO, *PCONSOLE_SCREEN_BUFFER_INFO;

typedef struct _CONSOLE_CURSOR_INFO {
    DWORD  dwSize;
    BOOL   bVisible;
} CONSOLE_CURSOR_INFO, *PCONSOLE_CURSOR_INFO;

typedef DWORD64 COMP_CHAR;

typedef struct _CHAR_INFO {
    union {
        // WCHAR or result of CompositeCharRegister() can be differentiated
        // using CI_USING_COMPOSITE_CHAR() that checks presence of highest bit
        // to change this field better use CI_SET_WCHAR/CI_SET_WCATTR/CI_SET_COMPOSITE
        // that guards against unwanted sign extension if casting from wchar_t
        COMP_CHAR UnicodeChar;
        CHAR   AsciiChar;
    } Char;

    // low 16 bits - usual attributes, followed by two 24-bit RGB colors that used
    // if FOREGROUND_TRUECOLOR/BACKGROUND_TRUECOLOR defined and backend supports truecolor
    DWORD64 Attributes;
} CHAR_INFO, *PCHAR_INFO;

#define COMPOSITE_CHAR_MARK (COMP_CHAR(1) << 63)

#define CI_SET_ATTR(CI, ATTR)       { (CI).Attributes = (DWORD64)ATTR; }
#define CI_SET_WCHAR(CI, WC)        { (CI).Char.UnicodeChar = (COMP_CHAR)(uint32_t)(WC); }
#define CI_SET_COMPOSITE(CI, PWC)   { (CI).Char.UnicodeChar = WINPORT(CompositeCharRegister)(PWC); }

#define CI_SET_WCATTR(CI, WC, ATTR) {(CI).Char.UnicodeChar = (COMP_CHAR)(uint32_t)(WC); (CI).Attributes = (DWORD64)ATTR;}

#define CI_USING_COMPOSITE_CHAR(CI) ( ((CI).Char.UnicodeChar & COMPOSITE_CHAR_MARK) != 0 )
#define CI_FULL_WIDTH_CHAR(CI) ( (!CI_USING_COMPOSITE_CHAR(CI) && IsCharFullWidth((CI).Char.UnicodeChar)) \
    || (CI_USING_COMPOSITE_CHAR(CI) && IsCharFullWidth(*WINPORT(CompositeCharLookup)((CI).Char.UnicodeChar))))

#define GET_RGB_FORE(ATTR)       ((DWORD)(((ATTR) >> 16) & 0xffffff))
#define GET_RGB_BACK(ATTR)       ((DWORD)(((ATTR) >> 40) & 0xffffff))
#define SET_RGB_FORE(ATTR, RGB)  \
    ((ATTR) = ((ATTR) & 0xffffff000000ffff) | FOREGROUND_TRUECOLOR | ((((DWORD64)(RGB)) & 0xffffff) << 16))
#define SET_RGB_BACK(ATTR, RGB)  \
    ((ATTR) = ((ATTR) & 0x000000ffffffffff) | BACKGROUND_TRUECOLOR | ((((DWORD64)(RGB)) & 0xffffff) << 40))
#define SET_RGB_BOTH(ATTR, RGB_FORE, RGB_BACK)  \
    ((ATTR) = ((ATTR) & 0xffff) | FOREGROUND_TRUECOLOR | BACKGROUND_TRUECOLOR | ((((DWORD64)(RGB_FORE)) & 0xffffff) << 16) | ((((DWORD64)(RGB_BACK)) & 0xffffff) << 40) )

typedef struct _WINDOW_BUFFER_SIZE_RECORD {
    COORD dwSize;
    BOOL bDamaged; // TRUE if screen area was damaged even in frames not affected by actual size change
} WINDOW_BUFFER_SIZE_RECORD, *PWINDOW_BUFFER_SIZE_RECORD;

typedef struct _MENU_EVENT_RECORD {
    UINT dwCommandId;
} MENU_EVENT_RECORD, *PMENU_EVENT_RECORD;

typedef struct _FOCUS_EVENT_RECORD {
    BOOL bSetFocus;
} FOCUS_EVENT_RECORD, *PFOCUS_EVENT_RECORD;

typedef struct _KEY_EVENT_RECORD {
    BOOL bKeyDown;
    WORD wRepeatCount;
    WORD wVirtualKeyCode;
    WORD wVirtualScanCode;
    union {
        WCHAR UnicodeChar;
        CHAR   AsciiChar;
    } uChar;
    DWORD dwControlKeyState;
} KEY_EVENT_RECORD, *PKEY_EVENT_RECORD;

typedef struct _CALLBACK_EVENT_RECORD {
    VOID (*Function)(VOID *Context);
    VOID *Context;
} CALLBACK_EVENT_RECORD, *PCALLBACK_EVENT_RECORD;

//
// ControlKeyState flags
//

#define RIGHT_ALT_PRESSED     0x0001 // the right alt key is pressed.
#define LEFT_ALT_PRESSED      0x0002 // the left alt key is pressed.
#define RIGHT_CTRL_PRESSED    0x0004 // the right ctrl key is pressed.
#define LEFT_CTRL_PRESSED     0x0008 // the left ctrl key is pressed.
#define SHIFT_PRESSED         0x0010 // the shift key is pressed.
#define NUMLOCK_ON            0x0020 // the numlock light is on.
#define SCROLLLOCK_ON         0x0040 // the scrolllock light is on.
#define CAPSLOCK_ON           0x0080 // the capslock light is on.
#define ENHANCED_KEY          0x0100 // the key is enhanced.
#define NLS_DBCSCHAR          0x00010000 // DBCS for JPN: SBCS/DBCS mode.
#define NLS_ALPHANUMERIC      0x00000000 // DBCS for JPN: Alphanumeric mode.
#define NLS_KATAKANA          0x00020000 // DBCS for JPN: Katakana mode.
#define NLS_HIRAGANA          0x00040000 // DBCS for JPN: Hiragana mode.
#define NLS_ROMAN             0x00400000 // DBCS for JPN: Roman/Noroman mode.
#define NLS_IME_CONVERSION    0x00800000 // DBCS for JPN: IME conversion.
#define NLS_IME_DISABLE       0x20000000 // DBCS for JPN: IME enable/disable.


typedef struct _MOUSE_EVENT_RECORD {
    COORD dwMousePosition;
    DWORD dwButtonState;
    DWORD dwControlKeyState;
    DWORD dwEventFlags;
} MOUSE_EVENT_RECORD, *PMOUSE_EVENT_RECORD;
#define FROM_LEFT_1ST_BUTTON_PRESSED    0x0001
#define RIGHTMOST_BUTTON_PRESSED        0x0002
#define FROM_LEFT_2ND_BUTTON_PRESSED    0x0004
#define FROM_LEFT_3RD_BUTTON_PRESSED    0x0008
#define FROM_LEFT_4TH_BUTTON_PRESSED    0x0010

#define MOUSE_MOVED   0x0001
#define DOUBLE_CLICK  0x0002
#define MOUSE_WHEELED 0x0004
#define MOUSE_HWHEELED 0x0008

typedef struct _BRACKETED_PASTE {
    BOOL bStartPaste;
} BRACKETED_PASTE, *PBRACKETED_PASTE;

#define KEY_EVENT         0x0001 // Event contains key event record
#define MOUSE_EVENT       0x0002 // Event contains mouse event record
#define WINDOW_BUFFER_SIZE_EVENT 0x0004 // Event contains window change event record
#define MENU_EVENT 0x0008 // Event contains menu event record
#define FOCUS_EVENT 0x0010 // event contains focus change
#define BRACKETED_PASTE_EVENT 0x0020 // event contains bracketed paste state change
#define CALLBACK_EVENT 0x0040 // callback to be invoked when its record dequeued, its translated into NOOP_EVENT when invoked
#define NOOP_EVENT 0x0080 // nothing interesting, typically injected to kick events dispatcher


typedef struct _INPUT_RECORD {
    WORD EventType;
    union {
        KEY_EVENT_RECORD KeyEvent;
        MOUSE_EVENT_RECORD MouseEvent;
        WINDOW_BUFFER_SIZE_RECORD WindowBufferSizeEvent;
        MENU_EVENT_RECORD MenuEvent;
        FOCUS_EVENT_RECORD FocusEvent;
        BRACKETED_PASTE BracketedPaste;
        CALLBACK_EVENT_RECORD CallbackEvent;
    } Event;
} INPUT_RECORD, *PINPUT_RECORD;



#define FOREGROUND_BLUE      0x0001 // text color contains blue.
#define FOREGROUND_GREEN     0x0002 // text color contains green.
#define FOREGROUND_RED       0x0004 // text color contains red.
#define FOREGROUND_INTENSITY 0x0008 // text color is intensified.
#define BACKGROUND_BLUE      0x0010 // background color contains blue.
#define BACKGROUND_GREEN     0x0020 // background color contains green.
#define BACKGROUND_RED       0x0040 // background color contains red.
#define BACKGROUND_INTENSITY 0x0080 // background color is intensified.
#define FOREGROUND_TRUECOLOR    0x0100 // Use 24 bit RGB colors set by SET_RGB_FORE
#define BACKGROUND_TRUECOLOR    0x0200 // Use 24 bit RGB colors set by SET_RGB_BACK
#define COMMON_LVB_REVERSE_VIDEO   0x4000 // Reverse fore/back ground attribute.
#define COMMON_LVB_UNDERSCORE      0x8000 // Underscore.
#define COMMON_LVB_STRIKEOUT       0x2000 // Striekout.

// Constants below not implemented and their bit values are reserved and must be zero-inited
// #define COMMON_LVB_GRID_HORIZONTAL
// #define COMMON_LVB_GRID_LVERTICAL
// #define COMMON_LVB_GRID_RVERTICAL
// #define COMMON_LVB_SBCSDBCS



//
//  String Flags.
//
#define NORM_IGNORECASE           0x00000001  // ignore case
#define NORM_IGNORENONSPACE       0x00000002  // ignore nonspacing chars
#define NORM_IGNORESYMBOLS        0x00000004  // ignore symbols
#define NORM_STOP_ON_NULL         0x10000000  // TODO!!!

#define LINGUISTIC_IGNORECASE     0x00000010  // linguistically appropriate 'ignore case'
#define LINGUISTIC_IGNOREDIACRITIC 0x00000020  // linguistically appropriate 'ignore nonspace'

#define NORM_IGNOREKANATYPE       0x00010000  // ignore kanatype
#define NORM_IGNOREWIDTH          0x00020000  // ignore width
#define NORM_LINGUISTIC_CASING    0x08000000  // use linguistic rules for casing

#define SORT_STRINGSORT           0x00001000  // use string sort method


#define CP_ACP                    0           // default to ANSI code page
#define CP_OEMCP                  1           // default to OEM  code page
#define CP_MACCP                  2           // default to MAC  code page
#define CP_THREAD_ACP             3           // current thread's ANSI code page
#define CP_SYMBOL                 42          // SYMBOL translations
#define CP_KOI8R                  20866       // UTF-7 translation
#define CP_UTF7                   65000       // UTF-7 translation
#define CP_UTF8                   65001       // UTF-8 translation
#define CP_UTF16LE                1200        // UTF-16 translation
#define CP_UTF16BE                1201        // UTF-16 translation
#define CP_UTF32LE                61200
#define CP_UTF32BE                61201

#if (__WCHAR_MAX__ > 0xffff)
# define CP_WIDE_LE CP_UTF32LE
# define CP_WIDE_BE CP_UTF32BE
# define WCHAR_REVERSE(w) ()
#else
# define CP_WIDE_LE CP_UTF16LE
# define CP_WIDE_BE CP_UTF16BE
#endif

#define CSTR_LESS_THAN            1           // string 1 less than string 2
#define CSTR_EQUAL                2           // string 1 equal to string 2
#define CSTR_GREATER_THAN         3           // string 1 greater than string 2

#define WINAPI
#define WINAPIV
#define CALLBACK
#define PASCAL


typedef struct _UNICODE_STRING {
    USHORT Length;
    USHORT MaximumLength;
    PWSTR  Buffer;
} UNICODE_STRING, *PUNICODE_STRING;

#define TRUE 1
#define FALSE 0

typedef struct _GUID {
    uint32_t Data1;
    uint16_t Data2;
    uint16_t Data3;
    uint8_t Data4[ 8 ];
} GUID, IID;

typedef struct tagRECT {
    LONG left;
    LONG top;
    LONG right;
    LONG bottom;
} RECT;


#define DRIVE_UNKNOWN     0
#define DRIVE_NO_ROOT_DIR 1
#define DRIVE_REMOVABLE   2
#define DRIVE_FIXED       3
#define DRIVE_REMOTE      4
#define DRIVE_CDROM       5
#define DRIVE_RAMDISK     6

#define FILE_BEGIN           0
#define FILE_CURRENT         1
#define FILE_END             2


#define INVALID_HANDLE_VALUE ((HANDLE)(LONG_PTR)-1)

#define MB_PRECOMPOSED            0x00000001  // use precomposed chars
#define MB_COMPOSITE              0x00000002  // use composite chars
#define MB_USEGLYPHCHARS          0x00000004  // use glyph chars, not ctrl chars
#define MB_ERR_INVALID_CHARS      0x00000008  // error for invalid chars

#define WC_COMPOSITECHECK         0x00000200  // convert composite to precomposed
#define WC_DISCARDNS              0x00000010  // discard non-spacing chars
#define WC_SEPCHARS               0x00000020  // generate separate chars
#define WC_DEFAULTCHAR            0x00000040  // replace w/ default char

#define WC_NO_BEST_FIT_CHARS      0x00000400  // do not use best fit chars
#define WC_ERR_INVALID_CHARS      0x00000080  // error for invalid chars
/*
 * Virtual Keys, Standard Set
 */
#define VK_LBUTTON        0x01
#define VK_RBUTTON        0x02
#define VK_CANCEL         0x03
#define VK_MBUTTON        0x04    /* NOT contiguous with L & RBUTTON */

#define VK_XBUTTON1       0x05    /* NOT contiguous with L & RBUTTON */
#define VK_XBUTTON2       0x06    /* NOT contiguous with L & RBUTTON */

/*
 * 0x07 : unassigned
 */

#define VK_BACK           0x08
#define VK_TAB            0x09

/*
 * 0x0A - 0x0B : reserved
 */

#define VK_CLEAR          0x0C
#define VK_RETURN         0x0D

#define VK_SHIFT          0x10
#define VK_CONTROL        0x11
#define VK_MENU           0x12
#define VK_PAUSE          0x13
#define VK_CAPITAL        0x14

#define VK_KANA           0x15
#define VK_HANGEUL        0x15  /* old name - should be here for compatibility */
#define VK_HANGUL         0x15
#define VK_JUNJA          0x17
#define VK_FINAL          0x18
#define VK_HANJA          0x19
#define VK_KANJI          0x19

#define VK_ESCAPE         0x1B

#define VK_CONVERT        0x1C
#define VK_NONCONVERT     0x1D
#define VK_ACCEPT         0x1E
#define VK_MODECHANGE     0x1F

#define VK_SPACE          0x20
#define VK_PRIOR          0x21
#define VK_NEXT           0x22
#define VK_END            0x23
#define VK_HOME           0x24
#define VK_LEFT           0x25
#define VK_UP             0x26
#define VK_RIGHT          0x27
#define VK_DOWN           0x28
#define VK_SELECT         0x29
#define VK_PRINT          0x2A
#define VK_EXECUTE        0x2B
#define VK_SNAPSHOT       0x2C
#define VK_INSERT         0x2D
#define VK_DELETE         0x2E
#define VK_HELP           0x2F

/*
 * VK_0 - VK_9 are the same as ASCII '0' - '9' (0x30 - 0x39)
 * 0x40 : unassigned
 * VK_A - VK_Z are the same as ASCII 'A' - 'Z' (0x41 - 0x5A)
 */

#define VK_LWIN           0x5B
#define VK_RWIN           0x5C
#define VK_APPS           0x5D

/*
 * 0x5E : reserved
 */

#define VK_SLEEP          0x5F

#define VK_NUMPAD0        0x60
#define VK_NUMPAD1        0x61
#define VK_NUMPAD2        0x62
#define VK_NUMPAD3        0x63
#define VK_NUMPAD4        0x64
#define VK_NUMPAD5        0x65
#define VK_NUMPAD6        0x66
#define VK_NUMPAD7        0x67
#define VK_NUMPAD8        0x68
#define VK_NUMPAD9        0x69
#define VK_MULTIPLY       0x6A
#define VK_ADD            0x6B
#define VK_SEPARATOR      0x6C
#define VK_SUBTRACT       0x6D
#define VK_DECIMAL        0x6E
#define VK_DIVIDE         0x6F
#define VK_F1             0x70
#define VK_F2             0x71
#define VK_F3             0x72
#define VK_F4             0x73
#define VK_F5             0x74
#define VK_F6             0x75
#define VK_F7             0x76
#define VK_F8             0x77
#define VK_F9             0x78
#define VK_F10            0x79
#define VK_F11            0x7A
#define VK_F12            0x7B
#define VK_F13            0x7C
#define VK_F14            0x7D
#define VK_F15            0x7E
#define VK_F16            0x7F
#define VK_F17            0x80
#define VK_F18            0x81
#define VK_F19            0x82
#define VK_F20            0x83
#define VK_F21            0x84
#define VK_F22            0x85
#define VK_F23            0x86
#define VK_F24            0x87

/*
 * 0x88 - 0x8F : unassigned
 */

#define VK_NUMLOCK        0x90
#define VK_SCROLL         0x91

/*
 * NEC PC-9800 kbd definitions
 */
#define VK_OEM_NEC_EQUAL  0x92   // '=' key on numpad

/*
 * Fujitsu/OASYS kbd definitions
 */
#define VK_OEM_FJ_JISHO   0x92   // 'Dictionary' key
#define VK_OEM_FJ_MASSHOU 0x93   // 'Unregister word' key
#define VK_OEM_FJ_TOUROKU 0x94   // 'Register word' key
#define VK_OEM_FJ_LOYA    0x95   // 'Left OYAYUBI' key
#define VK_OEM_FJ_ROYA    0x96   // 'Right OYAYUBI' key

/*
 * 0x97 - 0x9F : unassigned
 */

/*
 * VK_L* & VK_R* - left and right Alt, Ctrl and Shift virtual keys.
 * Used only as parameters to GetAsyncKeyState() and GetKeyState().
 * No other API or message will distinguish left and right keys in this way.
 */
#define VK_LSHIFT         0xA0
#define VK_RSHIFT         0xA1
#define VK_LCONTROL       0xA2
#define VK_RCONTROL       0xA3
#define VK_LMENU          0xA4
#define VK_RMENU          0xA5

#define VK_BROWSER_BACK        0xA6
#define VK_BROWSER_FORWARD     0xA7
#define VK_BROWSER_REFRESH     0xA8
#define VK_BROWSER_STOP        0xA9
#define VK_BROWSER_SEARCH      0xAA
#define VK_BROWSER_FAVORITES   0xAB
#define VK_BROWSER_HOME        0xAC

#define VK_VOLUME_MUTE         0xAD
#define VK_VOLUME_DOWN         0xAE
#define VK_VOLUME_UP           0xAF
#define VK_MEDIA_NEXT_TRACK    0xB0
#define VK_MEDIA_PREV_TRACK    0xB1
#define VK_MEDIA_STOP          0xB2
#define VK_MEDIA_PLAY_PAUSE    0xB3
#define VK_LAUNCH_MAIL         0xB4
#define VK_LAUNCH_MEDIA_SELECT 0xB5
#define VK_LAUNCH_APP1         0xB6
#define VK_LAUNCH_APP2         0xB7

/*
 * 0xB8 - 0xB9 : reserved
 */

#define VK_OEM_1          0xBA   // ';:' for US
#define VK_OEM_PLUS       0xBB   // '+' any country
#define VK_OEM_COMMA      0xBC   // ',' any country
#define VK_OEM_MINUS      0xBD   // '-' any country
#define VK_OEM_PERIOD     0xBE   // '.' any country
#define VK_OEM_2          0xBF   // '/?' for US
#define VK_OEM_3          0xC0   // '`~' for US

/*
 * 0xC1 - 0xD7 : reserved
 */

/*
 * 0xD8 - 0xDA : unassigned
 */

#define VK_OEM_4          0xDB  //  '[{' for US
#define VK_OEM_5          0xDC  //  '\|' for US
#define VK_OEM_6          0xDD  //  ']}' for US
#define VK_OEM_7          0xDE  //  ''"' for US
#define VK_OEM_8          0xDF

/*
 * 0xE0 : reserved
 */

/*
 * Various extended or enhanced keyboards
 */
#define VK_OEM_AX         0xE1  //  'AX' key on Japanese AX kbd
#define VK_OEM_102        0xE2  //  "<>" or "\|" on RT 102-key kbd.
#define VK_ICO_HELP       0xE3  //  Help key on ICO
#define VK_ICO_00         0xE4  //  00 key on ICO

#define VK_PROCESSKEY     0xE5

#define VK_ICO_CLEAR      0xE6


#define VK_PACKET         0xE7

#define VK_UNASSIGNED     0xE8

/*
 * 0xE8 : unassigned
 */

/*
 * Nokia/Ericsson definitions
 */
#define VK_OEM_RESET      0xE9
#define VK_OEM_JUMP       0xEA
#define VK_OEM_PA1        0xEB
#define VK_OEM_PA2        0xEC
#define VK_OEM_PA3        0xED
#define VK_OEM_WSCTRL     0xEE
#define VK_OEM_CUSEL      0xEF
#define VK_OEM_ATTN       0xF0
#define VK_OEM_FINISH     0xF1
#define VK_OEM_COPY       0xF2
#define VK_OEM_AUTO       0xF3
#define VK_OEM_ENLW       0xF4
#define VK_OEM_BACKTAB    0xF5

#define VK_ATTN           0xF6
#define VK_CRSEL          0xF7
#define VK_EXSEL          0xF8
#define VK_EREOF          0xF9
#define VK_PLAY           0xFA
#define VK_ZOOM           0xFB
#define VK_NONAME         0xFC
#define VK_PA1            0xFD
#define VK_OEM_CLEAR      0xFE

#define KEYEVENTF_EXTENDEDKEY 0x0001
#define KEYEVENTF_KEYUP       0x0002
#define KEYEVENTF_UNICODE     0x0004
#define KEYEVENTF_SCANCODE    0x0008

typedef void *HMODULE;

#define MAKELANGID(p, s)       ((((WORD  )(s)) << 10) | (WORD  )(p))
#define PRIMARYLANGID(lgid)    ((WORD  )(lgid) & 0x3ff)
#define SUBLANGID(lgid)        ((WORD  )(lgid) >> 10)


#define MAKEWORD(a, b)      ((WORD)(((BYTE)(((DWORD_PTR)(a)) & 0xff)) | ((WORD)((BYTE)(((DWORD_PTR)(b)) & 0xff))) << 8))
#define MAKELONG(a, b)      ((LONG)(((WORD)(((DWORD_PTR)(a)) & 0xffff)) | ((DWORD)((WORD)(((DWORD_PTR)(b)) & 0xffff))) << 16))
#define LOWORD(l)           ((WORD)(((DWORD_PTR)(l)) & 0xffff))
#define HIWORD(l)           ((WORD)((((DWORD_PTR)(l)) >> 16) & 0xffff))
#define LOBYTE(w)           ((BYTE)(((DWORD_PTR)(w)) & 0xff))
#define HIBYTE(w)           ((BYTE)((((DWORD_PTR)(w)) >> 8) & 0xff))

#define MAXIMUM_ALLOWED                  (0x02000000L)

#define GENERIC_READ                     (0x80000000L)
#define GENERIC_WRITE                    (0x40000000L)
#define GENERIC_EXECUTE                  (0x20000000L)
#define GENERIC_ALL                      (0x10000000L)


#define FILE_READ_DATA            ( 0x0001 )    // file & pipe
#define FILE_LIST_DIRECTORY       ( 0x0001 )    // directory

#define FILE_WRITE_DATA           ( 0x0002 )    // file & pipe
#define FILE_ADD_FILE             ( 0x0002 )    // directory

#define FILE_APPEND_DATA          ( 0x0004 )    // file
#define FILE_ADD_SUBDIRECTORY     ( 0x0004 )    // directory
#define FILE_CREATE_PIPE_INSTANCE ( 0x0004 )    // named pipe


#define FILE_READ_EA              ( 0x0008 )    // file & directory

#define FILE_WRITE_EA             ( 0x0010 )    // file & directory

#define FILE_EXECUTE              ( 0x0020 )    // file
#define FILE_TRAVERSE             ( 0x0020 )    // directory

#define FILE_DELETE_CHILD         ( 0x0040 )    // directory

#define FILE_READ_ATTRIBUTES      ( 0x0080 )    // all

#define FILE_WRITE_ATTRIBUTES     ( 0x0100 )    // all

#define FILE_ALL_ACCESS (STANDARD_RIGHTS_REQUIRED | SYNCHRONIZE | 0x1FF)


#define FILE_SHARE_READ                 0x00000001
#define FILE_SHARE_WRITE                0x00000002
#define FILE_SHARE_DELETE               0x00000004
#define FILE_ATTRIBUTE_READONLY             0x00000001
#define FILE_ATTRIBUTE_HIDDEN               0x00000002
#define FILE_ATTRIBUTE_SYSTEM               0x00000004
#define FILE_ATTRIBUTE_DIRECTORY            0x00000010
#define FILE_ATTRIBUTE_ARCHIVE              0x00000020
#define FILE_ATTRIBUTE_DEVICE_BLOCK         0x00000040
#define FILE_ATTRIBUTE_NORMAL               0x00000080
#define FILE_ATTRIBUTE_TEMPORARY            0x00000100
#define FILE_ATTRIBUTE_SPARSE_FILE          0x00000200
#define FILE_ATTRIBUTE_REPARSE_POINT        0x00000400
#define FILE_ATTRIBUTE_COMPRESSED           0x00000800
#define FILE_ATTRIBUTE_OFFLINE              0x00001000
#define FILE_ATTRIBUTE_NOT_CONTENT_INDEXED  0x00002000
#define FILE_ATTRIBUTE_ENCRYPTED            0x00004000
#define FILE_ATTRIBUTE_INTEGRITY_STREAM     0x00008000
#define FILE_ATTRIBUTE_VIRTUAL              0x00010000
#define FILE_ATTRIBUTE_NO_SCRUB_DATA        0x00020000
#define FILE_ATTRIBUTE_BROKEN               0x00200000
#define FILE_ATTRIBUTE_EXECUTABLE           0x00400000
#define FILE_ATTRIBUTE_DEVICE_CHAR          0x00800000
#define FILE_ATTRIBUTE_DEVICE_FIFO          0x01000000
#define FILE_ATTRIBUTE_DEVICE_SOCK          0x02000000
#define FILE_ATTRIBUTE_HARDLINKS            0x08000000


#define FILE_ATTRIBUTE_DEVICE               (FILE_ATTRIBUTE_DEVICE_CHAR | FILE_ATTRIBUTE_DEVICE_BLOCK | FILE_ATTRIBUTE_DEVICE_FIFO | FILE_ATTRIBUTE_DEVICE_SOCK)

// mask for those attributes that matches to usual win32 ones, others are specific to WinPort
#define COMPATIBLE_FILE_ATTRIBUTES          ( FILE_ATTRIBUTE_READONLY | FILE_ATTRIBUTE_HIDDEN | FILE_ATTRIBUTE_SYSTEM \
                                            | FILE_ATTRIBUTE_DIRECTORY | FILE_ATTRIBUTE_ARCHIVE | FILE_ATTRIBUTE_DEVICE_BLOCK \
                                            | FILE_ATTRIBUTE_NORMAL | FILE_ATTRIBUTE_TEMPORARY | FILE_ATTRIBUTE_SPARSE_FILE \
                                            | FILE_ATTRIBUTE_REPARSE_POINT | FILE_ATTRIBUTE_OFFLINE | FILE_ATTRIBUTE_NOT_CONTENT_INDEXED \
                                            | FILE_ATTRIBUTE_ENCRYPTED | FILE_ATTRIBUTE_INTEGRITY_STREAM | FILE_ATTRIBUTE_VIRTUAL \
                                            | FILE_ATTRIBUTE_NO_SCRUB_DATA )

#define FILE_FILE_COMPRESSION               0x00000010
#define FILE_SUPPORTS_SPARSE_FILES          0x00000040
#define FILE_SUPPORTS_REPARSE_POINTS        0x00000080
#define FILE_SUPPORTS_REMOTE_STORAGE        0x00000100
#define FILE_SUPPORTS_ENCRYPTION            0x00020000


#define FILE_FLAG_WRITE_THROUGH         0x80000000
#define FILE_FLAG_OVERLAPPED            0x40000000
#define FILE_FLAG_NO_BUFFERING          0x20000000
#define FILE_FLAG_RANDOM_ACCESS         0x10000000
#define FILE_FLAG_SEQUENTIAL_SCAN       0x08000000
#define FILE_FLAG_DELETE_ON_CLOSE       0x04000000
#define FILE_FLAG_BACKUP_SEMANTICS      0x02000000
#define FILE_FLAG_POSIX_SEMANTICS       0x01000000
#define FILE_FLAG_SESSION_AWARE         0x00800000
#define FILE_FLAG_OPEN_REPARSE_POINT    0x00200000
#define FILE_FLAG_OPEN_NO_RECALL        0x00100000
#define FILE_FLAG_FIRST_PIPE_INSTANCE   0x00080000

#define FILE_TYPE_UNKNOWN   0x0000
#define FILE_TYPE_DISK      0x0001
#define FILE_TYPE_CHAR      0x0002
#define FILE_TYPE_PIPE      0x0003
#define FILE_TYPE_REMOTE    0x8000


#define CREATE_NEW          1
#define CREATE_ALWAYS       2
#define OPEN_EXISTING       3
#define OPEN_ALWAYS         4
#define TRUNCATE_EXISTING   5

#define INVALID_FILE_SIZE ((DWORD)0xFFFFFFFF)
#define INVALID_FILE_SIZE64 ((DWORD64)0xFFFFFFFFFFFFFFFF)
#define INVALID_SET_FILE_POINTER ((DWORD)0xFFFFFFFF)
#define INVALID_FILE_ATTRIBUTES ((DWORD)0xFFFFFFFF)

typedef void *HKL;


#define MAKELCID(lgid, srtid)  ((DWORD)((((DWORD)((WORD  )(srtid))) << 16) |  \
                                         ((DWORD)((WORD  )(lgid)))))
#define MAKESORTLCID(lgid, srtid, ver)                                            \
                               ((DWORD)((MAKELCID(lgid, srtid)) |             \
                                    (((DWORD)((WORD  )(ver))) << 20)))
#define LANGIDFROMLCID(lcid)   ((WORD  )(lcid))
#define SORTIDFROMLCID(lcid)   ((WORD  )((((DWORD)(lcid)) >> 16) & 0xf))
#define SORTVERSIONFROMLCID(lcid)  ((WORD  )((((DWORD)(lcid)) >> 20) & 0xf))


#define LANG_SYSTEM_DEFAULT    (MAKELANGID(LANG_NEUTRAL, SUBLANG_SYS_DEFAULT))
#define LANG_USER_DEFAULT      (MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT))

#define LOCALE_SYSTEM_DEFAULT  (MAKELCID(LANG_SYSTEM_DEFAULT, SORT_DEFAULT))
#define LOCALE_USER_DEFAULT    (MAKELCID(LANG_USER_DEFAULT, SORT_DEFAULT))

#define LANG_NEUTRAL                     0x00
#define LANG_INVARIANT                   0x7f
#define SUBLANG_NEUTRAL                             0x00    // language neutral
#define SUBLANG_DEFAULT                             0x01    // user default
#define SUBLANG_SYS_DEFAULT                         0x02    // system default

#define SORT_DEFAULT                     0x0     // sorting default

#define SORT_INVARIANT_MATH              0x1     // Invariant (Mathematical Symbols)

#define SORT_JAPANESE_XJIS               0x0     // Japanese XJIS order
#define SORT_JAPANESE_UNICODE            0x1     // Japanese Unicode order (no longer supported)
#define SORT_JAPANESE_RADICALSTROKE      0x4     // Japanese radical/stroke order

#define SORT_CHINESE_BIG5                0x0     // Chinese BIG5 order
#define SORT_CHINESE_PRCP                0x0     // PRC Chinese Phonetic order
#define SORT_CHINESE_UNICODE             0x1     // Chinese Unicode order (no longer supported)
#define SORT_CHINESE_PRC                 0x2     // PRC Chinese Stroke Count order
#define SORT_CHINESE_BOPOMOFO            0x3     // Traditional Chinese Bopomofo order
#define SORT_CHINESE_RADICALSTROKE       0x4     // Traditional Chinese radical/stroke order.

#define SORT_KOREAN_KSC                  0x0     // Korean KSC order
#define SORT_KOREAN_UNICODE              0x1     // Korean Unicode order (no longer supported)

#define SORT_GERMAN_PHONE_BOOK           0x1     // German Phone Book order

#define SORT_HUNGARIAN_DEFAULT           0x0     // Hungarian Default order
#define SORT_HUNGARIAN_TECHNICAL         0x1     // Hungarian Technical order

#define SORT_GEORGIAN_TRADITIONAL        0x0     // Georgian Traditional order
#define SORT_GEORGIAN_MODERN             0x1     // Georgian Modern order


#define EXTERN_C extern "C"

#ifdef INITGUID
#define DEFINE_GUID(name, l, w1, w2, b1, b2, b3, b4, b5, b6, b7, b8) \
    EXTERN_C const GUID name \
                = { l, w1, w2, { b1, b2,  b3,  b4,  b5,  b6,  b7,  b8 } }
#else
#define DEFINE_GUID(name, l, w1, w2, b1, b2, b3, b4, b5, b6, b7, b8) \
    EXTERN_C const GUID name
#endif // INITGUID

#define ARRAYSIZE(A) (sizeof(A)/sizeof((A)[0]))

#define REG_NONE                    ( 0 )   // No value type
#define REG_SZ                      ( 1 )   // Wide nul terminated string
#define REG_EXPAND_SZ               ( 2 )   // Wide nul terminated string
                                            // (with environment variable references)
#define REG_BINARY                  ( 3 )   // Free form binary
#define REG_DWORD                   ( 4 )   // 32-bit number
#define REG_DWORD_LITTLE_ENDIAN     ( 4 )   // 32-bit number (same as REG_DWORD)
#define REG_DWORD_BIG_ENDIAN        ( 5 )   // 32-bit number
#define REG_LINK                    ( 6 )   // Symbolic Link (unicode)
#define REG_MULTI_SZ                ( 7 )   // Multiple Wide strings
#define REG_RESOURCE_LIST           ( 8 )   // Resource list in the resource map
#define REG_FULL_RESOURCE_DESCRIPTOR ( 9 )  // Resource list in the hardware description
#define REG_RESOURCE_REQUIREMENTS_LIST ( 10 )
#define REG_QWORD                   ( 11 )  // 64-bit number
#define REG_QWORD_LITTLE_ENDIAN     ( 11 )  // 64-bit number (same as REG_QWORD)

#define REG_SZ_MB                    ( 101 )   // UTF8 nul terminated string
#define REG_EXPAND_SZ_MB             ( 102 )   // UTF8 nul terminated string
#define REG_MULTI_SZ_MB              ( 107 )   // Multiple UTF8 strings


#define WAIT_TIMEOUT                     258L


#define NO_ERROR                         0L
#define ERROR_SUCCESS                    0L

#define ERROR_BAD_NET_RESP               EBADE
#define ERROR_BAD_DRIVER_LEVEL           EBADRQC
#define ERROR_TIMEOUT                    ETIME
#define ERROR_NO_MORE_ITEMS              ENODATA
#define ERROR_MORE_DATA                  EMSGSIZE
#define ERROR_CALL_NOT_IMPLEMENTED       EOPNOTSUPP
#define ERROR_ALREADY_EXISTS             EEXIST
#define ERROR_INVALID_PARAMETER          EINVAL
#define ERROR_NOT_SAME_DEVICE            EXDEV
#define ERROR_CANCELLED                  ECANCELED
#define ERROR_ACCESS_DENIED              EACCES
#define ERROR_BUFFER_OVERFLOW            EOVERFLOW
#define ERROR_WRITE_FAULT                EIO
#define ERROR_DISK_FULL                  ENOSPC
#define ERROR_FILE_NOT_FOUND             ENOENT
#define ERROR_PATH_NOT_FOUND             ENOTDIR
#define ERROR_INVALID_HANDLE             EBADF
#define ERROR_SHARING_VIOLATION          EBUSY
#define ERROR_NOT_SUPPORTED              ENOTSUP
#define ERROR_NO_MORE_FILES              ENOLINK
#define ERROR_INSUFFICIENT_BUFFER        ENOBUFS
#define ERROR_NO_UNICODE_TRANSLATION     EILSEQ
#define ERROR_DIRECTORY                  EISDIR
#define ERROR_INVALID_NAME               ENAMETOOLONG
#define ERROR_FILE_EXISTS                EEXIST
#define ERROR_OUTOFMEMORY                ENOMEM
#define ERROR_DIR_NOT_EMPTY              ENOTEMPTY
#define ERROR_INVALID_FLAGS              EDOM

#define HKEY_CLASSES_ROOT                   (( HKEY ) (ULONG_PTR)((LONG)0x80000000) )
#define HKEY_CURRENT_USER                   (( HKEY ) (ULONG_PTR)((LONG)0x80000001) )
#define HKEY_LOCAL_MACHINE                  (( HKEY ) (ULONG_PTR)((LONG)0x80000002) )
#define HKEY_USERS                          (( HKEY ) (ULONG_PTR)((LONG)0x80000003) )
#define HKEY_PERFORMANCE_DATA               (( HKEY ) (ULONG_PTR)((LONG)0x80000004) )
#define HKEY_PERFORMANCE_TEXT               (( HKEY ) (ULONG_PTR)((LONG)0x80000050) )
#define HKEY_PERFORMANCE_NLSTEXT            (( HKEY ) (ULONG_PTR)((LONG)0x80000060) )


#define COMPRESSION_FORMAT_NONE          (0x0000)
#define COMPRESSION_FORMAT_DEFAULT       (0x0001)
#define COMPRESSION_FORMAT_LZNT1         (0x0002)
#define COMPRESSION_FORMAT_XPRESS        (0x0003)
#define COMPRESSION_FORMAT_XPRESS_HUFF   (0x0004)
#define COMPRESSION_ENGINE_STANDARD      (0x0000)
#define COMPRESSION_ENGINE_MAXIMUM       (0x0100)
#define COMPRESSION_ENGINE_HIBER         (0x0200)


#define IO_REPARSE_TAG_SYMLINK                  (0xA000000CL)


#define CTRL_C_EVENT        0
#define CTRL_BREAK_EVENT    1
#define CTRL_CLOSE_EVENT    2
// 3 is reserved!
// 4 is reserved!
#define CTRL_LOGOFF_EVENT   5
#define CTRL_SHUTDOWN_EVENT 6

#define INFINITE            0xFFFFFFFF  // Infinite timeout

#define MOVEFILE_REPLACE_EXISTING       0x00000001
#define MOVEFILE_COPY_ALLOWED           0x00000002
#define MOVEFILE_DELAY_UNTIL_REBOOT     0x00000004
#define MOVEFILE_WRITE_THROUGH          0x00000008
#define MOVEFILE_CREATE_HARDLINK        0x00000010
#define MOVEFILE_FAIL_IF_NOT_TRACKABLE  0x00000020

#define BST_UNCHECKED      0x0000
#define BST_CHECKED        0x0001
#define BST_INDETERMINATE  0x0002
#define BST_PUSHED         0x0004
#define BST_FOCUS          0x0008

#define IS_TEXT_UNICODE_ASCII16               0x0001
#define IS_TEXT_UNICODE_REVERSE_ASCII16       0x0010

#define IS_TEXT_UNICODE_STATISTICS            0x0002
#define IS_TEXT_UNICODE_REVERSE_STATISTICS    0x0020

#define IS_TEXT_UNICODE_CONTROLS              0x0004
#define IS_TEXT_UNICODE_REVERSE_CONTROLS      0x0040

#define IS_TEXT_UNICODE_SIGNATURE             0x0008
#define IS_TEXT_UNICODE_REVERSE_SIGNATURE     0x0080

#define IS_TEXT_UNICODE_ILLEGAL_CHARS         0x0100
#define IS_TEXT_UNICODE_ODD_LENGTH            0x0200
#define IS_TEXT_UNICODE_DBCS_LEADBYTE         0x0400
#define IS_TEXT_UNICODE_NULL_BYTES            0x1000

#define IS_TEXT_UNICODE_UNICODE_MASK          0x000F
#define IS_TEXT_UNICODE_REVERSE_MASK          0x00F0
#define IS_TEXT_UNICODE_NOT_UNICODE_MASK      0x0F00
#define IS_TEXT_UNICODE_NOT_ASCII_MASK        0xF000

#define MAX_LEADBYTES             12          // 5 ranges, 2 bytes ea., 0 term.
#define MAX_DEFAULTCHAR           4           // single or double byte

typedef struct _cpinfo {
    UINT    MaxCharSize;                    // max length (in bytes) of a char
    BYTE    DefaultChar[MAX_DEFAULTCHAR];   // default character
    BYTE    LeadByte[MAX_LEADBYTES];        // lead byte ranges
} CPINFO, *LPCPINFO;

typedef struct _cpinfoex {
    UINT  MaxCharSize;
    BYTE  DefaultChar[MAX_DEFAULTCHAR];
    BYTE  LeadByte[MAX_LEADBYTES];
    WCHAR UnicodeDefaultChar;
    UINT  CodePage;
    TCHAR CodePageName[MAX_NAME];
} CPINFOEX, *LPCPINFOEX;

typedef BOOL (*CODEPAGE_ENUMPROCW)(LPWSTR);


#define ENABLE_PROCESSED_INPUT  0x0001
#define ENABLE_LINE_INPUT       0x0002
#define ENABLE_ECHO_INPUT       0x0004
#define ENABLE_WINDOW_INPUT     0x0008
#define ENABLE_MOUSE_INPUT      0x0010
#define ENABLE_INSERT_MODE      0x0020
#define ENABLE_QUICK_EDIT_MODE  0x0040
#define ENABLE_EXTENDED_FLAGS   0x0080
#define ENABLE_AUTO_POSITION    0x0100

//
// Output Mode flags:
//
typedef LONG NTSTATUS;
#define ENABLE_PROCESSED_OUTPUT    0x0001
#define ENABLE_WRAP_AT_EOL_OUTPUT  0x0002

#define STATUS_WAIT_0                    ((NTSTATUS)0x00000000L)    // winnt
#define STATUS_ABANDONED_WAIT_0          ((NTSTATUS)0x00000080L)    // winnt

#define WAIT_FAILED            ((DWORD)0xFFFFFFFF)
#define WAIT_OBJECT_0          ((STATUS_WAIT_0 ) + 0 )

#define WAIT_ABANDONED         ((STATUS_ABANDONED_WAIT_0 ) + 0 )
#define WAIT_ABANDONED_0       ((STATUS_ABANDONED_WAIT_0 ) + 0 )

#define WAIT_IO_COMPLETION               STATUS_USER_APC

#define DELETE                           (0x00010000L)
#define READ_CONTROL                     (0x00020000L)
#define WRITE_DAC                        (0x00040000L)
#define WRITE_OWNER                      (0x00080000L)
#define SYNCHRONIZE                      (0x00100000L)

#define STANDARD_RIGHTS_REQUIRED         (0x000F0000L)

#define STANDARD_RIGHTS_READ             (READ_CONTROL)
#define STANDARD_RIGHTS_WRITE            (READ_CONTROL)
#define STANDARD_RIGHTS_EXECUTE          (READ_CONTROL)

#define STANDARD_RIGHTS_ALL              (0x001F0000L)

#define KEY_QUERY_VALUE         (0x0001)
#define KEY_SET_VALUE           (0x0002)
#define KEY_CREATE_SUB_KEY      (0x0004)
#define KEY_ENUMERATE_SUB_KEYS  (0x0008)
#define KEY_NOTIFY              (0x0010)
#define KEY_CREATE_LINK         (0x0020)
#define KEY_WOW64_32KEY         (0x0200)
#define KEY_WOW64_64KEY         (0x0100)
#define KEY_WOW64_RES           (0x0300)


#define KEY_READ                ((STANDARD_RIGHTS_READ       |\
                                  KEY_QUERY_VALUE            |\
                                  KEY_ENUMERATE_SUB_KEYS     |\
                                  KEY_NOTIFY)                 \
                                  &                           \
                                 (~SYNCHRONIZE))


#define KEY_WRITE               ((STANDARD_RIGHTS_WRITE      |\
                                  KEY_SET_VALUE              |\
                                  KEY_CREATE_SUB_KEY)         \
                                  &                           \
                                 (~SYNCHRONIZE))

#define KEY_EXECUTE             ((KEY_READ)                   \
                                  &                           \
                                 (~SYNCHRONIZE))

#define KEY_ALL_ACCESS          ((STANDARD_RIGHTS_ALL        |\
                                  KEY_QUERY_VALUE            |\
                                  KEY_SET_VALUE              |\
                                  KEY_CREATE_SUB_KEY         |\
                                  KEY_ENUMERATE_SUB_KEYS     |\
                                  KEY_NOTIFY                 |\
                                  KEY_CREATE_LINK)            \
                                  &                           \
                                 (~SYNCHRONIZE))



#define LOAD_WITH_ALTERED_SEARCH_PATH       0x00000008

#define MAX_COMPUTERNAME_LENGTH 15

#define FILE_NOTIFY_CHANGE_FILE_NAME    0x00000001
#define FILE_NOTIFY_CHANGE_DIR_NAME     0x00000002
#define FILE_NOTIFY_CHANGE_ATTRIBUTES   0x00000004
#define FILE_NOTIFY_CHANGE_SIZE         0x00000008
#define FILE_NOTIFY_CHANGE_LAST_WRITE   0x00000010
#define FILE_NOTIFY_CHANGE_LAST_ACCESS  0x00000020
#define FILE_NOTIFY_CHANGE_CREATION     0x00000040
#define FILE_NOTIFY_CHANGE_SECURITY     0x00000100
#define FILE_ACTION_ADDED                   0x00000001
#define FILE_ACTION_REMOVED                 0x00000002
#define FILE_ACTION_MODIFIED                0x00000003
#define FILE_ACTION_RENAMED_OLD_NAME        0x00000004
#define FILE_ACTION_RENAMED_NEW_NAME        0x00000005

#ifndef MAPVK_VK_TO_VSC
#define MAPVK_VK_TO_VSC 0
#endif

#ifndef MAPVK_VK_TO_CHAR
#define MAPVK_VK_TO_CHAR 2
#endif

#define CT_CTYPE1                 0x00000001  // ctype 1 information
#define CT_CTYPE2                 0x00000002  // ctype 2 information
#define CT_CTYPE3                 0x00000004  // ctype 3 information

//
//  CType 1 Flag Bits.
//
#define C1_UPPER                  0x0001      // upper case
#define C1_LOWER                  0x0002      // lower case
#define C1_DIGIT                  0x0004      // decimal digits
#define C1_SPACE                  0x0008      // spacing characters
#define C1_PUNCT                  0x0010      // punctuation characters
#define C1_CNTRL                  0x0020      // control characters
#define C1_BLANK                  0x0040      // blank characters
#define C1_XDIGIT                 0x0080      // other digits
#define C1_ALPHA                  0x0100      // any linguistic character
#define C1_DEFINED                0x0200      // defined character


//
//  CType 2 Flag Bits.
//
#define C2_LEFTTORIGHT            0x0001      // left to right
#define C2_RIGHTTOLEFT            0x0002      // right to left

#define C2_EUROPENUMBER           0x0003      // European number, digit
#define C2_EUROPESEPARATOR        0x0004      // European numeric separator
#define C2_EUROPETERMINATOR       0x0005      // European numeric terminator
#define C2_ARABICNUMBER           0x0006      // Arabic number
#define C2_COMMONSEPARATOR        0x0007      // common numeric separator

#define C2_BLOCKSEPARATOR         0x0008      // block separator
#define C2_SEGMENTSEPARATOR       0x0009      // segment separator
#define C2_WHITESPACE             0x000A      // white space
#define C2_OTHERNEUTRAL           0x000B      // other neutrals

#define C2_NOTAPPLICABLE          0x0000      // no implicit directionality

//
//  CType 3 Flag Bits.
//
#define C3_NONSPACING             0x0001      // nonspacing character
#define C3_DIACRITIC              0x0002      // diacritic mark
#define C3_VOWELMARK              0x0004      // vowel mark
#define C3_SYMBOL                 0x0008      // symbols

#define C3_KATAKANA               0x0010      // katakana character
#define C3_HIRAGANA               0x0020      // hiragana character
#define C3_HALFWIDTH              0x0040      // half width character
#define C3_FULLWIDTH              0x0080      // full width character
#define C3_IDEOGRAPH              0x0100      // ideographic character
#define C3_KASHIDA                0x0200      // Arabic kashida character
#define C3_LEXICAL                0x0400      // lexical character
#define C3_HIGHSURROGATE          0x0800      // high surrogate code unit
#define C3_LOWSURROGATE           0x1000      // low surrogate code unit

#define C3_ALPHA                  0x8000      // any linguistic char (C1_ALPHA)

#define C3_NOTAPPLICABLE          0x0000      // ctype 3 is not applicable


#define DECLSPEC_HIDDEN

#define IS_INTRESOURCE(_r) ((((ULONG_PTR)(_r)) >> 16) == 0)

#define CREATE_SUSPENDED                  0x00000004

#define HGLOBAL     HANDLE
#define GMEM_FIXED          0x0000
#define GMEM_MOVEABLE       0x0002
#define GMEM_NOCOMPACT      0x0010
#define GMEM_NODISCARD      0x0020
#define GMEM_ZEROINIT       0x0040
#define GMEM_MODIFY         0x0080
#define GMEM_DISCARDABLE    0x0100
#define GMEM_NOT_BANKED     0x1000
#define GMEM_SHARE          0x2000
#define GMEM_DDESHARE       0x2000
#define GMEM_NOTIFY         0x4000
#define GMEM_LOWER          GMEM_NOT_BANKED
#define GMEM_VALID_FLAGS    0x7F72
#define GMEM_INVALID_HANDLE 0x8000

#define GHND                (GMEM_MOVEABLE | GMEM_ZEROINIT)
#define GPTR                (GMEM_FIXED | GMEM_ZEROINIT)

#define CF_TEXT             1
#define CF_BITMAP           2
#define CF_METAFILEPICT     3
#define CF_SYLK             4
#define CF_DIF              5
#define CF_TIFF             6
#define CF_OEMTEXT          7
#define CF_DIB              8
#define CF_PALETTE          9
#define CF_PENDATA          10
#define CF_RIFF             11
#define CF_WAVE             12
#define CF_UNICODETEXT      13
#define CF_ENHMETAFILE      14

#define REG_CREATED_NEW_KEY         (0x00000001L)   // New Registry Key created
#define REG_OPENED_EXISTING_KEY     (0x00000002L)   // Existing Key opened

typedef struct _nlsversioninfo{
    DWORD dwNLSVersionInfoSize;     // sizeof(NLSVERSIONINFO) == 32 bytes
    DWORD dwNLSVersion;
    DWORD dwDefinedVersion;         // Deprecated, use dwNLSVersion instead
    DWORD dwEffectiveId;            // Deprecated, use guidCustomVersion instead
    GUID  guidCustomVersion;        // Explicit sort version
} NLSVERSIONINFO, *LPNLSVERSIONINFO;


typedef UINT_PTR            WPARAM;
typedef LONG_PTR            LPARAM;
typedef LONG_PTR            LRESULT;

#define LCMAP_LOWERCASE           0x00000100  // lower case letters
#define LCMAP_UPPERCASE           0x00000200  // UPPER CASE LETTERS
#define LCMAP_TITLECASE           0x00000300  // Title Case Letters

#define LCMAP_SORTKEY             0x00000400  // WC sort key (normalize)
#define LCMAP_BYTEREV             0x00000800  // byte reversal

#define LCMAP_HIRAGANA            0x00100000  // map katakana to hiragana
#define LCMAP_KATAKANA            0x00200000  // map hiragana to katakana
#define LCMAP_HALFWIDTH           0x00400000  // map double byte to single byte
#define LCMAP_FULLWIDTH           0x00800000  // map single byte to double byte

#define LCMAP_LINGUISTIC_CASING   0x01000000  // use linguistic rules for casing

#define LCMAP_SIMPLIFIED_CHINESE  0x02000000  // map traditional chinese to simplified chinese
#define LCMAP_TRADITIONAL_CHINESE 0x04000000  // map simplified chinese to traditional chinese


#define LCMAP_SORTHANDLE   0x20000000
#define LCMAP_HASH         0x00040000

#define LOCALE_NOUSEROVERRIDE         0x80000000   // do not use user overrides
#define LOCALE_USE_CP_ACP             0x40000000   // use the system ACP

#define MAX_VKEY_CODE 0xffff
//#define MINSHORT    0x8000
#define MAXSHORT    0x7fff
#endif

typedef DWORD (*WINPORT_THREAD_START_ROUTINE)(LPVOID lpThreadParameter);
typedef BOOL (*WINPORT_HANDLER_ROUTINE)(  DWORD CtrlType );

#ifndef OCCASIONAL_WINDOWS_H
#define HINSTANCE HANDLE

typedef WINPORT_HANDLER_ROUTINE PHANDLER_ROUTINE;
typedef WINPORT_THREAD_START_ROUTINE LPTHREAD_START_ROUTINE, PTHREAD_START_ROUTINE;

typedef VOID (*PCONSOLE_SCROLL_CALLBACK)(PVOID pContext, HANDLE hConsole, unsigned int Width, CHAR_INFO *Chars);

#define STDMETHOD(method)        virtual HRESULT method
#define STDMETHOD_(type,method)  virtual type method
#define STDMETHODV(method)       virtual HRESULT method
#define STDMETHODV_(type,method) virtual type  method
#define PURE                    = 0
#define THIS_
#define THIS                    void
#define DECLARE_INTERFACE(iface)    struct iface
#define DECLARE_INTERFACE_(iface, baseiface)    struct iface : public baseiface

#define IS_SOCKET_NONBLOCKING_ERR(err)    ((err)==EINPROGRESS || (err)==EWOULDBLOCK)

#ifndef SOCKET
# define SOCKET int
#endif

#define SOCKET_ERROR -1
#define INVALID_SOCKET -1
#define _O_BINARY 0
#define SD_BOTH 2

#define MAKELONG(a, b)        ((LONG)(((WORD)(((DWORD_PTR)(a)) & 0xffff)) | ((DWORD)((WORD)(((DWORD_PTR)(b)) & 0xffff))) << 16))
#define MAKEWPARAM(l, h)      ((WPARAM)(DWORD)MAKELONG(l, h))
#define MAKELPARAM(l, h)      ((LPARAM)(DWORD)MAKELONG(l, h))
#define MAKELRESULT(l, h)     ((LRESULT)(DWORD)MAKELONG(l, h))

#endif

#ifdef __GNUC__
# define thread_local __thread
#elif __STDC_VERSION__ >= 201112L
# define thread_local _Thread_local
#elif defined(_MSC_VER)
# define thread_local __declspec(thread)
#else
# error Cannot define thread_local
#endif

#define CONSOLE_FKEYS_COUNT 12 // array count for SetConsoleFKeyTitles

// Virtual Scan Code for right Shift key
// This is only Virtual Scan Code that can not be translated from Virtual Key Code
// as Virtual Key Code in Windows key event records for Shift keys is always VK_SHIFT
// (not VK_LSHIFT or VK_RSHIFT). Right Shift key can be identified only using Virtual Scan Code.
// Left and right Control and Alt keys can be distinguished using ENHANCED_KEY flag, but Shift keys not.
// See also
// https://docs.vmware.com/en/VMware-Workstation-Player-for-Windows/17.0/com.vmware.player.win.using.doc/GUID-D2C43B86-32EF-44EA-A2ED-D890483D70BD.html
#define RIGHT_SHIFT_VSC 54
