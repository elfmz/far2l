#include "FileStatsOverride.h"

FileStatsOverride::~FileStatsOverride()
{
}

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

const FileStatsOverride::OverridenStats *FileStatsOverride::Lookup(const std::string &path) const
{
	if (!path.empty() && path.back() == '/')
		return Lookup(path.substr(0, path.size() - 1));

	if (path.empty())
		return nullptr;

	auto it = _path2ovrst.find(path);
	return (it != _path2ovrst.end()) ? &it->second : nullptr;
}

void FileStatsOverride::FilterFileInformation(const std::string &path, FileInformation &file_info) const
{
	const auto *ovrst = Lookup(path);
	if (ovrst) {
		if (ovrst->mode) {
			file_info.mode = ovrst->mode;
		}
		if (ovrst->access_time.tv_sec) {
			file_info.access_time = ovrst->access_time;
		}
		if (ovrst->modification_time.tv_sec) {
			file_info.modification_time = ovrst->modification_time;
		}
	}
}

void FileStatsOverride::FilterFileMode(const std::string &path, mode_t &mode) const
{
	const auto *ovrst = Lookup(path);
	if (ovrst && ovrst->mode) {
		mode = ovrst->mode;
	}
}

///////////////

DirectoryEnumerWithFileStatsOverride::DirectoryEnumerWithFileStatsOverride(
		const FileStatsOverride &file_stats_override,
		std::shared_ptr<IDirectoryEnumer> enumer,
		const std::string &path)
	:
	_file_stats_override(file_stats_override),
	_enumer(enumer),
	_path(path)
{

	if (!_path.empty() && _path.back() != '/') {
		_path+= '/';
	}
	_path_len = _path.size();
}

DirectoryEnumerWithFileStatsOverride::~DirectoryEnumerWithFileStatsOverride()
{
}

bool DirectoryEnumerWithFileStatsOverride::Enum(std::string &name, std::string &owner, std::string &group, FileInformation &file_info)
{
	if (!_enumer->Enum(name, owner, group, file_info)) {
		return false;
	}

	_path.replace(_path_len, _path.size() - _path_len, name);
	_file_stats_override.FilterFileInformation(_path, file_info);
	return true;
}
