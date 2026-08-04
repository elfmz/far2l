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
	mutable union U
	{
		void *raw = nullptr;
		wchar_t *pws;
		unsigned char *pmb;
		unsigned char lmb[sizeof(void *) / sizeof(unsigned char)];
		wchar_t lws[sizeof(void *) / sizeof(wchar_t)];
	} _data;
	// if _len < 0 && -len > sizeof(_data): using _data.pmb
	// if _len < 0 && -len <= sizeof(_data): using _data.lmb
	// if _len >= 0 && _len * sizeof(wc) >= sizeof(_data): using _data.pws
	// if _len >= 0 && _len * sizeof(wc) < sizeof(_data): using _data.lmb
	mutable int _len = 0;

	EcoString(const EcoString& src) = delete;
	EcoString &operator =(const EcoString& src) = delete;

	void MakeEmpty();
	bool MakeWideLength(int len);

	bool EnsureWide() const;

	void CopyTo(wchar_t *dst, int ofs, int cnt) const;

public:
	EcoString() = default;
	~EcoString();

	static void sDebugPrintStats(const char *info);

	inline int Size() const
	{
		return (_len < 0) ? -_len : _len;
	}

	bool Assign(const wchar_t *data, int len, bool compact = false);

	void Swap(EcoString &another);

	bool Truncate(int len = 0);
	bool Expand(int len, wchar_t ch = L' ');
	bool Resize(int len, wchar_t ch = L' ');

	bool Replace(int pos, int rcnt, wchar_t ch, int cnt = 1);
	bool Replace(int pos, int rcnt, const wchar_t *data, int cnt = -1);

	inline bool Insert(int pos, wchar_t ch, int cnt = 1)
	{
		return Replace(pos, 0, ch, cnt);
	}

	inline bool Insert(int pos, const wchar_t *data, int cnt = -1)
	{
		return Replace(pos, 0, data, cnt);
	}

	bool Remove(int ofs, int len);

	DWORD Transcode(UINT oldCodepage, UINT codepage);

	int Find(wchar_t ch, int pos = 0) const;

	wchar_t *Ptr();
	const wchar_t *CPtr() const;
	const wchar_t operator[](int i) const;
	wchar_t &operator[](int i);
};
