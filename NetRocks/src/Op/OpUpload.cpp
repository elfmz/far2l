#include "OpUpload.h"
#include "../UI/Confirm.h"
#include "../lng.h"
#include "../Utils.h"

OpUpload::OpUpload(std::shared_ptr<SiteConnection> &connection, int op_mode,
	const std::string &base_dir, const std::string &dst_dir,
	struct PluginPanelItem *items, int items_count, bool mv)
	:
	OpBase(connection, op_mode, base_dir, MNotificationUpload),
	ProgressStateUpdaterCallback(_state),
	_mv(mv),
	_default_xoa(XOA_ASK),
	_dst_dir(dst_dir)
{
	_enumer = std::make_shared<EnumerLocal>(_entries, _state, _base_dir, items, items_count, true);
}

bool OpUpload::Do()
{
	if (!IS_SILENT(_op_mode)) {
		if (!ConfirmXfer(_mv ? XK_MOVE : XK_COPY, XK_UPLOAD, _dst_dir).Ask(_default_xoa)) {
			fprintf(stderr, "NetRocks::Upload: cancel\n");
			return false;
		}
	}

	if (_dst_dir.empty()) {
		_dst_dir = "./";
	} else if (_dst_dir[_dst_dir.size() - 1] != '.')
		_dst_dir+= '/';

	if (!StartThread()) {
		fprintf(stderr, "NetRocks::Upload: start thread error\n");
		return false;
	}

	if (!WaitThread(IS_SILENT(_op_mode) ? 2000 : 500)) {
		XferProgress(_mv ? XK_MOVE : XK_COPY, XK_UPLOAD, _dst_dir, _state).Show();
		WaitThread();
	}

	return true;
}


void OpUpload::Process()
{
	if (_enumer) {
		_enumer->Scan();
		_enumer.reset();

		std::lock_guard<std::mutex> locker(_state.mtx);
		_state.stats.total_start = TimeMSNow();
		_state.stats.total_paused = std::chrono::milliseconds::zero();
	}

	Transfer();
}

void OpUpload::Transfer()
{
	std::string path_remote;
	for (const auto &e : _entries) {
		const std::string &subpath = e.first.substr(_base_dir.size());
		path_remote = _dst_dir;
		path_remote+= subpath;
		mode_t mode = 0;
		{
			std::unique_lock<std::mutex> lock(_state.mtx);
			_state.path = subpath;
			_state.stats.file_complete = 0;
			_state.stats.file_total = S_ISDIR(e.second.st_mode) ? 0 : e.second.st_size;
			_state.stats.current_start = TimeMSNow();
			_state.stats.current_paused = std::chrono::milliseconds::zero();
		}

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
