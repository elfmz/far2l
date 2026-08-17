#pragma once

#if defined(__APPLE__)

#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

namespace openwith::launch_services
{
	std::optional<std::string> ResolveFileUTI(const std::string& filepath);
	std::optional<std::string> GetDefaultBundlePath(const std::string& filepath);
	std::vector<std::string> GetCompatibleBundlePaths(const std::string& filepath);
	std::optional<std::unordered_map<std::string, std::string>> ParseAppBundleMetadata(const std::string& bundle_path, const std::vector<std::string>& keys);
	std::string ConvertUTIToMime(const std::string& uti);
}

#endif // __APPLE__
