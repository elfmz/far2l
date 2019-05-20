#pragma once
#include <set>
#include <string>

struct WordExpansion : std::set<std::string>
{
	WordExpansion(const char *expression = nullptr);
	WordExpansion(const std::string &expression);

	void Expand(const char *expression);
};
