#pragma once
#include <sys/stat.h>
#include <stddef.h>

// Recovering a mode_t from the textual permission field of an `ls -l` line, e.g. "drwxr-sr-t".
//
// Plugins that browse a remote filesystem through a shell have no stat(2) available, so a
// human-formatted listing is often the only metadata source. far2l renders the permission
// column, the executable highlighting and the FILE_ATTRIBUTE_* flags out of dwUnixMode, so
// getting this field right is what makes those correct.
namespace UnixModeStr
{
	inline mode_t Char2FileType(char c)
	{
		switch (c) {
			case 'l': return S_IFLNK;
			case 'd': return S_IFDIR;
			case 'c': return S_IFCHR;
			case 'b': return S_IFBLK;
			case 'p': return S_IFIFO;
			case 's': return S_IFSOCK;
			case 'f':
			default: return S_IFREG;
		}
	}

	// One "rwx" group. Note the execute bit also has to be recognised in its set-id/sticky
	// spellings: lowercase s/t mean "that bit AND executable", uppercase S/T mean the bit
	// without executable.
	inline mode_t Triplet2Perm(const char *c)
	{
		mode_t out = 0;
		if (c[0] == 'r') out|= 4;
		if (c[1] == 'w') out|= 2;
		if (c[2] == 'x' || c[2] == 's' || c[2] == 't') out|= 1;
		return out;
	}

	// Full field -> mode. Shorter or partly unknown fields degrade gracefully: entries the
	// remote shell could not stat come back as "d?????????", which yields the type and no
	// permission bits at all.
	inline mode_t Parse(const char *str, size_t len)
	{
		mode_t mode = 0;
		if (len >= 1) {
			mode = Char2FileType(*str);
		}
		if (len >= 4) {
			mode|= Triplet2Perm(str + 1) << 6;
		}
		if (len >= 7) {
			mode|= Triplet2Perm(str + 4) << 3;
		}
		if (len >= 10) {
			mode|= Triplet2Perm(str + 7);
		}

		// The set-id and sticky bits are encoded in the execute positions.
		if (len >= 4 && (str[3] == 's' || str[3] == 'S')) mode|= S_ISUID;
		if (len >= 7 && (str[6] == 's' || str[6] == 'S')) mode|= S_ISGID;
		if (len >= 10 && (str[9] == 't' || str[9] == 'T')) mode|= S_ISVTX;

		return mode;
	}
}
