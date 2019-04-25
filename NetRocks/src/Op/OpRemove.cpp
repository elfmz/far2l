#include "OpRemove.h"
#include "../UI/Confirm.h"


OpRemove::OpRemove(std::shared_ptr<SiteConnection> &connection, int op_mode,
	const std::string &src_dir, struct PluginPanelItem *items, int items_count)
	:
	OpBase(connection, op_mode, src_dir)
{
	_enumer = std::make_shared<EnumerRemote>(_entries, _state, _src_dir, items, items_count, true, _connection);
}


bool OpRemove::Do()
{
	if (!IS_SILENT(_op_mode)) {
		if (!RemoveConfirm(_src_dir).Ask()) {
			fprintf(stderr, "NetRocks::Remove: cancel\n");
			return false;
		}
	}

	if (!StartThread()) {
		fprintf(stderr, "NetRocks::Remove: start thread error\n");
		return false;
	}

	if (!WaitThread(IS_SILENT(_op_mode) ? 2000 : 500)) {
		RemoveProgress(_src_dir, _state).Show();
		WaitThread();
	}

	return true;
}


void OpRemove::Process()
{
	if (_enumer) {
		_enumer->Scan();
		_enumer.reset();
	}

	for (auto rev_i = _entries.rbegin(); rev_i != _entries.rend(); ++rev_i) {
		try {
			const std::string &subpath = rev_i->first.substr(_src_dir.size());

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
