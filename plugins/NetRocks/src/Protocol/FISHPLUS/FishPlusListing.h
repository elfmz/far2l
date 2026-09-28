#pragma once
#include <sys/stat.h>
#include <sys/types.h>
#include <string>
#include <vector>

/*
	Port of f4's plugins/netfox/fishplus/fs.go and ls.go.

	Listing is where remote hosts differ the most, so the helper probes what is
	there and names the winner on the first payload line of every listing:

		M find | M stat | M statbsd | M ls epoch | M ls iso | M ls bsd

	Because the marker travels with the data, the client always knows which
	parser to use - including when the backend was switched at runtime.
*/

namespace FishPlus
{
	struct Entry
	{
		std::string name;
		unsigned long long size{0};
		mode_t mode{0};				// raw st_mode, file type bits included
		timespec mtime{};
		timespec atime{};
		timespec ctime{};
		int uid{-1};
		int gid{-1};
		// Whether a symlink resolves to a directory. Only the find backend
		// reports it for free; elsewhere it stays false and the caller has to
		// resolve the link itself if it cares.
		bool target_is_dir{false};

		bool IsDir() const { return (mode & S_IFMT) == S_IFDIR; }
		bool IsSymlink() const { return (mode & S_IFMT) == S_IFLNK; }
	};

	// Parses a whole listing payload, first line being the mode marker.
	// keep_path makes Entry::name carry the full path it was given rather than
	// its base name, which is what a tree search needs.
	// Returns the backend name; a line that fails to parse is skipped, because
	// a stray diagnostic must not cost the user the panel.
	std::string ParseListing(const std::vector<std::string> &lines,
		std::vector<Entry> &out, bool keep_path = false);

	// Parses a single entry of an already known backend, used by the info and
	// linfo commands, whose answer is one marker line plus one entry.
	bool ParseSingle(const std::vector<std::string> &lines, Entry &out);
}
