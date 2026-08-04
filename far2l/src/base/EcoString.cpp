#include "headers.hpp"

#include "EcoString.hpp"
#include "StackHeapArray.hpp"

EcoString::~EcoString()
{
	MakeEmpty();
}

void EcoString::Swap(EcoString &another)
{
	std::swap(_data, another._data);
	std::swap(_len, another._len);
}

int EcoString::Find(wchar_t ch, int pos) const
{
	const auto *cptr = CPtr();
	for (;pos < _len; ++pos) {
		if (cptr[pos] == ch) {
			return pos;
		}
	}
	return -1;
}

void EcoString::MakeEmpty()
{
	if ((_len + 1) * sizeof(wchar_t) > sizeof(_data.local)) {
		free(_data.ptr);
	}
	_len = 0;
	_data.ptr = nullptr;
}

bool EcoString::MakeLength(int32_t len)
{
	if ((len + 1) * sizeof(wchar_t) > sizeof(_data.local)) {
		wchar_t *ptr = (wchar_t *)malloc((len + 1) * sizeof(wchar_t));
		if (!ptr) {
			return false;
		}
		MakeEmpty();
		_data.ptr = ptr;
	} else {
		MakeEmpty();
	}
	_len = len;
	Ptr()[len] = 0;
	return true;
}

bool EcoString::Truncate(int32_t len)
{
	if (len >= _len) {
		return true;
	}
	EcoString new_str;
	if (!new_str.Assign(Ptr(), len)) {
		return false;
	}
	Swap(new_str);
	return true;
}

bool EcoString::Expand(int32_t len, wchar_t ch)
{
	if (len <= _len) {
		return true;
	}
	EcoString new_str;
	if (!new_str.MakeLength(len)) {
		return false;
	}
	wmemcpy(new_str.Ptr(), CPtr(), _len);
	wmemset(new_str.Ptr() + _len, ch, len - _len);
	Swap(new_str);
	return true;
}

bool EcoString::Resize(int32_t len, wchar_t ch)
{
	if (len > _len) {
		return Expand(len, ch);
	}
	return Truncate(len);
}

bool EcoString::Replace(int32_t pos, int32_t rcnt, wchar_t ch, int32_t cnt)
{
	if (pos + rcnt > _len) {
		fprintf(stderr, "EcoString::InsertC: pos{%u} + rcnt{%u} > _len{%u}\n", pos, rcnt, _len);
		return false;
	}
	EcoString new_str;
	if (!new_str.MakeLength(_len - rcnt + cnt)) {
		return false;
	}
	wmemcpy(new_str.Ptr(), CPtr(), pos);
	wmemset(new_str.Ptr() + pos, ch, cnt);
	wmemcpy(new_str.Ptr() + pos + cnt, CPtr() + pos + rcnt, _len - pos - rcnt);
	Swap(new_str);
	return true;
}

bool EcoString::Replace(int32_t pos, int32_t rcnt, const wchar_t *data, int32_t cnt)
{
	if (cnt < 0) {
		cnt = wcslen(data);
	}
	if (pos + rcnt > _len) {
		fprintf(stderr, "EcoString::Replace: pos{%u} + rcnt{%u} > _len{%u}\n", pos, rcnt, _len);
		return false;
	}
	EcoString new_str;
	if (!new_str.MakeLength(_len - rcnt + cnt)) {
		return false;
	}
	wmemcpy(new_str.Ptr(), CPtr(), pos);
	wmemcpy(new_str.Ptr() + pos, data, cnt);
	wmemcpy(new_str.Ptr() + pos + cnt, CPtr() + pos + rcnt, _len - pos - rcnt);
	Swap(new_str);
	return true;
}


bool EcoString::Remove(int32_t ofs, int32_t len)
{
	if (len == 0) {
		return true;
	}
	if (ofs >= _len) {
		fprintf(stderr, "EcoString::Remove: ofs{%u} >= _len{%u}\n", ofs, _len);
		return false;
	}
	if (ofs + len > _len) {
		fprintf(stderr, "EcoString::Remove: ofs{%u} + len{%u} > _len{%u}\n", ofs, len, _len);
		len = _len - ofs;
	}
	EcoString new_str;
	if (!new_str.MakeLength(_len - len)) {
		return false;
	}
	wmemcpy(new_str.Ptr(), CPtr(), ofs);
	wmemcpy(new_str.Ptr() + ofs, CPtr() + ofs + len, _len - (ofs + len));
	Swap(new_str);
	return true;
}


bool EcoString::Assign(const wchar_t *data, int32_t len)
{
	if (!MakeLength(len)) {
		return false;
	}
	wmemcpy(Ptr(), data, len);
	return true;
}

DWORD EcoString::Transcode(UINT oldCodepage, UINT codepage)
{
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

		if (!MakeLength(length2)) {
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
