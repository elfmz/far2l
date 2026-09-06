#pragma once
#include <string>
#include <map>
# include <sys/types.h>
# include <sys/stat.h>

#define DEFAULT_ACCESS_MODE_FILE 		(S_IWUSR | S_IRUSR | S_IRGRP | S_IROTH)
#define DEFAULT_ACCESS_MODE_DIRECTORY 		(S_IWUSR | S_IRUSR | S_IRGRP | S_IROTH | S_IXUSR | S_IXGRP | S_IXOTH)

struct FileInformation
{
	timespec access_time;
	timespec modification_time;
	timespec status_change_time;
	unsigned long long size;
	mode_t mode;
};

typedef std::map<std::string, FileInformation> Path2FileInformation;

