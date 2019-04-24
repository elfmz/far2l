#include "Download.h"

Download::Download(std::shared_ptr<SiteConnection> &connection)
	: _connection(connection)
{
}

bool Download::Do(const std::string &dst_dir, const std::string &src_dir, struct PluginPanelItem *items, int items_count, bool mv, int op_mode)
{
	_mv = mv;
	_op_mode = op_mode;
	_src_dir_len = src_dir.size();
	_dst_dir = dst_dir;
	_items.clear();
	_xdoa = XDOA_ASK;
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
		if (!XferConfirm(_mv ? XK_MOVE : XK_COPY, XK_DOWNLOAD, _dst_dir).Ask(_xdoa)) {
			fprintf(stderr, "NetRocks::Download: cancel\n");
			return false;
		}
	}

	if (_dst_dir.empty()) {
		_dst_dir = "./";
	} else if (dst_dir[_dst_dir.size() - 1] != '.')
		_dst_dir+= '/';

	if (!StartThread()) {
		fprintf(stderr, "NetRocks::Download: start thread error\n");
		return false;
	}

	XferProgress(_mv ? XK_MOVE : XK_COPY, XK_DOWNLOAD, _dst_dir, _state).Show();

	WaitThread();

	return true;
}


void *Download::ThreadProc()
{
	void *out = nullptr;
	try {
		Scan();
		Transfer();
		fprintf(stderr,
			"NetRocks::Download: _dst_dir='%s' --> count=%lu all_total=%llu\n",
			_dst_dir.c_str(), _entries.size(), _state.stats.all_total);

	} catch (std::exception &e) {
		fprintf(stderr, "NetRocks::DownloadThread('%s') ERROR: %s\n", _dst_dir.c_str(), e.what());
		out = this;
	}
	std::lock_guard<std::mutex> locker(_state.mtx);
	_state.finished = true;
	return out;
}



void Download::CheckForUserInput(std::unique_lock<std::mutex> &lock)
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

bool Download::OnScannedPath(const std::string &path)
{
	FileInformation info = {};
	info.mode = _connection->GetMode(path, true);
	if (S_ISREG(info.mode)) {
		info.size = _connection->GetSize(path, true);
	} else if (!S_ISDIR(info.mode)) {
		return false;
	}

	if (!_entries.emplace(path, info).second)
		return false;

	std::unique_lock<std::mutex> lock(_state.mtx);
	CheckForUserInput(lock);

	_state.stats.count_total = _entries.size();
	if (!S_ISDIR(info.mode)) {
		_state.stats.all_total+= info.size;
		return false;
	}

	return true;
}

void Download::Scan()
{
	for (auto path : _items) {
		if (OnScannedPath(path)) {
			path+= '/';
			_scan_depth_limit = 255;
			ScanItem(path);
		}
	}
}

void Download::ScanItem(const std::string &path)
{
	UnixFileList ufl;
	_connection->DirectoryEnum(path, ufl, 0);
	if (ufl.empty())
		return;

	std::string subpath;
	for (const auto &e : ufl) {
		subpath = path;
		subpath+= e.name;
		if (OnScannedPath(subpath)) {
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

void Download::Transfer()
{
	std::string path_local;
	for (const auto &e : _entries) {
		const std::string &subpath = e.first.substr(_src_dir_len);
		path_local = _dst_dir;
		path_local+= subpath;

		{
			std::unique_lock<std::mutex> lock(_state.mtx);
			_state.path = subpath;
			_state.stats.file_complete = 0;
			_state.stats.file_total = S_ISDIR(e.second.mode) ? 0 : e.second.size;
		}

		if (S_ISDIR(e.second.mode)) {
			sdc_mkdir(path_local.c_str(), e.second.mode);

		} else {
			try {
				_connection->FileGet(e.first, path_local, e.second.mode, 0, this);
				if (_mv) {
					try {
						_connection->FileDelete(e.first);
					} catch (std::exception &ex) {
						fprintf(stderr, "NetRocks::Transfer: %s while rmfile '%s'\n",
							ex.what(), e.first.c_str());
					}
				}
			} catch (std::exception &ex) {
				fprintf(stderr, "NetRocks::Transfer: %s on '%s' -> '%s'\n", ex.what(), e.first.c_str(), path_local.c_str());
			}
		}

		std::unique_lock<std::mutex> lock(_state.mtx);
		_state.stats.count_complete++;
		CheckForUserInput(lock);
	}

	if (_mv) {
		for (auto rev_i = _entries.rbegin(); rev_i != _entries.rend(); ++rev_i) {
			if (S_ISDIR(rev_i->second.mode)) {
				try {
					_connection->DirectoryDelete(rev_i->first);
				} catch  (std::exception &ex) {
					fprintf(stderr, "NetRocks::Transfer: %s while rmdir '%s'\n",
						ex.what(), rev_i->first.c_str());
				}
			}
		}
	}
}

bool Download::OnIOStatus(unsigned long long transferred)
{
	std::unique_lock<std::mutex> lock(_state.mtx);
	_state.stats.all_complete+= transferred;
	_state.stats.file_complete+= transferred;
	for (;;) {
		if (_state.aborting)
			return false;
		if (!_state.paused)
			return true;

		lock.unlock();
		usleep(1000000);
		lock.lock();
	}
}

