#pragma once

#if defined(__FreeBSD__) || defined(__OpenBSD__) || defined(__NetBSD__) || defined(__DragonFly__)

#include "AppProvider.hpp"
#include "common.hpp"
#include <string>
#include <vector>

class BSDAppProvider : public AppProvider
{
public:
	explicit BSDAppProvider(TMsgGetter msg_getter);

	std::vector<CandidateInfo> GetAppCandidates(const std::wstring& pathname) override;
	std::wstring GetMimeType(const std::wstring& pathname) override;
	std::wstring ConstructCommandLine(const CandidateInfo& candidate, const std::wstring& pathname) override;

	std::vector<Field> GetCandidateDetails(const CandidateInfo& candidate) override;

	std::vector<ProviderSetting> GetPlatformSettings() override;
	void SetPlatformSettings(const std::vector<ProviderSetting>& settings) override;
	void LoadPlatformSettings() override;
	void SavePlatformSettings() override;
};

#endif
