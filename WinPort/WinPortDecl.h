	///console API


	/// Forked consoles are extension of WinPort, allowing to implement non-interactive virtual terminal
	/// or to save/restore content of console, while using usual screen resize recomposition logic.
	// Clones given console into new non-interactive one and returns handle of new console.
	WINPORT_DECL_DEF(ForkConsole,HANDLE,(HANDLE hParentConsole))
	// Copy state: output buffer, pending input events of previously cloned
	// non-interactive console into given parent and delete cloned console
	WINPORT_DECL_DEF(JoinConsole,VOID,(HANDLE hParentConsole, HANDLE hConsole))
	// Just delete cloned console, losing its state
	WINPORT_DECL_DEF(DiscardConsole,VOID,(HANDLE hConsole))

	WINPORT_DECL_DEF(GetLargestConsoleWindowSize,COORD,(HANDLE hConsoleOutput))
	WINPORT_DECL_DEF(SetConsoleWindowInfo,BOOL,(HANDLE hConsoleOutput, BOOL bAbsolute, const SMALL_RECT *lpConsoleWindow))
	WINPORT_DECL_DEF(SetConsoleTitle,BOOL,(HANDLE hConsoleOutput, const WCHAR *title))
	WINPORT_DECL_DEF(GetConsoleTitle,DWORD,(HANDLE hConsoleOutput, WCHAR *title, DWORD max_size))
	WINPORT_DECL_DEF(SetConsoleScreenBufferSize,BOOL,(HANDLE hConsoleOutput,COORD dwSize))
	WINPORT_DECL_DEF(GetConsoleScreenBufferInfo,BOOL,(HANDLE hConsoleOutput,CONSOLE_SCREEN_BUFFER_INFO *lpConsoleScreenBufferInfo))
	WINPORT_DECL_DEF(SetConsoleCursorPosition,BOOL,(HANDLE hConsoleOutput,COORD dwCursorPosition))
	WINPORT_DECL_DEF(SetConsoleCursorInfo,BOOL,(HANDLE hConsoleOutput,const CONSOLE_CURSOR_INFO *lpConsoleCursorInfon))
	WINPORT_DECL_DEF(GetConsoleCursorInfo,BOOL,(HANDLE hConsoleOutput,CONSOLE_CURSOR_INFO *lpConsoleCursorInfon))
	WINPORT_DECL_DEF(GetConsoleMode,BOOL,(HANDLE hConsoleHandle,LPDWORD lpMode))
	WINPORT_DECL_DEF(SetConsoleMode,BOOL,(HANDLE hConsoleHandle, DWORD dwMode))
	WINPORT_DECL_DEF(ScrollConsoleScreenBuffer,BOOL,(HANDLE hConsoleOutput, const SMALL_RECT *lpScrollRectangle, const SMALL_RECT *lpClipRectangle, COORD dwDestinationOrigin, const CHAR_INFO *lpFill))
	WINPORT_DECL_DEF(SetConsoleTextAttribute,BOOL,(HANDLE hConsoleOutput, DWORD64 qAttributes))
	WINPORT_DECL_DEF(CompositeCharRegister,COMP_CHAR,(const WCHAR *lpSequence))
	WINPORT_DECL_DEF(CompositeCharLookup,const WCHAR *,(COMP_CHAR CompositeChar))
	WINPORT_DECL_DEF(WriteConsole,BOOL,(HANDLE hConsoleOutput, const WCHAR *lpBuffer, DWORD nNumberOfCharsToWrite, LPDWORD lpNumberOfCharsWritten, LPVOID lpReserved))
	WINPORT_DECL_DEF(WriteConsoleOutput,BOOL,(HANDLE hConsoleOutput,const CHAR_INFO *lpBuffer,COORD dwBufferSize,COORD dwBufferCoord,PSMALL_RECT lpScreenRegion))
	WINPORT_DECL_DEF(WriteConsoleOutputCharacter,BOOL,(HANDLE hConsoleOutput, const WCHAR *lpCharacter, DWORD nLength, COORD dwWriteCoord, LPDWORD lpNumberOfCharsWritten))
	WINPORT_DECL_DEF(WaitConsoleInput, BOOL,(HANDLE hConsoleInput, DWORD dwTimeout))
	WINPORT_DECL_DEF(ReadConsoleOutput, BOOL, (HANDLE hConsoleOutput, CHAR_INFO *lpBuffer, COORD dwBufferSize, COORD dwBufferCoord, PSMALL_RECT lpScreenRegion))
	WINPORT_DECL_DEF(FillConsoleOutputAttribute, BOOL, (HANDLE hConsoleOutput, DWORD64 qAttributes, DWORD nLength, COORD dwWriteCoord, LPDWORD lpNumberOfAttrsWritten))
	WINPORT_DECL_DEF(FillConsoleOutputCharacter, BOOL, (HANDLE hConsoleOutput, WCHAR cCharacter, DWORD nLength, COORD dwWriteCoord, LPDWORD lpNumberOfCharsWritten))
	WINPORT_DECL_DEF(SetConsoleActiveScreenBuffer, BOOL,(HANDLE hConsoleOutput))
	WINPORT_DECL_DEF(SetConsoleCursorBlinkTime,VOID,(HANDLE hConsoleOutput, DWORD dwMilliseconds ))

	WINPORT_DECL_DEF(FlushConsoleInputBuffer,BOOL,(HANDLE hConsoleInput))
	WINPORT_DECL_DEF(GetNumberOfConsoleInputEvents,BOOL,(HANDLE hConsoleInput, LPDWORD lpcNumberOfEvents))
	WINPORT_DECL_DEF(PeekConsoleInput,BOOL,(HANDLE hConsoleInput, PINPUT_RECORD lpBuffer, DWORD nLength, LPDWORD lpNumberOfEventsRead))
	WINPORT_DECL_DEF(ReadConsoleInput,BOOL,(HANDLE hConsoleInput, PINPUT_RECORD lpBuffer, DWORD nLength, LPDWORD lpNumberOfEventsRead))
	WINPORT_DECL_DEF(ReadConsoleInputBacktrace, DWORD,(HANDLE hConsoleInput, CHAR *lpBuffer, DWORD nLength))
	WINPORT_DECL_DEF(WriteConsoleInput,BOOL,(HANDLE hConsoleInput, const INPUT_RECORD *lpBuffer, DWORD nLength, LPDWORD lpNumberOfEventsWritten))

	WINPORT_DECL_DEF(CheckForKeyPress,DWORD,(HANDLE hConsoleInput, const WORD *KeyCodes, DWORD KeyCodesCount, DWORD Flags))

	WINPORT_DECL_DEF(SetConsoleDisplayMode,BOOL,(DWORD ModeFlags))
	WINPORT_DECL_DEF(GetConsoleDisplayMode,BOOL,(LPDWORD lpModeFlags))
	WINPORT_DECL_DEF(SetConsoleWindowMaximized,VOID,(BOOL Maximized))
	WINPORT_DECL_DEF(GetConsoleColorPalette,BYTE,(HANDLE hConsoleOutput)) // Returns current color resolution: 4, 8, 24

	WINPORT_DECL_DEF(GetConsoleBasePalette,VOID,(HANDLE hConsoleOutput, void *p))
	WINPORT_DECL_DEF(SetConsoleBasePalette,BOOL,(HANDLE hConsoleOutput, void *p))

	WINPORT_DECL_DEF(GenerateConsoleCtrlEvent, BOOL, (DWORD dwCtrlEvent, DWORD dwProcessGroupId ))
	WINPORT_DECL_DEF(SetConsoleCtrlHandler, BOOL, (PHANDLER_ROUTINE HandlerRoutine, BOOL Add ))

	WINPORT_DECL_DEF(SetConsoleScrollRegion, VOID, (HANDLE hConsoleOutput, SHORT top, SHORT bottom))
	WINPORT_DECL_DEF(GetConsoleScrollRegion, VOID, (HANDLE hConsoleOutput, SHORT *top, SHORT *bottom))

	WINPORT_DECL_DEF(SetConsoleScrollCallback, VOID, (HANDLE hConsoleOutput, PCONSOLE_SCROLL_CALLBACK pCallback, PVOID pContext))
	WINPORT_DECL_DEF(BeginConsoleAdhocQuickEdit, BOOL, ())
	WINPORT_DECL_DEF(SetConsoleTweaks, DWORD64, (DWORD64 tweaks))

	WINPORT_DECL_DEF(SaveConsoleWindowState,VOID,())
	WINPORT_DECL_DEF(ConsoleChangeFont, VOID, ())
	WINPORT_DECL_DEF(IsConsoleActive, BOOL, ())
	WINPORT_DECL_DEF(ConsoleDisplayNotification, VOID, (const WCHAR *title, const WCHAR *text))
	WINPORT_DECL_DEF(ConsoleBackgroundMode, BOOL, (BOOL TryEnterBackgroundMode))
	WINPORT_DECL_DEF(SetConsoleFKeyTitles, BOOL, (HANDLE hConsoleOutput, const CHAR **titles))

	// Query/Change or only Query palette color with specified index:
	// if Index set to (DWORD)-1 then operates on currently chosen color (not using pallette)
	// if initial *ColorFG or *ColorBK was set to (DWORD)-1 - it will change to default color
	// if initial *ColorFG or *ColorBK was set to (DWORD)-2 - this color will not be changed (Query-only operation)
	WINPORT_DECL_DEF(OverrideConsoleColor, VOID, (HANDLE hConsoleOutput, DWORD Index, DWORD *ColorFG, DWORD *ColorBK))

	WINPORT_DECL_DEF(SetConsoleRepaintsDefer, VOID, (HANDLE hConsoleOutput, BOOL Deferring))

	// graphics API
	WINPORT_DECL_DEF(GetConsoleImageCaps, BOOL, (HANDLE hConsoleOutput, size_t sizeof_wgi, WinportGraphicsInfo *wgi))

	// flags: format identity, one of WP_IMG_ that is supported according to GetConsoleImageCaps
	// width and height is image pixels dimensions for RGB/RGBA formats, but for PNG - width specifies buffer byte size and height must be 1 (but ignored for now)
	// area is optional and its fields are also optional each one, use -1 to use default value
	WINPORT_DECL_DEF(SetConsoleImage, BOOL, (HANDLE hConsoleOutput, const char *id, DWORD64 flags, const SMALL_RECT *area, DWORD width, DWORD height, const void *buffer))

	// tf - is a valid combintation of WP_IMGTF_* values
	WINPORT_DECL_DEF(TransformConsoleImage, BOOL, (HANDLE hConsoleOutput, const char *id, const SMALL_RECT *area, uint16_t tf))
	WINPORT_DECL_DEF(DeleteConsoleImage, BOOL, (HANDLE hConsoleOutput, const char *id))

#ifdef WINPORT_REGISTRY
	///registry API
	WINPORT_DECL_DEF(RegOpenKeyEx, LONG, (HKEY hKey,LPCWSTR lpSubKey, DWORD ulOptions, REGSAM samDesired, PHKEY phkResult))
	WINPORT_DECL_DEF(RegCreateKeyEx, LONG, (HKEY hKey, LPCWSTR lpSubKey, DWORD Reserved, LPWSTR lpClass, DWORD dwOptions,
		REGSAM samDesired, LPSECURITY_ATTRIBUTES lpSecurityAttributes, PHKEY phkResult, LPDWORD lpdwDisposition))
	WINPORT_DECL_DEF(RegCloseKey, LONG, (HKEY hKey))
	WINPORT_DECL_DEF(RegDeleteKey, LONG, (HKEY hKey, LPCWSTR lpSubKey))
	WINPORT_DECL_DEF(RegDeleteValue, LONG, (HKEY hKey, LPCWSTR lpValueName))
	WINPORT_DECL_DEF(RegSetValueEx, LONG, (HKEY hKey, LPCWSTR lpValueName, DWORD Reserved,
		DWORD dwType, const BYTE *lpData, DWORD cbData))
	WINPORT_DECL_DEF(RegEnumKeyEx, LONG, (HKEY hKey, DWORD dwIndex, LPWSTR lpName, LPDWORD lpcName,
		LPDWORD lpReserved, LPWSTR lpClass,LPDWORD lpcClass, PFILETIME lpftLastWriteTime))
	WINPORT_DECL_DEF(RegEnumKey, LONG, (HKEY hKey, DWORD dwIndex, LPWSTR lpName, DWORD cchName))
	WINPORT_DECL_DEF(RegEnumValue, LONG, (HKEY hKey, DWORD dwIndex, LPWSTR lpValueName, LPDWORD lpcchValueName,
		LPDWORD lpReserved, LPDWORD lpType, LPBYTE lpData, LPDWORD lpcbData))
	WINPORT_DECL_DEF(RegQueryValueEx, LONG, (HKEY hKey, LPCWSTR lpValueName,
		LPDWORD lpReserved, LPDWORD lpType, LPBYTE lpData, LPDWORD lpcbData))
#if 0
// duplicated definition
	WINPORT_DECL_DEF(RegSetValueEx, LONG, (HKEY hKey, LPCWSTR lpValueName,
		DWORD lpReserved, DWORD lpType, CONST BYTE * lpData, DWORD cbData))
#endif
	WINPORT_DECL_DEF(RegQueryInfoKey, LONG, (HKEY hKey, LPTSTR lpClass, LPDWORD lpcClass,
		LPDWORD lpReserved, LPDWORD lpcSubKeys, LPDWORD lpcMaxSubKeyLen, LPDWORD lpcMaxClassLen,
		LPDWORD lpcValues, LPDWORD lpcMaxValueNameLen, LPDWORD lpcMaxValueLen,
		LPDWORD lpcbSecurityDescriptor, PFILETIME lpftLastWriteTime))

	WINPORT_DECL_DEF(RegWipeBegin, VOID, ())
	WINPORT_DECL_DEF(RegWipeEnd, VOID, ())
#endif

//other
	WINPORT_DECL_DEF(GetLastError, DWORD, ())
	WINPORT_DECL_DEF(SetLastError, VOID, (DWORD code))
	WINPORT_DECL_DEF(GetCurrentProcessId, DWORD, ())
	WINPORT_DECL_DEF(GetDoubleClickTime, DWORD, ())

//files
	WINPORT_DECL_DEF(CreateDirectory, BOOL, (LPCWSTR lpPathName, LPSECURITY_ATTRIBUTES lpSecurityAttributes ))
	WINPORT_DECL_DEF(RemoveDirectory, BOOL, ( LPCWSTR lpDirName))
	WINPORT_DECL_DEF(DeleteFile, BOOL, ( LPCWSTR lpFileName))
	WINPORT_DECL_DEF(CreateFile, HANDLE, ( LPCWSTR lpFileName, DWORD dwDesiredAccess, DWORD dwShareMode,
		const DWORD *UnixMode, DWORD dwCreationDisposition, DWORD dwFlagsAndAttributes, HANDLE hTemplateFile))
	WINPORT_DECL_DEF(GetFileDescriptor, int, (HANDLE hFile))
	WINPORT_DECL_DEF(CloseHandle, BOOL, (HANDLE hObject))
	WINPORT_DECL_DEF(MoveFile, BOOL, (LPCWSTR ExistingFileName, LPCWSTR NewFileName ))
	WINPORT_DECL_DEF(MoveFileEx, BOOL, (LPCWSTR ExistingFileName, LPCWSTR NewFileName,DWORD dwFlags))
	WINPORT_DECL_DEF(GetCurrentDirectory, DWORD, (DWORD nBufferLength, LPWSTR lpBuffer))
	WINPORT_DECL_DEF(SetCurrentDirectory, BOOL, (LPCWSTR lpPathName))
	WINPORT_DECL_DEF(GetFileSizeEx, BOOL, ( HANDLE hFile, PLARGE_INTEGER lpFileSize))
	WINPORT_DECL_DEF(GetFileSize, DWORD, ( HANDLE hFile, LPDWORD lpFileSizeHigh))
	WINPORT_DECL_DEF(GetFileSize64, DWORD64, ( HANDLE hFile))
	WINPORT_DECL_DEF(ReadFile, BOOL, ( HANDLE hFile, LPVOID lpBuffer, DWORD nNumberOfBytesToRead,
		LPDWORD lpNumberOfBytesRead, LPOVERLAPPED lpOverlapped))
	WINPORT_DECL_DEF(WriteFile, BOOL, ( HANDLE hFile, LPCVOID lpBuffer, DWORD nNumberOfBytesToWrite,
		LPDWORD lpNumberOfBytesWritten, LPOVERLAPPED lpOverlapped))
	WINPORT_DECL_DEF(SetFilePointerEx, BOOL, ( HANDLE hFile, LARGE_INTEGER liDistanceToMove,
		PLARGE_INTEGER lpNewFilePointer, DWORD dwMoveMethod))

	// hints that file will soon grow up to specified size, doesnt change actual size of file
	WINPORT_DECL_DEF(FileAllocationHint, VOID, (HANDLE hFile, DWORD64 HintFileSize))
	// grows file to specified size if it was smaller, ensuring that disk space is actually allocated
	WINPORT_DECL_DEF(FileAllocationRequire, BOOL, (HANDLE hFile, DWORD64 RequireFileSize))

	WINPORT_DECL_DEF(SetFilePointer, DWORD, ( HANDLE hFile,
		LONG lDistanceToMove, PLONG lpDistanceToMoveHigh, DWORD dwMoveMethod))
	WINPORT_DECL_DEF(GetFileTime, BOOL, ( HANDLE hFile, LPFILETIME lpCreationTime,
		LPFILETIME lpLastAccessTime, LPFILETIME lpLastWriteTime))
	WINPORT_DECL_DEF(SetFileTime, BOOL, ( HANDLE hFile, const FILETIME *lpCreationTime,
		const FILETIME *lpLastAccessTime, const FILETIME *lpLastWriteTime))
	WINPORT_DECL_DEF(SetEndOfFile, BOOL, ( HANDLE hFile))
	WINPORT_DECL_DEF(FlushFileBuffers, BOOL, ( HANDLE hFile))
	WINPORT_DECL_DEF(GetFileType, DWORD, ( HANDLE hFile))

	WINPORT_DECL_DEF(GetFileAttributes, DWORD, (LPCWSTR lpFileName))
	WINPORT_DECL_DEF(SetFileAttributes, DWORD, (LPCWSTR lpFileName, DWORD dwAttributes))

	WINPORT_DECL_DEF(FindFirstFileWithFlags, HANDLE, (LPCWSTR lpFileName, LPWIN32_FIND_DATAW lpFindFileData, DWORD dwFlags))
	WINPORT_DECL_DEF(FindFirstFile, HANDLE, (LPCWSTR lpFileName, LPWIN32_FIND_DATAW lpFindFileData))
	WINPORT_DECL_DEF(FindNextFile, BOOL, (HANDLE hFindFile, LPWIN32_FIND_DATAW lpFindFileData))
	WINPORT_DECL_DEF(FindClose, BOOL, (HANDLE hFindFile))

	WINPORT_DECL_DEF(GetTempFileName, UINT,( LPCWSTR path, LPCWSTR prefix, UINT unique, LPWSTR buffer ))
	WINPORT_DECL_DEF(GetFullPathName, DWORD, (LPCTSTR lpFileName, DWORD nBufferLength, LPTSTR lpBuffer, LPTSTR *lpFilePart))

#if 0
// not implemented
	WINPORT_DECL_DEF(GetDriveType, UINT, (LPCWSTR lpRootPathName))
#endif

	WINPORT_DECL_DEF(EvaluateAttributes, DWORD,( uint32_t unix_mode, const WCHAR *name ))
	WINPORT_DECL_DEF(EvaluateAttributesA, DWORD,( uint32_t unix_mode, const char *name ))

//time/date
	WINPORT_DECL_DEF(Sleep, VOID, (DWORD dwMilliseconds))
	WINPORT_DECL_DEF(GetTickCount, DWORD, ())
	WINPORT_DECL_DEF(GetLocalTime, VOID, (LPSYSTEMTIME lpSystemTime))
	WINPORT_DECL_DEF(GetSystemTime, VOID, (LPSYSTEMTIME lpSystemTime))
	WINPORT_DECL_DEF(SystemTimeToFileTime, BOOL, (const SYSTEMTIME *lpSystemTime, LPFILETIME lpFileTime))
	WINPORT_DECL_DEF(LocalFileTimeToFileTime, BOOL, (const FILETIME *lpLocalFileTime, LPFILETIME lpFileTime))
	WINPORT_DECL_DEF(CompareFileTime, LONG, (const FILETIME *lpFileTime1, const FILETIME *lpFileTime2))
	WINPORT_DECL_DEF(FileTimeToLocalFileTime, BOOL, (const FILETIME *lpFileTime, LPFILETIME lpLocalFileTime))
	WINPORT_DECL_DEF(FileTimeToSystemTime, BOOL, (const FILETIME *lpFileTime, LPSYSTEMTIME lpSystemTime))
	WINPORT_DECL_DEF(GetSystemTimeAsFileTime, VOID, (FILETIME *lpFileTime))
	WINPORT_DECL_DEF(FileTimeToDosDateTime, BOOL, (const FILETIME *lpFileTime, LPWORD lpFatDate, LPWORD lpFatTime))
	WINPORT_DECL_DEF(DosDateTimeToFileTime, BOOL, ( WORD fatdate, WORD fattime, LPFILETIME ft))

	//string
	WINPORT_DECL_DEF(LCMapString, INT, (LCID lcid, DWORD flags, LPCWSTR src, INT srclen, LPWSTR dst, INT dstlen))
	WINPORT_DECL_DEF(CharUpperBuff, DWORD, (LPWSTR lpsz, DWORD cchLength))
	WINPORT_DECL_DEF(CharLowerBuff, DWORD, (LPWSTR lpsz, DWORD cchLength))
	WINPORT_DECL_DEF(IsCharLower, BOOL, (WCHAR ch))
	WINPORT_DECL_DEF(IsCharUpper, BOOL, (WCHAR ch))
	WINPORT_DECL_DEF(IsCharAlpha, BOOL, (WCHAR ch))
	WINPORT_DECL_DEF(IsCharAlphaNumeric, BOOL, (WCHAR ch))
	WINPORT_DECL_DEF(CompareString, int, ( LCID Locale, DWORD dwCmpFlags, LPCWSTR lpString1, int cchCount1, LPCWSTR lpString2, int cchCount2))
	WINPORT_DECL_DEF(CompareStringA, int, ( LCID Locale, DWORD dwCmpFlags, LPCSTR lpString1, int cchCount1, LPCSTR lpString2, int cchCount2))
	WINPORT_DECL_DEF(WideCharToMultiByte, int, ( UINT CodePage, DWORD dwFlags, LPCWSTR lpWideCharStr,
		int cchWideChar, LPSTR lpMultiByteStr, int cbMultiByte, LPCSTR lpDefaultChar, LPBOOL lpUsedDefaultChar))
	WINPORT_DECL_DEF(MultiByteToWideChar, int, ( UINT CodePage, DWORD dwFlags,
		LPCSTR lpMultiByteStr, int cbMultiByte, LPWSTR lpWideCharStr, int cchWideChar))
	WINPORT_DECL_DEF(CharUpper, LPWSTR, (LPWSTR lpsz))
	WINPORT_DECL_DEF(CharLower, LPWSTR, (LPWSTR lpsz))
	WINPORT_DECL_DEF(GetOEMCP, UINT, ())
	WINPORT_DECL_DEF(GetACP, UINT, ())
	WINPORT_DECL_DEF(GetCPInfo, BOOL, (UINT CodePage, LPCPINFO lpCPInfo))
	WINPORT_DECL_DEF(GetCPInfoEx, BOOL, (UINT codepage, DWORD dwFlags, LPCPINFOEX cpinfo))
	WINPORT_DECL_DEF(EnumSystemCodePages, BOOL, (CODEPAGE_ENUMPROCW lpfnCodePageEnum, DWORD flags))
	WINPORT_DECL_DEF(MatchDecomposedUTF8, BOOL, (int flags, const char *s1, size_t l1, const char *s2, size_t l2))

	//clipboard
	WINPORT_DECL_DEF(RegisterClipboardFormat, UINT, (LPCWSTR lpszFormat))
	WINPORT_DECL_DEF(OpenClipboard, BOOL, (PVOID Reserved))
	WINPORT_DECL_DEF(CloseClipboard, BOOL, ())
	WINPORT_DECL_DEF(EmptyClipboard, BOOL, ())
	WINPORT_DECL_DEF(IsClipboardFormatAvailable, BOOL, (UINT format))

	// use Clipboard-Alloc/-Free/-Size to operate data pointers
	WINPORT_DECL_DEF(GetClipboardData, PVOID, (UINT format))
	WINPORT_DECL_DEF(SetClipboardData, PVOID, (UINT format, HANDLE mem))

	// these are simplified analogs for Win32's Global* APIs, that dedicated to reference clipboard data
	WINPORT_DECL_DEF(ClipboardAlloc, PVOID, (SIZE_T len)) // allocates zero-initialized memory
	WINPORT_DECL_DEF(ClipboardSize, SIZE_T, (PVOID mem))  // return _exact_ allocation size

	// note that like in win32, clipboard data is mostly owned by clipboard so ClipboardFree actually useful
	// only in case of SetClipboardData's failure.
	WINPORT_DECL_DEF(ClipboardFree, VOID, (PVOID mem))

	//keyboard
	WINPORT_DECL_DEF(GetKeyboardLayoutList, int, (int nBuff, HKL *lpList))
	WINPORT_DECL_DEF(MapVirtualKey, UINT, (UINT uCode, UINT uMapType))
	WINPORT_DECL_DEF(VkKeyScan, SHORT, (WCHAR ch))
	WINPORT_DECL_DEF(ToUnicodeEx, int, (UINT wVirtKey, UINT wScanCode, CONST BYTE *lpKeyState,
		LPWSTR pwszBuff, int cchBuff, UINT wFlags, HKL dwhkl))

	//mem
	WINPORT_DECL_DEF(GlobalMemoryStatusEx, BOOL, (LPMEMORYSTATUSEX lpBuffer))

	// thread
	WINPORT_DECL_DEF(SetThreadExecutionState, EXECUTION_STATE, (EXECUTION_STATE es))
	WINPORT_DECL_DEF(SetThreadPriority, BOOL, (int nPriority))
