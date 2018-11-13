#pragma once
#include "Backend.h"

class wxClipboardBackend : IClipboardBackend
{
	IClipboardBackend *_default_backend;
public:
	wxClipboardBackend();
	~wxClipboardBackend();

	virtual bool OnClipboardOpen();
	virtual void OnClipboardClose();
	virtual void OnClipboardEmpty();
	virtual bool OnClipboardIsFormatAvailable(UINT format);
	virtual void *OnClipboardSetData(UINT format, void *data);
	virtual void *OnClipboardGetData(UINT format);
	virtual UINT OnClipboardRegisterFormat(const wchar_t *lpszFormat);
};
