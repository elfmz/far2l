#pragma once
#include <memory>
#include <atomic>
#include <map>
#include <string>
#include <mutex>
#include <Threaded.h>
#include <StackSerializer.h>
#include "WinCompat.h"
#include "Backend.h"
#include "IFar2lInteractor.h"
#include "FSClipboardBackend.h"


class TTYFar2lClipboardBackend : public IClipboardBackend
{
	friend class SetDataThread;

	class SetDataThread : Threaded
	{
		TTYFar2lClipboardBackend *_backend;
		UINT _format;
		std::vector<unsigned char> _data;
		std::atomic<bool> _cancel{false}, _pending{false};

		virtual void *ThreadProc();

	public:
		SetDataThread(TTYFar2lClipboardBackend *backend, UINT format, const void *data, uint32_t len);
		virtual ~SetDataThread();

		void WaitCompletion() { WaitThread(); }
		bool Cancelled() const { return _cancel; }
		bool Pending() const { return _pending; }
		UINT Format() const { return _format; }
		const std::vector<unsigned char> &Data() const { return _data; }
	};

	struct CachedData
	{
		uint64_t id;
		std::vector<unsigned char> data;
	};
	struct Cache : std::map<UINT, CachedData> { } _data_cache;
	std::map<std::string, UINT> _formats_cache;

	std::unique_ptr<FSClipboardBackend> _fallback_backend;
	IFar2lInteractor *_interactor;
	std::atomic<int> _no_fallback_open_counter{0};
	std::shared_ptr<SetDataThread> _set_data_thread;

	std::mutex _mtx; // guards _cache, _set_data_thread


	std::string _client_id;
	uint64_t _features = 0;

	void Far2lInteract(StackSerializer &stk_ser, bool wait);
	bool GetCachedData(UINT format, void *&data, uint32_t &len);
	void SetCachedData(UINT format, const void *data, uint32_t len, uint64_t id);
	void OnSetDataThreadComplete(SetDataThread *set_data_thread, StackSerializer &stk_ser);
	void *InnerClipboardGetData(UINT format, uint32_t &len);

public:
	TTYFar2lClipboardBackend(IFar2lInteractor *interactor);
	virtual ~TTYFar2lClipboardBackend();

	virtual bool OnClipboardOpen();
	virtual void OnClipboardClose();
	virtual void OnClipboardEmpty();
	virtual bool OnClipboardIsFormatAvailable(UINT format);
	virtual void *OnClipboardSetData(UINT format, void *data);
	virtual void *OnClipboardGetData(UINT format);
	virtual UINT OnClipboardRegisterFormat(const wchar_t *lpszFormat);
};
