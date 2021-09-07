#pragma once
#include <string>

struct IFSNotify
{
	virtual ~IFSNotify() {};
	virtual bool Check() const noexcept = 0;
};

enum FSNotifyWhat
{
	FSNW_NAMES,
	FSNW_NAMES_AND_STATS
};

IFSNotify *IFSNotify_Create(const std::string &pathname, bool watch_subtree, FSNotifyWhat what);
