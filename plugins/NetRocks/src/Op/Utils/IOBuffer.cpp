#include <algorithm>
#include <string.h>
#include <stdlib.h>
#include "IOBuffer.h"

#define ALIGNMENT              ((size_t)0x1000)
#define ALIGN_SIZE(sz)         ((sz) & (~(ALIGNMENT - 1)))
#define LOW_MEM_COUNTDOWN      (0x1000)


IOBuffer::IOBuffer(size_t initial_size, size_t min_size, size_t max_size)
	: _min_size(ALIGN_SIZE(min_size)), _max_size(ALIGN_SIZE(max_size))
{
	if (!_min_size)
		_min_size = ALIGNMENT;

	if (_max_size < _min_size)
		_max_size = _min_size;

	_size = ALIGN_SIZE(initial_size);
	if (_size < _min_size)
		_size = _min_size;

	if (_size > _max_size)
		_size = _max_size;

	if (posix_memalign(&_data, ALIGNMENT, _size) != 0) {
		_data = malloc(_size);
		if (_data == nullptr) {
			_data = malloc(_min_size);
			if (_data == nullptr) {
				throw std::runtime_error("IOBuffer - no memory");
			}
			_size = _min_size;
		}
	}
	_capacity = initial_size;
}

IOBuffer::~IOBuffer()
{
	if (_data) {
		free(_data);
	}
}


bool IOBuffer::IncreaseTo(size_t new_size, bool preserve_data) //  tries to increase size of buffer (will not go above max_size)
{
	if (new_size <= _capacity) {
		_size = new_size;
		return true;
	}

	if (_low_mem_countdown) {
		--_low_mem_countdown;
		return false;
	}

	void *new_data;
	if (posix_memalign(&new_data, ALIGNMENT, new_size) != 0) {
		_low_mem_countdown = LOW_MEM_COUNTDOWN;
		return false;
	}
	if (_data) {
		if (preserve_data) {
			memcpy(new_data, _data, _size);
		}
		free(_data);
	}
	_data = new_data;
	_size = _capacity = new_size;
	return true;
}

bool IOBuffer::Increase(bool preserve_data)
{
	if (_size >= _max_size)
		return false;

	const size_t new_size = std::min(ALIGN_SIZE(_size * 3 / 2), _max_size);

	IncreaseTo(new_size, preserve_data);
	return true;
}

bool IOBuffer::Decrease()
{
	if (_size <= _min_size) {
		return false;
	}

	_size = std::max(_size / 2, _min_size);
	return true;
}

void IOBuffer::Desire(size_t size, bool preserve_data)
{
	if (size < _size) {
		_size = std::max(size, _min_size);

	} else if (size > _size) {
		size = std::min(size, _max_size);
		if (size > _size) {
			IncreaseTo(size, preserve_data);
		}
	}
}
