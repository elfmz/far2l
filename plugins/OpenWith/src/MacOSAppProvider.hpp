#pragma once

#if defined(__APPLE__)

#include "AppProvider.hpp"
#include "common.hpp"
#include "lng.hpp"
#include <atomic>
#include <map>
#include <functional>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

class KeyFileReadHelper;
class KeyFileHelper;

namespace openwith
{
	class MacOSAppProvider : public AppProvider
	{
	public:
		MacOSAppProvider();
		GetCandidatesResult GetAppCandidates(const std::vector<std::wstring>& filepaths,  ProgressCallback progress = nullptr,
											 const std::atomic<bool>* cancel_flag = nullptr) override;
		std::vector<std::wstring> ConstructLaunchCommands(const CandidateInfo& candidate, const std::vector<std::wstring>& filepaths) override;
		std::vector<Field> GetCandidateDetails(const CandidateInfo& candidate) override;
		std::vector<std::wstring> GetFileTypes() override;
		std::vector<CandidateContextLocation> GetCandidateContextLocations(const CandidateInfo& candidate) override;

		void LoadPlatformSettings(const KeyFileReadHelper &key_reader) override;
		void SavePlatformSettings(KeyFileHelper& key_writer) override;
		std::vector<ProviderSetting> GetPlatformSettings() override;
		void SetPlatformSettings(const std::vector<ProviderSetting>& settings) override;

	private:

		struct AppBundleMetadata
		{
			std::string bundle_path;
			std::string bundle_display_name;
			std::string bundle_name;
			std::string bundle_short_version_string;
			std::string bundle_version;
			std::string bundle_executable;
			std::string bundle_identifier;
			std::string presentation_name;
			std::string presentation_version;
		};

		using CompatibleAppsMetadata = std::vector<const AppBundleMetadata*>;


		struct FileUtiRecord
		{
			std::string uti;              // UTI, or an empty string if the file is inaccessible.
			bool is_uti_resolved = false; // true if the file was accessible and its UTI was successfully resolved.
			bool operator==(const FileUtiRecord& other) const
			{
				return is_uti_resolved == other.is_uti_resolved && uti == other.uti;
			}

			struct Hash
			{
				std::size_t operator()(const FileUtiRecord& p) const noexcept
				{
					std::size_t h1 = std::hash<std::string>{}(p.uti);
					std::size_t h2 = std::hash<bool>{}(p.is_uti_resolved);
					std::size_t seed = h1;
					seed ^= h2 + 0x9e3779b9 + (seed << 6) + (seed >> 2);
					return seed;
				}
			};
		};


		struct ScoredCandidate
		{
			const AppBundleMetadata* metadata = nullptr;
			int supported_files_count = 0;
			int default_handler_count = 0;
			double avg_suitability_score = 0.0;
		};


		struct PlatformSettingDefinition
		{
			std::string internal_key;                  // persistent INI key and internal identifier
			MsgID display_name_id;                     // ID to fetch the localized UI label
			bool MacOSAppProvider::* member_variable;  // pointer to the linked boolean class member
			bool default_value;                        // fallback value if missing in the INI file
			bool affects_candidates;                   // true if changing this setting affects the contents or order of the candidate list
		};


		void ClearLastQueryCaches();
		const AppBundleMetadata* GetOrParseMetadata(const std::string& bundle_path);
		const CompatibleAppsMetadata& GetCachedCompatibleApps(const std::string& filepath, const std::string& uti);
		static std::string_view ExtractBaseName(std::string_view bundle_path);
		static std::string_view SelectFirstNonEmpty(std::initializer_list<std::string_view> items);
		static std::wstring QuoteArgForShell(const std::wstring& arg);

		std::map<std::wstring, bool MacOSAppProvider::*> _key_wide_to_member_map;
		std::vector<PlatformSettingDefinition> _platform_settings_definitions;

		std::unordered_set<FileUtiRecord, FileUtiRecord::Hash> _last_uti_records;
		std::unordered_map<std::string, std::optional<AppBundleMetadata>> _app_bundle_metadata_cache;
		std::unordered_map<std::string, CompatibleAppsMetadata> _uti_to_apps_cache;

		bool _show_uti_instead_of_mime;
		bool _respect_system_ranking;
		bool _sort_alphabetically;
	};
} // namespace openwith

#endif
