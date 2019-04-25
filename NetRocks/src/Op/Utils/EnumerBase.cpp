#include "EnumerLocal.h"
#include "ProgressStateUpdate.h"

EnumerBase::EnumerBase(ProgressState &state, const std::string &src_dir, const struct PluginPanelItem *items, int items_count, bool no_special_files)
	:
	_state(state), _no_special_files(no_special_files)
{
	std::string item_path;
	for (int i = 0; i < items_count; ++i) {
		if (strcmp(items[i].FindData.cFileName, ".") != 0 && strcmp(items[i].FindData.cFileName, "..") != 0) {
			item_path = src_dir;
			item_path+= items[i].FindData.cFileName;
			_items.emplace(item_path);
		}
	}
}

void EnumerBase::Scan()
{
	for (auto path : _items) {
		if (OnScanningPath(path)) {
			path+= '/';
			_scan_depth_limit = 255;
			ScanItem(path);
		}
	}
}
