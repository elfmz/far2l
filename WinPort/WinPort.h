#pragma once
//#ifdef _WIN32
#if 0
# include <Windows.h>
#else
# include "WinCompat.h"
# include <sys/stat.h>
# include <sys/time.h>
# include <time.h>
# include <stdarg.h>
#endif

#define WINPORT_DECL(NAME, RV, ARGS) SHAREDSYMBOL RV WINPORT_##NAME ARGS
#define WINPORT(NAME) WINPORT_##NAME

#ifdef __cplusplus
extern "C" {
#endif
	int WinPortMain(int argc, char **argv, int (*AppMain)(int argc, char **argv));
	void WinPortHelp();

	///console API
	WINPORT_DECL(GetConsoleFontSize,COORD,( HANDLE hConsoleOutput, DWORD  nFont));
	WINPORT_DECL(GetCurrentConsoleFont,BOOL,( HANDLE hConsoleOutput, BOOL bMaximumWindow,PCONSOLE_FONT_INFO lpConsoleCurrentFont));
	WINPORT_DECL(GetLargestConsoleWindowSize,COORD,(HANDLE hConsoleOutput));
	WINPORT_DECL(SetConsoleWindowInfo,BOOL,(HANDLE hConsoleOutput, BOOL bAbsolute, const SMALL_RECT *lpConsoleWindow));
	WINPORT_DECL(SetConsoleTitle,BOOL,(const WCHAR *title));
	WINPORT_DECL(GetConsoleTitle,DWORD,(WCHAR *title, DWORD max_size));
	WINPORT_DECL(SetConsoleScreenBufferSize,BOOL,(HANDLE hConsoleOutput,COORD dwSize));
	WINPORT_DECL(GetConsoleScreenBufferInfo,BOOL,(HANDLE hConsoleOutput,CONSOLE_SCREEN_BUFFER_INFO *lpConsoleScreenBufferInfo));
	WINPORT_DECL(SetConsoleCursorPosition,BOOL,(HANDLE hConsoleOutput,COORD dwCursorPosition));
	WINPORT_DECL(SetConsoleCursorInfo,BOOL,(HANDLE hConsoleOutput,const CONSOLE_CURSOR_INFO *lpConsoleCursorInfon));
	WINPORT_DECL(GetConsoleCursorInfo,BOOL,(HANDLE hConsoleOutput,CONSOLE_CURSOR_INFO *lpConsoleCursorInfon));
	WINPORT_DECL(GetConsoleMode,BOOL,(HANDLE hConsoleHandle,LPDWORD lpMode));
	WINPORT_DECL(SetConsoleMode,BOOL,(HANDLE hConsoleHandle, DWORD dwMode));
	WINPORT_DECL(ScrollConsoleScreenBuffer,BOOL,(HANDLE hConsoleOutput, const SMALL_RECT *lpScrollRectangle, const SMALL_RECT *lpClipRectangle, COORD dwDestinationOrigin, const CHAR_INFO *lpFill));
	WINPORT_DECL(SetConsoleTextAttribute,BOOL,(HANDLE hConsoleOutput, WORD wAttributes));
	WINPORT_DECL(WriteConsole,BOOL,(HANDLE hConsoleOutput, const WCHAR *lpBuffer, DWORD nNumberOfCharsToWrite, LPDWORD lpNumberOfCharsWritten, LPVOID lpReserved));
	WINPORT_DECL(WriteConsoleOutput,BOOL,(HANDLE hConsoleOutput,const CHAR_INFO *lpBuffer,COORD dwBufferSize,COORD dwBufferCoord,PSMALL_RECT lpScreenRegion));
	WINPORT_DECL(WriteConsoleOutputCharacter,BOOL,(HANDLE hConsoleOutput, const WCHAR *lpCharacter, DWORD nLength, COORD dwWriteCoord, LPDWORD lpNumberOfCharsWritten));
	WINPORT_DECL(ReadConsoleOutput, BOOL, (HANDLE hConsoleOutput, CHAR_INFO *lpBuffer, COORD dwBufferSize, COORD dwBufferCoord, PSMALL_RECT lpScreenRegion));
	WINPORT_DECL(FillConsoleOutputAttribute, BOOL, (HANDLE hConsoleOutput, WORD wAttribute, DWORD nLength, COORD dwWriteCoord, LPDWORD lpNumberOfAttrsWritten));
	WINPORT_DECL(FillConsoleOutputCharacter, BOOL, (HANDLE hConsoleOutput, WCHAR cCharacter, DWORD nLength, COORD dwWriteCoord, LPDWORD lpNumberOfCharsWritten));
	WINPORT_DECL(SetConsoleActiveScreenBuffer, BOOL,(HANDLE hConsoleOutput));

	WINPORT_DECL(FlushConsoleInputBuffer,BOOL,(HANDLE hConsoleInput));
	WINPORT_DECL(GetNumberOfConsoleInputEvents,BOOL,(HANDLE hConsoleInput, LPDWORD lpcNumberOfEvents));
	WINPORT_DECL(PeekConsoleInput,BOOL,(HANDLE hConsoleInput, PINPUT_RECORD lpBuffer, DWORD nLength, LPDWORD lpNumberOfEventsRead));
	WINPORT_DECL(ReadConsoleInput,BOOL,(HANDLE hConsoleInput, PINPUT_RECORD lpBuffer, DWORD nLength, LPDWORD lpNumberOfEventsRead));
	WINPORT_DECL(ReadConsole,BOOL,(HANDLE hConsoleInput, WCHAR *lpBuffer, DWORD nNumberOfCharsToRead, LPDWORD lpNumberOfCharsRead, LPVOID pInputControl));
	WINPORT_DECL(WriteConsoleInput,BOOL,(HANDLE hConsoleInput, const INPUT_RECORD *lpBuffer, DWORD nLength, LPDWORD lpNumberOfEventsWritten));
	
	WINPORT_DECL(SetConsoleDisplayMode,BOOL,(DWORD ModeFlags));
	WINPORT_DECL(GetConsoleDisplayMode,BOOL,(LPDWORD lpModeFlags));
	WINPORT_DECL(SetConsoleWindowMaximized,VOID,(BOOL Maximized));

	//WINPORT_DECL(AddConsoleAlias, BOOL,( LPCWSTR Source, LPCWSTR Target, LPCWSTR ExeName));
	WINPORT_DECL(GetConsoleAlias, DWORD,(LPWSTR lpSource, LPWSTR lpTargetBuffer, DWORD TargetBufferLength, LPWSTR lpExeName));
	WINPORT_DECL(GenerateConsoleCtrlEvent, BOOL, (DWORD dwCtrlEvent, DWORD dwProcessGroupId ));
	WINPORT_DECL(SetConsoleCtrlHandler, BOOL, (PHANDLER_ROUTINE HandlerRoutine, BOOL Add ));
	
	WINPORT_DECL(SetConsoleScrollRegion, VOID, (HANDLE hConsoleOutput, SHORT top, SHORT bottom));
	WINPORT_DECL(GetConsoleScrollRegion, VOID, (HANDLE hConsoleOutput, SHORT *top, SHORT *bottom));

	typedef VOID (*PCONSOLE_SCROLL_CALLBACK)(PVOID pContext, unsigned int Width, CHAR_INFO *Charss);
	WINPORT_DECL(SetConsoleScrollCallback, VOID, (HANDLE hConsoleOutput, PCONSOLE_SCROLL_CALLBACK pCallback, PVOID pContext));
	WINPORT_DECL(BeginConsoleAdhocQuickEdit, BOOL, ());	
	WINPORT_DECL(SetConsoleTweaks, DWORD, (DWORD tweaks));
#define EXCLUSIVE_CTRL_LEFT			0x00000001
#define EXCLUSIVE_CTRL_RIGHT		0x00000002
#define EXCLUSIVE_ALT_LEFT			0x00000004
#define EXCLUSIVE_ALT_RIGHT			0x00000008
#define EXCLUSIVE_WIN_LEFT			0x00000010
#define EXCLUSIVE_WIN_RIGHT			0x00000020
#define CONSOLE_PAINT_SHARP			0x00010000
#define TWEAK_STATUS_SUPPORT_EXCLUSIVE_KEYS	0x1
#define TWEAK_STATUS_SUPPORT_PAINT_SHARP	0x2

	WINPORT_DECL(ConsoleChangeFont, VOID, ());
	WINPORT_DECL(IsConsoleActive, BOOL, ());
	WINPORT_DECL(ConsoleDisplayNotification, VOID, (const WCHAR *title, const WCHAR *text));
	WINPORT_DECL(ConsoleBackgroundMode, BOOL, (BOOL TryEnterBackgroundMode));

	///Registry API
	WINPORT_DECL(RegOpenKeyEx, LONG, (HKEY hKey,LPCWSTR lpSubKey, DWORD ulOptions, REGSAM samDesired, PHKEY phkResult));
	WINPORT_DECL(RegCreateKeyEx, LONG, (HKEY hKey, LPCWSTR lpSubKey, DWORD Reserved, LPWSTR lpClass, DWORD dwOptions, 
		REGSAM samDesired, LPSECURITY_ATTRIBUTES lpSecurityAttributes, PHKEY phkResult, LPDWORD lpdwDisposition));
	WINPORT_DECL(RegCloseKey, LONG, (HKEY hKey));
	WINPORT_DECL(RegDeleteKey, LONG, (HKEY hKey, LPCWSTR lpSubKey));
	WINPORT_DECL(RegDeleteValue, LONG, (HKEY hKey, LPCWSTR lpValueName));
	WINPORT_DECL(RegSetValueEx, LONG, (HKEY hKey, LPCWSTR lpValueName, DWORD Reserved, 
		DWORD dwType, const BYTE *lpData, DWORD cbData));
	WINPORT_DECL(RegEnumKeyEx, LONG, (HKEY hKey, DWORD dwIndex, LPWSTR lpName, LPDWORD lpcName, 
		LPDWORD lpReserved, LPWSTR lpClass,LPDWORD lpcClass, PFILETIME lpftLastWriteTime));
	WINPORT_DECL(RegEnumKey, LONG, (HKEY hKey, DWORD dwIndex, LPWSTR lpName, DWORD cchName));
	WINPORT_DECL(RegEnumValue, LONG, (HKEY hKey, DWORD dwIndex, LPWSTR lpValueName, LPDWORD lpcchValueName,
		LPDWORD lpReserved, LPDWORD lpType, LPBYTE lpData, LPDWORD lpcbData));
	WINPORT_DECL(RegQueryValueEx, LONG, (HKEY hKey, LPCWSTR lpValueName, 
		LPDWORD lpReserved, LPDWORD lpType, LPBYTE lpData, LPDWORD lpcbData));
	WINPORT_DECL(RegSetValueEx, LONG, (HKEY hKey, LPCWSTR lpValueName, 
		DWORD lpReserved, DWORD lpType, CONST BYTE * lpData, DWORD cbData));
	WINPORT_DECL(RegQueryInfoKey, LONG, (HKEY hKey, LPTSTR lpClass, LPDWORD lpcClass,
		LPDWORD lpReserved, LPDWORD lpcSubKeys, LPDWORD lpcMaxSubKeyLen, LPDWORD lpcMaxClassLen,
		LPDWORD lpcValues, LPDWORD lpcMaxValueNameLen, LPDWORD lpcMaxValueLen,
		LPDWORD lpcbSecurityDescriptor, PFILETIME lpftLastWriteTime));

	WINPORT_DECL(RegWipeBegin, VOID, ());
	WINPORT_DECL(RegWipeEnd, VOID, ());

//other
	WINPORT_DECL(TranslateErrno, VOID, ());
	WINPORT_DECL(GetLastError, DWORD, ());
	WINPORT_DECL(SetLastError, VOID, (DWORD code));
	WINPORT_DECL(InterlockedIncrement, LONG, (LONG volatile *Value));
	WINPORT_DECL(InterlockedDecrement, LONG, (LONG volatile *Value));
	WINPORT_DECL(InterlockedExchange, LONG, (LONG volatile *Value, LONG NewValue));
	WINPORT_DECL(InterlockedCompareExchange, LONG, (LONG volatile *Value, LONG NewValue, LONG CompareValue));
	WINPORT_DECL(GetCurrentProcessId, DWORD, ());
	WINPORT_DECL(LoadLibraryEx, PVOID, (LPCWSTR lpLibFileName, HANDLE hFile, DWORD dwFlags));	
	WINPORT_DECL(FreeLibrary, BOOL, (HMODULE hModule));
	WINPORT_DECL(GetProcAddress, PVOID, (HMODULE hModule, LPCSTR lpProcName));	
	WINPORT_DECL(GetDoubleClickTime, DWORD, ());
	WINPORT_DECL(GetComputerName, BOOL, (LPWSTR lpBuffer, LPDWORD nSize));
	WINPORT_DECL(GetUserName, BOOL, (LPWSTR lpBuffer, LPDWORD nSize));
	WINPORT_DECL(GetEnvironmentVariable, DWORD, (LPCWSTR lpName, LPWSTR lpBuffer, DWORD nSize));
	WINPORT_DECL(SetEnvironmentVariable, BOOL, (LPCWSTR lpName, LPCWSTR lpValue));
	WINPORT_DECL(ExpandEnvironmentStrings, DWORD, (LPCWSTR lpSrc, LPWSTR lpDst, DWORD nSize));
	

//files

	WINPORT_DECL(CreateDirectory, BOOL, (LPCWSTR lpPathName, LPSECURITY_ATTRIBUTES lpSecurityAttributes ));
	WINPORT_DECL(RemoveDirectory, BOOL, ( LPCWSTR lpDirName));
	WINPORT_DECL(DeleteFile, BOOL, ( LPCWSTR lpFileName));
	WINPORT_DECL(CreateFile, HANDLE, ( LPCWSTR lpFileName, DWORD dwDesiredAccess, DWORD dwShareMode,
		LPSECURITY_ATTRIBUTES lpSecurityAttributes, DWORD dwCreationDisposition, 
		DWORD dwFlagsAndAttributes, HANDLE hTemplateFile));
	WINPORT_DECL(GetFileDescriptor, int, (HANDLE hFile));
	WINPORT_DECL(CloseHandle, BOOL, (HANDLE hObject));
	WINPORT_DECL(MoveFile, BOOL, (LPCWSTR ExistingFileName, LPCWSTR NewFileName ));
	WINPORT_DECL(MoveFileEx, BOOL, (LPCWSTR ExistingFileName, LPCWSTR NewFileName,DWORD dwFlags));
	WINPORT_DECL(GetCurrentDirectory, DWORD, (DWORD  nBufferLength, LPWSTR lpBuffer));
	WINPORT_DECL(SetCurrentDirectory, BOOL, (LPCWSTR lpPathName));
	WINPORT_DECL(GetFileSizeEx, BOOL, ( HANDLE hFile, PLARGE_INTEGER lpFileSize));
	WINPORT_DECL(GetFileSize, DWORD, ( HANDLE  hFile, LPDWORD lpFileSizeHigh));
	WINPORT_DECL(ReadFile, BOOL, ( HANDLE hFile, LPVOID lpBuffer, DWORD nNumberOfBytesToRead, 
		LPDWORD lpNumberOfBytesRead, LPOVERLAPPED lpOverlapped));
	WINPORT_DECL(WriteFile, BOOL, ( HANDLE hFile, LPCVOID lpBuffer, DWORD nNumberOfBytesToWrite, 
		LPDWORD lpNumberOfBytesWritten, LPOVERLAPPED lpOverlapped));
	WINPORT_DECL(SetFilePointerEx, BOOL, ( HANDLE hFile, LARGE_INTEGER liDistanceToMove, 
		PLARGE_INTEGER lpNewFilePointer, DWORD dwMoveMethod));
	WINPORT_DECL(SetFilePointer, DWORD, ( HANDLE hFile, 
		LONG lDistanceToMove, PLONG  lpDistanceToMoveHigh, DWORD  dwMoveMethod));
	WINPORT_DECL(GetFileTime, BOOL, ( HANDLE hFile, LPFILETIME lpCreationTime, 
		LPFILETIME lpLastAccessTime, LPFILETIME lpLastWriteTime));
	WINPORT_DECL(SetFileTime, BOOL, ( HANDLE hFile, const FILETIME *lpCreationTime, 
		const FILETIME *lpLastAccessTime, const FILETIME *lpLastWriteTime));
	WINPORT_DECL(SetEndOfFile, BOOL, ( HANDLE hFile));
	WINPORT_DECL(FlushFileBuffers, BOOL, ( HANDLE hFile));
	WINPORT_DECL(GetFileType, DWORD, ( HANDLE hFile));

	WINPORT_DECL(GetFileAttributes, DWORD, (LPCWSTR lpFileName));
	WINPORT_DECL(SetFileAttributes, DWORD, (LPCWSTR lpFileName, DWORD dwAttributes));
	
#define FIND_FILE_FLAG_NO_DIRS		0x01
#define FIND_FILE_FLAG_NO_FILES		0x02
#define FIND_FILE_FLAG_NO_LINKS		0x04
#define FIND_FILE_FLAG_NO_DEVICES	0x08
#define FIND_FILE_FLAG_NO_CUR_UP	0x10 //skip virtual . and ..
#define FIND_FILE_FLAG_CASE_INSENSITIVE	0x1000 //currently affects only english characters
#define FIND_FILE_FLAG_NOT_ANNOYING	0x2000 //avoid sudo prompt if can't query some not very important information without it
	
	WINPORT_DECL(FindFirstFileWithFlags, HANDLE, (LPCWSTR lpFileName, LPWIN32_FIND_DATAW lpFindFileData, DWORD dwFlags));
	WINPORT_DECL(FindFirstFile, HANDLE, (LPCWSTR lpFileName, LPWIN32_FIND_DATAW lpFindFileData));
	WINPORT_DECL(FindNextFile, BOOL, (HANDLE hFindFile, LPWIN32_FIND_DATAW lpFindFileData));
	WINPORT_DECL(FindClose, BOOL, (HANDLE hFindFile));

	WINPORT_DECL(GetDriveType, UINT, (LPCWSTR lpRootPathName));
	WINPORT_DECL(GetTempFileName, UINT,( LPCWSTR path, LPCWSTR prefix, UINT unique, LPWSTR buffer ));
	WINPORT_DECL(GetFullPathName, DWORD, (LPCTSTR lpFileName,  DWORD nBufferLength, LPTSTR lpBuffer, LPTSTR *lpFilePart));

	WINPORT_DECL(EvaluateAttributes, DWORD,( uint32_t unix_mode, const WCHAR *name ));
	WINPORT_DECL(EvaluateAttributesA, DWORD,( uint32_t unix_mode, const char *name ));

//time/date
	SHAREDSYMBOL clock_t GetProcessUptimeMSec();//use instead of Windows's clock()
	WINPORT_DECL(Sleep, VOID, (DWORD dwMilliseconds));
	WINPORT_DECL(GetTickCount, DWORD, ());
	WINPORT_DECL(GetLocalTime, VOID, (LPSYSTEMTIME lpSystemTime));
	WINPORT_DECL(GetSystemTime, VOID, (LPSYSTEMTIME lpSystemTime));
	WINPORT_DECL(SystemTimeToFileTime, BOOL, (const SYSTEMTIME *lpSystemTime, LPFILETIME lpFileTime));
	WINPORT_DECL(LocalFileTimeToFileTime, BOOL, (const FILETIME *lpLocalFileTime, LPFILETIME lpFileTime));
	WINPORT_DECL(CompareFileTime, LONG, (const FILETIME *lpFileTime1, const FILETIME *lpFileTime2));
	WINPORT_DECL(FileTimeToLocalFileTime, BOOL, (const FILETIME *lpFileTime, LPFILETIME lpLocalFileTime));
	WINPORT_DECL(FileTimeToSystemTime, BOOL, (const FILETIME *lpFileTime, LPSYSTEMTIME lpSystemTime));
	WINPORT_DECL(GetSystemTimeAsFileTime, VOID, (FILETIME *lpFileTime));
	WINPORT_DECL(FileTimeToDosDateTime, BOOL, (const FILETIME *lpFileTime, LPWORD   lpFatDate, LPWORD   lpFatTime));
	WINPORT_DECL(DosDateTimeToFileTime, BOOL, ( WORD fatdate, WORD fattime, LPFILETIME ft));
	WINPORT_DECL(FileTime_UnixToWin32, VOID, (struct timespec ts, FILETIME *lpFileTime));
	WINPORT_DECL(FileTime_Win32ToUnix, VOID, (const FILETIME *lpFileTime, struct timespec *ts));
	

	//String
	WINPORT_DECL(LCMapString, INT, (LCID lcid, DWORD flags, LPCWSTR src, INT srclen, LPWSTR dst, INT dstlen));
	WINPORT_DECL(CharUpperBuff, DWORD, (LPWSTR lpsz, DWORD  cchLength));
	WINPORT_DECL(CharLowerBuff, DWORD, (LPWSTR lpsz, DWORD  cchLength));
	WINPORT_DECL(IsCharLower, BOOL, (WCHAR ch));
	WINPORT_DECL(IsCharUpper, BOOL, (WCHAR ch));
	WINPORT_DECL(IsCharAlpha, BOOL, (WCHAR ch));
	WINPORT_DECL(IsCharAlphaNumeric, BOOL, (WCHAR ch));
	WINPORT_DECL(CompareString, int, ( LCID Locale, DWORD dwCmpFlags, LPCWSTR lpString1, int cchCount1, LPCWSTR lpString2, int cchCount2));
	WINPORT_DECL(CompareStringA, int, ( LCID Locale, DWORD dwCmpFlags, LPCSTR lpString1, int cchCount1, LPCSTR lpString2, int cchCount2));
	WINPORT_DECL(IsTextUnicode, BOOL, (CONST VOID* lpv, int iSize, LPINT lpiResult));
	WINPORT_DECL(WideCharToMultiByte, int, ( UINT CodePage, DWORD dwFlags, LPCWSTR lpWideCharStr, 
		int cchWideChar, LPSTR lpMultiByteStr, int cbMultiByte, LPCSTR lpDefaultChar, LPBOOL lpUsedDefaultChar));
	WINPORT_DECL(MultiByteToWideChar, int, ( UINT CodePage, DWORD dwFlags, 
		LPCSTR lpMultiByteStr, int cbMultiByte, LPWSTR lpWideCharStr, int cchWideChar));
	WINPORT_DECL(CharUpper, LPWSTR, (LPWSTR lpsz));
	WINPORT_DECL(CharLower, LPWSTR, (LPWSTR lpsz));
	WINPORT_DECL(GetOEMCP, UINT, ());
	WINPORT_DECL(GetACP, UINT, ());
	WINPORT_DECL(GetCPInfo, BOOL, (UINT CodePage, LPCPINFO lpCPInfo));
	WINPORT_DECL(GetCPInfoEx, BOOL, (UINT codepage, DWORD dwFlags, LPCPINFOEX cpinfo));
	WINPORT_DECL(EnumSystemCodePages, BOOL, (CODEPAGE_ENUMPROCW lpfnCodePageEnum, DWORD flags));

	//synch
	WINPORT_DECL(CreateThread, HANDLE, (LPSECURITY_ATTRIBUTES lpThreadAttributes, SIZE_T dwStackSize, 
		WINPORT_THREAD_START_ROUTINE lpStartAddress, LPVOID lpParameter, DWORD dwCreationFlags, LPDWORD lpThreadId));	
	WINPORT_DECL(WaitForSingleObject, DWORD, (HANDLE hHandle, DWORD dwMilliseconds));	
	WINPORT_DECL(WaitForMultipleObjects, DWORD, (DWORD nCount, HANDLE *pHandles, BOOL bWaitAll, DWORD dwMilliseconds));
	WINPORT_DECL(CreateEvent, HANDLE, (LPSECURITY_ATTRIBUTES lpEventAttributes,
		BOOL bManualReset, BOOL bInitialState, LPCWSTR lpName));
	WINPORT_DECL(SetEvent, BOOL, (HANDLE hEvent));
	WINPORT_DECL(ResetEvent, BOOL, (HANDLE hEvent));
	WINPORT_DECL( CreateSemaphore, HANDLE, (LPSECURITY_ATTRIBUTES lpSemaphoreAttributes, LONG lInitialCount, LONG lMaximumCount, LPCTSTR lpName));
	WINPORT_DECL( ReleaseSemaphore, BOOL, (HANDLE hSemaphore, LONG lReleaseCount, PLONG lpPreviousCount));
	WINPORT_DECL(GetCurrentThreadId, DWORD, ());
	WINPORT_DECL(ResumeThread, DWORD, (HANDLE hThread));

	//FS notify
	WINPORT_DECL(FindFirstChangeNotification, HANDLE, (LPCWSTR lpPathName, BOOL bWatchSubtree, DWORD dwNotifyFilter));
	WINPORT_DECL(FindNextChangeNotification, BOOL, (HANDLE hChangeHandle));
	WINPORT_DECL(FindCloseChangeNotification, BOOL, (HANDLE hChangeHandle));

	//memory
    WINPORT_DECL(GlobalAlloc, HGLOBAL, ( UINT   uFlags, SIZE_T dwBytes));
	WINPORT_DECL(GlobalFree, HGLOBAL, ( HGLOBAL hMem));
    WINPORT_DECL(GlobalLock, PVOID, ( HGLOBAL hMem));
	WINPORT_DECL(GlobalUnlock, BOOL, ( HGLOBAL hMem));

	//clipboard
	WINPORT_DECL(TextToClipboard, BOOL, (LPCWSTR Tex));
	WINPORT_DECL(TextFromClipboard, LPWSTR, (int MaxLength));

	WINPORT_DECL(RegisterClipboardFormat, UINT, (LPCWSTR lpszFormat));
	WINPORT_DECL(OpenClipboard, BOOL, (PVOID Reserved));
	WINPORT_DECL(CloseClipboard, BOOL, ());
	WINPORT_DECL(EmptyClipboard, BOOL, ());
	WINPORT_DECL(IsClipboardFormatAvailable, BOOL, (UINT format));
	WINPORT_DECL(GetClipboardData, HANDLE, (UINT format));
	WINPORT_DECL(SetClipboardData, HANDLE, (UINT format, HANDLE mem));


	//keyboard
	WINPORT_DECL(GetKeyboardLayoutList, int, (int nBuff, HKL *lpList));
	WINPORT_DECL(MapVirtualKey, UINT, (UINT uCode, UINT uMapType));
	WINPORT_DECL(VkKeyScan, SHORT, (WCHAR ch));
	WINPORT_DECL(ToUnicodeEx, int, (UINT wVirtKey, UINT wScanCode, CONST BYTE *lpKeyState, 
		LPWSTR pwszBuff, int cchBuff, UINT wFlags, HKL dwhkl));
	
	WINPORT_DECL(WSAGetLastError, DWORD, ());


	//%s -> %ls, %ws -> %ls
	SHAREDSYMBOL int vswprintf_ws2ls(wchar_t * ws, size_t len, const wchar_t * format, va_list arg );
	SHAREDSYMBOL int swprintf_ws2ls (wchar_t* ws, size_t len, const wchar_t* format, ...);


	SHAREDSYMBOL void SetPathTranslationPrefix(const wchar_t *prefix);

	SHAREDSYMBOL const wchar_t *GetPathTranslationPrefix();
	SHAREDSYMBOL const char *GetPathTranslationPrefixA();
#ifdef __cplusplus
}

struct __attribute__ ((visibility("default"))) WINPORT(LastErrorGuard)
{
	DWORD value;
	
	WINPORT(LastErrorGuard)();
	~ WINPORT(LastErrorGuard)();
};


struct RegWipeScope
{
	inline RegWipeScope()
	{
		WINPORT(RegWipeBegin)();
	}
	inline ~RegWipeScope()
	{
		WINPORT(RegWipeEnd)();
	}
};
#endif
