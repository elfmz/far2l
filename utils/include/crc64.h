#pragma once
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif

uint64_t crc64(uint64_t crc, const unsigned char *s, uint64_t l);

#ifdef __cplusplus
}
#endif
