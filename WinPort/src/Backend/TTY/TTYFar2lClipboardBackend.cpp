#include <fcntl.h>
#include <utils.h>
#include <base64.h>
#include "TTYFar2lClipboardBackend.h"
#include "FSClipboardBackend.h"

TTYFar2lClipboardBackend::TTYFar2lClipboardBackend(IFar2lInterractor *interractor) :
	_interractor(interractor)
{
	_prior_backend = WinPortClipboard_SetBackend(this);

	int fd = open(InMyConfig("tty_clipboard/me").c_str(), O_RDWR|O_CREAT, 0600);
	ssize_t r = 0;
	char buf[0x40] = {};
	if (fd != -1) {
		r = read(fd, buf, sizeof(buf));
	}

	if (r < (ssize_t)sizeof(buf)) {
		srand(fd ^ getpid() ^ time(NULL) ^ (unsigned int)(uintptr_t)&fd);
		gethostname(buf, sizeof(buf) - 1);
		size_t l = strnlen(buf, sizeof(buf) / 2);
		buf[l++] = '-';
		for (; l < sizeof(buf); ++l) {
			if (rand() & 3) {
				buf[l] = 'a' + rand() % ('z' + 1 - 'a');
			} else {
				buf[l] = '0' + rand() % ('9' + 1 - '0');
			}
		}
		if (fd != -1) {
			if (pwrite(fd, buf, sizeof(buf), 0) != sizeof(buf)) {
				perror("pwrite");
			}
		}
	}

	if (fd != -1)
		close(fd);

	_client_id.assign(buf, sizeof(buf));
	for (auto &c : _client_id) {
		if ( (c < 'a' || c > 'z') && (c < '0' || c > '9') && c != '-') {
			c = '_';
		}
	}
}


TTYFar2lClipboardBackend::~TTYFar2lClipboardBackend()
{
	WinPortClipboard_SetBackend(_prior_backend);
	delete _fallback_backend;
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
	if (_fallback_backend) {
		return _fallback_backend->OnClipboardOpen();
	}

	try {
		StackSerializer stk_ser;
		stk_ser.PushStr(_client_id);
		stk_ser.PushPOD('o');
		Far2lInterract(stk_ser, true);
		switch (stk_ser.PopChar()) {
			case 1:
				return true;

			case -1:
				if (!_fallback_backend)
					_fallback_backend = new FSClipboardBackend;

				return _fallback_backend->OnClipboardOpen();

			default: ;
		}

	} catch (std::exception &) {}
	return false;
}

void TTYFar2lClipboardBackend::OnClipboardClose()
{
	if (_fallback_backend) {
		_fallback_backend->OnClipboardClose();
		return;
	}

	try {
		StackSerializer stk_ser;
		stk_ser.PushPOD('c');
		Far2lInterract(stk_ser, false);

	} catch (std::exception &) {}
}

void TTYFar2lClipboardBackend::OnClipboardEmpty()
{
	if (_fallback_backend) {
		_fallback_backend->OnClipboardEmpty();
		return;
	}

	try {
		StackSerializer stk_ser;
		stk_ser.PushPOD('e');
		Far2lInterract(stk_ser, false);

	} catch (std::exception &) {}
}

bool TTYFar2lClipboardBackend::OnClipboardIsFormatAvailable(UINT format)
{
	if (_fallback_backend) {
		return _fallback_backend->OnClipboardIsFormatAvailable(format);
	}

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
	if (_fallback_backend) {
		return _fallback_backend->OnClipboardSetData(format, data);
	}

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
	if (_fallback_backend) {
		return _fallback_backend->OnClipboardGetData(format);
	}

	void *data = nullptr;
	try {
		StackSerializer stk_ser;
		stk_ser.PushPOD(format);
		stk_ser.PushPOD('g');
		Far2lInterract(stk_ser, true);
		uint32_t len = stk_ser.PopU32();
		if (len && len != (uint32_t)-1) {
			data = malloc(len);
			if (data) {
				stk_ser.Pop(data, len);
				return data;
			}
		}

	} catch (std::exception &) {
		free(data);
	}
	return nullptr;
}

UINT TTYFar2lClipboardBackend::OnClipboardRegisterFormat(const wchar_t *lpszFormat)
{
	if (_fallback_backend) {
		return _fallback_backend->OnClipboardRegisterFormat(lpszFormat);
	}

	try {
		StackSerializer stk_ser;
		stk_ser.PushStr(StrWide2MB(lpszFormat));
		stk_ser.PushPOD('r');
		Far2lInterract(stk_ser, true);
		return stk_ser.PopU32();

	} catch (std::exception &) {}
	return 0;
}
