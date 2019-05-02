#include "OpDownload.h"
#include "../UI/Confirm.h"
#include "../UI/ConfirmOverwrite.h"
#include "../lng.h"
#include "../Utils.h"

OpDownload::OpDownload(std::shared_ptr<SiteConnection> &connection, int op_mode,
	const std::string &base_dir, const std::string &dst_dir,
	struct PluginPanelItem *items, int items_count, bool mv)
	:
	OpBase(connection, op_mode, base_dir, MNotificationDownload),
	ProgressStateUpdaterCallback(_state),
	_mv(mv),
	_default_xoa(XOA_ASK),
	_dst_dir(dst_dir)
{
	_enumer = std::make_shared<EnumerRemote>(_entries, _state, _base_dir, items, items_count, true, _connection);
	_diffname_suffix = ".NetRocks@";
	_diffname_suffix+= TimeString(TSF_FOR_FILENAME);
}

bool OpDownload::Do()
{
	if (!IS_SILENT(_op_mode)) {
		if (!ConfirmXfer(_mv ? XK_MOVE : XK_COPY, XK_DOWNLOAD, _dst_dir).Ask(_default_xoa)) {
			fprintf(stderr, "NetRocks::Download: cancel\n");
			return false;
		}
	}

	if (_dst_dir.empty()) {
		_dst_dir = "./";
	} else if (_dst_dir[_dst_dir.size() - 1] != '.')
		_dst_dir+= '/';

	if (!StartThread()) {
		fprintf(stderr, "NetRocks::Download: start thread error\n");
		return false;
	}

	if (!WaitThread(IS_SILENT(_op_mode) ? 2000 : 500)) {
		XferProgress(_mv ? XK_MOVE : XK_COPY, XK_DOWNLOAD, _dst_dir, _state).Show();
		WaitThread();
	}

	return true;
}


void OpDownload::Process()
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


void OpDownload::Transfer()
{
	std::string path_local;
	for (const auto &e : _entries) {
		const std::string &subpath = e.first.substr(_base_dir.size());
		path_local = _dst_dir;
		path_local+= subpath;

		{
			std::unique_lock<std::mutex> lock(_state.mtx);
			_state.path = subpath;
			_state.stats.file_complete = 0;
			_state.stats.file_total = S_ISDIR(e.second.mode) ? 0 : e.second.size;
			_state.stats.current_start = TimeMSNow();
			_state.stats.current_paused = std::chrono::milliseconds::zero();
		}

		if (S_ISDIR(e.second.mode)) {
			sdc_mkdir(path_local.c_str(), e.second.mode);

		} else {
			struct stat s = {};
			unsigned long long pos = 0;
			if (sdc_stat(path_local.c_str(), &s) == 0) {
				auto xoa = _default_xoa;
				if (xoa == XOA_ASK) {
					xoa = ConfirmOverwrite(_mv ? XK_MOVE : XK_COPY, XK_DOWNLOAD, path_local,
						e.second.modification_time, e.second.size, s.st_mtim, s.st_size).Ask(_default_xoa);
					if (xoa == XOA_CANCEL) {
						return;
					}
				}
				if (xoa == XOA_OVERWRITE_IF_NEWER) {
					if (e.second.modification_time.tv_sec < s.st_mtim.tv_sec ||
					(e.second.modification_time.tv_sec == s.st_mtim.tv_sec && e.second.modification_time.tv_nsec <= s.st_mtim.tv_nsec)) {
						xoa = XOA_SKIP;
					} else {
						xoa = XOA_OVERWRITE;
					}
				}
				if (xoa == XOA_SKIP) {
					std::unique_lock<std::mutex> lock(_state.mtx);
					_state.stats.all_complete+= e.second.size;
					_state.stats.file_complete+= e.second.size;
					_state.stats.count_complete++;
					continue;
				}
				if (xoa == XOA_RESUME) {
					_state.stats.all_complete+= s.st_size;
					_state.stats.file_complete+= s.st_size;
					pos = s.st_size;

				} else if (xoa == XOA_CREATE_DIFFERENT_NAME) {
					path_local+= _diffname_suffix;
				}
			}
			try {
				_connection->FileGet(e.first, path_local, e.second.mode, pos, this);
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

		ProgressStateUpdate psu(_state);
		_state.stats.count_complete++;
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
