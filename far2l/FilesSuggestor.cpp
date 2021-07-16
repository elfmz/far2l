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

	std::string filter_mb;
	Wide2MB(filter, filter_mb);
	std::string orig_filter_mb = filter_mb;

	Environment::Tokens tokens;
	Environment::ExpandAndTokenizeString(filter_mb, tokens);
	if (tokens.empty() || tokens.back().len == 0) {
		return;
	}

	const std::string &last_token = filter_mb.substr(tokens.back().begin, tokens.back().len);

	_suggestions.clear();
	FilesSuggestor::Suggest(last_token, _suggestions);
	if (_suggestions.empty()) {
		return;
	}

	std::sort(_suggestions.begin(), _suggestions.end());

	if (menu.GetItemCount()) {
		MenuItemEx item{};
		item.Flags = LIF_SEPARATOR;
		menu.AddItem(&item);
	}

	std::string orig_prefix = orig_filter_mb.substr(tokens.back().orig_begin, tokens.back().orig_len);

	size_t path_part_len = orig_prefix.rfind(GOOD_SLASH);
	orig_prefix.resize((path_part_len == std::string::npos) ? 0 : path_part_len + 1);

	if (tokens.back().orig_begin) {
		orig_prefix.insert(0, orig_filter_mb.substr(0, tokens.back().orig_begin));
	}

	for (const auto &suggestion : _suggestions) {
		FARString str_tmp(orig_prefix);
		str_tmp+= suggestion;
		if (orig_filter_mb[tokens.back().orig_begin] == '\''
				|| orig_filter_mb[tokens.back().orig_begin] == '"') {
			str_tmp+= (wchar_t)orig_filter_mb[tokens.back().orig_begin];
		}
		menu.AddItem(str_tmp);
	}
}

