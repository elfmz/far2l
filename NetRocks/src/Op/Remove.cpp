#include "Remove.h"

Remove::Remove(std::shared_ptr<SiteConnection> &connection)
	: _connection(connection)
{
}

bool Remove::Do(const std::string &src_dir, struct PluginPanelItem *items, int items_count, int op_mode)
{
	_op_mode = op_mode;
	_src_dir_len = src_dir.size();
	_state.Reset();

	_entries.clear();
	_enumer = std::make_shared<EnumerRemote>(_entries, _state, src_dir, items, items_count, true, _connection);

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

	if (!WaitThread(500)) {
		RemoveProgress(src_dir, _state).Show();
		WaitThread();
	}

	return true;
}


void *Remove::ThreadProc()
{
	void *out = nullptr;
	try {
		if (_enumer) {
			_enumer->Scan();
			_enumer.reset();
		}

		Process();
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

void Remove::Process()
{
	for (auto rev_i = _entries.rbegin(); rev_i != _entries.rend(); ++rev_i) {
		try {
			const std::string &subpath = rev_i->first.substr(_src_dir_len);

			if (S_ISDIR(rev_i->second.mode)) {
				_connection->DirectoryDelete(rev_i->first);
			} else {
				_connection->FileDelete(rev_i->first);
			}

			ProgressStateUpdate psu(_state);
			_state.path = subpath;
			if (!S_ISDIR(rev_i->second.mode))
				_state.stats.all_complete+= rev_i->second.size;
			_state.stats.count_complete++;

		} catch  (std::exception &ex) {
			fprintf(stderr, "NetRocks::Remove: %s while %s '%s'\n", ex.what(),
				S_ISDIR(rev_i->second.mode) ? "rmdir" : "rmfile", rev_i->first.c_str());
		}
	}
}
