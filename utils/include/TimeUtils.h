#pragma once
#include <string>
#include <chrono>
#include <sys/types.h>

std::chrono::milliseconds TimeMSNow();

enum TimeStringFormat
{
	TSF_FOR_UI,
	TSF_FOR_FILENAME
};

std::string TimeString(const struct tm &t, TimeStringFormat tsf);
std::string TimeString(const struct timespec &ts, TimeStringFormat tsf);
std::string TimeString(TimeStringFormat tsf);

