#pragma once

#include "common.hpp"
#include <string>
#include <vector>
#include <memory>
#include <functional>
#include <utility>


struct ProviderSetting
{
	std::wstring internal_key;  // persistent INI key and internal identifier
	std::wstring display_name;  // localized UI label
	bool value;
	bool disabled;              // true if the setting should be grayed out in the UI
	bool affects_candidates;    // true if changing this setting affects the contents or order of the candidate list
};

class KeyFileReadHelper;
class KeyFileHelper;

class AppProvider
{
public:
	using TMsgGetter = std::function<const wchar_t*(int)>;

	explicit AppProvider(TMsgGetter msg_getter) : m_GetMsg(std::move(msg_getter)) {}
	virtual ~AppProvider() = default;

	static AppProvider* GetInstance(TMsgGetter msg_getter = nullptr);

	virtual std::vector<CandidateInfo> GetAppCandidates(const std::vector<std::wstring>& filepaths) = 0;
	virtual std::vector<std::wstring> GetMimeTypes() = 0;
	virtual std::vector<std::wstring> ConstructLaunchCommands(const CandidateInfo& candidate, const std::vector<std::wstring>& filepaths) = 0;
	virtual std::vector<Field> GetCandidateDetails(const CandidateInfo& candidate) = 0;
	virtual std::vector<CandidateContextLocation> GetCandidateContextLocations(const CandidateInfo& candidate) { return {}; }

	virtual std::vector<ProviderSetting> GetPlatformSettings() { return {}; }
	virtual void SetPlatformSettings(const std::vector<ProviderSetting>& settings) { }
	virtual void LoadPlatformSettings(const KeyFileReadHelper& key_reader) { }
	virtual void SavePlatformSettings(KeyFileHelper& key_writer) { }


protected:
	TMsgGetter m_GetMsg;

private:
	static std::unique_ptr<AppProvider> CreateAppProvider(TMsgGetter msg_getter);
};
