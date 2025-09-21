#pragma once

#if defined (__linux__) || defined(__FreeBSD__) || defined(__OpenBSD__) || defined(__NetBSD__) || defined(__DragonFly__)

#include "AppProvider.hpp"
#include "common.hpp"
#include <optional>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <map>


struct Token
{
	std::string text;
	bool quoted;
	bool single_quoted;
};


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


class LinuxAppProvider : public AppProvider
{
public:
	explicit LinuxAppProvider(TMsgGetter msg_getter);
	std::vector<CandidateInfo> GetAppCandidates(const std::wstring& pathname) override;
	std::wstring ConstructCommandLine(const CandidateInfo& candidate, const std::wstring& pathname) override;
	std::wstring GetMimeType(const std::wstring& pathname) override;

	std::vector<Field> GetCandidateDetails(const CandidateInfo& candidate) override;

	std::vector<ProviderSetting> GetPlatformSettings() override;
	void SetPlatformSettings(const std::vector<ProviderSetting>& settings) override;
	void LoadPlatformSettings() override;
	void SavePlatformSettings() override;

private:
	struct RankedCandidate {
		DesktopEntry entry;
		int rank = 0;

		bool operator<(const RankedCandidate& other) const {
			return rank > other.rank;
		}
	};

	struct MimeAssociation {
		std::unordered_map<std::string, std::string> defaults;
		std::unordered_map<std::string, std::vector<std::string>> added;
		std::unordered_map<std::string, std::unordered_set<std::string>> removed;
	};

	std::map<std::string, std::optional<DesktopEntry>> _desktop_entry_cache;

	bool _filter_by_show_in = false;
	bool _validate_try_exec = false;
	bool _use_mimeinfo_cache = true;
	bool _use_extension_based_fallback = true;
	bool _use_xdg_mime_tool = true;
	bool _use_file_tool = true;


	std::vector<std::string> CollectAndPrioritizeMimeTypes(const std::string& pathname);
	std::string MimeTypeFromXdgMimeTool(const std::string& escaped_pathname);
	std::string MimeTypeFromFileTool(const std::string& escaped_pathname);
	std::string MimeTypeByExtension(const std::string& escaped_pathname);

	static std::string GetEnv(const char* var, const char* default_val = "");
	static std::vector<std::string> SplitString(const std::string& str, char delimiter);
	static bool CheckExecutable(const std::string& path);
	static const std::optional<DesktopEntry>& GetCachedDesktopEntry(const std::string& desktop_file, const std::vector<std::string>& search_paths, std::map<std::string, std::optional<DesktopEntry>>& cache);
	static std::vector<std::string> GetDesktopFileSearchPaths();
	static std::vector<std::string> GetMimeappsListSearchPaths();
	static MimeAssociation ParseMimeappsLists(const std::vector<std::string>& paths);
	static void ParseMimeappsList(const std::string& path, MimeAssociation& associations);
	static void ParseMimeinfoCache(const std::string& path, std::unordered_map<std::string, std::vector<std::string>>& mime_cache);
	static std::string GetDefaultApp(const std::string& mime_type);
	static bool IsValidDir(const std::string& path);
	static bool IsReadableFile(const std::string &path);
	static std::optional<DesktopEntry> ParseDesktopFile(const std::string& path);
	static std::string GetLocalizedValue(const std::unordered_map<std::string, std::string>& values, const std::string& key);
	static std::string RunCommandAndCaptureOutput(const std::string& cmd);
	static std::string Trim(std::string str);
	static bool IsDesktopWhitespace(char c);
	static std::vector<Token> TokenizeDesktopExec(const std::string& str);
	static std::string UndoEscapes(const Token& token);
	static std::string EscapePathForShell(const std::string& path);
	static bool ExpandFieldCodes(const DesktopEntry& candidate, const std::string& pathname, const std::string& unescaped, std::vector<std::string>& out_args);
	static std::string EscapeArg(const std::string& arg);
	static std::string GetBaseName(const std::string& path);
	static CandidateInfo ConvertDesktopEntryToCandidateInfo(const DesktopEntry& desktop_entry);
};

#endif
