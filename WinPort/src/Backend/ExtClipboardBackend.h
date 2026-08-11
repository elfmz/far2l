#pragma once
#include "WinCompat.h"
#include "Backend.h"


class ExtClipboardBackend : public IClipboardBackend
{
	std::string _exec;

public:
	ExtClipboardBackend(const char *exec);
	virtual ~ExtClipboardBackend();

	virtual bool OnClipboardOpen();
	virtual void OnClipboardClose();
	virtual void OnClipboardEmpty();
	virtual bool OnClipboardIsFormatAvailable(UINT format);
	virtual void *OnClipboardSetData(UINT format, void *data);
	virtual void *OnClipboardGetData(UINT format);
	virtual UINT OnClipboardRegisterFormat(const wchar_t *lpszFormat);
	virtual INT ChooseClipboard(INT format);
};

