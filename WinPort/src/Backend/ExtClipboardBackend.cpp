#include "ExtClipboardBackend.h"
#include "WinPort.h"
#include <utils.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
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

bool ExtClipboardBackend::OnClipboardOpen()
{
	return true;
}

void ExtClipboardBackend::OnClipboardClose()
{
}

void ExtClipboardBackend::OnClipboardEmpty()
{
	std::string cmd = _exec;
	cmd+= " empty";

	int r = system(cmd.c_str());
	if (r != 0) {
		fprintf(stderr, "%s: r=%d for %s\n", __FUNCTION__, r, cmd.c_str());
	}
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
		fprintf(stderr, "%s: err=%d for %s\n", __FUNCTION__, errno, cmd.c_str());
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
		fprintf(stderr, "%s: err=%d for %s\n", __FUNCTION__, errno, cmd.c_str());
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
	pclose(f);

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
