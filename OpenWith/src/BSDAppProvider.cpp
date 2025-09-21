#if defined(__FreeBSD__) || defined(__OpenBSD__) || defined(__NetBSD__) || defined(__DragonFly__)

#include "BSDAppProvider.hpp"
#include "common.hpp"
#include <string>
#include <vector>

BSDAppProvider::BSDAppProvider(TMsgGetter msg_getter) : AppProvider(std::move(msg_getter))
{
}


std::vector<CandidateInfo> BSDAppProvider::GetAppCandidates(const std::wstring& pathname)
{
	return {};
}


std::wstring BSDAppProvider::ConstructCommandLine(const CandidateInfo& candidate, const std::wstring& pathname)
{
	return {};
}


std::wstring BSDAppProvider::GetMimeType(const std::wstring& pathname)
{
	return L"application/octet-stream";
}


std::vector<Field> BSDAppProvider::GetCandidateDetails(const CandidateInfo& candidate)
{
	return {};
}


std::vector<ProviderSetting> BSDAppProvider::GetPlatformSettings()
{
	return {};
}


void BSDAppProvider::SetPlatformSettings(const std::vector<ProviderSetting>& settings)
{
}


void BSDAppProvider::LoadPlatformSettings()
{
}


void BSDAppProvider::SavePlatformSettings()
{
}

#endif
