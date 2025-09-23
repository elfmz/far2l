#include "AppProvider.hpp"
#include "DummyAppProvider.hpp"
#include "common.hpp"
#include <string>
#include <vector>

DummyAppProvider::DummyAppProvider(TMsgGetter msg_getter) : AppProvider(std::move(msg_getter))
{
}


std::vector<CandidateInfo> DummyAppProvider::GetAppCandidates(const std::wstring& pathname)
{
	return {};
}


std::wstring DummyAppProvider::ConstructCommandLine(const CandidateInfo& candidate, const std::wstring& pathname)
{
	return L"";
}


std::wstring DummyAppProvider::GetMimeType(const std::wstring& pathname)
{
	return L"application/octet-stream";
}


std::vector<Field> DummyAppProvider::GetCandidateDetails(const CandidateInfo& candidate)
{
	return {};
}


std::vector<ProviderSetting> DummyAppProvider::GetPlatformSettings()
{
	return {};
}


void DummyAppProvider::SetPlatformSettings(const std::vector<ProviderSetting>& settings)
{
}


void DummyAppProvider::LoadPlatformSettings()
{
}


void DummyAppProvider::SavePlatformSettings()
{
}
