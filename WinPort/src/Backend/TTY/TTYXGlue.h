#pragma once
#include <memory>
#include "../Backend.h"
#include "../FSClipboardBackend.h"

struct ITTYXGlue
{
	virtual ~ITTYXGlue() {}

	virtual DWORD GetModifiers() noexcept = 0;
	virtual bool SetClipboard(const std::string &s) noexcept = 0;
	virtual bool GetClipboard(std::string &s) noexcept = 0;
};

typedef std::shared_ptr<ITTYXGlue> ITTYXGluePtr;

// Starts far2l_ttyx.broker process and returns instance of glue that communicates to it
// NB: instance of glue must be destroyed by same thread as one that called StartTTYX in 
// order to correctly join broker process.
ITTYXGluePtr StartTTYX(const char *full_exe_path);

class TTYXClipboard : public IClipboardBackend
{
	ITTYXGluePtr _ttyx;
	FSClipboardBackend _fs_fallback;
	bool _empty_pending = false;

protected:
	virtual bool OnClipboardOpen();
	virtual void OnClipboardClose();
	virtual void OnClipboardEmpty();
	virtual bool OnClipboardIsFormatAvailable(UINT format);
	virtual void *OnClipboardSetData(UINT format, void *data);
	virtual void *OnClipboardGetData(UINT format);
	virtual UINT OnClipboardRegisterFormat(const wchar_t *lpszFormat);

public:
	TTYXClipboard(ITTYXGluePtr &ttyx);
	virtual ~TTYXClipboard();
};
