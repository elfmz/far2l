#pragma once

#include <memory>
#include <KeyFileHelper.h>
#include <SharedResource.h>
#include "WinCompat.h"
#include "Backend.h"


class FSClipboardBackend : public IClipboardBackend
{
	SharedResource _shared_resource;
	std::shared_ptr<KeyFileHelper> _kfh;

public:
	FSClipboardBackend();
	virtual ~FSClipboardBackend();

	virtual bool OnClipboardOpen();
	virtual void OnClipboardClose();
	virtual void OnClipboardEmpty();
	virtual bool OnClipboardIsFormatAvailable(UINT format);
	virtual void *OnClipboardSetData(UINT format, void *data);
	virtual void *OnClipboardGetData(UINT format);
	virtual UINT OnClipboardRegisterFormat(const wchar_t *lpszFormat);
};
