#include <fcntl.h>
#include "utils.h"
#include "ScopeHelpers.h"

bool WriteWholeFile(const char *path, const void *content, size_t length, unsigned int mode)
{
	FDScope fd(path, O_RDWR | O_CREAT | O_TRUNC, mode);
	if (!fd.Valid())
		return false;

	return WriteAll(fd, content, length, 0x100000) == length;
}

bool WriteWholeFile(const char *path, const std::string &content, unsigned int mode)
{
	return WriteWholeFile(path, content.data(), content.size(), mode);
}
