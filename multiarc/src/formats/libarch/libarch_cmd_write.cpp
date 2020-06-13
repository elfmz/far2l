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

struct AddFileCtx {
	AddFileCtx(LibArchOpenWrite &arc_) : arc(arc_) {}

	LibArchOpenWrite &arc;
	bool with_path = true;
	bool out = false;
	std::unique_ptr<std::set<std::string>> rm_set;
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
	if (!s_addfile_ctx->with_path) {
		const char *slash = strrchr(fpath, '/');
		archive_entry_set_pathname(entry, slash ? slash + 1 : fpath);
	} else {
		archive_entry_set_pathname(entry, fpath);
	}
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
	if (r != ARCHIVE_OK) {
		fprintf(stderr, "Error %d writing header: %s", r, fpath);
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

static bool LIBARCH_CommandHandlerOnFixedPath(LibArchOpenWrite &arc, const char *cmd, const char *wanted_path)
{
	std::lock_guard<std::mutex> locker(s_addfile_mtx);
	s_addfile_ctx.reset(new AddFileCtx(arc) ) ;

	std::unique_ptr<std::set<std::string> > rm_set;
	if (*cmd == 'm' || *cmd == 'M') {
		s_addfile_ctx->rm_set.reset(new std::set<std::string>);
	}

	s_addfile_ctx->with_path = (*cmd == 'A' || *cmd == 'M');

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

bool LIBARCH_CommandHandler(LibArchOpenWrite &arc, const char *cmd, const char *wanted_path = nullptr)
{
	if (!wanted_path || !*wanted_path) {
		throw std::runtime_error("no files to add specified");
	}

	std::string wanted_path_fixed = wanted_path;
	while (wanted_path_fixed.size() >= 2 && wanted_path_fixed.rfind("/*") == wanted_path_fixed.size() - 2) {
		wanted_path_fixed.resize(wanted_path_fixed.size() - 2);
	}

	return LIBARCH_CommandHandlerOnFixedPath(arc, cmd, wanted_path_fixed.c_str());
}
