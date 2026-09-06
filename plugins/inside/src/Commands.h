#pragma once
#include <set>
#include <string>

namespace Commands
{
	void Enum(const char *section, std::set<std::string> &out);
	std::string Get(const char *section, const std::string &name);
	void Execute(const std::string &cmd, const std::string &name, const std::string &result_file);
}
