#pragma once
#include "WinCompat.h"
#include <string>

/// This file defines all interfacing between console API and rendering backends.

/// Increment FAR2L_BACKEND_ABI_VERSION each time when:
///   Something changed in code below.
///   "WinCompat.h" changed in a way affecting code below.
///   Behavior of backend's code changed in incompatible way.
#define FAR2L_BACKEND_ABI_VERSION	0x0B

#define NODETECT_NONE   0x0000
#define NODETECT_XI     0x0001
#define NODETECT_X      0x0002
#define NODETECT_F      0x0004
#define NODETECT_W      0x0008
#define NODETECT_A      0x0010
#define NODETECT_K      0x0020

class IConsoleOutputBackend
{
protected:
	virtual ~IConsoleOutputBackend() {}

public:
	virtual void OnConsoleOutputUpdated(const SMALL_RECT *areas, size_t count) = 0;
	virtual void OnConsoleOutputResized() = 0;
	virtual void OnConsoleOutputTitleChanged() = 0;
	virtual void OnConsoleOutputWindowMoved(bool absolute, COORD pos) = 0;
	virtual COORD OnConsoleGetLargestWindowSize() = 0;
	virtual void OnConsoleAdhocQuickEdit() = 0;
	virtual DWORD64 OnConsoleSetTweaks(DWORD64 tweaks) = 0;
	virtual void OnConsoleChangeFont() = 0;
	virtual void OnConsoleSaveWindowState() = 0;
	virtual void OnConsoleSetMaximized(bool maximized) = 0;
	virtual void OnConsoleExit() = 0;
	virtual bool OnConsoleIsActive() = 0;
	virtual void OnConsoleDisplayNotification(const wchar_t *title, const wchar_t *text) = 0;
	virtual bool OnConsoleBackgroundMode(bool TryEnterBackgroundMode) = 0;
	virtual bool OnConsoleSetFKeyTitles(const char **titles) = 0;
	virtual BYTE OnConsoleGetColorPalette() = 0;
	virtual void OnConsoleGetBasePalette(void *pbuff) = 0;
	virtual bool OnConsoleSetBasePalette(void *pbuff) = 0;
	virtual void OnConsoleOverrideColor(DWORD Index, DWORD *ColorFG, DWORD *ColorBK) = 0;
	virtual void OnConsoleSetCursorBlinkTime(DWORD interval) = 0;
};

class IClipboardBackend
{
protected:
	friend class ClipboardBackendSetter;
	virtual ~IClipboardBackend() {}

public:
	virtual bool OnClipboardOpen() = 0;
	virtual void OnClipboardClose() = 0;
	virtual void OnClipboardEmpty() = 0;
	virtual bool OnClipboardIsFormatAvailable(UINT format) = 0;
	virtual void *OnClipboardSetData(UINT format, void *data) = 0;
	virtual void *OnClipboardGetData(UINT format) = 0;
	virtual UINT OnClipboardRegisterFormat(const wchar_t *lpszFormat) = 0;
};

IClipboardBackend *WinPortClipboard_SetBackend(IClipboardBackend *clipboard_backend);

class ClipboardBackendSetter
{
	IClipboardBackend *_prev_cb = nullptr;
	bool _is_set = false;

public:
	inline bool IsSet() const { return _is_set; }

	template <class BACKEND_T, typename... ArgsT>
		inline void Set(ArgsT... args)
	{
		IClipboardBackend *cb = new BACKEND_T(args...);
		IClipboardBackend *prev_cb = WinPortClipboard_SetBackend(cb);
		if (!_is_set) {
			_prev_cb = prev_cb;
			_is_set = true;

		} else {
			delete prev_cb;
		}
	}

	inline ~ClipboardBackendSetter()
	{
		if (_is_set) {
			IClipboardBackend *cb = WinPortClipboard_SetBackend(_prev_cb);
			if (cb != _prev_cb) {
				delete cb;
			}
		}
	}
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

class IConsoleInput
{
protected:
	virtual ~IConsoleInput() {}

public:
	virtual IConsoleInput *ForkConsoleInput(HANDLE con_handle) = 0;
	virtual void JoinConsoleInput(IConsoleInput *con_in) = 0;

	virtual void Enqueue(const INPUT_RECORD *data, DWORD size) = 0;
	virtual DWORD Peek(INPUT_RECORD *data, DWORD size, unsigned int requestor_priority = 0) = 0;
	virtual DWORD Dequeue(INPUT_RECORD *data, DWORD size, unsigned int requestor_priority = 0) = 0;
	virtual DWORD Count(unsigned int requestor_priority = 0) = 0;
	virtual DWORD Flush(unsigned int requestor_priority = 0) = 0;
	virtual void WaitForNonEmpty(unsigned int requestor_priority = 0) = 0;
	virtual bool WaitForNonEmptyWithTimeout(unsigned int timeout_msec = (unsigned int)-1, unsigned int requestor_priority = 0) = 0;

	virtual unsigned int RaiseRequestorPriority() = 0;
	virtual void LowerRequestorPriority(unsigned int released_priority) = 0;
};

class ConsoleInputPriority
{
	IConsoleInput *_con_input;
	unsigned int _my_priority;

	public:
	inline ConsoleInputPriority(IConsoleInput *con_input)
		: _con_input(con_input),
		_my_priority(con_input->RaiseRequestorPriority())
	{
	}

	inline ~ConsoleInputPriority()
	{
		_con_input->LowerRequestorPriority(_my_priority);
	}


	inline operator int() const {return _my_priority; }
};

////////////////////////////////////////////////////////////////

class IConsoleOutput
{
protected:
	virtual ~IConsoleOutput() {}

	friend class DirectLineAccess;

	virtual CHAR_INFO *LockedDirectLineAccess(size_t line_index, unsigned int &width) = 0;
	virtual const wchar_t *LockedGetTitle() = 0;
	virtual void Unlock() = 0;

public:
	virtual unsigned int WaitForChange(unsigned int prev_change_id, unsigned int timeout_msec = -1) = 0;

	virtual IConsoleOutput *ForkConsoleOutput(HANDLE con_handle) = 0;
	virtual void JoinConsoleOutput(IConsoleOutput *con_out) = 0;

	virtual void SetBackend(IConsoleOutputBackend *listener) = 0;

	virtual void SetAttributes(DWORD64 attributes) = 0;
	virtual DWORD64 GetAttributes() = 0;

	virtual void SetCursorBlinkTime(DWORD interval) = 0;
	virtual void SetCursor(COORD pos) = 0;
	virtual void SetCursor(UCHAR height, bool visible) = 0;
	virtual COORD GetCursor() = 0;
	virtual COORD GetCursor(UCHAR &height, bool &visible) = 0;

	virtual void SetSize(unsigned int width, unsigned int height) = 0;
	virtual void GetSize(unsigned int &width, unsigned int &height) = 0;

	virtual COORD GetLargestConsoleWindowSize() = 0;
	virtual void SetWindowMaximized(bool maximized) = 0;

	virtual void SetWindowInfo(bool absolute, const SMALL_RECT &rect) = 0;
	virtual void SetTitle(const WCHAR *title) = 0;

	virtual DWORD GetMode() = 0;
	virtual void SetMode(DWORD mode) = 0;

	virtual void Read(CHAR_INFO *data, COORD data_size, COORD data_pos, SMALL_RECT &screen_rect) = 0;
	virtual void Write(const CHAR_INFO *data, COORD data_size, COORD data_pos, SMALL_RECT &screen_rect) = 0;
	virtual bool Read(CHAR_INFO &data, COORD screen_pos) = 0;
	virtual bool Write(const CHAR_INFO &data, COORD screen_pos) = 0;

	virtual size_t WriteString(const WCHAR *data, size_t count) = 0;
	virtual size_t WriteStringAt(const WCHAR *data, size_t count, COORD &pos) = 0;
	virtual size_t FillCharacterAt(WCHAR cCharacter, size_t count, COORD &pos) = 0;
	virtual size_t FillAttributeAt(DWORD64 qAttribute, size_t count, COORD &pos) = 0;
	
	virtual bool Scroll(const SMALL_RECT *lpScrollRectangle, const SMALL_RECT *lpClipRectangle, 
				COORD dwDestinationOrigin, const CHAR_INFO *lpFill) = 0;
				
	virtual void SetScrollRegion(SHORT top, SHORT bottom) = 0;
	virtual void GetScrollRegion(SHORT &top, SHORT &bottom) = 0;
	virtual void SetScrollCallback(PCONSOLE_SCROLL_CALLBACK pCallback, PVOID pContext) = 0;
	
	virtual void AdhocQuickEdit() = 0;
	virtual DWORD64 SetConsoleTweaks(DWORD64 tweaks) = 0;
	virtual void ConsoleChangeFont() = 0;
	virtual void ConsoleSaveWindowState() = 0;
	virtual bool IsActive() = 0;
	virtual void ConsoleDisplayNotification(const WCHAR *title, const WCHAR *text) = 0;
	virtual bool ConsoleBackgroundMode(bool TryEnterBackgroundMode) = 0;
	virtual bool SetFKeyTitles(const CHAR **titles) = 0;
	virtual BYTE GetColorPalette() = 0;
	virtual void GetBasePalette(void *p) = 0;
	virtual bool SetBasePalette(void *p) = 0;
	virtual void OverrideColor(DWORD Index, DWORD *ColorFG, DWORD *ColorBK) = 0;
	virtual void RepaintsDeferStart() = 0;
	virtual void RepaintsDeferFinish() = 0;

	inline std::wstring GetTitle()
	{
		std::wstring out(LockedGetTitle());
		Unlock();
		return out;
	}

	class DirectLineAccess
	{
		IConsoleOutput *_co;
		CHAR_INFO *_line;
		unsigned int _width;

	public:
		inline DirectLineAccess(IConsoleOutput *co, size_t line_index)
			: _co(co)
		{
			_line = _co->LockedDirectLineAccess(line_index, _width);
		}

		inline ~DirectLineAccess()
		{
			_co->Unlock();
		}

		inline CHAR_INFO *Line() { return _line; }
		inline unsigned int Width() const { return _width; }
	};
};

//////////////////////////////////////////////////////////////////////////////////

extern IConsoleOutput *g_winport_con_out;
extern IConsoleInput *g_winport_con_in;
extern const wchar_t *g_winport_backend;

//////////////////////////////////////////////////////////////////////////////////

struct WinPortMainBackendArg
{
	unsigned int abi_version; // set to/check with FAR2L_BACKEND_ABI_VERSION
	int argc;
	char **argv;
	int (*app_main)(int argc, char **argv);
	int *result;
	IConsoleOutput *winport_con_out;
	IConsoleInput *winport_con_in;
	bool ext_clipboard;
	bool norgb;
};

