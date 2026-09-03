#include <utils.h>
#include <TimeUtils.h>
#include "OpRemove.h"
#include "../UI/Activities/Confirm.h"
#include "../UI/Activities/ComplexOperationProgress.h"


OpRemove::OpRemove(int op_mode, std::shared_ptr<IHost> &base_host,
	const std::string &base_dir, struct PluginPanelItem *items, int items_count)
	:
	OpBase(op_mode, base_host, base_dir)
{
	_enumer = std::make_shared<Enumer>(_entries, _base_host, _base_dir, items, items_count, false, _state, _wea_state);
}


bool OpRemove::Do()
{
	if (!IS_SILENT(_op_mode)) {
		if (!ConfirmRemove(_base_dir).Ask()) {
			fprintf(stderr, "NetRocks::Remove: cancel\n");
			return false;
		}
	}

	if (!StartThread()) {
		fprintf(stderr, "NetRocks::Remove: start thread error\n");
		return false;
	}

	if (!WaitThreadBeforeShowProgress()) {
		RemoveProgress p(_base_dir, _state, _wea_state);
		p.Show();
		WaitThread();
	}

	return true;
}


void OpRemove::Process()
{
	if (_enumer) {
		_enumer->Scan();
		_enumer.reset();

		std::lock_guard<std::mutex> locker(_state.mtx);
		_state.stats.total_start = TimeMSNow();
		_state.stats.total_paused = std::chrono::milliseconds::zero();
	}

	for (auto rev_i = _entries.rbegin(); rev_i != _entries.rend(); ++rev_i) {
		{
			std::lock_guard<std::mutex> locker(_state.mtx);
			_state.stats.current_start = TimeMSNow();
			_state.stats.current_paused = std::chrono::milliseconds::zero();
		}

		WhatOnErrorWrap<WEK_REMOVE>(_wea_state, _state, _base_host.get(), _base_dir,
			[&] () mutable
			{
				const std::string &subpath = rev_i->first.substr(_base_dir.size());

				if (S_ISDIR(rev_i->second.mode)) {
					_base_host->DirectoryDelete(rev_i->first);
				} else {
					_base_host->FileDelete(rev_i->first);
				}

				ProgressStateUpdate psu(_state);
				_state.path = subpath;
				if (!S_ISDIR(rev_i->second.mode))
					_state.stats.all_complete+= rev_i->second.size;
				_state.stats.count_complete++;
			}
		);
	}
}
