#pragma once

#include <functional>
#include <memory>
#include <string>

// A search-scoped snapshot of Git's ignore decisions for local worktrees.
class GitIgnoreFilter
{
	struct Impl;
	std::unique_ptr<Impl> _impl;

public:
	static bool IsAvailable();
	GitIgnoreFilter(std::function<bool()> cancelled, std::function<void(const std::string &)> failed);
	~GitIgnoreFilter();
	void SetSearchRoot(const std::string &path, bool recursive);
	bool IsIgnored(const std::string &path);
};
