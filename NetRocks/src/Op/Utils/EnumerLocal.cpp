#include "EnumerLocal.h"
#include "ProgressStateUpdate.h"

EnumerLocal::EnumerLocal(LocalEntries &entries, ProgressState &state, const std::string base_dir,
	const struct PluginPanelItem *items, int items_count, bool no_special_files)
	:
	EnumerBase(state, base_dir, items, items_count, no_special_files),
	_entries(entries)
{
}

void EnumerLocal::ScanItem(const std::string &path)
{
	DIR *dir = opendir(path.c_str());
	if (dir) {
		std::string subpath;
		for (;;) {
			struct dirent *de = readdir(dir);
			if (!de) break;
			if (de->d_name[0] && (de->d_name[0] != '.'
			 || (de->d_name[1] && (de->d_name[1] != '.' || de->d_name[2])))) {
				subpath = path;
				subpath+= de->d_name;
				if (OnScanningPath(subpath)) {
					if (_scan_depth_limit) {
						--_scan_depth_limit;
						subpath+= '/';
						ScanItem(subpath);
						++_scan_depth_limit;
					} else {
						fprintf(stderr, "NetRocks::EnumerLocal: depth limit exhausted - '%s'\n", subpath.c_str());
					}
				}
			}
		}
		closedir(dir);
	}
}

bool EnumerLocal::OnScanningPath(const std::string &path)
{
	struct stat s = {};
	//fprintf(stderr, "EnumerLocal::OnScannedPath: '%s'\n", path.c_str());
	if (sdc_lstat(path.c_str(), &s) != 0) {
		// if (ScanErrorDialog(path)
		//	throw std::runtime_error("Failed to stat path");
		return false;
	}
	if (!S_ISDIR(s.st_mode) && !S_ISREG(s.st_mode) && _no_special_files) {
		return false;
	}
	if (!_entries.emplace(path, s).second) {
		return false;
	}

	ProgressStateUpdate psu(_state);
	_state.stats.count_total = _entries.size();
	if (!S_ISDIR(s.st_mode)) {
		_state.stats.all_total+= s.st_size;
		return false;
	}

	return true;
}

