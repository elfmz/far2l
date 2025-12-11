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
#include <set>
#include <utility>
#include <sys/types.h>


class XDGBasedAppProvider : public AppProvider
{

public:

	explicit XDGBasedAppProvider(TMsgGetter msg_getter);
	std::vector<CandidateInfo> GetAppCandidates(const std::vector<std::wstring>& filepaths_wide) override;
	std::vector<std::wstring> ConstructLaunchCommands(const CandidateInfo& candidate, const std::vector<std::wstring>& filepaths_wide) override;
	std::vector<std::wstring> GetMimeTypes() override;
	std::vector<Field> GetCandidateDetails(const CandidateInfo& candidate) override;

	// Platform-specific settings API
	std::vector<ProviderSetting> GetPlatformSettings() override;
	void SetPlatformSettings(const std::vector<ProviderSetting>& settings) override;
	void LoadPlatformSettings() override;
	void SavePlatformSettings() override;

private:

	// ******************************************************************************
	// Group 1: Core XDG Desktop Entry Types
	// Structures representing the content and logic of .desktop files.
	// ******************************************************************************

	// Defines how the application expects arguments based on Field Codes in the Exec key.
	enum class ExecutionModel
	{
		LegacyImplicit, // no field codes found; append files to the end of the command.
		PerFile,        // %f or %u found; launch a separate process for each file.
		FileList        // %F or %U found; pass all files as a list to a single process.
	};


	// Represents a parsed argument template from the Exec key, potentially containing field codes.
	struct ExecArgTemplate
	{
		std::string value;
		bool is_quoted_literal = false; // if true, field codes inside this argument must be ignored.
	};


	// Represents a parsed .desktop file from the XDG specifications.
	struct DesktopEntry
	{
		std::string id;
		std::string desktop_filepath;
		std::string name;
		std::string generic_name;
		std::string comment;
		std::string categories;
		std::string exec;
		std::string try_exec;
		std::string mimetype;
		std::string only_show_in;
		std::string not_show_in;
		std::string terminal;

		// Mutable cache for lazy parsing: analysis is performed only if the application is selected as a candidate.
		mutable bool is_exec_parsed = false;
		mutable ExecutionModel execution_model = ExecutionModel::LegacyImplicit;
		mutable std::vector<ExecArgTemplate> arg_templates;
	};


	// ******************************************************************************
	// Group 2: MIME Detection Types
	// Structures used for identifying file types before application lookup.
	// ******************************************************************************

	// Represents the "raw" MIME profile of a file, derived from all available detection tools before any expansion.
	struct RawMimeProfile
	{
		std::string xdg_mime;		// result from 'xdg-mime query filetype'
		std::string file_mime;		// result from 'file --mime-type'
		std::string magika_mime;	// result from 'magika --format '%m''
		std::string globs2_mime;	// result from XDG glob rules matching (globs2)
		std::string ext_mime;		// result from internal extension fallback map
		std::string stat_mime;		// result from internal stat() analysis (e.g., inode/directory)

		bool is_regular_file;		// true if S_ISREG

		bool operator==(const RawMimeProfile& other) const
		{
			return std::tie(is_regular_file, xdg_mime, file_mime, magika_mime, globs2_mime, ext_mime, stat_mime) ==
				   std::tie(other.is_regular_file, other.xdg_mime, other.file_mime, other.magika_mime, other.globs2_mime, other.ext_mime, other.stat_mime);
		}

		// Custom hash function to allow RawMimeProfile to be used as a key in std::unordered_map.
		struct Hash
		{
			std::size_t operator()(const RawMimeProfile& s) const noexcept
			{
				auto hash_combine = [](std::size_t& seed, std::size_t hash_val) {
					seed ^= hash_val + 0x9e3779b9 + (seed << 6) + (seed >> 2);
				};
				std::size_t seed = std::hash<std::string>{}(s.xdg_mime);
				hash_combine(seed, std::hash<std::string>{}(s.file_mime));
				hash_combine(seed, std::hash<std::string>{}(s.magika_mime));
				hash_combine(seed, std::hash<std::string>{}(s.globs2_mime));
				hash_combine(seed, std::hash<std::string>{}(s.ext_mime));
				hash_combine(seed, std::hash<std::string>{}(s.stat_mime));
				hash_combine(seed, std::hash<bool>{}(s.is_regular_file));
				return seed;
			}
		};
	};


	// Represents a single pattern matching rule from parsed 'globs2' file.
	struct GlobRule
	{
		int weight;
		std::string mime_type;
		std::string pattern;
		bool case_sensitive;
		int source_rank;
		bool is_literal;

		// Defines the sorting order for glob matching.
		// Returns true if 'this' rule should be checked BEFORE 'other'.
		bool operator<(const GlobRule& other) const
		{
			// Higher weights are checked first.
			if (weight != other.weight)
			{
				return weight > other.weight;
			}

			// Exact filenames (e.g., 'Makefile') take precedence over globs (*.txt).
			if (is_literal != other.is_literal)
			{
				return is_literal;
			}

			// Longer patterns are more specific (e.g., '*.tar.gz' > '*.gz').
			if (pattern.length() != other.pattern.length())
			{
				return pattern.length() > other.pattern.length();
			}

			// User-defined rules (higher rank) override system rules.
			return source_rank > other.source_rank;
		}
	};


	// ******************************************************************************
	// Group 3: Association Database Types
	// Structures representing parsed data from 'mimeapps.list', 'mimeinfo.cache', etc.
	// ******************************************************************************

	// Links a Desktop ID to the source file ('mimeapps.list' or 'mimeinfo.cache') defining the association.
	struct DesktopAssociation
	{
		std::string desktop_id;
		std::string source_filepath;

		DesktopAssociation() = default;
		DesktopAssociation(const std::string& did, const std::string& sfp)
			: desktop_id(did), source_filepath(sfp) {}
	};


	// Represents the merged content of all parsed 'mimeapps.list' files found in XDG directories.
	struct MimeappsListsConfig
	{
		// MIME type -> default application from [Default Applications].
		std::unordered_map<std::string, DesktopAssociation> defaults;
		// MIME type -> list of apps from [Added Associations].
		std::unordered_map<std::string, std::vector<DesktopAssociation>> added;
		// MIME type -> set of apps from [Removed Associations].
		std::unordered_map<std::string, std::unordered_set<std::string>> removed;
	};


	// ******************************************************************************
	// Group 4: Ranking & Candidate Identification
	// Structures used in the logic for selecting and sorting the best applications.
	// ******************************************************************************

	// Constants for the tiered ranking system.
	struct Ranking
	{
		// Specificity determines the base tier (Specific MIME > Generic MIME > Fallback).
		static constexpr int SPECIFICITY_MULTIPLIER = 100;

		// Within the same specificity tier, the source determines priority.
		static constexpr int SOURCE_RANK_GLOBAL_DEFAULT = 5;   // 'xdg-mime query default'
		static constexpr int SOURCE_RANK_MIMEAPPS_DEFAULT = 4; // [Default Applications] in 'mimeapps.list'
		static constexpr int SOURCE_RANK_MIMEAPPS_ADDED = 3;   // [Added Associations] in 'mimeapps.list'
		static constexpr int SOURCE_RANK_CACHE_OR_SCAN = 2;    // 'mimeinfo.cache' or full .desktop scan
	};


	// Holds a non-owning pointer to a cached DesktopEntry and its calculated rank.
	// Used for sorting candidates before display.
	struct RankedCandidate
	{
		const DesktopEntry* desktop_entry = nullptr;
		int rank = 0;
		std::string source_info;

		bool operator<(const RankedCandidate& other) const {
			if (rank != other.rank) {
				return rank > other.rank; // Descending by rank.
			}
			if (desktop_entry && other.desktop_entry) {
				return desktop_entry->name < other.desktop_entry->name;	// Ascending by name.
			}
			return desktop_entry == nullptr && other.desktop_entry != nullptr;
		}
	};


	// Temporary structure to track the best score found for a specific Desktop ID
	// while iterating through multiple MIME types (specific -> generic).
	struct AssociationScore
	{
		int rank;
		std::string source_info;
		AssociationScore(int r, std::string s) : rank(r), source_info(std::move(s)) {}
	};


	// Unique identifier for a candidate in the final map.
	// Used to deduplicate identical applications that might be defined in different .desktop files.
	struct AppUniqueKey
	{
		std::string_view name;
		std::string_view exec;

		[[nodiscard]] bool operator==(const AppUniqueKey& other) const {
			return name == other.name && exec == other.exec;
		}

		// Custom hash function to allow AppUniqueKey to be used as a key in std::unordered_map.
		struct Hash
		{
			size_t operator()(const AppUniqueKey& k) const {
				const auto h1 = std::hash<std::string_view>{}(k.name);
				const auto h2 = std::hash<std::string_view>{}(k.exec);
				return h1 ^ (h2 + 0x9e3779b9 + (h1 << 6) + (h1 >> 2));
			}
		};
	};





	// ******************************************************************************
	// Group 5: Plugin Internal Configuration
	// ******************************************************************************

	// A helper struct to define a platform setting, linking its INI key,
	// localized UI display name, its corresponding class member variable, and default value.

	struct PlatformSettingDefinition {
		std::string key;
		LanguageID  display_name_id;
		bool XDGBasedAppProvider::* member_variable;
		bool default_value;
		bool affects_candidates;
	};


	// ******************************************************************************
	// Group 6: Lifecycle & State Management
	// ******************************************************************************

	// RAII helper to manage "Operation-Scoped" state.
	// Caches expensive lookups (like parsing all .desktop files or checking tool existence)
	// for the duration of a single user action (GetAppCandidates), then releases them.
	// This avoids passing massive context structures through every function.

	struct OperationContext
	{
		XDGBasedAppProvider& provider;
		OperationContext(XDGBasedAppProvider& p);
		~OperationContext();
	};
	friend struct OperationContext;


	// ******************************************************************************
	// ALIASES
	// ******************************************************************************

	using CandidateMap = std::unordered_map<AppUniqueKey, RankedCandidate, AppUniqueKey::Hash>;
	using MimeToDesktopEntryIndex = std::unordered_map<std::string, std::vector<const DesktopEntry*>>;
	using MimeToDesktopAssociationsMap = std::unordered_map<std::string, std::vector<DesktopAssociation>>;
	using VisitedInodeSet = std::set<std::pair<dev_t, ino_t>>;


	// ******************************************************************************
	// METHODS
	// ******************************************************************************

	// --- Searching and ranking candidates logic ---
	CandidateMap DiscoverCandidatesForExpandedMimes(const std::vector<std::string>& expanded_mimes);
	std::string QuerySystemDefaultApplication(const std::string& mime);
	void AppendCandidatesFromMimeAppsLists(const std::vector<std::string>& expanded_mimes, CandidateMap& unique_candidates);
	void AppendCandidatesFromMimeinfoCache(const std::vector<std::string>& expanded_mimes, CandidateMap& unique_candidates);
	void AppendCandidatesFromDesktopEntryIndex(const std::vector<std::string>& expanded_mimes, CandidateMap& unique_candidates);
	void RegisterCandidateById(CandidateMap& unique_candidates, const std::string& desktop_id, int rank, const std::string& source_info);
	void RegisterCandidateFromDesktopEntry(CandidateMap& unique_candidates, const DesktopEntry& desktop_entry, int rank, const std::string& source_info);
	void AddOrUpdateCandidate(CandidateMap& unique_candidates, const DesktopEntry& desktop_entry, int rank, const std::string& source_info);
	bool IsAssociationRemoved(const std::string& mime, const std::string& desktop_id);
	std::vector<RankedCandidate> BuildSortedRankedCandidatesList(const CandidateMap& candidate_map);
	std::vector<CandidateInfo> FormatCandidatesForUI(const std::vector<RankedCandidate>& ranked_candidates, bool store_source_info);
	static CandidateInfo ConvertDesktopEntryToCandidateInfo(const DesktopEntry& desktop_entry);

	// --- File MIME type detection & expansion ---
	RawMimeProfile GetRawMimeProfile(const std::string& filepath);
	std::vector<std::string> ExpandAndPrioritizeMimeTypes(const RawMimeProfile& profile);
	std::string DetectMimeTypeWithXdgMimeTool(const std::string& filepath_escaped);
	std::string DetectMimeTypeWithFileTool(const std::string& filepath_escaped);
	std::string DetectMimeTypeWithMagikaTool(const std::string& filepath_escaped);
	std::string DetectMimeTypeViaGlobRules(const std::string& filepath);
	static bool GlobMatch(const std::string &text, const std::string &pattern, bool case_sensitive);
	std::string GuessMimeTypeByExtension(const std::string& filepath);

	// --- XDG database parsing & caching ---
	std::unordered_map<std::string, std::string> IndexAllDesktopFiles();
	void IndexDirectoryRecursively(std::unordered_map<std::string, std::string>& desktop_id_to_path_map, const std::string& current_path, const std::string& base_dir_prefix, VisitedInodeSet& visited_inodes);
	const std::optional<XDGBasedAppProvider::DesktopEntry>& GetOrLoadDesktopEntry(const std::string& desktop_id);
	MimeToDesktopEntryIndex ParseAllDesktopFiles();
	MimeToDesktopAssociationsMap ParseAllMimeinfoCacheFiles();
	static void ParseMimeinfoCache(const std::string& filepath, MimeToDesktopAssociationsMap& mime_to_desktop_associations_map);
	MimeappsListsConfig ParseMimeappsLists();
	static void ParseMimeappsList(const std::string& filepath, MimeappsListsConfig& mimeapps_lists, std::unordered_set<std::string>& blacklist);
	std::optional<XDGBasedAppProvider::DesktopEntry> ParseDesktopFile(const std::string& filepath);
	std::string GetLocalizedValue(const std::unordered_map<std::string, std::string>& kv_entries, const std::string& base_key) const;
	static std::vector<std::string> GenerateLocaleSuffixes();
	std::unordered_map<std::string, std::string> LoadMimeAliases();
	static std::string_view GetMajorMimeType(const std::string& mime);
	std::unordered_map<std::string, std::string> LoadMimeSubclasses();
	std::vector<GlobRule> LoadGlobRules();
	void ParseGlobs2File(const std::string& filepath, std::vector<GlobRule>& rules, int source_rank);
	bool IsLiteralPattern(const std::string& pattern);
	static std::vector<std::string> GetDesktopFileSearchDirpaths();
	std::vector<std::string> GetMimeappsListSearchFilepaths();
	static std::vector<std::string> GetMimeDatabaseSearchDirpaths();

	// --- Launch command constructing ---
	static void AnalyzeExecLine(const DesktopEntry& desktop_entry);
	static std::vector<XDGBasedAppProvider::ExecArgTemplate> TokenizeExecString(const std::string& exec_value);
	std::string AssembleLaunchCommand(const DesktopEntry& desktop_entry, const std::vector<std::string>& files) const;
	std::vector<std::string> ExpandFieldCodes(const ExecArgTemplate& arg_template, const std::vector<std::string>& files, const DesktopEntry& desktop_entry) const;
	static std::string PathToUri(const std::string &path);
	static std::string UnescapeGKeyFileString(const std::string& str);

	// --- System, environment and common helpers ---
	static bool IsReadableFile(const std::string& filepath);
	static bool IsTraversableDirectory(const std::string& dirpath);
	static bool IsExecutableAvailable(const std::string& command);
	static std::string RunCommandAndCaptureOutput(const std::string& cmd);
	static std::string GetEnv(const char* var, const char* default_val = "");
	static std::string EscapeArgForShell(const std::string& arg);
	static std::string GetBaseName(const std::string& filepath);
	static std::string Trim(const std::string& str);
	static std::vector<std::string> SplitString(const std::string& str, char delimiter);
	static std::string ToLowerASCII(std::string str);

	// ******************************************************************************
	// DATA MEMBERS
	// ******************************************************************************

	// CRITICAL: Must use std::map (not std::unordered_map) to guarantee pointer stability!
	// RankedCandidate holds non-owning pointers to DesktopEntry objects stored here,
	// which would be invalidated by unordered_map rehashing.
	std::map<std::string, std::optional<DesktopEntry>> _desktop_id_to_desktop_entry_cache;

	// Maps a candidate's ID to its source info string from the last GetAppCandidates call for the single selected file.
	// It's used by GetCandidateDetails to display where the association came from (e.g., 'mimeapps.list').
	std::map<std::wstring, std::string> _last_candidates_source_info;

	// Caches all unique RawMimeProfile objects collected during the last GetAppCandidates call.
	// This is used by GetMimeTypes to avoid redundant work.
	std::unordered_set<RawMimeProfile, RawMimeProfile::Hash> _last_unique_mime_profiles;

	// --- Platform-specific settings (values are loaded from INI) ---
	bool _use_xdg_mime_tool;
	bool _use_file_tool;
	bool _use_magika_tool;
	bool _use_glob_rules;
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
	std::map<std::wstring, bool XDGBasedAppProvider::*> _key_wide_to_member_map;

	// Maps internal setting keys to required command-line tools.
	// Used to disable settings in the UI if the required tool is missing.
	using ToolKeyMap = std::map<std::string, std::string>;

	inline static const ToolKeyMap s_tool_key_map = {
		{ "UseXdgMimeTool", "xdg-mime" },
		{ "UseFileTool", "file" },
		{ "UseMagikaTool", "magika" }
	};

	// --- Operation-Scoped State ---
	// Fields managed by OperationContext. Valid only during a GetAppCandidates call.
	std::vector<std::string> _op_locale_suffixes;
	std::vector<std::string> _op_current_desktop_names;  // from $XDG_CURRENT_DESKTOP
	bool _op_xdg_mime_exists = false;
	bool _op_file_tool_enabled_and_exists = false;
	bool _op_magika_tool_enabled_and_exists = false;
	std::vector<std::string> _op_desktop_file_dirpaths;
	std::vector<std::string> _op_mimeapps_list_filepaths;
	std::vector<std::string> _op_mime_database_dirpaths;
	std::unordered_map<std::string, std::string> _op_desktop_id_to_path_index;
	std::vector<GlobRule> _op_glob_rules_cache; // from 'globs2'
	std::unordered_map<std::string, std::string> _op_alias_to_canonical_cache;  // from 'aliases'
	std::unordered_map<std::string, std::vector<std::string>> _op_canonical_to_aliases_cache;  // from 'aliases'
	std::unordered_map<std::string, std::string> _op_subclass_to_parent_cache;  // from 'subclasses'
	MimeappsListsConfig _op_mimeapps_lists_cache;  // from 'mimeapps.list'
	std::map<std::string, std::string> _op_mime_to_default_desktop_id_cache;  // from 'xdg-mime query default'
	MimeToDesktopAssociationsMap _op_mime_to_desktop_associations_index; 	// from 'mimeinfo.cache'
	MimeToDesktopEntryIndex _op_mime_to_desktop_entry_index;  // from full .desktop scan
};

#endif
