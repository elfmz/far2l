#include "AWSFile.h"
#include <cstring>

timespec AWSFile::FromISO8601(const std::string &s)
{
	timespec ts = {};
	if (s.empty()) return ts;

	struct tm tm = {};
	if (strptime(s.c_str(), "%Y-%m-%dT%H:%M:%S", &tm)) {
		tm.tm_isdst = 0;
		ts.tv_sec = timegm(&tm);
	}
	return ts;
}

AWSFile::AWSFile(std::string name, bool isFile)
	: name(std::move(name)), isFile(isFile)
{
	modified.tv_sec  = 0;
	modified.tv_nsec = 0;
}

AWSFile::AWSFile(std::string name, bool isFile, const std::string &iso8601_date, long long size)
	: name(std::move(name)), isFile(isFile), size(size)
{
	modified = FromISO8601(iso8601_date);
}

void AWSFile::UpdateModification(const std::string &iso8601_date)
{
	timespec ts = FromISO8601(iso8601_date);
	if (ts.tv_sec > modified.tv_sec) {
		modified.tv_sec = ts.tv_sec;
	}
}
