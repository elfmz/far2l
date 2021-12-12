#include <fcntl.h>
#include <utils.h>
#include <base64.h>
#include <UtfConvert.hpp>
#include "TTYFar2lClipboardBackend.h"
#include "WinPort.h"
#include "FarTTY.h"

#define CHUNK_SIZE 0x4000  // must be aligned by 0x100 and be less than 0x1000000

/////////////

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
	_set_data_thread.reset();
}

void TTYFar2lClipboardBackend::Far2lInterract(StackSerializer &stk_ser, bool wait)
{
	try {
		stk_ser.PushPOD(FARTTY_INTERRACT_CLIPBOARD);
		_interractor->Far2lInterract(stk_ser, wait);
	} catch (std::exception &e) {
		fprintf(stderr, "TTYFar2lClipboardBackend::Far2lInterract: %s\n", e.what());
	}
}

/////////////////////

uint64_t TTYFar2lClipboardBackend::GetDataID(UINT format)
{
	if (_fallback_backend || (_features&FARTTY_FEATCLIP_DATA_ID) == 0) {
		return 0;
	}

	uint64_t out = 0;
	StackSerializer stk_ser;
	stk_ser.PushPOD(format);
	stk_ser.PushPOD(FARTTY_INTERRACT_CLIP_GETDATAID);
	Far2lInterract(stk_ser, true);
	stk_ser.PopPOD(out);

	return out;
}

void *TTYFar2lClipboardBackend::GetCachedData(UINT format, uint32_t &len)
{
	std::lock_guard<std::mutex> lock(_mtx);

	auto cache_it = _cache.find(format);
	if (cache_it == _cache.end()) {
		return nullptr;
	}

	const uint64_t id = GetDataID(format);
	if (id && id == cache_it->second.id) {
		return MallocedVectorCopy(cache_it->second.data, len);
	}

	_cache.erase(cache_it);
	return nullptr;
}

/////////////////////

void TTYFar2lClipboardBackend::SetCachedData(UINT format, const void *data, uint32_t len, uint64_t id)
{
	std::lock_guard<std::mutex> lock(_mtx);
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

	auto no_fallback_open_counter = ++_no_fallback_open_counter;
	if (no_fallback_open_counter > 1) {
		if (no_fallback_open_counter > 2) {
			fprintf(stderr, "TTYFar2lClipboardBackend::OnClipboardOpen: no_fallback_open_counter too large (%d)\n", no_fallback_open_counter);
		}
		return true;
	}
	if (no_fallback_open_counter <= 0) {
		fprintf(stderr, "TTYFar2lClipboardBackend::OnClipboardOpen: no_fallback_open_counter too small (%d)\n", no_fallback_open_counter);
		_no_fallback_open_counter = 1;
	}

	_features = 0;

	try {
		StackSerializer stk_ser;
		stk_ser.PushStr(_client_id);
		stk_ser.PushPOD(FARTTY_INTERRACT_CLIP_OPEN);
		Far2lInterract(stk_ser, true);
		switch (stk_ser.PopChar()) {
			case 1: {
				if (!stk_ser.IsEmpty()) {
					try {
						stk_ser.PopPOD(_features);
					} catch (std::exception &e) {
						fprintf(stderr, "TTYFar2lClipboardBackend::OnClipboardOpen FEATURES: %s\n", e.what());
					}
				}
				fprintf(stderr, "TTYFar2lClipboardBackend::OnClipboardOpen: OK [0x%llx]\n", (unsigned long long)_features);
				return true;
			}

			case (char)-1:
				--_no_fallback_open_counter;
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

	--_no_fallback_open_counter;
	return false;
}

void TTYFar2lClipboardBackend::OnClipboardClose()
{
	if (_fallback_backend) {
		_fallback_backend->OnClipboardClose();
		return;
	}

	if (--_no_fallback_open_counter > 0) {
		return;
	}

	try {
		StackSerializer stk_ser;
		stk_ser.PushPOD(FARTTY_INTERRACT_CLIP_CLOSE);
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

	if (_set_data_thread) {
		std::lock_guard<std::mutex> lock(_mtx);
		_set_data_thread.reset();
	}

	try {
		StackSerializer stk_ser;
		stk_ser.PushPOD(FARTTY_INTERRACT_CLIP_EMPTY);
		Far2lInterract(stk_ser, false);

		std::lock_guard<std::mutex> lock(_mtx);
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

	if (format == CF_UNICODETEXT) { // using UTF8 instead of UTF32 to reduce traffic
		format = CF_TEXT;
	}

	if (_set_data_thread) {
		std::lock_guard<std::mutex> lock(_mtx);
		if (_set_data_thread && _set_data_thread->Pending()) {
			return (_set_data_thread->Format() == format);
		}
	}

	try {
		StackSerializer stk_ser;
		stk_ser.PushPOD(format);
		stk_ser.PushPOD(FARTTY_INTERRACT_CLIP_ISAVAIL);
		Far2lInterract(stk_ser, true);
		if (stk_ser.PopChar() == 1)
			return true;

		std::lock_guard<std::mutex> lock(_mtx);
		_cache.erase(format);

	} catch (std::exception &e) {
		fprintf(stderr, "TTYFar2lClipboardBackend::OnClipboardIsFormatAvailable: %s\n", e.what());
	}
	return false;
}

/** To improve UI responsivity setting clipboard data is done by separate thread,
 this thread sends data as sequence of CHUNK_SIZE blocks with 8:1 throttling
 to keep some part of bandwidth available for UI communication. Also this send can be
 cancelled at any moment by subsequent clipboard set/empty request.
 To allow getting clipboard data while transfer in progress OnClipboardGetData checks
 if set-data thread still pending and uses its data as current clipboard content if so
*/
void *TTYFar2lClipboardBackend::SetDataThread::ThreadProc()
{
	try {
		size_t ofs = 0;
		StackSerializer stk_ser;
		if ((_backend->_features & FARTTY_FEATCLIP_CHUNKED_SET) != 0 && _data.size() > CHUNK_SIZE) {
			DWORD busy_period_start = WINPORT(GetTickCount)();
			unsigned int i = 0;
			do {
				fprintf(stderr, "TTYFar2lClipboardBackend::SetDataThread: chunk @0x%lx of 0x%lx\n",
					(unsigned long)ofs, (unsigned long)_data.size());
				++i;
				stk_ser.Push(&_data[ofs], CHUNK_SIZE);
				stk_ser.PushPOD(uint16_t(CHUNK_SIZE >> 8));
				stk_ser.PushPOD(FARTTY_INTERRACT_CLIP_SETDATACHUNK);
				// wait for reply only each 16th request to reduce round-trip delays
				_backend->Far2lInterract(stk_ser, (i % 16) == 0);
				ofs+= CHUNK_SIZE;
				stk_ser.Clear();
				if (_cancel) { // discard pending chunks by posting zero-length chunk
					stk_ser.PushPOD(uint16_t(0));
					stk_ser.PushPOD(FARTTY_INTERRACT_CLIP_SETDATACHUNK);
					_backend->Far2lInterract(stk_ser, false);
					throw std::runtime_error("cancelled");
				}
				const DWORD busy_period = WINPORT(GetTickCount)() - busy_period_start;
				if (busy_period >= 256) { // throttle bandwidth usage to avoid full stuck of UI
					WINPORT(Sleep)(busy_period / 8);
					busy_period_start = WINPORT(GetTickCount)();
				}
			} while (_data.size() - ofs > CHUNK_SIZE);
			fprintf(stderr, "TTYFar2lClipboardBackend::SetDataThread: final @0x%lx of 0x%lx\n",
				(unsigned long)ofs, (unsigned long)_data.size());

		} else {
			fprintf(stderr, "TTYFar2lClipboardBackend::SetDataThread: 0x%lx\n", (unsigned long)_data.size());
		}

		const uint32_t len = uint32_t(_data.size() - ofs);
		stk_ser.Push(&_data[ofs], len);
		stk_ser.PushPOD(len);
		stk_ser.PushPOD(_format);
		stk_ser.PushPOD(FARTTY_INTERRACT_CLIP_SETDATA);
		_backend->Far2lInterract(stk_ser, true);
		_backend->OnSetDataThreadComplete(this, stk_ser);

	} catch (std::exception &e) {
		fprintf(stderr, "TTYFar2lClipboardBackend::SetDataThread: %s\n", e.what());
	}

	try {
		_backend->OnClipboardClose();
	} catch (std::exception &e) {
		fprintf(stderr, "TTYFar2lClipboardBackend::SetDataThread - CLOSE: %s\n", e.what());
	}

	_pending = false;
	return nullptr;
}

/////////

TTYFar2lClipboardBackend::SetDataThread::SetDataThread(TTYFar2lClipboardBackend *backend, UINT format, const void *data, uint32_t len)
	: _backend(backend), _format(format)
{
	if (_format == CF_UNICODETEXT) { // CF_UNICODETEXT -> CF_TEXT to reduce traffic
		const size_t wchar_cnt = wcsnlen((const wchar_t *)data, len / sizeof(wchar_t));
		UtfConverter<wchar_t, unsigned char>((const wchar_t *)data, wchar_cnt).CopyToVector(_data);
		_format = CF_TEXT;

	} else {
		_data.resize(len);
		memcpy(_data.data(), data, len);
	}

	_backend->OnClipboardOpen();
	_pending = true;
	if (!StartThread()) {
		fprintf(stderr, "TTYFar2lClipboardBackend::SetDataThread: can't start thread\n");
		_pending = false;
		_backend->OnClipboardClose();
	}
}

TTYFar2lClipboardBackend::SetDataThread::~SetDataThread()
{
	_cancel = true;
	WaitThread();
}

void *TTYFar2lClipboardBackend::OnClipboardSetData(UINT format, void *data)
{
	if (_fallback_backend) {
		fprintf(stderr, "TTYFar2lClipboardBackend::OnClipboardSetData(0x%x): fallback\n", format);
		return _fallback_backend->OnClipboardSetData(format, data);
	}

	const uint32_t len = GetMallocSize(data);
	std::lock_guard<std::mutex> lock(_mtx);
	_set_data_thread.reset();
	_set_data_thread.reset(new SetDataThread(this, format, data, len));
	return data;
}

void TTYFar2lClipboardBackend::OnSetDataThreadComplete(TTYFar2lClipboardBackend::SetDataThread *set_data_thread, StackSerializer &stk_ser)
{
	try {
		const char reply = stk_ser.PopChar();
		fprintf(stderr, "TTYFar2lClipboardBackend::OnSetDataThreadComplete: reply=%d\n", (int)reply);
		if (reply == 1 && (_features & FARTTY_FEATCLIP_DATA_ID) != 0) {
			uint64_t id = 0;
			stk_ser.PopPOD(id);
			if (id) {
				SetCachedData(set_data_thread->Format(),
					set_data_thread->Data().data(), set_data_thread->Data().size(), id);
			}
		}

	} catch (std::exception &e) {
		fprintf(stderr, "TTYFar2lClipboardBackend::OnSetDataThreadComplete: %s\n", e.what());
	}
}

/////////

void *TTYFar2lClipboardBackend::OnClipboardGetData(UINT format)
{
	if (_fallback_backend) {
		fprintf(stderr, "TTYFar2lClipboardBackend::OnClipboardGetData(0x%x): fallback\n", format);
		return _fallback_backend->OnClipboardGetData(format);
	}

	uint32_t len = 0;
	if (format == CF_UNICODETEXT) { // use UTF8 instead of UTF32 to reduce traffic
		void *data = InnerClipboardGetData(CF_TEXT, len);
		if (data) {
			const size_t chars_cnt = strnlen((const char *)data, len);
			UtfConverter<char, wchar_t> cvt((const char *)data, chars_cnt);
			free(data);
			return cvt.MallocedCopy();
		}
	}

	return InnerClipboardGetData(format, len);
}

void *TTYFar2lClipboardBackend::InnerClipboardGetData(UINT format, uint32_t &len)
{
	if (_set_data_thread) {
		std::lock_guard<std::mutex> lock(_mtx);
		if (_set_data_thread && _set_data_thread->Pending()) {
			if (_set_data_thread->Format() != format) {
				fprintf(stderr, "TTYFar2lClipboardBackend::InnerClipboardGetData(0x%x): pending miss\n", format);
				return nullptr;
			}
			fprintf(stderr, "TTYFar2lClipboardBackend::InnerClipboardGetData(0x%x): pending hit\n", format);
			return MallocedVectorCopy(_set_data_thread->Data(), len);
		}
	}

	void *data = GetCachedData(format, len);
	if (data) {
		fprintf(stderr, "TTYFar2lClipboardBackend::InnerClipboardGetData(0x%x): cache hit\n", format);
		return data;
	}
	fprintf(stderr, "TTYFar2lClipboardBackend::InnerClipboardGetData(0x%x): cache miss\n", format);

	try {
		StackSerializer stk_ser;
		stk_ser.PushPOD(format);
		stk_ser.PushPOD(FARTTY_INTERRACT_CLIP_GETDATA);
		Far2lInterract(stk_ser, true);
		len = stk_ser.PopU32();
		if (len == (uint32_t)-1) {
			len = 0;

		} else if (len) {
			data = malloc(len);
			if (data) {
				stk_ser.Pop(data, len);
				if ((_features & FARTTY_FEATCLIP_DATA_ID) != 0) {
					uint64_t id = 0;
					stk_ser.PopPOD(id);
					if (id) {
						SetCachedData(format, data, len, id);
					}
				}
				return data;
			}
		}

	} catch (std::exception &e) {
		fprintf(stderr, "TTYFar2lClipboardBackend::InnerClipboardGetData: %s\n", e.what());
		free(data);
	}

	return nullptr;
}

/////////

UINT TTYFar2lClipboardBackend::OnClipboardRegisterFormat(const wchar_t *lpszFormat)
{
	if (_fallback_backend) {
		return _fallback_backend->OnClipboardRegisterFormat(lpszFormat);
	}

	try {
		StackSerializer stk_ser;
		stk_ser.PushStr(StrWide2MB(lpszFormat));
		stk_ser.PushPOD(FARTTY_INTERRACT_CLIP_REGISTER_FORMAT);
		Far2lInterract(stk_ser, true);
		return stk_ser.PopU32();

	} catch (std::exception &e) {
		fprintf(stderr, "TTYFar2lClipboardBackend::OnClipboardRegisterFormat: %s\n", e.what());
	}
	return 0;
}
