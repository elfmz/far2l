#include "ProgressStateUpdate.h"
#include "../../Globals.h"

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

static bool ProcessAbortingPaused(ProgressState &state_ref, std::unique_lock<std::mutex> &lock)
{
	for (std::chrono::milliseconds pause_ticks = {};;) {
		if (state_ref.aborting) {
			return false;
		}
		if (!state_ref.paused) {
			if (pause_ticks.count() != 0) {
				pause_ticks = TimeMSNow() - pause_ticks;
				state_ref.stats.total_paused+= pause_ticks;
				state_ref.stats.current_paused+= pause_ticks;
			}
			return true;
		}

		lock.unlock();
		if (pause_ticks.count() == 0) {
			pause_ticks = TimeMSNow();
		}
		usleep(1000000);
		lock.lock();
	}
}

bool ProgressStateUpdaterCallback::OnIOStatus(unsigned long long transferred)
{
	std::unique_lock<std::mutex> lock(_state_ref.mtx);
	_state_ref.stats.all_complete+= transferred;
	_state_ref.stats.file_complete+= transferred;
	return ProcessAbortingPaused(_state_ref, lock);
}

bool ProgressStateUpdaterCallback::OnEnumEntry()
{
	//usleep(10000);
	std::unique_lock<std::mutex> lock(_state_ref.mtx);
	_state_ref.stats.count_complete++;
	return ProcessAbortingPaused(_state_ref, lock);
}
