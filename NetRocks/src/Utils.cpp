#include "Utils.h"

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
				t.tm_year, t.tm_mon, t.tm_mday, t.tm_hour, t.tm_min, t.tm_sec);
			break;

		case TSF_FOR_FILENAME:
			snprintf(buf, sizeof(buf) - 1, "%04d-%02d-%02d_%02d-%02d-%02d",
				t.tm_year, t.tm_mon, t.tm_mday, t.tm_hour, t.tm_min, t.tm_sec);
			break;

		default:
			abort();
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

void AbbreviateFilePath(std::string &path, size_t needed_length)
{
	size_t len = path.size();
	if (needed_length < 3) {
		needed_length = 3;
	}
	if (len > needed_length) {
		size_t delta = len - (needed_length - 3);
		path.replace((path.size() - delta) / 2, delta, "...");
	}
}

const char *FileSizeToFractionAndUnits(unsigned long long &value)
{
	if (value > 100ll * 1024ll * 1024ll * 1024ll * 1024ll) {
		value = (1024ll * 1024ll * 1024ll * 1024ll);
		return "TB";
	}

	if (value > 100ll * 1024ll * 1024ll * 1024ll) {
		value = (1024ll * 1024ll * 1024ll);
		return "GB";
	}

	if (value > 100ll * 1024ll * 1024ll ) {
		value = (1024ll * 1024ll);
		return "MB";

	}

	if (value > 100ll * 1024ll ) {
		value = (1024ll);
		return "KB";
	}

	value = 1;
	return "B";
}

std::string FileSizeString(unsigned long long value)
{
	unsigned long long fraction = value;
	const char *units = FileSizeToFractionAndUnits(fraction);
	value/= fraction;

	char str[0x100] = {};
	snprintf(str, sizeof(str) - 1, "%lld %s", value, units);
	return str;
}
