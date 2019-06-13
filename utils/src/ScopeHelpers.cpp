#include <unistd.h>
#include "ScopeHelpers.h"
#include "utils.h"

void FDScope::CheckedClose()
{
	CheckedCloseFD(_fd);
}

FDScope::~FDScope()
{
	CheckedClose();
}

UnlinkScope::~UnlinkScope()
{
	if (!_path.empty())
		unlink(_path.c_str());
}
