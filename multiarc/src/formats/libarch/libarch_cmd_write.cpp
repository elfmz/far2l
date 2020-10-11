#include <unistd.h>
#include <memory>
#include <mutex>
#include <set>
#include <fcntl.h>
#include <string.h>
#include <ftw.h>

#include <utils.h>
#include <os_call.hpp>
#include <ScopeHelpers.h>

#include "libarch_utils.h"
#include "libarch_cmd.h"

struct AddFileCtx {
	AddFileCtx(LibArchOpenWrite &arc_) : arc(arc_) {}

	LibArchOpenWrite &arc;
	bool with_path = true;
	bool out = false;
	std::unique_ptr<std::set<std::string>> rm_set;
	std::string arc_root_path;
};

static std::unique_ptr<AddFileCtx> s_addfile_ctx;
static std::mutex s_addfile_mtx;

static bool LIBARCH_CommandAddFile(const char *fpath)
{
	char buf[0x10000];
	struct stat s = {};
	if (lstat(fpath, &s) == -1) {
		fprintf(stderr, "error %d on stat '%s'\n", errno, fpath);
		return false;
	}

	struct archive_entry *entry = archive_entry_new();
	if (!entry) {
		return false;
	}

	bool out = true;
	std::string entry_fpath = s_addfile_ctx->arc_root_path;
	if (!entry_fpath.empty() && entry_fpath.back() != '/') {
		entry_fpath+= '/';
	}
	if (!s_addfile_ctx->with_path) {
		const char *slash = strrchr(fpath, '/');
		entry_fpath+= slash ? slash + 1 : fpath;
	} else {
		entry_fpath+= fpath;
	}

	archive_entry_set_pathname(entry, entry_fpath.c_str());
	archive_entry_set_size(entry, s.st_size);
	archive_entry_set_perm(entry, s.st_mode & 07777);

	off_t data_len = 0;

	if (S_ISLNK(s.st_mode)) {
		archive_entry_set_filetype(entry, AE_IFLNK);
		ssize_t r = readlink(fpath, buf, sizeof(buf) - 1);
		if (r >= 0) {
			buf[r] = 0;
			archive_entry_set_symlink(entry, buf);
		} else {
			fprintf(stderr, "readlink error %d: %s", errno, fpath);
			archive_entry_set_symlink(entry, "???");
			out = false;
		}

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

	int r = LibArchCall(archive_write_header, s_addfile_ctx->arc.Get(), entry);
	if (r != ARCHIVE_OK && r != ARCHIVE_WARN) {
		fprintf(stderr, "Error %d (%s) writing header: %s",
			r, archive_error_string(s_addfile_ctx->arc.Get()), fpath);
		archive_entry_free(entry);
		return false;
	}

	if (data_len) {
		off_t written = 0;
		FDScope fd(open(fpath, O_RDONLY));
		if (fd.Valid()) {
			while (written < data_len) {
				ssize_t rd = os_call_ssize(read, (int)fd, (void *)buf, sizeof(buf));
				if (rd <= 0) {
					break;
				}
				written+= rd;
				if (written > data_len) {
					fprintf(stderr, "Too much data (%lld): %s",
						(long long)data_len, fpath);
					rd-= (size_t)(written - data_len);
					written = data_len;
					out = false;
				}
				if (!s_addfile_ctx->arc.WriteData(buf, (size_t)rd)) {
					throw std::runtime_error("write data failed");
				}
			}
		}
		if (written < data_len) {
			fprintf(stderr, "Too little data (%lld of %lld): %s",
				(long long)written, (long long)data_len, fpath);
			memset(buf, 0, sizeof(buf));
			do {
				size_t piece = (size_t) ((data_len - written < (off_t)sizeof(buf))
					? (size_t)(data_len - written) : sizeof(buf));
				if (!s_addfile_ctx->arc.WriteData(buf, piece)) {
					throw std::runtime_error("write zeroes failed");
				}
				written+= piece;
			} while (written < data_len);
			out = false;
		}
	}

	archive_entry_free(entry);

	if (out && s_addfile_ctx->rm_set) {
		s_addfile_ctx->rm_set->emplace(fpath);
	}

	return out;
}

static int LIBARCH_CommandHandlerFTW(const char *fpath, const struct stat *sb, int typeflag)
{
	try {
		if (!LIBARCH_CommandAddFile(fpath)) {
			s_addfile_ctx->out = false;
		}

	} catch (std::exception &e) {
		fprintf(stderr, "LIBARCH_CommandHandlerFTW('%s'): %s\n", fpath, e.what());
		s_addfile_ctx->out = false;
	}

	return 0;
}

static bool LIBARCH_CommandInsertFileFixedPath(const char *cmd, LibArchOpenWrite &arc, const LibarchCommandOptions &arc_opts, const char *wanted_path)
{
	std::lock_guard<std::mutex> locker(s_addfile_mtx);
	s_addfile_ctx.reset(new AddFileCtx(arc) ) ;

	std::unique_ptr<std::set<std::string> > rm_set;
	if (*cmd == 'm' || *cmd == 'M') {
		s_addfile_ctx->rm_set.reset(new std::set<std::string>);
	}

	s_addfile_ctx->with_path = (*cmd == 'A' || *cmd == 'M');
	s_addfile_ctx->arc_root_path = arc_opts.root_path.c_str();

	s_addfile_ctx->out = LIBARCH_CommandAddFile(wanted_path);

	struct stat s = {};
	if (stat(wanted_path, &s) == 0 && S_ISDIR(s.st_mode)) {
		ftw(wanted_path, LIBARCH_CommandHandlerFTW, 10);
	}

	if (s_addfile_ctx->rm_set) {
		for (auto it = s_addfile_ctx->rm_set->rbegin(); it != s_addfile_ctx->rm_set->rend(); ++it) {
			if (remove(it->c_str()) == -1) {
				fprintf(stderr, "RM error %u for '%s'\n", errno, it->c_str());
			}
		}
	}

	return s_addfile_ctx->out;
}

static bool LIBARCH_CommandInsertFile(const char *cmd, LibArchOpenWrite &arc, const LibarchCommandOptions &arc_opts, const char *wanted_path)
{
	if (!wanted_path || !*wanted_path) {
		throw std::runtime_error("no files to add specified");
	}

	std::string wanted_path_fixed = wanted_path;
	while (wanted_path_fixed.size() >= 2 && wanted_path_fixed.rfind("/*") == wanted_path_fixed.size() - 2) {
		wanted_path_fixed.resize(wanted_path_fixed.size() - 2);
	}

	return LIBARCH_CommandInsertFileFixedPath(cmd, arc, arc_opts, wanted_path_fixed.c_str());
}

///////////////////////////////////////////////////////////////////////////////////////////////////////

static bool LIBARCH_CommandRemoveOrReplace(const char *cmd, const char *arc_path, const LibarchCommandOptions &arc_opts, int files_cnt, char *files[])
{
	std::string tmp_path = arc_path;
	tmp_path+= ".tmp";

	std::string str;
	std::vector<std::vector<std::string> > files_parts;
	for (int i = 0; i < files_cnt; ++i) if (files[i]) {
		files_parts.emplace_back();
		if (!arc_opts.root_path.empty()) {
			LibArch_ParsePathToParts(files_parts.back(), arc_opts.root_path);
		}
		str = files[i];
		LibArch_ParsePathToParts(files_parts.back(), str);
	}

	std::vector<std::string> parts;

	try {
		LibArchOpenRead arc_src(arc_path, "", arc_opts.charset.c_str());
		LibArchOpenWrite arc_dst(tmp_path.c_str(), arc_src.Get(), "");//arc_opts.charset.c_str());
		for (;;) {
			struct archive_entry *entry = arc_src.NextHeader();
			if (!entry) {
				break;
			}
			const char *pathname = LibArch_EntryPathname(entry);
			if (pathname) {
				parts.clear();
				str = pathname;
				LibArch_ParsePathToParts(parts, str);
				bool matches = false;
				for (const auto &fp : files_parts) {
					size_t i = 0;
					while (i != fp.size() && i != parts.size() && (fp[i] == parts[i] || fp[i] == "*")) {
						++i;
					}
					if (i == fp.size()) {
						matches = true;
						break;
					}
				}
				if (matches) {
					arc_src.SkipData();
					printf("Deleted: %s\n", pathname);
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

			for (__LA_INT64_T offset = 0, sz = archive_entry_size(entry); offset < sz;) {
				const void *buf = nullptr;
				size_t size = 0;
				int r = LibArchCall(archive_read_data_block, arc_src.Get(), &buf, &size, &offset);
				if ((r != ARCHIVE_OK  && r != ARCHIVE_WARN) || size == 0) {
					throw std::runtime_error(StrPrintf("Error %d (%s) reading at 0x%llx : %s",
						r, archive_error_string(arc_src.Get()),
						(unsigned long long)offset, LibArch_EntryPathname(entry)));
				}
				if (!arc_dst.WriteData(buf, size)) {
					throw std::runtime_error("write data failed");
				}
				offset+= size;
			}
		}

		if (*cmd != 'd' && *cmd != 'D') for (int i = 0; i < files_cnt; ++i) if (files[i]) {
			if (!LIBARCH_CommandInsertFile(cmd, arc_dst, arc_opts, files[i])) {
				throw std::runtime_error(StrPrintf("Failed to add: %s", files[i]));
			}
		}

	} catch (std::exception &e) {
		remove(tmp_path.c_str());
		fprintf(stderr, "Exception: %s\n", e.what());
		return false;
	}

	if (rename(tmp_path.c_str(), arc_path) == -1) {
		fprintf(stderr, "Rename error %u: %s -> %s\n", errno, tmp_path.c_str(), arc_path);
		remove(tmp_path.c_str());
		return false;
	}

	return true;
}

bool LIBARCH_CommandDelete(const char *cmd, const char *arc_path, const LibarchCommandOptions &arc_opts, int files_cnt, char *files[])
{
	return LIBARCH_CommandRemoveOrReplace(cmd, arc_path, arc_opts, files_cnt, files);
}

bool LIBARCH_CommandAdd(const char *cmd, const char *arc_path, const LibarchCommandOptions &arc_opts, int files_cnt, char *files[])
{
	struct stat s{};
	if (lstat(arc_path, &s) == 0) {
		return LIBARCH_CommandRemoveOrReplace(cmd, arc_path, arc_opts, files_cnt, files);
	}

	LibArchOpenWrite arc(arc_path, cmd, "");//arc_opts.charset.c_str());
	bool out = true;
	for (int i = 0; i < files_cnt; ++i) {
		if (!LIBARCH_CommandInsertFile(cmd, arc, arc_opts, files[i])) {
			out = false;
		}
	}

	return out;
}
