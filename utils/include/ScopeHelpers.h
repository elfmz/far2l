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
	UnlinkScope();
	UnlinkScope(const std::string &path);
	UnlinkScope &operator = (const std::string &path);

	inline operator const std::string &() const { return _path; }
	inline const char *c_str() const { return _path.c_str(); }

	~UnlinkScope();
};
