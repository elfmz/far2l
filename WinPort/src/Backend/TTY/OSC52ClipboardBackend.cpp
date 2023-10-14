#include "OSC52ClipboardBackend.h"
#include "WinPort.h"
#include <utils.h>

OSC52ClipboardBackend::OSC52ClipboardBackend(IOSC52Interactor *interactor) :
	_interactor(interactor)
{
}

void *OSC52ClipboardBackend::OnClipboardSetData(UINT format, void *data)
{
	if (format == CF_UNICODETEXT) {
		std::string str;
		Wide2MB((const wchar_t *)data, WINPORT(ClipboardSize)(data), str);
		_interactor->OSC52SetClipboard(str.c_str());
	}
	return FSClipboardBackend::OnClipboardSetData(format, data);
}
