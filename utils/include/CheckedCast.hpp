#pragma once

#include <stdio.h> // for stderr

template <class DST_I, class SRC_I> DST_I CheckedCast(const SRC_I &src)
{
	DST_I out = (DST_I)src;
	if ((SRC_I)out != src) {
		fprintf(stderr, "CheckedCast: 0x%llx != 0x%llx\n", (unsigned long long)src, (unsigned long long)out);
//		if (THROWING)
//			throw std::exception();
	}
	return out;
}
