#include <assert.h>

#include "Enumer.h"
#include "ProgressStateUpdate.h"

Enumer::Enumer(Path2FileInformation &result, std::shared_ptr<IHost> &host, const std::string &dir,
		const struct PluginPanelItem *items, int items_count, bool no_special_files, ProgressState &state)
	:
	_result(result), _host(host), _no_special_files(no_special_files), _state(state)
{
	assert(dir.empty() || dir[dir.size() - 1] == '/');
	std::string item_path;
	for (int i = 0; i < items_count; ++i) {
		if (FILENAME_ENUMERABLE(items[i].FindData.cFileName)) {
			item_path = dir;
			item_path+= items[i].FindData.cFileName;
			_items.emplace(item_path);
		}
	}
}

void Enumer::Scan()
{
	for (auto path : _items) {
		if (OnScanningPath(path)) {
			path+= '/';
			_scan_depth_limit = 255;
			ScanItem(path);
		}
	}
}

void Enumer::GetSubitems(const std::string &path, Path2FileInformation &subitems)
{
	std::shared_ptr<IDirectoryEnumer> enumer = _host->DirectoryEnum(path);
	std::string name, owner, group;
	FileInformation file_info;
	for (;;) {
		if (!enumer->Enum(name, owner, group, file_info)) {
			break;
		}
		subitems.emplace(name, file_info);
	}
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
		info.mode = _host->GetMode(path, true);
	}

	if (S_ISREG(info.mode)) {
		if (!file_info) {
			info.size = _host->GetSize(path, false);
		}

	} else if (!S_ISDIR(info.mode)) {
		if (_no_special_files)
			return false;

		if (!S_ISDIR(_host->GetMode(path, false))) {
			info.size = _host->GetSize(path, false);
		}
	}

	if (!_result.emplace(path, info).second)
		return false;

	ProgressStateUpdate psu(_state);

	_state.stats.count_total++;
	if (!S_ISDIR(info.mode)) {
		_state.stats.all_total+= info.size;
		return false;
	}

	return true;
}
