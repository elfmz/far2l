#include <unistd.h>
#include <memory>
#include <mutex>
#include <set>
#include <algorithm>
#include <fcntl.h>
#include <string.h>
#include <sys/stat.h>
#include <ftw.h>

#include <utils.h>
#include <os_call.hpp>
#include <ScopeHelpers.h>

#include "libarch_utils.h"
#include "libarch_cmd.h"

class LIBARCH_Modify
{
protected:
	const LibarchCommandOptions &_arc_opts;
	const char *_arc_path;
	std::string _tmp_path;
	bool _done = true;

	virtual void OnStart(LibArchOpenWrite &arc_dst)
	{
	}

	virtual bool OnCopyEntry(const PathParts &parts)
	{
		return true;
	}

public:
	LIBARCH_Modify(const char *arc_path, const LibarchCommandOptions &arc_opts)
		:
		_arc_opts(arc_opts),
		_arc_path(arc_path),
		_tmp_path(arc_path)
	{
		_tmp_path+= StrPrintf(".%u.tmp", getpid());
	}

	virtual ~LIBARCH_Modify()
	{
		if (!_done) {
			unlink(_tmp_path.c_str());
		}
	}

	void Do()
	{
		unlink(_tmp_path.c_str());
		_done = false;
		LibArchOpenRead arc_src(_arc_path, "", _arc_opts.charset.c_str());
		LibArchOpenWrite arc_dst(_tmp_path.c_str(), arc_src.Get(), "", _arc_opts.compression_level);//_arc_opts.charset.c_str());

		OnStart(arc_dst);

		PathParts parts;
		std::string str;

		for (;;) {
			struct archive_entry *entry = arc_src.NextHeader();
			if (!entry) {
				break;
			}

			const char *pathname = LibArch_EntryPathname(entry);
			if (pathname) {
				parts.clear();
				str = pathname;
				parts.Traverse(str);
				if (!OnCopyEntry(parts)) {
					arc_src.SkipData();
					continue;
				}
				archive_entry_set_pathname(entry, str.c_str());
			}

			int r = LibArchCall(archive_write_header, arc_dst.Get(), entry);
			if (r != ARCHIVE_OK && r != ARCHIVE_WARN) {
				throw std::runtime_error(StrPrintf(
					"Error %d (%s) writing header: %s",
						r, archive_error_string(arc_dst.Get()), pathname));
			}

			if (archive_entry_filetype(entry) != AE_IFREG || archive_entry_size(entry) == 0) {
				continue;
			}

			for (__LA_INT64_T offset = 0, size = archive_entry_size(entry), zeroreads = 0; offset < size;) {
				const void *buf = nullptr;
				size_t size_cur = 0;
				__LA_INT64_T offset_cur = offset;
				int r = LibArchCall(archive_read_data_block, arc_src.Get(), &buf, &size_cur, &offset_cur);
				if ((r != ARCHIVE_OK  && r != ARCHIVE_WARN) || (size_cur == 0 && zeroreads > 1000)) {
					throw std::runtime_error(StrPrintf("Error %d (%s) reading at 0x%llx of 0x%llx: %s",
						r, archive_error_string(arc_src.Get()),
						(unsigned long long)offset, (unsigned long long)size,
						LibArch_EntryPathname(entry)));
				}
				if (size_cur == 0) {
					// sometimes libarchive returns zero length blocks, but next time it return non-zero
					// not sure how to handle that right, so roughly giving it 1000 attempts..
					++zeroreads;

				} else if (!arc_dst.WriteData(buf, size_cur)) {
					throw std::runtime_error("write data failed");

				} else {
					zeroreads = 0;
				}
				offset+= size_cur;
			}
		}

		if (rename(_tmp_path.c_str(), _arc_path) == -1) {
			throw std::runtime_error(StrPrintf("Error %d renaming '%s' to '%s'",
				errno, _tmp_path.c_str(), _arc_path));
		}

		_done = true;
	}
};

////////////////////////////////////////////////////////////

class LIBARCH_Delete : public LIBARCH_Modify
{
	std::vector<PathParts > _files_parts;

	virtual bool OnCopyEntry(const PathParts &parts)
	{
		for (const auto &fp : _files_parts) if (parts.size() >= fp.size()) {
			size_t i = 0;
			while (i != fp.size() && (fp[i] == parts[i] || fp[i] == "*")) {
				++i;
			}
			if (i == fp.size()) {
				printf("Deleted: %s\n", parts.Join().c_str());
				return false;
			}
		}

		return LIBARCH_Modify::OnCopyEntry(parts);
	}

public:
	LIBARCH_Delete(const char *arc_path, const LibarchCommandOptions &arc_opts, int files_cnt, char *files[])
		: LIBARCH_Modify(arc_path, arc_opts)
	{
		std::string str;
		for (int i = 0; i < files_cnt; ++i) if (files[i]) {
			_files_parts.emplace_back();
			if (!arc_opts.root_path.empty()) {
				_files_parts.back().Traverse(arc_opts.root_path);
			}
			str = files[i];
			_files_parts.back().Traverse(str);
		}
	}
};

////////////////////////////////////////////////////////////

class LIBARCH_Add
{
	static std::mutex s_ftw_mtx;
	static LIBARCH_Add *s_ftw_caller;

	const LibarchCommandOptions &_arc_opts;
	PathParts _add_paths;
	PathParts _well_added_paths;

	LibArchOpenWrite *_arc_dst = nullptr;


	void AddPath(const char *path)
	{
		char buf[0x10000];

		struct stat s = {};
		if (lstat(path, &s) == -1) {
			throw std::runtime_error("lstat failed");
		}

		std::string entry_path = _arc_opts.root_path;
		if (!entry_path.empty() && entry_path.back() != '/') {
			entry_path+= '/';
		}
		if (*_cmd == 'm' || *_cmd == 'a') {
			const char *slash = strrchr(path, '/');
			entry_path+= slash ? slash + 1 : path;
		} else {
			entry_path+= path;
		}

		LibArchTempEntry entry;

		archive_entry_set_pathname(entry, entry_path.c_str());
		archive_entry_set_size(entry, s.st_size);
		archive_entry_set_perm(entry, s.st_mode & 07777);
		archive_entry_set_ctime(entry, s.st_ctim.tv_sec, s.st_ctim.tv_nsec);
		archive_entry_set_atime(entry, s.st_atim.tv_sec, s.st_atim.tv_nsec);
		archive_entry_set_mtime(entry, s.st_mtim.tv_sec, s.st_mtim.tv_nsec);
		off_t data_len = 0;

		if (S_ISLNK(s.st_mode)) {
			archive_entry_set_filetype(entry, AE_IFLNK);
			ssize_t r = readlink(path, buf, sizeof(buf) - 1);
			if (r < 0) {
				throw std::runtime_error("readlink failed");
			}
			buf[r] = 0;
			archive_entry_set_symlink(entry, buf);
		} else if (S_ISDIR(s.st_mode)) {
			archive_entry_set_filetype(entry, AE_IFDIR);
		} else if (S_ISCHR(s.st_mode)) {
			archive_entry_set_filetype(entry, AE_IFCHR);
		} else if (S_ISBLK(s.st_mode)) {
			archive_entry_set_filetype(entry, AE_IFBLK);
		} else if (S_ISFIFO(s.st_mode)) {
			archive_entry_set_filetype(entry, AE_IFIFO);
		} else if (S_ISSOCK(s.st_mode)) {
			archive_entry_set_filetype(entry, AE_IFSOCK);
		} else {
			data_len = s.st_size;
			archive_entry_set_filetype(entry, AE_IFREG);
			archive_entry_set_size(entry, data_len);
		}

		int r = LibArchCall(archive_write_header, _arc_dst->Get(), entry.Get());
		if (r != ARCHIVE_OK && r != ARCHIVE_WARN) {
			throw std::runtime_error(StrPrintf(
				"write header error %d (%s)",
				r, archive_error_string(_arc_dst->Get())));
		}

		_added_parts.emplace_back();
		_added_parts.back().Traverse(_arc_opts.root_path);
		_added_parts.back().Traverse(path);

		if (data_len) {
			// once file entry header has been added it cannot be dismissed,
			// so throw different exception types to tell that to caller
			// and keep list of illformed parts into _illformed_parts

			// add it now and (if) when reach finish - will remove it
			_illformed_parts.emplace_back(_added_parts.back());

			off_t written = 0;
			FDScope fd(path, O_RDONLY | O_CLOEXEC);
			if (!fd.Valid()) {
				throw std::underflow_error("open file failed");
			}
			while (written < data_len) {
				ssize_t rd = os_call_ssize(read, (int)fd, (void *)buf, sizeof(buf));
				if (rd == 0) {
					throw std::underflow_error("file unexpectedly shrinked");
				}
				if (rd < 0) {
					throw std::underflow_error("read data failed");
				}
				written+= rd;
				if (written > data_len) {
					rd-= (size_t)(written - data_len);
				}
				if (!_arc_dst->WriteData(buf, (size_t)rd)) {
					throw std::underflow_error("write data failed");
				}
				if (written > data_len) {
					throw std::overflow_error("file unexpectedly enlarged");
				}
			}

			_illformed_parts.pop_back();
		}

		_well_added_paths.emplace_back(path);
	}

	void AddPathHandlingErrors(const char *path)
	{
		try {
			printf("Adding: %s\n", path);
			AddPath(path);

		} catch (std::exception &e) {
			fprintf(stderr, "Exception '%s' errno %u for: %s\n",
				e.what(), errno, path);
			_good = false;
		}
	}

	static int sFTWCallback(const char *path, const struct stat *sb, int typeflag)
	{
		s_ftw_caller->AddPathHandlingErrors(path);
		return 0;
	}

	void AddPathWithRecursion(const char *path)
	{
		struct stat s = {};
		if (lstat(path, &s) == 0 && S_ISDIR(s.st_mode)) {
			// thunk to static function cuz ftw
			std::lock_guard<std::mutex> locker(s_ftw_mtx);
			s_ftw_caller = this;
			ftw(path, sFTWCallback, 10);
			s_ftw_caller = nullptr;

		} else {
			AddPathHandlingErrors(path);
		}
	}


protected:
	const char *_cmd;
	std::vector< PathParts > _added_parts, _illformed_parts;
	bool _good;

public:
	LIBARCH_Add(const char *cmd, const LibarchCommandOptions &arc_opts, int files_cnt, char *files[])
		:
		_arc_opts(arc_opts),
		_cmd(cmd),
		_good(true)
	{
		_add_paths.reserve(files_cnt);
		for (int i = 0; i < files_cnt; ++i) if (files[i]) {
			_add_paths.emplace_back(files[i]);
		}
	}

	bool IsGood() const
	{
		return _good;
	}

	void AddInto(LibArchOpenWrite &arc_dst)
	{
		_arc_dst = &arc_dst;
		for (auto add_path : _add_paths) try {
			// remove ending /* that means all files in dir, that is dir itself
			while (add_path.size() >= 2 && add_path.rfind("/*") == add_path.size() - 2) {
				add_path.resize(add_path.size() - 2);
			}

			// remove ending directoty path with slash, otherwise macos ftw report paths with double slashes that
			// consequently causes resulted zip archive entries not enumed correctly
			while (add_path.size() > 1 && add_path.back() == '/') {
				add_path.resize(add_path.size() - 1);
			}

			AddPathWithRecursion(add_path.c_str());

		} catch (std::exception &e) {
			fprintf(stderr, "Exception: '%s' adding '%s'", e.what(), add_path.c_str());
		}

		_arc_dst = nullptr;
		std::sort(_illformed_parts.begin(), _illformed_parts.end());
		std::sort(_added_parts.begin(), _added_parts.end());
	}

	void DoRemoval()
	{
		if (*_cmd == 'm' || *_cmd == 'M') {
			std::sort(_well_added_paths.begin(), _well_added_paths.end());
			for (auto it = _well_added_paths.rbegin(); it != _well_added_paths.rend(); ++it) {
				if (remove(it->c_str()) == 0) {
					printf("Removed: %s\n", it->c_str());
				} else {
					fprintf(stderr, "Error %u removing: %s\n", errno, it->c_str());
				}
			}
		}
	}
};

std::mutex LIBARCH_Add::s_ftw_mtx;
LIBARCH_Add *LIBARCH_Add::s_ftw_caller = nullptr;
////

class LIBARCH_ReplacingAdd : public LIBARCH_Modify, public LIBARCH_Add
{
	virtual void OnStart(LibArchOpenWrite &arc_dst)
	{
		AddInto(arc_dst);
	}

	virtual bool OnCopyEntry(const PathParts &parts)
	{
		if (std::binary_search(_added_parts.begin(), _added_parts.end(), parts)) {
			if (std::binary_search(_illformed_parts.begin(), _illformed_parts.end(), parts)) {
				throw std::runtime_error("tried to replace existing file by illformed one");
			}
			printf("Replaced: %s\n", parts.Join().c_str());
			return false;
		}

		return LIBARCH_Modify::OnCopyEntry(parts);
	}

public:
	LIBARCH_ReplacingAdd(const char *cmd, const char *arc_path, const LibarchCommandOptions &arc_opts, int files_cnt, char *files[])
		:
		LIBARCH_Modify(arc_path, arc_opts),
		LIBARCH_Add(cmd, arc_opts, files_cnt, files)
	{
	}
};

bool LIBARCH_CommandDelete(const char *cmd, const char *arc_path, const LibarchCommandOptions &arc_opts, int files_cnt, char *files[])
{
	try {
		LIBARCH_Delete d(arc_path, arc_opts, files_cnt, files);
		d.Do();
		return true;

	} catch (std::exception &e) {
		fprintf(stderr, "Exception: %s\n", e.what());
	}

	return false;
}

bool LIBARCH_CommandAdd(const char *cmd, const char *arc_path, const LibarchCommandOptions &arc_opts, int files_cnt, char *files[])
{
	try {
		struct stat s{};
		if (lstat(arc_path, &s) == 0) {
			LIBARCH_ReplacingAdd ra(cmd, arc_path, arc_opts, files_cnt, files);
			ra.Do();
			ra.DoRemoval();
			return ra.IsGood();
		}

		LIBARCH_Add a(cmd, arc_opts, files_cnt, files);
		LibArchOpenWrite arc(arc_path, cmd, "", arc_opts.compression_level);//arc_opts.charset.c_str());
		a.AddInto(arc);
		a.DoRemoval();
		return a.IsGood();

	} catch (std::exception &e) {
		fprintf(stderr, "Exception: %s\n", e.what());
	}

	return false;
}
