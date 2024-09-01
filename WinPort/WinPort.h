#pragma once
#ifndef FAR_PYTHON_GEN
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
	int WinPortMain(const char *full_exe_path, int argc, char **argv, int (*AppMain)(int argc, char **argv));
	void WinPortHelp();
	const wchar_t *WinPortBackend();

	// true means far2l runs under smoke testing and code must
	// not skip events from input queue that sometimes used to make UX smoother
	BOOL WinPortTesting();

	///console API
	WINPORT_DECL(ForkConsole,HANDLE,());
	WINPORT_DECL(JoinConsole,VOID,(HANDLE hConsole));

	WINPORT_DECL(GetLargestConsoleWindowSize,COORD,(HANDLE hConsoleOutput));
	WINPORT_DECL(SetConsoleWindowInfo,BOOL,(HANDLE hConsoleOutput, BOOL bAbsolute, const SMALL_RECT *lpConsoleWindow));
	WINPORT_DECL(SetConsoleTitle,BOOL,(HANDLE hConsoleOutput, const WCHAR *title));
	WINPORT_DECL(GetConsoleTitle,DWORD,(HANDLE hConsoleOutput, WCHAR *title, DWORD max_size));
	WINPORT_DECL(SetConsoleScreenBufferSize,BOOL,(HANDLE hConsoleOutput,COORD dwSize));
	WINPORT_DECL(GetConsoleScreenBufferInfo,BOOL,(HANDLE hConsoleOutput,CONSOLE_SCREEN_BUFFER_INFO *lpConsoleScreenBufferInfo));
	WINPORT_DECL(SetConsoleCursorPosition,BOOL,(HANDLE hConsoleOutput,COORD dwCursorPosition));
	WINPORT_DECL(SetConsoleCursorInfo,BOOL,(HANDLE hConsoleOutput,const CONSOLE_CURSOR_INFO *lpConsoleCursorInfon));
	WINPORT_DECL(GetConsoleCursorInfo,BOOL,(HANDLE hConsoleOutput,CONSOLE_CURSOR_INFO *lpConsoleCursorInfon));
	WINPORT_DECL(GetConsoleMode,BOOL,(HANDLE hConsoleHandle,LPDWORD lpMode));
	WINPORT_DECL(SetConsoleMode,BOOL,(HANDLE hConsoleHandle, DWORD dwMode));
	WINPORT_DECL(ScrollConsoleScreenBuffer,BOOL,(HANDLE hConsoleOutput, const SMALL_RECT *lpScrollRectangle, const SMALL_RECT *lpClipRectangle, COORD dwDestinationOrigin, const CHAR_INFO *lpFill));
	WINPORT_DECL(SetConsoleTextAttribute,BOOL,(HANDLE hConsoleOutput, DWORD64 qAttributes));
	WINPORT_DECL(CompositeCharRegister,COMP_CHAR,(const WCHAR *lpSequence));
	WINPORT_DECL(CompositeCharLookup,const WCHAR *,(COMP_CHAR CompositeChar));
	WINPORT_DECL(WriteConsole,BOOL,(HANDLE hConsoleOutput, const WCHAR *lpBuffer, DWORD nNumberOfCharsToWrite, LPDWORD lpNumberOfCharsWritten, LPVOID lpReserved));
	WINPORT_DECL(WriteConsoleOutput,BOOL,(HANDLE hConsoleOutput,const CHAR_INFO *lpBuffer,COORD dwBufferSize,COORD dwBufferCoord,PSMALL_RECT lpScreenRegion));
	WINPORT_DECL(WriteConsoleOutputCharacter,BOOL,(HANDLE hConsoleOutput, const WCHAR *lpCharacter, DWORD nLength, COORD dwWriteCoord, LPDWORD lpNumberOfCharsWritten));
	WINPORT_DECL(WaitConsoleInput, BOOL,(HANDLE hConsoleInput, DWORD dwTimeout));
	WINPORT_DECL(ReadConsoleOutput, BOOL, (HANDLE hConsoleOutput, CHAR_INFO *lpBuffer, COORD dwBufferSize, COORD dwBufferCoord, PSMALL_RECT lpScreenRegion));
	WINPORT_DECL(FillConsoleOutputAttribute, BOOL, (HANDLE hConsoleOutput, DWORD64 qAttributes, DWORD nLength, COORD dwWriteCoord, LPDWORD lpNumberOfAttrsWritten));
	WINPORT_DECL(FillConsoleOutputCharacter, BOOL, (HANDLE hConsoleOutput, WCHAR cCharacter, DWORD nLength, COORD dwWriteCoord, LPDWORD lpNumberOfCharsWritten));
	WINPORT_DECL(SetConsoleActiveScreenBuffer, BOOL,(HANDLE hConsoleOutput));
	WINPORT_DECL(SetConsoleCursorBlinkTime,VOID,(HANDLE hConsoleOutput, DWORD dwMilliseconds ));

	WINPORT_DECL(FlushConsoleInputBuffer,BOOL,(HANDLE hConsoleInput));
	WINPORT_DECL(GetNumberOfConsoleInputEvents,BOOL,(HANDLE hConsoleInput, LPDWORD lpcNumberOfEvents));
	WINPORT_DECL(PeekConsoleInput,BOOL,(HANDLE hConsoleInput, PINPUT_RECORD lpBuffer, DWORD nLength, LPDWORD lpNumberOfEventsRead));
	WINPORT_DECL(ReadConsoleInput,BOOL,(HANDLE hConsoleInput, PINPUT_RECORD lpBuffer, DWORD nLength, LPDWORD lpNumberOfEventsRead));
	WINPORT_DECL(WriteConsoleInput,BOOL,(HANDLE hConsoleInput, const INPUT_RECORD *lpBuffer, DWORD nLength, LPDWORD lpNumberOfEventsWritten));

	// Checks if any of specified keys pressed
	// Optionally preserves specified classes of input events
	// return one plus array index of pressed key or zero if no one of specified keys is pressed
#define CFKP_KEEP_MATCHED_KEY_EVENTS    0x001
#define CFKP_KEEP_UNMATCHED_KEY_EVENTS  0x002
#define CFKP_KEEP_MOUSE_EVENTS          0x004
#define CFKP_KEEP_OTHER_EVENTS          0x100
	WINPORT_DECL(CheckForKeyPress,DWORD,(HANDLE hConsoleInput, const WORD *KeyCodes, DWORD KeyCodesCount, DWORD Flags));
	
	WINPORT_DECL(SetConsoleDisplayMode,BOOL,(DWORD ModeFlags));
	WINPORT_DECL(GetConsoleDisplayMode,BOOL,(LPDWORD lpModeFlags));
	WINPORT_DECL(SetConsoleWindowMaximized,VOID,(BOOL Maximized));
	WINPORT_DECL(GetConsoleColorPalette,BYTE,(HANDLE hConsoleOutput)); // Returns current color resolution: 4, 8, 24

	WINPORT_DECL(GetConsoleBasePalette,VOID,(HANDLE hConsoleOutput, void *p));
	WINPORT_DECL(SetConsoleBasePalette,BOOL,(HANDLE hConsoleOutput, void *p));

	WINPORT_DECL(GenerateConsoleCtrlEvent, BOOL, (DWORD dwCtrlEvent, DWORD dwProcessGroupId ));
	WINPORT_DECL(SetConsoleCtrlHandler, BOOL, (PHANDLER_ROUTINE HandlerRoutine, BOOL Add ));
	
	WINPORT_DECL(SetConsoleScrollRegion, VOID, (HANDLE hConsoleOutput, SHORT top, SHORT bottom));
	WINPORT_DECL(GetConsoleScrollRegion, VOID, (HANDLE hConsoleOutput, SHORT *top, SHORT *bottom));

	WINPORT_DECL(SetConsoleScrollCallback, VOID, (HANDLE hConsoleOutput, PCONSOLE_SCROLL_CALLBACK pCallback, PVOID pContext));
	WINPORT_DECL(BeginConsoleAdhocQuickEdit, BOOL, ());	
	WINPORT_DECL(SetConsoleTweaks, DWORD64, (DWORD64 tweaks));
#define EXCLUSIVE_CTRL_LEFT			0x00000001
#define EXCLUSIVE_CTRL_RIGHT		0x00000002
#define EXCLUSIVE_ALT_LEFT			0x00000004
#define EXCLUSIVE_ALT_RIGHT			0x00000008
#define EXCLUSIVE_WIN_LEFT			0x00000010
#define EXCLUSIVE_WIN_RIGHT			0x00000020

#define CONSOLE_PAINT_SHARP			0x00010000
#define CONSOLE_OSC52CLIP_SET		0x00020000

#define CONSOLE_TTY_PALETTE_OVERRIDE	0x00040000

#define TWEAK_STATUS_SUPPORT_EXCLUSIVE_KEYS	0x01
#define TWEAK_STATUS_SUPPORT_PAINT_SHARP	0x02
#define TWEAK_STATUS_SUPPORT_OSC52CLIP_SET	0x04
#define TWEAK_STATUS_SUPPORT_CHANGE_FONT	0x08
#define TWEAK_STATUS_SUPPORT_TTY_PALETTE	0x10
#define TWEAK_STATUS_SUPPORT_BLINK_RATE		0x20

	WINPORT_DECL(SaveConsoleWindowState,VOID,());
	WINPORT_DECL(ConsoleChangeFont, VOID, ());
	WINPORT_DECL(IsConsoleActive, BOOL, ());
	WINPORT_DECL(ConsoleDisplayNotification, VOID, (const WCHAR *title, const WCHAR *text));
	WINPORT_DECL(ConsoleBackgroundMode, BOOL, (BOOL TryEnterBackgroundMode));
	WINPORT_DECL(SetConsoleFKeyTitles, BOOL, (HANDLE hConsoleOutput, const CHAR **titles));

	// Query/Change or only Query palette color with specified index:
	// if Index set to (DWORD)-1 then operates on currently chosen color (not using pallette)
	// if initial *ColorFG or *ColorBK was set to (DWORD)-1 - it will change to default color
	// if initial *ColorFG or *ColorBK was set to (DWORD)-2 - this color will not be changed (Query-only operation)
	WINPORT_DECL(OverrideConsoleColor, VOID, (HANDLE hConsoleOutput, DWORD Index, DWORD *ColorFG, DWORD *ColorBK));

	WINPORT_DECL(SetConsoleRepaintsDefer, VOID, (HANDLE hConsoleOutput, BOOL Deferring));

#ifdef WINPORT_REGISTRY
	///registry API
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
#endif

//other
	WINPORT_DECL(GetLastError, DWORD, ());
	WINPORT_DECL(SetLastError, VOID, (DWORD code));
	WINPORT_DECL(GetCurrentProcessId, DWORD, ());
	WINPORT_DECL(GetDoubleClickTime, DWORD, ());

//files
	WINPORT_DECL(CreateDirectory, BOOL, (LPCWSTR lpPathName, LPSECURITY_ATTRIBUTES lpSecurityAttributes ));
	WINPORT_DECL(RemoveDirectory, BOOL, ( LPCWSTR lpDirName));
	WINPORT_DECL(DeleteFile, BOOL, ( LPCWSTR lpFileName));
	WINPORT_DECL(CreateFile, HANDLE, ( LPCWSTR lpFileName, DWORD dwDesiredAccess, DWORD dwShareMode,
		const DWORD *UnixMode, DWORD dwCreationDisposition, DWORD dwFlagsAndAttributes, HANDLE hTemplateFile));
	WINPORT_DECL(GetFileDescriptor, int, (HANDLE hFile));
	WINPORT_DECL(CloseHandle, BOOL, (HANDLE hObject));
	WINPORT_DECL(MoveFile, BOOL, (LPCWSTR ExistingFileName, LPCWSTR NewFileName ));
	WINPORT_DECL(MoveFileEx, BOOL, (LPCWSTR ExistingFileName, LPCWSTR NewFileName,DWORD dwFlags));
	WINPORT_DECL(GetCurrentDirectory, DWORD, (DWORD nBufferLength, LPWSTR lpBuffer));
	WINPORT_DECL(SetCurrentDirectory, BOOL, (LPCWSTR lpPathName));
	WINPORT_DECL(GetFileSizeEx, BOOL, ( HANDLE hFile, PLARGE_INTEGER lpFileSize));
	WINPORT_DECL(GetFileSize, DWORD, ( HANDLE hFile, LPDWORD lpFileSizeHigh));
	WINPORT_DECL(GetFileSize64, DWORD64, ( HANDLE hFile));
	WINPORT_DECL(ReadFile, BOOL, ( HANDLE hFile, LPVOID lpBuffer, DWORD nNumberOfBytesToRead, 
		LPDWORD lpNumberOfBytesRead, LPOVERLAPPED lpOverlapped));
	WINPORT_DECL(WriteFile, BOOL, ( HANDLE hFile, LPCVOID lpBuffer, DWORD nNumberOfBytesToWrite, 
		LPDWORD lpNumberOfBytesWritten, LPOVERLAPPED lpOverlapped));
	WINPORT_DECL(SetFilePointerEx, BOOL, ( HANDLE hFile, LARGE_INTEGER liDistanceToMove, 
		PLARGE_INTEGER lpNewFilePointer, DWORD dwMoveMethod));

	// hints that file will soon grow up to specified size, doesnt change actual size of file
	WINPORT_DECL(FileAllocationHint, VOID, (HANDLE hFile, DWORD64 HintFileSize));
	// grows file to specified size if it was smaller, ensuring that disk space is actually allocated
	WINPORT_DECL(FileAllocationRequire, BOOL, (HANDLE hFile, DWORD64 RequireFileSize));

	WINPORT_DECL(SetFilePointer, DWORD, ( HANDLE hFile, 
		LONG lDistanceToMove, PLONG lpDistanceToMoveHigh, DWORD dwMoveMethod));
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
	WINPORT_DECL(GetFullPathName, DWORD, (LPCTSTR lpFileName, DWORD nBufferLength, LPTSTR lpBuffer, LPTSTR *lpFilePart));

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
	WINPORT_DECL(FileTimeToDosDateTime, BOOL, (const FILETIME *lpFileTime, LPWORD lpFatDate, LPWORD lpFatTime));
	WINPORT_DECL(DosDateTimeToFileTime, BOOL, ( WORD fatdate, WORD fattime, LPFILETIME ft));
	WINPORT_DECL(FileTime_UnixToWin32, VOID, (struct timespec ts, FILETIME *lpFileTime));
	WINPORT_DECL(FileTime_Win32ToUnix, VOID, (const FILETIME *lpFileTime, struct timespec *ts));

	//string
	WINPORT_DECL(LCMapString, INT, (LCID lcid, DWORD flags, LPCWSTR src, INT srclen, LPWSTR dst, INT dstlen));
	WINPORT_DECL(CharUpperBuff, DWORD, (LPWSTR lpsz, DWORD cchLength));
	WINPORT_DECL(CharLowerBuff, DWORD, (LPWSTR lpsz, DWORD cchLength));
	WINPORT_DECL(IsCharLower, BOOL, (WCHAR ch));
	WINPORT_DECL(IsCharUpper, BOOL, (WCHAR ch));
	WINPORT_DECL(IsCharAlpha, BOOL, (WCHAR ch));
	WINPORT_DECL(IsCharAlphaNumeric, BOOL, (WCHAR ch));
	WINPORT_DECL(CompareString, int, ( LCID Locale, DWORD dwCmpFlags, LPCWSTR lpString1, int cchCount1, LPCWSTR lpString2, int cchCount2));
	WINPORT_DECL(CompareStringA, int, ( LCID Locale, DWORD dwCmpFlags, LPCSTR lpString1, int cchCount1, LPCSTR lpString2, int cchCount2));
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

	//clipboard
	WINPORT_DECL(RegisterClipboardFormat, UINT, (LPCWSTR lpszFormat));
	WINPORT_DECL(OpenClipboard, BOOL, (PVOID Reserved));
	WINPORT_DECL(CloseClipboard, BOOL, ());
	WINPORT_DECL(EmptyClipboard, BOOL, ());
	WINPORT_DECL(IsClipboardFormatAvailable, BOOL, (UINT format));

	// use Clipboard-Alloc/-Free/-Size to operate data pointers
	WINPORT_DECL(GetClipboardData, PVOID, (UINT format));
	WINPORT_DECL(SetClipboardData, PVOID, (UINT format, HANDLE mem));

	// these are simplified analogs for Win32's Global* APIs, that dedicated to reference clipboard data
	WINPORT_DECL(ClipboardAlloc, PVOID, (SIZE_T len)); // allocates zero-initialized memory
	WINPORT_DECL(ClipboardSize, SIZE_T, (PVOID mem));  // return _exact_ allocation size

	// note that like in win32, clipboard data is mostly owned by clipboard so ClipboardFree actually useful
	// only in case of SetClipboardData's failure.
	WINPORT_DECL(ClipboardFree, VOID, (PVOID mem));

	//keyboard
	WINPORT_DECL(GetKeyboardLayoutList, int, (int nBuff, HKL *lpList));
	WINPORT_DECL(MapVirtualKey, UINT, (UINT uCode, UINT uMapType));
	WINPORT_DECL(VkKeyScan, SHORT, (WCHAR ch));
	WINPORT_DECL(ToUnicodeEx, int, (UINT wVirtKey, UINT wScanCode, CONST BYTE *lpKeyState, 
		LPWSTR pwszBuff, int cchBuff, UINT wFlags, HKL dwhkl));
	
	//%s -> %ls, %ws -> %ls
	SHAREDSYMBOL int vswprintf_ws2ls(wchar_t * ws, size_t len, const wchar_t * format, va_list arg );
	SHAREDSYMBOL int swprintf_ws2ls (wchar_t* ws, size_t len, const wchar_t* format, ...);

	SHAREDSYMBOL void SetPathTranslationPrefix(const wchar_t *prefix);

	SHAREDSYMBOL const wchar_t *GetPathTranslationPrefix();
	SHAREDSYMBOL const char *GetPathTranslationPrefixA();
#ifdef __cplusplus
}

#ifdef WINPORT_REGISTRY
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

#include <vector>

template <class CHAR_T, class LEN_T>
	void *ClipboardAllocFromVector(const std::vector<CHAR_T> &src, LEN_T &len)
{
	len = LEN_T(src.size() * sizeof(CHAR_T));
	if (size_t(len) != (src.size() * sizeof(CHAR_T)))
		return nullptr;

	void *out = len ? WINPORT(ClipboardAlloc)(len) : nullptr;
	if (out) {
		memcpy(out, src.data(), len);
	}
	return out;
}

template <class CHAR_T>
	void *ClipboardAllocFromVector(const std::vector<CHAR_T> &src)
{
	size_t len;
	return ClipboardAllocFromVector<CHAR_T, size_t>(src, len);
}

template <class CHAR_T>
	void *ClipboardAllocFromZeroTerminatedString(const CHAR_T *src)
{
	const size_t len = tzlen(src) + 1;
	void *out = len ? WINPORT(ClipboardAlloc)(len * sizeof(CHAR_T)) : nullptr;
	if (out) {
		memcpy(out, src, len * sizeof(CHAR_T));
	}
	return out;
}

struct ConsoleRepaintsDeferScope
{
	HANDLE _con_out;

	ConsoleRepaintsDeferScope(HANDLE hConOut) : _con_out(hConOut)
	{
		WINPORT(SetConsoleRepaintsDefer)(_con_out, TRUE);
	}
	~ConsoleRepaintsDeferScope()
	{
		WINPORT(SetConsoleRepaintsDefer)(_con_out, FALSE);
	}
};

#endif
#endif /* FAR_PYTHON_GEN */
