#include "ProgressStateUpdate.h"

ProgressStateUpdate::ProgressStateUpdate(ProgressState &state)
	:std::unique_lock<std::mutex>(state.mtx)
{
	for (;;) {
		if (state.aborting)
			throw std::runtime_error("Aborted");
		if (!state.paused)
			break;

		unlock();
		usleep(1000000);
		lock();
	}
}

/////////

ProgressStateIOUpdater::ProgressStateIOUpdater(ProgressState &state)
	: _state(state)
{
}


bool ProgressStateIOUpdater::OnIOStatus(unsigned long long transferred)
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
