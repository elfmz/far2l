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

bool FilesSuggestor::Suggestion::operator < (const Suggestion &another) const
{
#if 0 // dirs on top
	if (dir != another.dir) {
		return dir > another.dir;
	}
#endif

	return name < another.name;
}

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
	_suggestions.clear();
	_dir_st = dir_st;
	_stopping = false;
	return StartThread();
}

void FilesSuggestor::Suggest(const std::string &filter, std::vector<Suggestion> &result)
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
	for (const auto &suggestion : _suggestions) {
		if (suggestion.name.size() >= name_prefix.size()
				&& memcmp(suggestion.name.c_str(), name_prefix.c_str(), name_prefix.size()) == 0) {
			result.push_back(suggestion);
		}
	}
}

void *FilesSuggestor::ThreadProc()
{
	SudoClientRegion scr;
	SudoSilentQueryRegion ssqr;
	struct stat s;
	// _dir_path modified only when thread is not active, so its safe to read it here
	DIR *d = sdc_opendir(_dir_path.c_str());
	if (d) {
		try {
			std::string stat_path = _dir_path;
			if (!stat_path.empty() && stat_path.back() != GOOD_SLASH) {
				stat_path+= '/';
			}
			size_t stat_path_len = stat_path.size();
			for (;;) {
				struct dirent *de = sdc_readdir(d);
				if (!de) {
					break;
				}
				if (de->d_name[0] && strcmp(de->d_name, ".") && strcmp(de->d_name, "..")) {
					bool dir = false;
					switch (de->d_type) {
						case DT_DIR:
							dir = true;
							break;

						case DT_BLK:
						case DT_FIFO:
						case DT_CHR:
						case DT_SOCK:
						case DT_REG:
							dir = false;
							break;

						default:
							stat_path.resize(stat_path_len);
							stat_path+= de->d_name;
							dir = (sdc_stat(stat_path.c_str(), &s) == 0 && S_ISDIR(s.st_mode));
					}

					std::lock_guard<std::mutex> lock(_mtx);
					_suggestions.emplace_back(Suggestion{de->d_name, dir});
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

	Environment::Arguments args;
	Environment::ParseCommandLine(filter_mb, args, false);
	if (args.empty() || args.back().len == 0) {
		return;
	}

	const auto &last_arg = args.back();
	const std::string &last_arg_str = filter_mb.substr(last_arg.begin, last_arg.len);

	_suggestions.clear();
	FilesSuggestor::Suggest(last_arg_str, _suggestions);
	if (_suggestions.empty()) {
		return;
	}

	std::sort(_suggestions.begin(), _suggestions.end());

	if (menu.GetItemCount()) {
		MenuItemEx item{};
		item.Flags = LIF_SEPARATOR;
		menu.AddItem(&item);
	}

	std::string orig_prefix = orig_filter_mb.substr(last_arg.orig_begin, last_arg.orig_len);

	size_t path_part_len = orig_prefix.rfind(GOOD_SLASH);

	//fprintf(stderr, "!!! orig_prefix='%s'\n", orig_prefix.c_str());

	if (path_part_len == std::string::npos) {
		path_part_len = 0;
	} else {
		path_part_len++;
	}

	if (orig_prefix.size() > path_part_len + 1
			&& orig_prefix[path_part_len] == '$' && orig_prefix[path_part_len + 1] == '\'') {
		path_part_len+= 2;

	} else if (orig_prefix.size() > path_part_len
			&& (orig_prefix[path_part_len] == '"' || orig_prefix[path_part_len] == '\'')) {
		path_part_len+= 1;
	}

	orig_prefix.resize(path_part_len);

	if (last_arg.orig_begin) {
		orig_prefix.insert(0, orig_filter_mb.substr(0, last_arg.orig_begin));
	}

	for (auto &suggestion : _suggestions) {
		FARString str_tmp(orig_prefix);
		if (last_arg.quot == Environment::QUOT_DOUBLE) {
			str_tmp+= EscapeCmdStr(suggestion.name);
		} else if (last_arg.quot == Environment::QUOT_NONE) {
			str_tmp+= EscapeCmdStr(suggestion.name, "\\\"$*?' ");
		} else if (last_arg.quot == Environment::QUOT_DOLLAR_SINGLE) {
			str_tmp+= EscapeLikeInC(suggestion.name);
		} else {
			str_tmp+= suggestion.name;
		}
//		if (suggestion.dir) {
//			str_tmp+= GOOD_SLASH;
//		}
		if (last_arg.quot == Environment::QUOT_DOUBLE) {
			str_tmp+= L'"';
		} else if (last_arg.quot == Environment::QUOT_SINGLE
				|| last_arg.quot == Environment::QUOT_DOLLAR_SINGLE) {
			str_tmp+= L'\'';
		}
		menu.AddItem(str_tmp);
	}
}

