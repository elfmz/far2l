#include "Upload.h"

Upload::Upload(std::shared_ptr<SiteConnection> &connection)
	: ProgressStateIOUpdater(_state), _connection(connection)
{
}

bool Upload::Do(const std::string &dst_dir, const std::string &src_dir, struct PluginPanelItem *items, int items_count, bool mv, int op_mode)
{
	_mv = mv;
	_op_mode = op_mode;
	_src_dir_len = src_dir.size();
	_dst_dir = dst_dir;
	_xdoa = XDOA_ASK;
	_state.Reset();
	_entries.clear();

	_enumer = std::make_shared<EnumerLocal>(_entries, _state, src_dir, items, items_count, true);

	if (!IS_SILENT(op_mode)) {
		if (!XferConfirm(_mv ? XK_MOVE : XK_COPY, XK_UPLOAD, _dst_dir).Ask(_xdoa)) {
			fprintf(stderr, "NetRocks::Upload: cancel\n");
			return false;
		}
	}

	if (_dst_dir.empty()) {
		_dst_dir = "./";
	} else if (dst_dir[_dst_dir.size() - 1] != '.')
		_dst_dir+= '/';

	if (!StartThread()) {
		fprintf(stderr, "NetRocks::Upload: start thread error\n");
		return false;
	}

	if (!WaitThread(500)) {
		XferProgress(_mv ? XK_MOVE : XK_COPY, XK_UPLOAD, _dst_dir, _state).Show();
		WaitThread();
	}

	return true;
}


void *Upload::ThreadProc()
{
	void *out = nullptr;
	try {
		if (_enumer) {
			_enumer->Scan();
			_enumer.reset();
		}

		Transfer();
		fprintf(stderr,
			"NetRocks::Upload: _dst_dir='%s' --> count=%lu all_total=%llu\n",
			_dst_dir.c_str(), _entries.size(), _state.stats.all_total);

	} catch (std::exception &e) {
		fprintf(stderr, "NetRocks::UploadThread('%s') ERROR: %s\n", _dst_dir.c_str(), e.what());
		out = this;
	}
	std::lock_guard<std::mutex> locker(_state.mtx);
	_state.finished = true;
	return out;
}



void Upload::Transfer()
{
	std::string path_remote;
	for (const auto &e : _entries) {
		const std::string &subpath = e.first.substr(_src_dir_len);
		path_remote = _dst_dir;
		path_remote+= subpath;
		mode_t mode = 0;
		try {
			mode = _connection->GetMode(path_remote, false);
		} catch (ProtocolError &) { ; }

		{
			std::unique_lock<std::mutex> lock(_state.mtx);
			_state.path = subpath;
			_state.stats.file_complete = 0;
			_state.stats.file_total = S_ISDIR(e.second.st_mode) ? 0 : e.second.st_size;
		}

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

		ProgressStateUpdate psu(_state);
		_state.stats.count_complete++;
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
