#pragma once
#include <sys/stat.h>
#include <unistd.h>
#include <string>

class FSFileFlags
{
	unsigned long _flags = 0, _actual_flags = 0;
	bool _valid = false;
	int _errno = 0;

public:
	FSFileFlags(const std::string &path);
	void Apply(const std::string &path, bool force = false);
	inline void Reset() { _flags = _actual_flags; }

	inline bool Valid() const { return _valid; }

	inline int Errno() const { return _errno; }

	inline unsigned long GetFlags() const { return _flags; }
	inline unsigned long GetActualFlags() const { return _actual_flags; }

	inline bool FlagIsOn(unsigned long flag) const { return _flags & flag; }
	inline bool FlagEqActual(unsigned long flag) const { return (_flags & flag) == (_actual_flags & flag); }
	inline void FlagInverse(unsigned long flag) { _flags = _flags ^ flag; }

	bool Immutable() const;
	void SetImmutable(bool v);

	bool Append() const;
	void SetAppend(bool v);

#if defined(__APPLE__) || defined(__FreeBSD__) || defined(__DragonFly__)
	bool Hidden() const;
	void SetHidden(bool v);
#endif
};

#if defined(__APPLE__) || defined(__FreeBSD__) || defined(__DragonFly__)
# define FS_FLAGS_CONTAIN_IMMUTABLE(flags) (((flags) & (UF_IMMUTABLE | SF_IMMUTABLE)) != 0)
# define FS_FLAGS_WITHOUT_IMMUTABLE(flags) ((flags) & (~(UF_IMMUTABLE | SF_IMMUTABLE)))
# define FS_FLAGS_WITH_IMMUTABLE(flags) ((flags) | (UF_IMMUTABLE))

# define FS_FLAGS_CONTAIN_APPEND(flags) (((flags) & (UF_APPEND | SF_APPEND)) != 0)
# define FS_FLAGS_WITHOUT_APPEND(flags) ((flags) & (~(UF_APPEND | SF_APPEND)))
# define FS_FLAGS_WITH_APPEND(flags) ((flags) | (UF_APPEND))

# define FS_FLAGS_CONTAIN_HIDDEN(flags) (((flags) & (UF_HIDDEN)) != 0)
# define FS_FLAGS_WITHOUT_HIDDEN(flags) ((flags) & (~(UF_HIDDEN)))
# define FS_FLAGS_WITH_HIDDEN(flags) ((flags) | (UF_HIDDEN))

#elif defined(__HAIKU__)
// TODO ?
# define FS_FLAGS_CONTAIN_IMMUTABLE(flags) false
# define FS_FLAGS_WITHOUT_IMMUTABLE(flags) false
# define FS_FLAGS_WITH_IMMUTABLE(flags) false

# define FS_FLAGS_CONTAIN_APPEND(flags) false
# define FS_FLAGS_WITHOUT_APPEND(flags) false
# define FS_FLAGS_WITH_APPEND(flags) false
#else
# define FS_FLAGS_CONTAIN_IMMUTABLE(flags) (((flags) & FS_IMMUTABLE_FL) != 0)
# define FS_FLAGS_WITHOUT_IMMUTABLE(flags) ((flags) & (~FS_IMMUTABLE_FL))
# define FS_FLAGS_WITH_IMMUTABLE(flags) ((flags) | FS_IMMUTABLE_FL)

# define FS_FLAGS_CONTAIN_APPEND(flags) (((flags) & FS_APPEND_FL) != 0)
# define FS_FLAGS_WITHOUT_APPEND(flags) ((flags) & (~FS_APPEND_FL))
# define FS_FLAGS_WITH_APPEND(flags) ((flags) | (FS_APPEND_FL))

#endif
