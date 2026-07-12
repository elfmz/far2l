#include "Config.hpp"

#include <WideMB.h>

#include <algorithm>
#include <cctype>
#include <filesystem>
#include <fstream>
#include <fnmatch.h>
#include <optional>
#include <set>
#include <system_error>

namespace Transformer
{
	namespace
	{
		namespace fs = std::filesystem;

		struct PendingItem
		{
			Group *group = nullptr;
			Transform transform;
			bool has_command = false;
			bool valid = true;
		};

		std::string Trim(const std::string &value)
		{
			auto is_space = [](unsigned char ch) {
				return ch == ' ' || ch == '\t' || ch == '\r' || ch == '\n' || ch == '\f' || ch == '\v';
			};

			auto first = std::find_if_not(value.begin(), value.end(), is_space);
			if (first == value.end()) {
				return {};
			}
			auto last = std::find_if_not(value.rbegin(), value.rend(), is_space).base();
			return std::string(first, last);
		}

		template <typename Char>
		Char LowerASCII(Char ch)
		{
			return ch >= static_cast<Char>('A') && ch <= static_cast<Char>('Z')
				? static_cast<Char>(ch + ('a' - 'A')) : ch;
		}

		template <typename Char>
		bool EqualsASCIIInsensitive(const std::basic_string<Char> &left, const Char *right)
		{
			const std::size_t right_size = std::char_traits<Char>::length(right);
			return left.size() == right_size
				&& std::equal(left.begin(), left.end(), right, [](Char a, Char b) {
					return LowerASCII(a) == LowerASCII(b);
				});
		}

		bool StartsWithWordASCIIInsensitive(const std::string &value, const char *word)
		{
			const std::size_t word_size = std::char_traits<char>::length(word);
			if (value.size() < word_size
					|| !EqualsASCIIInsensitive(value.substr(0, word_size), word)) {
				return false;
			}
			return value.size() == word_size
				|| std::isspace(static_cast<unsigned char>(value[word_size])) != 0;
		}

		std::string DecodeValue(const std::string &value)
		{
			if (value.size() < 2 || value.front() != '"' || value.back() != '"') {
				return value;
			}
			std::string result;
			result.reserve(value.size() - 2);
			for (std::size_t index = 1; index + 1 < value.size(); ++index) {
				if (value[index] != '\\' || index + 2 >= value.size()) {
					result.push_back(value[index]);
					continue;
				}
				switch (value[++index]) {
					case 'r': result.push_back('\r'); break;
					case 'n': result.push_back('\n'); break;
					case 't': result.push_back('\t'); break;
					case '0': result.push_back('\0'); break;
					case '\\': result.push_back('\\'); break;
					default:
						result.push_back('\\');
						result.push_back(value[index]);
						break;
				}
			}
			return result;
		}

		void AddDiagnostic(Config &config, const std::string &source, std::size_t line,
				const std::wstring &message)
		{
			std::wstring diagnostic = StrMB2Wide(source);
			if (line != 0) {
				diagnostic += L":" + std::to_wstring(line);
			}
			diagnostic += L": " + message;
			config.diagnostics.emplace_back(std::move(diagnostic));
		}

		void FinishPending(Config &config, std::optional<PendingItem> &pending)
		{
			if (!pending) {
				return;
			}

			if (!pending->group || pending->transform.name.empty()
					|| !pending->has_command || pending->transform.command.empty()) {
				AddDiagnostic(config, pending->transform.source, pending->transform.line,
					L"incomplete Item (a non-empty Item and Command are required)");
			} else if (pending->valid) {
				pending->group->items.emplace_back(std::move(pending->transform));
			}
			pending.reset();
		}

		Group *FindOrCreateGroup(Config &config, std::wstring name, bool common)
		{
			for (auto &group : config.groups) {
				const bool same_name = common || group.common
					? EqualsASCIIInsensitive(group.name, name.c_str())
					: group.name == name;
				if (same_name) {
					group.common = group.common || common;
					return &group;
				}
			}

			config.groups.push_back({std::move(name), {}, common, {}});
			return &config.groups.back();
		}

		bool ParseSectionName(const std::string &line, std::wstring &name, bool &common)
		{
			if (line.size() < 2 || line.front() != '[' || line.back() != ']') {
				return false;
			}

			const std::string section = Trim(line.substr(1, line.size() - 2));
			if (section.empty()) {
				return false;
			}

			if (EqualsASCIIInsensitive(section, "Common")) {
				name = L"Common";
				common = true;
				return true;
			}

			if (StartsWithWordASCIIInsensitive(section, "FileType")) {
				const std::string display = Trim(section.substr(sizeof("FileType") - 1));
				if (display.size() < 2 || display.front() != '"' || display.back() != '"') {
					return false;
				}
				const std::string unquoted = display.substr(1, display.size() - 2);
				if (unquoted.empty() || unquoted.find('"') != std::string::npos) {
					return false;
				}
				name = StrMB2Wide(unquoted);
				common = false;
				return true;
			}

			name = StrMB2Wide(section);
			common = false;
			return true;
		}

		bool ParseOutputMode(const std::string &value, OutputMode &result)
		{
			const std::string decoded = DecodeValue(value);
			if (EqualsASCIIInsensitive(decoded, "Replace")) {
				result = OutputMode::Replace;
				return true;
			}
			if (EqualsASCIIInsensitive(decoded, "Message")) {
				result = OutputMode::Message;
				return true;
			}
			if (EqualsASCIIInsensitive(decoded, "Viewer")) {
				result = OutputMode::Viewer;
				return true;
			}
			return false;
		}

		void AppendMasks(Group &group, const std::string &value)
		{
			std::size_t begin = 0;
			while (begin <= value.size()) {
				const std::size_t end = value.find(';', begin);
				const std::string mask = Trim(value.substr(begin,
					end == std::string::npos ? std::string::npos : end - begin));
				if (!mask.empty()) {
					group.masks.emplace_back(StrMB2Wide(DecodeValue(mask)));
				}
				if (end == std::string::npos) {
					break;
				}
				begin = end + 1;
			}
		}

		void ParseFile(Config &config, const fs::path &path)
		{
			const std::string source = path.string();
			std::ifstream stream(path, std::ios::binary);
			if (!stream) {
				AddDiagnostic(config, source, 0, L"cannot read configuration file");
				return;
			}

			std::string contents((std::istreambuf_iterator<char>(stream)),
				std::istreambuf_iterator<char>());
			if (contents.size() >= 3
					&& static_cast<unsigned char>(contents[0]) == 0xef
					&& static_cast<unsigned char>(contents[1]) == 0xbb
					&& static_cast<unsigned char>(contents[2]) == 0xbf) {
				contents.erase(0, 3);
			}

			Group *current_group = nullptr;
			std::optional<PendingItem> pending;
			std::size_t line_number = 0;
			std::size_t offset = 0;

			while (offset <= contents.size()) {
				const std::size_t newline = contents.find('\n', offset);
				std::string raw = contents.substr(offset,
					newline == std::string::npos ? std::string::npos : newline - offset);
				++line_number;
				if (!raw.empty() && raw.back() == '\r') {
					raw.pop_back();
				}
				const std::string line = Trim(raw);

				if (!line.empty() && line.front() != '#' && line.front() != ';') {
					if (line.front() == '[') {
						FinishPending(config, pending);
						std::wstring group_name;
						bool common = false;
						if (!ParseSectionName(line, group_name, common)) {
							AddDiagnostic(config, source, line_number, L"malformed section");
							current_group = nullptr;
						} else {
							current_group = FindOrCreateGroup(config, std::move(group_name), common);
						}
					} else {
						const std::size_t equals = line.find('=');
						const std::string key = Trim(line.substr(0, equals));
						const std::string value = equals == std::string::npos
							? std::string() : Trim(line.substr(equals + 1));

						if (EqualsASCIIInsensitive(key, "Item")) {
							FinishPending(config, pending);
							pending.emplace();
							pending->group = current_group;
							pending->transform.name = StrMB2Wide(DecodeValue(value));
							pending->transform.source = source;
							pending->transform.line = line_number;
						} else if (EqualsASCIIInsensitive(key, "Command")) {
							if (!pending || !pending->group || pending->has_command) {
								AddDiagnostic(config, source, line_number, L"orphan Command");
							} else {
								pending->transform.command = DecodeValue(value);
								pending->has_command = true;
							}
						} else if (EqualsASCIIInsensitive(key, "Output")) {
							if (!pending || !pending->group || !pending->has_command) {
								AddDiagnostic(config, source, line_number, L"orphan Output");
							} else {
								OutputMode output_mode = OutputMode::Replace;
								if (!ParseOutputMode(value, output_mode)) {
									AddDiagnostic(config, source, line_number,
										L"malformed Output (expected Replace, Message, or Viewer)");
									pending->valid = false;
								} else {
									pending->transform.output_mode = output_mode;
								}
							}
						} else if (EqualsASCIIInsensitive(key, "Masks") && current_group) {
							AppendMasks(*current_group, value);
						}
					}
				}

				if (newline == std::string::npos) {
					break;
				}
				offset = newline + 1;
			}

			FinishPending(config, pending);
		}

		std::string DirectoryIdentity(const fs::path &path)
		{
			std::error_code error;
			const fs::path canonical = fs::weakly_canonical(path, error);
			return error ? path.lexically_normal().string() : canonical.string();
		}

		std::string Basename(const std::string &filename)
		{
			const std::size_t separator = filename.find_last_of("/\\");
			return separator == std::string::npos ? filename : filename.substr(separator + 1);
		}
	}

	Config LoadFormatDirectories(const std::vector<std::string> &directories)
	{
		Config config;
		std::set<std::string> visited;

		for (const auto &directory_name : directories) {
			const fs::path directory(directory_name);
			std::error_code error;
			if (!fs::is_directory(directory, error)) {
				continue;
			}
			if (!visited.emplace(DirectoryIdentity(directory)).second) {
				continue;
			}

			std::vector<fs::path> files;
			fs::directory_iterator iterator(directory, fs::directory_options::skip_permission_denied, error);
			const fs::directory_iterator end;
			while (!error && iterator != end) {
				std::error_code file_error;
				if (iterator->is_regular_file(file_error)
						&& !file_error && iterator->path().extension() == ".ini") {
					files.emplace_back(iterator->path());
				}
				iterator.increment(error);
			}
			if (error) {
				AddDiagnostic(config, directory.string(), 0,
					L"cannot enumerate configuration directory");
			}

			std::sort(files.begin(), files.end(), [](const fs::path &left, const fs::path &right) {
				return left.filename().string() < right.filename().string();
			});
			for (const auto &file : files) {
				ParseFile(config, file);
			}
		}

		return config;
	}

	bool MatchGroup(const Group &group, const std::vector<std::wstring> &filenames)
	{
		if (group.common) {
			return true;
		}

		for (const auto &filename_wide : filenames) {
			const std::string filename = Basename(StrWide2MB(filename_wide));
			for (const auto &mask_wide : group.masks) {
				const std::string mask = StrWide2MB(mask_wide);
				if (fnmatch(mask.c_str(), filename.c_str(), 0) == 0) {
					return true;
				}
			}
		}
		return false;
	}
}
