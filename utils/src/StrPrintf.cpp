#include <utils.h>

std::string StrPrintfV(const char *format, va_list args)
{
	std::string out(15, '#'); // 15 is maximum number of characters when SSO still being used

	va_list args_copy;
	va_copy(args_copy, args);
	int rv = vsnprintf(&out[0], out.size(), format, args_copy);
	va_end(args_copy);

	if (rv >= 0 && rv < (int)out.size()) {
		out.resize(rv);
		return out;
	}

	out.resize(rv + 1);
	rv = vsnprintf(&out[0], out.size(), format, args);

	if (rv >= 0 && rv < (int)out.size()) {
		out.resize(rv);
		return out;
	}

	out = "Bad format string: ";
	out+= format;
	return out;
}

std::string StrPrintf(const char *format, ...)
{
	va_list args;
	va_start(args, format);

	const std::string &out = StrPrintfV(format, args);

	va_end(args);

	return out;
}

