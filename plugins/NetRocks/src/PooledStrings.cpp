#include <utils.h>
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
	if (!*s)
		return "";

	return PooledString(std::string(s));
}

const char *PooledString(const wchar_t *s)
{
	return PooledString(Wide2MB(s));
}

////////////////////////
static std::unordered_set<std::wstring> g_wide_strings_pool;
static std::mutex g_wide_strings_pool_mutex;
static std::wstring g_wide_strings_pool_tmp;


const wchar_t *MB2WidePooled(const char *str)
{
	if (str == nullptr)
		return nullptr;

	if (!*str)
		return L"";

	std::lock_guard<std::mutex> locker(g_wide_strings_pool_mutex);
	MB2Wide(str, g_wide_strings_pool_tmp);
	return g_wide_strings_pool.emplace(g_wide_strings_pool_tmp).first->c_str();
}

const wchar_t *MB2WidePooled(const std::string &str)
{
	return MB2WidePooled(str.c_str());
}

//////////////////////////////////
void PurgePooledStrings()
{
	{
		std::lock_guard<std::mutex> locker(g_strings_pool_mutex);
		g_strings_pool.clear();
	}

	{
		std::lock_guard<std::mutex> locker(g_wide_strings_pool_mutex);
		g_wide_strings_pool.clear();
	}
}
