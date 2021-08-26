#pragma once

#if !defined(__APPLE__) && !defined(__FreeBSD__)
# include <alloca.h>
#endif

# include <stdlib.h>

template <class WHAT, size_t THRESHOLD>
	struct HeapStackArrayDestroyer
{
	HeapStackArrayDestroyer(WHAT *arr, size_t cnt)
	:
		_arr(arr),
		_cnt(cnt)
	{
	}

	~HeapStackArrayDestroyer()
	{
		if (_cnt > THRESHOLD) {
			delete[] _arr;

		} else for (size_t i = 0; i < _cnt; ++i) {
			_arr[i].~WHAT();
		}
	}


private:
	WHAT *_arr = nullptr;
	size_t _cnt;
};

#define HEAP_STACK_ARRAY(WHAT, NAME, COUNT, THRESHOLD) \
	WHAT *NAME; \
	if ((COUNT) > (THRESHOLD)) { \
		NAME = new WHAT[(COUNT)](); \
	} else { \
		NAME = (WHAT *)alloca((COUNT) * sizeof(WHAT)); \
		for (auto i = (COUNT); i > 0; ) new (&NAME[--i])(WHAT)(); \
	} \
	HeapStackArrayDestroyer<WHAT, THRESHOLD> NAME##__destroyer__(NAME, COUNT); \


