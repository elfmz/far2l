#if defined (__linux__) || defined(__FreeBSD__) || defined(__OpenBSD__) || defined(__NetBSD__) || defined(__DragonFly__)

#include "AppProvider.hpp"
#include "XDGBasedAppProvider.hpp"
#include "lng.hpp"
#include "KeyFileHelper.h"
#include "WideMB.h"
#include "common.hpp"
#include "utils.h"
#include <dirent.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <cctype>
#include <algorithm>
#include <cstring>
#include <fstream>
#include <sstream>
#include <unordered_set>
#include <utility>
#include <vector>
#include <map>


#define INI_LOCATION_LINUX InMyConfig("plugins/openwith/config.ini")
#define INI_SECTION_LINUX  "Settings.Linux"

// ****************************** Public API  ******************************


XDGBasedAppProvider::XDGBasedAppProvider(TMsgGetter msg_getter) : AppProvider(std::move(msg_getter))
{
}


void XDGBasedAppProvider::LoadPlatformSettings()
{
	KeyFileReadSection kfh(INI_LOCATION_LINUX, INI_SECTION_LINUX);
	_filter_by_show_in = kfh.GetInt("FilterByShowIn", 0) != 0;
	_validate_try_exec = kfh.GetInt("ValidateTryExec", 0) != 0;
	_use_mimeinfo_cache = kfh.GetInt("UseMimeinfoCache", 1) != 0;
	_use_extension_based_fallback = kfh.GetInt("UseExtensionBasedFallback", 1) != 0;
	_use_xdg_mime_tool = kfh.GetInt("UseXdgMimeTool", 1) != 0;
	_use_file_tool = kfh.GetInt("UseFileTool", 1) != 0;
}


void XDGBasedAppProvider::SavePlatformSettings()
{
	KeyFileHelper kfh(INI_LOCATION_LINUX);
	kfh.SetInt(INI_SECTION_LINUX, "FilterByShowIn", _filter_by_show_in);
	kfh.SetInt(INI_SECTION_LINUX, "ValidateTryExec", _validate_try_exec);
	kfh.SetInt(INI_SECTION_LINUX, "UseMimeinfoCache", _use_mimeinfo_cache);
	kfh.SetInt(INI_SECTION_LINUX, "UseExtensionBasedFallback", _use_extension_based_fallback);
	kfh.SetInt(INI_SECTION_LINUX, "UseXdgMimeTool", _use_xdg_mime_tool);
	kfh.SetInt(INI_SECTION_LINUX, "UseFileTool", _use_file_tool);
	kfh.Save();
}


std::vector<ProviderSetting> XDGBasedAppProvider::GetPlatformSettings()
{
	return {
		{ L"UseXdgMimeTool", m_GetMsg(MUseXdgMimeTool), _use_xdg_mime_tool },
		{ L"UseFileTool", m_GetMsg(MUseFileTool), _use_file_tool },
		{ L"UseExtensionBasedFallback", m_GetMsg(MUseExtensionBasedFallback), _use_extension_based_fallback },
		{ L"UseMimeinfoCache", m_GetMsg(MUseMimeinfoCache), _use_mimeinfo_cache },
		{ L"FilterByShowIn", m_GetMsg(MFilterByShowIn), _filter_by_show_in },
		{ L"ValidateTryExec", m_GetMsg(MValidateTryExec), _validate_try_exec }
	};
}


void XDGBasedAppProvider::SetPlatformSettings(const std::vector<ProviderSetting>& settings)
{
	for (const auto& s : settings)
	{
		if (s.internal_key == L"FilterByShowIn") _filter_by_show_in = s.value;
		else if (s.internal_key == L"ValidateTryExec") _validate_try_exec = s.value;
		else if (s.internal_key == L"UseMimeinfoCache") _use_mimeinfo_cache = s.value;
		else if (s.internal_key == L"UseExtensionBasedFallback") _use_extension_based_fallback = s.value;
		else if (s.internal_key == L"UseXdgMimeTool") _use_xdg_mime_tool = s.value;
		else if (s.internal_key == L"UseFileTool") _use_file_tool = s.value;
	}
}


std::vector<CandidateInfo> XDGBasedAppProvider::GetAppCandidates(const std::wstring& pathname)
{
	_desktop_entry_cache.clear();
	std::vector<std::string> prioritized_mimes = CollectAndPrioritizeMimeTypes(StrWide2MB(pathname));
	auto desktop_paths = GetDesktopFileSearchPaths();
	auto mimeapps_paths = GetMimeappsListSearchPaths();
	auto associations = ParseMimeappsLists(mimeapps_paths);
	std::string top_default_app = GetDefaultApp(prioritized_mimes.empty() ? "" : prioritized_mimes[0]);
	std::string current_desktop_env = _filter_by_show_in ? GetEnv("XDG_CURRENT_DESKTOP") : "";

	CandidateSearchContext context(prioritized_mimes, associations, desktop_paths, current_desktop_env);

	FindCandidatesFromMimeLists(context, top_default_app);

	std::unordered_map<std::string, std::vector<std::string>> mime_cache;

	bool cache_file_found = false;

	if (_use_mimeinfo_cache) {
		for (const auto& dir : desktop_paths) {
			std::string cache_path = dir + "/mimeinfo.cache";
			if (IsReadableFile(cache_path)) {
				ParseMimeinfoCache(cache_path, mime_cache);
				cache_file_found = true;
			}
		}
	}

	if (cache_file_found) {
		FindCandidatesFromCache(context, mime_cache);
	} else {
		FindCandidatesByFullScan(context);
	}

	std::vector<RankedCandidate> sorted_candidates;
	sorted_candidates.reserve(context.unique_candidates.size());
	for (auto& [key, ranked_candidate] : context.unique_candidates) {
		sorted_candidates.push_back(ranked_candidate);
	}
	std::sort(sorted_candidates.begin(), sorted_candidates.end());

	std::vector<CandidateInfo> result;
	result.reserve(sorted_candidates.size());
	for (const auto& ranked_candidate : sorted_candidates) {
		result.push_back(ConvertDesktopEntryToCandidateInfo(*ranked_candidate.entry));
	}

	return result;
}


std::wstring XDGBasedAppProvider::ConstructCommandLine(const CandidateInfo& candidate, const std::wstring& pathname)
{
	std::string desktop_file_name = StrWide2MB(candidate.id);

	auto it = _desktop_entry_cache.find(desktop_file_name);
	if (it == _desktop_entry_cache.end() || !it->second.has_value()) {
		return L"";
	}
	const DesktopEntry& desktop_entry = it->second.value();

	std::string narrow_exec = desktop_entry.exec;
	if (narrow_exec.empty()) return std::wstring();

	// Tokenize the Exec field respecting shell quoting rules (' and ").
	std::vector<Token> tokens = TokenizeDesktopExec(narrow_exec);
	if (tokens.empty()) {
		return std::wstring();
	}

	std::vector<std::string> args;
	args.reserve(tokens.size());
	bool has_field_code = false;

	// Check if any tokens contain XDG field codes (e.g., %f, %U).
	for (const Token& t : tokens) {
		std::string unescaped = UndoEscapes(t);
		if (unescaped.find('%') != std::string::npos) {
			has_field_code = true;
			break;
		}
	}

	std::string narrow_pathname = StrWide2MB(pathname);

	// Process each token, expanding field codes and handling escapes.
	for (const Token& t : tokens) {
		std::string unescaped = UndoEscapes(t);
		std::vector<std::string> expanded;
		if (!ExpandFieldCodes(desktop_entry, narrow_pathname, unescaped, expanded)) {
			return std::wstring(); // Return empty on invalid field code.
		}
		for (auto &a : expanded) args.push_back(std::move(a));
	}

	// According to the spec, if no field codes are present, the file path should be appended as the final argument.
	if (!has_field_code && !args.empty()) {
		args.push_back(narrow_pathname);
	}

	if (args.empty()) {
		return std::wstring();
	}

	// Build the final command line with proper shell escaping for each argument.
	std::string cmd;
	for (size_t i = 0; i < args.size(); ++i) {
		if (i) cmd.push_back(' ');
		cmd += EscapeArg(args[i]);
	}
	return StrMB2Wide(cmd);
}


std::vector<Field> XDGBasedAppProvider::GetCandidateDetails(const CandidateInfo& candidate)
{
	std::vector<Field> details;
	std::string desktop_file_name = StrWide2MB(candidate.id);

	auto it = _desktop_entry_cache.find(desktop_file_name);
	if (it == _desktop_entry_cache.end() || !it->second.has_value()) {
		return details;
	}
	const DesktopEntry& entry = it->second.value();

	details.push_back({m_GetMsg(MDesktopFile), StrMB2Wide(entry.desktop_file)});
	details.push_back({L"Name =", StrMB2Wide(entry.name)});
	if (!entry.generic_name.empty()) {
		details.push_back({L"GenericName =", StrMB2Wide(entry.generic_name)});
	}
	if (!entry.comment.empty()) {
		details.push_back({L"Comment =", StrMB2Wide(entry.comment)});
	}
	if (!entry.categories.empty()) {
		details.push_back({L"Categories =", StrMB2Wide(entry.categories)});
	}
	details.push_back({L"Exec =", StrMB2Wide(entry.exec)});
	if (!entry.try_exec.empty()) {
		details.push_back({L"TryExec =", StrMB2Wide(entry.try_exec)});
	}
	details.push_back({L"Terminal =", entry.terminal ? L"true" : L"false"});
	if (!entry.mimetype.empty()) {
		details.push_back({L"MimeType =", StrMB2Wide(entry.mimetype)});
	}
	return details;
}


std::wstring XDGBasedAppProvider::GetMimeType(const std::wstring& pathname)
{
	std::vector<std::string> mime_types;
	std::unordered_set<std::string> seen;

	// Helper to add a MIME type only if it's valid and not already present.
	auto add_unique = [&](std::string mime) {
		mime = Trim(mime);
		if (!mime.empty() && mime.find('/') != std::string::npos && seen.insert(mime).second) {
			mime_types.push_back(std::move(mime));
		}
	};

	std::string path_mb = StrWide2MB(pathname);

	// Determine MIME type using different methods in order of preference.
	add_unique(MimeTypeFromXdgMimeTool(path_mb));
	add_unique(MimeTypeFromFileTool(path_mb));
	add_unique(MimeTypeByExtension(path_mb));

	std::string result;
	for (auto& m : mime_types) { result += m; result += ';'; }
	return StrMB2Wide(result);
}


// ****************************** Private implementation ******************************

void XDGBasedAppProvider::AddOrUpdateCandidate(CandidateSearchContext& context, const DesktopEntry& entry, int rank)
{
	std::pair<std::string_view, std::string_view> unique_key(entry.name, entry.exec);

	auto [it, inserted] = context.unique_candidates.try_emplace(unique_key, RankedCandidate{&entry, rank});

	if (!inserted && rank > it->second.rank) {
		it->second.rank = rank;
		it->second.entry = &entry;
	}
}


void XDGBasedAppProvider::ProcessApp(CandidateSearchContext& context, const std::string& app_desktop_file, int rank)
{
	if (app_desktop_file.empty()) return;

	const auto& entry_opt = GetCachedDesktopEntry(app_desktop_file, context.desktop_paths, _desktop_entry_cache);
	if (!entry_opt) {
		return;
	}
	const DesktopEntry& entry = *entry_opt;

	if (_validate_try_exec && !entry.try_exec.empty() && !CheckExecutable(entry.try_exec)) {
		return;
	}

	if (_filter_by_show_in && !context.current_desktop_env.empty()) {
		if (!entry.only_show_in.empty()) {
			bool found = false;
			for (const auto& desktop : SplitString(entry.only_show_in, ';')) {
				if (desktop == context.current_desktop_env) { found = true; break; }
			}
			if (!found) return;
		}
		if (!entry.not_show_in.empty()) {
			bool found = false;
			for (const auto& desktop : SplitString(entry.not_show_in, ';')) {
				if (desktop == context.current_desktop_env) { found = true; break; }
			}
			if (found) return;
		}
	}
	AddOrUpdateCandidate(context, entry, rank);
}


void XDGBasedAppProvider::FindCandidatesFromMimeLists(CandidateSearchContext& context, const std::string& top_default_app)
{
	const int total_mimes = context.prioritized_mimes.size();
	for (int i = 0; i < total_mimes; ++i) {
		const auto& mime = context.prioritized_mimes[i];
		int mime_rank_base = (total_mimes - i) * 100;

		auto is_removed = [&](const std::string& app_desktop_file) {
			auto it = context.associations.removed.find(mime);
			return it != context.associations.removed.end() && it->second.count(app_desktop_file);
		};

		if (!top_default_app.empty() && !is_removed(top_default_app)) {
			ProcessApp(context, top_default_app, mime_rank_base + 10000);
		}

		auto it_defaults = context.associations.defaults.find(mime);
		if (it_defaults != context.associations.defaults.end()) {
			if (!is_removed(it_defaults->second)) {
				ProcessApp(context, it_defaults->second, mime_rank_base + 5000);
			}
		}

		auto it_added = context.associations.added.find(mime);
		if (it_added != context.associations.added.end()) {
			for (const auto& app : it_added->second) {
				if (!is_removed(app)) {
					ProcessApp(context, app, mime_rank_base + 2000);
				}
			}
		}
	}
}


void XDGBasedAppProvider::FindCandidatesFromCache(CandidateSearchContext& context, const std::unordered_map<std::string, std::vector<std::string>>& mime_cache)
{
	const int total_mimes = context.prioritized_mimes.size();
	std::map<std::string, int> app_best_rank;

	for (int i = 0; i < total_mimes; ++i) {
		const auto& mime = context.prioritized_mimes[i];
		auto it_cache = mime_cache.find(mime);
		if (it_cache != mime_cache.end()) {
			int mime_rank_base = (total_mimes - i) * 100;
			for (const auto& app_desktop_file : it_cache->second) {
				if (app_desktop_file.empty()) continue;
				auto it_removed = context.associations.removed.find(mime);
				if (it_removed != context.associations.removed.end() && it_removed->second.count(app_desktop_file)) continue;

				auto it_rank = app_best_rank.find(app_desktop_file);
				if (it_rank == app_best_rank.end() || mime_rank_base > it_rank->second) {
					app_best_rank[app_desktop_file] = mime_rank_base;
				}
			}
		}
	}

	for (const auto& [app_desktop_file, rank] : app_best_rank) {
		ProcessApp(context, app_desktop_file, rank);
	}
}


void XDGBasedAppProvider::FindCandidatesByFullScan(CandidateSearchContext& context)
{
	const int total_mimes = context.prioritized_mimes.size();
	for (const auto& dir : context.desktop_paths) {
		DIR* dp = opendir(dir.c_str());
		if (!dp) continue;
		struct dirent* ep;
		while ((ep = readdir(dp))) {
			std::string filename = ep->d_name;
			if (filename.size() <= 8 || filename.substr(filename.size() - 8) != ".desktop") {
				continue;
			}

			const auto& desktop_entry_opt = GetCachedDesktopEntry(filename, context.desktop_paths, _desktop_entry_cache);
			if (!desktop_entry_opt) continue;
			const DesktopEntry& entry = *desktop_entry_opt;

			std::vector<std::string> entry_mimes = SplitString(entry.mimetype, ';');
			int best_rank = -1;
			for (int i = 0; i < total_mimes; ++i) {
				const auto& mime = context.prioritized_mimes[i];
				bool matched = false;
				for (const auto& entry_mime : entry_mimes) {
					if (entry_mime.empty()) continue;
					if (mime == entry_mime) { matched = true; break; }
				}
				if (matched) {
					auto it_removed = context.associations.removed.find(mime);
					if (it_removed != context.associations.removed.end() && it_removed->second.count(filename)) continue;
					best_rank = (total_mimes - i) * 100;
					break;
				}
			}
			if (best_rank != -1) {
				ProcessApp(context, filename, best_rank);
			}
		}
		closedir(dp);
	}
}


std::string XDGBasedAppProvider::GetBaseName(const std::string& path)
{
	size_t pos = path.find_last_of('/');
	if (pos == std::string::npos) {
		return path;
	}
	return path.substr(pos + 1);
}


const std::optional<DesktopEntry>& XDGBasedAppProvider::GetCachedDesktopEntry(const std::string& desktop_file,
																		   const std::vector<std::string>& search_paths,
																		   std::map<std::string, std::optional<DesktopEntry>>& cache)
{
	if (auto it = cache.find(desktop_file); it != cache.end()) {
		return it->second;
	}

	// Search for the .desktop file in the prioritized list of directories.
	// The first one found is used, implementing the XDG override mechanism.
	for (const auto& base_dir : search_paths) {
		std::string full_path = base_dir + "/" + desktop_file;
		if (auto entry = ParseDesktopFile(full_path)) {
			return cache[desktop_file] = std::move(entry);
		}
	}
	// Cache a nullopt if the file is not found to avoid repeated searches.
	return cache[desktop_file] = std::nullopt;
}


// Parses mimeinfo.cache file format: [MIME Cache] section with mime/type=app1.desktop;app2.desktop;
void XDGBasedAppProvider::ParseMimeinfoCache(const std::string& path, std::unordered_map<std::string, std::vector<std::string>>& mime_cache)
{
	std::ifstream file(path);
	if (!file.is_open()) return;

	std::string line;
	bool in_cache_section = false;
	while (std::getline(file, line)) {
		line = Trim(line);
		if (line.empty() || line[0] == '#') continue;

		if (line == "[MIME Cache]") {
			in_cache_section = true;
			continue;
		}

		if (line[0] == '[') {
			in_cache_section = false;
			continue;
		}

		if (in_cache_section) {
			auto eq_pos = line.find('=');
			if (eq_pos == std::string::npos) continue;

			std::string mime = Trim(line.substr(0, eq_pos));
			std::string apps_str = Trim(line.substr(eq_pos + 1));

			auto apps = SplitString(apps_str, ';');
			if (!mime.empty() && !apps.empty()) {
				auto& existing = mime_cache[mime];
				// Append new associations; duplicates are handled later.
				existing.insert(existing.end(), apps.begin(), apps.end());
			}
		}
	}
}


// Safe environment variable access with optional default value
std::string XDGBasedAppProvider::GetEnv(const char* var, const char* default_val)
{
	const char* val = getenv(var);
	return val ? val : default_val;
}


// String splitting utility with trimming of individual tokens
std::vector<std::string> XDGBasedAppProvider::SplitString(const std::string& str, char delimiter)
{
	if (str.empty()) return {};

	std::vector<std::string> tokens;
	std::istringstream stream(str);
	std::string token;

	while (std::getline(stream, token, delimiter)) {
		if (auto trimmed = Trim(token); !trimmed.empty()) {
			tokens.push_back(std::move(trimmed));
		}
	}
	return tokens;
}


// Checks if executable exists and is runnable (absolute path or in $PATH)
bool XDGBasedAppProvider::CheckExecutable(const std::string& path)
{
	if (path.find('/') != std::string::npos) {
		// If it's a path, check directly.
		return access(path.c_str(), X_OK) == 0;
	}
	// If it's just a command, use `which` to check if it's in the PATH.
	std::string which_cmd = "which " + EscapePathForShell(path) + " >/dev/null 2>&1";
	return system(which_cmd.c_str()) == 0;
}


// Returns XDG-compliant search paths for .desktop files, ordered by priority.
std::vector<std::string> XDGBasedAppProvider::GetDesktopFileSearchPaths()
{
	std::vector<std::string> paths;
	std::unordered_set<std::string> seen_paths;

	auto add_path = [&](const std::string& p) {
		if (!p.empty() && IsValidDir(p) && seen_paths.insert(p).second) {
			paths.push_back(p);
		}
	};

	// User-specific data directory ($XDG_DATA_HOME or ~/.local/share) has the highest priority.
	std::string xdg_data_home = GetEnv("XDG_DATA_HOME", "");
	if (!xdg_data_home.empty()) {
		add_path(xdg_data_home + "/applications");
	} else {
		add_path(GetEnv("HOME", "") + "/.local/share/applications");
	}

	// System-wide data directories ($XDG_DATA_DIRS) have lower priority.
	std::string xdg_data_dirs = GetEnv("XDG_DATA_DIRS", "/usr/local/share:/usr/share");
	for (const auto& dir : SplitString(xdg_data_dirs, ':')) {
		if (dir.empty() || dir[0] != '/') continue;
		add_path(dir + "/applications");
	}

	// Additional paths for Flatpak and Snap applications.
	add_path(GetEnv("HOME", "") + "/.local/share/flatpak/exports/share/applications");
	add_path("/var/lib/flatpak/exports/share/applications");
	add_path("/var/lib/snapd/desktop/applications");

	return paths;
}


// Returns XDG-compliant search paths for mimeapps.list files, ordered by priority.
std::vector<std::string> XDGBasedAppProvider::GetMimeappsListSearchPaths()
{
	std::vector<std::string> paths;
	std::unordered_set<std::string> seen_paths;

	auto add_path = [&](const std::string& p) {
		if (p.empty() || !seen_paths.insert(p).second) return;
		if (IsReadableFile(p)) {
			paths.push_back(p);
		}
	};

	std::string home = GetEnv("HOME", "");
	// User config directory ($XDG_CONFIG_HOME or ~/.config) has the highest priority.
	std::string xdg_config_home = GetEnv("XDG_CONFIG_HOME", "");
	if (!xdg_config_home.empty() && xdg_config_home[0] == '/') {
		add_path(xdg_config_home + "/mimeapps.list");
	} else if (!home.empty()) {
		add_path(home + "/.config/mimeapps.list");
	}

	// System-wide config directories ($XDG_CONFIG_DIRS) have lower priority.
	std::string xdg_config_dirs = GetEnv("XDG_CONFIG_DIRS", "/etc/xdg");
	for (const auto& dir : SplitString(xdg_config_dirs, ':')) {
		if (dir.empty() || dir[0] != '/') continue;
		add_path(dir + "/mimeapps.list");
	}

	// Data directory mimeapps.list files (user and system) are legacy locations.
	std::string xdg_data_home = GetEnv("XDG_DATA_HOME", "");
	if (!xdg_data_home.empty() && xdg_data_home[0] == '/') {
		add_path(xdg_data_home + "/applications/mimeapps.list");
	} else if (!home.empty()) {
		add_path(home + "/.local/share/applications/mimeapps.list");
	}

	std::string xdg_data_dirs = GetEnv("XDG_DATA_DIRS", "/usr/local/share:/usr/share");
	for (const auto& dir : SplitString(xdg_data_dirs, ':')) {
		if (dir.empty() || dir[0] != '/') continue;
		add_path(dir + "/applications/mimeapps.list");
	}

	return paths;
}


// Parses single mimeapps.list file, extracting Default/Added/Removed associations
void XDGBasedAppProvider::ParseMimeappsList(const std::string& path, MimeAssociation& associations)
{
	std::ifstream file(path);
	if (!file.is_open()) return;

	std::string line, current_section;
	while (std::getline(file, line)) {
		line = Trim(line);
		if (line.empty() || line[0] == '#') continue;

		// Section headers: [Default Applications], [Added Associations], [Removed Associations]
		if (line[0] == '[' && line.back() == ']') {
			current_section = line;
			continue;
		}

		auto eq_pos = line.find('=');
		if (eq_pos == std::string::npos) continue;

		std::string key = Trim(line.substr(0, eq_pos)); // MIME type
		std::string value = Trim(line.substr(eq_pos + 1)); // Semicolon-separated .desktop files
		auto values = SplitString(value, ';');

		if (values.empty()) continue;

		// Process entries based on the current section.
		if (current_section == "[Default Applications]") {
			// Only use the first default if not already set, as higher priority files are parsed first.
			if (associations.defaults.find(key) == associations.defaults.end()) {
				associations.defaults[key] = values[0];
			}
		} else if (current_section == "[Added Associations]") {
			auto& vec = associations.added[key];
			vec.insert(vec.end(), values.begin(), values.end());
		} else if (current_section == "[Removed Associations]") {
			for(const auto& v : values) {
				if(!v.empty()) associations.removed[key].insert(v);
			}
		}
	}
}


// Combines multiple mimeapps.list files into a single association structure.
// The order of paths is important, from high priority to low.
XDGBasedAppProvider::MimeAssociation XDGBasedAppProvider::ParseMimeappsLists(const std::vector<std::string>& paths)
{
	MimeAssociation associations;
	for (const auto& path : paths) {
		ParseMimeappsList(path, associations);
	}
	return associations;
}


std::string XDGBasedAppProvider::EscapePathForShell(const std::string& path)
{
	std::string escaped_path;
	escaped_path.reserve(path.size() + 2);
	escaped_path += '\'';
	for (char c : path) {
		if (c == '\'') {
			escaped_path += "'\\''";
		} else {
			escaped_path += c;
		}
	}
	escaped_path += '\'';
	return escaped_path;
}


std::string XDGBasedAppProvider::Trim(std::string str)
{
	str.erase(str.begin(), std::find_if(str.begin(), str.end(), [](unsigned char ch) { return !std::isspace(ch); }));
	str.erase(std::find_if(str.rbegin(), str.rend(), [](unsigned char ch) { return !std::isspace(ch); }).base(), str.end());
	return str;
}


std::string XDGBasedAppProvider::RunCommandAndCaptureOutput(const std::string& cmd)
{
	std::string result;
	return POpen(result, cmd.c_str()) ? Trim(result) : "";
}


std::string XDGBasedAppProvider::GetDefaultApp(const std::string& mime_type)
{
	if (mime_type.empty()) return "";
	std::string escaped_mime = EscapePathForShell(mime_type);
	std::string cmd = "xdg-mime query default " + escaped_mime + " 2>/dev/null";
	return RunCommandAndCaptureOutput(cmd);
}


bool XDGBasedAppProvider::IsValidDir(const std::string& path)
{
	struct stat buffer;
	if (stat(path.c_str(), &buffer) != 0) return false;
	return S_ISDIR(buffer.st_mode);
}


bool XDGBasedAppProvider::IsReadableFile(const std::string &path) {
	struct stat st;
	if (stat(path.c_str(), &st) != 0) return false;
	if (!(S_ISREG(st.st_mode) || S_ISLNK(st.st_mode))) return false;
	int fd = open(path.c_str(), O_RDONLY);
	if (fd < 0) return false;
	close(fd);
	return true;
}


std::string XDGBasedAppProvider::MimeTypeFromXdgMimeTool(const std::string& pathname)
{
	std::string result;
	if (_use_xdg_mime_tool) {
		auto escaped_pathname = EscapePathForShell(pathname);
		result = RunCommandAndCaptureOutput("xdg-mime query filetype " + escaped_pathname + " 2>/dev/null");
	}
	return result;
}


std::string XDGBasedAppProvider::MimeTypeFromFileTool(const std::string& pathname)
{
	std::string result;
	if(_use_file_tool) {
		auto escaped_pathname = EscapePathForShell(pathname);
		result = RunCommandAndCaptureOutput("file -b --mime-type " + escaped_pathname + " 2>/dev/null");
	}
	return result;
}


std::string XDGBasedAppProvider::MimeTypeByExtension(const std::string& pathname)
{
	// A static map for common file extensions as a last-resort fallback.
	static const std::unordered_map<std::string, std::string> ext_to_type_map = {
		{".sh",  "text/x-shellscript"},
		{".bash","text/x-shellscript"},
		{".csh", "text/x-shellscript"},
		{".py",  "text/x-python"},
		{".pl",  "text/x-perl"},
		{".rb",  "text/x-ruby"},
		{".js",  "text/javascript"},
		{".html","text/html"},
		{".htm", "text/html"},
		{".xml", "application/xml"},
		{".pdf", "application/pdf"},
		{".exe", "application/x-ms-dos-executable"},
		{".bin", "application/x-executable"},
		{".elf", "application/x-executable"},
		{".txt", "text/plain"},
		{".conf","text/plain"},
		{".cfg", "text/plain"},
		{".md",  "text/markdown"},
		{".jpg", "image/jpeg"},
		{".jpeg","image/jpeg"},
		{".png", "image/png"},
		{".gif", "image/gif"},
		{".doc", "application/msword"},
		{".odt", "application/vnd.oasis.opendocument.text"},
		{".zip", "application/zip"},
		{".tar", "application/x-tar"},
		{".gz",  "application/gzip"}
	};

	std::string result;

	if (_use_extension_based_fallback) {
		auto dot_pos = pathname.rfind('.');
		if (dot_pos != std::string::npos) {
			std::string ext = pathname.substr(dot_pos);
			std::transform(ext.begin(), ext.end(), ext.begin(), [](unsigned char c){ return static_cast<char>(std::tolower(c)); });

			auto it = ext_to_type_map.find(ext);
			if (it != ext_to_type_map.end()) {
				result = it->second;
			}
		}
	}
	return result;
}


// Collects and prioritizes MIME types for a file using multiple detection methods.
std::vector<std::string> XDGBasedAppProvider::CollectAndPrioritizeMimeTypes(const std::string& pathname)
{
	std::vector<std::string> mime_types;
	std::unordered_set<std::string> seen;

	auto add_unique = [&](std::string mime) {
		mime = Trim(mime);
		if (!mime.empty() && mime.find('/') != std::string::npos && seen.insert(mime).second) {
			mime_types.push_back(std::move(mime));
		}
	};

	bool is_valid_dir = IsValidDir(pathname);
	bool is_readable_file = IsReadableFile(pathname);

	if (is_valid_dir || is_readable_file) {
		std::string escaped_path = EscapePathForShell(pathname);

		add_unique(MimeTypeFromXdgMimeTool(pathname));
		add_unique(MimeTypeFromFileTool(pathname));

		if (is_readable_file) {
			add_unique(MimeTypeByExtension(pathname));
		}

		// Add base types for structured MIME types (e.g., application/xml+svg -> application/xml).
		auto obtained_types = mime_types;

		for (const auto& mime : obtained_types) {
			size_t plus_pos = mime.find('+');
			if (plus_pos != std::string::npos) {
				add_unique(mime.substr(0, plus_pos));
			}
		}

		// Add generic fallback MIME types (e.g., image/jpeg -> image/*, text/plain for any text/*).
		for (const auto& mime : obtained_types) {
			if (mime.rfind("text/", 0) == 0) {
				add_unique("text/plain");
			}
			size_t slash_pos = mime.find('/');
			if (slash_pos != std::string::npos) {
				add_unique(mime.substr(0, slash_pos) + "/*");
			}
		}

		// Ultimate fallback for any binary data.
		if (is_readable_file) {
			add_unique("application/octet-stream");
		}
	}

	return mime_types;
}


bool XDGBasedAppProvider::IsDesktopWhitespace(char c)
{
	return c == ' ' || c == '\t' || c == '\n' || c == '\r' || c == '\f' || c == '\v';
}


// Tokenizes the Exec= line according to the Desktop Entry Specification's rules
// for quoting and escaping. It is not a full shell parser.
std::vector<Token> XDGBasedAppProvider::TokenizeDesktopExec(const std::string& str)
{
	std::vector<Token> tokens;
	std::string cur;
	bool in_double_quotes = false;
	bool in_single_quotes = false;
	bool cur_quoted = false;
	bool cur_single_quoted = false;
	bool prev_backslash = false;

	for (size_t i = 0; i < str.size(); ++i) {
		char c = str[i];

		if (prev_backslash) {
			cur.push_back('\\');
			cur.push_back(c);
			prev_backslash = false;
			continue;
		}

		if (c == '\\') {
			prev_backslash = true;
			continue;
		}

		if (c == '"' && !in_single_quotes) {
			in_double_quotes = !in_double_quotes;
			cur_quoted = true;
			continue;
		}

		if (c == '\'' && !in_double_quotes) {
			in_single_quotes = !in_single_quotes;
			cur_single_quoted = true;
			continue;
		}

		if (!in_double_quotes && !in_single_quotes && IsDesktopWhitespace(c)) {
			if (!cur.empty() || cur_quoted || cur_single_quoted) {
				tokens.push_back({cur, cur_quoted, cur_single_quoted});
				cur.clear();
				cur_quoted = false;
				cur_single_quoted = false;
			}
			continue;
		}

		cur.push_back(c);
	}

	if (prev_backslash) {
		cur.push_back('\\');
	}

	if (!cur.empty() || cur_quoted || cur_single_quoted) {
		// Detect unclosed quotes as a parsing error.
		if ((cur_quoted && in_double_quotes) || (cur_single_quoted && in_single_quotes)) {
			return {};
		}
		tokens.push_back({cur, cur_quoted, cur_single_quoted});
	}

	return tokens;
}


// Processes escape sequences (\n, \t, \\, etc.) in a token's text.
std::string XDGBasedAppProvider::UndoEscapes(const Token& token)
{
	std::string result;
	result.reserve(token.text.size());

	for (size_t i = 0; i < token.text.size(); ++i) {
		if (token.text[i] == '\\' && i + 1 < token.text.size()) {
			char next = token.text[i + 1];
			switch (next) {
			case '"': case '\'': case '`': case '$': case '\\':
				result.push_back(next);
				break;
			case 'n':
				result.push_back('\n');
				break;
			case 't':
				result.push_back('\t');
				break;
			case 'r':
				result.push_back('\r');
				break;
			default: // Unrecognized escape sequences are kept literally.
				result.push_back('\\');
				result.push_back(next);
				break;
			}
			++i;
		} else {
			result.push_back(token.text[i]);
		}
	}

	return result;
}


// Expands XDG Desktop Entry field codes (%f, %F, %u, %U, %c, etc.).
bool XDGBasedAppProvider::ExpandFieldCodes(const DesktopEntry& candidate,
										const std::string& pathname,
										const std::string& unescaped,
										std::vector<std::string>& out_args)
{
	std::string cur;
	for (size_t i = 0; i < unescaped.size(); ++i) {
		char c = unescaped[i];
		if (c == '%') {
			if (i + 1 >= unescaped.size()) return false; // Dangling % is an error.
			char code = unescaped[i + 1];
			++i;
			switch (code) {
			case 'f': case 'F': // A single file path. %F is for multiple files, but we handle one.
			case 'u': case 'U': // A single URI. %U is for multiple URIs.
				cur += pathname; // For our purposes, path and URI are treated the same.
				break;
			case 'c': // The application's name (Name= key).
				cur += candidate.name;
				break;
			case '%': // A literal % character.
				cur.push_back('%');
				break;
			// Deprecated or unused field codes are silently ignored as per the spec.
			case 'n': case 'd': case 'D': case 't': case 'T': case 'v': case 'm':
			case 'k': case 'i':
				break;
			default:
				return false; // Any other field code is invalid.
			}
		} else {
			cur.push_back(c);
		}
	}
	if (!cur.empty()) out_args.push_back(cur);
	return true;
}


// Escapes a single command-line argument for safe execution by the shell.
std::string XDGBasedAppProvider::EscapeArg(const std::string& arg)
{
	std::string out;
	out.push_back('"');
	for (char c : arg) {
		// Escape characters that have special meaning inside double quotes in shell.
		if (c == '\\' || c == '"' || c == '$' || c == '`') {
			out.push_back('\\');
			out.push_back(c);
		} else {
			out.push_back(c);
		}
	}
	out.push_back('"');
	return out;
}


// Retrieves a localized string value (e.g., Name[en_US]) from a map of key-value pairs,
// following the locale resolution logic based on LC_*, LANG environment variables.
std::string XDGBasedAppProvider::GetLocalizedValue(const std::unordered_map<std::string, std::string>& values,
												const std::string& key)
{
	const char* env_vars[] = {"LC_ALL", "LC_MESSAGES", "LANG"};
	for (const auto* var : env_vars) {
		const char* value = getenv(var);
		if (value && *value && std::strlen(value) >= 2) {
			std::string locale(value);
			// Remove character set suffix (e.g., en_US.UTF-8 -> en_US).
			size_t dot_pos = locale.find('.');
			if (dot_pos != std::string::npos) {
				locale = locale.substr(0, dot_pos);
			}
			if (!locale.empty()) {
				// Try full locale (e.g., Name[en_US]).
				auto it = values.find(key + "[" + locale + "]");
				if (it != values.end()) return it->second;
				// Try language part only (e.g., Name[en]).
				size_t underscore_pos = locale.find('_');
				if (underscore_pos != std::string::npos) {
					std::string lang_only = locale.substr(0, underscore_pos);
					it = values.find(key + "[" + lang_only + "]");
					if (it != values.end()) return it->second;
				}
			}
		}
	}
	// Fallback to the unlocalized key (e.g., Name=).
	auto it = values.find(key);
	return (it != values.end()) ? it->second : "";
}


// Parses a .desktop file according to the Desktop Entry Specification.
std::optional<DesktopEntry> XDGBasedAppProvider::ParseDesktopFile(const std::string& path)
{
	std::ifstream file(path);
	if (!file.is_open()) {
		return std::nullopt;
	}

	std::string line;
	bool in_main_section = false;
	DesktopEntry desktop_entry;
	desktop_entry.desktop_file = path;

	std::unordered_map<std::string, std::string> entries;

	// Parse key-value pairs from the [Desktop Entry] section only.
	while (std::getline(file, line)) {
		line = Trim(line);
		if (line.empty() || line[0] == '#') continue;
		if (line == "[Desktop Entry]") {
			in_main_section = true;
			continue;
		}
		if (line[0] == '[') {
			in_main_section = false;
			continue;
		}
		if (in_main_section) {
			auto eq_pos = line.find('=');
			if (eq_pos == std::string::npos) continue;
			std::string key = Trim(line.substr(0, eq_pos));
			std::string value = Trim(line.substr(eq_pos + 1));
			entries[key] = value;
		}
	}

	// Validate required fields and application type.
	bool is_application = entries.count("Type") && entries.at("Type") == "Application";
	bool hidden = entries.count("Hidden") && entries.at("Hidden") == "true";

	if (hidden) {
		return std::nullopt;
	}

	// An application must have Type=Application and a non-empty Exec field.
	if (!is_application || !entries.count("Exec") || entries.at("Exec").empty()) {
		return std::nullopt;
	}
	desktop_entry.exec = Trim(entries.at("Exec"));
	if (desktop_entry.exec.empty()) {
		return std::nullopt;
	}

	// The Name field is required for a valid desktop entry.
	desktop_entry.name = GetLocalizedValue(entries, "Name");
	if (desktop_entry.name.empty()) {
		return std::nullopt;
	}

	// Extract optional fields with localization support.
	desktop_entry.generic_name = GetLocalizedValue(entries, "GenericName");
	desktop_entry.comment = GetLocalizedValue(entries, "Comment");
	if (entries.count("Categories")) desktop_entry.categories = entries.at("Categories");
	if (entries.count("TryExec")) desktop_entry.try_exec = entries.at("TryExec");
	if (entries.count("Terminal")) desktop_entry.terminal = (entries.at("Terminal") == "true");
	if (entries.count("MimeType")) desktop_entry.mimetype = entries.at("MimeType");
	if (entries.count("OnlyShowIn")) desktop_entry.only_show_in = entries.at("OnlyShowIn");
	if (entries.count("NotShowIn")) desktop_entry.not_show_in = entries.at("NotShowIn");
	// if (entries.count("DBusActivatable")) desktop_entry.dbus_activatable = (entries.at("DBusActivatable") == "true");

	return desktop_entry;
}


CandidateInfo XDGBasedAppProvider::ConvertDesktopEntryToCandidateInfo(const DesktopEntry& desktop_entry)
{
	CandidateInfo candidate;
	candidate.terminal = desktop_entry.terminal;
	candidate.name = StrMB2Wide(desktop_entry.name);

	// The ID is the basename of the .desktop file, used for lookups and overrides.
	candidate.id = StrMB2Wide(GetBaseName(desktop_entry.desktop_file));
	return candidate;
}

#endif
