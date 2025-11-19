#pragma once

#define LIKELY(x)       __builtin_expect(!!(x), 1)
#define UNLIKELY(x)     __builtin_expect(!!(x), 0)

#define FN_NORETURN              __attribute__ ((noreturn))
#define FN_NOINLINE              __attribute__ ((noinline))
#define FN_PRINTF_ARGS(FMT_ARG)  __attribute__ ((format(printf, FMT_ARG, FMT_ARG + 1)))

