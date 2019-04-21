#include "Upload.h"

Upload::Upload(std::shared_ptr<SiteConnection> &connection)
	: _connection(connection)
{
}

bool Upload::Do(const std::string &dst_dir, const std::string &src_dir, struct PluginPanelItem *items, int items_count, bool mv, int op_mode)
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
		if (items[i].FindData.cFileName[0] && strcmp(items[i].FindData.cFileName, ".") != 0 && strcmp(items[i].FindData.cFileName, "..") != 0) {
			item_path = src_dir;
			item_path+= items[i].FindData.cFileName;
			_items.emplace(item_path);
		}
	}

	if (!XferConfirm(_mv ? XK_MOVE : XK_COPY, XK_UPLOAD, _dst_dir).Ask(_xdoa)) {
		fprintf(stderr, "NetRocks::Upload: cancel\n");
		return false;
	}

	if (_dst_dir.empty()) {
		_dst_dir = "./";
	} else if (dst_dir[_dst_dir.size() - 1] != '.')
		_dst_dir+= '/';

	if (!StartThread()) {
		fprintf(stderr, "NetRocks::Upload: start thread error\n");
		return false;
	}

	XferProgress(_mv ? XK_MOVE : XK_COPY, XK_UPLOAD, _dst_dir, _state).Show();

	WaitThread();

	return true;
}


void *Upload::ThreadProc()
{
	void *out = nullptr;
	try {
		Scan();
		Transfer();
		fprintf(stderr,
			"NetRocks::Upload: _dst_dir='%s' --> count=%lu total_size=%llu\n",
			_dst_dir.c_str(), _entries.size(), _state.stats.total_size);

	} catch (std::exception &e) {
		fprintf(stderr, "NetRocks::UploadThread('%s') ERROR: %s\n", _dst_dir.c_str(), e.what());
		out = this;
	}
	std::lock_guard<std::mutex> locker(_state.mtx);
	_state.finished = true;
	return out;
}



void Upload::CheckForUserInput(std::unique_lock<std::mutex> &lock)
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

bool Upload::OnScannedPath(const std::string &path)
{
	struct stat s = {};
	fprintf(stderr, "OnScannedPath: '%s'\n", path.c_str());
	if (sdc_lstat(path.c_str(), &s) != 0) {
		throw std::runtime_error("Failed to stat path");
	}
	if (!S_ISDIR(s.st_mode) && !S_ISREG(s.st_mode)) {
		return false;
	}
	if (!_entries.emplace(path, s).second) {
		return false;
	}

	std::unique_lock<std::mutex> lock(_state.mtx);
	CheckForUserInput(lock);

	_state.stats.total_count = _entries.size();
	if (!S_ISDIR(s.st_mode)) {
		_state.stats.total_size+= s.st_size;
		return false;
	}

	return true;
}

void Upload::Scan()
{
	for (auto path : _items) {
		if (OnScannedPath(path)) {
			path+= '/';
			_scan_depth_limit = 255;
			ScanItem(path);
		}
	}
}

void Upload::ScanItem(const std::string &path)
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
				if (OnScannedPath(subpath)) {
					if (_scan_depth_limit) {
						--_scan_depth_limit;
						subpath+= '/';
						ScanItem(subpath);
						++_scan_depth_limit;
					} else {
						fprintf(stderr, "NetRocks::Item('%s'): depth limit exhausted\n", subpath.c_str());
					}
				}
			}
		}
		closedir(dir);
	}
}

void Upload::Transfer()
{
	std::string path_remote;
	for (const auto &e : _entries) {
		path_remote = _dst_dir;
		path_remote+= e.first.substr(_src_dir_len);
		mode_t mode = 0;
		try {
			mode = _connection->GetMode(path_remote, false);
		} catch (ProtocolError &) { ; }

		if (S_ISDIR(e.second.st_mode)) {
			if (mode == 0) {
				try {
					_connection->DirectoryCreate(path_remote, e.second.st_mode);
				} catch (std::exception &ex) {
					fprintf(stderr, "NetRocks::Upload: %s on mkdir '%s'\n", ex.what(), path_remote.c_str());
				}

			} else if (!S_ISDIR(mode)) {
				fprintf(stderr, "NetRocks::Upload: expected dir but mode=0x%x at '%s'\n",
					mode, path_remote.c_str());
			}

		} else {
			try {
				_connection->FilePut(path_remote, e.first, e.second.st_mode, 0, this);
				if (_mv) {
					if (unlink(e.first.c_str()) == -1) {
						fprintf(stderr, "NetRocks::Transfer: error %d while rmfile '%s'\n",
							errno, e.first.c_str());
					}
				}
			} catch (std::exception &ex) {
				fprintf(stderr, "NetRocks::Transfer: %s on '%s' -> '%s'\n", ex.what(), e.first.c_str(), path_remote.c_str());
			}
		}

		std::unique_lock<std::mutex> lock(_state.mtx);
		if (!S_ISDIR(e.second.st_mode))
			_state.stats.current_size+= e.second.st_size;
		_state.stats.current_count++;
		CheckForUserInput(lock);
	}

	if (_mv) {
		for (auto rev_i = _entries.rbegin(); rev_i != _entries.rend(); ++rev_i) {
			if (S_ISDIR(rev_i->second.st_mode)) {
				if (rmdir(rev_i->first.c_str()) == -1) {
					fprintf(stderr, "NetRocks::Transfer: error %d while rmdir '%s'\n",
						errno, rev_i->first.c_str());
				}
			}
		}
	}
}

bool Upload::OnIOStatus(unsigned long long transferred)
{
	std::unique_lock<std::mutex> lock(_state.mtx);
	_state.stats.current_size+= transferred;
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

