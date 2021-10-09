#pragma once
#include "WinCompat.h"
#include <memory>

class IConsoleOutputBackend
{
public:
	virtual ~IConsoleOutputBackend() {};

	virtual void OnConsoleOutputUpdated(const SMALL_RECT *areas, size_t count) = 0;
	virtual void OnConsoleOutputResized() = 0;
	virtual void OnConsoleOutputTitleChanged() = 0;
	virtual void OnConsoleOutputWindowMoved(bool absolute, COORD pos) = 0;
	virtual COORD OnConsoleGetLargestWindowSize() = 0;
	virtual void OnConsoleAdhocQuickEdit() = 0;
	virtual DWORD OnConsoleSetTweaks(DWORD tweaks) = 0;
	virtual void OnConsoleChangeFont() = 0;
	virtual void OnConsoleSetMaximized(bool maximized) = 0;
	virtual void OnConsoleExit() = 0;
	virtual bool OnConsoleIsActive() = 0;
	virtual void OnConsoleDisplayNotification(const wchar_t *title, const wchar_t *text) = 0;
	virtual bool OnConsoleBackgroundMode(bool TryEnterBackgroundMode) = 0;
	virtual bool OnConsoleSetFKeyTitles(const char **titles) = 0;
};

class IClipboardBackend
{
public:
	virtual ~IClipboardBackend() {};

	virtual bool OnClipboardOpen() = 0;
	virtual void OnClipboardClose() = 0;
	virtual void OnClipboardEmpty() = 0;
	virtual bool OnClipboardIsFormatAvailable(UINT format) = 0;
	virtual void *OnClipboardSetData(UINT format, void *data) = 0;
	virtual void *OnClipboardGetData(UINT format) = 0;
	virtual UINT OnClipboardRegisterFormat(const wchar_t *lpszFormat) = 0;
};

std::shared_ptr<IClipboardBackend> WinPortClipboard_SetBackend(std::shared_ptr<IClipboardBackend> &clipboard_backend);

class ClipboardBackendSetter
{
	std::shared_ptr<IClipboardBackend> _prev_cb;
	bool _is_set = false;

public:
	inline bool IsSet() const { return _is_set; }

	template <class BACKEND_T, typename... ArgsT>
		inline void Set(ArgsT... args)
	{
		std::shared_ptr<IClipboardBackend> cb = std::make_shared<BACKEND_T>(args...);
		std::shared_ptr<IClipboardBackend> prev_cb = WinPortClipboard_SetBackend(cb);
		if (!_is_set) {
			_prev_cb = prev_cb;
			_is_set = true;
		}
	}

	inline ~ClipboardBackendSetter()
	{
		if (_is_set) {
			WinPortClipboard_SetBackend(_prev_cb);
		}
	}
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

class IConsoleInput
{
public:
	virtual ~IConsoleInput() {};

	virtual void Enqueue(const INPUT_RECORD *data, DWORD size) = 0;
	virtual DWORD Peek(INPUT_RECORD *data, DWORD size, unsigned int requestor_priority = 0) = 0;
	virtual DWORD Dequeue(INPUT_RECORD *data, DWORD size, unsigned int requestor_priority = 0) = 0;
	virtual DWORD Count(unsigned int requestor_priority = 0) = 0;
	virtual DWORD Flush(unsigned int requestor_priority = 0) = 0;
	virtual bool WaitForNonEmpty(unsigned int timeout_msec = (unsigned int)-1, unsigned int requestor_priority = 0) = 0;

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
	friend class DirectLineAccess;

	virtual CHAR_INFO *DirectLineAccess_Start(size_t line_index, unsigned int &width) = 0;
	virtual void DirectLineAccess_Finish() = 0;

public:
	virtual ~IConsoleOutput() {};
	virtual void SetBackend(IConsoleOutputBackend *listener) = 0;

	virtual void SetAttributes(USHORT attributes) = 0;
	virtual USHORT GetAttributes() = 0;
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
	virtual std::wstring GetTitle() = 0;

	virtual DWORD GetMode() = 0;
	virtual void SetMode(DWORD mode) = 0;

	virtual void Read(CHAR_INFO *data, COORD data_size, COORD data_pos, SMALL_RECT &screen_rect) = 0;
	virtual void Write(const CHAR_INFO *data, COORD data_size, COORD data_pos, SMALL_RECT &screen_rect) = 0;
	virtual bool Read(CHAR_INFO &data, COORD screen_pos) = 0;
	virtual bool Write(const CHAR_INFO &data, COORD screen_pos) = 0;

	virtual size_t WriteString(const WCHAR *data, size_t count) = 0;
	virtual size_t WriteStringAt(const WCHAR *data, size_t count, COORD &pos) = 0;
	virtual size_t FillCharacterAt(WCHAR cCharacter, size_t count, COORD &pos) = 0;
	virtual size_t FillAttributeAt(WORD wAttribute, size_t count, COORD &pos) = 0;
	
	virtual bool Scroll(const SMALL_RECT *lpScrollRectangle, const SMALL_RECT *lpClipRectangle, 
				COORD dwDestinationOrigin, const CHAR_INFO *lpFill) = 0;
				
	virtual void SetScrollRegion(SHORT top, SHORT bottom) = 0;
	virtual void GetScrollRegion(SHORT &top, SHORT &bottom) = 0;
	virtual void SetScrollCallback(PCONSOLE_SCROLL_CALLBACK pCallback, PVOID pContext) = 0;
	
	virtual void AdhocQuickEdit() = 0;
	virtual DWORD SetConsoleTweaks(DWORD tweaks) = 0;
	virtual void ConsoleChangeFont() = 0;
	virtual bool IsActive() = 0;
	virtual void ConsoleDisplayNotification(const WCHAR *title, const WCHAR *text) = 0;
	virtual bool ConsoleBackgroundMode(bool TryEnterBackgroundMode) = 0;
	virtual bool SetFKeyTitles(const CHAR **titles) = 0;

	class DirectLineAccess
	{
		IConsoleOutput *_co;
		CHAR_INFO *_line;
		unsigned int _width;

	public:
		inline DirectLineAccess(IConsoleOutput *co, size_t line_index)
			: _co(co)
		{
			_line = _co->DirectLineAccess_Start(line_index, _width);
		}

		inline ~DirectLineAccess()
		{
			_co->DirectLineAccess_Finish();
		}

		inline CHAR_INFO *Line() { return _line; }
		inline unsigned int Width() const { return _width; }
	};
};

//////////////////////////////////////////////////////////////////////////////////

extern IConsoleOutput *g_winport_con_out;
extern IConsoleInput *g_winport_con_in;

//////////////////////////////////////////////////////////////////////////////////

struct WinPortMainGUI_Arg
{
	const char *abi_hash;
	int argc;
	char **argv;
	int (*app_main)(int argc, char **argv);
	int *result;
	IConsoleOutput *winport_con_out;
	IConsoleInput *winport_con_in;
};
