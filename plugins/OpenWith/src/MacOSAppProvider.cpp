#if defined(__APPLE__)

#include "MacOSAppProvider.hpp"
#include "LaunchServicesBridge.hpp"
#include "common.hpp"
#include "lng.hpp"
#include "KeyFileHelper.h"
#include "WideMB.h"
#include <algorithm>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <utility>


namespace openwith
{
	constexpr const char* INI_SECTION_MACOS_PROVIDER = "Settings.MacOS";

	MacOSAppProvider::MacOSAppProvider()
	{
		_platform_settings_definitions = {
			{ "ShowUtiInsteadOfMime", MsgID::ShowUtiInsteadOfMime, &MacOSAppProvider::_show_uti_instead_of_mime, false, false},
			{ "RespectSystemRanking", MsgID::RespectSystemRanking, &MacOSAppProvider::_respect_system_ranking,   false, true},
			{ "SortAlphabetically",   MsgID::SortAlphabetically,   &MacOSAppProvider::_sort_alphabetically,      false, true}
		};

		for (const auto& def : _platform_settings_definitions) {
			_key_wide_to_member_map[StrMB2Wide(def.internal_key)] = def.member_variable;
			this->*(def.member_variable) = def.default_value;
		}
	}


	void MacOSAppProvider::LoadPlatformSettings(const KeyFileReadHelper &key_reader)
	{
		for (const auto& def : _platform_settings_definitions) {
			this->*(def.member_variable) = key_reader.GetInt(INI_SECTION_MACOS_PROVIDER, def.internal_key, def.default_value) != 0;
		}
	}


	void MacOSAppProvider::SavePlatformSettings(KeyFileHelper& key_writer)
	{
		for (const auto& def : _platform_settings_definitions) {
			key_writer.SetInt(INI_SECTION_MACOS_PROVIDER, def.internal_key, this->*(def.member_variable));
		}
	}


	std::vector<ProviderSetting> MacOSAppProvider::GetPlatformSettings()
	{
		std::vector<ProviderSetting> settings;
		settings.reserve(_platform_settings_definitions.size());
		constexpr bool is_disabled = false;

		for (const auto& def : _platform_settings_definitions) {
			settings.push_back({StrMB2Wide(def.internal_key), GetMsg(def.display_name_id), this->*(def.member_variable),
								is_disabled, def.affects_candidates});
		}
		return settings;
	}


	void MacOSAppProvider::SetPlatformSettings(const std::vector<ProviderSetting>& settings)
	{
		for (const auto& s : settings) {
			if (auto it = _key_wide_to_member_map.find(s.internal_key); it != _key_wide_to_member_map.end()) {
				this->*(it->second) = s.value;
			}
		}
	}


	AppProvider::GetCandidatesResult MacOSAppProvider::GetAppCandidates(const std::vector<std::wstring>& filepaths, ProgressCallback progress, const std::atomic<bool>* cancel_flag)
	{
		ClearLastQueryCaches();
		if (filepaths.empty()) {
			return {};
		}

		OperationGuard guard(*this, std::move(progress), cancel_flag);
		GetCandidatesResult result;

		try {
			std::unordered_map<std::string, ScoredCandidate> candidates_pool;
			std::unordered_set<std::string_view> bundle_paths_seen_for_file;

			ReportProgress({GetMsg(MsgID::IdentifyingUTIsDiscoveringApps), GetMsg(MsgID::PleaseWait)});
			const size_t files_total = filepaths.size();
			size_t files_processed = 0;

			for (const auto& filepath_wide : filepaths) {
				bundle_paths_seen_for_file.clear();

				wchar_t status_buf[256];
				swprintf(status_buf, std::size(status_buf), GetMsg(MsgID::ProcessingFiles), ++files_processed, files_total);
				ReportProgress({nullptr, status_buf});
				CheckCancellation();

				std::string filepath = StrWide2MB(filepath_wide);
				auto uti_opt = openwith::launch_services::ResolveFileUTI(filepath);

				static const std::string empty_string;
				const std::string& uti = uti_opt ? *uti_opt : empty_string;
				bool is_uti_resolved = uti_opt.has_value();

				_last_uti_records.insert({ uti, is_uti_resolved });

				if (!is_uti_resolved || uti.empty()) {
					continue;
				}

				// Fetch default app PER FILE to respect user overrides for specific files.
				auto default_bundle_path_opt = openwith::launch_services::GetDefaultBundlePath(filepath);
				const AppBundleMetadata* default_app_metadata = nullptr;
				if (default_bundle_path_opt) {
					default_app_metadata = GetOrParseMetadata(*default_bundle_path_opt);
				}

				const auto& compatible_apps = GetCachedCompatibleApps(filepath, uti);

				// ---------- Per-file scoring and accumulation ----------
				// The bundle_paths_seen_for_file set prevents scoring the same app twice within a single file
				// (deduplication against the default app potentially appearing in both lists).

				auto score_app_for_file = [&](const AppBundleMetadata* metadata, bool is_default, size_t handler_index, size_t total_handlers) {
					if (!metadata || (metadata->bundle_path == filepath) || (!bundle_paths_seen_for_file.insert(metadata->bundle_path).second)) {
						return;
					}
					auto [it, inserted] = candidates_pool.try_emplace(metadata->bundle_path);
					ScoredCandidate& scored_candidate = it->second;
					if (inserted) {
						scored_candidate.metadata = metadata;
					}
					if (is_default) {
						scored_candidate.default_handler_count++;
					}
					// Welford's online mean update
					double suitability_score_for_current_file = (total_handlers > 1) ? static_cast<double>(handler_index) / (total_handlers - 1) : 0.0;
					scored_candidate.supported_files_count++;
					scored_candidate.avg_suitability_score +=
						(suitability_score_for_current_file - scored_candidate.avg_suitability_score) / scored_candidate.supported_files_count;
				};

				std::string_view default_bundle_path = default_app_metadata ? std::string_view(default_app_metadata->bundle_path) : std::string_view();

				for (size_t i = 0; i < compatible_apps.size(); ++i) {
					const auto* metadata = compatible_apps[i];
					const bool is_default = (!default_bundle_path.empty() && metadata->bundle_path == default_bundle_path);
					score_app_for_file(metadata, is_default, i, compatible_apps.size());
				}

				// Fallback: Ensure the default app is registered even if Launch Services omitted it
				// from URLsForApplicationsToOpenURL (e.g., on older macOS versions or legacy handlers).

				if (default_app_metadata) {
					size_t assumed_handlers_count = std::max(size_t(1), compatible_apps.size());
					score_app_for_file(default_app_metadata, true, 0, assumed_handlers_count);
				}
			}

			// ---------- Filtering and sorting ----------
			// Only applications that can open every selected file survive the intersection.

			ReportProgress({GetMsg(MsgID::FilteringSortingResults), GetMsg(MsgID::PleaseWait)});

			std::vector<ScoredCandidate> scored_finalists;

			for (auto& [bundle_path, scored_candidate] : candidates_pool) {
				if (scored_candidate.supported_files_count == files_total) {
					scored_finalists.push_back(std::move(scored_candidate));
				}
			}

			std::sort(scored_finalists.begin(), scored_finalists.end(),
				[alphabetical = _sort_alphabetically,
				 use_sys_rank = _respect_system_ranking](const ScoredCandidate& a, const ScoredCandidate& b) {

			        // Lexicographic comparison by name then version, used as a stable tie-breaker
			        // throughout this comparator to produce deterministic ordering for same-named apps
					auto cmp_names_and_versions = [](const ScoredCandidate& x, const ScoredCandidate& y) {
						if (x.metadata->presentation_name != y.metadata->presentation_name) {
							return x.metadata->presentation_name < y.metadata->presentation_name;
						}
						return x.metadata->presentation_version < y.metadata->presentation_version;
					};

			        // If strict global alphabetical sorting is enabled, it overrides everything
					if (alphabetical) {
						return cmp_names_and_versions(a, b);
					}

			        // Default handlers for the selected files always take top priority
					if (a.default_handler_count != b.default_handler_count) {
						return a.default_handler_count > b.default_handler_count;
					}

			        // If enabled, sort by Launch Services suitability (lower average score is better);
			        // fall through to name/version tie-breaker when scores are equal
					if (use_sys_rank) {
						if (a.avg_suitability_score < b.avg_suitability_score) {
							return true;
						}
						if (b.avg_suitability_score < a.avg_suitability_score) {
							return false;
						}
					}

			        // Name and version tie-breaker
					return cmp_names_and_versions(a, b);
			});

			// ---------- Final list generation ----------
			// Convert internal structures to CandidateInfo output. If multiple bundles share
			// the same presentation_name, append the presentation_version to disambiguate them in the UI.

			std::vector<CandidateInfo> out_candidates;
			if (!scored_finalists.empty()) {
				out_candidates.reserve(scored_finalists.size());

				std::unordered_map<std::string_view, int> app_name_frequency;
				for (const auto& scored_finalist : scored_finalists) {
					app_name_frequency[scored_finalist.metadata->presentation_name]++;
				}

				for (const auto& scored_finalist : scored_finalists) {
					CandidateInfo out_candidate;
					out_candidate.id = StrMB2Wide(scored_finalist.metadata->bundle_path);
					out_candidate.terminal = false;
					out_candidate.multi_file_aware = true;
					std::string name_for_menu = scored_finalist.metadata->presentation_name;
					if (app_name_frequency[scored_finalist.metadata->presentation_name] > 1 && !scored_finalist.metadata->presentation_version.empty()) {
						name_for_menu += " (" + scored_finalist.metadata->presentation_version + ")";
					}
					out_candidate.name = StrMB2Wide(name_for_menu);
					out_candidates.push_back(out_candidate);
				}
			}

			result.candidates = std::move(out_candidates);

		} catch (const OperationCancelledException&) {
			ClearLastQueryCaches();
			result.was_cancelled = true;
		}
		return result;

	}


	std::vector<std::wstring> MacOSAppProvider::ConstructLaunchCommands(const CandidateInfo& candidate, const std::vector<std::wstring>& filepaths)
	{
		if (candidate.id.empty() || filepaths.empty()) {
			return {};
		}
		// The 'open -a <app_path>' command tells the system to open files with a specific application.
		std::wstring cmd = L"open -a " + QuoteArgForShell(candidate.id);
		for (const auto& filepath : filepaths) {
			cmd += L" " + QuoteArgForShell(filepath);
		}
		return {cmd};
	}


	std::vector<Field> MacOSAppProvider::GetCandidateDetails(const CandidateInfo& candidate)
	{
		std::string bundle_path = StrWide2MB(candidate.id);
		auto it = _app_bundle_metadata_cache.find(bundle_path);
		if (it == _app_bundle_metadata_cache.end() || !it->second.has_value()) {
			return {};
		}
		const auto& metadata = *it->second;

		static constexpr std::pair<MsgID, std::string AppBundleMetadata::*> field_map[] = {
			{MsgID::Location,                 &AppBundleMetadata::bundle_path},
			{MsgID::BundleDisplayName,        &AppBundleMetadata::bundle_display_name},
			{MsgID::BundleName,               &AppBundleMetadata::bundle_name},
			{MsgID::BundleShortVersionString, &AppBundleMetadata::bundle_short_version_string},
			{MsgID::BundleVersion,            &AppBundleMetadata::bundle_version},
			{MsgID::BundleExecutable,         &AppBundleMetadata::bundle_executable},
			{MsgID::BundleIdentifier,         &AppBundleMetadata::bundle_identifier}
		};

		std::vector<Field> details;
		for (const auto& [msg_id, member_ptr] : field_map) {
			const std::string& val = metadata.*member_ptr;
			if (!val.empty()) {
				details.push_back({GetMsg(msg_id), StrMB2Wide(val)});
			}
		}

		return details;
	}


	std::vector<std::wstring> MacOSAppProvider::GetFileTypes()
	{
		std::unordered_set<std::string> unique_filetypes;
		unique_filetypes.reserve(_last_uti_records.size());

		for (const auto& record : _last_uti_records) {

			if (!record.is_uti_resolved) {
				unique_filetypes.insert("(inaccessible)");
				continue;
			}

			if (_show_uti_instead_of_mime) {
				if (record.uti.empty()) {
					unique_filetypes.insert("(none)");
				} else {
					unique_filetypes.insert(record.uti);
				}
				continue;
			}

			std::string mimetype = openwith::launch_services::ConvertUTIToMime(record.uti);

			if (mimetype.empty()) {
				unique_filetypes.insert("(none)");
			} else {
				unique_filetypes.insert(mimetype);
			}
		}

		std::vector<std::wstring> out_filetypes;
		out_filetypes.reserve(unique_filetypes.size());

		for (const auto& filetype : unique_filetypes) {
			out_filetypes.push_back(StrMB2Wide(filetype));
		}
		return out_filetypes;
	}


	std::vector<CandidateContextLocation> MacOSAppProvider::GetCandidateContextLocations(const CandidateInfo& candidate)
	{
		return {{GetMsg(MsgID::GoToBundle), candidate.id}};
	}


	void MacOSAppProvider::ClearLastQueryCaches()
	{
		_last_uti_records.clear();
		_app_bundle_metadata_cache.clear();
		_uti_to_apps_cache.clear();
	}


	const MacOSAppProvider::AppBundleMetadata* MacOSAppProvider::GetOrParseMetadata(const std::string& bundle_path)
	{
		if (bundle_path.empty()) {
			return nullptr;
		}

		auto [it, inserted] = _app_bundle_metadata_cache.try_emplace(bundle_path, std::nullopt);
		std::optional<AppBundleMetadata>& metadata_opt = it->second;
		if (!inserted) {
			return metadata_opt.has_value() ? &(*metadata_opt) : nullptr;
		}

		static constexpr std::pair<const char*, std::string AppBundleMetadata::*> plist_map[] = {
			{"CFBundleDisplayName",        &AppBundleMetadata::bundle_display_name},
			{"CFBundleName",               &AppBundleMetadata::bundle_name},
			{"CFBundleShortVersionString", &AppBundleMetadata::bundle_short_version_string},
			{"CFBundleVersion",            &AppBundleMetadata::bundle_version},
			{"CFBundleExecutable",         &AppBundleMetadata::bundle_executable},
			{"CFBundleIdentifier",         &AppBundleMetadata::bundle_identifier}
		};

		static const std::vector<std::string> keys = [] {
			std::vector<std::string> v;
			v.reserve(std::size(plist_map));
			for (const auto& item : plist_map) {
				v.emplace_back(item.first);
			}
			return v;
		}();

		if (auto info_plist_properties_opt = openwith::launch_services::ParseAppBundleMetadata(bundle_path, keys)) {
			const auto& bundle_properties = *info_plist_properties_opt;

			AppBundleMetadata metadata;
			metadata.bundle_path = bundle_path;

			for (const auto& [key, member_ptr] : plist_map) {
				if (auto property_it = bundle_properties.find(key); property_it != bundle_properties.end()) {
					metadata.*member_ptr = property_it->second;
				}
			}

			metadata.presentation_name = std::string(SelectFirstNonEmpty({
				metadata.bundle_display_name,
				metadata.bundle_name,
				ExtractBaseName(bundle_path)
			}));

			metadata.presentation_version = std::string(SelectFirstNonEmpty({
				metadata.bundle_short_version_string,
				metadata.bundle_version
			}));

			metadata_opt.emplace(std::move(metadata));
			return &(*metadata_opt);
		}

		return nullptr;
	}


	const MacOSAppProvider::CompatibleAppsMetadata& MacOSAppProvider::GetCachedCompatibleApps(const std::string& filepath, const std::string& uti)
	{
		auto [it, inserted] = _uti_to_apps_cache.try_emplace(uti);
		if (!inserted) {
			return it->second;
		}

		CompatibleAppsMetadata& new_cache_entry = it->second;
		auto compatible_bundle_paths = openwith::launch_services::GetCompatibleBundlePaths(filepath);

		for (const auto& bundle_path : compatible_bundle_paths) {
			if (const auto* meta = GetOrParseMetadata(bundle_path)) {
				new_cache_entry.push_back(meta);
			}
		}

		return new_cache_entry;
	}


	std::string_view MacOSAppProvider::ExtractBaseName(std::string_view bundle_path)
	{
		while (!bundle_path.empty() && bundle_path.back() == '/') {
			bundle_path.remove_suffix(1);
		}
		const size_t pos = bundle_path.rfind('/');
		return (pos == std::string_view::npos) ? bundle_path : bundle_path.substr(pos + 1);
	}


	std::string_view MacOSAppProvider::SelectFirstNonEmpty(std::initializer_list<std::string_view> items)
	{
		for (const auto& item : items) {
			if (!item.empty()) {
				return item;
			}
		}
		return {};
	}


	std::wstring MacOSAppProvider::QuoteArgForShell(const std::wstring& arg)
	{
		std::wstring out;
		out.push_back(L'\'');

		for (wchar_t c : arg) {
			if (c == L'\'') {
				out.append(L"'\\''");
			} else {
				out.push_back(c);
			}
		}
		out.push_back(L'\'');
		return out;
	}

} // namespace openwith
#endif // __APPLE__
