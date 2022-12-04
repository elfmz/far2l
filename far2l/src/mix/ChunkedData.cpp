#include <stdlib.h>
#include <string.h>
#include <wchar.h>
#include "ChunkedData.hpp"

void ChunkedData::Append(const void *ptr, size_t len)
{
	if (!ptr) {
		_recent = nullptr;

	} else {
		if (_ptr) {
			memcpy(_ptr, ptr, len);
		}
		Inflate(len);
	}
}

void ChunkedData::Append(const wchar_t *ptr)
{
	if (!ptr) {
		_recent = nullptr;

	} else {
		Append(ptr, (wcslen(ptr) + 1) * sizeof(*ptr));
	}
}
