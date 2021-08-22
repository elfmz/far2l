#pragma once
#include <iterator>
#include <stdexcept>
#include <vector>
#include <assert.h>
#include <string.h>
#include "ww898/utf_converters.hpp"
#include "UtfDefines.h"


typedef ww898::utf::utf8 Utf8;
typedef ww898::utf::utf16 Utf16;
typedef ww898::utf::utf32 Utf32;
typedef ww898::utf::utfw UtfW;

struct ArrayPushBackOverflow : std::runtime_error
{
	ArrayPushBackOverflow() : std::runtime_error("array overflow") {}
};

template <class T>
	class ArrayPushBack
{
	T *_cur = nullptr, *_begin = nullptr, *_end = nullptr;

public:
	typedef T value_type;

	inline ArrayPushBack(T *beg, T *end) : _cur(beg), _begin(beg), _end(end) { }

	inline void push_back(const T &v)
	{
		if (_cur == _end) {
			throw ArrayPushBackOverflow();
		}
		*_cur = v;
		++_cur;
	}
	inline size_t size() const
	{
		return _cur - _begin;
	}
	inline bool fully_filled() const
	{
		return _cur == _end;
	}
};

template <class T>
	class DummyPushBack
{
	size_t _cur = 0;
	T _v{};

public:
	typedef T value_type;

	inline void push_back(const T &)
	{
		++_cur;
	}
	inline size_t size() const
	{
		return _cur;
	}
	inline bool fully_filled() const
	{
		return false;
	}
};

template <class CONTAINER_T>
	class StdPushBack
{
	CONTAINER_T &_container;

public:
	typedef typename CONTAINER_T::value_type value_type;

	inline StdPushBack(CONTAINER_T &container)
		: _container(container) {}

	inline void push_back(const value_type &v)
	{
		_container.push_back(v);
	}

	inline bool fully_filled() const
	{
		return false;
	}
};


template <typename CONV_SRC, typename CONV_DST, typename CHAR_SRC, typename PUSHBACK_DST>
	static inline unsigned UtfTransform(const CHAR_SRC *src_begin, size_t &src_len, PUSHBACK_DST &dst_pb, bool fail_on_illformed)
{
	if (src_len == 0) {
		return 0;
	}

	const CHAR_SRC *src = src_begin;
	const CHAR_SRC *src_end = src_begin + src_len;
	unsigned out = 0;
	for (;;) {
		try {
			auto bi = std::back_inserter(dst_pb);
			if (ww898::utf::conv<CONV_SRC, CONV_DST>(src, src_end, bi)) {
				break;
			}
		} catch (std::exception &e) {
			fprintf(stderr, "!!!: %s\n", e.what());
		}

		if (dst_pb.fully_filled()) {
			out|= CONV_NEED_MORE_DST;
			break;
		}
		if (fail_on_illformed || src_end - src < MAX_MB_CHARS_PER_WCHAR) {
			src_len = src - src_begin;
			out|= ((src_end - src < MAX_MB_CHARS_PER_WCHAR) ? CONV_NEED_MORE_SRC : CONV_ILLFORMED_CHARS);
			break;
		}
		try {
			dst_pb.push_back(CHAR_REPLACEMENT);

		} catch (std::runtime_error &) {
			assert(dst_pb.fully_filled());
			out|= CONV_NEED_MORE_DST;
			break;
		}
		++src;
		out|= CONV_ILLFORMED_CHARS;
	}

	return out;
}


template <typename CONV_SRC, typename CONV_DST, typename CHAR_SRC, typename CHAR_DST>
	static inline size_t UtfCalcSpace(const CHAR_SRC *src_begin, size_t src_len, bool fail_on_illformed)
{
	DummyPushBack<CHAR_DST> dpb;
	UtfTransform<CONV_SRC, CONV_DST>(src_begin, src_len, dpb, fail_on_illformed);
	return dpb.size();
}

template <typename CONV_SRC, typename CONV_DST, typename CHAR_SRC, typename CHAR_DST>
	struct UtfTransformer : std::vector<CHAR_DST>
{
	UtfTransformer(const CHAR_SRC *src, size_t src_len)
	{
		StdPushBack<std::vector<CHAR_DST>> pb(*this);
		UtfTransform<CONV_SRC, CONV_DST>(&src, src_len, pb, false);
	}

	void *CopyMalloc(uint32_t &len) const
	{
		const size_t sz = std::vector<CHAR_DST>::size();
		void *out = malloc((sz + 1) * sizeof(CHAR_DST));
		if (out) {
			memcpy(out, std::vector<CHAR_DST>::data(), sz * sizeof(CHAR_DST));
			memset((char *)out + sz * sizeof(CHAR_DST), 0, sizeof(CHAR_DST));
			len = sz * sizeof(CHAR_DST);
		}
		return out;
	}
};

