#pragma once

#if defined(__APPLE__)

#include "AppProvider.hpp"
#include "common.hpp"
#include <string>
#include <vector>

class MacOSAppProvider : public AppProvider
{
public:
	explicit MacOSAppProvider(TMsgGetter msg_getter);
	std::vector<CandidateInfo> GetAppCandidates(const std::vector<std::wstring>& pathnames) override;
	std::vector<std::wstring> ConstructCommandLine(const CandidateInfo& candidate, const std::vector<std::wstring>& pathnames) override;
	std::vector<std::wstring> GetMimeTypes(const std::vector<std::wstring>& pathnames) override;
	std::vector<Field> GetCandidateDetails(const CandidateInfo& candidate) override;
	std::vector<ProviderSetting> GetPlatformSettings() override { return {}; }
	void SetPlatformSettings(const std::vector<ProviderSetting>& settings) override {}
	void LoadPlatformSettings() override {}
	void SavePlatformSettings() override {}
};

#endif
