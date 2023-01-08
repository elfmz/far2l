#pragma once

#define RNDF_ZERO    0x01 // NUL char (\0)
#define RNDF_LOCASE  0x02 // a..z
#define RNDF_HICASE  0x04 // A..Z
#define RNDF_DIGITS  0x08 // 0..9
#define RNDF_X128    0x10 // 128..255
#define RNDF_ETC     0x20 // all others

#define RNDF_ALNUM   (RNDF_LOCASE | RNDF_HICASE | RNDF_DIGITS)
#define RNDF_NZ      (RNDF_LOCASE | RNDF_HICASE | RNDF_DIGITS | RNDF_X128 | RNDF_ETC)
#define RNDF_ANY     (RNDF_ZERO | RNDF_LOCASE | RNDF_HICASE | RNDF_DIGITS | RNDF_X128 | RNDF_ETC)

size_t RandomStringBuffer(unsigned char *out, size_t min_len, size_t max_len, unsigned int flags);

static inline size_t RandomStringBuffer(char *out, size_t min_len, size_t max_len, unsigned int flags)
{
	return RandomStringBuffer((unsigned char *)out, min_len, max_len, flags);
}

void RandomStringAppend(std::string &out, size_t min_len, size_t max_len, unsigned int flags);
