#include <sudo.h>
#include <stdio.h>
#include <errno.h>

#ifdef __linux__
# include <linux/fs.h>
#endif

#include "FSFileFlags.h"

#ifdef __DragonFly__
/* This is the same as the MacOS X definition of UF_HIDDEN. */
#define UF_HIDDEN      0x00008000      /* file is hidden */
#endif

FSFileFlags::FSFileFlags(const std::string &path)
{
	if (sdc_fs_flags_get(path.c_str(), &_flags) == 0) {
		_valid = true;
		_actual_flags = _flags;

	} else {
		fprintf(stderr, "FSFileFlags: error %d; path='%s'\n", errno, path.c_str());
	}
}

void FSFileFlags::Apply(const std::string &path, bool force)
{
	if (!_valid) {
		fprintf(stderr, "FSFileFlags::Apply: not valid; path='%s'\n", path.c_str());

	} else if (_flags != _actual_flags || force) {
		if (sdc_fs_flags_set(path.c_str(), _flags) == 0) {
			_actual_flags = _flags;
			fprintf(stderr, "FSFileFlags::Apply: OK; path='%s'\n", path.c_str());

		} else {
			fprintf(stderr, "FSFileFlags::Apply: error %d; path='%s'\n", errno, path.c_str());
		}
	}
}

////////

bool FSFileFlags::Immutable() const
{
	return FS_FLAGS_CONTAIN_IMMUTABLE(_flags);
}

void FSFileFlags::SetImmutable(bool v)
{
	if (v && !FS_FLAGS_CONTAIN_IMMUTABLE(_flags)) {
		_flags = FS_FLAGS_WITH_IMMUTABLE(_flags);

	} else if (!v && FS_FLAGS_CONTAIN_IMMUTABLE(_flags)) {
		_flags = FS_FLAGS_WITHOUT_IMMUTABLE(_flags);
	}
}

bool FSFileFlags::Append() const
{
	return FS_FLAGS_CONTAIN_APPEND(_flags);
}

void FSFileFlags::SetAppend(bool v)
{
	if (v && !FS_FLAGS_CONTAIN_APPEND(_flags)) {
		_flags = FS_FLAGS_WITH_APPEND(_flags);

	} else if (!v && FS_FLAGS_CONTAIN_APPEND(_flags)) {
		_flags = FS_FLAGS_WITHOUT_APPEND(_flags);
	}
}

#if defined(__APPLE__) || defined(__FreeBSD__)  || defined(__DragonFly__)

bool FSFileFlags::Hidden() const
{
	return FS_FLAGS_CONTAIN_HIDDEN(_flags);
}

void FSFileFlags::SetHidden(bool v)
{
	if (v && !FS_FLAGS_CONTAIN_HIDDEN(_flags)) {
		_flags = FS_FLAGS_WITH_HIDDEN(_flags);

	} else if (!v && FS_FLAGS_CONTAIN_HIDDEN(_flags)) {
		_flags = FS_FLAGS_WITHOUT_HIDDEN(_flags);
	}
}

#endif
