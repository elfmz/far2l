#pragma once
#include <exception>
#include <stdexcept>

template <class T>
	class TailedStruct
{
	T *_t = nullptr;

	TailedStruct(const TailedStruct&) = delete;

public:
	TailedStruct(size_t tail_size = 0, bool zerofill = true)
	{
		_t =  (T*)malloc(sizeof(T) + tail_size);
		if (_t == nullptr)
			throw std::runtime_error("TailedStruct - alloc failed");

		if (zerofill)
			memset(_t, 0, sizeof(T) + tail_size);
	}

	~TailedStruct()
	{
		free(_t);
	}

	inline T *ptr()
	{
		return _t;
	}
	inline T *operator ->()
	{
		return _t;
	}
};
