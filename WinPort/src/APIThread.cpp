#include <pthread.h>
#include <mutex>
#include <condition_variable>
#include <time.h>
#include <errno.h>
#include <signal.h>

#include "WinCompat.h"
#include "WinPort.h"

WINPORT_DECL(SetThreadPriority, BOOL, (int priority))
{
	struct sched_param sched;
	int policy;
	pthread_t thread = pthread_self();

	if (pthread_getschedparam(thread, &policy, &sched) < 0) {
		return FALSE;
	}
	if (priority == THREAD_PRIORITY_IDLE) {
		sched.sched_priority = sched_get_priority_min(policy);
	} else if (priority == THREAD_PRIORITY_HIGHEST) {
		sched.sched_priority = sched_get_priority_max(policy);
	} else {
		int min_priority = sched_get_priority_min(policy);
		int max_priority = sched_get_priority_max(policy);
		sched.sched_priority = (min_priority + (max_priority - min_priority) / 2);
	}
	if (pthread_setschedparam(thread, policy, &sched) < 0) {
		return FALSE;
	}

	return TRUE;
}

WINPORT_DECL(SetThreadExecutionState, EXECUTION_STATE, (EXECUTION_STATE es))
{
	return es;
}
