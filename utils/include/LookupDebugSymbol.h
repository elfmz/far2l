#pragma once
#include <stdint.h>
#include <string>

struct LookupDebugSymbol
{
	std::string name;
	unsigned long offset{0};

	LookupDebugSymbol(const char *module_file, const void *module_base, const void *lookup_addr);
};
