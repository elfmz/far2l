#include <fcntl.h>
#include <utils.h>
#include <base64.h>
#include <UtfConvert.hpp>
#include "TTYFar2lClipboardBackend.h"

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

uint64_t TTYFar2lClipboardBackend::GetDataID(UINT format)
{
	if (_fallback_backend || !_data_id_supported) {
		return 0;
	}

	uint64_t out = 0;
	try {
		StackSerializer stk_ser;
		stk_ser.PushPOD(format);
		stk_ser.PushPOD('i');
		Far2lInterract(stk_ser, true);
		stk_ser.PopPOD(out);

	} catch (std::exception &e) {
		fprintf(stderr, "TTYFar2lClipboardBackend::GetDataID: %s\n", e.what());
		_data_id_supported = false;
		out = 0;
	}

	return out;
}

void *TTYFar2lClipboardBackend::GetCachedData(UINT format)
{
	std::lock_guard<std::mutex> lock(_cache);

	auto cache_it = _cache.find(format);
	if (cache_it == _cache.end()) {
		return nullptr;
	}

	const uint64_t id = GetDataID(format);
	if (id && id == cache_it->second.id) {
#if (__WCHAR_MAX__ <= 0xffff)
		if (format == CF_UNICODETEXT) { // UTF32 -> UTF16
			uint32_t len = cache_it->second.id.len;
			return UtfConverter<uint32_t, uint16_t>
				((const uint32_t*)cache_it->second.data.data(), len / sizeof(uint32_t)).MallocedCopy(len);
		}
#endif
		unsigned char *data = (unsigned char *)malloc(cache_it->second.data.size());
		if (data) {
			memcpy(data, cache_it->second.data.data(), cache_it->second.data.size());
			return data;
		}
	}

	_cache.erase(cache_it);
	return nullptr;
}

void TTYFar2lClipboardBackend::SetCachedData(UINT format, const void *data, uint32_t len, uint64_t id)
{
	std::lock_guard<std::mutex> lock(_cache);
	try {
		CachedData &cd = _cache[format];
		cd.id = id;
		cd.data.resize(len);
		memcpy(cd.data.data(), data, len);

	} catch(std::exception &e) {
		fprintf(stderr,
			"TTYFar2lClipboardBackend::SetCachedData(0x%u, %p, %u, 0x%llx): %s\n",
			format, data, len, (unsigned long long)id, e.what());
		_cache.erase(format);
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

			case (char)-1:
				fprintf(stderr, "TTYFar2lClipboardBackend::OnClipboardOpen: fallback\n");
				if (!_fallback_backend)
					_fallback_backend.reset(new FSClipboardBackend);

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

		std::lock_guard<std::mutex> lock(_cache);
		_cache.clear();

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

		std::lock_guard<std::mutex> lock(_cache);
		_cache.erase(format);

	} catch (std::exception &e) {
		fprintf(stderr, "TTYFar2lClipboardBackend::OnClipboardIsFormatAvailable: %s\n", e.what());
	}
	return false;
}

void *TTYFar2lClipboardBackend::OnClipboardSetData(UINT format, void *data)
{
	if (_fallback_backend) {
		fprintf(stderr, "TTYFar2lClipboardBackend::OnClipboardSetData(0x%x): fallback\n", format);
		return _fallback_backend->OnClipboardSetData(format, data);
	}

	uint32_t len = GetMallocSize(data);

	try {
		StackSerializer stk_ser;

#if (__WCHAR_MAX__ <= 0xffff)
		UTF32 *new_data = nullptr;
		if (format == CF_UNICODETEXT && len != 0) { // UTF16 -> UTF32
			new_data = UtfConverter<uint16_t, uint32_t>
				((const uint16_t*)data, len / sizeof(uint16_t)).MallocedCopy(len);
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
		if (stk_ser.PopChar() == 1) {
			if (_data_id_supported) {
				try {
					uint64_t id = 0;
					stk_ser.PopPOD(id);
					if (id) {
						SetCachedData(format, data, len, id);
					}

				} catch (std::exception &e) {
					fprintf(stderr, "TTYFar2lClipboardBackend::OnClipboardSetData: ID failed - %s\n", e.what());
					_data_id_supported = false;
				}
			}
			return data;
		}

	} catch (std::exception &e) {
		fprintf(stderr, "TTYFar2lClipboardBackend::OnClipboardSetData: %s\n", e.what());
	}
	return nullptr;
}

void *TTYFar2lClipboardBackend::OnClipboardGetData(UINT format)
{
	if (_fallback_backend) {
		fprintf(stderr, "TTYFar2lClipboardBackend::OnClipboardGetData(0x%x): fallback\n", format);
		return _fallback_backend->OnClipboardGetData(format);
	}

	void *data = GetCachedData(format);
	if (data) {
		fprintf(stderr, "TTYFar2lClipboardBackend::OnClipboardGetData(0x%x): cache hit\n", format);
		return data;
	}
	fprintf(stderr, "TTYFar2lClipboardBackend::OnClipboardGetData(0x%x): cache miss\n", format);

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
				if (_data_id_supported) {
					try {
						uint64_t id = 0;
						stk_ser.PopPOD(id);
						if (id) {
							SetCachedData(format, data, len, id);
						}

					} catch (std::exception &e) {
						fprintf(stderr, "TTYFar2lClipboardBackend::OnClipboardGetData::ID: %s\n", e.what());
						_data_id_supported = false;
					}
				}
#if (__WCHAR_MAX__ <= 0xffff)
				if (format == CF_UNICODETEXT) { // UTF32 -> UTF16
					void *new_data = UtfConverter<uint32_t, uint16_t>
						((const uint32_t*)data, len / sizeof(uint32_t)).MallocedCopy(len);
					if (new_data != nullptr) {
						free(data);
						data = new_data;
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
