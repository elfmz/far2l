#pragma once
#include <string>
#include <vector>
#include <memory>

struct Mountpoint
{
	std::string path;
	std::string filesystem;
	std::string device;
	bool multi_thread_friendly;

	// following fields valid only if for_location_menu set to true
	volatile bool bad;
	volatile bool read_only;
	volatile unsigned long long total;
	volatile unsigned long long avail;
	volatile unsigned long long free_;
};

struct Mountpoints;

/*
	This class enumerates mountpoints and provides extra info
	like disk sizes (if for_location_menu enabled) and also
	(if for_location_menu disabled) - it provides ability to check
	if specified path points to device that is best to be
	accessed in multi-thread parallel manner, like SSD drives.
	Later currently works only under Linux, others defaulted to <true>.
*/
class MountInfo
{
	std::shared_ptr<Mountpoints> _mountpoints;
	char _mtfs = 0;

public:
	MountInfo(bool for_location_menu = false);

	const std::vector<Mountpoint> &Enum() const;

	std::string GetFileSystem(const std::string &path) const;

	/// Returns true if path fine to be used multi-threaded-ly
	bool IsMultiThreadFriendly(const std::string &path) const;
};
