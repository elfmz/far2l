#pragma once

#if defined(__APPLE__)

#include "AppProvider.hpp"
#include "common.hpp"
#include <string>
#include <vector>
#include <unordered_set>
#include <functional>

class KeyFileReadHelper;
class KeyFileHelper;

namespace openwith
{
	class MacOSAppProvider : public AppProvider
	{
	public:
		MacOSAppProvider();
		std::vector<CandidateInfo> GetAppCandidates(const std::vector<std::wstring>& filepaths) override;
		std::vector<std::wstring> ConstructLaunchCommands(const CandidateInfo& candidate, const std::vector<std::wstring>& filepaths) override;
		std::vector<std::wstring> GetMimeTypes() override;
		std::vector<Field> GetCandidateDetails(const CandidateInfo& candidate) override;
		std::vector<ProviderSetting> GetPlatformSettings() override { return {}; }
		void SetPlatformSettings(const std::vector<ProviderSetting>& settings) override {}
		void LoadPlatformSettings(const KeyFileReadHelper &key_reader) override {}
		void SavePlatformSettings(KeyFileHelper& key_writer) override {}

	private:
		// A struct to cache the results of a file type query. It stores the file's UTI and accessibility state.
		struct MacFileProfile
		{
			std::string uti; // Contains the resolved UTI string, or an empty string if the file is inaccessible.
			bool accessible; // true if file existed and UTI was gettable.

			bool operator==(const MacFileProfile& other) const
			{
				return accessible == other.accessible && uti == other.uti;
			}

			struct Hash
			{
				std::size_t operator()(const MacFileProfile& p) const noexcept
				{
					std::size_t h1 = std::hash<std::string>{}(p.uti);
					std::size_t h2 = std::hash<bool>{}(p.accessible);
					std::size_t seed = h1;
					seed ^= h2 + 0x9e3779b9 + (seed << 6) + (seed >> 2);
					return seed;
				}
			};
		};

		// Caches all unique MacFileProfile objects collected during the last GetAppCandidates call.
		std::unordered_set<MacFileProfile, MacFileProfile::Hash> _last_uti_profiles;
	};
} // namespace openwith

#endif
