#include "TTYFar2lClipboardBackend.h"
#include <utils.h>
#include <base64.h>

TTYFar2lClipboardBackend::TTYFar2lClipboardBackend(IFar2lInterractor *interractor) :
	_interractor(interractor)
{
	_default_backend = WinPortClipboard_SetBackend(this);
}


TTYFar2lClipboardBackend::~TTYFar2lClipboardBackend()
{
	WinPortClipboard_SetBackend(_default_backend);
}

void TTYFar2lClipboardBackend::Far2lInterract(StackSerializer &stk_ser, bool wait)
{
	try {
		stk_ser.PushPOD('c');
		_interractor->Far2lInterract(stk_ser, wait);
	} catch (std::exception &) {}
}

bool TTYFar2lClipboardBackend::OnClipboardOpen()
{
	try {
		StackSerializer stk_ser;
		stk_ser.PushPOD('o');
		Far2lInterract(stk_ser, true);
		if (stk_ser.PopChar() == 1) {
			return true;
		}

	} catch (std::exception &) {}
	return false;
}

void TTYFar2lClipboardBackend::OnClipboardClose()
{
	try {
		StackSerializer stk_ser;
		stk_ser.PushPOD('c');
		Far2lInterract(stk_ser, false);

	} catch (std::exception &) {}
}

void TTYFar2lClipboardBackend::OnClipboardEmpty()
{
	try {
		StackSerializer stk_ser;
		stk_ser.PushPOD('e');
		Far2lInterract(stk_ser, false);

	} catch (std::exception &) {}
}

bool TTYFar2lClipboardBackend::OnClipboardIsFormatAvailable(UINT format)
{
	try {
		StackSerializer stk_ser;
		stk_ser.PushPOD(format);
		stk_ser.PushPOD('a');
		Far2lInterract(stk_ser, true);
		if (stk_ser.PopChar() == 1)
			return true;

	} catch (std::exception &) {}
	return false;
}

void *TTYFar2lClipboardBackend::OnClipboardSetData(UINT format, void *data)
{
	uint32_t len = GetMallocSize(data);

	try {
		StackSerializer stk_ser;
		stk_ser.Push(data, len);
		stk_ser.PushPOD(len);
		stk_ser.PushPOD(format);
		stk_ser.PushPOD('s');
		Far2lInterract(stk_ser, true);
		if (stk_ser.PopChar() == 1)
			return data;

	} catch (std::exception &) {}
	return nullptr;
}

void *TTYFar2lClipboardBackend::OnClipboardGetData(UINT format)
{
	void *data = nullptr;
	try {
		StackSerializer stk_ser;
		stk_ser.PushPOD(format);
		stk_ser.PushPOD('g');
		Far2lInterract(stk_ser, true);
		uint32_t len = stk_ser.PopU32();
		if (len) {
			data = malloc(len);
			stk_ser.Pop(data, len);
			return data;
		}

	} catch (std::exception &) {
		free(data);
	}
	return nullptr;
}

UINT TTYFar2lClipboardBackend::OnClipboardRegisterFormat(const wchar_t *lpszFormat)
{
	try {
		StackSerializer stk_ser;
		stk_ser.PushStr(StrWide2MB(lpszFormat));
		stk_ser.PushPOD('r');
		Far2lInterract(stk_ser, true);
		return stk_ser.PopU32();

	} catch (std::exception &) {}
	return 0;
}
