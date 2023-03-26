#pragma once

/**
	This class can be used to pack sequence of data chunks into preallocated memory
	or (if NULL ptr specified) to estimate size of memory needed for such packing.
*/

class ChunkedData
{
	void *_ptr;
	size_t _len;
	void *_recent;

public:
	inline ChunkedData(void *ptr = nullptr) : _ptr(ptr), _len(0), _recent(nullptr) {}

	/// Reserves given amount on buffer without any data being copied
	inline void Inflate(size_t len)
	{
		_recent = _ptr;
		if (_ptr) {
			_ptr = (unsigned char *)_ptr + len;
		}
		_len+= len;
	}

	/// If data len not aligned to given value - then inflates
	/// by remained amount to satisfy required pointer alignment
	inline void Align(size_t al)
	{
		const size_t r = _len % al;
		if (r) {
			Inflate(al - r);
		}
	}

	/// Inflates by given len and copy given data into reserved space
	/// ptr can be nullptr, in such case it doesn't reserve space
	void Append(const void *ptr, size_t len);

	/// Appends given NUL-terminated wide string (including NUL character)
	/// ptr can be nullptr, in such case it doesn't reserve any space
	void Append(const wchar_t *ptr);

	/// Returns current data length. Use it to estimate size of memory
	/// area required to store given sequence of data chunks
	inline size_t Length() const
	{
		return _len;
	}

	/// Returns pointer where recently Append()'ed data stored or
	/// pointer before Inflate() was invoked.
	/// It also returns NULL if:
	///   Recent Append() was invoked with source ptr set to NULL
	///   ChunkedData was constructed with NULL memory area
	inline void *Recent() const
	{
		return _recent;
	}
};
