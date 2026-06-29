#pragma once

#include "common.hpp"
#include <memory>
#include <string>
#include <vector>

class KeyFileReadHelper;
class KeyFileHelper;

namespace openwith
{
	struct ProviderSetting
	{
		std::wstring internal_key;      // persistent INI key and internal identifier
		std::wstring display_name;      // localized UI label
		bool value = false;
		bool disabled = false;          // true if the setting should be grayed out in the UI
		bool affects_candidates = true; // true if changing this setting affects the contents or order of the candidate list
	};


	class AppProvider
	{
	public:
		virtual ~AppProvider() = default;

		AppProvider(const AppProvider&) = delete;
		AppProvider& operator=(const AppProvider&) = delete;
		AppProvider(AppProvider&&) = delete;
		AppProvider& operator=(AppProvider&&) = delete;

		static AppProvider* GetInstance();

		virtual std::vector<CandidateInfo> GetAppCandidates(const std::vector<std::wstring>& filepaths) = 0;
		virtual std::vector<std::wstring> GetMimeTypes() = 0;
		virtual std::vector<std::wstring> ConstructLaunchCommands(const CandidateInfo& candidate, const std::vector<std::wstring>& filepaths) = 0;
		virtual std::vector<Field> GetCandidateDetails(const CandidateInfo& candidate) = 0;
		virtual std::vector<CandidateContextLocation> GetCandidateContextLocations(const CandidateInfo& candidate) { return {}; }

		virtual std::vector<ProviderSetting> GetPlatformSettings() { return {}; }
		virtual void SetPlatformSettings(const std::vector<ProviderSetting>& settings) {}
		virtual void LoadPlatformSettings(const KeyFileReadHelper& key_reader) {}
		virtual void SavePlatformSettings(KeyFileHelper& key_writer) {}

	protected:
		AppProvider() = default;

	private:
		static std::unique_ptr<AppProvider> CreateAppProvider();
	};

} // namespace openwith
