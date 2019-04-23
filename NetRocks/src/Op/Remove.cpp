#include "Remove.h"

Remove::Remove(std::shared_ptr<SiteConnection> &connection)
	: _connection(connection)
{
}

bool Remove::Do(const std::string &src_dir, struct PluginPanelItem *items, int items_count, int op_mode)
{
	_op_mode = op_mode;
	_src_dir_len = src_dir.size();
	_items.clear();
	_state.Reset();


	std::string item_path;
	for (int i = 0; i < items_count; ++i) {
		if (strcmp(items[i].FindData.cFileName, ".") != 0 && strcmp(items[i].FindData.cFileName, "..") != 0) {
			item_path = src_dir;
			item_path+= items[i].FindData.cFileName;
			_items.emplace(item_path);
		}
	}

	if (!IS_SILENT(op_mode)) {
		if (!RemoveConfirm(src_dir).Ask()) {
			fprintf(stderr, "NetRocks::Remove: cancel\n");
			return false;
		}
	}

	if (!StartThread()) {
		fprintf(stderr, "NetRocks::Remove: start thread error\n");
		return false;
	}

	RemoveProgress(src_dir, _state).Show();

	WaitThread();

	return true;
}


void *Remove::ThreadProc()
{
	void *out = nullptr;
	try {
		Scan();
		Removal();
		fprintf(stderr,
			"NetRocks::Remove: count=%lu all_total=%llu\n",
			_entries.size(), _state.stats.all_total);

	} catch (std::exception &e) {
		fprintf(stderr, "NetRocks::RemoveThread ERROR: %s\n", e.what());
		out = this;
	}
	std::lock_guard<std::mutex> locker(_state.mtx);
	_state.finished = true;
	return out;
}



void Remove::CheckForUserInput(std::unique_lock<std::mutex> &lock)
{
	for (;;) {
		if (_state.aborting)
			throw std::runtime_error("Aborted");
		if (!_state.paused)
			break;

		lock.unlock();
		usleep(1000000);
		lock.lock();
	}
}

void Remove::Scan()
{
	FileInformation info = {};
	for (auto path : _items) {
		info.mode = _connection->GetMode(path, true);
		info.size = S_ISDIR(info.mode) ? 0 :_connection->GetSize(path, true);
		if (_entries.emplace(path, info).second) {
			if (S_ISDIR(info.mode)) {
				path+= '/';
				_scan_depth_limit = 255;
				ScanItem(path);
			}
		
			std::unique_lock<std::mutex> lock(_state.mtx);
			_state.stats.all_total+= info.size;
			_state.stats.count_total = _entries.size();
			CheckForUserInput(lock);
		}
	}
}

void Remove::ScanItem(const std::string &path)
{
	UnixFileList ufl;
	_connection->DirectoryEnum(path, ufl, 0);
	if (ufl.empty())
		return;

	std::string subpath;
	for (const auto &e : ufl) {
		subpath = path;
		subpath+= e.name;
		if (_entries.emplace(subpath, e.info).second) {
			if (S_ISDIR(e.info.mode)) {
				if (_scan_depth_limit) {
					subpath+= '/';
					--_scan_depth_limit;
					ScanItem(subpath);
					++_scan_depth_limit;
				} else {
					fprintf(stderr, "NetRocks::Item('%s'): depth limit exhausted\n", subpath.c_str());
					// _failed = true;
				}
			}

			std::unique_lock<std::mutex> lock(_state.mtx);
			if (!S_ISDIR(e.info.mode))
				_state.stats.all_total+= e.info.size;
			_state.stats.count_total = _entries.size();
			CheckForUserInput(lock);
		}
	}
}

void Remove::Removal()
{
	for (auto rev_i = _entries.rbegin(); rev_i != _entries.rend(); ++rev_i) {
		try {
			const std::string &subpath = rev_i->first.substr(_src_dir_len);

			if (S_ISDIR(rev_i->second.mode)) {
				_connection->DirectoryDelete(rev_i->first);
			} else {
				_connection->FileDelete(rev_i->first);
			}

			std::unique_lock<std::mutex> lock(_state.mtx);
			_state.path = subpath;
			if (!S_ISDIR(rev_i->second.mode))
				_state.stats.all_complete+= rev_i->second.size;
			_state.stats.count_complete++;
			CheckForUserInput(lock);
		} catch  (std::exception &ex) {
			fprintf(stderr, "NetRocks::Remove: %s while %s '%s'\n", ex.what(),
				S_ISDIR(rev_i->second.mode) ? "rmdir" : "rmfile", rev_i->first.c_str());
		}
	}
}
