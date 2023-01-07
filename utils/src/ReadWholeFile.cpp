#include <fcntl.h>
#include "utils.h"
#include "ScopeHelpers.h"

bool ReadWholeFile(const char *path, std::string &result, size_t limit)
{
	FDScope fd(path, O_RDONLY);
	if (!fd.Valid())
		return false;

	for (;;) {
		char buf[0x1000] = {};
		ssize_t r = ReadAll(fd, buf, sizeof(buf));
		if (r > 0) {
			result.append(buf, r);
			if (result.size() >= limit) {
				return true;
			}

		} else if (r < 0 && result.empty()) {
			return false;

		} else {
			return true;
		}
	}
}
