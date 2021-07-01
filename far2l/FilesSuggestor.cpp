#include "FilesSuggestor.hpp"
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include "../WinPort/sudo.h"

FilesSuggestor::~FilesSuggestor()
{
	EnsureThreadStopped();
}

void FilesSuggestor::EnsureThreadStopped()
{
	bool need_wait_thread = false;
	{
		std::unique_lock<std::mutex> lock(_mtx);
		if (_state == S_ACTIVATED) {
			_state = S_STOPPING;
			need_wait_thread = true;
		}
	}

	if (need_wait_thread) {
		WaitThread();
	}
}

bool FilesSuggestor::Start(const std::string &dir_path, const struct stat &dir_st)
{
	fprintf(stderr, "FilesSuggestor: enum '%s'\n", dir_path.c_str());

	std::lock_guard<std::mutex> lock(_mtx);
	_dir_path = dir_path;
	_files.clear();
	_dir_st = dir_st;
	_state = S_ACTIVATED;
	if (!StartThread()) {
		_state = S_IDLE;
		return false;
	}
	return true;
}

void FilesSuggestor::Suggest(const std::string &filter, std::vector<std::string> &result)
{
	std::string dir_path, name_prefix;

	size_t last_slash = filter.rfind('/');
	if (last_slash != std::string::npos) {
		name_prefix = filter.substr(last_slash + 1);
		if (last_slash > 0) {
			dir_path = filter.substr(0, last_slash);
			if (dir_path[0] != '/') {
				dir_path.insert(0, "./");
			}
		} else {
			dir_path = '/';
		}

	} else {
		name_prefix = filter;
		dir_path = ".";
	}

	struct stat dir_st{};
	if (sdc_stat(dir_path.c_str(), &dir_st) == -1) {
		fprintf(stderr, "FilesSuggestor: error %u stat '%s'\n", errno, _dir_path.c_str());
		return;
	}

	bool need_reenumerate;

	{
		std::unique_lock<std::mutex> lock(_mtx);
		need_reenumerate = (dir_path != _dir_path
			  || dir_st.st_dev != _dir_st.st_dev || dir_st.st_ino != _dir_st.st_ino
			  || memcmp(&dir_st.st_mtim, &_dir_st.st_mtim, sizeof(dir_st.st_mtim)) != 0
			  || memcmp(&dir_st.st_ctim, &_dir_st.st_ctim, sizeof(dir_st.st_ctim)) != 0);
	}

	if (need_reenumerate) {
		EnsureThreadStopped();
		if (Start(dir_path, dir_st)) {
			// dont keep user waiting longer than 1.5 second
			WaitThread(1500);
		}
	}

	std::lock_guard<std::mutex> lock(_mtx);
	for (const auto &file : _files) {
		if (file.size() >= name_prefix.size() && memcmp(file.c_str(), name_prefix.c_str(), name_prefix.size()) == 0) {
			result.push_back(file);
		}
	}
}

void *FilesSuggestor::ThreadProc()
{
	// _dir_path modified only when thread is not active, so its safe to read it here
	DIR *d = sdc_opendir(_dir_path.c_str());
	if (d) {
		for (;;) {
			struct dirent *de = sdc_readdir(d);
			if (!de) {
				break;
			}
			if (de->d_name[0] && strcmp(de->d_name, ".") && strcmp(de->d_name, "..")) {
				std::lock_guard<std::mutex> lock(_mtx);
				_files.emplace_back(de->d_name);
				if (_state == S_STOPPING) {
					break;
				}
			}
		}
		sdc_closedir(d);

	} else {
		fprintf(stderr, "FilesSuggestor: error %u opendir '%s'\n", errno, _dir_path.c_str());
	}

	return nullptr;
}
