#pragma once
#include <stdint.h>
#include <time.h>
#include <string>
#include <list>
#include <memory>
#include "Protocol.h"
#include "../FileInformation.h"

class DirectoryEnumCache
{
	friend class CachedDirectoryEnumer;
	friend class CachingWrapperDirectoryEnumer;

	time_t _expiration;

	struct DirCachedListEntry
	{
		std::string name;
		std::string owner;
		std::string group;
		FileInformation file_info;
	};

	struct DirCacheEntry : std::enable_shared_from_this<DirCacheEntry>, std::list<DirCachedListEntry>
	{
		time_t ts = 0;

		std::map<std::string, iterator> index;
	};

	std::map<std::string, std::shared_ptr<DirCacheEntry> > _dir_enum_cache;

	time_t _last_expiration_inspection = 0;


public:
	DirectoryEnumCache(unsigned int expiration);

	bool HasValidEntries();

	void Clear();
	void Remove(const std::string &path, const std::string &name);
	void Remove(const std::string &path);

	std::shared_ptr<IDirectoryEnumer> GetCachedDirectoryEnumer(const std::string &path);
	std::shared_ptr<IDirectoryEnumer> GetCachingWrapperDirectoryEnumer(const std::string &path, std::shared_ptr<IDirectoryEnumer> &enumer);
};
