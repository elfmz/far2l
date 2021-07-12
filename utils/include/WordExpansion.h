#pragma once
#include <vector>
#include <string>

struct WordExpansion : std::vector<std::string>
{
	WordExpansion(const char *expression = nullptr);
	WordExpansion(const std::string &expression);

	void Expand(const char *expression);
};
