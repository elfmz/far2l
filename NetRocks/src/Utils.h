#pragma once
#include <chrono>
#include <string>

std::chrono::milliseconds TimeMSNow();

enum TimeStringFormat
{
	TSF_FOR_UI,
	TSF_FOR_FILENAME
};

std::string TimeString(const struct tm &t, TimeStringFormat tsf);
std::string TimeString(const struct timespec &ts, TimeStringFormat tsf);
std::string TimeString(TimeStringFormat tsf);

void AbbreviateFilePath(std::string &path, size_t needed_length);

const char *FileSizeToFractionAndUnits(unsigned long long &value);
std::string FileSizeString(unsigned long long value);


