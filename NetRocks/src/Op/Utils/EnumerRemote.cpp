#include "EnumerRemote.h"
#include "ProgressStateUpdate.h"

EnumerRemote::EnumerRemote(RemoteEntries &entries, ProgressState &state,
	const std::string src_dir, const struct PluginPanelItem *items, int items_count,
	bool no_special_files, std::shared_ptr<SiteConnection> &connection)
	:
	EnumerBase(state, src_dir, items, items_count, no_special_files),
	_entries(entries), _connection(connection)
{
}

void EnumerRemote::ScanItem(const std::string &path)
{
	UnixFileList ufl;
	_connection->DirectoryEnum(path, ufl, 0);
	if (ufl.empty())
		return;

	std::string subpath;
	for (const auto &e : ufl) {
		subpath = path;
		subpath+= e.name;
		if (OnScanningPath(subpath)) {
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

bool EnumerRemote::OnScanningPath(const std::string &path)
{
	FileInformation info = {};
	info.mode = _connection->GetMode(path, true);


	if (S_ISREG(info.mode)) {
			info.size = _connection->GetSize(path, false);

	} else if (!S_ISDIR(info.mode)) {
		if (_no_special_files)
			return false;

		if (!S_ISDIR(_connection->GetMode(path, false))) {
			info.size = _connection->GetSize(path, false);
		}
	}

	if (!_entries.emplace(path, info).second)
		return false;

	ProgressStateUpdate psu(_state);

	_state.stats.count_total = _entries.size();
	if (!S_ISDIR(info.mode)) {
		_state.stats.all_total+= info.size;
		return false;
	}

	return true;
}
