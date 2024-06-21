#pragma once
#include <unistd.h>
#include <errno.h>

//todo: use this everywhere else. Likely sdc_ is better place for that.
template<class V, V BADV, typename ... ARGS>
	static V os_call_v(V (*pfn)(ARGS ... args), ARGS ... args)
{
	for (;;) {
		V r = pfn(args ...);
		if (r == BADV && (errno == EAGAIN || errno == EINTR)) {
			usleep(10000);
		} else
			return r;
	}
}

template<class V, typename ... ARGS>
	static V *os_call_pv(V *(*pfn)(ARGS ... args), ARGS ... args)
{
	return os_call_v<V *, nullptr>(pfn, args ... );
}

template<typename ... ARGS>
	static int os_call_int(int (*pfn)(ARGS ... args), ARGS ... args)
{
	return os_call_v<int, -1>(pfn, args ... );
}

template<typename ... ARGS>
	static ssize_t os_call_ssize(ssize_t (*pfn)(ARGS ... args), ARGS ... args)
{
	return os_call_v<ssize_t, (ssize_t)-1>(pfn, args ... );
}

