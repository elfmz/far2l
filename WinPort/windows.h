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
#endif

#define    GetConsoleFontSize			WINPORT(GetConsoleFontSize)
#define    GetCurrentConsoleFont		WINPORT(GetCurrentConsoleFont)
#define    GetLargestConsoleWindowSize		WINPORT(GetLargestConsoleWindowSize)
#define    SetConsoleWindowInfo			WINPORT(SetConsoleWindowInfo)
#define    SetConsoleTitle			WINPORT(SetConsoleTitle)
#define    GetConsoleTitle			WINPORT(GetConsoleTitle)
#define    SetConsoleScreenBufferSize			WINPORT(SetConsoleScreenBufferSize)
#define    GetConsoleScreenBufferInfo			WINPORT(GetConsoleScreenBufferInfo)
#define    SetConsoleCursorPosition			WINPORT(SetConsoleCursorPosition)
#define    SetConsoleCursorInfo			WINPORT(SetConsoleCursorInfo)
#define    GetConsoleCursorInfo			WINPORT(GetConsoleCursorInfo)
#define    GetConsoleMode			WINPORT(GetConsoleMode)
#define    SetConsoleMode			WINPORT(SetConsoleMode)
#define    ScrollConsoleScreenBuffer			WINPORT(ScrollConsoleScreenBuffer)
#define    SetConsoleTextAttribute			WINPORT(SetConsoleTextAttribute)
#define    WriteConsole				WINPORT(WriteConsole)
#define    WriteConsoleOutput			WINPORT(WriteConsoleOutput)
#define    WriteConsoleOutputCharacter			WINPORT(WriteConsoleOutputCharacter)
#define    ReadConsoleOutput			WINPORT(ReadConsoleOutput)
#define    FillConsoleOutputAttribute			WINPORT(FillConsoleOutputAttribute)
#define    FillConsoleOutputCharacter			WINPORT(FillConsoleOutputCharacter)
#define    SetConsoleActiveScreenBuffer			WINPORT(SetConsoleActiveScreenBuffer)

#define    FlushConsoleInputBuffer			WINPORT(FlushConsoleInputBuffer)
#define    GetNumberOfConsoleInputEvents			WINPORT(GetNumberOfConsoleInputEvents)
#define    PeekConsoleInput			WINPORT(PeekConsoleInput)
#define    ReadConsoleInput			WINPORT(ReadConsoleInput)
#define    ReadConsole			WINPORT(ReadConsole)
#define    WriteConsoleInput			WINPORT(WriteConsoleInput)
	
#define    SetConsoleDisplayMode			WINPORT(SetConsoleDisplayMode)
#define    GetConsoleDisplayMode			WINPORT(GetConsoleDisplayMode)

#define    AddConsoleAlias			WINPORT(AddConsoleAlias)
#define    GetConsoleAlias			WINPORT(GetConsoleAlias)
#define    GenerateConsoleCtrlEvent			WINPORT(GenerateConsoleCtrlEvent)
#define    GenerateConsoleCtrlEvent			WINPORT(GenerateConsoleCtrlEvent)

	///Registry API
#define    RegOpenKeyEx			WINPORT(RegOpenKeyEx)
#define    RegCreateKeyEx			WINPORT(RegCreateKeyEx)
#define    RegCloseKey			WINPORT(RegCloseKey)
#define    RegDeleteKey			WINPORT(RegDeleteKey)
#define    RegDeleteValue			WINPORT(RegDeleteValue)
#define    RegSetValueEx			WINPORT(RegSetValueEx)
#define    RegEnumKey			WINPORT(RegEnumKey)
#define    RegEnumValue			WINPORT(RegEnumValue)
#define    RegEnumKeyEx			WINPORT(RegEnumKeyEx)
#define    RegQueryValueEx			WINPORT(RegQueryValueEx)
#define    RegSetValueEx			WINPORT(RegSetValueEx)
#define    RegQueryInfoKey		WINPORT(RegQueryInfoKey)

//other
#define    GetLastError			WINPORT(GetLastError)
#define    SetLastError			WINPORT(SetLastError)
#define    InterlockedIncrement			WINPORT(InterlockedIncrement)
#define    InterlockedDecrement			WINPORT(InterlockedDecrement)
#define    InterlockedExchange			WINPORT(InterlockedExchange)
#define    InterlockedCompareExchange			WINPORT(InterlockedCompareExchange)
#define    GetCurrentProcessId			WINPORT(GetCurrentProcessId)
#define    LoadLibraryEx			WINPORT(LoadLibraryEx)
#define    FreeLibrary			WINPORT(FreeLibrary)
#define    GetProcAddress			WINPORT(GetProcAddress)
#define    GetDoubleClickTime			WINPORT(GetDoubleClickTime)
#define    GetComputerName			WINPORT(GetComputerName)
#define    GetUserName			WINPORT(GetUserName)
#define    GetEnvironmentVariable			WINPORT(GetEnvironmentVariable)
#define    SetEnvironmentVariable			WINPORT(SetEnvironmentVariable)
#define    ExpandEnvironmentStrings			WINPORT(ExpandEnvironmentStrings)
	

//files

#define    CreateDirectory			WINPORT(CreateDirectory)
#define    RemoveDirectory			WINPORT(RemoveDirectory)
#define    DeleteFile			WINPORT(DeleteFile)
#define    CreateFile			WINPORT(CreateFile)
#define    GetFileDescriptor			WINPORT(GetFileDescriptor)
#define    CloseHandle			WINPORT(CloseHandle)
#define    MoveFile			WINPORT(MoveFile)
#define    MoveFileEx			WINPORT(MoveFileEx)
#define    GetCurrentDirectory			WINPORT(GetCurrentDirectory)
#define    SetCurrentDirectory			WINPORT(SetCurrentDirectory)
#define    GetFileSizeEx			WINPORT(GetFileSizeEx)
#define    GetFileSize			WINPORT(GetFileSize)
#define    ReadFile			WINPORT(ReadFile)
#define    WriteFile			WINPORT(WriteFile)
#define    SetFilePointerEx			WINPORT(SetFilePointerEx)
#define    SetFilePointer			WINPORT(SetFilePointer)
#define    GetFileTime			WINPORT(GetFileTime)
#define    SetEndOfFile			WINPORT(SetEndOfFile)
#define    FlushFileBuffers			WINPORT(FlushFileBuffers)
#define    GetFileType			WINPORT(GetFileType)

#define    GetFileAttributes			WINPORT(GetFileAttributes)
#define    SetFileAttributes			WINPORT(SetFileAttributes)
#define    FindFirstFile			WINPORT(FindFirstFile)
#define    FindNextFile			WINPORT(FindNextFile)
#define    FindClose			WINPORT(FindClose)

#define    GetDriveType			WINPORT(GetDriveType)
#define    GetTempFileName			WINPORT(GetTempFileName)
#define    GetFullPathName			WINPORT(GetFullPathName)

#define    EvaluateAttributes		WINPORT(Sleep)

#define    Sleep			WINPORT(Sleep)
#define    GetTickCount			WINPORT(GetTickCount)
#define    GetLocalTime			WINPORT(GetLocalTime)
#define    GetSystemTime			WINPORT(GetSystemTime)
#define    SystemTimeToFileTime			WINPORT(SystemTimeToFileTime)
#define    LocalFileTimeToFileTime			WINPORT(LocalFileTimeToFileTime)
#define    CompareFileTime			WINPORT(CompareFileTime)
#define    FileTimeToLocalFileTime			WINPORT(FileTimeToLocalFileTime)
#define    FileTimeToSystemTime			WINPORT(FileTimeToSystemTime)
#define    GetSystemTimeAsFileTime			WINPORT(GetSystemTimeAsFileTime)
#define    FileTimeToDosDateTime			WINPORT(FileTimeToDosDateTime)
#define    DosDateTimeToFileTime			WINPORT(DosDateTimeToFileTime)
#define    FileTime_UnixToWin32			WINPORT(FileTime_UnixToWin32)
	

	//String
#define    LCMapString			WINPORT(LCMapString)
#define    CharUpperBuff			WINPORT(CharUpperBuff)
#define    CharLowerBuff			WINPORT(CharLowerBuff)
#define    IsCharLower			WINPORT(IsCharLower)
#define    IsCharUpper			WINPORT(IsCharUpper)
#define    IsCharAlpha			WINPORT(IsCharAlpha)
#define    IsCharAlphaNumeric			WINPORT(IsCharAlphaNumeric)
#define    CompareString			WINPORT(CompareString)
#define    CompareStringA			WINPORT(CompareStringA)
#define    IsTextUnicode			WINPORT(IsTextUnicode)
#define    WideCharToMultiByte			WINPORT(WideCharToMultiByte)
#define    MultiByteToWideChar			WINPORT(MultiByteToWideChar)
#define    CharUpper			WINPORT(CharUpper)
#define    CharLower			WINPORT(CharLower)
#define    GetOEMCP			WINPORT(GetOEMCP)
#define    GetACP			WINPORT(GetACP)
#define    GetCPInfo			WINPORT(GetCPInfo)
#define    GetCPInfoEx			WINPORT(GetCPInfoEx)
#define    EnumSystemCodePages		WINPORT(EnumSystemCodePages)

	//synch
#define    CreateThread			WINPORT(CreateThread)
#define    WaitForSingleObject			WINPORT(WaitForSingleObject)
#define    WaitForMultipleObjects			WINPORT(WaitForMultipleObjects)
#define    CreateEvent			WINPORT(CreateEvent)
#define    SetEvent			WINPORT(SetEvent)
#define    ResetEvent			WINPORT(ResetEvent)
#define     CreateSemaphore			WINPORT(CreateSemaphore)
#define     ReleaseSemaphore			WINPORT(ReleaseSemaphore)
#define    GetCurrentThreadId			WINPORT(GetCurrentThreadId)
#define    ResumeThread			WINPORT(ResumeThread)

	//FS notify
#define    FindFirstChangeNotification			WINPORT(FindFirstChangeNotification)
#define    FindNextChangeNotification			WINPORT(FindNextChangeNotification)
#define    FindCloseChangeNotification			WINPORT(FindCloseChangeNotification)

#define    GlobalAlloc			WINPORT(GlobalAlloc)
#define    GlobalFree			WINPORT(GlobalFree)
#define    GlobalLock			WINPORT(GlobalLock)
#define    GlobalUnlock			WINPORT(GlobalUnlock)

	//clipboard
#define    TextToClipboard			WINPORT(TextToClipboard)
#define    TextFromClipboard			WINPORT(TextFromClipboard)

#define    RegisterClipboardFormat		WINPORT(RegisterClipboardFormat)
#define    OpenClipboard			WINPORT(OpenClipboard)
#define    CloseClipboard			WINPORT(CloseClipboard)
#define    EmptyClipboard			WINPORT(EmptyClipboard)
#define    IsClipboardFormatAvailable		WINPORT(IsClipboardFormatAvailable)
#define    GetClipboardData			WINPORT(GetClipboardData)
#define    SetClipboardData			WINPORT(SetClipboardData)


	//keyboard
#define    GetKeyboardLayoutList		WINPORT(GetKeyboardLayoutList)
#define    MapVirtualKey			WINPORT(MapVirtualKey)
#define    VkKeyScan				WINPORT(VkKeyScan)
#define    ToUnicodeEx				WINPORT(ToUnicodeEx)
	
	
#define    WSAGetLastError			WINPORT(WSAGetLastError)

#endif //#ifdef WINPORT_DIRECT
