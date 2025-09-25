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
	std::vector<CandidateInfo> GetAppCandidates(const std::wstring& pathname) override;
	std::wstring ConstructCommandLine(const CandidateInfo& candidate, const std::wstring& pathname) override;
	std::wstring GetMimeType(const std::wstring& pathname) override;
	std::vector<Field> GetCandidateDetails(const CandidateInfo& candidate) override;

	std::vector<ProviderSetting> GetPlatformSettings() override;
	void SetPlatformSettings(const std::vector<ProviderSetting>& settings) override;
	void LoadPlatformSettings() override;
	void SavePlatformSettings() override;

private:

	// A single-source-of-truth definition for a platform-specific setting.
	struct PlatformSettingDefinition {

		std::string key;
		LanguageID  display_name_id;
		bool XDGBasedAppProvider::* member_variable;
		bool default_value;
	};

	// Holds a non-owning pointer to a cached DesktopEntry and its calculated rank.
	// The operator< is overloaded to sort candidates in descending order of rank.
	struct RankedCandidate {
		const DesktopEntry* entry = nullptr;
		int rank = 0;

		bool operator<(const RankedCandidate& other) const {
			return rank > other.rank;
		}
	};

	// Represents the combined associations from all parsed mimeapps.list files.
	struct MimeAssociation
	{
		// MIME type -> default application (.desktop file
		std::unordered_map<std::string, std::string> defaults;
		// MIME type -> list of additionally associated applications
		std::unordered_map<std::string, std::vector<std::string>> added;
		// MIME type -> set of applications that should not be associated
		std::unordered_map<std::string, std::unordered_set<std::string>> removed;
	};

	// A key for the unique_candidates map
	struct AppUniqueKey
	{
		std::string_view name;
		std::string_view exec;

		[[nodiscard]] bool operator==(const AppUniqueKey& other) const {
			return name == other.name && exec == other.exec;
		}
	};

	// Custom hash function for AppUniqueKey
	struct AppUniqueKeyHash
	{
		std::size_t operator()(const AppUniqueKey& k) const {
			const auto h1 = std::hash<std::string_view>{}(k.name);
			const auto h2 = std::hash<std::string_view>{}(k.exec);
			return h1 ^ (h2 << 1);
		}
	};

	// A "parameter object" that bundles all the state needed for a single app search operation.
	// This avoids passing many individual arguments between internal helper functions.
	struct CandidateSearchContext
	{
		// Stores unique candidates to avoid duplicates in the final list.
		// The key distinguishes different applications that might have the same name.
		std::unordered_map<AppUniqueKey, RankedCandidate, AppUniqueKeyHash> unique_candidates;
		const std::vector<std::string>& prioritized_mimes;
		const MimeAssociation& associations;
		const std::vector<std::string>& desktop_paths;
		const std::string& current_desktop_env;

		CandidateSearchContext(
			const std::vector<std::string>& mimes,
			const MimeAssociation& assocs,
			const std::vector<std::string>& paths,
			const std::string& env)
			: prioritized_mimes(mimes), associations(assocs), desktop_paths(paths), current_desktop_env(env) {}
	};


	// Represents a single token from the Exec= command line, preserving quoting info.
	struct ExecToken
	{
		std::string text;
		bool quoted;
		bool single_quoted;
	};


	// Searching and ranking candidates logic
	void FindCandidatesFromMimeLists(CandidateSearchContext& context, const std::string& global_default_app);
	void FindCandidatesFromCache(CandidateSearchContext& context, const std::unordered_map<std::string, std::vector<std::string>>& mime_cache);
	void FindCandidatesByFullScan(CandidateSearchContext& context);
	void ValidateAndRegisterCandidate(CandidateSearchContext& context, const std::string& app_desktop_file, int rank);
	void AddOrUpdateCandidate(CandidateSearchContext& context, const DesktopEntry& entry, int rank);

	// MIME types detection
	std::vector<std::string> CollectAndPrioritizeMimeTypes(const std::string& pathname);
	std::string MimeTypeFromXdgMimeTool(const std::string& escaped_pathname);
	std::string MimeTypeFromFileTool(const std::string& escaped_pathname);
	std::string MimeTypeByExtension(const std::string& escaped_pathname);

	// Parsing XDG files and data
	static const std::optional<DesktopEntry>& GetCachedDesktopEntry(const std::string& desktop_file, const std::vector<std::string>& search_paths, std::map<std::string, std::optional<DesktopEntry>>& cache);
	static std::optional<DesktopEntry> ParseDesktopFile(const std::string& path);
	static void ParseMimeappsList(const std::string& path, MimeAssociation& associations);
	static MimeAssociation ParseMimeappsLists(const std::vector<std::string>& paths);
	static void ParseMimeinfoCache(const std::string& path, std::unordered_map<std::string, std::vector<std::string>>& mime_cache);
	static std::string GetLocalizedValue(const std::unordered_map<std::string, std::string>& values, const std::string& base_key);

	// Command line constructing
	static std::vector<ExecToken> TokenizeDesktopExec(const std::string& str);
	static bool ExpandFieldCodes(const DesktopEntry& candidate, const std::string& pathname, const std::string& unescaped, std::vector<std::string>& out_args);
	static std::string UndoEscapes(const ExecToken& token);
	static std::string EscapeArg(const std::string& arg);
	static bool IsDesktopWhitespace(char c);

	// Paths and the system environment helpers
	static std::vector<std::string> GetDesktopFileSearchPaths();
	static std::vector<std::string> GetMimeappsListSearchPaths();
	static std::string GetDefaultApp(const std::string& mime_type);
	static bool CheckExecutable(const std::string& path);
	static std::string GetEnv(const char* var, const char* default_val = "");

	// Common helper functions
	static std::string Trim(std::string str);
	static std::vector<std::string> SplitString(const std::string& str, char delimiter);
	static std::string EscapePathForShell(const std::string& path);
	static std::string GetBaseName(const std::string& path);
	static bool IsValidDir(const std::string& path);
	static bool IsReadableFile(const std::string &path);
	static std::string RunCommandAndCaptureOutput(const std::string& cmd);
	static CandidateInfo ConvertDesktopEntryToCandidateInfo(const DesktopEntry& desktop_entry);

	// This cache owns all DesktopEntry objects for the duration of a GetAppCandidates call.
	// RankedCandidate pointers will point to the entries stored here.
	std::map<std::string, std::optional<DesktopEntry>> _desktop_entry_cache;

	// Platform-specific settings.
	bool _filter_by_show_in;
	bool _validate_try_exec;
	bool _use_mimeinfo_cache;
	bool _use_extension_based_fallback;
	bool _use_xdg_mime_tool;
	bool _use_file_tool;

	// Holds all setting definitions. Initialized once in the constructor.
	std::vector<PlatformSettingDefinition> _platform_settings_definitions;

	// A pre-calculated lookup map for efficient updates in SetPlatformSettings.
	std::map<std::wstring, bool XDGBasedAppProvider::*> _key_to_member_map;
};

#endif
