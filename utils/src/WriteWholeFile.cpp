#include <fcntl.h>
#include "utils.h"
#include "ScopeHelpers.h"

bool WriteWholeFile(const char *path, const std::string &content, unsigned int mode)
{
	FDScope fd(path, O_RDWR | O_CREAT | O_TRUNC, mode);
	if (!fd.Valid())
		return false;

	return WriteAll(fd, content.c_str(), content.size(), 0x100000) == content.size();
}
