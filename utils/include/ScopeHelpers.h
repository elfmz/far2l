#pragma once
#include <string>
#include "utils.h"

class FDScope
{
	int _fd;
public:
	FDScope(const FDScope &) = delete;

	FDScope(int fd = -1) : _fd(fd) {}
	~FDScope();

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
