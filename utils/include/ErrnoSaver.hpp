#pragma once
#include <errno.h>

class ErrnoSaver
{
	int _errno;

public:
	inline ErrnoSaver(int err) : _errno(err) {}
	inline ErrnoSaver() : _errno(errno) {}
	inline ~ErrnoSaver() { errno = _errno; }

	inline void Set(int err) { _errno = err; }

	inline void Set() { _errno = errno; }

	inline int Get() const { return _errno; };

	inline bool IsAccessDenied() const
	{
		return _errno == EPERM
			|| _errno == EACCES
#ifdef ENOTCAPABLE
			|| _errno == ENOTCAPABLE
#endif
			;
	}
};
