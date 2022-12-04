#pragma once
#include "WinCompat.h"
#include "FSClipboardBackend.h"

struct IOSC52Interractor
{
	virtual void OSC52SetClipboard(const char *text) = 0;
};

class OSC52ClipboardBackend : public FSClipboardBackend
{
	IOSC52Interractor *_interractor;

public:
	OSC52ClipboardBackend(IOSC52Interractor *interractor);
	virtual void *OnClipboardSetData(UINT format, void *data);
};
