#pragma once

#include <cstddef>
#include <string>
#include <vector>

namespace Transformer
{
	enum class OutputMode
	{
		Replace,
		Message,
		Viewer,
	};

	struct Transform
	{
		std::wstring name;
		std::string command;
		OutputMode output_mode = OutputMode::Replace;
		std::string source;
		std::size_t line = 0;
	};

	struct Group
	{
		std::wstring name;
		std::vector<std::wstring> masks;
		bool common = false;
		std::vector<Transform> items;
	};

	struct Config
	{
		std::vector<Group> groups;
		std::vector<std::wstring> diagnostics;
	};

	// Directories are considered in the supplied order.  Within a directory,
	// regular files whose names end in ".ini" are loaded lexicographically.
	Config LoadFormatDirectories(const std::vector<std::string> &directories);

	// Common groups always match.  Other groups match when at least one mask
	// matches the basename of at least one filename.
	bool MatchGroup(const Group &group, const std::vector<std::wstring> &filenames);
}
