#pragma once
#include <string>
#include <map>
#ifdef __FreeBSD__
# include <sys/types.h>
#endif

struct FileInformation
{
	timespec access_time;
	timespec modification_time;
	timespec status_change_time;
	unsigned long long size;
	mode_t mode;
};

typedef std::map<std::string, FileInformation> Path2FileInformation;

