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

ProgressStateUpdaterCallback::ProgressStateUpdaterCallback(ProgressState &state)
	: _state_ref(state)
{
}


bool ProgressStateUpdaterCallback::OnIOStatus(unsigned long long transferred)
{
	std::unique_lock<std::mutex> lock(_state_ref.mtx);
	_state_ref.stats.all_complete+= transferred;
	_state_ref.stats.file_complete+= transferred;
	for (;;) {
		if (_state_ref.aborting)
			return false;
		if (!_state_ref.paused)
			return true;

		lock.unlock();
		usleep(1000000);
		lock.lock();
	}
}

bool ProgressStateUpdaterCallback::OnEnumEntry()
{
	//usleep(10000);
	std::unique_lock<std::mutex> lock(_state_ref.mtx);
	_state_ref.stats.count_complete++;
	for (;;) {
		if (_state_ref.aborting)
			return false;
		if (!_state_ref.paused)
			return true;

		lock.unlock();
		usleep(1000000);
		lock.lock();
	}
}
