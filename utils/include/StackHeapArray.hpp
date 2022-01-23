#pragma once
#include <stdint.h>

/**
 * Minimalistic array that keeps data either on stack either in heap
 * depending if elements count exceeds given threshold
 */
template <class WHAT, size_t THRESHOLD = 16 + (0x80 / sizeof(WHAT))>
	class StackHeapArray
{
	typedef WHAT StackPlacementPrototype[THRESHOLD];

	WHAT *_arr;
	size_t _cnt;
	char _stk_placement[sizeof(StackPlacementPrototype)] __attribute__((aligned(0x10)));

	StackHeapArray() = delete;
	StackHeapArray(const StackHeapArray &) = delete;

public:
	StackHeapArray(size_t cnt) : _cnt(cnt)
	{
		if (cnt > THRESHOLD) {
			_arr = new WHAT[cnt];

		} else {
			_arr = (WHAT *)_stk_placement;
			for (size_t i = 0; i < cnt; ++i) {
				new (&_arr[i])(WHAT)();
			}
		}
	}

	~StackHeapArray()
	{
		if (_cnt > THRESHOLD) {
			delete[] _arr;

		} else for (size_t i = _cnt; i;) {
			--i;
			_arr[i].~WHAT();
		}
	}

	inline WHAT *Get()
	{
		return _arr;
	}

	inline size_t Count() const
	{
		return _cnt;
	}

	WHAT & operator [](size_t i)
	{
		return _arr[i];
	}

	const WHAT & operator [](size_t i) const
	{
		return _arr[i];
	}
};
