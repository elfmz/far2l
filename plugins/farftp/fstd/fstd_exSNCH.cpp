#include <all_far.h>

#include "fstdlib.h"

int WINAPI StrNChr(const char *str, char ch, int maxlen)
{
	for (int n = 0; str[n] && (maxlen == -1 || n < maxlen); n++)
		if (str[n] == ch)
			return n;

	return -1;
}
