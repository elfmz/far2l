#pragma once
#include <time.h>
#include <sys/stat.h>
#include <map>
#include <string>
#include <memory>
#include "Protocol.h"

class FileStatsOverride
{
public:
	~FileStatsOverride();

	struct OverridenStats
	{
		timespec access_time{};
		timespec modification_time{};
		mode_t mode{};
	};

	bool NonEmpty() const { return !_path2ovrst.empty(); }
	void Cleanup(const std::string &path);
	void Rename(std::string old_path, std::string new_path);

	void OverrideTimes(const std::string &path, const timespec &access_time, const timespec &modification_time);
	void OverrideMode(const std::string &path, mode_t mode);

	void FilterFileInformation(const std::string &path, FileInformation &file_info) const;
	void FilterFileMode(const std::string &path, mode_t &mode) const;

private:
	std::map<std::string, OverridenStats> _path2ovrst;

	const OverridenStats *Lookup(const std::string &path) const;
};


class DirectoryEnumerWithFileStatsOverride : public IDirectoryEnumer
{
	const FileStatsOverride &_file_stats_override;
	std::shared_ptr<IDirectoryEnumer> _enumer;
	std::string _path;
	size_t _path_len{};

public:
	DirectoryEnumerWithFileStatsOverride(const FileStatsOverride &file_stats_override,
		std::shared_ptr<IDirectoryEnumer> enumer, const std::string &path);
	virtual ~DirectoryEnumerWithFileStatsOverride();
	virtual bool Enum(std::string &name, std::string &owner, std::string &group, FileInformation &file_info);
};
