#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include "utils.h"

bool EnsureDir(const char *path, int mode)
{
	for (;; usleep(1000)) {
		struct stat s{};
		if (stat(path, &s) == 0) {
			if (!S_ISDIR(s.st_mode)) {
				fprintf(stderr, "%s('%s'): not dir [%u]", __FUNCTION__, path, s.st_mode);
				return false;
			}
			return true;
		}

		int r = mkdir(path, mode);
		if (r != 0) {
			char *xpath = strdup(path);
			if (xpath) {
				for (size_t i = 0; ;++i) {
					const char c = xpath[i];
					if (i > 0 && (!c || c == '/')) {
						xpath[i] = 0;
						r = mkdir(xpath, mode);
						xpath[i] = c;
					}
					if (!c) {
						break;
					}
				}
				free(xpath);
			}
		}

		if (r != 0) {
			const int err = errno;
			if (err != EEXIST && err != EINTR && err != EAGAIN) {
				fprintf(stderr, "%s('%s'): error %u", __FUNCTION__, path, err);
				return false;
			}
		}
	}
}

