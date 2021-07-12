#include "headers.hpp"
#include "strmix.hpp"
#include "pathmix.hpp"
#include "FilesSuggestor.hpp"
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <utils.h>
#include <algorithm>

#include "../WinPort/sudo.h"

FilesSuggestor::~FilesSuggestor()
{
	{
		std::lock_guard<std::mutex> lock(_mtx);
		_stopping = true;
	}

	WaitThread();
}

bool FilesSuggestor::StartEnum(const std::string &dir_path, const struct stat &dir_st)
{
	fprintf(stderr, "FilesSuggestor: enum '%s'\n", dir_path.c_str());

	std::lock_guard<std::mutex> lock(_mtx);
	_dir_path = dir_path;
	_files.clear();
	_dir_st = dir_st;
	_stopping = false;
	return StartThread();
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
		std::lock_guard<std::mutex> lock(_mtx);
		need_reenumerate = (dir_path != _dir_path
			  || dir_st.st_dev != _dir_st.st_dev || dir_st.st_ino != _dir_st.st_ino
#ifdef __APPLE__
			  || memcmp(&dir_st.st_mtimespec, &_dir_st.st_mtimespec, sizeof(dir_st.st_mtimespec)) != 0
			  || memcmp(&dir_st.st_ctimespec, &_dir_st.st_ctimespec, sizeof(dir_st.st_ctimespec)) != 0
#else
			  || memcmp(&dir_st.st_mtim, &_dir_st.st_mtim, sizeof(dir_st.st_mtim)) != 0
			  || memcmp(&dir_st.st_ctim, &_dir_st.st_ctim, sizeof(dir_st.st_ctim)) != 0
#endif
			);

		if (need_reenumerate) {
			_stopping = true;
		}
	}

	if (need_reenumerate) {
		WaitThread();
		if (!StartEnum(dir_path, dir_st)) {
			fprintf(stderr, "FilesSuggestor: thread start error %u\n", errno);
			return;
		}

		// dont keep user waiting longer than 1.5 second
		if (!WaitThread(1500)) {
			fprintf(stderr, "FilesSuggestor: timed out on '%s'\n", _dir_path.c_str());
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
	SudoClientRegion scr;
	SudoSilentQueryRegion ssqr;
	// _dir_path modified only when thread is not active, so its safe to read it here
	DIR *d = sdc_opendir(_dir_path.c_str());
	if (d) {
		try {
			for (;;) {
				struct dirent *de = sdc_readdir(d);
				if (!de) {
					break;
				}
				if (de->d_name[0] && strcmp(de->d_name, ".") && strcmp(de->d_name, "..")) {
					std::lock_guard<std::mutex> lock(_mtx);
					_files.emplace_back(de->d_name);
					if (_stopping) {
						break;
					}
				}
			}
		} catch (std::exception &e) {
			fprintf(stderr, "FilesSuggestor: exception '%s'\n", e.what());
		}
		sdc_closedir(d);

	} else {
		fprintf(stderr, "FilesSuggestor: error %u opendir '%s'\n", errno, _dir_path.c_str());
	}

	return nullptr;
}

///////////////////////////////////////////////////////////////////////////////////////////

void MenuFilesSuggestor::Suggest(const wchar_t *filter, VMenu& menu)
{
	if (!filter || !*filter) {
		return;
	}

	FARString str_filter(filter);

	bool odd_quote = false;
	for (size_t i = 0; i < str_filter.GetLength(); i++) {
		if (str_filter.At(i) == L'"') {
			odd_quote = !odd_quote;
		}
	}

	size_t pos = 0;
	if (odd_quote) {
		str_filter.RPos(pos, L'"');

	} else for (pos = str_filter.GetLength() - 1; pos != static_cast<size_t>(-1); pos--) {
		if (str_filter.At(pos) == L'"') {
			pos--;
			while (str_filter.At(pos) != L'"' && pos != static_cast<size_t>(-1)) {
				pos--;
			}
		} else if (str_filter.At(pos) == L' ') {
			pos++;
			break;
		}
	}

	if (pos == static_cast<size_t>(-1)) {
		pos = 0;
	}

	bool start_quote = false;
	if (str_filter.At(pos) == L'"') {
		pos++;
		start_quote = true;
	}

	FARString str_start(str_filter, pos);
	str_filter.LShift(pos);
	Unquote(str_filter);
	if (!str_filter.IsEmpty()) {
		FARString str_filter_expanded;
		apiExpandEnvironmentStrings(str_filter, str_filter_expanded);
		_suggestions.clear();
		FilesSuggestor::Suggest(str_filter_expanded.GetMB(), _suggestions);
		if (!_suggestions.empty()) {
			std::sort(_suggestions.begin(), _suggestions.end());
			if (menu.GetItemCount()) {
				MenuItemEx item{};
				item.Flags = LIF_SEPARATOR;
				menu.AddItem(&item);
			}
			size_t path_part_len = PointToName(str_filter) - str_filter;
			for (const auto &suggestion : _suggestions) {
				str_filter.SetLength(path_part_len);
				FARString str_tmp(str_start + str_filter);
				str_tmp+= suggestion;
				if (start_quote) {
					str_tmp+= L'"';
				}
				menu.AddItem(str_tmp);
			}
		}
	}
}

