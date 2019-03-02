#include <unordered_set>
#include <string>
#include <mutex>
#include "PooledStrings.h"

static std::unordered_set<std::string> g_strings_pool;
static std::mutex g_strings_pool_mutex;

const char *PooledString(const std::string &s)
{
	if (s.empty())
		return "";

	std::lock_guard<std::mutex> locker(g_strings_pool_mutex);
	return g_strings_pool.insert(s).first->c_str();
}

const char *PooledString(const char *s)
{
	if (!s)
		return nullptr;

	const std::string tmp_str(s);
	return PooledString(tmp_str);
}

