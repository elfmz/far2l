#pragma once
#include <string>
#include <vector>
#include <memory>

struct Mountpoints;

/* This class detects if path points to device that is best to be
 * accessed in multi-thread parallel manner, like SSD drives.
 * Currently works only under Linux, others defaulted to <true>.
 */
class MountInfo
{
	std::shared_ptr<Mountpoints> _mountpoints;
	char _mtfs = 0;

public:
	MountInfo(bool query_space);

	std::string GetFileSystem(const std::string &path) const;

	/// Returns true if path fine to be used multi-threaded-ly
	bool IsMultiThreadFriendly(const std::string &path) const;
};
