#pragma once
#include <map>
#include <string>
#include <time.h>
#include <sys/stat.h>

class FileStatsOverride
{
public:
	struct OverridenStats
	{
		timespec access_time{};
		timespec modification_time{};
		mode_t mode{};
	};

	void Cleanup(const std::string &path);
	void Rename(std::string old_path, std::string new_path);
	void OverrideTimes(const std::string &path, const timespec &access_time, const timespec &modification_time);
	void OverrideMode(const std::string &path, mode_t mode);
	const OverridenStats *Lookup(const std::string &path);
	const OverridenStats *Lookup(const std::string &path, const std::string &name);

private:
	std::map<std::string, OverridenStats> _path2ovrst;
};
