#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "EnsureDir.h"
#include "os_call.hpp"
#include "ScopeHelpers.h"
#include "utils.h"

static int NonZeroErrno()
{
	const int err = errno;
	return err ? err : -1;
}

static int MakeDirs(const char *dir, mode_t mode)
{
	if (LIKELY(os_call_int(mkdir, dir, mode) == 0)) {
		return 0;
	}

	if (LIKELY(NonZeroErrno() == EEXIST)) {
		return EEXIST;
	}

	char *xdir = strdup(dir);
	if (UNLIKELY(!xdir)) {
		return ENOMEM;
	}

	int err = 0;
	for (size_t i = 0; ;++i) {
		const auto c = xdir[i];
		if (i > 0 && (!c || c == '/')) {
			xdir[i] = 0;
			if (UNLIKELY(os_call_int(mkdir, (const char *)xdir, mode) == -1)) {
				err = NonZeroErrno();
			}
			xdir[i] = c;
		}
		if (!c) {
			break;
		}
	}

	free(xdir);

	return err;
}

bool EnsureDir(const char *dir, PrivacyLevel pl)
{
	struct stat s{};
	for (int i = 0; UNLIKELY(os_call_int(stat, dir, &s) == -1); ++i) {
		if (i > 10) {
			fprintf(stderr, "%s('%s', %u): stat errno=%u\n", __FUNCTION__, dir, pl, errno);
			return false;
		}
		const int err = MakeDirs(dir, (pl == PL_ALL) ? 0777 : 0700);
		if (LIKELY(err == 0)) {
			return true;
		}
		if (UNLIKELY(err != EEXIST)) {
			fprintf(stderr, "%s('%s', %u): make error=%u\n", __FUNCTION__, dir, pl, err);
			return false;
		}
		usleep(i * 1000);
	}

	if (UNLIKELY((s.st_mode & S_IFMT) != S_IFDIR)) {
		fprintf(stderr, "%s('%s', %u): not-dir mode=0%o\n", __FUNCTION__, dir, pl, s.st_mode);
		return false;
	}

	const auto uid = geteuid();
	if (UNLIKELY(pl >= PL_PRIVATE && uid != s.st_uid && s.st_uid != 0)) {
		fprintf(stderr, "%s('%s', %u): uid=%u but st_uid=%u\n", __FUNCTION__, dir, pl, uid, s.st_uid);
		return false;
	}

	if (uid == 0
		|| ((s.st_mode & S_IWOTH) != 0)
		|| ((s.st_mode & S_IWUSR) != 0 && s.st_uid == uid)
		|| ((s.st_mode & S_IWGRP) != 0 && s.st_gid == getegid()))
	{
		return true;
	}

	// may be mode bits lie us? check it with stick
	std::string check_path = dir;
	check_path+= "/.far2l-stick-check.tmp";
	{
		FDScope fd(check_path.c_str(), O_CREAT | O_RDWR | O_CLOEXEC, 0600);
		if (!fd.Valid()) {
			return false;
		}
	}

	fprintf(stderr, "%s('%s', %u): file allowed, mode=0%o\n", __FUNCTION__, dir, pl, s.st_mode);
	if (os_call_int(unlink, check_path.c_str()) == -1) {
		perror("unlink stick-check");
	}

	return true;
}

