#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <os_call.hpp>
#include "ScopeHelpers.h"
#include "utils.h"

static int CallOpen(const char *path, int opts, int mode)
{
	return open(path, opts, mode);
}

FDScope::FDScope(const char *path, int opts, int mode)
	: _fd(os_call_int(CallOpen, path, opts, mode))
{
}

void FDScope::CheckedClose()
{
	CheckedCloseFD(_fd);
}

FDScope::~FDScope()
{
	CheckedClose();
}

//////////

UnlinkScope::UnlinkScope()
{
}

UnlinkScope::UnlinkScope(const std::string &path)
	: _path(path)
{
}

UnlinkScope &UnlinkScope::operator = (const std::string &path)
{
	_path = path;
	return *this;
}

UnlinkScope::~UnlinkScope()
{
	if (!_path.empty()) {
		if (os_call_int(unlink, _path.c_str()) == -1) {
			fprintf(stderr, "UnlinkScope: err %d removing '%s'\n", errno, _path.c_str());
		}
	}
}
