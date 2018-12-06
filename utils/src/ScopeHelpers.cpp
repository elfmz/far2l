#include <unistd.h>
#include "ScopeHelpers.h"
#include "utils.h"

FDScope::~FDScope()
{
	CheckedCloseFD(_fd);
}

UnlinkScope::~UnlinkScope()
{
	if (!_path.empty())
		unlink(_path.c_str());
}
