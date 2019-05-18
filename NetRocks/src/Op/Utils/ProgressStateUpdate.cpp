#include <utils.h>
#include <TimeUtils.h>
#include <unistd.h>
#include "ProgressStateUpdate.h"
#include "Erroring.h"

ProgressStateUpdate::ProgressStateUpdate(ProgressState &state)
	:std::unique_lock<std::mutex>(state.mtx)
{
	std::chrono::milliseconds pause_ticks = {};

	for (;;) {
		if (state.aborting)
			throw AbortError();
		if (!state.paused)
			break;

		unlock();
		if (pause_ticks.count() == 0) {
			pause_ticks = TimeMSNow();
		}
		usleep(1000000);
		lock();
	}

	if (pause_ticks.count() != 0) {
		pause_ticks = TimeMSNow() - pause_ticks;
		state.stats.total_paused+= pause_ticks;
		state.stats.current_paused+= pause_ticks;
	}
}
