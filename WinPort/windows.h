#pragma once
#include "WinCompat.h"
#include "WinPort.h"

#ifdef WINPORT_DIRECT

//force 'normal' wsprintf be declared before our macro-redefinitions will
# include <stdio.h>
# include <wchar.h>

# ifdef vswprintf
#  undef vswprintf
# endif
#define vswprintf vswprintf_ws2ls

# ifdef swprintf_ws2ls
#  undef swprintf_ws2ls
# endif
#define swprintf swprintf_ws2ls

#define    GetLargestConsoleWindowSize      WINPORT(GetLargestConsoleWindowSize)
#define    SetConsoleWindowInfo             WINPORT(SetConsoleWindowInfo)
#define    SetConsoleTitle                  WINPORT(SetConsoleTitle)
#define    GetConsoleTitle                  WINPORT(GetConsoleTitle)
#define    SetConsoleScreenBufferSize       WINPORT(SetConsoleScreenBufferSize)
#define    GetConsoleScreenBufferInfo       WINPORT(GetConsoleScreenBufferInfo)
#define    SetConsoleCursorPosition         WINPORT(SetConsoleCursorPosition)
#define    SetConsoleCursorInfo             WINPORT(SetConsoleCursorInfo)
#define    GetConsoleCursorInfo             WINPORT(GetConsoleCursorInfo)
#define    GetConsoleMode                   WINPORT(GetConsoleMode)
#define    SetConsoleMode                   WINPORT(SetConsoleMode)
#define    ScrollConsoleScreenBuffer        WINPORT(ScrollConsoleScreenBuffer)
#define    SetConsoleTextAttribute          WINPORT(SetConsoleTextAttribute)
#define    WriteConsole                     WINPORT(WriteConsole)
#define    WriteConsoleOutput               WINPORT(WriteConsoleOutput)
#define    WriteConsoleOutputCharacter      WINPORT(WriteConsoleOutputCharacter)
#define    ReadConsoleOutput                WINPORT(ReadConsoleOutput)
#define    FillConsoleOutputAttribute       WINPORT(FillConsoleOutputAttribute)
#define    FillConsoleOutputCharacter       WINPORT(FillConsoleOutputCharacter)
#define    SetConsoleActiveScreenBuffer     WINPORT(SetConsoleActiveScreenBuffer)
#define    SetConsoleCursorBlinkTime        WINPORT(SetConsoleCursorBlinkTime)
#define    GetConsoleBasePalette            WINPORT(GetConsoleBasePalette)
#define    SetConsoleBasePalette            WINPORT(SetConsoleBasePalette)

#define    FlushConsoleInputBuffer          WINPORT(FlushConsoleInputBuffer)
#define    GetNumberOfConsoleInputEvents    WINPORT(GetNumberOfConsoleInputEvents)
#define    PeekConsoleInput                 WINPORT(PeekConsoleInput)
#define    ReadConsoleInput                 WINPORT(ReadConsoleInput)
#define    WriteConsoleInput                WINPORT(WriteConsoleInput)

#define    SetConsoleDisplayMode            WINPORT(SetConsoleDisplayMode)
#define    GetConsoleDisplayMode            WINPORT(GetConsoleDisplayMode)

#define    GenerateConsoleCtrlEvent         WINPORT(GenerateConsoleCtrlEvent)
#define    GenerateConsoleCtrlEvent         WINPORT(GenerateConsoleCtrlEvent)

///registry API
#define    RegOpenKeyEx           WINPORT(RegOpenKeyEx)
#define    RegCreateKeyEx         WINPORT(RegCreateKeyEx)
#define    RegCloseKey            WINPORT(RegCloseKey)
#define    RegDeleteKey           WINPORT(RegDeleteKey)
#define    RegDeleteValue         WINPORT(RegDeleteValue)
#define    RegSetValueEx          WINPORT(RegSetValueEx)
#define    RegEnumKey             WINPORT(RegEnumKey)
#define    RegEnumValue           WINPORT(RegEnumValue)
#define    RegEnumKeyEx           WINPORT(RegEnumKeyEx)
#define    RegQueryValueEx        WINPORT(RegQueryValueEx)
#define    RegSetValueEx          WINPORT(RegSetValueEx)
#define    RegQueryInfoKey        WINPORT(RegQueryInfoKey)

//other
#define    GetLastError           WINPORT(GetLastError)
#define    SetLastError           WINPORT(SetLastError)
#define    GetCurrentProcessId    WINPORT(GetCurrentProcessId)
#define    GetDoubleClickTime     WINPORT(GetDoubleClickTime)

//files
#define    CreateDirectory        WINPORT(CreateDirectory)
#define    RemoveDirectory        WINPORT(RemoveDirectory)
#define    DeleteFile             WINPORT(DeleteFile)
#define    CreateFile             WINPORT(CreateFile)
#define    GetFileDescriptor      WINPORT(GetFileDescriptor)
#define    CloseHandle            WINPORT(CloseHandle)
#define    MoveFile               WINPORT(MoveFile)
#define    MoveFileEx             WINPORT(MoveFileEx)
#define    GetCurrentDirectory    WINPORT(GetCurrentDirectory)
#define    SetCurrentDirectory    WINPORT(SetCurrentDirectory)
#define    GetFileSizeEx          WINPORT(GetFileSizeEx)
#define    GetFileSize            WINPORT(GetFileSize)
#define    GetFileSize64          WINPORT(GetFileSize64)
#define    ReadFile               WINPORT(ReadFile)
#define    WriteFile              WINPORT(WriteFile)
#define    SetFilePointerEx       WINPORT(SetFilePointerEx)
#define    SetFilePointer         WINPORT(SetFilePointer)
#define    GetFileTime            WINPORT(GetFileTime)
#define    SetFileTime            WINPORT(SetFileTime)
#define    SetEndOfFile           WINPORT(SetEndOfFile)
#define    FlushFileBuffers       WINPORT(FlushFileBuffers)
#define    GetFileType            WINPORT(GetFileType)

#define    GetFileAttributes      WINPORT(GetFileAttributes)
#define    SetFileAttributes      WINPORT(SetFileAttributes)
#define    FindFirstFile          WINPORT(FindFirstFile)
#define    FindNextFile           WINPORT(FindNextFile)
#define    FindClose              WINPORT(FindClose)

#define    GetTempFileName        WINPORT(GetTempFileName)
#define    GetFullPathName        WINPORT(GetFullPathName)

#define    EvaluateAttributes     WINPORT(Sleep)

#define    Sleep                      WINPORT(Sleep)
#define    GetTickCount               WINPORT(GetTickCount)
#define    GetLocalTime               WINPORT(GetLocalTime)
#define    GetSystemTime              WINPORT(GetSystemTime)
#define    SystemTimeToFileTime       WINPORT(SystemTimeToFileTime)
#define    LocalFileTimeToFileTime    WINPORT(LocalFileTimeToFileTime)
#define    CompareFileTime            WINPORT(CompareFileTime)
#define    FileTimeToLocalFileTime    WINPORT(FileTimeToLocalFileTime)
#define    FileTimeToSystemTime       WINPORT(FileTimeToSystemTime)
#define    GetSystemTimeAsFileTime    WINPORT(GetSystemTimeAsFileTime)
#define    FileTimeToDosDateTime      WINPORT(FileTimeToDosDateTime)
#define    DosDateTimeToFileTime      WINPORT(DosDateTimeToFileTime)
#define    FileTime_UnixToWin32       WINPORT(FileTime_UnixToWin32)

//string
#define    LCMapString            WINPORT(LCMapString)
#define    CharUpperBuff          WINPORT(CharUpperBuff)
#define    CharLowerBuff          WINPORT(CharLowerBuff)
#define    IsCharLower            WINPORT(IsCharLower)
#define    IsCharUpper            WINPORT(IsCharUpper)
#define    IsCharAlpha            WINPORT(IsCharAlpha)
#define    IsCharAlphaNumeric     WINPORT(IsCharAlphaNumeric)
#define    CompareString          WINPORT(CompareString)
#define    CompareStringA         WINPORT(CompareStringA)
#define    WideCharToMultiByte    WINPORT(WideCharToMultiByte)
#define    MultiByteToWideChar    WINPORT(MultiByteToWideChar)
#define    CharUpper              WINPORT(CharUpper)
#define    CharLower              WINPORT(CharLower)
#define    GetOEMCP               WINPORT(GetOEMCP)
#define    GetACP                 WINPORT(GetACP)
#define    GetCPInfo              WINPORT(GetCPInfo)
#define    GetCPInfoEx            WINPORT(GetCPInfoEx)
#define    EnumSystemCodePages    WINPORT(EnumSystemCodePages)

//clipboard
#define    ClipboardAlloc         WINPORT(ClipboardAlloc)
#define    ClipboardFree          WINPORT(ClipboardFree)
#define    ClipboardSize          WINPORT(ClipboardSize)

#define    RegisterClipboardFormat       WINPORT(RegisterClipboardFormat)
#define    OpenClipboard                 WINPORT(OpenClipboard)
#define    CloseClipboard                WINPORT(CloseClipboard)
#define    EmptyClipboard                WINPORT(EmptyClipboard)
#define    IsClipboardFormatAvailable    WINPORT(IsClipboardFormatAvailable)
#define    GetClipboardData              WINPORT(GetClipboardData)
#define    SetClipboardData              WINPORT(SetClipboardData)

//keyboard
#define    GetKeyboardLayoutList  WINPORT(GetKeyboardLayoutList)
#define    MapVirtualKey          WINPORT(MapVirtualKey)
#define    VkKeyScan              WINPORT(VkKeyScan)
#define    ToUnicodeEx            WINPORT(ToUnicodeEx)

#endif //#ifdef WINPORT_DIRECT

#ifdef UNICODE
# define lstrcpy wcscpy
# define lstrlen wcslen
# define lstrcmp wcscmp
# define _tcschr wcschr
# define _tcsrchr wcsrchr
# define _tcsdup wcsdup
# define lstrcat wcscat
# define lstrcpyn wcsncpy
# define lstrcmpi wcscasecmp
# define _istspace iswspace
# define _tmemset(t,c,s) wmemset(t,c,s)
# define _tmemcpy(t,s,c) wmemcpy(t,s,c)
# define _tmemchr(b,c,n) wmemchr(b,c,n)
# define _tmemmove(b,c,n) wmemmove(b,c,n)
#endif

