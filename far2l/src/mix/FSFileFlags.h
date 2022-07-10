#pragma once
#include <sys/stat.h>
#include <unistd.h>
#include <string>

class FSFileFlags
{
	unsigned long _flags = 0, _actual_flags = 0;
	bool _valid = false;

public:
	FSFileFlags(const std::string &path);
	void Apply(const std::string &path, bool force = false);

	inline bool Valid() const { return _valid; }


	bool Immutable() const;
	void SetImmutable(bool v);

#if defined(__APPLE__) || defined(__FreeBSD__)
	bool Undeletable() const;
	void SetUndeletable(bool v);

	bool Hidden() const;
	void SetHidden(bool v);
#endif
};

#if defined(__APPLE__) || defined(__FreeBSD__)
# define FS_FLAGS_CONTAIN_IMMUTABLE(flags) (((flags) & (UF_IMMUTABLE | SF_IMMUTABLE)) != 0)
# define FS_FLAGS_WITHOUT_IMMUTABLE(flags) ((flags) & (~(UF_IMMUTABLE | SF_IMMUTABLE)))
# define FS_FLAGS_WITH_IMMUTABLE(flags) ((flags) | (UF_IMMUTABLE))

# define FS_FLAGS_CONTAIN_UNDELETABLE(flags) (((flags) & (UF_NOUNLINK | SF_NOUNLINK)) != 0)
# define FS_FLAGS_WITHOUT_UNDELETABLE(flags) ((flags) & (~(UF_NOUNLINK | SF_NOUNLINK)))
# define FS_FLAGS_WITH_UNDELETABLE(flags) ((flags) | (UF_NOUNLINK))

# define FS_FLAGS_CONTAIN_HIDDEN(flags) (((flags) & (UF_HIDDEN | SF_HIDDEN)) != 0)
# define FS_FLAGS_WITHOUT_HIDDEN(flags) ((flags) & (~(UF_HIDDEN | SF_HIDDEN)))
# define FS_FLAGS_WITH_HIDDEN(flags) ((flags) | (UF_HIDDEN))

#else
# define FS_FLAGS_CONTAIN_IMMUTABLE(flags) (((flags) & FS_IMMUTABLE_FL) != 0)
# define FS_FLAGS_WITHOUT_IMMUTABLE(flags) ((flags) & (~FS_IMMUTABLE_FL))
# define FS_FLAGS_WITH_IMMUTABLE(flags) ((flags) | FS_IMMUTABLE_FL)

#endif
