#pragma once
#include <string>

namespace Storage
{
	void Clear();
	void Clear(const std::string &key_file, const std::string &key_string);
	void Put(const std::string &key_file, const std::string &key_string, const std::string &data_file);
	bool Get(const std::string &key_file, const std::string &key_string, const std::string &data_file);
}

