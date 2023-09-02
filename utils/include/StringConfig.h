#pragma once
#include <string>
#include <map>

class StringConfig
{
	std::map<std::string, std::string> _entries;

public:
	StringConfig();
	StringConfig(const std::string &serialized_str);
	~StringConfig();

	std::string Serialize() const;

	int GetInt(const char *name, int def = 0) const;
	unsigned long long GetHexULL(const char *name, unsigned long long def = 0) const;
	std::string GetString(const char *name, const char *def = "") const;

	void SetInt(const char *name, int val);
	void SetHexULL(const char *name, unsigned long long val);
	void SetString(const char *name, const std::string &val);
	void SetString(const char *name, const char *val);

	void Delete(const char *name);
};
