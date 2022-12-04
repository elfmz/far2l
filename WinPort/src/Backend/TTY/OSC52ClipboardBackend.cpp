#include "OSC52ClipboardBackend.h"
#include "WinPort.h"
#include <utils.h>

OSC52ClipboardBackend::OSC52ClipboardBackend(IOSC52Interractor *interractor) :
	_interractor(interractor)
{
}

void *OSC52ClipboardBackend::OnClipboardSetData(UINT format, void *data)
{
	if (format == CF_UNICODETEXT) {
		std::string str;
		Wide2MB((const wchar_t *)data, WINPORT(ClipboardSize)(data), str);
		_interractor->OSC52SetClipboard(str.c_str());
	}
	return FSClipboardBackend::OnClipboardSetData(format, data);
}
