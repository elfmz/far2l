#include <fcntl.h>
#include <utils.h>
#include <base64.h>
#include <ConvertUTF.h>
#include "TTYFar2lClipboardBackend.h"
#include "FSClipboardBackend.h"

TTYFar2lClipboardBackend::TTYFar2lClipboardBackend(IFar2lInterractor *interractor) :
	_interractor(interractor)
{
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
	delete _fallback_backend;
}

void TTYFar2lClipboardBackend::Far2lInterract(StackSerializer &stk_ser, bool wait)
{
	try {
		stk_ser.PushPOD('c');
		_interractor->Far2lInterract(stk_ser, wait);
	} catch (std::exception &e) {
		fprintf(stderr, "TTYFar2lClipboardBackend::Far2lInterract: %s\n", e.what());
	}
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
				fprintf(stderr, "TTYFar2lClipboardBackend::OnClipboardOpen: fallback\n");
				if (!_fallback_backend)
					_fallback_backend = new FSClipboardBackend;

				return _fallback_backend->OnClipboardOpen();

			default:
				fprintf(stderr, "TTYFar2lClipboardBackend::OnClipboardOpen: denied\n");
		}

	} catch (std::exception &e) {
		fprintf(stderr, "TTYFar2lClipboardBackend::OnClipboardOpen: %s\n", e.what());
	}

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

	} catch (std::exception &e) {
		fprintf(stderr, "TTYFar2lClipboardBackend::OnClipboardClose: %s\n", e.what());
	}
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

	} catch (std::exception &e) {
		fprintf(stderr, "TTYFar2lClipboardBackend::OnClipboardEmpty: %s\n", e.what());
	}
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

	} catch (std::exception &e) {
		fprintf(stderr, "TTYFar2lClipboardBackend::OnClipboardIsFormatAvailable: %s\n", e.what());
	}
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

#if (__WCHAR_MAX__ <= 0xffff)
		UTF32 *new_data = nullptr;
		if (format == CF_UNICODETEXT && len != 0) { // UTF16 -> UTF32
			int cnt = 0;
			const UTF16 *src = (const UTF16 *)data;
			CalcSpaceUTF16toUTF32(&cnt, &src, src + len / sizeof(UTF16), lenientConversion);
			new_data = (UTF32 *)malloc((cnt + 1) * sizeof(UTF32));
			if (new_data != nullptr) {
				new_data[cnt] = 0;
				src = (const UTF16 *)data;
				UTF32 *dst = new_data;
				ConvertUTF16toUTF32( &src, src + len / sizeof(UTF16), &dst, dst + cnt, lenientConversion);
				len = cnt * sizeof(UTF32);
			}
		}
		stk_ser.Push(new_data ? new_data : data, len);
		stk_ser.PushPOD(len);
		free(new_data);
#else
		stk_ser.Push(data, len);
		stk_ser.PushPOD(len);
#endif

		stk_ser.PushPOD(format);
		stk_ser.PushPOD('s');
		Far2lInterract(stk_ser, true);
		if (stk_ser.PopChar() == 1)
			return data;

	} catch (std::exception &e) {
		fprintf(stderr, "TTYFar2lClipboardBackend::OnClipboardSetData: %s\n", e.what());
	}
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
#if (__WCHAR_MAX__ <= 0xffff)
				if (format == CF_UNICODETEXT) { // UTF32 -> UTF16
					int cnt = 0;
					const UTF32 *src = (const UTF32 *)data;
					CalcSpaceUTF32toUTF16(&cnt, &src, src + len / sizeof(UTF32), lenientConversion);
					UTF16 *new_data = (UTF16 *)malloc((cnt + 1) * sizeof(UTF16));
					if (new_data != nullptr) {
						new_data[cnt] = 0;
						src = (const UTF32 *)data;
						UTF16 *dst = new_data;
						ConvertUTF32toUTF16( &src, src + len / sizeof(UTF32), &dst, dst + cnt, lenientConversion);
						free(data);
						data = new_data;
						len = cnt * sizeof(UTF16);
					}
				}
#endif

				return data;
			}
		}

	} catch (std::exception &e) {
		fprintf(stderr, "TTYFar2lClipboardBackend::OnClipboardGetData: %s\n", e.what());
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

	} catch (std::exception &e) {
		fprintf(stderr, "TTYFar2lClipboardBackend::OnClipboardRegisterFormat: %s\n", e.what());
	}
	return 0;
}
