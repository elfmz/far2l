#pragma once

#if defined (__linux__) || defined(__FreeBSD__) || defined(__OpenBSD__) || defined(__NetBSD__) || defined(__DragonFly__)

#include "AppProvider.hpp"
#include "common.hpp"
#include "lng.hpp"
#include <optional>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <map>
#include <tuple>


// Represents the parsed data from a .desktop file, according to the XDG specification.
struct DesktopEntry
{
	std::string desktop_file;
	std::string name;
	std::string generic_name;
	std::string comment;
	std::string categories;
	std::string exec;
	std::string try_exec;
	std::string mimetype;
	std::string only_show_in;
	std::string not_show_in;
	bool terminal = false;
};


class XDGBasedAppProvider : public AppProvider
{

public:

	explicit XDGBasedAppProvider(TMsgGetter msg_getter);
	std::vector<CandidateInfo> GetAppCandidates(const std::vector<std::wstring>& pathnames) override;
	std::vector<std::wstring> ConstructCommandLine(const CandidateInfo& candidate, const std::vector<std::wstring>& pathnames) override;
	std::vector<std::wstring> GetMimeTypes() override;
	std::vector<Field> GetCandidateDetails(const CandidateInfo& candidate) override;

	// Platform-specific settings API
	std::vector<ProviderSetting> GetPlatformSettings() override;
	void SetPlatformSettings(const std::vector<ProviderSetting>& settings) override;
	void LoadPlatformSettings() override;
	void SavePlatformSettings() override;

private:

	// Represents the "raw" MIME profile of a file, derived from all available detection tools before any expansion.
	struct RawMimeProfile
	{
		// MIME type results from different tools
		std::string xdg_mime;  // result from xdg-mime query filetype
		std::string file_mime; // result from file --mime-type
		std::string ext_mime;  // result from internal extension fallback map
		std::string stat_mime; // result from internal stat() analysis (e.g., inode/directory)

		bool is_regular_file;           // True if S_ISREG

		bool operator==(const RawMimeProfile& other) const
		{
			// Compare all fields that define the profile
			return std::tie(is_regular_file, xdg_mime, file_mime, ext_mime, stat_mime) ==
				   std::tie(other.is_regular_file, other.xdg_mime, other.file_mime, other.ext_mime, other.stat_mime);
		}

		// Custom hash function to allow RawMimeProfile to be used as a key in std::unordered_map.
		struct Hash
		{
			std::size_t operator()(const RawMimeProfile& s) const noexcept
			{
				std::size_t h1 = std::hash<std::string>{}(s.xdg_mime);
				std::size_t h2 = std::hash<std::string>{}(s.file_mime);
				std::size_t h3 = std::hash<std::string>{}(s.ext_mime);
				std::size_t h4 = std::hash<std::string>{}(s.stat_mime);
				std::size_t h5 = std::hash<bool>{}(s.is_regular_file);

				// Combine hashes using a simple boost-like hash_combine
				std::size_t seed = h1;
				seed ^= h2 + 0x9e3779b9 + (seed << 6) + (seed >> 2);
				seed ^= h3 + 0x9e3779b9 + (seed << 6) + (seed >> 2);
				seed ^= h4 + 0x9e3779b9 + (seed << 6) + (seed >> 2);
				seed ^= h5 + 0x9e3779b9 + (seed << 6) + (seed >> 2);
				return seed;
			}
		};
	};


	// Constants for the tiered ranking system.
	struct Ranking
	{
		// A multiplier for MIME type specificity. Must be greater than the max source rank.
		// This ensures that specificity is the primary sorting factor.
		static constexpr int SPECIFICITY_MULTIPLIER = 100;

		// Ranks for association sources, from highest to lowest.
		static constexpr int SOURCE_RANK_GLOBAL_DEFAULT = 5;   // xdg-mime query default
		static constexpr int SOURCE_RANK_MIMEAPPS_DEFAULT = 4; // [Default Applications] in mimeapps.list
		static constexpr int SOURCE_RANK_MIMEAPPS_ADDED = 3;   // [Added Associations] in mimeapps.list
		static constexpr int SOURCE_RANK_CACHE_OR_SCAN = 2;    // mimeinfo.cache or full .desktop scan
	};

	// A helper struct to define a platform setting, linking its INI key,
	// localized UI display name, its corresponding class member variable, and default value.
	struct PlatformSettingDefinition {
		std::string key;
		LanguageID  display_name_id;
		bool XDGBasedAppProvider::* member_variable;
		bool default_value;
	};

	// Holds a non-owning pointer to a cached DesktopEntry and its calculated rank.
	struct RankedCandidate
	{
		const DesktopEntry* entry = nullptr;
		int rank = 0;
		std::string source_info;

		bool operator<(const RankedCandidate& other) const {
			if (rank != other.rank) {
				return rank > other.rank; // primary sort: descending by rank (highest rank first).
			}
			return entry && other.entry && entry->name < other.entry->name; // secondary sort: ascending by name
		}
	};

	// This struct represents a single association rule and links a handler's .desktop file
	// to the configuration file (e.g., mimeapps.list or mimeinfo.cache) that specified the rule.
	struct HandlerProvenance
	{
		std::string desktop_file;
		std::string source_path;
	};

	// Represents the combined associations from all parsed mimeapps.list files.
	struct MimeappsListsData
	{
		// MIME type -> default application from [Default Applications].
		std::unordered_map<std::string, HandlerProvenance> defaults;
		// MIME type -> list of apps from [Added Associations].
		std::unordered_map<std::string, std::vector<HandlerProvenance>> added;
		// MIME type -> set of apps from [Removed Associations].
		std::unordered_map<std::string, std::unordered_set<std::string>> removed;
	};

	// A key for the unique_candidates map to distinguish between different applications
	// that might have the same name but different Exec commands (e.g., from different .desktop files).
	struct AppUniqueKey
	{
		std::string_view name;
		std::string_view exec;

		[[nodiscard]] bool operator==(const AppUniqueKey& other) const {
			return name == other.name && exec == other.exec;
		}
	};

	// Custom hash function for AppUniqueKey.
	struct AppUniqueKeyHash
	{
		size_t operator()(const AppUniqueKey& k) const {
			const auto h1 = std::hash<std::string_view>{}(k.name);
			const auto h2 = std::hash<std::string_view>{}(k.exec);
			return h1 ^ (h2 << 1); // Simple combination hash
		}
	};

	using CandidateMap = std::unordered_map<AppUniqueKey, RankedCandidate, AppUniqueKeyHash>;
	using MimeToDesktopEntryIndex = std::unordered_map<std::string, std::vector<const DesktopEntry*>>;
	using MimeinfoCacheData = std::unordered_map<std::string, std::vector<HandlerProvenance>>;

	// --- Searching and ranking candidates logic ---
	CandidateMap ResolveMimesToCandidateMap(const std::vector<std::string>& prioritized_mimes);
	static std::string GetDefaultApp(const std::string& mime_type);
	void AppendCandidatesFromMimeAppsLists(const std::vector<std::string>& prioritized_mimes, CandidateMap& unique_candidates);
	void AppendCandidatesFromMimeinfoCache(const std::vector<std::string>& prioritized_mimes, CandidateMap& unique_candidates);
	void AppendCandidatesByFullScan(const std::vector<std::string>& prioritized_mimes, CandidateMap& unique_candidates);
	void RegisterCandidateById(CandidateMap& unique_candidates, const std::string& app_desktop_file, int rank, const std::string& source_info);
	void RegisterCandidateFromObject(CandidateMap& unique_candidates, const DesktopEntry& entry, int rank, const std::string& source_info);
	void AddOrUpdateCandidate(CandidateMap& unique_candidates, const DesktopEntry& entry, int rank, const std::string& source_info);
	bool IsAssociationRemoved(const std::string& mime_type, const std::string& app_desktop_file);
	std::vector<RankedCandidate> BuildSortedRankedCandidatesList(const CandidateMap& candidate_map);
	std::vector<CandidateInfo> FormatCandidatesForUI(const std::vector<RankedCandidate>& ranked_candidates, bool store_source_info);
	static CandidateInfo ConvertDesktopEntryToCandidateInfo(const DesktopEntry& desktop_entry);

	// --- File MIME Type Detection & Expansion ---
	RawMimeProfile GetRawMimeProfile(const std::string& pathname);
	std::vector<std::string> ExpandAndPrioritizeMimeTypes(const RawMimeProfile& profile);
	std::string MimeTypeFromXdgMimeTool(const std::string& pathname);
	std::string MimeTypeFromFileTool(const std::string& pathname);
	std::string MimeTypeByExtension(const std::string& pathname);

	// --- XDG Database Parsing & Caching ---
	const std::optional<DesktopEntry>& GetCachedDesktopEntry(const std::string& desktop_file);
	MimeToDesktopEntryIndex FullScanDesktopFilesAndBuildIndex(const std::vector<std::string>& search_paths);
	static MimeinfoCacheData ParseAllMimeinfoCacheFiles(const std::vector<std::string>& search_paths);
	static void ParseMimeinfoCache(const std::string& path, MimeinfoCacheData& mimeinfo_cache_data);
	static MimeappsListsData ParseMimeappsLists(const std::vector<std::string>& paths);
	static void ParseMimeappsList(const std::string& path, MimeappsListsData& mimeapps_lists_data);
	static std::optional<DesktopEntry> ParseDesktopFile(const std::string& path);
	static std::string GetLocalizedValue(const std::unordered_map<std::string, std::string>& values, const std::string& base_key);
	static std::unordered_map<std::string, std::string> LoadMimeAliases();
	static std::unordered_map<std::string, std::string> LoadMimeSubclasses();
	static std::vector<std::string> GetDesktopFileSearchPaths();
	static std::vector<std::string> GetMimeappsListSearchPaths();
	static std::vector<std::string> GetMimeDatabaseSearchPaths();

	// --- Command line constructing ---
	static std::vector<std::string> TokenizeExecString(const std::string& exec_str);
	static std::string UnescapeGeneralString(const std::string& raw_str);
	static bool ExpandFieldCodes(const DesktopEntry& candidate, const std::string& pathname, const std::string& unescaped, std::vector<std::string>& out_args, bool treat_urls_as_paths);
	static bool HasFieldCode(const std::string& exec, const std::string& codes_to_find);
	static std::string PathToUri(const std::string& path);

	// --- System & Environment Helpers ---
	static bool CheckExecutable(const std::string& path);
	static std::string GetEnv(const char* var, const char* default_val = "");
	static std::string RunCommandAndCaptureOutput(const std::string& cmd);

	// --- Common helper functions ---
	static std::string Trim(std::string str);
	static std::vector<std::string> SplitString(const std::string& str, char delimiter);
	static std::string EscapeArgForShell(const std::string& arg);
	static std::string GetBaseName(const std::string& path);
	static bool IsReadableFile(const std::string& path);
	static bool IsTraversableDirectory(const std::string& path);


	// WARNING: This cache is a std::map on purpose.
	// It owns all DesktopEntry objects for the duration of a GetAppCandidates call.
	// RankedCandidate pointers will point to the entries stored here.
	// This is safe ONLY because std::map guarantees pointer/reference stability on insertion,
	// meaning that pointers to existing elements are not invalidated when new elements are added.
	// DO NOT change this to std::unordered_map.
	// An unordered_map may rehash on insertion, which would invalidate all existing pointers
	// stored in RankedCandidate instances, leading to dangling pointers and undefined behavior.
	std::map<std::string, std::optional<DesktopEntry>> _desktop_entry_cache;

	// This cache maps a candidate's ID to its source info string from the last GetAppCandidates call.
	// It's used by GetCandidateDetails to display where the association came from (e.g., mimeapps.list).
	// This is only populated for single-file lookups.
	std::map<std::wstring, std::string> _last_candidates_source_info;

	// Caches all unique RawMimeProfile objects collected during the last GetAppCandidates call.
	// This is used by GetMimeTypes to avoid redundant work.
	std::unordered_set<RawMimeProfile, RawMimeProfile::Hash> _last_mime_profiles;

	// --- Platform-specific settings (values are loaded from INI) ---
	bool _use_xdg_mime_tool;
	bool _use_file_tool;
	bool _use_extension_based_fallback;
	bool _load_mimetype_aliases;
	bool _load_mimetype_subclasses;
	bool _resolve_structured_suffixes;
	bool _use_generic_mime_fallbacks;
	bool _show_universal_handlers;
	bool _use_mimeinfo_cache;
	bool _filter_by_show_in;
	bool _validate_try_exec;
	bool _sort_alphabetically;
	bool _treat_urls_as_paths;

	// Holds all setting definitions. Initialized once in the constructor.
	std::vector<PlatformSettingDefinition> _platform_settings_definitions;

	// A pre-calculated lookup map (Key -> MemberPtr) for efficient updates in SetPlatformSettings.
	std::map<std::wstring, bool XDGBasedAppProvider::*> _key_to_member_map;


	// --- Operation-Scoped State ---
	// These fields are managed by the OperationContext RAII helper.
	// They are populated once at the start of GetAppCandidates and cleared at the end
	// to avoid passing them as parameters through the entire call stack.

	std::optional<std::unordered_map<std::string, std::string>> _op_alias_to_canonical_map;
	std::optional<std::unordered_map<std::string, std::vector<std::string>>> _op_canonical_to_aliases_map;
	std::optional<std::unordered_map<std::string, std::string>> _op_subclass_to_parent_map;
	std::optional<MimeappsListsData> _op_mimeapps_lists_data;      // combined mimeapps.list data
	std::optional<std::vector<std::string>> _op_desktop_paths; // XDG .desktop file search paths
	std::optional<std::string> _op_current_desktop_env; // $XDG_CURRENT_DESKTOP

	// One of the following two caches will be populated based on settings.
	std::optional<MimeinfoCacheData> _op_mime_to_handlers_map;	// from mimeinfo.cache
	std::optional<MimeToDesktopEntryIndex> _op_mime_to_desktop_entry_map;	// from full .desktop scan

	// RAII helper to manage the lifecycle of the operation-scoped state.
	struct OperationContext
	{
		XDGBasedAppProvider& provider;
		OperationContext(XDGBasedAppProvider& p);
		~OperationContext();
	};
	friend struct OperationContext;
};

#endif
