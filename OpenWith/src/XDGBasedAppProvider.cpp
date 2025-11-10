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
#include <iomanip>
#include <unordered_set>
#include <utility>
#include <vector>
#include <map>
#include <set>

#define INI_LOCATION_XDG InMyConfig("plugins/openwith/config.ini")
#define INI_SECTION_XDG  "Settings.XDG"



// ****************************** Public API ******************************


XDGBasedAppProvider::XDGBasedAppProvider(TMsgGetter msg_getter) : AppProvider(std::move(msg_getter))
{
	_platform_settings_definitions = {
		{ "UseXdgMimeTool", MUseXdgMimeTool, &XDGBasedAppProvider::_use_xdg_mime_tool, true },
		{ "UseFileTool", MUseFileTool, &XDGBasedAppProvider::_use_file_tool, true },
		{ "UseExtensionBasedFallback", MUseExtensionBasedFallback, &XDGBasedAppProvider::_use_extension_based_fallback, false },
		{ "LoadMimeTypeAliases", MLoadMimeTypeAliases, &XDGBasedAppProvider::_load_mimetype_aliases, true },
		{ "LoadMimeTypeSubclasses", MLoadMimeTypeSubclasses, &XDGBasedAppProvider::_load_mimetype_subclasses, true },
		{ "ResolveStructuredSuffixes", MResolveStructuredSuffixes, &XDGBasedAppProvider::_resolve_structured_suffixes, true },
		{ "UseGenericMimeFallbacks", MUseGenericMimeFallbacks, &XDGBasedAppProvider::_use_generic_mime_fallbacks, true },
		{ "ShowUniversalHandlers", MShowUniversalHandlers, &XDGBasedAppProvider::_show_universal_handlers, true },
		{ "UseMimeinfoCache", MUseMimeinfoCache, &XDGBasedAppProvider::_use_mimeinfo_cache, true },
		{ "FilterByShowIn", MFilterByShowIn, &XDGBasedAppProvider::_filter_by_show_in, false },
		{ "ValidateTryExec", MValidateTryExec, &XDGBasedAppProvider::_validate_try_exec, false },
		{ "SortAlphabetically", MSortAlphabetically, &XDGBasedAppProvider::_sort_alphabetically, false },
		{ "TreatUrlsAsPaths", MTreatUrlsAsPaths, &XDGBasedAppProvider::_treat_urls_as_paths, false }
	};

	for (const auto& def : _platform_settings_definitions) {
		_key_to_member_map[StrMB2Wide(def.key)] = def.member_variable;
	}
}


void XDGBasedAppProvider::LoadPlatformSettings()
{
	KeyFileReadSection key_file_reader(INI_LOCATION_XDG, INI_SECTION_XDG);
	for (const auto& def : _platform_settings_definitions) {
		this->*(def.member_variable) = key_file_reader.GetInt(def.key.c_str(), def.default_value) != 0;
	}
}


void XDGBasedAppProvider::SavePlatformSettings()
{
	KeyFileHelper key_file_helper(INI_LOCATION_XDG);
	for (const auto& def : _platform_settings_definitions) {
		key_file_helper.SetInt(INI_SECTION_XDG, def.key.c_str(), this->*(def.member_variable));
	}
	key_file_helper.Save();
}


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


void XDGBasedAppProvider::SetPlatformSettings(const std::vector<ProviderSetting>& settings)
{
	for (const auto& s : settings) {
		auto it = _key_to_member_map.find(s.internal_key);
		if (it != _key_to_member_map.end()) {
			this->*(it->second) = s.value;
		}
	}
}


// Finds applications that can open all specified files.
std::vector<CandidateInfo> XDGBasedAppProvider::GetAppCandidates(const std::vector<std::wstring>& pathnames)
{
	if (pathnames.empty()) {
		return {};
	}

	// Clear class-level caches from the previous operation
	_desktop_entry_cache.clear();
	_last_candidates_source_info.clear();
	_last_mime_profiles.clear();

	// Initialize all operation-scoped state
	OperationContext op_context(*this);

	CandidateMap final_candidates;

	// --- Case 1: Handle the simple single-file ---
	if (pathnames.size() == 1) {
		auto profile = GetRawMimeProfile(StrWide2MB(pathnames[0]));
		_last_mime_profiles.insert(profile); // Cache the single profile
		auto prioritized_mimes = ExpandAndPrioritizeMimeTypes(profile);
		final_candidates = ResolveMimesToCandidateMap(prioritized_mimes);
	}
	else
	{
		// --- Case 2: Logic for Multiple Files ---

		// Step 1: Group N files into K unique "MIME profiles".
		std::unordered_set<RawMimeProfile, RawMimeProfile::Hash> unique_profiles;
		for (const auto& w_pathname : pathnames) {
			unique_profiles.insert(GetRawMimeProfile(StrWide2MB(w_pathname))); // N calls to external tools
		}

		// Cache all unique profiles. This is done *before* any early exits
		// so that GetMimeTypes() can report them even if no apps are found.
		_last_mime_profiles = unique_profiles;

		// Step 2: Gather candidates K times, not N times. (K = unique_profiles.size())
		std::unordered_map<RawMimeProfile, CandidateMap, RawMimeProfile::Hash> candidate_cache;

		if (unique_profiles.empty()) {
			return {};
		}

		for (const auto& profile : unique_profiles) {
			auto prioritized_mimes = ExpandAndPrioritizeMimeTypes(profile);
			auto candidates_for_current_profile = ResolveMimesToCandidateMap(prioritized_mimes);
			// Fail-fast optimization: If any profile has zero candidates, the final intersection will be empty.
			if (candidates_for_current_profile.empty()) {
				return {}; // we can stop all work immediately.
			}
			candidate_cache[profile] = std::move(candidates_for_current_profile);
		}

		// Step 3: Iterative Intersection using the K-sized cache.

		// Step 3a: Optimization: Find the profile with the *smallest* candidate set.
		// Intersection will be much faster if we start with the most restrictive set.

		auto smallest_set_it = unique_profiles.begin();
		size_t min_size = candidate_cache.at(*smallest_set_it).size();

		for (auto it = std::next(smallest_set_it); it != unique_profiles.end(); ++it) {
			size_t current_size = candidate_cache.at(*it).size();
			if (current_size < min_size) {
				min_size = current_size;
				smallest_set_it = it;
			}
		}

		// Step 3b: Establish a baseline set by *moving* the smallest map. This avoids any conversion.
		const RawMimeProfile base_profile = *smallest_set_it;
		// Use the map that will hold the final intersected results
		final_candidates = std::move(candidate_cache.at(base_profile));

		// Step 3c: Iteratively intersect with candidates from all *other* profiles. This loop runs (K - 1) times.
		for (const auto& current_profile : unique_profiles) {
			if (current_profile == base_profile) {
				continue;	// skip the profile we already used as the baseline.
			}

			// Get a reference to the *cached map* for the current profile.
			CandidateMap& candidates_for_current_profile = candidate_cache.at(current_profile);

			// Iterate through our master list (`final_candidates`) and remove any app that cannot handle the current profile.
			for (auto it = final_candidates.begin(); it != final_candidates.end(); ) {
				// Search directly in the cached map for the other profile.
				auto find_it = candidates_for_current_profile.find(it->first);
				if (find_it == candidates_for_current_profile.end()) {
					// This app is not in the list for the current profile, so remove it.
					it = final_candidates.erase(it);
				} else {
					// The app is valid. Update its rank if this profile provides a better one.
					const RankedCandidate& current_candidate = find_it->second;
					if (current_candidate.rank > it->second.rank) {
						it->second.rank = current_candidate.rank;
					}
					++it;
				}
			}

			// Early exit optimization.
			if (final_candidates.empty()) {
				return {};
			}
		}
	}

	// --- Step 4: Common Post-processing (for both 1 and >1 file cases) ---

	// Convert the final map to a vector and sort it
	auto final_candidates_sorted = BuildSortedRankedCandidatesList(final_candidates);

	// Only store source info (for F3) when a single file is selected
	return FormatCandidatesForUI(final_candidates_sorted, /* store_source_info = */ (pathnames.size() == 1));
}


// Constructs command lines according to the Exec= key in the .desktop file.
// It handles field codes for multi-file (%F, %U) and single-file (%f, %u) apps,
// generating one or multiple command lines respectively.
std::vector<std::wstring> XDGBasedAppProvider::ConstructCommandLine(const CandidateInfo& candidate, const std::vector<std::wstring>& pathnames)
{
	if (pathnames.empty()) {
		return {};
	}

	// Retrieve the full DesktopEntry from the cache, using the ID (basename)
	std::string desktop_file_name = StrWide2MB(candidate.id);
	auto it = _desktop_entry_cache.find(desktop_file_name);
	if (it == _desktop_entry_cache.end() || !it->second.has_value()) {
		return {};
	}
	const DesktopEntry& desktop_entry = it->second.value();

	std::string exec_mb = desktop_entry.exec;
	if (exec_mb.empty()) {
		return {};
	}

	// Tokenize the Exec= string, handling quotes and escapes
	std::vector<std::string> tokens = TokenizeExecString(exec_mb);
	if (tokens.empty()) {
		return {};
	}

	bool has_multi_file_code = false;
	bool has_single_file_code = false;
	for (const auto& token : tokens) {
		if (token.find('%') != std::string::npos) {
			if (token.find('F') != std::string::npos || token.find('U') != std::string::npos) {
				has_multi_file_code = true;
			}
			if (token.find('f') != std::string::npos || token.find('u') != std::string::npos) {
				has_single_file_code = true;
			}
		}
	}

	// If an app supports multi-file codes, single-file codes should be ignored.
	// If no file codes are present, paths are appended, implying multi-file support.
	bool use_multi_file_logic = has_multi_file_code || (!has_multi_file_code && !has_single_file_code);

	if (use_multi_file_logic) {
		// Case 1: App supports multiple files. Generate ONE command line.
		std::vector<std::string> final_args;
		final_args.reserve(tokens.size() + pathnames.size());

		// Use the first file as a context for expanding non-list field codes like %c or a standalone %f.
		std::string first_pathname = StrWide2MB(pathnames[0]);

		for (const std::string& token : tokens) {
			// A single token can contain multiple codes, but only one list code (%F or %U).
			if (token.find("%F") != std::string::npos) {
				for (const auto& wpathname : pathnames) {
					final_args.push_back(StrWide2MB(wpathname));
				}
			} else if (token.find("%U") != std::string::npos) {
				for (const auto& wpathname : pathnames) {
					if (_treat_urls_as_paths) { // compatibility mode?
						final_args.push_back(StrWide2MB(wpathname));
					} else {
						final_args.push_back(PathToUri(StrWide2MB(wpathname)));
					}
				}
			} else {
				// Expand other codes like %f, %u, %c using the first file as context.
				std::vector<std::string> expanded;
				if (!ExpandFieldCodes(desktop_entry, first_pathname, token, expanded, _treat_urls_as_paths)) {
					return {}; // Invalid field code.
				}
				final_args.insert(final_args.end(), expanded.begin(), expanded.end());
			}
		}

		// If no file codes were present at all, append all files at the end.
		if (!has_multi_file_code && !has_single_file_code) {
			for (const auto& wpathname : pathnames) {
				final_args.push_back(StrWide2MB(wpathname));
			}
		}

		if (final_args.empty()) {
			return {};
		}

		// Reconstruct the single command line, escaping each argument.
		std::string cmd;
		for (size_t i = 0; i < final_args.size(); ++i) {
			if (i > 0) cmd.push_back(' ');
			cmd += EscapeArgForShell(final_args[i]);
		}
		return { StrMB2Wide(cmd) };

	} else {

		// Case 2: App only supports single files (%f, %u). Generate MULTIPLE command lines.
		std::vector<std::wstring> final_commands;
		final_commands.reserve(pathnames.size());

		for (const auto& wpathname : pathnames) {
			std::vector<std::string> current_args;
			current_args.reserve(tokens.size());
			std::string pathname_mb = StrWide2MB(wpathname);

			for (const std::string& token : tokens) {
				std::vector<std::string> expanded;
				// Expand all codes using the current file as context.
				if (!ExpandFieldCodes(desktop_entry, pathname_mb, token, expanded, _treat_urls_as_paths)) {
					// Skip this file on error, but don't fail the whole operation.
					current_args.clear();
					break;
				}
				current_args.insert(current_args.end(), expanded.begin(), expanded.end());
			}

			if (current_args.empty()) {
				continue;
			}

			// Reconstruct one command line per file, escaping each argument.
			std::string cmd;
			for (size_t i = 0; i < current_args.size(); ++i) {
				if (i > 0) cmd.push_back(' ');
				cmd += EscapeArgForShell(current_args[i]);
			}
			final_commands.push_back(StrMB2Wide(cmd));
		}
		return final_commands;
	}
}


// Gets the detailed information for a candidate to display in the F3 dialog.
std::vector<Field> XDGBasedAppProvider::GetCandidateDetails(const CandidateInfo& candidate)
{
	std::vector<Field> details;
	std::string desktop_file_name = StrWide2MB(candidate.id);

	// Retrieve the cached DesktopEntry. This was populated during GetAppCandidates.
	auto it = _desktop_entry_cache.find(desktop_file_name);
	if (it == _desktop_entry_cache.end() || !it->second.has_value()) {
		return details;
	}
	const DesktopEntry& entry = it->second.value();

	details.push_back({m_GetMsg(MDesktopFile), StrMB2Wide(entry.desktop_file)});

	// Retrieve the source info (e.g., "mimeapps.list") stored during GetAppCandidates.
	// This is only available for single-file selections.
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
	if (!entry.not_show_in.empty()) {
		details.push_back({L"NotShowIn =", StrMB2Wide(entry.not_show_in)});
	}
	if (!entry.only_show_in.empty()) {
		details.push_back({L"OnlyShowIn =", StrMB2Wide(entry.only_show_in)});
	}

	return details;
}


// Returns a list of formatted strings representing the unique MIME profiles
// that were collected during the last GetAppCandidates() call.
std::vector<std::wstring> XDGBasedAppProvider::GetMimeTypes()
{
	std::set<std::wstring> final_unique_strings;
	bool has_none = false;

	for (const auto& profile : _last_mime_profiles)
	{

		std::set<std::string> unique_mimes_for_profile;

		if (!profile.xdg_mime.empty()) {
			unique_mimes_for_profile.insert(profile.xdg_mime);
		}
		if (!profile.file_mime.empty()) {
			unique_mimes_for_profile.insert(profile.file_mime);
		}
		if (!profile.stat_mime.empty()) {
			unique_mimes_for_profile.insert(profile.stat_mime);
		}
		if (!profile.ext_mime.empty()) {
			unique_mimes_for_profile.insert(profile.ext_mime);
		}

		if (unique_mimes_for_profile.empty()) {
			has_none = true;
		} else {
			std::stringstream ss;
			ss << "(";
			for (auto it = unique_mimes_for_profile.begin(); it != unique_mimes_for_profile.end(); ++it) {
				if (it != unique_mimes_for_profile.begin()) {
					ss << ";";
				}
				ss << *it;
			}
			ss << ")";

			final_unique_strings.insert(StrMB2Wide(ss.str()));
		}
	}

	std::vector<std::wstring> result(final_unique_strings.begin(), final_unique_strings.end());

	if (has_none) {
		result.insert(result.begin(), L"(none)");
	}

	return result;
}



// ****************************** Searching and ranking candidates logic ******************************

// Resolves MIME types to a map of unique candidates.
XDGBasedAppProvider::CandidateMap XDGBasedAppProvider::ResolveMimesToCandidateMap(const std::vector<std::string>& prioritized_mimes)
{
	// This map will store the final, unique candidates for this MIME list.
	CandidateMap unique_candidates;

	// Check for a global default app using 'xdg-mime query default'.
	std::string global_default_app = GetDefaultApp(prioritized_mimes.empty() ? "" : prioritized_mimes[0]);

	if (!global_default_app.empty()) {
		const auto& mime_for_default = prioritized_mimes[0];
		if (!IsAssociationRemoved(mime_for_default, global_default_app)) {
			int rank = (prioritized_mimes.size() - 0) * Ranking::SPECIFICITY_MULTIPLIER + Ranking::SOURCE_RANK_GLOBAL_DEFAULT;
			RegisterCandidateById(unique_candidates, global_default_app, rank,  "xdg-mime query default " + mime_for_default);
		}
	}

	AppendCandidatesFromMimeAppsLists(prioritized_mimes, unique_candidates);

	// Find candidates using the cache prepared by OperationContext.
	if (_op_mime_to_handlers_map.has_value()) {
		// Use the results from mimeinfo.cache (it was successfully loaded and was not empty).
		AppendCandidatesFromMimeinfoCache(prioritized_mimes, unique_candidates);
	} else {
		// Use the full scan index (either because caching was disabled, or as a fallback).
		AppendCandidatesByFullScan(prioritized_mimes, unique_candidates);
	}

	return unique_candidates;
}


// Find the system's global default handler for a MIME type.
std::string XDGBasedAppProvider::GetDefaultApp(const std::string& mime_type)
{
	if (mime_type.empty()) return "";
	std::string escaped_mime = EscapeArgForShell(mime_type);
	std::string cmd = "xdg-mime query default " + escaped_mime + " 2>/dev/null";
	return RunCommandAndCaptureOutput(cmd);
}


// Finds candidates from the parsed mimeapps.list files. This is a high-priority source.
void XDGBasedAppProvider::AppendCandidatesFromMimeAppsLists(const std::vector<std::string>& prioritized_mimes, CandidateMap& unique_candidates)
{
	const int total_mimes = prioritized_mimes.size();
	const auto& mimeapps_lists_data = _op_mimeapps_lists_data.value();

	for (int i = 0; i < total_mimes; ++i) {
		const auto& mime = prioritized_mimes[i];
		// Rank based on MIME type specificity. The more specific, the higher the rank.
		int mime_specificity_rank = (total_mimes - i);

		// 1. Default application from mimeapps.list: high source rank.
		auto it_defaults = mimeapps_lists_data.defaults.find(mime);
		if (it_defaults != mimeapps_lists_data.defaults.end()) {
			const auto& default_app = it_defaults->second;
			if (!IsAssociationRemoved(mime, default_app.desktop_file)) {
				int rank = mime_specificity_rank * Ranking::SPECIFICITY_MULTIPLIER + Ranking::SOURCE_RANK_MIMEAPPS_DEFAULT;
				std::string source_info = default_app.source_path + StrWide2MB(m_GetMsg(MIn)) + " [Default Applications] " + StrWide2MB(m_GetMsg(MFor)) + mime;
				RegisterCandidateById(unique_candidates, default_app.desktop_file, rank, source_info);
			}
		}
		// 2. Added associations from mimeapps.list: medium source rank.
		auto it_added = mimeapps_lists_data.added.find(mime);
		if (it_added != mimeapps_lists_data.added.end()) {
			for (const auto& app_assoc : it_added->second) {
				if (!IsAssociationRemoved(mime, app_assoc.desktop_file)) {
					int rank = mime_specificity_rank * Ranking::SPECIFICITY_MULTIPLIER + Ranking::SOURCE_RANK_MIMEAPPS_ADDED;
					std::string source_info = app_assoc.source_path + StrWide2MB(m_GetMsg(MIn)) + "[Added Associations]" + StrWide2MB(m_GetMsg(MFor)) + mime;
					RegisterCandidateById(unique_candidates, app_assoc.desktop_file, rank, source_info);
				}
			}
		}
	}
}


// Finds candidates from the pre-generated mimeinfo.cache file for performance.
void XDGBasedAppProvider::AppendCandidatesFromMimeinfoCache(const std::vector<std::string>& prioritized_mimes, CandidateMap& unique_candidates)
{
	const int total_mimes = prioritized_mimes.size();
	// This map tracks the best rank for each app to avoid rank demotion by a less-specific MIME type.
	std::map<std::string, std::pair<int, std::string>> app_best_rank_and_source;
	const auto& mime_to_handlers_map = _op_mime_to_handlers_map.value();
	// First, find the best possible rank for each application across all matching MIME types.
	// This prevents an app from getting a low rank for a generic MIME type (e.g., text/plain)
	// if it has already been matched with a high-rank specific MIME type.
	for (int i = 0; i < total_mimes; ++i) {
		const auto& mime = prioritized_mimes[i];
		auto it_cache = mime_to_handlers_map.find(mime);
		if (it_cache != mime_to_handlers_map.end()) {
			// Calculate rank using the tiered formula.
			int rank = (total_mimes - i) * Ranking::SPECIFICITY_MULTIPLIER + Ranking::SOURCE_RANK_CACHE_OR_SCAN;
			for (const auto& handler_provenance : it_cache->second) {
				const auto& app_desktop_file = handler_provenance.desktop_file;
				if (app_desktop_file.empty()) continue;
				if (IsAssociationRemoved(mime, app_desktop_file)) continue;
				std::string source_info = handler_provenance.source_path + StrWide2MB(m_GetMsg(MFor)) + mime;
				auto [it, inserted] = app_best_rank_and_source.try_emplace(app_desktop_file, std::make_pair(rank, source_info));
				// If not inserted (key existed), update only if the new rank is better.
				if (!inserted && rank > it->second.first) {
					it->second.first = rank;
					it->second.second = source_info;
				}
			}
		}
	}

	// Now, process each application with its best-found rank.
	for (const auto& [app_desktop_file, rank_and_source] : app_best_rank_and_source) {
		RegisterCandidateById(unique_candidates, app_desktop_file, rank_and_source.first, rank_and_source.second);
	}
}


// Finds candidates by looking up the pre-built mime-to-app index. Used when _use_mimeinfo_cache is false.
void XDGBasedAppProvider::AppendCandidatesByFullScan(const std::vector<std::string>& prioritized_mimes, CandidateMap& unique_candidates)
{
	const int total_mimes = prioritized_mimes.size();
	// A map to store the best rank found for each unique application.
	std::map<const DesktopEntry*, std::pair<int, std::string>> app_best_rank_and_source;
	const auto& mime_to_desktop_entry_map = _op_mime_to_desktop_entry_map.value();

	// Iterate through all prioritized MIME types for the current file.
	for (int i = 0; i < total_mimes; ++i) {
		const auto& mime = prioritized_mimes[i];

		// Find this MIME type in the pre-built mime-to-app index.
		auto it_index = mime_to_desktop_entry_map.find(mime);
		if (it_index == mime_to_desktop_entry_map.end()) {
			continue; // no applications are associated with this MIME type in the index.
		}

		// Calculate the rank for this level of MIME specificity.
		int rank = (total_mimes - i) * Ranking::SPECIFICITY_MULTIPLIER + Ranking::SOURCE_RANK_CACHE_OR_SCAN;

		// Iterate through all applications found for this MIME type.
		for (const DesktopEntry* entry_ptr : it_index->second) {
			if (!entry_ptr) continue;

			// Check if this specific association is explicitly removed in mimeapps.list.
			if (IsAssociationRemoved(mime, GetBaseName(entry_ptr->desktop_file))) {
				continue;
			}

			std::string source_info = StrWide2MB(m_GetMsg(MFullScanFor)) + mime;

			// Try to insert the app with its rank. If it already exists, update its rank only if the new one is higher.
			auto [it, inserted] = app_best_rank_and_source.try_emplace(entry_ptr, std::make_pair(rank, source_info));
			if (!inserted && rank > it->second.first) {
				it->second.first = rank;
				it->second.second = source_info;
			}
		}
	}

	// Now, register each unique candidate with its best-found rank.
	for (const auto& [entry_ptr, rank_and_source] : app_best_rank_and_source) {
		// Call the registration helper directly, passing the dereferenced entry to avoid a redundant cache lookup.
		RegisterCandidateFromObject(unique_candidates, *entry_ptr, rank_and_source.first, rank_and_source.second);
	}
}


// Processes a single application candidate: validates it, filters it, and adds it to the map.
// This version retrieves the DesktopEntry from cache via its name.
void XDGBasedAppProvider::RegisterCandidateById(CandidateMap& unique_candidates, const std::string& app_desktop_file,
													   int rank, const std::string& source_info)
{
	if (app_desktop_file.empty()) return;

	// Retrieve the full DesktopEntry, either from cache or by parsing it.
	const auto& entry_opt = GetCachedDesktopEntry(app_desktop_file);
	if (!entry_opt) {
		return;
	}

	// Pass the actual DesktopEntry object to the core validation and registration logic.
	RegisterCandidateFromObject(unique_candidates, *entry_opt, rank, source_info);
}


// This is the core validation and registration helper.
// It filters a candidate (TryExec, ShowIn/NotShowIn) and adds it to the map.
void XDGBasedAppProvider::RegisterCandidateFromObject(CandidateMap& unique_candidates, const DesktopEntry& entry,
											int rank, const std::string& source_info)
{
	// Optionally validate the TryExec key to ensure the executable exists.
	if (_validate_try_exec && !entry.try_exec.empty() && !CheckExecutable(entry.try_exec)) {
		return;
	}

	const auto& current_desktop_env = _op_current_desktop_env.value();

	// Optionally filter applications based on the current desktop environment
	// using the OnlyShowIn and NotShowIn keys.
	if (_filter_by_show_in && !current_desktop_env.empty()) {
		if (!entry.only_show_in.empty()) {
			bool found = false;
			for (const auto& desktop : SplitString(entry.only_show_in, ';')) {
				if (desktop == current_desktop_env) { found = true; break; }
			}
			if (!found) return;
		}
		if (!entry.not_show_in.empty()) {
			bool found = false;
			for (const auto& desktop : SplitString(entry.not_show_in, ';')) {
				if (desktop == current_desktop_env) { found = true; break; }
			}
			if (found) return;
		}
	}

	AddOrUpdateCandidate(unique_candidates, entry, rank, source_info);
}


// Adds a new candidate to the results map or updates its rank if a better one is found.
void XDGBasedAppProvider::AddOrUpdateCandidate(CandidateMap& unique_candidates, const DesktopEntry& entry,
											   int rank, const std::string& source_info)
{
	// A unique key to identify an application.
	AppUniqueKey unique_key{entry.name, entry.exec};

	auto [it, inserted] = unique_candidates.try_emplace(unique_key, RankedCandidate{&entry, rank, source_info});

	// If the candidate already exists, update it only if the new rank is higher.
	if (!inserted && rank > it->second.rank) {
		it->second.rank = rank;
		it->second.entry = &entry;
		it->second.source_info = source_info;
	}
}


// Checks if an application association for a given MIME type is explicitly removed
// in the [Removed Associations] section of mimeapps.list.
bool XDGBasedAppProvider::IsAssociationRemoved(const std::string& mime_type, const std::string& app_desktop_file)
{
	// Get the parsed config from the operation-scoped cache.
	const auto& mimeapps_lists_data = _op_mimeapps_lists_data.value();

	// 1. Check for an exact match (e.g., "image/jpeg")
	auto it_exact = mimeapps_lists_data.removed.find(mime_type);
	if (it_exact != mimeapps_lists_data.removed.end() && it_exact->second.count(app_desktop_file)) {
		return true;
	}

	// 2. Check for a wildcard match (e.g., "image/*")
	const size_t slash_pos = mime_type.find('/');
	if (slash_pos != std::string::npos) {
		const std::string wildcard_mime = mime_type.substr(0, slash_pos) + "/*";
		auto it_wildcard = mimeapps_lists_data.removed.find(wildcard_mime);
		if (it_wildcard != mimeapps_lists_data.removed.end() && it_wildcard->second.count(app_desktop_file)) {
			return true;
		}
	}

	return false;
}


// Converts the final map of candidates into a vector and sorts it.
std::vector<XDGBasedAppProvider::RankedCandidate> XDGBasedAppProvider::BuildSortedRankedCandidatesList(const CandidateMap& candidate_map)
{
	// Convert the map of unique candidates to a vector for sorting.
	std::vector<RankedCandidate> sorted_candidates;
	sorted_candidates.reserve(candidate_map.size());
	for (const auto& [key, ranked_candidate] : candidate_map) {
		sorted_candidates.push_back(ranked_candidate);
	}

	// Sort the final vector based on rank or alphabetical settings.
	if (_sort_alphabetically) {
		// Sort candidates based on their name only
		std::sort(sorted_candidates.begin(), sorted_candidates.end(), [](const RankedCandidate& a, const RankedCandidate& b) {
			if (!a.entry || !b.entry) return b.entry != nullptr;
			return a.entry->name < b.entry->name;
		});
	} else {
		// Use the standard ranking defined in the < operator for RankedCandidate (sorts by rank descending, then name ascending).
		std::sort(sorted_candidates.begin(), sorted_candidates.end());
	}

	return sorted_candidates;
}


// Converts the sorted list of RankedCandidates into the final CandidateInfo list for the UI.
std::vector<CandidateInfo> XDGBasedAppProvider::FormatCandidatesForUI(
	const std::vector<RankedCandidate>& ranked_candidates,
	bool store_source_info)
{
	std::vector<CandidateInfo> result;
	result.reserve(ranked_candidates.size());

	for (const auto& ranked_candidate : ranked_candidates) {
		// Convert the internal DesktopEntry representation to the UI-facing CandidateInfo.
		CandidateInfo ci = ConvertDesktopEntryToCandidateInfo(*ranked_candidate.entry);

		// If requested (only for single-file lookups), store the association's source
		// for the F3 details dialog.
		if (store_source_info) {
			_last_candidates_source_info[ci.id] = ranked_candidate.source_info;
		}
		result.push_back(ci);
	}
	return result;
}


// Converts a parsed DesktopEntry object into a CandidateInfo struct for the UI.
CandidateInfo XDGBasedAppProvider::ConvertDesktopEntryToCandidateInfo(const DesktopEntry& desktop_entry)
{
	CandidateInfo candidate;
	candidate.terminal = desktop_entry.terminal;
	candidate.name = StrMB2Wide(desktop_entry.name);

	// The ID is the basename of the .desktop file (e.g., "firefox.desktop").
	// This is used for lookups and to handle overrides correctly.
	candidate.id = StrMB2Wide(GetBaseName(desktop_entry.desktop_file));

	const std::string& exec = desktop_entry.exec;

	bool has_multi_file_code = HasFieldCode(exec, "FU");
	bool has_single_file_code = HasFieldCode(exec, "fu");
	candidate.multi_file_aware = has_multi_file_code || (!has_multi_file_code && !has_single_file_code);

	return candidate;
}



// ****************************** File MIME Type Detection & Expansion ******************************

// Gathers file attributes and "raw" MIME types from all enabled detection methods for a single file.
XDGBasedAppProvider::RawMimeProfile XDGBasedAppProvider::GetRawMimeProfile(const std::string& pathname)
{
	RawMimeProfile profile = {};
	struct stat st;

	if (stat(pathname.c_str(), &st) != 0) {
		// stat() failed (e.g., ENOENT).
		// We can't determine any file type. Return the empty profile.
		return profile;
	}

	// Path exists. Determine file type and get MIME types based on it.
	if (S_ISREG(st.st_mode)) {
		profile.is_regular_file = true;

		// Only call extension-based lookup for regular files
		profile.ext_mime = MimeTypeByExtension(pathname);

		if (access(pathname.c_str(), R_OK) == 0) {
			// Run expensive external tools ONLY for accessible regular files.
			profile.xdg_mime = MimeTypeFromXdgMimeTool(pathname);
			profile.file_mime = MimeTypeFromFileTool(pathname);
		}

	} else if (S_ISDIR(st.st_mode)) {
		profile.stat_mime = "inode/directory";
	} else if (S_ISFIFO(st.st_mode)) {
		profile.stat_mime = "inode/fifo";
	} else if (S_ISSOCK(st.st_mode)) {
		profile.stat_mime = "inode/socket";
	} else if (S_ISCHR(st.st_mode)) {
		profile.stat_mime = "inode/chardevice";
	} else if (S_ISBLK(st.st_mode)) {
		profile.stat_mime = "inode/blockdevice";
	}

	// Return the populated profile.
	return profile;
}


// Expands and prioritizes MIME types for a file using multiple methods.
std::vector<std::string> XDGBasedAppProvider::ExpandAndPrioritizeMimeTypes(const RawMimeProfile& profile)
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

	// --- Step 1: Initial, most specific MIME type detection ---
	// Use the pre-detected types from the RawMimeProfile in order of priority.
	add_unique(profile.xdg_mime);
	add_unique(profile.file_mime);
	add_unique(profile.stat_mime);
	add_unique(profile.ext_mime);

	// --- Step 2: Iteratively expand the MIME type list with parents and aliases ---
	if (_op_subclass_to_parent_map || _op_alias_to_canonical_map) {

		for (size_t i = 0; i < mime_types.size(); ++i) {
			const std::string current_mime = mime_types[i];

			// Expansion A: process aliases
			if (_op_alias_to_canonical_map) {

				// --- Standard forward lookup (alias -> canonical) ---
				const auto& alias_to_canonical_map = *_op_alias_to_canonical_map;
				auto it_canonical = alias_to_canonical_map.find(current_mime);
				if (it_canonical != alias_to_canonical_map.end()) {
					add_unique(it_canonical->second);
				}

				// --- Smart reverse lookup (canonical -> alias) using the pre-built map ---
				// Find aliases for the current canonical MIME type, but filter them
				// to avoid incorrect associations (e.g., image/* -> text/*).

				auto it_aliases = _op_canonical_to_aliases_map->find(current_mime);
				if (it_aliases != _op_canonical_to_aliases_map->end()) {
					size_t canonical_slash_pos = current_mime.find('/');
					// Proceed only if the canonical MIME type has a valid format (e.g., "type/subtype").
					if (canonical_slash_pos != std::string::npos) {
						std::string_view canonical_major_type(current_mime.data(), canonical_slash_pos);
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
			}

			// Expansion B: Add parent from subclass hierarchy.
			if (_op_subclass_to_parent_map) {
				auto it = _op_subclass_to_parent_map->find(current_mime);
				if (it != _op_subclass_to_parent_map->end()) {
					add_unique(it->second);
				}
			}
		}
	}

	if (_resolve_structured_suffixes) {

		// --- Step 3: Add base types for structured syntaxes (e.g., +xml, +zip) ---
		// This is a reliable fallback that works even if the subclass hierarchy is incomplete in the system's MIME database.

		static const std::map<std::string, std::string> s_suffix_to_base_mime = {
			{"xml", "application/xml"},
			{"zip", "application/zip"},
			{"json", "application/json"},
			{"gzip", "application/gzip"}
		};

		auto obtained_types_before_suffix_check = mime_types;
		for (const auto& mime : obtained_types_before_suffix_check) {
			size_t plus_pos = mime.rfind('+');
			if (plus_pos != std::string::npos && plus_pos < mime.length() - 1) {
				std::string suffix = mime.substr(plus_pos + 1);
				auto it = s_suffix_to_base_mime.find(suffix);
				if (it != s_suffix_to_base_mime.end()) {
					add_unique(it->second);
				}
			}
		}
	}

	if (_use_generic_mime_fallbacks) {
		// --- Step 4: Add generic fallback MIME types ---
		auto obtained_types_before_fallback = mime_types;
		for (const auto& mime : obtained_types_before_fallback) {
			// text/plain is a safe fallback for any text/* type.
			if (mime.rfind("text/", 0) == 0) {
				add_unique("text/plain");
			}
			// Add wildcard for the major type (e.g., image/jpeg -> image/*)
			size_t slash_pos = mime.find('/');
			if (slash_pos != std::string::npos) {
				add_unique(mime.substr(0, slash_pos) + "/*");
			}
		}
	}

	if (_show_universal_handlers) {
		// --- Step 5: Add the ultimate fallback for any binary data ---
		if (profile.is_regular_file) {
			add_unique("application/octet-stream");
		}
	}

	return mime_types;
}


std::string XDGBasedAppProvider::MimeTypeFromXdgMimeTool(const std::string& pathname)
{
	std::string result;
	if (_use_xdg_mime_tool) {
		auto escaped_pathname = EscapeArgForShell(pathname);
		result = RunCommandAndCaptureOutput("xdg-mime query filetype " + escaped_pathname + " 2>/dev/null");
	}
	return result;
}


std::string XDGBasedAppProvider::MimeTypeFromFileTool(const std::string& pathname)
{
	std::string result;
	if(_use_file_tool) {
		auto escaped_pathname = EscapeArgForShell(pathname);
		result = RunCommandAndCaptureOutput("file --brief --dereference --mime-type " + escaped_pathname + " 2>/dev/null");
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



// ****************************** XDG Database Parsing & Caching ******************************

// Retrieves a DesktopEntry from the cache or parses it from disk if not present.
const std::optional<DesktopEntry>& XDGBasedAppProvider::GetCachedDesktopEntry(const std::string& desktop_file)
{
	// Use the class-level cache
	if (auto it = _desktop_entry_cache.find(desktop_file); it != _desktop_entry_cache.end()) {
		return it->second;
	}

	// The `desktop_file` parameter is just the basename (e.g., "firefox.desktop")
	for (const auto& base_dir : _op_desktop_paths.value()) {
		std::string full_path = base_dir + "/" + desktop_file;
		if (auto entry = ParseDesktopFile(full_path)) {
			// A valid entry was found and parsed, cache and return it.
			return _desktop_entry_cache[desktop_file] = std::move(entry);
		}
	}
	// Cache a nullopt if the file is not found anywhere to avoid repeated searches.
	return _desktop_entry_cache[desktop_file] = std::nullopt;
}


// Builds in-memory index that maps a MIME type to a list of DesktopEntry pointers
// that can handle it. This avoids repeated filesystem scanning.
XDGBasedAppProvider::MimeToDesktopEntryIndex XDGBasedAppProvider::FullScanDesktopFilesAndBuildIndex(const std::vector<std::string>& search_paths)
{
	MimeToDesktopEntryIndex index;
	for (const auto& dir : search_paths) {
		DIR* dir_stream = opendir(dir.c_str());
		if (!dir_stream) continue;

		struct dirent* dir_entry;
		while ((dir_entry = readdir(dir_stream))) {
			std::string filename = dir_entry->d_name;
			if (filename.size() <= 8 || filename.substr(filename.size() - 8) != ".desktop") {
				continue;
			}

			// This call populates _desktop_entry_cache as a side effect.
			const auto& desktop_entry_opt = GetCachedDesktopEntry(filename);
			if (!desktop_entry_opt.has_value()) {
				continue;
			}

			// A non-owning pointer is safe because _desktop_entry_cache is a std::map.
			const DesktopEntry* entry_ptr = &desktop_entry_opt.value();

			// Split the MimeType= key and populate the index for each MIME type.
			std::vector<std::string> entry_mimes = SplitString(entry_ptr->mimetype, ';');
			for (const auto& mime : entry_mimes) {
				if (!mime.empty()) {
					// Add the pointer to the list for this MIME type.
					index[mime].push_back(entry_ptr);
				}
			}
		}
		closedir(dir_stream);
	}
	return index;
}


// Parses all mimeinfo.cache files found in the XDG search paths into a single map, avoiding repeated file I/O.
XDGBasedAppProvider::MimeinfoCacheData XDGBasedAppProvider::ParseAllMimeinfoCacheFiles(const std::vector<std::string>& search_paths)
{
	MimeinfoCacheData data;
	for (const auto& dir : search_paths) {
		std::string cache_path = dir + "/mimeinfo.cache";
		if (IsReadableFile(cache_path)) {
			ParseMimeinfoCache(cache_path, data);
		}
	}
	return data;
}


// Parses mimeinfo.cache file format: [MIME Cache] section with mime/type=app1.desktop;app2.desktop;
void XDGBasedAppProvider::ParseMimeinfoCache(const std::string& path, MimeinfoCacheData& mimeinfo_cache_data)
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
				auto& existing = mimeinfo_cache_data[mime];
				for (const auto& app : apps) {
					if (!app.empty()) {
						// We append apps from all cache files; duplicates are okay.
						existing.push_back({app, path});
					}
				}
			}
		}
	}
}


// Combines multiple mimeapps.list files into a single association structure.
XDGBasedAppProvider::MimeappsListsData XDGBasedAppProvider::ParseMimeappsLists(const std::vector<std::string>& paths)
{
	MimeappsListsData mimeapps_lists_data;
	for (const auto& path : paths) {
		ParseMimeappsList(path, mimeapps_lists_data);
	}
	return mimeapps_lists_data;
}


// Parses a single mimeapps.list file, extracting Default/Added/Removed associations.
void XDGBasedAppProvider::ParseMimeappsList(const std::string& path, MimeappsListsData& mimeapps_lists_data)
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
			if (mimeapps_lists_data.defaults.find(key) == mimeapps_lists_data.defaults.end()) {
				mimeapps_lists_data.defaults[key] = { values[0], path };
			}
		} else if (current_section == "[Added Associations]") {
			auto& vec = mimeapps_lists_data.added[key];
			for (const auto& v : values) {
				if (!v.empty()) {
					vec.push_back({v, path});
				}
			}
		} else if (current_section == "[Removed Associations]") {
			for(const auto& v : values) {
				if(!v.empty()) mimeapps_lists_data.removed[key].insert(v);
			}
		}
	}
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

	// Ignore hidden entries.
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


// Loads the MIME alias map (alias -> canonical) from all 'aliases' files.
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


// Loads the MIME type inheritance map (child -> parent) from all 'subclasses' files.
// Files with higher priority (user-specific) override lower-priority (system) ones.
std::unordered_map<std::string, std::string> XDGBasedAppProvider::LoadMimeSubclasses()
{
	std::unordered_map<std::string, std::string> subclass_to_parent_map;
	auto mime_paths = GetMimeDatabaseSearchPaths();

	// Iterate paths from low priority (system) to high (user).
	// This ensures that user-defined rules in higher-priority files
	// will overwrite the system-wide ones when inserted into the map.
	for (auto it = mime_paths.rbegin(); it != mime_paths.rend(); ++it) {
		std::string subclasses_file_path = *it + "/subclasses";
		std::ifstream file(subclasses_file_path);
		if (!file.is_open()) continue;

		std::string line;
		while (std::getline(file, line)) {
			line = Trim(line);
			if (line.empty() || line[0] == '#') continue;

			std::stringstream ss(line);
			std::string child_mime, parent_mime;
			if (ss >> child_mime >> parent_mime) {
				subclass_to_parent_map[child_mime] = parent_mime;
			}
		}
	}
	return subclass_to_parent_map;
}


// Returns XDG-compliant search paths for .desktop files, ordered by priority.
std::vector<std::string> XDGBasedAppProvider::GetDesktopFileSearchPaths()
{
	std::vector<std::string> paths;
	std::unordered_set<std::string> seen_paths;

	auto add_path = [&](const std::string& p) {
		if (!p.empty() && IsTraversableDirectory(p) && seen_paths.insert(p).second) {
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

	// This helper only adds paths that point to existing, readable files.
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


// Returns XDG-compliant search paths for MIME database files (aliases, subclasses, etc.)
std::vector<std::string> XDGBasedAppProvider::GetMimeDatabaseSearchPaths()
{
	std::vector<std::string> paths;
	std::unordered_set<std::string> seen_paths;

	auto add_path = [&](const std::string& p) {
		if (!p.empty() && IsTraversableDirectory(p) && seen_paths.insert(p).second) {
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



// ****************************** Command line constructing ******************************

// Tokenizes the Exec= string into a vector of arguments, handling quotes and escapes
// as defined by the Desktop Entry Specification.
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


// Expands XDG Desktop Entry field codes (%f, %u, %c, etc.).
// This function correctly distinguishes between field codes that expect a local
// file path (%f, %F) and those that expect a URI (%u, %U).
bool XDGBasedAppProvider::ExpandFieldCodes(const DesktopEntry& candidate,
										   const std::string& pathname,
										   const std::string& unescaped,
										   std::vector<std::string>& out_args,
										   bool treat_urls_as_paths)
{
	std::string cur;
	for (size_t i = 0; i < unescaped.size(); ++i) {
		char c = unescaped[i];
		if (c == '%') {
			if (i + 1 >= unescaped.size()) return false; // Dangling '%' is an error.
			char code = unescaped[i + 1];
			++i;
			switch (code) {
			case 'f': case 'F': // %f, %F expect a local path.
				cur += pathname;
				break;
			case 'u': case 'U': // %u, %U expect a URI.
				if (treat_urls_as_paths) { // compatibility mode?
					cur += pathname;
				} else {
					cur += PathToUri(pathname);
				}
				break;
			case 'c': // The application's name (from the Name= key).
				cur += candidate.name;
				break;
			case '%': // A literal '%' character.
				cur.push_back('%');
				break;
			// Deprecated and unused field codes are silently ignored
			case 'n': case 'd': case 'D': case 't': case 'T': case 'v': case 'm':
			case 'k': case 'i':
				break;
			default:
				// Any other field code is invalid.
				return false;
			}
		} else {
			cur.push_back(c);
		}
	}
	if (!cur.empty()) out_args.push_back(cur);
	return true;
}


// Searches the Exec string for any field code characters specified in 'codes_to_find'.
// This function correctly handles escaped '%%' sequences according to the XDG specification.
bool XDGBasedAppProvider::HasFieldCode(const std::string& exec, const std::string& codes_to_find)
{
	size_t pos = 0;
	// Find the next occurrence of '%' starting from the current position.
	while ((pos = exec.find('%', pos)) != std::string::npos) {
		// Ensure there is a character following the '%'. A trailing '%' is not a field code.
		if (pos + 1 >= exec.size()) {
			break;
		}

		const char next_char = exec[pos + 1];

		// If the character following '%' is not one of the codes we are looking for,
		// it might be '%%' or another irrelevant code. Skip it and continue searching.
		if (codes_to_find.find(next_char) == std::string::npos) {
			pos++;
			continue;
		}

		// A potential field code (e.g., %F) has been found. Now, we must check if it's escaped.
		// Count the number of consecutive '%' characters immediately preceding our find.
		size_t preceding_percents = 0;
		size_t check_pos = pos;
		while (check_pos > 0 && exec[check_pos - 1] == '%') {
			preceding_percents++;
			check_pos--;
		}

		// If the number of preceding '%' is even (0, 2, 4...), the current '%' is not escaped
		// and thus starts a valid field code (e.g., %F, %%%F).
		if (preceding_percents % 2 == 0) {
			return true;
		}

		// If the number is odd, the current '%' is part of an escaped sequence (e.g., %%F, %%%%F).
		// We must ignore it and continue the search from the next character.
		pos++;
	}

	return false;
}


// Converts absolute local filepath to a file:// URI.
std::string XDGBasedAppProvider::PathToUri(const std::string& path)
{
	std::stringstream uri_stream;
	uri_stream << "file://";
	uri_stream << std::hex << std::uppercase << std::setfill('0');

	for (const unsigned char c : path) {
		// Perform a locale-independent check for unreserved characters (RFC 3986) plus the path separator '/'.
		if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9')
			|| c == '-' || c == '_' || c == '.' || c == '~' || c == '/')
		{
			uri_stream << c;
		} else {
			// Percent-encode all other characters.
			uri_stream << '%' << std::setw(2) << static_cast<int>(c);
		}
	}
	return uri_stream.str();
}


// ****************************** System & Environment Helpers ******************************


// Checks if an executable exists and is runnable.
// If the path contains a slash, it's checked directly. Otherwise, it's searched in $PATH.
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


// Runs a shell command and captures its standard output.
std::string XDGBasedAppProvider::RunCommandAndCaptureOutput(const std::string& cmd)
{
	std::string result;
	return POpen(result, cmd.c_str()) ? Trim(result) : "";
}



// ****************************** Common helper functions ******************************

// Trims whitespace from both ends of a string.
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


// Escapes a single command-line argument for safe execution by the shell.
std::string XDGBasedAppProvider::EscapeArgForShell(const std::string& arg)
{
	if (arg.empty()) {
		return "''"; // An empty string is represented as '' in the shell.
	}

	bool is_safe = true;
	for (const unsigned char c : arg) {
		// Perform a locale-independent check for 7-bit ASCII characters.
		if (!((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9')
			  || c == '/' || c == '.' || c == '_' || c == '-'))
		{
			is_safe = false;
			break;
		}
	}

	if (is_safe) {
		return arg;
	}

	std::string out;
	out.push_back('\'');
	for (const unsigned char uc : arg) {
		if (uc == '\'') {
			// A single quote cannot be escaped inside a single-quoted string.
			// The standard method is to end the string (''), add an escaped quote (\'),
			// and start a new string (''). For example, "it's" becomes 'it'\''s'.
			out.append("'\\''");
		} else {
			out.push_back(static_cast<char>(uc));
		}
	}
	out.push_back('\'');
	return out;
}


// Extracts the basename (filename) from a full path.
std::string XDGBasedAppProvider::GetBaseName(const std::string& path)
{
	size_t pos = path.find_last_of('/');
	if (pos == std::string::npos) {
		return path;
	}
	return path.substr(pos + 1);
}


// Checks if a path is a regular file (S_ISREG) AND is readable (R_OK).
bool XDGBasedAppProvider::IsReadableFile(const std::string& path)
{
	struct stat st;
	// Use stat() to follow symlinks
	if (stat(path.c_str(), &st) == 0) {
		// Check type first, then check permissions
		if (S_ISREG(st.st_mode)) {
			return (access(path.c_str(), R_OK) == 0);
		}
	}
	return false;
}


// Checks if a path is a directory (S_ISDIR) AND is traversable (X_OK).
bool XDGBasedAppProvider::IsTraversableDirectory(const std::string& path)
{
	struct stat st;
	// Use stat() to follow symlinks
	if (stat(path.c_str(), &st) == 0) {
		// Check type first, then check permissions
		if (S_ISDIR(st.st_mode)) {
			return (access(path.c_str(), X_OK) == 0);
		}
	}
	return false;
}


// ****************************** RAII cache helper ******************************

// Constructor for the RAII helper. This populates all operation-scoped class fields.
XDGBasedAppProvider::OperationContext::OperationContext(XDGBasedAppProvider& p) : provider(p)
{
	// 1. Load MIME-related databases
	if (provider._load_mimetype_aliases) {
		provider._op_alias_to_canonical_map = provider.LoadMimeAliases();
		// Build the reverse (canonical -> alias) map once for the entire operation
		provider._op_canonical_to_aliases_map.emplace();
		for (const auto& [alias, canonical] : provider._op_alias_to_canonical_map.value()) {
			provider._op_canonical_to_aliases_map.value()[canonical].push_back(alias);
		}
	}
	if (provider._load_mimetype_subclasses) {
		provider._op_subclass_to_parent_map = provider.LoadMimeSubclasses();
	}

	// 2. Load system paths and association files
	provider._op_desktop_paths = provider.GetDesktopFileSearchPaths();
	auto mimeapps_paths = provider.GetMimeappsListSearchPaths();
	provider._op_mimeapps_lists_data = provider.ParseMimeappsLists(mimeapps_paths);
	provider._op_current_desktop_env = provider._filter_by_show_in ? provider.GetEnv("XDG_CURRENT_DESKTOP", "") : "";

	// 3. Build the primary application lookup cache
	// We build *either* the mimeinfo.cache or the full mime-to-app index, based on settings.
	if (provider._use_mimeinfo_cache) {
		// Attempt to load from mimeinfo.cache first, as per user setting.
		provider._op_mime_to_handlers_map = XDGBasedAppProvider::ParseAllMimeinfoCacheFiles(provider._op_desktop_paths.value());

		// Fallback: If the cache is empty (files not found or all are empty),
		// reset the optional. This will trigger the full scan logic below.
		if (provider._op_mime_to_handlers_map.value().empty()) {
			provider._op_mime_to_handlers_map.reset();
		}
	}

	// If caching is disabled (by user) OR the fallback was triggered (cache was empty)...
	if (!provider._op_mime_to_handlers_map.has_value()) {
		// ...populate the index by performing a full scan.
		// This call will populate the _desktop_entry_cache as a side effect.
		provider._op_mime_to_desktop_entry_map = provider.FullScanDesktopFilesAndBuildIndex(provider._op_desktop_paths.value());
	}
}


// Destructor for the RAII helper. This clears all operation-scoped class fields.
XDGBasedAppProvider::OperationContext::~OperationContext()
{
	provider._op_alias_to_canonical_map.reset();
	provider._op_canonical_to_aliases_map.reset();
	provider._op_subclass_to_parent_map.reset();
	provider._op_mimeapps_lists_data.reset();
	provider._op_desktop_paths.reset();
	provider._op_current_desktop_env.reset();
	provider._op_mime_to_handlers_map.reset();
	provider._op_mime_to_desktop_entry_map.reset();
}

#endif
