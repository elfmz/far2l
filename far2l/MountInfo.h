#pragma once
#include <string>
#include <vector>

/* This class detects if path points to device that is best to be
 * accessed in multi-thread parallel manner, like SSD drives.
 * Currently works only under Linux, others defaulted to <true>.
 */
class MountInfo
{
	struct Mountpoint {
		std::string path;
		std::string filesystem;
		bool multi_thread_friendly;
	};
	struct Mountpoints : std::vector<Mountpoint> {} _mountpoints;
	char _mtfs = 0;

public:
	MountInfo();

	std::string GetFileSystem(const std::string &path) const;

	/// Returns true if path fine to be used multi-threaded-ly
	bool IsMultiThreadFriendly(const std::string &path) const;
};
