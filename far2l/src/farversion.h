#pragma once

extern const uint32_t FAR_VERSION;
extern const char *FAR_BUILD;

extern const char *Copyright;

#if defined(__x86_64__)
#define FAR_PLATFORM "x64"
#elif defined(__ppc64__)
#define FAR_PLATFORM "ppc64"
#elif defined(__arm64__) || defined(__aarch64__)
#define FAR_PLATFORM "arm64"
#elif defined(__arm__)
#define FAR_PLATFORM "arm"
#elif defined(__mips__)
#ifdef __MIPSEL__
#define FAR_PLATFORM "mipsel"
#else
#define FAR_PLATFORM "mips"
#endif
#elif defined(__e2k__)
#define FAR_PLATFORM "e2k"
#elif defined(__riscv)
#if __riscv_xlen == 64
#define FAR_PLATFORM "rv64"
#else
#define FAR_PLATFORM "rv32"
#endif
#elif defined(__i386__)
#define FAR_PLATFORM "x86"
#else
#define FAR_PLATFORM "unknown"
#endif
