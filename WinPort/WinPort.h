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
#endif /* FAR_PYTHON_GEN */

// CheckForKeyPress Checks if any of specified keys pressed
// Optionally preserves specified classes of input events
// return one plus array index of pressed key or zero if no one of specified keys is pressed
#define CFKP_KEEP_MATCHED_KEY_EVENTS    0x001
#define CFKP_KEEP_UNMATCHED_KEY_EVENTS  0x002
#define CFKP_KEEP_MOUSE_EVENTS          0x004
#define CFKP_KEEP_OTHER_EVENTS          0x100

// SetConsoleTweaks
#define EXCLUSIVE_CTRL_LEFT			0x00000001
#define EXCLUSIVE_CTRL_RIGHT		0x00000002
#define EXCLUSIVE_ALT_LEFT			0x00000004
#define EXCLUSIVE_ALT_RIGHT			0x00000008
#define EXCLUSIVE_WIN_LEFT			0x00000010
#define EXCLUSIVE_WIN_RIGHT			0x00000020

#define CONSOLE_PAINT_SHARP			0x00010000
#define CONSOLE_OSC52CLIP_SET		0x00020000

#define CONSOLE_TTY_PALETTE_OVERRIDE	0x00040000

// using this value causes SetConsoleTweaks not to change any existing tweak(s) but only return support status
#define TWEAKS_ONLY_QUERY_SUPPORTED 0xffffffffffffffff


#define TWEAK_STATUS_SUPPORT_EXCLUSIVE_KEYS	0x01
#define TWEAK_STATUS_SUPPORT_PAINT_SHARP	0x02
#define TWEAK_STATUS_SUPPORT_OSC52CLIP_SET	0x04
#define TWEAK_STATUS_SUPPORT_CHANGE_FONT	0x08
#define TWEAK_STATUS_SUPPORT_TTY_PALETTE	0x10
#define TWEAK_STATUS_SUPPORT_BLINK_RATE		0x20

// FindFirstFileWithFlags
#define FIND_FILE_FLAG_NO_DIRS		0x01
#define FIND_FILE_FLAG_NO_FILES		0x02
#define FIND_FILE_FLAG_NO_LINKS		0x04
#define FIND_FILE_FLAG_NO_DEVICES	0x08
#define FIND_FILE_FLAG_NO_CUR_UP	0x10 //skip virtual . and ..
#define FIND_FILE_FLAG_CASE_INSENSITIVE	0x1000 //currently affects only english characters
#define FIND_FILE_FLAG_NOT_ANNOYING	0x2000 //avoid sudo prompt if can't query some not very important information without it

#ifdef __cplusplus
extern "C" {
#endif
#ifndef FAR_PYTHON_GEN
	int WinPortMain(const char *full_exe_path, int argc, char **argv, int (*AppMain)(int argc, char **argv));
	void WinPortHelp();

	// entity = -1 - current backend flavor
	// entity = [0 .. until NULL result) - version information of different components
	const char *WinPortBackendInfo(int entity);

	// true means far2l runs under smoke testing and code must
	// not skip events from input queue that sometimes used to make UX smoother
	BOOL WinPortTesting();
#endif /* FAR_PYTHON_GEN */

#ifndef FAR_PYTHON_GEN
#define WINPORT_DECL_DEF(NAME, RV, ARGS) SHAREDSYMBOL RV WINPORT_##NAME ARGS;
#define WINPORT_DECL(NAME, RV, ARGS) SHAREDSYMBOL RV WINPORT_##NAME ARGS
#define WINPORT(NAME) WINPORT_##NAME
#include "WinPortDecl.h"
#endif /* FAR_PYTHON_GEN */

#ifndef FAR_PYTHON_GEN
//time/date
	SHAREDSYMBOL clock_t GetProcessUptimeMSec();//use instead of Windows's clock()
	WINPORT_DECL_DEF(FileTime_UnixToWin32, VOID, (struct timespec ts, FILETIME *lpFileTime))
	WINPORT_DECL_DEF(FileTime_Win32ToUnix, VOID, (const FILETIME *lpFileTime, struct timespec *ts))

	//%s -> %ls, %ws -> %ls
	SHAREDSYMBOL int vswprintf_ws2ls(wchar_t * ws, size_t len, const wchar_t * format, va_list arg );
	SHAREDSYMBOL int swprintf_ws2ls (wchar_t* ws, size_t len, const wchar_t* format, ...);

	SHAREDSYMBOL void SetPathTranslationPrefix(const wchar_t *prefix);

	SHAREDSYMBOL const wchar_t *GetPathTranslationPrefix();
	SHAREDSYMBOL const char *GetPathTranslationPrefixA();
#endif

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

class ConsoleForkScope
{
	HANDLE _parent{NULL}, _handle{NULL};

	ConsoleForkScope(const ConsoleForkScope &) = delete;
	ConsoleForkScope &operator =(const ConsoleForkScope &) = delete;

public:
	ConsoleForkScope() = default;

	~ConsoleForkScope()
	{
		if (_handle) {
			WINPORT(JoinConsole)(_parent, _handle);
		}
	}

	void Fork(HANDLE parent = NULL)
	{
		if (_handle) {
			WINPORT(DiscardConsole)(_handle);
		}
		_handle = WINPORT(ForkConsole)(parent);
		_parent = parent;
	}

	void Show()
	{
		HANDLE prev_handle = _handle;
		if (prev_handle) {
			_handle = WINPORT(ForkConsole)(prev_handle);
			WINPORT(JoinConsole)(_parent, prev_handle);
		}
	}

	void Discard()
	{
		if (_handle) {
			WINPORT(DiscardConsole)(_handle);
			_handle = NULL;
		}
	}

	HANDLE Handle() const { return _handle; }
	operator bool() const { return _handle != NULL; }
};

#endif
