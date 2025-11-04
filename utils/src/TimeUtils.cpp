#include "TimeUtils.h"
#include "debug.h"
#include <stdlib.h>

std::chrono::milliseconds TimeMSNow()
{
	return std::chrono::duration_cast< std::chrono::milliseconds >( std::chrono::system_clock::now().time_since_epoch());
}

std::string TimeString(const struct tm &t, TimeStringFormat tsf)
{
	char buf[0x40] = {};

	switch (tsf) {
		case TSF_FOR_UI:
			snprintf(buf, sizeof(buf) - 1, "%04d/%02d/%02d %02d:%02d.%02d",
				t.tm_year + 1900, t.tm_mon + 1, t.tm_mday, t.tm_hour, t.tm_min, t.tm_sec);
			break;

		case TSF_FOR_FILENAME:
			snprintf(buf, sizeof(buf) - 1, "%04d-%02d-%02d_%02d-%02d-%02d",
				t.tm_year + 1900, t.tm_mon + 1, t.tm_mday, t.tm_hour, t.tm_min, t.tm_sec);
			break;

		default:
			ABORT();
	}

	return buf;
}

std::string TimeString(const struct timespec &ts, TimeStringFormat tsf)
{
	struct tm t = {};
	time_t tt = ts.tv_sec;
	localtime_r(&tt, &t);
	return TimeString(t, tsf);
}

std::string TimeString(TimeStringFormat tsf)
{
	time_t now = time(NULL);
	struct tm t = {};
	localtime_r(&now, &t);
	return TimeString(t, tsf);
}

int TimeSpecCompare(const struct timespec &ts_left, const struct timespec &ts_right)
{
	if (ts_left.tv_sec < ts_right.tv_sec)
		return -1;
	if (ts_left.tv_sec > ts_right.tv_sec)
		return 1;

	if (ts_left.tv_nsec < ts_right.tv_nsec)
		return -1;
	if (ts_left.tv_nsec > ts_right.tv_nsec)
		return 1;

	return 0;
}
