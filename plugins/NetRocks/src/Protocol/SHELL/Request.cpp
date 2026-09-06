#include <utils.h>
#include "Request.h"

Request::Request(const char *cmd_ending)
	: _request(cmd_ending)
{
}

Request &Request::Add(const std::string &arg, char ending)
{
	for (size_t ofs = 0; ofs < arg.size();) {
		const size_t p = arg.find_first_of(" \t\r\n\\", ofs);
		if (p == std::string::npos) {
			_request.append(arg.data() + ofs, arg.size() - ofs);
			break;
		}
		_request.append(arg.data() + ofs, p - ofs);
		_request+= '\\';
		_request+= arg[p];
		ofs = p + 1;
	}

	_request+= ending;
	return *this;
}

Request &Request::AddFmt(const char *fmt_ending, ...)
{
	va_list args;
	va_start(args, fmt_ending);
	_request+= StrPrintfV(fmt_ending, args);
	va_end(args);
	return *this;
}
