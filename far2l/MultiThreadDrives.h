#pragma once
#include <string>
#include <vector>

/* This class detects if path points to device that is best to be
 * accessed in multi-thread parallel manner, like SSD drives.
 * Currently works only under Linux, others defaulted to <true>.
 */
class MultiThreadDrives
{
	std::vector<std::pair<std::string, bool> > _mountpoint_2_multithreaded;

public:
	MultiThreadDrives();

	/// Returns true if path fine to be used multi-threaded-ly
	bool Check(const std::string &path);
};
