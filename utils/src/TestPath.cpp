#include <sys/types.h>
#include <sys/stat.h>
#include "TestPath.h"

TestPath::TestPath(const char *path, Mode m)
	: _out(false)
{
	struct stat s;
	if (stat(path, &s) == 0) {

		switch (m) {
			case EXISTS:
				_out = true;
				break;

			case DIRECTORY:
				_out = S_ISDIR(s.st_mode);
				break;

			case REGULAR:
				_out = S_ISREG(s.st_mode);
				break;

			case EXECUTABLE:
				_out = S_ISREG(s.st_mode) && (s.st_mode & (S_IXUSR | S_IXGRP | S_IXOTH)) != 0;
				break;
		}
	}
}
