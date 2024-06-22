#include "utils.h"
#include "StringConfig.h"
#include <stdlib.h>

static std::string StringEscape(const std::string &str)
{
	std::string out;
	out.reserve(str.size());
	for (auto ch : str) {
		switch (ch) {
			case 0: out+= "\\0"; break;
			case '\t': out+= "\\t"; break;
			case '\r': out+= "\\r"; break;
			case '\n': out+= "\\n"; break;
			case '\\': out+= "\\\\"; break;
			case ':': out+= "\\^"; break;
			case ' ': out+= "\\_"; break;
			default: out+= ch;
		}
	}
	return out;
}

static std::string StringUnescape(const std::string &str)
{
	std::string out;
	out.reserve(str.size());
	for (size_t i = 0; i < str.size(); ++i) {
		if (str[i] == '\\' && i + 1 < str.size()) {
			++i;
			switch (str[i]) {
				case 0: out.append(1, (char)0); break;
				case 't':  out+= '\t'; break;
				case 'r':  out+= '\r'; break;
				case 'n':  out+= '\n'; break;
				case '\\': out+= '\\'; break;
				case '^':  out+= ':'; break;
				case '_':  out+= ' '; break;
				default: 
					out+= '\\';
					out+= str[i];
					fprintf(stderr, "StringUnescape: wrong escape char '%c'\n", str[i]);
			}
		} else {
			out+= str[i];
		}
	}
	return out;
}

StringConfig::StringConfig()
{
}

StringConfig::StringConfig(const std::string &serialized_str)
{
	std::string s;
	for (size_t i = 0, ii = 0; i <= serialized_str.size(); ++i) {
		if (i == serialized_str.size() || serialized_str[i] == ' ') {
			s = serialized_str.substr(ii, i - ii);
			size_t p = s.find(':');
			if (p != std::string::npos) {
				const std::string &value = s.substr( p + 1);
				s.resize(p);
				_entries[StringUnescape(s)] = StringUnescape(value);
			}
			ii = i + 1;
		}
	}
}

StringConfig::~StringConfig()
{
}

std::string StringConfig::Serialize() const
{
	std::string out;
	for (const auto &it : _entries) {
		out+= StringEscape(it.first);
		out+= ':';
		out+= StringEscape(it.second);
		out+= ' ';
	}
	return out;
}

int StringConfig::GetInt(const char *name, int def) const
{
	const auto &it = _entries.find(name);
	return (it != _entries.end()) ? atoi(it->second.c_str()) : def;
}

unsigned long long StringConfig::GetHexULL(const char *name, unsigned long long def) const
{
	const auto &it = _entries.find(name);
	return (it != _entries.end()) ? strtoull(it->second.c_str(), nullptr, 16) : def;
}

std::string StringConfig::GetString(const char *name, const char *def) const
{
	const auto &it = _entries.find(name);
	return (it != _entries.end()) ? it->second : def;
}

void StringConfig::SetInt(const char *name, int val)
{
	_entries[name] = ToDec(val);
}

void StringConfig::SetHexULL(const char *name, unsigned long long val)
{
	_entries[name] = ToHex(val);
}

void StringConfig::SetString(const char *name, const std::string &val)
{
	_entries[name] = val;
}

void StringConfig::SetString(const char *name, const char *val)
{
	_entries[name] = val;
}

void StringConfig::Delete(const char *name)
{
	_entries.erase(name);
}
