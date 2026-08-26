#include "DirectoryEnumCache.h"

DirectoryEnumCache::DirectoryEnumCache(unsigned int expiration)
	: _expiration((time_t)expiration)
{
}

bool DirectoryEnumCache::HasValidEntries()
{
	time_t ts = time(NULL);
	for (auto it = _dir_enum_cache.begin(); it != _dir_enum_cache.end(); ) {
		if (it->second->ts > ts || it->second->ts + _expiration < ts) {
			it = _dir_enum_cache.erase(it);

		} else {
			return true;
		}
	}

	return false;
}

void DirectoryEnumCache::Clear()
{
	_dir_enum_cache.clear();
}

void DirectoryEnumCache::Remove(const std::string &path, const std::string &name)
{
	auto it = _dir_enum_cache.find(path);
	if (it == _dir_enum_cache.end()) {
		return;
	}

	time_t ts = time(NULL);
	if (it->second->ts > ts || it->second->ts + _expiration < ts) {
		_dir_enum_cache.erase(it);
		return;
	}

	if (it->second->index.empty()) {
		for (auto entry_it = it->second->begin(); entry_it != it->second->end(); ++entry_it) {
			it->second->index.emplace(entry_it->name, entry_it);
		}
	}

	auto index_it = it->second->index.find(name);
	if (index_it != it->second->index.end()) {
		it->second->erase(index_it->second);
		it->second->index.erase(index_it);
	}
}

void DirectoryEnumCache::Remove(const std::string &path)
{
	auto it = _dir_enum_cache.find(path);
	if (it != _dir_enum_cache.end()) {
		_dir_enum_cache.erase(it);
	}
}

class CachedDirectoryEnumer : public IDirectoryEnumer
{
	std::shared_ptr<DirectoryEnumCache::DirCacheEntry> _dce_sp;
	DirectoryEnumCache::DirCacheEntry::const_iterator _it;

public:
	CachedDirectoryEnumer(std::shared_ptr<DirectoryEnumCache::DirCacheEntry> &dce_sp)
		: _dce_sp(dce_sp)
	{
		_it = _dce_sp->begin();
	}

	virtual bool Enum(std::string &name, std::string &owner, std::string &group, FileInformation &file_info)
	{
		if (_it == _dce_sp->end()) {
			return false;
		}

		name = _it->name;
		owner = _it->owner;
		group = _it->group;
		file_info = _it->file_info;

		++_it;
		return true;
	}
};


std::shared_ptr<IDirectoryEnumer> DirectoryEnumCache::GetCachedDirectoryEnumer(const std::string &path)
{
	auto it = _dir_enum_cache.find(path);
	if (it == _dir_enum_cache.end()) {
		return std::shared_ptr<IDirectoryEnumer>();
	}

	time_t ts = time(NULL);
	if (it->second->ts > ts || it->second->ts + _expiration < ts) {
		_dir_enum_cache.erase(it);
		return std::shared_ptr<IDirectoryEnumer>();
	}

	return std::shared_ptr<IDirectoryEnumer>(new CachedDirectoryEnumer(it->second));
}

class CachingWrapperDirectoryEnumer : public IDirectoryEnumer
{
	std::shared_ptr<DirectoryEnumCache::DirCacheEntry> _dce_sp;
	std::shared_ptr<IDirectoryEnumer> _enumer;
	bool _enumed_til_the_end = false;

public:
	CachingWrapperDirectoryEnumer(std::shared_ptr<DirectoryEnumCache::DirCacheEntry> &dce_sp, std::shared_ptr<IDirectoryEnumer> &enumer)
		: _dce_sp(dce_sp), _enumer(enumer)
	{
	}

	virtual ~CachingWrapperDirectoryEnumer()
	{
		try {
			if (!_enumed_til_the_end) {
				std::string name, owner, group;
				FileInformation file_info;
				while (Enum(name, owner, group, file_info));
			}

			_dce_sp->ts = time(NULL);

		} catch (std::exception &) {
			_dce_sp->ts = 0;
		}
	}

	virtual bool Enum(std::string &name, std::string &owner, std::string &group, FileInformation &file_info)
	{
		if (!_enumer->Enum(name, owner, group, file_info)) {
			_enumed_til_the_end = true;
			return false;
		}

		_dce_sp->emplace_back();

		auto &e = _dce_sp->back();
		e.name = name;
		e.owner = owner;
		e.group = group;
		e.file_info = file_info;

		return true;
	}
};

std::shared_ptr<IDirectoryEnumer> DirectoryEnumCache::GetCachingWrapperDirectoryEnumer(const std::string &path, std::shared_ptr<IDirectoryEnumer> &enumer)
{
	time_t ts = time(NULL);
	if (_last_expiration_inspection > ts || _last_expiration_inspection + _expiration < ts) {
		for (auto it = _dir_enum_cache.begin(); it != _dir_enum_cache.end(); ) {
			if (it->second->ts > ts || it->second->ts + _expiration < ts) {
				it = _dir_enum_cache.erase(it);
			} else {
				++it;
			}
		}
		_last_expiration_inspection = ts;
	}

	auto &dce_sp_ref = _dir_enum_cache[path];
	if (dce_sp_ref) {
		dce_sp_ref->clear();
		dce_sp_ref->index.clear();
		dce_sp_ref->ts = 0;

	} else {
		dce_sp_ref = std::make_shared<DirCacheEntry>();
	}

	return std::shared_ptr<IDirectoryEnumer>(new CachingWrapperDirectoryEnumer(dce_sp_ref, enumer));
}
