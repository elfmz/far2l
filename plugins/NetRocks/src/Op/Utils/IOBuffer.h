#pragma once
#include <exception>
#include <stdexcept>

class IOBuffer
{
	void *_data = nullptr;
	size_t _size = 0, _capacity = 0;

	unsigned int _low_mem_countdown = 0;

	size_t _min_size, _max_size;

	IOBuffer(const IOBuffer &) = delete;
	IOBuffer& operator=(const IOBuffer &) = delete;

	bool IncreaseTo(size_t new_size, bool preserve_data = true); //  tries to increase size of buffer (will not go above max_size)

public:
	IOBuffer(size_t initial_size, size_t min_size, size_t max_size);
	~IOBuffer();

	inline void *Data() { return _data; }
	inline size_t Size() { return _size; }

	bool Increase(bool preserve_data = true); //  tries to increase size of buffer (will not go above max_size)
	bool Decrease(); // tries to decrease size of buffer (will not go below min_size)

	void Desire(size_t size, bool preserve_data = true); // tries to change size of of buffer (within [min_size..max_size] )
};
