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



// ****************************** Public API ******************************


XDGBasedAppProvider::XDGBasedAppProvider(TMsgGetter msg_getter) : AppProvider(std::move(msg_getter))
{
	_platform_settings_definitions = {
		{ "UseXdgMimeTool", MUseXdgMimeTool, &XDGBasedAppProvider::_use_xdg_mime_tool, true },
		{ "UseFileTool", MUseFileTool, &XDGBasedAppProvider::_use_file_tool, true },
		{ "UseExtensionBasedFallback", MUseExtensionBasedFallback, &XDGBasedAppProvider::_use_extension_based_fallback, true },
		{ "LoadMimeTypeAliases", MLoadMimeTypeAliases, &XDGBasedAppProvider::_load_mimetype_aliases, true },
		{ "UseMimeinfoCache", MUseMimeinfoCache, &XDGBasedAppProvider::_use_mimeinfo_cache, true },
		{ "FilterByShowIn", MFilterByShowIn, &XDGBasedAppProvider::_filter_by_show_in, false },
		{ "ValidateTryExec", MValidateTryExec, &XDGBasedAppProvider::_validate_try_exec, false }
	};

	// Pre-calculate the lookup map for SetPlatformSettings.
	for (const auto& def : _platform_settings_definitions) {
		_key_to_member_map[StrMB2Wide(def.key)] = def.member_variable;
	}
}


// Loads all settings from the INI file based on their definitions.
void XDGBasedAppProvider::LoadPlatformSettings()
{
	KeyFileReadSection key_file_reader(INI_LOCATION_LINUX, INI_SECTION_LINUX);
	for (const auto& def : _platform_settings_definitions) {
		this->*(def.member_variable) = key_file_reader.GetInt(def.key.c_str(), def.default_value) != 0;
	}
}


// Saves all settings to the INI file based on their definitions.
void XDGBasedAppProvider::SavePlatformSettings()
{
	KeyFileHelper key_file_helper(INI_LOCATION_LINUX);
	for (const auto& def : _platform_settings_definitions) {
		key_file_helper.SetInt(INI_SECTION_LINUX, def.key.c_str(), this->*(def.member_variable));
	}
	key_file_helper.Save();
}


// Constructs a vector of UI-ready ProviderSetting objects from current values.
std::vector<ProviderSetting> XDGBasedAppProvider::GetPlatformSettings()
{
	std::vector<ProviderSetting> settings;
	settings.reserve(_platform_settings_definitions.size());
	for (const auto& def : _platform_settings_definitions) {
		settings.push_back({
			StrMB2Wide(def.key),
			m_GetMsg(def.display_name_id),
			this->*(def.member_variable)
		});
	}
	return settings;
}


// Updates internal setting values from a vector of ProviderSetting objects
// using the pre-calculated lookup map for O(log N) performance.
void XDGBasedAppProvider::SetPlatformSettings(const std::vector<ProviderSetting>& settings)
{
	for (const auto& s : settings) {
		auto it = _key_to_member_map.find(s.internal_key);
		if (it != _key_to_member_map.end()) {
			this->*(it->second) = s.value;
		}
	}
}


// Main orchestration method for finding and ranking aapplication candidates based on XDG standards
std::vector<CandidateInfo> XDGBasedAppProvider::GetAppCandidates(const std::wstring& pathname)
{
	// 1. Prepare context: determine MIME types, find system paths, parse associations.
	_desktop_entry_cache.clear();
	_last_candidates_source_info.clear();
	auto prioritized_mimes = CollectAndPrioritizeMimeTypes(StrWide2MB(pathname));
	auto desktop_paths = GetDesktopFileSearchPaths();
	auto mimeapps_paths = GetMimeappsListSearchPaths();
	auto associations = ParseMimeappsLists(mimeapps_paths);
	std::string current_desktop_env = _filter_by_show_in ? GetEnv("XDG_CURRENT_DESKTOP") : "";
	CandidateSearchContext context(prioritized_mimes, associations, desktop_paths, current_desktop_env);

	// 2. Handle the global default app (from xdg-mime) separately and with the highest priority.
	std::string global_default_app = GetDefaultApp(prioritized_mimes.empty() ? "" : prioritized_mimes[0]);
	if (!global_default_app.empty()) {
		const auto& mime_for_default = prioritized_mimes[0];
		if (!IsAssociationRemoved(context.associations,mime_for_default,global_default_app)) {
			// Rank is based on the highest possible specificity (index 0).
			int rank = (prioritized_mimes.size() - 0) * Ranking::SPECIFICITY_MULTIPLIER + Ranking::SOURCE_RANK_GLOBAL_DEFAULT;
			ValidateAndRegisterCandidate(context, global_default_app, rank,  "xdg-mime query default " + mime_for_default);
		}
	}

	// 3. Find candidates using high-priority sources (mimeapps.list).
	FindCandidatesFromMimeLists(context);

	// 4. Find candidates using mimeinfo.cache (if enabled and found) with fallback to full scan of all .desktop files.
	std::unordered_map<std::string, std::vector<MimeAssociation::AssociationSource>> mime_cache;
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

	// 5. Sort all found candidates by their calculated rank and then by name.
	std::vector<RankedCandidate> sorted_candidates;
	sorted_candidates.reserve(context.unique_candidates.size());
	for (const auto& [key, ranked_candidate] : context.unique_candidates) {
		sorted_candidates.push_back(ranked_candidate);
	}
	std::sort(sorted_candidates.begin(), sorted_candidates.end());

	// 6. Convert the ranked internal structs to the public CandidateInfo format
	//    and populate the source info cache.
	std::vector<CandidateInfo> result;
	result.reserve(sorted_candidates.size());
	for (const auto& ranked_candidate : sorted_candidates) {
		CandidateInfo ci = ConvertDesktopEntryToCandidateInfo(*ranked_candidate.entry);
		_last_candidates_source_info[ci.id] = ranked_candidate.source_info;
		result.push_back(ci);
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

	std::string exec_mb = desktop_entry.exec;
	if (exec_mb.empty()) return L"";

	std::vector<std::string> tokens = TokenizeExecString(exec_mb);
	if (tokens.empty()) {
		return L"";
	}

	std::vector<std::string> args;
	args.reserve(tokens.size());
	bool has_field_code = false;

	for (const std::string& t : tokens) {
		if (t.find('%') != std::string::npos) {
			has_field_code = true;
			break;
		}
	}

	std::string pathname_mb = StrWide2MB(pathname);

	for (const std::string& token : tokens) {
		std::vector<std::string> expanded;
		if (!ExpandFieldCodes(desktop_entry, pathname_mb, token, expanded)) {
			return L""; // error in field code (e.g. %)
		}
		for (auto& a : expanded) {
			args.push_back(std::move(a));
		}
	}

	// According to the spec, if no field codes are present, the file path must be appended.
	if (!has_field_code && !args.empty()) {
		args.push_back(pathname_mb);
	}

	if (args.empty()) {
		return L"";
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

	auto it_source = _last_candidates_source_info.find(candidate.id);
	if (it_source != _last_candidates_source_info.end()) {
		details.push_back({m_GetMsg(MSource), StrMB2Wide(it_source->second)});
	}

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
	if (result.empty()) {
		result = "(none)";
	}
	return StrMB2Wide(result);
}



// ******************** Searching and ranking candidates logic ********************


// Finds candidates from the parsed mimeapps.list files. This is a high-priority source.
void XDGBasedAppProvider::FindCandidatesFromMimeLists(CandidateSearchContext& context)
{
	const int total_mimes = context.prioritized_mimes.size();
	for (int i = 0; i < total_mimes; ++i) {
		const auto& mime = context.prioritized_mimes[i];
		// Rank based on MIME type specificity. The more specific, the higher the rank.
		int mime_specificity_rank = (total_mimes - i);

		// 1. Default application from mimeapps.list: high source rank.
		auto it_defaults = context.associations.defaults.find(mime);
		if (it_defaults != context.associations.defaults.end()) {
			const auto& default_app = it_defaults->second;
			if (!IsAssociationRemoved(context.associations, mime, default_app.desktop_file)) {
				int rank = mime_specificity_rank * Ranking::SPECIFICITY_MULTIPLIER + Ranking::SOURCE_RANK_MIMEAPPS_DEFAULT;
				std::string source_info = default_app.source_path + StrWide2MB(m_GetMsg(MIn)) + " [Default Applications] " + StrWide2MB(m_GetMsg(MFor)) + mime;
				ValidateAndRegisterCandidate(context, default_app.desktop_file, rank, source_info);
			}
		}
		// 2. Added associations from mimeapps.list: medium source rank.
		auto it_added = context.associations.added.find(mime);
		if (it_added != context.associations.added.end()) {
			for (const auto& app_assoc : it_added->second) {
				if (!IsAssociationRemoved(context.associations, mime, app_assoc.desktop_file)) {
					int rank = mime_specificity_rank * Ranking::SPECIFICITY_MULTIPLIER + Ranking::SOURCE_RANK_MIMEAPPS_ADDED;
					std::string source_info = app_assoc.source_path + StrWide2MB(m_GetMsg(MIn)) + "[Added Associations]" + StrWide2MB(m_GetMsg(MFor)) + mime;
					ValidateAndRegisterCandidate(context, app_assoc.desktop_file, rank, source_info);
				}
			}
		}
	}
}


// Finds candidates from the pre-generated mimeinfo.cache file for performance.
// The logic is split into two passes:
// 1. Iterate through all MIME types to find the best possible rank for each application.
// 2. Register each application using only its highest-found rank.
// This prevents a high-rank association (e.g., for 'text/x-c++src') from being
// overwritten by a lower-rank one found for the same app (e.g., for 'text/plain').
void XDGBasedAppProvider::FindCandidatesFromCache(CandidateSearchContext& context, const std::unordered_map<std::string, std::vector<MimeAssociation::AssociationSource>>& mime_cache)
{
	const int total_mimes = context.prioritized_mimes.size();
	std::map<std::string, std::pair<int, std::string>> app_best_rank_and_source;

	// First, find the best possible rank for each application across all matching MIME types.
	// This prevents an app from getting a low rank for a generic MIME type (e.g., text/plain)
	// if it has already been matched with a high-rank specific MIME type.
	for (int i = 0; i < total_mimes; ++i) {
		const auto& mime = context.prioritized_mimes[i];
		auto it_cache = mime_cache.find(mime);
		if (it_cache != mime_cache.end()) {
			// Calculate rank using the tiered formula.
			int rank = (total_mimes - i) * Ranking::SPECIFICITY_MULTIPLIER + Ranking::SOURCE_RANK_CACHE_OR_SCAN;
			for (const auto& assoc : it_cache->second) {
				const auto& app_desktop_file = assoc.desktop_file;
				if (app_desktop_file.empty()) continue;

				if (IsAssociationRemoved(context.associations, mime, app_desktop_file)) continue;

				std::string source_info = assoc.source_path + StrWide2MB(m_GetMsg(MFor)) + mime;
				auto [it, inserted] = app_best_rank_and_source.try_emplace(app_desktop_file, std::make_pair(rank, source_info));
				if (!inserted && rank > it->second.first) {
					it->second.first = rank;
					it->second.second = source_info;
				}
			}
		}
	}

	// Now, process each application with its best-found rank.
	for (const auto& [app_desktop_file, rank_and_source] : app_best_rank_and_source) {
		ValidateAndRegisterCandidate(context, app_desktop_file, rank_and_source.first, rank_and_source.second);
	}
}


// A slow fallback method that manually iterates through all .desktop file and checks their MimeType= line.
void XDGBasedAppProvider::FindCandidatesByFullScan(CandidateSearchContext& context)
{
	const int total_mimes = context.prioritized_mimes.size();
	for (const auto& dir : context.desktop_paths) {
		DIR* dir_stream = opendir(dir.c_str());
		if (!dir_stream) continue;
		struct dirent* dir_entry;
		while ((dir_entry = readdir(dir_stream))) {
			std::string filename = dir_entry->d_name;
			if (filename.size() <= 8 || filename.substr(filename.size() - 8) != ".desktop") {
				continue;
			}

			const auto& desktop_entry_opt = GetCachedDesktopEntry(filename, context.desktop_paths, _desktop_entry_cache);
			if (!desktop_entry_opt) continue;
			const DesktopEntry& entry = *desktop_entry_opt;

			std::vector<std::string> entry_mimes = SplitString(entry.mimetype, ';');
			int best_rank = -1;
			std::string matched_mime;
			for (int i = 0; i < total_mimes; ++i) {
				const auto& mime = context.prioritized_mimes[i];
				bool matched = false;
				for (const auto& entry_mime : entry_mimes) {
					if (entry_mime.empty()) continue;
					if (mime == entry_mime) { matched = true; break; }
				}
				if (matched) {
					// Check if this association is explicitly removed in mimeapps.list
					if (IsAssociationRemoved(context.associations, mime, filename)) continue;

					// Calculate rank using the tiered formula.
					best_rank = (total_mimes - i) * Ranking::SPECIFICITY_MULTIPLIER + Ranking::SOURCE_RANK_CACHE_OR_SCAN;
					matched_mime = mime;
					break; // Found the best rank for this app, no need to check other mimes.
				}
			}
			if (best_rank != -1) {
				auto source_info = StrWide2MB(m_GetMsg(MFullScanFor)) + matched_mime;
				ValidateAndRegisterCandidate(context, filename, best_rank, source_info);
			}
		}
		closedir(dir_stream);
	}
}


// Processes a single application candidate: validates it, filters it, and adds it to the context.
void XDGBasedAppProvider::ValidateAndRegisterCandidate(CandidateSearchContext& context, const std::string& app_desktop_file, int rank, const std::string& source_info)
{
	if (app_desktop_file.empty()) return;

	// Retrieve the full DesktopEntry, either from cache or by parsing it.
	const auto& entry_opt = GetCachedDesktopEntry(app_desktop_file, context.desktop_paths, _desktop_entry_cache);
	if (!entry_opt) {
		return;
	}
	const DesktopEntry& entry = *entry_opt;

	// Optionally validate the TryExec key to ensure the executable exists.
	if (_validate_try_exec && !entry.try_exec.empty() && !CheckExecutable(entry.try_exec)) {
		return;
	}

	// Optionally filter applications based on the current desktop environment
	// using the OnlyShowIn and NotShowIn keys.
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
	AddOrUpdateCandidate(context, entry, rank, source_info);
}


// Adds a new candidate to the results map or updates its rank if a better one is found.
void XDGBasedAppProvider::AddOrUpdateCandidate(CandidateSearchContext& context, const DesktopEntry& entry, int rank, const std::string& source_info)
{
	// A unique key to identify an application. Some apps (e.g., text editors) might have
	// multiple .desktop files (e.g., for different profiles) that point to the same
	// executable but have different names. This ensures they are treated as distinct entries.
	AppUniqueKey unique_key{entry.name, entry.exec};

	auto [it, inserted] = context.unique_candidates.try_emplace(unique_key, RankedCandidate{&entry, rank, source_info});

	// If the candidate already exists, update it only if the new rank is higher.
	if (!inserted && rank > it->second.rank) {
		it->second.rank = rank;
		it->second.entry = &entry;
		it->second.source_info = source_info;
	}
}


bool XDGBasedAppProvider::IsAssociationRemoved(const MimeAssociation& associations, const std::string& mime_type, const std::string& app_desktop_file)
{
	// 1. Check for an exact match (e.g., "image/jpeg")
	auto it_exact = associations.removed.find(mime_type);
	if (it_exact != associations.removed.end() && it_exact->second.count(app_desktop_file)) {
		return true;
	}

	// 2. Check for a wildcard match (e.g., "image/*")
	const size_t slash_pos = mime_type.find('/');
	if (slash_pos != std::string::npos) {
		const std::string wildcard_mime = mime_type.substr(0, slash_pos) + "/*";
		auto it_wildcard = associations.removed.find(wildcard_mime);
		if (it_wildcard != associations.removed.end() && it_wildcard->second.count(app_desktop_file)) {
			return true;
		}
	}

	return false;
}


// ****************************** MIME types detection ******************************


// Collects and prioritizes MIME types for a file using multiple detection methods.
std::vector<std::string> XDGBasedAppProvider::CollectAndPrioritizeMimeTypes(const std::string& pathname)
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

	bool is_valid_dir = IsValidDir(pathname);
	bool is_readable_file = IsReadableFile(pathname);

	if (is_valid_dir || is_readable_file) {

		// Primary methods for MIME detection.
		add_unique(MimeTypeFromXdgMimeTool(pathname));
		add_unique(MimeTypeFromFileTool(pathname));

		if (is_readable_file) {
			add_unique(MimeTypeByExtension(pathname));
		}

		if (_load_mimetype_aliases) {
			// Load all alias definitions from XDG paths into a forward map (alias -> canonical).
			auto alias_to_canonical_map = LoadMimeAliases();

			// Create a reverse map (canonical -> list_of_aliases) for reverse lookups.
			std::unordered_map<std::string, std::vector<std::string>> canonical_to_aliases_map;
			for (const auto& [alias, canonical] : alias_to_canonical_map) {
				canonical_to_aliases_map[canonical].push_back(alias);
			}

			// Iterate over a copy of initially found types to safely modify the original `mime_types` vector.
			auto initially_found_types = mime_types;
			for (const auto& mime : initially_found_types) {
				// --- Block 1: Smart reverse lookup (canonical -> alias) ---
				// Find aliases for the current canonical MIME type, but filter them to avoid incorrect associations (e.g., image/* -> text/*).
				auto it_aliases = canonical_to_aliases_map.find(mime);
				if (it_aliases != canonical_to_aliases_map.end()) {
					size_t canonical_slash_pos = mime.find('/');
					// Proceed only if the canonical MIME type has a valid format (e.g., "type/subtype").
					if (canonical_slash_pos != std::string::npos) {
						// Use string_view to avoid memory allocations from substr().
						std::string_view canonical_major_type(mime.data(), canonical_slash_pos);

						for (const auto& alias : it_aliases->second) {
							size_t alias_slash_pos = alias.find('/');
							if (alias_slash_pos != std::string::npos) {
								std::string_view alias_major_type(alias.data(), alias_slash_pos);

								// Add the alias ONLY if its major type matches the canonical one.
								// This prevents adding, for example, "text/ico" for "image/vnd.microsoft.icon",
								// but allows adding "image/x-icon".
								if (alias_major_type == canonical_major_type) {
									add_unique(alias);
								}
							}
						}
					}
				}

				// --- Block 2: Standard forward lookup (alias -> canonical) ---
				// If the current MIME type is an alias, add its canonical type. This is the standard behavior.
				auto it_canonical = alias_to_canonical_map.find(mime);
				if (it_canonical != alias_to_canonical_map.end()) {
					add_unique(it_canonical->second);
				}
			}
		}

		// Add base types for structured MIME types (e.g., image/svg+xml -> application/xml).
		// This allows an app associated with a generic structured syntax (like XML or ZIP)
		// to be suggested as a candidate for a more specific file type.

		static const std::map<std::string, std::string> suffix_to_base_mime = {
			{"xml", "application/xml"},
			{"zip", "application/zip"},
			{"json", "application/json"},
			{"gzip", "application/gzip"}
		};

		auto obtained_types = mime_types;
		for (const auto& mime : obtained_types) {
			size_t plus_pos = mime.find('+');
			if (plus_pos != std::string::npos && plus_pos < mime.length() - 1) {
				std::string suffix = mime.substr(plus_pos + 1);
				auto it = suffix_to_base_mime.find(suffix);
				if (it != suffix_to_base_mime.end()) {
					add_unique(it->second);
				}
			}
		}

		// Add generic fallback MIME types (e.g., image/jpeg -> image/*).
		// Also adds text/plain for any text/* type, as most text editors can handle it.
		for (const auto& mime : obtained_types) {
			if (mime.rfind("text/", 0) == 0) {
				add_unique("text/plain");
			}
			size_t slash_pos = mime.find('/');
			if (slash_pos != std::string::npos) {
				add_unique(mime.substr(0, slash_pos) + "/*");
			}
		}

		// Ultimate fallback for any binary data, allowing generic binary editors to be suggested.
		if (is_readable_file) {
			add_unique("application/octet-stream");
		}
	}

	return mime_types;
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
	// This is not comprehensive but covers many common cases if other tools fail.
	static const std::unordered_map<std::string, std::string> s_ext_to_type_map = {

		// Shell / scripts / source code

		{".sh",    "application/x-shellscript"},
		{".bash",  "application/x-shellscript"},
		{".csh",   "application/x-csh"},
		{".zsh",   "application/x-shellscript"},
		{".ps1",   "application/x-powershell"},
		{".py",    "text/x-python"},
		{".pyw",   "text/x-python"},
		{".pl",    "text/x-perl"},
		{".pm",    "text/x-perl"},
		{".rb",    "text/x-ruby"},
		{".php",   "application/x-php"},
		{".phps",  "application/x-php"},
		{".js",    "application/javascript"},
		{".mjs",   "application/javascript"},
		{".java",  "text/x-java-source"},
		{".c",     "text/x-csrc"},
		{".h",     "text/x-chdr"},
		{".cpp",   "text/x-c++src"},
		{".cc",    "text/x-c++src"},
		{".cxx",   "text/x-c++src"},
		{".hpp",   "text/x-c++hdr"},
		{".go",    "text/x-go"},
		{".rs",    "text/rust"},
		{".swift", "text/x-swift"},

		// Plain text / markup / data

		{".txt",   "text/plain"},
		{".md",    "text/markdown"},
		{".markdown","text/markdown"},
		{".rtf",   "application/rtf"},
		{".tex",   "application/x-tex"},
		{".csv",   "text/csv"},
		{".tsv",   "text/tab-separated-values"},
		{".log",   "text/plain"},
		{".json",  "application/json"},
		{".yaml",  "text/yaml"},
		{".yml",   "text/yaml"},
		{".xml",   "application/xml"},
		{".html",  "text/html"},
		{".htm",   "text/html"},
		{".xhtml", "application/xhtml+xml"},
		{".ics",   "text/calendar"},

		// Office / documents

		{".pdf",   "application/pdf"},
		{".doc",   "application/msword"},
		{".docx",  "application/vnd.openxmlformats-officedocument.wordprocessingml.document"},
		{".xls",   "application/vnd.ms-excel"},
		{".xlsx",  "application/vnd.openxmlformats-officedocument.spreadsheetml.sheet"},
		{".ppt",   "application/vnd.ms-powerpoint"},
		{".pptx",  "application/vnd.openxmlformats-officedocument.presentationml.presentation"},
		{".odt",   "application/vnd.oasis.opendocument.text"},
		{".ods",   "application/vnd.oasis.opendocument.spreadsheet"},
		{".odp",   "application/vnd.oasis.opendocument.presentation"},
		{".epub",  "application/epub+zip"},
		{".rtf",   "application/rtf"},

		// Images

		{".jpg",   "image/jpeg"},
		{".jpeg",  "image/jpeg"},
		{".jpe",   "image/jpeg"},
		{".png",   "image/png"},
		{".gif",   "image/gif"},
		{".webp",  "image/webp"},
		{".svg",   "image/svg+xml"},
		{".ico",   "image/vnd.microsoft.icon"},
		{".bmp",   "image/bmp"},
		{".tif",   "image/tiff"},
		{".tiff",  "image/tiff"},
		{".heic",  "image/heic"},
		{".avif",  "image/avif"},
		{".apng",  "image/apng"},

		// Audio

		{".mp3",   "audio/mpeg"},
		{".m4a",   "audio/mp4"},
		{".aac",   "audio/aac"},
		{".ogg",   "audio/ogg"},
		{".oga",   "audio/ogg"},
		{".opus",  "audio/opus"},
		{".wav",   "audio/x-wav"},
		{".flac",  "audio/flac"},
		{".mid",   "audio/midi"},
		{".midi",  "audio/midi"},
		{".weba",  "audio/webm"},

		// Video

		{".mp4",   "video/mp4"},
		{".m4v",   "video/mp4"},
		{".mov",   "video/quicktime"},
		{".mkv",   "video/x-matroska"},
		{".webm",  "video/webm"},
		{".ogv",   "video/ogg"},
		{".avi",   "video/x-msvideo"},
		{".flv",   "video/x-flv"},
		{".wmv",   "video/x-ms-wmv"},
		{".3gp",   "video/3gpp"},
		{".3g2",   "video/3gpp2"},
		{".ts",    "video/mp2t"},

		// Archives / compressed

		{".zip",   "application/zip"},
		{".tar",   "application/x-tar"},
		{".gz",    "application/gzip"},
		{".tgz",   "application/gzip"},
		{".bz",    "application/x-bzip"},
		{".bz2",   "application/x-bzip2"},
		{".xz",    "application/x-xz"},
		{".7z",    "application/x-7z-compressed"},
		{".rar",   "application/vnd.rar"},
		{".jar",   "application/java-archive"},

		// Executables / binaries

		{".exe",   "application/x-ms-dos-executable"},
		{".dll",   "application/x-msdownload"},
		{".so",    "application/x-sharedlib"},
		{".elf",   "application/x-executable"},
		{".bin",   "application/octet-stream"},
		{".class", "application/java-vm"},

		// Fonts

		{".ttf",   "font/ttf"},
		{".otf",   "font/otf"},
		{".woff",  "font/woff"},
		{".woff2", "font/woff2"},
		{".eot",   "application/vnd.ms-fontobject"},

		// PostScript / vector

		{".ps",    "application/postscript"},
		{".eps",   "application/postscript"},
		{".ai",    "application/postscript"},

		// Disk images / containers

		{".iso",   "application/x-iso9660-image"},
		{".img",   "application/octet-stream"},
		{".dmg",   "application/x-apple-diskimage"},

		// Web / misc

		{".css",   "text/css"},
		{".map",   "application/json"},
		{".wasm",  "application/wasm"},
		{".jsonld","application/ld+json"},
		{".webmanifest","application/manifest+json"},

		// CAD / specialized

		{".dxf",   "image/vnd.dxf"},
		{".dwg",   "application/acad"},

		// Mail / office miscellany

		{".msg",   "application/vnd.ms-outlook"}
	};


	std::string result;

	if (_use_extension_based_fallback) {
		auto dot_pos = pathname.rfind('.');
		if (dot_pos != std::string::npos) {
			std::string ext = pathname.substr(dot_pos);
			std::transform(ext.begin(), ext.end(), ext.begin(), [](unsigned char c){ return static_cast<char>(std::tolower(c)); });
			auto it = s_ext_to_type_map.find(ext);
			if (it != s_ext_to_type_map.end()) {
				result = it->second;
			}
		}
	}
	return result;
}


std::vector<std::string> XDGBasedAppProvider::GetMimeDatabaseSearchPaths()
{
	std::vector<std::string> paths;
	std::unordered_set<std::string> seen_paths;

	auto add_path = [&](const std::string& p) {
		if (!p.empty() && seen_paths.insert(p).second) {
			paths.push_back(p);
		}
	};

	std::string xdg_data_home = XDGBasedAppProvider::GetEnv("XDG_DATA_HOME", "");
	if (!xdg_data_home.empty()) {
		add_path(xdg_data_home + "/mime");
	} else {
		add_path(XDGBasedAppProvider::GetEnv("HOME", "") + "/.local/share/mime");
	}

	std::string xdg_data_dirs = XDGBasedAppProvider::GetEnv("XDG_DATA_DIRS", "/usr/local/share:/usr/share");
	for (const auto& dir : XDGBasedAppProvider::SplitString(xdg_data_dirs, ':')) {
		add_path(dir + "/mime");
	}
	return paths;
}


std::unordered_map<std::string, std::string> XDGBasedAppProvider::LoadMimeAliases()
{
	std::unordered_map<std::string, std::string> alias_to_canonical_map;
	auto mime_paths = GetMimeDatabaseSearchPaths();

	// Iterate paths from high priority (user) to low priority (system).
	for (const auto& path : mime_paths) {
		std::string alias_file_path = path + "/aliases";
		std::ifstream file(alias_file_path);
		if (!file.is_open()) continue;

		std::string line;
		while (std::getline(file, line)) {
			line = XDGBasedAppProvider::Trim(line);
			if (line.empty() || line[0] == '#') continue;

			std::stringstream ss(line);
			std::string alias_mime, canonical_mime;
			if (ss >> alias_mime >> canonical_mime) {
				// Use try_emplace to ensure that only the first-encountered alias
				// (from the highest-priority file) is inserted.
				alias_to_canonical_map.try_emplace(alias_mime, canonical_mime);
			}
		}
	}
	return alias_to_canonical_map;
}



// ****************************** Parsing XDG files and data ******************************


// Retrieves a DesktopEntry from the cache or parses it from disk if not present.
// It searches for the .desktop file in the prioritized list of directories.
// The first one found is used, correctly implementing the XDG override mechanism.
// (e.g., a user's copy in ~/.local/share/applications overrides a system-wide one).
const std::optional<DesktopEntry>& XDGBasedAppProvider::GetCachedDesktopEntry(const std::string& desktop_file,
																			  const std::vector<std::string>& search_paths,
																			  std::map<std::string, std::optional<DesktopEntry>>& cache)
{
	if (auto it = cache.find(desktop_file); it != cache.end()) {
		return it->second;
	}
	for (const auto& base_dir : search_paths) {
		std::string full_path = base_dir + "/" + desktop_file;
		if (auto entry = ParseDesktopFile(full_path)) {
			// A valid entry was found and parsed, cache and return it.
			return cache[desktop_file] = std::move(entry);
		}
	}
	// Cache a nullopt if the file is not found anywhere to avoid repeated searches.
	return cache[desktop_file] = std::nullopt;
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

	// Validate required fields and application type according to the spec.
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

	return desktop_entry;
}


// Parses a single mimeapps.list file, extracting Default/Added/Removed associations.
void XDGBasedAppProvider::ParseMimeappsList(const std::string& path, MimeAssociation& associations)
{
	std::ifstream file(path);
	if (!file.is_open()) return;

	std::string line, current_section;
	while (std::getline(file, line)) {
		line = Trim(line);
		if (line.empty() || line[0] == '#') continue;

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

		if (current_section == "[Default Applications]") {
			// Only use the first default if not already set, as higher priority files are parsed first.
			if (associations.defaults.find(key) == associations.defaults.end()) {
				associations.defaults[key] = { values[0], path };
			}
		} else if (current_section == "[Added Associations]") {
			auto& vec = associations.added[key];
			for (const auto& v : values) {
				if (!v.empty()) {
					vec.push_back({v, path});
				}
			}
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


// Parses mimeinfo.cache file format: [MIME Cache] section with mime/type=app1.desktop;app2.desktop;
void XDGBasedAppProvider::ParseMimeinfoCache(const std::string& path,
											 std::unordered_map<std::string, std::vector<MimeAssociation::AssociationSource>>& mime_cache)
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
				for (const auto& app : apps) {
					if (!app.empty()) {
						existing.push_back({app, path});
					}
				}
			}
		}
	}
}



// Retrieves a localized string value (e.g., Name[en_US]) from a map of key-value pairs,
// following the locale resolution logic based on environment variables.
// Priority: LC_ALL -> LC_MESSAGES -> LANG.
// Fallback: full locale (en_US) -> language only (en) -> unlocalized key.
std::string XDGBasedAppProvider::GetLocalizedValue(const std::unordered_map<std::string, std::string>& values,
												   const std::string& base_key)
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
				auto it = values.find(base_key + "[" + locale + "]");
				if (it != values.end()) return it->second;
				// Try language part only (e.g., Name[en]).
				size_t underscore_pos = locale.find('_');
				if (underscore_pos != std::string::npos) {
					std::string lang_only = locale.substr(0, underscore_pos);
					it = values.find(base_key + "[" + lang_only + "]");
					if (it != values.end()) return it->second;
				}
			}
		}
	}
	// Fallback to the unlocalized key (e.g., Name=).
	auto it = values.find(base_key);
	return (it != values.end()) ? it->second : "";
}



// ****************************** Command line constructing ******************************


// Performs the first-pass un-escaping for a raw string from a .desktop file.
// This handles general GKeyFile-style escape sequences like '\\' -> '\', '\s' -> ' ', etc.
std::string XDGBasedAppProvider::UnescapeGeneralString(const std::string& raw_str)
{
	std::string result;
	result.reserve(raw_str.length());

	for (size_t i = 0; i < raw_str.length(); ++i) {
		if (raw_str[i] == '\\') {
			if (i + 1 < raw_str.length()) {
				i++; // Move to the character after the backslash
				switch (raw_str[i]) {
				case 's': result += ' '; break;
				case 'n': result += '\n'; break;
				case 't': result += '\t'; break;
				case 'r': result += '\r'; break;
				case '\\': result += '\\'; break;
				default:
					// For unknown escape sequences, the backslash is dropped
					// and the character is preserved.
					result += raw_str[i];
					break;
				}
			} else {
				// A trailing backslash at the end of the string is treated as a literal.
				result += '\\';
			}
		} else {
			result += raw_str[i];
		}
	}
	return result;
}


std::vector<std::string> XDGBasedAppProvider::TokenizeExecString(const std::string& exec_str)
{
	// Pass 1: Handle general GKeyFile string escapes.
	const std::string unescaped_str = UnescapeGeneralString(exec_str);

	// Pass 2: Tokenize the result.
	std::vector<std::string> tokens;
	std::string current_token;
	bool in_quotes = false;

	for (size_t i = 0; i < unescaped_str.length(); ++i) {
		char c = unescaped_str[i];

		if (in_quotes) {
			if (c == '"') {
				in_quotes = false; // end of quoted section
			} else if (c == '\\') {
				// Handle escaped characters inside a quoted section.
				if (i + 1 < unescaped_str.length()) {
					// The spec mentions `\"`, `\``, `\$`, and `\\`.
					// A robust implementation simply un-escapes the next character.
					current_token += unescaped_str[++i];
				} else {
					// A trailing backslash is treated as a literal.
					current_token += c;
				}
			} else {
				current_token += c;
			}
		} else { // not in a quoted section
			if (isspace(c)) { // use isspace for both ' ' and '\t'
				// A whitespace character separates arguments.
				if (!current_token.empty()) {
					tokens.push_back(current_token);
					current_token.clear();
				}
			} else if (c == '"') {
				in_quotes = true; // start of a quoted section
			} else {
				// Append regular character to the current token.
				current_token += c;
			}
		}
	}

	// Add the final token if the string didn't end with a separator.
	if (!current_token.empty()) {
		tokens.push_back(current_token);
	}

	return tokens;
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
// This wraps the argument in double quotes and escapes special characters.
std::string XDGBasedAppProvider::EscapeArg(const std::string& arg)
{
	std::string out;
	out.push_back('"');
	for (char c : arg) {
		// Escape characters that have special meaning inside double quotes in shell.
		if (c == '\\' || c == '"' || c == '$' || c == '`') {
			out.push_back('\\');
		}
		out.push_back(c);
	}
	out.push_back('"');
	return out;
}


// ****************************** Paths and the System environment helpers ******************************


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

	// Additional common paths for Flatpak and Snap applications.
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


	// User config directory ($XDG_CONFIG_HOME or ~/.config) has the highest priority.
	std::string home = GetEnv("HOME", "");
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


std::string XDGBasedAppProvider::GetDefaultApp(const std::string& mime_type)
{
	if (mime_type.empty()) return "";
	std::string escaped_mime = EscapePathForShell(mime_type);
	std::string cmd = "xdg-mime query default " + escaped_mime + " 2>/dev/null";
	return RunCommandAndCaptureOutput(cmd);
}


// Checks if an executable exists and is runnable.
bool XDGBasedAppProvider::CheckExecutable(const std::string& path)
{
	if (path.empty()) {
		return false;
	}

	// If the path contains a slash, it's an absolute or relative path.
	// In that case we do not search $PATH, but check it directly.
	if (path.find('/') != std::string::npos) {
		return access(path.c_str(), X_OK) == 0;
	}

	// Otherwise, it's a command name and we must find it in $PATH.
	const char* path_env = getenv("PATH");
	if (!path_env) {
		// If $PATH is not set, search is impossible.
		return false;
	}

	std::string path_env_str(path_env);
	if (path_env_str.empty()) {
		return false;
	}

	std::istringstream path_stream(path_env_str);
	std::string dir;

	while (std::getline(path_stream, dir, ':')) {
		// Skip empty components in $PATH for safety,
		// to avoid checking the current working directory.
		if (dir.empty()) {
			continue;
		}

		std::string full_path = dir + '/' + path;
		if (access(full_path.c_str(), X_OK) == 0) {
			// File found and has execute permission.
			return true;
		}
	}

	// Command not found in any of the $PATH directories.
	return false;
}


// Safe environment variable access with an optional default value.
std::string XDGBasedAppProvider::GetEnv(const char* var, const char* default_val)
{
	const char* val = getenv(var);
	return val ? val : default_val;
}



// ****************************** Common helper functions ******************************


std::string XDGBasedAppProvider::Trim(std::string str)
{
	str.erase(str.begin(), std::find_if(str.begin(), str.end(), [](unsigned char ch) { return !std::isspace(ch); }));
	str.erase(std::find_if(str.rbegin(), str.rend(), [](unsigned char ch) { return !std::isspace(ch); }).base(), str.end());
	return str;
}


// String splitting utility with trimming of individual tokens.
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


// Escapes a path for safe use in a shell command by wrapping it in single quotes.
// It also handles any single quotes that might be inside the path itself.
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


std::string XDGBasedAppProvider::GetBaseName(const std::string& path)
{
	size_t pos = path.find_last_of('/');
	if (pos == std::string::npos) {
		return path;
	}
	return path.substr(pos + 1);
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


std::string XDGBasedAppProvider::RunCommandAndCaptureOutput(const std::string& cmd)
{
	std::string result;
	return POpen(result, cmd.c_str()) ? Trim(result) : "";
}


CandidateInfo XDGBasedAppProvider::ConvertDesktopEntryToCandidateInfo(const DesktopEntry& desktop_entry)
{
	CandidateInfo candidate;
	candidate.terminal = desktop_entry.terminal;
	candidate.name = StrMB2Wide(desktop_entry.name);

	// The ID is the basename of the .desktop file (e.g., "firefox.desktop").
	// This is used for lookups and to handle overrides correctly.
	candidate.id = StrMB2Wide(GetBaseName(desktop_entry.desktop_file));
	return candidate;
}

#endif
