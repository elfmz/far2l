#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <time.h>
#include "utils.h"

extern "C" void
	__attribute__ ((visibility("default")))
	__attribute__((weak))
	FN_NORETURN
		CustomPanic(const char *format, va_list args) noexcept
{
	// default weak implementation
	// to be overridden by WinPort implementation if process is hosting it
	abort();
}

void FN_NORETURN Panic(const char *format, ...) noexcept
{
	va_list args, args4log, args4cust;
	va_start(args, format);
	va_copy(args4log, args);
	va_copy(args4cust, args);
	vfprintf(stderr, format, args);
	fflush(stderr);
	va_end(args);

	FILE *flog = fopen(InMyConfig("crash.log").c_str(), "a");
	if (flog) {
		time_t now = time(NULL);
		struct tm t{};
		localtime_r(&now, &t);
		fprintf(flog, "[%u/%02u/%02u %02u:%02u] ",
			t.tm_year + 1900, t.tm_mon + 1, t.tm_mday, t.tm_hour, t.tm_min);
		vfprintf(flog, format, args4log);
		fputc('\n', flog);
		fclose(flog);
	}
	va_end(args4log);

	CustomPanic(format, args4cust);
	// --- CustomPanic doesn't return --- va_end(args4cust);
}
