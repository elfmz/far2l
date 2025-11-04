#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <vector>
#include "utils.h"
#include "UtfConvert.hpp"

//NB: Routines here should not affect errno

//////////////////////////////////////////////////////

static size_t MB2Wide_Internal(const char *src_begin, size_t &src_len, std::wstring &dst, bool append)
{
	if (!append) {
		dst.clear();
	}
	if (!src_len) {
		return std::string::npos;
	}

	auto src = src_begin;
	auto src_end = src_begin + src_len;
	auto src_incomplete_tail = src_end;
	size_t dst_incomplete_tail_pos = (size_t)-1;

	struct EscapedStringPushBack : public StdPushBack<std::wstring>
	{
		EscapedStringPushBack(std::wstring &dst) : StdPushBack<std::wstring>(dst) {}

		inline void push_back(const value_type &v)
		{
			StdPushBack<std::wstring>::push_back(v);
			if (UNLIKELY(v == WCHAR_ESCAPING)) {
				StdPushBack<std::wstring>::push_back(WCHAR_ESCAPING);
			}
		}

	} pb(dst);

	for (;;) {
		size_t src_piece = src_end - src;
		const unsigned ucr = UtfConvert(src, src_piece, pb, true);
		src+= src_piece;
		if (ucr == 0) {
			break;
		}

		if (dst_incomplete_tail_pos == (size_t)-1 && src != src_end
				&& src_end - src < MAX_MB_CHARS_PER_WCHAR) {
			dst_incomplete_tail_pos = dst.size();
			src_incomplete_tail = src;
		}

		const auto remain = src_end - src;
		if (remain != 0) {
			const unsigned char uc = (unsigned char)*src;
			dst.push_back(WCHAR_ESCAPING);
			dst.push_back((wchar_t)MakeHexDigit(uc >> 4));
			dst.push_back((wchar_t)MakeHexDigit(uc & 0xf));

//			fprintf(stderr, "CE: @%lu %x\n", src - src_begin, ucr);
		}
		if (remain <= 1) {
			if (dst_incomplete_tail_pos != (size_t)-1) {
				src_len = src_incomplete_tail - src_begin;
				return dst_incomplete_tail_pos;
			}
			src_len = src - src_begin;
			break;
		}
		++src;
	}

	return dst.size();
}

size_t MB2Wide_HonorIncomplete(const char *src, size_t src_len, std::wstring &dst, bool append)
{
	size_t dst_incomplete_tail_pos = MB2Wide_Internal(src, src_len, dst, append);
	if (dst_incomplete_tail_pos != std::string::npos) {
		dst.resize(dst_incomplete_tail_pos);
	}
	return src_len;
}

unsigned MB2Wide_Unescaped(const char *src_begin, size_t &src_len, wchar_t &dst, bool fail_on_illformed)
{
	ArrayPushBack<wchar_t> pb(&dst, (&dst) + 1);
	unsigned out = UtfConvert(src_begin, src_len, pb, fail_on_illformed);
	out&= ~CONV_NEED_MORE_DST;
	return out;
}

unsigned MB2Wide_Unescaped(const char *src, size_t &src_len, wchar_t *dst, size_t &dst_len, bool fail_on_illformed)
{
	ArrayPushBack<wchar_t> pb(dst, dst + dst_len);
	unsigned out = UtfConvert(src, src_len, pb, fail_on_illformed);
	dst_len = pb.size();
	return out;
}

unsigned int Wide2MB_Unescaped(const wchar_t *src, size_t &src_len, char *dst, size_t &dst_len, bool fail_on_illformed)
{
	ArrayPushBack<char> pb(dst, dst + dst_len);
	unsigned out = UtfConvert(src, src_len, pb, fail_on_illformed);
	dst_len = pb.size();
	return out;
}

void MB2Wide(const char *src, size_t src_len, std::wstring &dst, bool append)
{
	MB2Wide_Internal(src, src_len, dst, append);
}

//////////////////

void Wide2MB_UnescapedAppend(const wchar_t wc, std::string &dst)
{
	size_t len = 1;
	StdPushBack<std::string> pb(dst);
	UtfConvert(&wc, len, pb, false);
}

void Wide2MB_UnescapedAppend(const wchar_t *src_begin, size_t src_len, std::string &dst)
{
	StdPushBack<std::string> pb(dst);
	UtfConvert(src_begin, src_len, pb, false);
}

static inline bool IsLowCaseHexDigit(const wchar_t c)
{
	return (c >= L'0' && c <= L'9') || (c >= L'a' && c <= L'f');
}

void Wide2MB(const wchar_t *src_begin, size_t src_len, std::string &dst, bool append)
{
	if (!append) {
		dst.clear();
	}
	if (!src_len) {
		return;
	}

	auto src = src_begin, src_end = src_begin + src_len;

	while (src != src_end) {
		if (LIKELY(src[0] != WCHAR_ESCAPING)) {
			++src;

		} else {
			if (src_begin != src) {
				Wide2MB_UnescapedAppend(src_begin, src - src_begin, dst);
			}
			if (LIKELY(src_end - src >= 3 && IsLowCaseHexDigit(src[1]) && IsLowCaseHexDigit(src[2]))) {
				dst+= ParseHexByte(&src[1]);
				src+= 3;

			} else {
				Wide2MB_UnescapedAppend(src, 1, dst);
				++src;
				if (src_end != src && *src == WCHAR_ESCAPING) {
					++src;
				}
			}
			src_begin = src;
		}
	}

	if (src_begin != src) {
		Wide2MB_UnescapedAppend(src_begin, src - src_begin, dst);
	}
}

//////////////////

void Wide2MB(const wchar_t *src, std::string &dst, bool append)
{
	Wide2MB(src, wcslen(src), dst, append);
}

void MB2Wide(const char *src, std::wstring &dst, bool append)
{
	MB2Wide(src, strlen(src), dst, append);
}

std::string Wide2MB(const wchar_t *src)
{
	std::string dst;
	Wide2MB(src, dst, true);
	return dst;
}

std::wstring MB2Wide(const char *src)
{
	std::wstring dst;
	MB2Wide(src, dst, true);
	return dst;
}

void StrWide2MB(const std::wstring &src, std::string &dst, bool append)
{
	Wide2MB(src.c_str(), src.size(), dst, append);
}

std::string StrWide2MB(const std::wstring &src)
{
	std::string dst;
	Wide2MB(src.c_str(), src.size(), dst, true);
	return dst;
}

void StrMB2Wide(const std::string &src, std::wstring &dst, bool append)
{
	MB2Wide(src.c_str(), src.size(), dst, append);
}

std::wstring StrMB2Wide(const std::string &src)
{
	std::wstring dst;
	MB2Wide(src.c_str(), src.size(), dst, true);
	return dst;
}
