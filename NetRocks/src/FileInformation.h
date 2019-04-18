#pragma once

struct FileInformation
{
	timespec access_time;
	timespec modification_time;
	timespec status_change_time;
	unsigned long long size;
	mode_t mode;
};

