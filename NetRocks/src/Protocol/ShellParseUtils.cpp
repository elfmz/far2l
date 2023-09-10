#include <sys/stat.h>

#include "ShellParseUtils.h"

namespace ShellParseUtils
{
	unsigned int Char2FileType(char c)
	{
		switch (c) {
			case 'l':
				return S_IFLNK;

			case 'd':
				return S_IFDIR;

			case 'c':
				return S_IFCHR;

			case 'b':
				return S_IFBLK;

			case 'p':
				return S_IFIFO;

			case 's':
				return S_IFSOCK;

			case 'f':
			default:
				return S_IFREG;
		}
	}

	unsigned int Triplet2FileMode(const char *c)
	{
		unsigned int out = 0;
		if (c[0] == 'r') out|= 4;
		if (c[1] == 'w') out|= 2;
		if (c[2] == 'x' || c[1] == 's' || c[1] == 't') out|= 1;
		return out;
	}

	unsigned int Str2Mode(const char *str, size_t len)
	{
		unsigned int mode = 0;
		if (len >= 1) {
			mode = Char2FileType(*str);
		}

		if (len >= 4) {
			mode|= Triplet2FileMode(str + 1) << 6;
		}

		if (len >= 7) {
			mode|= Triplet2FileMode(str + 4) << 3;
		}

		if (len >= 10) {
			mode|= Triplet2FileMode(str + 7);
		}

		return mode;
	}
}
