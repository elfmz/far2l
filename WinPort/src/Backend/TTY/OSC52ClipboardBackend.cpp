#include "OSC52ClipboardBackend.h"
#include "WinPort.h"
#include <utils.h>

#include <RandomString.h>
#include <base64.h>
#include <UtfConvert.hpp>

OSC52ClipboardBackend::OSC52ClipboardBackend(IOSC52Interactor *interactor) :
	_interactor(interactor)
{
}

void *OSC52ClipboardBackend::OnClipboardSetData(UINT format, void *data)
{
	if (format == CF_UNICODETEXT) {
		std::string str;
		Wide2MB((const wchar_t *)data, WINPORT(ClipboardSize)(data) / sizeof(wchar_t), str);
		_interactor->OSC52SetClipboard(str.c_str(), _is_primary);
	}
	return FSClipboardBackend::OnClipboardSetData(format, data);
}

void *OSC52ClipboardBackend::OnClipboardGetData(UINT format) 
{
	// VK: first, we need to send ESC "[52;" _is _primary ? 'p' : 'c' ";?\a"
	// then we need to wait on guard until the data is arrived; and then 
	// decode it from base64 and return the result

	fprintf(stderr, "OSC52ClipboardBackend: OnClipboardGetData\n");

	const char*	stx = _interactor->OSC52RequestClipboardData(_is_primary);
	if (!stx)
		return FSClipboardBackend::OnClipboardGetData(format);

	std::string _text(stx);
	if (_text.empty())	
		return nullptr;

	const std::vector<unsigned char> &data	= base64_decode(_text.data(), _text.size());
	if (data.empty()) 
		return nullptr;

	void *out;
	if (format == CF_UNICODETEXT) {
		size_t utf8_len = data.size();
		while (utf8_len && !data[utf8_len - 1]) {
			--utf8_len;
		}

		std::wstring ws;
		MB2Wide((const char *)data.data(), utf8_len, ws);

		out = WINPORT(ClipboardAlloc)( (ws.size() + 1) * sizeof(wchar_t));
		if (out) {
			memcpy(out, ws.c_str(), (ws.size() + 1) * sizeof(wchar_t));
		}

	} else {
		out = WINPORT(ClipboardAlloc)(data.empty() ? 1 : data.size());
		if (out && !data.empty()) {
			memcpy(out, data.data(), data.size());
		}
	}

	return out;
}
