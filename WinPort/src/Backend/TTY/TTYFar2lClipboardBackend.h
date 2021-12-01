#pragma once
#include <memory>
#include <atomic>
#include <mutex>
#include <StackSerializer.h>
#include "WinCompat.h"
#include "Backend.h"
#include "IFar2lInterractor.h"
#include "FSClipboardBackend.h"


class TTYFar2lClipboardBackend : public IClipboardBackend
{
	struct CachedData
	{
		uint64_t id;
		std::vector<unsigned char> data;
	};
	struct Cache : std::mutex, std::map<UINT, CachedData> { } _cache;

	std::unique_ptr<FSClipboardBackend> _fallback_backend;
	IFar2lInterractor *_interractor;
	std::string _client_id;
	bool _data_id_supported = true;

	void Far2lInterract(StackSerializer &stk_ser, bool wait);
	uint64_t GetDataID(UINT format);
	void *GetCachedData(UINT format);
	void SetCachedData(UINT format, const void *data, uint32_t len, uint64_t id);

public:
	TTYFar2lClipboardBackend(IFar2lInterractor *interractor);
	virtual ~TTYFar2lClipboardBackend();

	virtual bool OnClipboardOpen();
	virtual void OnClipboardClose();
	virtual void OnClipboardEmpty();
	virtual bool OnClipboardIsFormatAvailable(UINT format);
	virtual void *OnClipboardSetData(UINT format, void *data);
	virtual void *OnClipboardGetData(UINT format);
	virtual UINT OnClipboardRegisterFormat(const wchar_t *lpszFormat);
};
