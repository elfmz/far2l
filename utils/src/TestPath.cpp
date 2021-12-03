#include <sys/types.h>
#include <sys/stat.h>
#include "TestPath.h"

void TestPath::Init(const char *path)
{
	struct stat s;
	if (stat(path, &s) == 0) {
		_exists = true;
		_directory = S_ISDIR(s.st_mode);
		_regular = S_ISREG(s.st_mode);
		_executable = S_ISREG(s.st_mode) && (s.st_mode & (S_IXUSR | S_IXGRP | S_IXOTH)) != 0;

	} else {
		_exists = _directory = _regular = _executable = false;
	}
}
