#include "FileStatsOverride.h"

void FileStatsOverride::Cleanup(const std::string &path)
{
	if (!path.empty() && path.back() == '/')
		Cleanup(path.substr(0, path.size() - 1));
	else
		_path2ovrst.erase(path);
}

void FileStatsOverride::Rename(std::string old_path, std::string new_path)
{
	if (!old_path.empty() && old_path.back() == '/')
		old_path.pop_back();
	if (!new_path.empty() && new_path.back() == '/')
		new_path.pop_back();

	if (old_path == new_path) {
		return;
	}

	auto it = _path2ovrst.find(old_path);
	if (it != _path2ovrst.end()) {
		_path2ovrst[new_path] = it->second;
		_path2ovrst.erase(it);
	}
}

void FileStatsOverride::OverrideTimes(const std::string &path, const timespec &access_time, const timespec &modification_time)
{
	OverridenStats &ovrst = _path2ovrst[path];
	ovrst.access_time = access_time;
	ovrst.modification_time = modification_time;
}

void FileStatsOverride::OverrideMode(const std::string &path, mode_t mode)
{
	_path2ovrst[path].mode = mode;
}

const FileStatsOverride::OverridenStats *FileStatsOverride::Lookup(const std::string &path)
{
	if (!path.empty() && path.back() == '/')
		return Lookup(path.substr(0, path.size() - 1));

	if (path.empty())
		return nullptr;

	auto it = _path2ovrst.find(path);
	return (it != _path2ovrst.end()) ? &it->second : nullptr;
}

const FileStatsOverride::OverridenStats *FileStatsOverride::Lookup(const std::string &path, const std::string &name)
{
	std::string full_path = path;
	if (!name.empty() && !full_path.empty() && full_path.back() != '/' && name.front() != '/') {
		full_path+= '/';
	}
	full_path+= name;
	return Lookup(full_path);
}
