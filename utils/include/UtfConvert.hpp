#pragma once
#include <iterator>
#include <stdexcept>
#include <vector>
#include <assert.h>
#include <string.h>
#include "cctweaks.h"
#include "ww898/utf_converters.hpp"
#include "UtfDefines.h"

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
		if (UNLIKELY(_cur == _end)) {
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


template <typename CHAR_SRC, typename PUSHBACK_DST>
	static inline unsigned UtfConvert(const CHAR_SRC *src_begin, size_t &src_len, PUSHBACK_DST &dst_pb, bool fail_on_illformed)
{
	if (src_len == 0) {
		return 0;
	}

	const CHAR_SRC *src = src_begin;
	const CHAR_SRC *src_end = src_begin + src_len;
	unsigned out = 0;
	for (;;) {
		try {
			if (ww898::utf::conv<
					ww898::utf::utf_selector_t<CHAR_SRC>,
					ww898::utf::utf_selector_t<typename PUSHBACK_DST::value_type>
						>(src, src_end, dst_pb)) {
				break;
			}
		} catch (std::exception &e) {
			fprintf(stderr, "UtfConvert<%u, %u>: %s\n",
				(unsigned)sizeof(CHAR_SRC), (unsigned)sizeof(typename PUSHBACK_DST::value_type), e.what());
		}

		if (dst_pb.fully_filled()) {
			src_len = src - src_begin;
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
			src_len = src - src_begin;
			out|= CONV_NEED_MORE_DST;
			break;
		}
		++src;
		out|= CONV_ILLFORMED_CHARS;
	}

	return out;
}

template <typename CHAR_SRC, typename DST>
	static inline unsigned UtfConvertStd(const CHAR_SRC *src_begin, size_t &src_len, DST &dst, bool fail_on_illformed)
{
	StdPushBack<DST> dst_pb(dst);
	return UtfConvert(src_begin, src_len, dst_pb, fail_on_illformed);
}


template <typename CHAR_SRC, typename CHAR_DST>
	static inline size_t UtfCalcSpace(const CHAR_SRC *src_begin, size_t src_len, bool fail_on_illformed)
{
	DummyPushBack<CHAR_DST> dpb;
	UtfConvert(src_begin, src_len, dpb, fail_on_illformed);
	return dpb.size();
}

template <typename CHAR_SRC, typename CHAR_DST, bool ENSURE_NULL_TERMINATOR = true>
	struct UtfConverter : std::vector<CHAR_DST>
{
	UtfConverter(const CHAR_SRC *src, size_t src_len)
	{
		StdPushBack<std::vector<CHAR_DST>> pb(*this);
		UtfConvert(src, src_len, pb, false);

		if (ENSURE_NULL_TERMINATOR) {
			if (std::vector<CHAR_DST>::empty() || std::vector<CHAR_DST>::back() != 0) {
				std::vector<CHAR_DST>::emplace_back(0);
			}
		}
	}

	template <class OUT_ELEMENT_T>
		void CopyToVector(std::vector<OUT_ELEMENT_T> &out) const
	{
		static_assert(sizeof(CHAR_DST) % sizeof(OUT_ELEMENT_T) == 0, "Misaligned character types");
		const size_t sz = std::vector<CHAR_DST>::size() * sizeof(CHAR_DST);
		out.resize(sz / sizeof(OUT_ELEMENT_T));
		memcpy(out.data(), std::vector<CHAR_DST>::data(), sz);
	}
};

