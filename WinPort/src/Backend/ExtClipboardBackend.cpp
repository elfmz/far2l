#include "ExtClipboardBackend.h"
#include "WinPort.h"
#include <utils.h>
#include <stdio.h>
#include <UtfConvert.hpp>

ExtClipboardBackend::ExtClipboardBackend(const char *exec)
	:
	_exec(exec)
{
	QuoteCmdArgIfNeed(_exec);
}


ExtClipboardBackend::~ExtClipboardBackend()
{
}

bool ExtClipboardBackend::RunSimpleCommand(const char *arg)
{
	std::string cmd = _exec;
	cmd+= ' ';
	cmd+= arg;

	std::string result;
	POpen(result, cmd.c_str());
	return atoi(result.c_str()) > 0;
}

bool ExtClipboardBackend::OnClipboardOpen()
{
	return true;
}

void ExtClipboardBackend::OnClipboardClose()
{
}

void ExtClipboardBackend::OnClipboardEmpty()
{
	RunSimpleCommand("empty");
}

bool ExtClipboardBackend::OnClipboardIsFormatAvailable(UINT format)
{
	return format == CF_UNICODETEXT || format == CF_TEXT;
}

void *ExtClipboardBackend::OnClipboardSetData(UINT format, void *data)
{
	if (format != CF_UNICODETEXT && format != CF_TEXT)
		return data;

	std::string cmd = _exec;
	cmd+= " set";

	FILE *f = popen(cmd.c_str(), "w");
	if (!f) {
		return data;
	}

	const size_t len = WINPORT(ClipboardSize)(data);

	if (format == CF_UNICODETEXT) {
		UtfConverter<wchar_t, unsigned char> cvt((const wchar_t *)data, len / sizeof(wchar_t));
		fwrite(cvt.data(), 1, cvt.size(), f);
	} else {
		fwrite(data, 1, len, f);
	}
	pclose(f);
	return data;
}

void *ExtClipboardBackend::OnClipboardGetData(UINT format)
{
	if (format != CF_UNICODETEXT && format != CF_TEXT)
		return nullptr;

	std::string cmd = _exec;
	cmd+= " get";

	FILE *f = popen(cmd.c_str(), "r");
	if (!f) {
		return nullptr;
	}

	std::vector<unsigned char> data;
	unsigned char buf[0x800];
	for (;;) {
		size_t n = fread(buf, 1, sizeof(buf), f);
		if (n) {
			data.resize(data.size() + n);
			memcpy(data.data() + data.size() - n, buf, n);
		}
		if (n < sizeof(buf)) break;
	}

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

UINT ExtClipboardBackend::OnClipboardRegisterFormat(const wchar_t *lpszFormat)
{
	return 0;
}
