#pragma once
#include "WinCompat.h"
#include "FSClipboardBackend.h"

struct IOSC52Interactor
{
	virtual void OSC52SetClipboard(const char *text, bool is_primary) = 0;
	virtual std::string OSC52RequestClipboardData(bool is_primary) = 0;
};

class OSC52ClipboardBackend : public FSClipboardBackend
{
	IOSC52Interactor *_interactor;
	bool _is_primary {false};

public:
	OSC52ClipboardBackend(IOSC52Interactor *interactor);
	virtual void *OnClipboardSetData(UINT format, void *data);

	virtual void *OnClipboardGetData(UINT format);

	virtual INT ChooseClipboard(INT format) {
		_is_primary = format == 1;
		return _is_primary;
	}
};
