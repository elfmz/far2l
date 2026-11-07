#include "GitIgnore.hpp"
#include "ExecAsync.h"
#include <cstdlib>
#include <map>
#include <set>
#include <sys/stat.h>
#include <unistd.h>
#include <utility>
#include <vector>

namespace
{
std::string Parent(const std::string &path)
{
	const size_t slash = path.rfind('/');
	return slash == std::string::npos ? "." : (slash == 0 ? "/" : path.substr(0, slash));
}

bool IsWithin(const std::string &path, const std::string &directory)
{
	return directory == "/" || path == directory
			|| (path.size() > directory.size() && path.compare(0, directory.size(), directory) == 0
					&& path[directory.size()] == '/');
}

struct Snapshot
{
	std::set<std::string> ignored;
	bool ready{false};
};

struct Repository
{
	std::string root;
	std::map<std::pair<std::string, bool>, Snapshot> snapshots;
};
}

struct GitIgnoreFilter::Impl
{
	std::function<bool()> cancelled;
	std::function<void(const std::string &)> failed;
	std::map<std::string, std::shared_ptr<Repository>> directories;
	std::map<std::string, std::string> resolvedDirectories;
	std::string searchRoot;
	bool recursive{true};
	std::vector<std::string> linkedScopes;

	Impl(std::function<bool()> cancelled_, std::function<void(const std::string &)> failed_)
		: cancelled(std::move(cancelled_)), failed(std::move(failed_))
	{}

	std::string ResolveDirectory(const std::string &directory)
	{
		const auto cached = resolvedDirectories.find(directory);
		if (cached != resolvedDirectories.end())
			return cached->second;
		std::string physical;
		char *resolved = realpath(directory.c_str(), nullptr);
		if (resolved) {
			physical = resolved;
			free(resolved);
		}
		resolvedDirectories.emplace(directory, physical);
		return physical;
	}

	bool LoadIgnored(const std::string &scope, std::set<std::string> &paths)
	{
		ExecAsync git("git");
		git.AddArguments("git", "-C", scope, "ls-files", "--full-name", "-z",
				"--others", "--ignored", "--exclude-standard", "--directory", "--",
				recursive ? ":(literal)." : ":(glob)*");
		if (!git.Start())
			return false;

		std::string pending;
		for (;;) {
			const bool complete = git.Wait(50);
			if (cancelled()) {
				if (!complete)
					git.KillHardly();
				git.Wait();
				return false;
			}
			pending+= git.FetchStdout();
			size_t start = 0, end;
			while ((end = pending.find('\0', start)) != std::string::npos) {
				std::string path = pending.substr(start, end - start);
				if (!path.empty() && path.back() == '/')
					path.pop_back();
				if (!path.empty())
					paths.insert(std::move(path));
				start = end + 1;
			}
			pending.erase(0, start);
			// Drain diagnostics as well; ExecAsync mirrors them to the log.
			git.FetchStderr();
			if (complete)
				break;
		}
		return pending.empty() && !git.ExecError() && !git.ExitSignal() && git.ExitCode() == 0;
	}

	std::shared_ptr<Repository> Context(const std::string &directory)
	{
		const auto cached = directories.find(directory);
		if (cached != directories.end())
			return cached->second;
		if (cancelled())
			return {};

		std::shared_ptr<Repository> repository;
		struct stat marker{};
		// Worktree markers may be files. All ancestors here are physical paths.
		if (lstat((directory + "/.git").c_str(), &marker) == 0
				&& (S_ISDIR(marker.st_mode) || S_ISREG(marker.st_mode))) {
			repository = std::make_shared<Repository>();
			repository->root = directory;
		} else if (directory != "/") {
			repository = Context(Parent(directory));
		}
		directories.emplace(directory, repository);
		return repository;
	}

	const Snapshot &Ignored(Repository &repository, const std::string &parent)
	{
		std::string scope = repository.root;
		if (IsWithin(parent, searchRoot)) {
			if (IsWithin(searchRoot, repository.root))
				scope = searchRoot;
		} else {
			// A followed directory link can enter a different subtree or worktree.
			scope.clear();
			for (const auto &linked : linkedScopes) {
				if (IsWithin(parent, linked) && IsWithin(linked, repository.root)) {
					scope = linked;
					break;
				}
			}
			if (scope.empty()) {
				scope = parent;
				linkedScopes.push_back(scope);
			}
		}
		const auto inserted = repository.snapshots.emplace(std::make_pair(scope, recursive), Snapshot{});
		auto &snapshot = inserted.first->second;
		if (inserted.second) {
			// Git's --others already excludes tracked files and never collapses
			// directories containing them. A separate index snapshot is unnecessary.
			snapshot.ready = LoadIgnored(scope, snapshot.ignored);
			if (!snapshot.ready && !cancelled())
				failed(scope);
		}
		return snapshot;
	}
};

GitIgnoreFilter::GitIgnoreFilter(std::function<bool()> cancelled,
		std::function<void(const std::string &)> failed)
	: _impl(new Impl(std::move(cancelled), std::move(failed)))
{
}

GitIgnoreFilter::~GitIgnoreFilter() = default;

bool GitIgnoreFilter::IsAvailable()
{
	std::string paths;
	if (const char *env = getenv("PATH")) {
		paths = env;
	} else {
		const size_t size = confstr(_CS_PATH, nullptr, 0);
		if (size) {
			paths.resize(size);
			confstr(_CS_PATH, &paths[0], size);
			paths.resize(size - 1);
		} else {
			paths = "/usr/bin:/bin";
		}
	}
	for (size_t start = 0;;) {
		const size_t end = paths.find(':', start);
		std::string file = paths.substr(start, end == std::string::npos ? end : end - start);
		file = file.empty() ? "git" : file + "/git";
		struct stat info{};
		if (stat(file.c_str(), &info) == 0 && S_ISREG(info.st_mode) && access(file.c_str(), X_OK) == 0)
			return true;
		if (end == std::string::npos)
			return false;
		start = end + 1;
	}
}

void GitIgnoreFilter::SetSearchRoot(const std::string &path, bool recursive)
{
	_impl->searchRoot = _impl->ResolveDirectory(path);
	_impl->recursive = recursive;
	_impl->linkedScopes.clear();
	if (_impl->searchRoot.empty() && !_impl->cancelled())
		_impl->failed(path);
}

bool GitIgnoreFilter::IsIgnored(const std::string &path)
{
	if (path.empty() || _impl->searchRoot.empty() || _impl->cancelled())
		return false;
	std::string normalized = path;
	while (normalized.size() > 1 && normalized.back() == '/')
		normalized.pop_back();
	const std::string parent = _impl->ResolveDirectory(Parent(normalized));
	if (parent.empty())
		return false;
	const auto repository = _impl->Context(parent);
	if (!repository)
		return false;

	// Resolve parent links, keeping the entry's own name so ignored symlinks
	// are checked before traversal can follow them into another worktree.
	const size_t slash = normalized.rfind('/');
	const std::string name = normalized.substr(slash == std::string::npos ? 0 : slash + 1);
	if (name == ".git")
		return true;
	const std::string physical = parent + (parent == "/" ? "" : "/") + name;
	const std::string relative = physical.substr(repository->root == "/" ? 1 : repository->root.size() + 1);
	if (relative.compare(0, 5, ".git/") == 0)
		return true;
	const auto &snapshot = _impl->Ignored(*repository, parent);
	if (!snapshot.ready)
		return false;
	if (snapshot.ignored.count(relative))
		return true;
	for (size_t pos = relative.find('/'); pos != std::string::npos; pos = relative.find('/', pos + 1)) {
		if (snapshot.ignored.count(relative.substr(0, pos)))
			return true;
	}
	return false;
}
