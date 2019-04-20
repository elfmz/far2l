#include "FilesGetter.h"

FilesGetter::FilesGetter(std::shared_ptr<SiteConnection> &connection)
	: _connection(connection)
{
}

bool FilesGetter::Do(const std::string &dst_dir, const std::string &src_dir, struct PluginPanelItem *items, int items_count, bool mv, int op_mode)
{
	_mv = mv;
	_op_mode = op_mode;
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

	if (!XferConfirm(_mv ? XK_MOVE : XK_COPY, XK_DOWNLOAD, _dst_dir).Ask(_xdoa)) {
		fprintf(stderr, "NetRocks::FilesGetter: cancel\n");
		return false;
	}

	if (!StartThread()) {
		fprintf(stderr, "NetRocks::FilesGetter: start thread error\n");
		return false;
	}

	XferProgress(_mv ? XK_MOVE : XK_COPY, XK_DOWNLOAD, _dst_dir, _state).Show();

	WaitThread();

	return true;
}


void *FilesGetter::ThreadProc()
{
	void *out = nullptr;
	try {
		ScanItems();
		fprintf(stderr,
			"NetRocks::FilesGetter: _dst_dir='%s' --> count=%lu total_size=%llu\n",
			_dst_dir.c_str(), _entries.size(), _state.stats.total_size);

	} catch (std::exception &e) {
		fprintf(stderr, "NetRocks::FilesGetterThread('%s') ERROR: %s\n", _dst_dir.c_str(), e.what());
		out = this;
	}
	std::lock_guard<std::mutex> locker(_state.mtx);
	_state.finished = true;
	return out;
}



void FilesGetter::CheckForUserInput(std::unique_lock<std::mutex> &lock)
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

void FilesGetter::ScanItems()
{
	FileInformation info = {};
	for (auto path : _items) {
		info.mode = _connection->GetMode(path, true);
		info.size = S_ISDIR(info.mode) ? 0 :_connection->GetSize(path, true);
		if (_entries.emplace(path, info).second) {
			if (S_ISDIR(info.mode)) {
				path+= '/';
				_scan_depth_limit = 255;
				Scan(path);
			}
		
			std::unique_lock<std::mutex> lock(_state.mtx);
			if (!S_ISDIR(info.mode))
				_state.stats.total_size+= info.size;
			_state.stats.total_count = _entries.size();
			CheckForUserInput(lock);
		}
	}
}

void FilesGetter::Scan(const std::string &path)
{
	UnixFileList ufl;
	_connection->DirectoryEnum(path, ufl, 0);
	if (ufl.empty())
		return;

	std::string subpath;
	for (const auto &e : ufl) {
		subpath = path;
		subpath+= '/';
		subpath+= e.name;
		if (_entries.emplace(subpath, e.info).second) {
			if (S_ISDIR(e.info.mode)) {
				if (_scan_depth_limit) {
					subpath+= '/';
					--_scan_depth_limit;
					Scan(subpath);
					++_scan_depth_limit;
				} else {
					fprintf(stderr, "NetRocks::Scan('%s'): depth limit exhausted\n", subpath.c_str());
					// _failed = true;
				}
			}

			std::unique_lock<std::mutex> lock(_state.mtx);
			if (!S_ISDIR(e.info.mode))
				_state.stats.total_size+= e.info.size;
			_state.stats.total_count = _entries.size();
			CheckForUserInput(lock);
		}
	}
}

