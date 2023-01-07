#pragma once
#include <string>
#include "utils.h"

class FDScope
{
	int _fd;
public:
	FDScope(const FDScope &) = delete;
	inline FDScope &operator = (const FDScope &) = delete;

	FDScope(const char *path, int opts, int mode = 0700);
	FDScope(int fd = -1) : _fd(fd) {}
	~FDScope();

	inline int Detach()
	{
		int out = _fd;
		_fd = -1;
		return out;
	}

	inline FDScope &operator = (int fd)
	{
		CheckedClose();
		_fd = fd;
		return *this;
	}

	void CheckedClose();

	bool Valid() const
	{
		return _fd != -1;
	}

	operator int() const
	{
		return _fd;
	}
};


class UnlinkScope
{
	std::string _path;
public:
	UnlinkScope(const std::string &path) : _path(path) {}
	~UnlinkScope();
};
