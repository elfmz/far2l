#pragma once
#include <stdlib.h>
#include <time.h>
#include <string>

namespace ShellParseUtils
{
	std::string ExtractStringTail(std::string &line, const char *separators);
	std::string ExtractStringHead(std::string &line, const char *separators);

	unsigned int Str2Mode(const char *str, size_t len);

	bool ParseLineFromLS(std::string &line, // input but also used as scratch string
		std::string &name, std::string &owner, std::string &group,
		timespec &access_time, timespec &modification_time, timespec &status_change_time,
		unsigned long long &size, mode_t &mode);
}
