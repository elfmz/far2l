#pragma once
#include "WinCompat.h"
#include "FSClipboardBackend.h"

struct IOSC52Interactor
{
	virtual void OSC52SetClipboard(const char *text, bool is_primary) = 0;
	virtual const char* OSC52RequestClipboardData(bool is_primary) = 0;
};

class OSC52ClipboardBackend : public FSClipboardBackend
{
	IOSC52Interactor *_interactor;
	bool _is_primary;

public:
	OSC52ClipboardBackend(IOSC52Interactor *interactor);
	virtual void *OnClipboardSetData(UINT format, void *data);

	virtual void *OnClipboardGetData(UINT format);

	virtual INT ChooseClipboard(INT format) {
		INT old = _is_primary ? 1 : 0;
		_is_primary = format == 1;
		return old;
	}
};
