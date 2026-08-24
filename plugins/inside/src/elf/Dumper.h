#pragma once
#include <string>
#include <set>
#include <cstdint>

namespace Root
{
	void Commands(std::set<std::string> &out);
	void Query(const std::string &command, const std::string &name, const std::string &result_file);
	void Store(const std::string &command, const std::string &name, const std::string &result_file);
	void Clear(const std::string &command, const std::string &name);
}

namespace Disasm
{
	void Commands(uint16_t machine, std::set<std::string> &out);
	void Query(uint16_t machine, const std::string &command, const std::string &name, const std::string &result_file);
	void Store(uint16_t machine, const std::string &command, const std::string &name, const std::string &result_file);
	void Clear(uint16_t machine, const std::string &command, const std::string &name);
}

namespace Binary
{
	void Query(unsigned long long ofs, unsigned long long len, const std::string &name, const std::string &result_file);
}
