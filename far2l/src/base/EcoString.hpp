#pragma once

#include <sys/types.h>
#include <string.h>
#include <string>
#include <cctweaks.h>
#include <WinCompat.h>
#include "locale.hpp"
#include "lang.hpp"


enum SetCPFlags
{
	SETCP_NOERROR    = 0x00000000,
	SETCP_WC2MBERROR = 0x00000001,
	SETCP_MB2WCERROR = 0x00000002,
	SETCP_OTHERERROR = 0x10000000,
};

// memory-economic string, used primarily for Edit.hpp/Edit.cpp
class EcoString
{
	union U
	{
		wchar_t *ptr = nullptr;
		wchar_t local[sizeof(void *) / sizeof(wchar_t)];
	} _data;
	int32_t _len = 0;

	void MakeEmpty();
	bool MakeLength(int32_t len);

	EcoString(const EcoString& src) = delete;
	EcoString &operator =(const EcoString& src) = delete;

public:
	EcoString() = default;
	~EcoString();

	bool Assign(const wchar_t *data, int32_t len);

	void Swap(EcoString &another);

	bool Truncate(int32_t len = 0);
	bool Expand(int32_t len, wchar_t ch = L' ');
	bool Resize(int32_t len, wchar_t ch = L' ');

	bool Replace(int32_t pos, int32_t rcnt, wchar_t ch, int32_t cnt = 1);
	bool Replace(int32_t pos, int32_t rcnt, const wchar_t *data, int32_t cnt = -1);

	inline bool Insert(int32_t pos, wchar_t ch, int32_t cnt = 1)
	{
		return Replace(pos, 0, ch, cnt);
	}

	inline bool Insert(int32_t pos, const wchar_t *data, int32_t cnt = -1)
	{
		return Replace(pos, 0, data, cnt);
	}

	bool Remove(int32_t ofs, int32_t len);

	DWORD Transcode(UINT oldCodepage, UINT codepage);

	int Find(wchar_t ch, int pos = 0) const;

	inline wchar_t *Ptr()
	{
		return ((_len + 1) * sizeof(wchar_t) > sizeof(_data)) ? _data.ptr : _data.local;
	}

	inline const wchar_t *CPtr() const
	{
		return ((_len + 1) * sizeof(wchar_t) > sizeof(_data)) ? _data.ptr : _data.local;
	}

	inline int32_t Size() const
	{
		return _len;
	}

	const wchar_t &operator[](int i) const
	{
		// allow access to ending NUL char as Edit.cpp doing this sometimes for historically legal reasons
		ASSERT_MSG(i >= 0 && i <= _len,  "EcoString[] const: bad %d while _len=%d\n", i, _len);

		if ((_len + 1) * sizeof(wchar_t) > sizeof(_data)) {
			return _data.ptr[i];
		}

		return _data.local[i];
	}

	wchar_t &operator[](int i)
	{
		// allow access to ending NUL char as Edit.cpp doing this sometimes for historically legal reasons
		ASSERT_MSG(i >= 0 && i <= _len,  "EcoString[]: bad %d while _len=%d\n", i, _len);

		if ((_len + 1) * sizeof(wchar_t) > sizeof(_data)) {
			return _data.ptr[i];
		}

		return _data.local[i];
	}
};
