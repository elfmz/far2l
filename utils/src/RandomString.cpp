#include <random>
#include <time.h>
#include <unistd.h>
#include <stdlib.h>
#include "RandomString.h"

static const char s_rnd_alnum_chars[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789";

size_t RandomStringBuffer(char *out, size_t min_len, size_t max_len, bool only_alnum)
{
	std::mt19937 rng;
	rng.seed(getpid() ^ time(NULL) ^ (clock() << 16));

	size_t len = min_len;
	if (max_len > min_len) {
		std::uniform_int_distribution<std::mt19937::result_type> len_dist(0, max_len - min_len);
		len+= len_dist(rng);
	}

	if (only_alnum) {
		std::uniform_int_distribution<std::mt19937::result_type> chr_dist(0, sizeof(s_rnd_alnum_chars) - 2);
		for (size_t i = 0; i < len; ++i) {
			out[i] = s_rnd_alnum_chars[chr_dist(rng) % (sizeof(s_rnd_alnum_chars) - 1)];
		}
	} else {
		std::uniform_int_distribution<std::mt19937::result_type> chr_dist(0, 0xff);
		for (size_t i = 0; i < len; ++i) {
			out[i] = (char)(unsigned char)chr_dist(rng);
		}
	}

	return len;
}

void RandomStringAppend(std::string &out, size_t min_len, size_t max_len)
{
	const size_t orig_size = out.size();
	out.resize(orig_size + max_len);
	size_t len = RandomStringBuffer(&out[orig_size], min_len, max_len);
	out.resize(orig_size + len);
}
