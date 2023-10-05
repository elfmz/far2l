#pragma once
#include "WinCompat.h"
#include "FSClipboardBackend.h"

struct IOSC52Interactor
{
	virtual void OSC52SetClipboard(const char *text) = 0;
};

class OSC52ClipboardBackend : public FSClipboardBackend
{
	IOSC52Interactor *_interactor;

public:
	OSC52ClipboardBackend(IOSC52Interactor *interactor);
	virtual void *OnClipboardSetData(UINT format, void *data);
};
