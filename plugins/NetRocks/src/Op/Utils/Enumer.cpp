#include <sys/stat.h>
#include <utils.h>
#include "Enumer.h"
#include "ProgressStateUpdate.h"

Enumer::Enumer(Path2FileInformation &result, std::shared_ptr<IHost> &host, const std::string &dir,
		const struct PluginPanelItem *items, int items_count, bool no_special_files,
		ProgressState &state, std::shared_ptr<WhatOnErrorState> &wea_state)
	:
	_result(result), _host(host), _no_special_files(no_special_files), _state(state), _wea_state(wea_state)
{
	ASSERT(dir.empty() || dir[dir.size() - 1] == '/');
	std::string item_path;
	for (int i = 0; i < items_count; ++i) {
		if (FILENAME_ENUMERABLE(items[i].FindData.lpwszFileName)) {
			item_path = dir;
			item_path+= Wide2MB(items[i].FindData.lpwszFileName);
			_items.emplace(item_path);
		}
	}
}

void Enumer::Scan(bool recurse)
{
	std::string subpath;
	for (const auto &path : _items) {
		if (OnScanningPath(path)) {
			if (recurse) {
				subpath = path;
				subpath+= '/';
				_scan_depth_limit = 255;
				ScanItem(subpath);
			}
		}
	}
}

void Enumer::GetSubitems(const std::string &path, Path2FileInformation &subitems)
{
	WhatOnErrorWrap<WEK_ENUMDIR>(_wea_state, _state, _host.get(), path,
		[&] () mutable
		{
			std::shared_ptr<IDirectoryEnumer> enumer = _host->DirectoryEnum(path);
			std::string name, owner, group;
			FileInformation file_info;
			for (;;) {
				if (!enumer->Enum(name, owner, group, file_info)) {
					break;
				}
				subitems.emplace(name, file_info);
				ProgressStateUpdate psu(_state); // check for abort/pause
			}
		}
	);
}

void Enumer::ScanItem(const std::string &path)
{
	Path2FileInformation subitems;
	GetSubitems(path, subitems);

	if (subitems.empty())
		return;

	std::string subpath;
	for (const auto &e : subitems) {
		subpath = path;
		subpath+= e.first;
		if (OnScanningPath(subpath, &e.second)) {
			if (_scan_depth_limit) {
				subpath+= '/';
				--_scan_depth_limit;
				ScanItem(subpath);
				++_scan_depth_limit;
			} else {
				fprintf(stderr, "NetRocks::Item('%s'): depth limit exhausted\n", subpath.c_str());
			}
		}
	}
}

bool Enumer::OnScanningPath(const std::string &path, const FileInformation *file_info)
{
	FileInformation info = {};
	if (file_info) {
		info = *file_info;
	} else {
		_host->GetInformation(info, path, false);
	}

	if (!S_ISREG(info.mode)) {
		if (_no_special_files && !S_ISDIR(info.mode) && !S_ISLNK(info.mode)) {
			return false;
		}

		info.size = 0;
	}

	if (!_result.emplace(path, info).second)
		return false;

	ProgressStateUpdate psu(_state);
//	_state.path = path;
	_state.stats.count_total++;
	if (!S_ISDIR(info.mode)) {
		_state.stats.all_total+= info.size;
		return false;
	}

	return true;
}
