#include "headers.hpp"

#include "EcoString.hpp"
#include "StackHeapArray.hpp"

static struct Stats
{
	unsigned long char_local{};
	unsigned long char_heap{};
	unsigned long wide_local{};
	unsigned long wide_heap{};
	unsigned long failed_allocs{};
} s_stats;

void EcoString::sDebugPrintStats(const char *info)
{
	if (s_stats.failed_allocs != 0) {
		fprintf(stderr, "! EcoString: char_local:%lu char_heap:%lu wide_local:%lu wide_heap:%lu failed_allocs:%lu (%s)\n",
			s_stats.char_local, s_stats.char_heap, s_stats.wide_local, s_stats.wide_heap, s_stats.failed_allocs, info);
	} else {
		fprintf(stderr, "EcoString: char_local:%lu char_heap:%lu wide_local:%lu wide_heap:%lu (%s)\n",
			s_stats.char_local, s_stats.char_heap, s_stats.wide_local, s_stats.wide_heap, info);
	}
}

EcoString::~EcoString()
{
	MakeEmpty();
}

EcoString &EcoString::operator =(const EcoString& src)
{
	if (this != &src) {
		if (src._len < 0) {
			MakeEmpty();
			_len = src._len;
			_data = src._data;
			if (size_t(-_len) > sizeof(_data)) {
				_data.pmb = (unsigned char *)malloc(-_len);
				if (LIKELY(_data.pws)) {
					memcpy(_data.pmb, src._data.pmb, -_len);
					++s_stats.char_heap;
				} else {
					fprintf(stderr, "EcoString::operator= strdup failed len=%d\n", _len);
					_len = 0;
				}
			} else {
				++s_stats.char_local;
			}
		} else {
			Assign(src.CPtr(), src.Size());
		}
	}
	return *this;
}

void EcoString::Swap(EcoString &another)
{
	std::swap(_data, another._data);
	std::swap(_len, another._len);
}

int EcoString::Find(wchar_t ch, int pos) const
{
	if (_len >= 0) {
		const wchar_t *cptr = ((_len + 1) * sizeof(wchar_t) > sizeof(_data)) ? _data.pws : _data.lws;
		for (;pos < _len; ++pos) {
			if (cptr[pos] == ch) {
				return pos;
			}
		}
	} else if (unsigned(ch) <= 0xff) {
		const int len = -_len;
		const unsigned char *cptr = (size_t(len) > sizeof(_data)) ? _data.pmb : _data.lmb;
		for (;pos < len; ++pos) {
			if (cptr[pos] == unsigned(ch)) {
				return pos;
			}
		}
	}

	return -1;
}

void EcoString::MakeEmpty()
{
	if (_len < 0) {
		if (size_t(-_len) > sizeof(_data)) {
			free(_data.pmb);
			s_stats.char_heap--;
		} else {
			s_stats.char_local--;
		}
	} else if ((_len + 1) * sizeof(wchar_t) > sizeof(_data)) {
		free(_data.pws);
		s_stats.wide_heap--;
	} else if (_len > 0) {
		s_stats.wide_local--;
	}
	_len = 0;
	_data.raw = nullptr;
}

bool EcoString::TryAssignCompact(const wchar_t *data, int len)
{
	if ((len + 1) * sizeof(wchar_t) <= sizeof(_data)) {
		return false; // use wide form if it can fit into local data
	}

	// check that data contains only chars with values representable in byte range: 0 .. 0xff
	for (auto i = 0; i < len; ++i) {
		if (unsigned(data[i]) > 0xff) {
			return false;
		}
	}

	if (len > int(sizeof(_data))) {
		unsigned char *pmb = (unsigned char *)malloc(len);
		if (!pmb) {
			fprintf(stderr, "EcoString::TryAssignCompact: malloc failed len=%d\n", len);
			s_stats.failed_allocs++;
			return false;
		}
		for (auto i = 0; i < len; ++i) {
			pmb[i] = unsigned(data[i]);
		}
		MakeEmpty();
		_data.pmb = pmb;
		s_stats.char_heap++;
	} else {
		MakeEmpty();
		for (auto i = 0; i < len; ++i) {
			_data.lmb[i] = unsigned(data[i]);
		}
		s_stats.char_local++;
	}
	_len = -len;
	return true;
}

bool EcoString::Assign(const wchar_t *data, int len, bool try_compact)
{
	if (!try_compact || !TryAssignCompact(data, len)) {
		if (!MakeWideLength(len)) {
			return false;
		}
		wmemcpy(Ptr(), data, len);
	}
	return true;
}

void EcoString::Compact()
{
	if (_len > 0) {
		EcoString tmp;
		if (tmp.TryAssignCompact(CPtr(), _len)) {
			Swap(tmp);
		}
	}
}

bool EcoString::EnsureWide() const
{
	if (_len >= 0) {
		return true;
	}
	const size_t alloc_len = sizeof(wchar_t) * (1 - _len);
	wchar_t *pws = (wchar_t *)malloc(alloc_len);
	if (!pws) {
		s_stats.failed_allocs++;
		fprintf(stderr, "EcoString::EnsureWide: malloc failed len=%ld\n", (unsigned long)alloc_len);
		return false;
	}
	s_stats.wide_heap++;
	pws[-_len] = 0;
	if (size_t(-_len) > sizeof(_data)) {
		for (auto i = -_len; i-- > 0;) {
			pws[i] = (wchar_t)(unsigned int)_data.pmb[i];
		}
		free(_data.pmb);
		s_stats.char_heap--;
	} else {
		for (auto i = -_len; i-- > 0;) {
			pws[i] = (wchar_t)(unsigned int)_data.lmb[i];
		}
		s_stats.char_local--;
	}
	_data.pws = pws;
	_len = -_len;
	return true;
}


template <class CHAR_LEFT, class CHAR_RIGHT>
	static bool TypeInvariantEqual(const CHAR_LEFT *left, const CHAR_RIGHT *right, int cnt)
{
	for (int i = 0; i < cnt; ++i) {
		if ((unsigned int)left[i] != (unsigned int)right[i]) {
			return false;
		}
	}
	return true;
}

bool EcoString::EqualTo(const wchar_t *data, int cnt) const
{
	if (cnt != Size())
		return false;

	if (_len < 0) {
		if (size_t(cnt) > sizeof(_data)) {
			return TypeInvariantEqual(_data.pmb, data, cnt);
		}
		return TypeInvariantEqual(_data.lmb, data, cnt);

	} else if (((cnt + 1) * sizeof(wchar_t) > sizeof(_data))) {
		return TypeInvariantEqual(_data.pws, data, cnt);
	}
	return TypeInvariantEqual(_data.lws, data, cnt);
}

void EcoString::CopyTo(wchar_t *dst, int ofs, int cnt) const
{
	if (_len < 0) {
		if (size_t(-_len) > sizeof(_data)) {
			for (auto i = 0; i < cnt; ++i) {
				dst[i] = (wchar_t)(unsigned int)_data.pmb[i + ofs];
			}
		} else {
			for (auto i = 0; i < cnt; ++i) {
				dst[i] = (wchar_t)(unsigned int)_data.lmb[i + ofs];
			}
		}
	} else if (((_len + 1) * sizeof(wchar_t) > sizeof(_data))) {
		wmemcpy(dst, &_data.pws[ofs], cnt);
	} else {
		wmemcpy(dst, &_data.lws[ofs], cnt);
	}
}

void EcoString::CopyTo(std::wstring &dst) const
{
	if (_len < 0) {
		if (size_t(-_len) > sizeof(_data)) {
			dst.assign(_data.pmb, _data.pmb + -_len);
		} else {
			dst.assign(&_data.lmb[0], &_data.lmb[0] + -_len);
		}
	} else if (((_len + 1) * sizeof(wchar_t) > sizeof(_data))) {
		dst.assign(&_data.pws[0], _len);
	} else {
		dst.assign(&_data.lws[0], _len);
	}
}

void EcoString::CopyTo(FARString &dst) const
{
	if (const auto sz = Size()) {
		wchar_t *buf = dst.GetBuffer(sz);
		if (buf) {
			CopyTo(buf, 0, sz);
			dst.ReleaseBuffer(sz);
			return;
		}
	}
	dst.Clear();
}

bool EcoString::MakeWideLength(int len)
{
	if ((len + 1) * sizeof(wchar_t) > sizeof(_data.lws)) {
		wchar_t *pws = (wchar_t *)malloc((len + 1) * sizeof(wchar_t));
		if (!pws) {
			s_stats.failed_allocs++;
			return false;
		}
		MakeEmpty();
		_data.pws = pws;
		s_stats.wide_heap++;
	} else {
		MakeEmpty();
		if (len > 0) {
			s_stats.wide_local++;
		}
	}

	_len = len;
	Ptr()[len] = 0;
	return true;
}

bool EcoString::Replace(int pos, int rcnt, wchar_t ch, int cnt)
{
	const int prev_len = Size();
	if (pos + rcnt > prev_len) {
		fprintf(stderr, "EcoString::ReplaceC: pos{%d} + rcnt{%d} > abs(_len{%d})\n", pos, rcnt, _len);
		return false;
	}
	EcoString new_str;
	if (!new_str.MakeWideLength(prev_len - rcnt + cnt)) {
		return false;
	}
	CopyTo(new_str.Ptr(), 0, pos);
	wmemset(new_str.Ptr() + pos, ch, cnt);
	CopyTo(new_str.Ptr() + pos + cnt, pos + rcnt, prev_len - pos - rcnt);
	Swap(new_str);
	return true;
}

bool EcoString::Replace(int pos, int rcnt, const wchar_t *data, int cnt)
{
	if (cnt < 0) {
		cnt = wcslen(data);
	}
	const int prev_len = Size();
	if (pos + rcnt > prev_len) {
		fprintf(stderr, "EcoString::Replace: pos{%d} + rcnt{%d} > abs(_len{%d})\n", pos, rcnt, _len);
		return false;
	}
	EcoString new_str;
	if (!new_str.MakeWideLength(prev_len - rcnt + cnt)) {
		return false;
	}
	CopyTo(new_str.Ptr(), 0, pos);
	wmemcpy(new_str.Ptr() + pos, data, cnt);
	CopyTo(new_str.Ptr() + pos + cnt, pos + rcnt, prev_len - pos - rcnt);
	Swap(new_str);
	return true;
}

bool EcoString::Remove(int ofs, int len)
{
	if (len == 0) {
		return true;
	}
	const int prev_len = Size();
	if (ofs >= prev_len) {
		fprintf(stderr, "EcoString::Remove: ofs{%u} >= _len{%u}\n", ofs, prev_len);
		return false;
	}
	if (ofs + len > prev_len) {
		fprintf(stderr, "EcoString::Remove: ofs{%u} + len{%u} > _len{%u}\n", ofs, len, prev_len);
		len = prev_len - ofs;
	}
	return Replace(ofs, len, L'\0', 0);
}


bool EcoString::Truncate(int len)
{
	const int prev_len = Size();
	if (len >= prev_len) {
		return true;
	}
	return Replace(len, prev_len - len, L'\0', 0);
}

bool EcoString::Expand(int len, wchar_t ch)
{
	const int prev_len = Size();
	if (len <= prev_len) {
		return true;
	}
	return Replace(prev_len, 0, ch, len - prev_len);
}

bool EcoString::Resize(int len, wchar_t ch)
{
	if (len > _len) {
		return Expand(len, ch);
	}
	return Truncate(len);
}


DWORD EcoString::Transcode(UINT oldCodepage, UINT codepage)
{
	if (!EnsureWide()) {
		return SETCP_OTHERERROR;
	}
	DWORD Ret = SETCP_NOERROR;
	DWORD wc2mbFlags = WC_NO_BEST_FIT_CHARS;
	BOOL UsedDefaultChar = FALSE;
	LPBOOL lpUsedDefaultChar = &UsedDefaultChar;

	if (oldCodepage == CP_UTF7 || oldCodepage == CP_UTF8 || oldCodepage == CP_UTF16LE
			|| oldCodepage == CP_UTF16BE)	// BUGBUG: CP_SYMBOL, 50xxx, 57xxx too
	{
		wc2mbFlags = 0;
		lpUsedDefaultChar = nullptr;
	}

	DWORD mb2wcFlags = MB_ERR_INVALID_CHARS;

	if (codepage == CP_UTF7)	// BUGBUG: CP_SYMBOL, 50xxx, 57xxx too
		mb2wcFlags = 0;

	if (Size()) {
		const int length = WINPORT(WideCharToMultiByte)(
			oldCodepage, wc2mbFlags, CPtr(), Size(), nullptr, 0, nullptr, lpUsedDefaultChar);

		if (UsedDefaultChar) {
			Ret|= SETCP_WC2MBERROR;
		}

		StackHeapArray<char> decoded(length);
		if (!decoded.Get()) {
			Ret|= SETCP_OTHERERROR;
			return Ret;
		}

		WINPORT(WideCharToMultiByte)(oldCodepage, 0, CPtr(), Size(), decoded.Get(), decoded.Count(), nullptr, nullptr);
		int length2 = WINPORT(MultiByteToWideChar)(codepage, mb2wcFlags, decoded.Get(), decoded.Count(), nullptr, 0);

		if (length2 <= 0 && WINPORT(GetLastError)() == ERROR_NO_UNICODE_TRANSLATION) {
			Ret|= SETCP_MB2WCERROR;
			length2 = WINPORT(MultiByteToWideChar)(codepage, 0, decoded.Get(), decoded.Count(), nullptr, 0);
			if (length2 < 0) {
				Ret|= SETCP_OTHERERROR;
				return Ret;
			}
		}

		if (!MakeWideLength(length2)) {
			Ret|= SETCP_OTHERERROR;
			fprintf(stderr, "EcoString::Transcode(%u, %u): length2=%d alloc failed\n", oldCodepage, codepage, length2);
			return Ret;
		}

		const int length3 = WINPORT(MultiByteToWideChar)(codepage, 0, decoded.Get(), decoded.Count(), Ptr(), Size());
		if (length2 != length3) {
			fprintf(stderr, "EcoString::Transcode(%u, %u): length2{%d} != length3{%d}\n", oldCodepage, codepage, length2, length3);
			Ret|= SETCP_OTHERERROR;
		}
	}

	return Ret;
}

wchar_t *EcoString::Ptr()
{
	if (!EnsureWide()) {
		return nullptr;
	}
	return ((_len + 1) * sizeof(wchar_t) > sizeof(_data)) ? _data.pws : _data.lws;
}

const wchar_t *EcoString::CPtr() const
{
	if (!EnsureWide()) {
		return nullptr;
	}
	return ((_len + 1) * sizeof(wchar_t) > sizeof(_data)) ? _data.pws : _data.lws;
}

wchar_t EcoString::At(int i) const
{
	const size_t sz = Size();
	if (size_t(i) == sz) {
		return 0; // allow access to ending NUL char as Edit.cpp doing this sometimes for historically legal reasons
	}
	ASSERT_MSG(i >= 0 && size_t(i) < sz,  "EcoString::At: bad %d while _len=%d\n", i, _len);

	if (_len < 0) {
		return (wchar_t)(unsigned char)((sz > sizeof(_data)) ? _data.pmb[i] : _data.lmb[i]);
	}

	return ((_len + 1) * sizeof(wchar_t) > sizeof(_data)) ? _data.pws[i] : _data.lws[i];
}

void EcoString::Set(int i, wchar_t wc) const
{
	if (wc == 0 && i == Size()) {
		return;
	}
	ASSERT_MSG(i >= 0 && i < Size(),  "EcoString::Set: bad %d while _len=%d; wc=0x%x\n", i, _len, (unsigned int)wc);
	if (unsigned(wc) <= 0xff && _len < 0) {
		if (size_t(-_len) > sizeof(_data)) {
			_data.pmb[i] = unsigned(wc);
		} else {
			_data.lmb[i] = unsigned(wc);
		}
	} else if (EnsureWide()) {
		if (((_len + 1) * sizeof(wchar_t) > sizeof(_data))) {
			_data.pws[i] = wc;
		} else {
			_data.lws[i] = wc;
		}
	}
}
