#pragma once
#include "Backend.h"

class wxClipboardBackend : public IClipboardBackend
{
	int _copy_mode;
	int _paste_mode;

public:
	wxClipboardBackend(int copy_mode, int paste_mode);
	~wxClipboardBackend();

	virtual bool OnClipboardOpen();
	virtual void OnClipboardClose();
	virtual void OnClipboardEmpty();
	virtual bool OnClipboardIsFormatAvailable(UINT format);
	virtual void *OnClipboardSetData(UINT format, void *data);
	virtual void *OnClipboardGetData(UINT format);
	virtual UINT OnClipboardRegisterFormat(const wchar_t *lpszFormat);
};
