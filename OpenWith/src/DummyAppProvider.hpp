#pragma once

#include "AppProvider.hpp"
#include "common.hpp"
#include <string>
#include <vector>

class DummyAppProvider : public AppProvider
{
public:
	explicit DummyAppProvider(TMsgGetter msg_getter);

	std::vector<CandidateInfo> GetAppCandidates(const std::vector<std::wstring>& filepaths) override;
	std::vector<std::wstring> GetMimeTypes() override;
	std::vector<std::wstring> ConstructLaunchCommands(const CandidateInfo& candidate, const std::vector<std::wstring>& filepaths) override;
	std::vector<Field> GetCandidateDetails(const CandidateInfo& candidate) override;

	std::vector<ProviderSetting> GetPlatformSettings() override;
	void SetPlatformSettings(const std::vector<ProviderSetting>& settings) override;
	void LoadPlatformSettings() override;
	void SavePlatformSettings() override;
};
