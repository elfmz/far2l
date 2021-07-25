#include <unistd.h>
#include <stdio.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <fstream>
#include "MultiThreadDrives.h"
#include <ScopeHelpers.h>
#include <os_call.hpp>

MultiThreadDrives::MultiThreadDrives()
{
#ifdef __linux__
	struct stat s;
	if (stat(InMyConfig("mtfs").c_str(), &s) == 0) {
		fprintf(stderr, "%s: forced\n", __FUNCTION__);
		return;
	}

	std::ifstream is("/proc/mounts");
	if (is.is_open()) {
		std::string line, sys_path;
		std::vector<std::string> parts;
		while (std::getline(is, line)) {
			parts.clear();
			StrExplode(parts, line, " \t");
			if (parts.size() > 1 && StrStartsFrom(parts[1], "/")) {
				bool guessing;
				if (StrStartsFrom(parts[0], "/dev/")) {
					sys_path = "/sys/block/";
					sys_path+= parts[0].substr(5);
					// strip device suffix, sda1 -> sda, mmcblk0p1 -> mmcblk0
					while (sys_path.size() > 12 && stat(sys_path.c_str(), &s) == -1) {
						sys_path.resize(sys_path.size() - 1);
					}
					sys_path+= "/queue/rotational";
					char c = '?';
					FDScope fd(open(sys_path.c_str(), O_RDONLY));
					if (fd.Valid()) {
						os_call_ssize(read, (int)fd, (void *)&c, sizeof(c));
					} else {
						fprintf(stderr, "%s: can't read '%s'\n", __FUNCTION__, sys_path.c_str());
					}
					guessing = (c == '0');
				} else {
					guessing = (parts[0] == "tmpfs" || parts[0] == "proc");
				}
				fprintf(stderr, "%s: %d for '%s'\n", __FUNCTION__, guessing, parts[0].c_str());
				_mountpoint_2_multithreaded.emplace_back(std::make_pair(parts[1], guessing));
			}
		}
	}
	if (_mountpoint_2_multithreaded.empty()) {
		fprintf(stderr, "%s: failed to parse /proc/mounts\n", __FUNCTION__);
	}
#endif

	// TODO: BSD, MacOS
}


bool MultiThreadDrives::Check(const std::string &path)
{
	bool out = true;
	size_t longest_match = 0;
	for (const auto &it : _mountpoint_2_multithreaded) {
		if (it.first.size() > longest_match && StrStartsFrom(path, it.first.c_str())) {
			longest_match = it.first.size();
			out = it.second;
		}
	}
	return out;
}
