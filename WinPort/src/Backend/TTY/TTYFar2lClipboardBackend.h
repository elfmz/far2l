#pragma once
#include <memory>
#include <atomic>
#include <StackSerializer.h>
#include "WinCompat.h"
#include "Backend.h"
#include "IFar2lInterractor.h"


class TTYFar2lClipboardBackend : public IClipboardBackend
{
	IClipboardBackend *_fallback_backend = nullptr;
	IFar2lInterractor *_interractor;
	std::string _client_id;

	void Far2lInterract(StackSerializer &stk_ser, bool wait);

public:
	TTYFar2lClipboardBackend(IFar2lInterractor *interractor);
	virtual ~TTYFar2lClipboardBackend();

	virtual bool OnClipboardOpen();
	virtual void OnClipboardClose();
	virtual void OnClipboardEmpty();
	virtual bool OnClipboardIsFormatAvailable(UINT format);
	virtual void *OnClipboardSetData(UINT format, void *data);
	virtual void *OnClipboardGetData(UINT format);
	virtual UINT OnClipboardRegisterFormat(const wchar_t *lpszFormat);
};
