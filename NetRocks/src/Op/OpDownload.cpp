#include "OpDownload.h"
#include "../UI/Confirm.h"

OpDownload::OpDownload(std::shared_ptr<SiteConnection> &connection, int op_mode,
	const std::string &src_dir, const std::string &dst_dir,
	struct PluginPanelItem *items, int items_count, bool mv)
	:
	OpBase(connection, op_mode, src_dir),
	ProgressStateIOUpdater(_state),
	_mv(mv),
	_xdoa(XDOA_ASK),
	_dst_dir(dst_dir)
{
	_enumer = std::make_shared<EnumerRemote>(_entries, _state, _src_dir, items, items_count, true, _connection);
}

bool OpDownload::Do()
{
	if (!IS_SILENT(_op_mode)) {
		if (!XferConfirm(_mv ? XK_MOVE : XK_COPY, XK_DOWNLOAD, _dst_dir).Ask(_xdoa)) {
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
	}

	Transfer();
}


void OpDownload::Transfer()
{
	std::string path_local;
	for (const auto &e : _entries) {
		const std::string &subpath = e.first.substr(_src_dir.size());
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
