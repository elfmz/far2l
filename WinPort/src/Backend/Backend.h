#pragma once
#include "WinCompat.h"
#include <memory>

class IConsoleOutputBackend
{
public:
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
