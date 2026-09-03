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

// Memory-economic string, used primarily for Edit.hpp/Edit.cpp
// Reasons:
// - sizeof(EcoString) == sizeof(void *) + sizeof(int)
// - When assigned with compact=true and if data contains only ASCII-representable wchars
//   then data is kept in compact ASCII form without NUL termination character, however
//   later automatically converted to wchars on any later modification or CPtr()/Ptr() use
//   Use sDebugPrintStats to see current per-kind allocations statistics
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

	void MakeEmpty();
	bool MakeWideLength(int len);

	bool EnsureWide() const;
	bool TryAssignCompact(const wchar_t *data, int len);

public:
	static void sDebugPrintStats(const char *info);

	EcoString() = default;
	~EcoString();

	EcoString(EcoString &&o) noexcept
	{
		Swap(o);
	}

	EcoString &operator=(EcoString &&o) noexcept
	{
		Swap(o);
		return *this;
	}


	EcoString(const EcoString& src)
	{
		operator=(src);
	}

	EcoString &operator =(const EcoString& src);

	inline int Size() const
	{
		return __builtin_abs(_len);// (_len < 0) ? -_len : _len;
	}

	bool Assign(const wchar_t *data, int len, bool try_compact = true);
	void Compact(); // makes string compact if possible
	bool IsCompact() const { return _len < 0; }
	void CopyTo(wchar_t *dst, int ofs, int cnt) const;
	void CopyTo(std::wstring &dst) const;
	void CopyTo(FARString &dst) const;
	void CopyTo(EcoString &dst) const { dst = *this; }

	bool EqualTo(const wchar_t *data, int cnt) const;
	bool EqualTo(const std::wstring &to) const { return EqualTo(to.c_str(), to.size()); }
	bool EqualTo(const FARString &to) const { return EqualTo(to.CPtr(), to.GetLength()); }

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

	wchar_t At(int i) const;
	void Set(int i, wchar_t wc) const;

	bool IsXxxfix(int i) const
	{
		// ASCII chars cant be prefix/suffix, right?
		return _len > 0 && CharClasses::IsXxxfix(At(i));
	}

	bool IsFullWidth(int i) const
	{
		// ASCII chars cant be fullwidth, right?
		return _len > 0 && CharClasses::IsFullWidth(CPtr() + i);
	}

	class Access
	{
		EcoString &_es;
		const int _i;
	public:
		Access(EcoString &es, int i) : _es(es), _i(i) {}
		operator wchar_t() const { return _es.At(_i); }
		Access &operator =(wchar_t c) { _es.Set(_i, c); return *this; }
		Access &operator =(const Access &a) { _es.Set(_i, a); return *this; }
	};

	inline wchar_t operator[](int i) const { return At(i); }
	inline Access operator[](int i) { return Access(*this, i); }
};
