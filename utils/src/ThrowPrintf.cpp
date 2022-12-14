#include "utils.h"
#include <stdexcept>

void FN_NORETURN ThrowPrintf(const char *format, ...)
{
	va_list args;
	va_start(args, format);
	const std::string &what = StrPrintfV(format, args);
	va_end(args);

	throw std::runtime_error(what);
}
