#include <random>
#include <time.h>
#include <unistd.h>
#include <stdlib.h>
#include <atomic>
#include "BitTwiddle.hpp"
#include "RandomString.h"

static std::atomic<uint32_t> s_rnd_next_seed;

size_t RandomStringBuffer(unsigned char *out, size_t min_len, size_t max_len, unsigned int flags)
{
	std::mt19937 rng;
	rng.seed(getpid() ^ time(NULL) ^ RevBytes(uint32_t(clock())) ^ uint32_t(s_rnd_next_seed));
	size_t len = min_len;
	if (max_len > min_len) {
		std::uniform_int_distribution<std::mt19937::result_type> len_dist(0, max_len - min_len);
		len+= len_dist(rng);
	}

	std::uniform_int_distribution<std::mt19937::result_type> chr_dist(0, 0xff);
	for (size_t i = 0; i < len; ) {
		unsigned char v = chr_dist(rng);
		if (v == 0) {
			if ((flags & RNDF_ZERO) == 0) {
				continue;
			}
		} else if (v >= 'A' && v <= 'Z') {
			if ((flags & RNDF_HICASE) == 0) {
				continue;
			}
		} else if (v >= 'a' && v <= 'z') {
			if ((flags & RNDF_LOCASE) == 0) {
				continue;
			}
		} else if (v >= '0' && v <= '9') {
			if ((flags & RNDF_DIGITS) == 0) {
				continue;
			}
		} else if (v >= 128) {
			if ((flags & RNDF_X128) == 0) {
				continue;
			}
		} else if ((flags & RNDF_ETC) == 0) {
			continue;
		}

		out[i] = v;
		++i;
	}

	std::uniform_int_distribution<std::mt19937::result_type> seed_dist(0, 0xffffffff);
	s_rnd_next_seed = seed_dist(rng);

	return len;
}

void RandomStringAppend(std::string &out, size_t min_len, size_t max_len, unsigned int flags)
{
	const size_t orig_size = out.size();
	out.resize(orig_size + max_len);
	size_t len = RandomStringBuffer(&out[orig_size], min_len, max_len, flags);
	out.resize(orig_size + len);
}
