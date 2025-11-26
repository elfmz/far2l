#include "headers.hpp"

#include "msg.hpp"
#include "utils.hpp"
#include "sysutils.hpp"
#include "farutils.hpp"
#include "common.hpp"
#include "ui.hpp"
#include "archive.hpp"
#include "options.hpp"
#include "error.hpp"

#include <sys/mman.h>
#include <pwd.h>
#include <grp.h>

ExtractOptions::ExtractOptions()
	: ignore_errors(false),
	  overwrite(oaOverwrite),
	  move_files(triUndef),
	  separate_dir(triFalse),
	  delete_archive(false),
	  disable_delete_archive(false),
	  double_buffering(triUndef),
	  open_dir(triFalse)
{}

struct PendingHardlink {
	uint32_t src_index;
	uint32_t dst_index;
};

struct PendingSymlink {
	std::wstring src_path;
	uint32_t dst_index;
};

using HardlinkIndexMap = std::unordered_map<UInt32, UInt32>;
using PendingHardlinks = std::vector<PendingHardlink>;
using PendingSymlinks = std::vector<PendingSymlink>;
using PendingSPFiles = std::vector<UInt32>;

static std::wstring get_progress_bar_str(unsigned width, unsigned percent1, unsigned percent2)
{
	const wchar_t c_pb_black = 9608;
	const wchar_t c_pb_gray = 9619;
	const wchar_t c_pb_white = 9617;

	unsigned len1 = al_round(static_cast<double>(percent1) / 100 * width);
	if (len1 > width)
		len1 = width;
	unsigned len2 = al_round(static_cast<double>(percent2) / 100 * width);
	if (len2 > width)
		len2 = width;
	if (len2 > len1)
		len2 -= len1;
	else
		len2 = 0;
	unsigned len3 = width - (len1 + len2);
	std::wstring result;
	result.append(len1, c_pb_black);
	result.append(len2, c_pb_gray);
	result.append(len3, c_pb_white);
	return result;
}

class ExtractProgress : public ProgressMonitor
{
private:
	std::wstring arc_path;
	UInt64 extract_completed;
	UInt64 extract_total;
	std::wstring extract_file_path;
	UInt64 cache_stored;
	UInt64 cache_written;
	UInt64 cache_total;
	std::wstring cache_file_path;
	bool bDoubleBuffering;

	void do_update_ui() override
	{
		const unsigned c_width = 60;

		percent_done = calc_percent(extract_completed, extract_total);

		UInt64 extract_speed;
		if (time_elapsed() == 0)
			extract_speed = 0;
		else
			extract_speed = al_round(static_cast<double>(extract_completed)
					/ static_cast<double>(time_elapsed()) * static_cast<double>(ticks_per_sec()));

		if (extract_total && cache_total > extract_total)
			cache_total = extract_total;

		unsigned cache_stored_percent = calc_percent(cache_stored, cache_total);
		unsigned cache_written_percent = calc_percent(cache_written, cache_total);

		std::wostringstream st;
		st << fit_str(arc_path, c_width) << L'\n';
		st << L"\x1\n";
		st << fit_str(extract_file_path, c_width) << L'\n';
		st << std::setw(7) << format_data_size(extract_completed, get_size_suffixes()) << L" / "
		   << format_data_size(extract_total, get_size_suffixes()) << L" @ " << std::setw(9)
		   << format_data_size(extract_speed, get_speed_suffixes()) << L'\n';
		st << Far::get_progress_bar_str(c_width, percent_done, 100) << L'\n';
		st << L"\x1\n";
		st << fit_str(cache_file_path, c_width) << L'\n';
		st << L"(" << format_data_size(cache_stored, get_size_suffixes()) << L" - "
		   << format_data_size(cache_written, get_size_suffixes()) << L") / "
		   << format_data_size(cache_total, get_size_suffixes()) << (bDoubleBuffering ? L"x2" : L"") << L'\n';
		st << get_progress_bar_str(c_width, cache_written_percent, cache_stored_percent) << L'\n';
		progress_text = st.str();
	}

public:
	ExtractProgress(const std::wstring &arc_path, bool bDoubleBuffering = false)
		: ProgressMonitor(Far::get_msg(MSG_PROGRESS_EXTRACT)),
		  arc_path(arc_path),
		  extract_completed(0),
		  extract_total(0),
		  cache_stored(0),
		  cache_written(0),
		  cache_total(0),
		  bDoubleBuffering(bDoubleBuffering)
	{}

	void update_extract_file(const std::wstring &file_path)
	{
		CriticalSectionLock lock(GetSync());
		extract_file_path = file_path;
		update_ui();
	}
	void set_extract_total(UInt64 size)
	{
		CriticalSectionLock lock(GetSync());
		extract_total = size;
	}
	void update_extract_completed(UInt64 size)
	{
		CriticalSectionLock lock(GetSync());
		extract_completed = size;
		update_ui();
	}
	void update_cache_file(const std::wstring &file_path)
	{
		CriticalSectionLock lock(GetSync());
		cache_file_path = file_path;
		update_ui();
	}
	void set_cache_total(UInt64 size)
	{
		CriticalSectionLock lock(GetSync());
		cache_total = size;
	}
	void update_cache_stored(UInt64 size)
	{
		CriticalSectionLock lock(GetSync());
		cache_stored += size;
		update_ui();
	}
	void update_cache_written(UInt64 size)
	{
		CriticalSectionLock lock(GetSync());
		cache_written += size;
		update_ui();
	}
	void reset_cache_stats()
	{
		cache_stored = 0;
		cache_written = 0;
	}
};

template<bool UseVirtualDestructor>
std::wstring Archive<UseVirtualDestructor>::build_extract_path(
	UInt32 file_index,
	const std::wstring &dst_dir,
	UInt32 src_dir_index
) const
{
	const auto &file_info = file_list[file_index];
	const auto cmode = static_cast<int>(g_options.correct_name_mode);
	std::wstring path = correct_filename(file_info.name, cmode, file_info.is_altstream);
	UInt32 parent_index = file_info.parent;

	while (parent_index != src_dir_index && parent_index != c_root_index) {
		const auto &parent_info = file_list[parent_index];
		path.insert(0, 1, L'/')
			.insert(0, correct_filename(parent_info.name, cmode & ~(0x10 | 0x40), false));
		parent_index = parent_info.parent;
	}

	return add_trailing_slash(dst_dir) + path;
}

static int set_file_times(const std::string &path, FILETIME atime_ft, FILETIME mtime_ft)
{
	if (!atime_ft.dwHighDateTime && !atime_ft.dwLowDateTime &&
		!mtime_ft.dwHighDateTime && !mtime_ft.dwLowDateTime) {
		return 0;
	}

	struct stat st;
	if (sdc_stat(path.c_str(), &st) != 0) {
		memset(&st, 0, sizeof(st));
	}

	struct timespec times[2];
	if (atime_ft.dwHighDateTime || atime_ft.dwLowDateTime) {
		WINPORT(FileTime_Win32ToUnix)(&atime_ft, &times[0]);
	} else {
		times[0] = st.st_atim;
	}
	if (mtime_ft.dwHighDateTime || mtime_ft.dwLowDateTime) {
		WINPORT(FileTime_Win32ToUnix)(&mtime_ft, &times[1]);
	} else {
		times[1] = st.st_mtim;
	}

	return sdc_utimens(path.c_str(), times);
}

static int set_symlink_times(const std::string &path, FILETIME atime_ft, FILETIME mtime_ft)
{
	if (!atime_ft.dwHighDateTime && !atime_ft.dwLowDateTime &&
		!mtime_ft.dwHighDateTime && !mtime_ft.dwLowDateTime) {
		return 0;
	}

	struct timeval times[2];
	if (atime_ft.dwHighDateTime || atime_ft.dwLowDateTime) {
		struct timespec ts;
		WINPORT(FileTime_Win32ToUnix)(&atime_ft, &ts);
		times[0].tv_sec = ts.tv_sec;
		times[0].tv_usec = ts.tv_nsec / 1000;
	} else {
		times[0].tv_sec = 0;
		times[0].tv_usec = 0;
	}
	if (mtime_ft.dwHighDateTime || mtime_ft.dwLowDateTime) {
		struct timespec ts;
		WINPORT(FileTime_Win32ToUnix)(&mtime_ft, &ts);
		times[1].tv_sec = ts.tv_sec;
		times[1].tv_usec = ts.tv_nsec / 1000;
	} else {
		times[1].tv_sec = 0;
		times[1].tv_usec = 0;
	}

	return sdc_lutimes(path.c_str(), times);
}

#if 1

static int set_file_owner_group(const std::string &path, const ArcFileInfo &fi, int mode )
{
	if (mode <= 0) return 0;

	uid_t target_uid = static_cast<uid_t>(-1);
	gid_t target_gid = static_cast<gid_t>(-1);
	bool uid_valid = false;
	bool gid_valid = false;

	auto uid_by_name = [&](const std::string &name) -> bool {
		struct passwd *pw = getpwnam(name.c_str());
		if (pw) { target_uid = pw->pw_uid; return true; }
		return false;
	};
	auto gid_by_name = [&](const std::string &name) -> bool {
		struct group *gr = getgrnam(name.c_str());
		if (gr) { target_gid = gr->gr_gid; return true; }
		return false;
	};
	auto uid_exists = [&](uid_t uid) -> bool { return getpwuid(uid) != nullptr; };
	auto gid_exists = [&](gid_t gid) -> bool { return getgrgid(gid) != nullptr; };

	std::string owner_mb = StrWide2MB(fi.owner);
	std::string group_mb = StrWide2MB(fi.group);

	if (mode == 1 || mode == 3) {
		if (!owner_mb.empty()) uid_valid = uid_by_name(owner_mb);
		if (!group_mb.empty()) gid_valid = gid_by_name(group_mb);
	}

	if ((mode == 2 || mode == 3) && (!uid_valid || !gid_valid)) { // ids or ids + names
		if (!uid_valid && fi.fuid != static_cast<uid_t>(-1)) {
			if (mode == 2 || (mode == 3 && uid_exists(fi.fuid))) {
				target_uid = fi.fuid;
				uid_valid = true;
			}
		}
		if (!gid_valid && fi.fgid != static_cast<gid_t>(-1)) {
			if (mode == 2 || (mode == 3 && gid_exists(fi.fgid))) {
				target_gid = fi.fgid;
				gid_valid = true;
			}
		}
	}

	if (uid_valid || gid_valid) {
		uid_t use_uid = uid_valid ? target_uid : static_cast<uid_t>(-1);
		gid_t use_gid = gid_valid ? target_gid : static_cast<gid_t>(-1);
		return sdc_lchown(path.c_str(), use_uid, use_gid);
	}

	return 0;
}

static int set_file_owner_group(const std::wstring &path, const ArcFileInfo &fi, int mode )
{
	std::string mb_path = StrWide2MB(path);
	return set_file_owner_group(mb_path, fi, mode);
}

#endif


template <bool UseVirtualDestructor>
class FileWriteCache {
private:
	static constexpr size_t c_min_cache_size = 32 * 1024 * 1024; // x2 for double buffer
	static constexpr size_t c_max_cache_size = 100 * 1024 * 1024; // x2 for double buffer
	static constexpr size_t c_block_size = 1 * 1024 * 1024; // Write block size

	struct CacheRecord {
		std::wstring file_path;
		UInt32 file_id {};
		OverwriteAction overwrite { OverwriteAction::oaAsk };
		size_t buffer_pos {};
		size_t buffer_size {};
		bool continue_file {};
	};

	std::shared_ptr<Archive<UseVirtualDestructor>> archive;
	std::shared_ptr<bool> ignore_errors;
	const ExtractOptions &options;
	std::shared_ptr<ErrorLog> error_log;
	std::shared_ptr<ExtractProgress> progress;
	bool bDouble_buffering = true;

	unsigned char* _buffer;
	unsigned char* buffer[2];
	size_t buffer_size;
	size_t commit_size {};
	size_t buffer_pos {};
	uint32_t cbi = 0;
	uint32_t wbi = 0;

	std::list<CacheRecord> cache_records[2];
	File file;
	CacheRecord current_rec;
	bool error_state {};

	std::unique_ptr<std::thread> worker_thread;
	mutable std::mutex io_mutex;
	std::condition_variable worker_cv;
	bool worker_data_ready = false;
	bool worker_data_processed = true;
	std::atomic<bool> stop_worker_flag { false };
	std::atomic<bool> worker_started { false };
	std::atomic<bool> finalize_called { false };
	std::atomic<bool> worker_has_unfinished_work { false };

	size_t get_max_cache_size() const
	{
		MEMORYSTATUSEX mem_st { sizeof(mem_st) };
		WINPORT_GlobalMemoryStatusEx(&mem_st);

		auto size = static_cast<size_t>(mem_st.ullAvailPhys);

		if (bDouble_buffering) {
			if (size >= c_max_cache_size * 2 ) {
				return c_max_cache_size;
			}
		}
		else {
			if (size >= c_max_cache_size) {
				return c_max_cache_size;
			}
		}

		return c_min_cache_size;
	}

	void create_file()
	{
		std::wstring file_path;
		if (current_rec.overwrite == oaRename)
			file_path = auto_rename(current_rec.file_path);
		else
			file_path = current_rec.file_path;

		//fprintf(stderr, "FWC: create_file( %ls ) [TID: %lu]\n", file_path.c_str(), static_cast<unsigned long>(pthread_self()));

		if (current_rec.overwrite == oaOverwrite || current_rec.overwrite == oaOverwriteCase
			|| current_rec.overwrite == oaAppend) {
			File::set_attr_nt(file_path, FILE_ATTRIBUTE_NORMAL);
		}

		RETRY_OR_IGNORE_BEGIN
		const DWORD access = FILE_WRITE_DATA | FILE_WRITE_ATTRIBUTES;
		const DWORD shares = FILE_SHARE_READ;
		DWORD attrib = FILE_ATTRIBUTE_TEMPORARY;
//		const ArcFileInfo &fi = archive->file_list[current_rec.file_id];
		DWORD attr, posixattr = 0;
		attr = archive->get_attr(current_rec.file_id, &posixattr);
		(void)attr;
		if ((posixattr & S_IFMT) == S_IFLNK) {
			if (archive->get_size(current_rec.file_id) <= PATH_MAX)
				attrib |= FILE_FLAG_CREATE_REPARSE_POINT;
		}
		bool opened = false;
		if (current_rec.overwrite == oaAppend) {
			opened = file.open_nt(file_path, access, shares, OPEN_EXISTING, attrib);
		} else {
			opened = current_rec.overwrite != oaOverwriteCase
				&& file.open_nt(file_path, access, shares, CREATE_ALWAYS, attrib);
			if (!opened) {
				File::delete_file_nt(file_path);
				opened = file.open_nt(file_path, access, shares, CREATE_ALWAYS, attrib);
			}
		}
		CHECK_FILE(opened, file_path);
		RETRY_OR_IGNORE_END(*ignore_errors, *error_log, *progress)

		if (error_ignored)
			error_state = true;
		progress->update_cache_file(current_rec.file_path);
	}

	void allocate_file()
	{
		//fprintf(stderr, "FWC: allocate_file() [TID: %lu]\n", static_cast<unsigned long>(pthread_self()));
		if (error_state)
			return;
		if (archive->get_size(current_rec.file_id) == 0)
			return;

		UInt64 size;
		if (current_rec.overwrite == oaAppend)
			size = file.size();
		else
			size = 0;

		RETRY_OR_IGNORE_BEGIN
		file.set_pos(size + archive->get_size(current_rec.file_id), FILE_BEGIN);
		file.set_end();
		file.set_pos(size, FILE_BEGIN);
		RETRY_OR_IGNORE_END(*ignore_errors, *error_log, *progress)

		if (error_ignored)
			error_state = true;
	}

	void write_file()
	{
		//fprintf(stderr, "FWC: write_file() START [TID: %lu], size=%zu, pos=%zu\n", static_cast<unsigned long>(pthread_self()),
		//							current_rec.buffer_size, current_rec.buffer_pos);
		if (error_state)
			return;

		//fprintf(stderr, "FWC: write_file() current_rec.buffer_size = %zu\n", current_rec.buffer_size );

		size_t pos = 0;
		while (pos < current_rec.buffer_size) {
			DWORD size = static_cast<DWORD>(std::min(c_block_size, current_rec.buffer_size - pos));
			size_t size_written = 0;

			RETRY_OR_IGNORE_BEGIN
			if (current_rec.buffer_pos + pos + size > buffer_size * 2)
				FAIL(E_FAIL);

			//FAIL(E_FAIL); // TEST FAIL
			//fprintf(stderr, "FWC: file.write pos=%zu current_rec.buffer_size =%zu\n", pos, current_rec.buffer_size );

			size_written = file.write(buffer[wbi] + current_rec.buffer_pos + pos, size);
			RETRY_OR_IGNORE_END(*ignore_errors, *error_log, *progress)

			if (error_ignored) {
				error_state = true;
				return;
			}

			pos += size_written;
			progress->update_cache_written(size_written);
		}

		//fprintf(stderr, "FWC: write_file() EXIT [TID: %lu], size=%zu, pos=%zu\n", static_cast<unsigned long>(pthread_self()),
		//					current_rec.buffer_size, current_rec.buffer_pos);
	}

	void close_file()
	{
		if (!file.is_open())
			return;

		if (error_state) {
			file.close();
			return;
		}

		const ArcFileInfo &fi = archive->file_list[current_rec.file_id];
		std::string mb_path = StrWide2MB(current_rec.file_path);
		DWORD attr, posixattr = 0;
		attr = archive->get_attr(current_rec.file_id, &posixattr);
		(void)attr;

		RETRY_OR_IGNORE_BEGIN
		file.set_end();

		if ((posixattr & S_IFMT) == S_IFLNK) {
			size_t slsize = file.getsymlinkdatasize();

			if (slsize) {
				if (slsize >= PATH_MAX)
					slsize = PATH_MAX - 1;
				char* symlinkdata = file.getsymlink();
				symlinkdata[slsize] = 0;

				std::wstring file_path = current_rec.file_path;
				mb_path = StrWide2MB(current_rec.file_path);
				struct stat st;
				if (sdc_lstat(mb_path.c_str(), &st) == 0) {
					if (current_rec.overwrite == oaRename)
						file_path = auto_rename(current_rec.file_path);
					else if (sdc_unlink(mb_path.c_str()) != 0) {
						FAIL(errno);
					}
				}

				mb_path = StrWide2MB(file_path);

				if (sdc_symlink(symlinkdata, mb_path.c_str()) != 0) {
					FAIL(errno);
				}

				if (options.extract_owners_groups) {
					set_file_owner_group(mb_path, fi, options.extract_owners_groups);
				}

				FILETIME atime_ft = archive->get_atime(current_rec.file_id);
				FILETIME mtime_ft = archive->get_mtime(current_rec.file_id);
				if (set_symlink_times(mb_path, atime_ft, mtime_ft)) {
					FAIL(errno);
				}
			}
		} else {
			mb_path = StrWide2MB(current_rec.file_path);

			if (options.extract_access_rights && posixattr) {
				if (sdc_chmod(mb_path.c_str(), posixattr & 0xFFF) != 0) {
					FAIL(errno);
				}
			}

			if (options.extract_owners_groups) {
				if (set_file_owner_group(mb_path, fi, options.extract_owners_groups)) {
					FAIL(errno);
				}
			}

			FILETIME atime_ft = archive->get_atime(current_rec.file_id);
			FILETIME mtime_ft = archive->get_mtime(current_rec.file_id);
			if (set_file_times(mb_path, atime_ft, mtime_ft)) {
				FAIL(errno);
			}
		}
		RETRY_OR_IGNORE_END(*ignore_errors, *error_log, *progress)
		if (error_ignored) {
			error_state = true;
		}

		file.close();
	}

	void perform_write()
	{
		//fprintf(stderr, "FWC: perform_write() START [TID: %lu]\n", static_cast<unsigned long>(pthread_self()));

		for (const auto &rec : cache_records[wbi]) {
			if (rec.continue_file) {
				current_rec = rec;
			} else {
				close_file();
				error_state = false;
				current_rec = rec;
				create_file();
				allocate_file();
			}
			write_file();
		}

		// leave only last file record in cache

		if (!bDouble_buffering) {
			if (!cache_records[wbi].empty()) {
				current_rec = cache_records[wbi].back();
				current_rec.buffer_pos = 0;
				current_rec.buffer_size = 0;
				current_rec.continue_file = true;
				cache_records[wbi].assign(1, current_rec);
			}
			buffer_pos = 0;
		}

		progress->reset_cache_stats();
	}

	void worker_thread_func()
	{
		SudoRegionGuard sudo_guard;
		//fprintf(stderr, "FWC: WorkerThread Started [TID: %lu]\n", static_cast<unsigned long>(pthread_self()));
		try {

		while (!stop_worker_flag.load()) {
			std::unique_lock<std::mutex> lock(io_mutex);
			//fprintf(stderr, "FWC: WorkerThread Wait return worker_data_ready || stop_worker_flag.load(); [TID: %lu]\n", static_cast<unsigned long>(pthread_self()));
			worker_cv.wait(lock, [this] {
				return worker_data_ready || stop_worker_flag.load();
			});

			if (stop_worker_flag.load()) {
				//fprintf(stderr, "FWC: WorkerThread - Stop flag set, exiting loop [TID: %lu]\n", static_cast<unsigned long>(pthread_self()));
				break;
			}

			//fprintf(stderr, "FWC: WorkerThread worker_data_ready>?? [TID: %lu]\n", static_cast<unsigned long>(pthread_self()));
			if (worker_data_ready) {
				//fprintf(stderr, "FWC: WorkerThread unlock & write [TID: %lu]\n", static_cast<unsigned long>(pthread_self()));
				worker_has_unfinished_work.store(true);
				lock.unlock();
				perform_write();
				{
					//fprintf(stderr, "FWC: WorkerThread worker_data_processed = true; [TID: %lu]\n", static_cast<unsigned long>(pthread_self()));
					std::lock_guard<std::mutex> lock2(io_mutex);
					worker_data_ready = false;
					worker_data_processed = true;
					wbi = 0xFFFFFFFF;
					worker_has_unfinished_work.store(false);
				}
				worker_cv.notify_one();
				//fprintf(stderr, "FWC: WorkerThread Batch Processed [TID: %lu]\n", static_cast<unsigned long>(pthread_self()));
			}
		}

		} catch (const std::exception& e) {
			//fprintf(stderr, "FWC: WorkerThread catch1 [TID: %lu]\n", static_cast<unsigned long>(pthread_self()));
			stop_worker_flag.store(true);
			worker_has_unfinished_work.store(false);
			worker_cv.notify_one();
		} catch (...) {
		//error_state = true;
			//fprintf(stderr, "FWC: WorkerThread catch2 [TID: %lu]\n", static_cast<unsigned long>(pthread_self()));
			stop_worker_flag.store(true);
			worker_has_unfinished_work.store(false);
			worker_cv.notify_one();
		}
		//fprintf(stderr, "FWC: WorkerThread Stopped [TID: %lu]\n", static_cast<unsigned long>(pthread_self()));

		stop_worker_flag.store(true);
		worker_has_unfinished_work.store(false);
	}

	void write()
	{
		//fprintf(stderr, "FWC: write() [TID: %lu] - Initiating flush\n", static_cast<unsigned long>(pthread_self()));
		if (!worker_thread) {
			//fprintf(stderr, "FWC: write() - Creating WorkerThread [TID: %lu]\n", static_cast<unsigned long>(pthread_self()));
			worker_thread = std::make_unique<std::thread>(&FileWriteCache::worker_thread_func, this);
			worker_started.store(true);
		}

		{
			std::unique_lock<std::mutex> lock(io_mutex);
			//fprintf(stderr, "FWC: write() !worker_data_processed ? [TID: %lu]\n", static_cast<unsigned long>(pthread_self()));
			if (!worker_data_processed) { /// wait for writer
				//fprintf(stderr, "FWC: write() wait worker_data_processed || stop_worker_flag.load(); [TID: %lu]\n", static_cast<unsigned long>(pthread_self()));
				worker_cv.wait(lock, [this] {
					return worker_data_processed || stop_worker_flag.load();
				});
			}
			if (stop_worker_flag.load()) {
				//fprintf(stderr, "FWC: write() - Worker stopped unexpectedly before send [TID: %lu]\n", static_cast<unsigned long>(pthread_self()));
				throw E_ABORT;
				return;
			}

			worker_data_ready = true;
			worker_data_processed = false;
			wbi = cbi;

			if (bDouble_buffering) {
				cbi ^= 1;
				// leave only last file record in cache
				if (!cache_records[wbi].empty()) {
					CacheRecord last_rec = cache_records[wbi].back();
					last_rec.buffer_pos = 0;
					last_rec.buffer_size = 0;
					last_rec.continue_file = true;
					cache_records[cbi].assign(1, last_rec);
				}
				buffer_pos = 0;
			}
		}
		worker_cv.notify_one();

		if (!bDouble_buffering) {
			std::unique_lock<std::mutex> lock(io_mutex);
			worker_cv.wait(lock, [this] {
				return worker_data_processed || stop_worker_flag.load();
			});
			if (stop_worker_flag.load()) {
				//fprintf(stderr, "FWC: write() - Worker stopped while waiting for completion [TID: %lu]\n", static_cast<unsigned long>(pthread_self()));
				throw E_ABORT;
			} else {
				//fprintf(stderr, "FWC: write() [TID: %lu] - Flush completed by worker\n", static_cast<unsigned long>(pthread_self()));
			}
		}
	}

	void store(const unsigned char* data, size_t size)
	{
		//fprintf(stderr, "FWC: store() [TID: %lu], size=%zu, buffer_pos=%zu\n", static_cast<unsigned long>(pthread_self()), size, buffer_pos);
		assert(!cache_records[cbi].empty());
		assert(size <= buffer_size);

		if (buffer_pos + size > buffer_size) {
			write();
		}

		CacheRecord &rec = cache_records[cbi].back();
		memcpy(buffer[cbi] + buffer_pos, data, size);
		rec.buffer_size += size;
		buffer_pos += size;
		progress->update_cache_stored(size);
	}

public:
	FileWriteCache(std::shared_ptr<Archive<UseVirtualDestructor>> archive,
		std::shared_ptr<bool> ignore_errors, const ExtractOptions &options,
		std::shared_ptr<ErrorLog> error_log, std::shared_ptr<ExtractProgress> progress,
		bool double_buffering = false )
		: archive(archive),
		ignore_errors(ignore_errors),
		options(options),
		error_log(error_log),
		progress(progress),
		bDouble_buffering(double_buffering),
		buffer_size(get_max_cache_size())
	{
		progress->set_cache_total(buffer_size);
		_buffer = (unsigned char *)mmap(NULL, buffer_size * 2, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
		if (_buffer == MAP_FAILED) {
			FAIL(E_OUTOFMEMORY);
		}
		buffer[0] = _buffer;
		buffer[1] = _buffer + buffer_size;

		worker_thread = nullptr;
	}

	~FileWriteCache()
	{
		if (worker_started.load()) {
			if (!stop_worker_flag.load()) {
				//fprintf(stderr, "FWC:: Destructor - finalize not called or incomplete, stopping worker [TID: %lu]\n", pthread_self());
				stop_worker_flag.store(true);
				worker_cv.notify_all();
			}

			if (worker_thread && worker_thread->joinable()) {
				//fprintf(stderr, "FWC:: Destructor - Joining worker thread [TID: %lu]\n", pthread_self());
				try {
					worker_thread->join();
					//fprintf(stderr, "FWC:: Destructor - Worker thread joined [TID: %lu]\n", pthread_self());
				} catch (...) {
					//fprintf(stderr, "FWC:: Destructor - Exception during worker join [TID: %lu]\n", pthread_self());
				}
			}
		}

		if (_buffer) {
			munmap(_buffer, buffer_size);
		}

		if (file.is_open()) {
			file.close();
			File::delete_file_nt(current_rec.file_path);
		}
	}

	void store_file(const std::wstring& file_path, UInt32 file_id, OverwriteAction overwrite_action)
	{
		//fprintf(stderr, "FWC: store_file( %ls ) [TID: %lu]\n", file_path.c_str(), static_cast<unsigned long>(pthread_self()));
		CacheRecord rec;
		rec.file_path = file_path;
		rec.file_id = file_id;
		rec.overwrite = overwrite_action;
		rec.buffer_pos = buffer_pos;
		rec.buffer_size = 0;
		rec.continue_file = false;
		cache_records[cbi].push_back(rec);
		progress->update_cache_file(file_path);
	}

	void store_data(const unsigned char* data, size_t size)
	{
		//fprintf(stderr, "FWC: store_data() [TID: %lu], size=%zu\n", static_cast<unsigned long>(pthread_self()), size);
		unsigned full_buffer_cnt = static_cast<unsigned>(size / buffer_size);

		for (unsigned i = 0; i < full_buffer_cnt; i++)
			store(data + i * buffer_size, buffer_size);

		store(data + full_buffer_cnt * buffer_size, size % buffer_size);
	}

	bool has_background_work() const {
		return worker_started.load(std::memory_order_acquire) &&
			   worker_has_unfinished_work.load(std::memory_order_acquire);
	}

	void finalize()
	{
		//fprintf(stderr, "FWC::finalize() [TID: %lu]\n", static_cast<unsigned long>(pthread_self()));
		if (finalize_called.exchange(true)) {
			//fprintf(stderr, "FWC::finalize() - Already called, skipping. [TID: %lu]\n", pthread_self());
			return;
		}

		if (worker_started.load()) {
			//fprintf(stderr, "FWC::finalize() - Worker was started, stopping it. [TID: %lu]\n", pthread_self());
			stop_worker_flag.store(true);
			worker_cv.notify_all();

			//fprintf(stderr, "FWC::finalize() - Waiting for worker thread to join... [TID: %lu]\n", pthread_self());
			if (worker_thread && worker_thread->joinable()) {
				try {
					worker_thread->join();
					//fprintf(stderr, "FWC::finalize() - Worker thread joined. [TID: %lu]\n", pthread_self());
				} catch (...) {
					//fprintf(stderr, "FWC::finalize() - Exception during worker join. [TID: %lu]\n", pthread_self());
				}
			} else {
				//fprintf(stderr, "FWC::finalize() - Worker thread was not joinable. [TID: %lu]\n", pthread_self());
			}
		}

		wbi = cbi;

		bool has_remaining_data = !cache_records[wbi].empty()
			&& (cache_records[wbi].size() > 1 || cache_records[wbi].front().buffer_size > 0
				|| (cache_records[wbi].size() == 1 && cache_records[wbi].front().buffer_size > 0));

		if (has_remaining_data) {
			//fprintf(stderr, "FWC::finalize() - Writing remaining data synchronously. [TID: %lu]\n", pthread_self());
			perform_write();
			close_file();
		} else {
			//fprintf(stderr, "FWC::finalize() - No remaining data to write. [TID: %lu]\n", pthread_self());
			close_file();
		}
	}
};

template<bool UseVirtualDestructor>
class CachedFileExtractStream : public ISequentialOutStream<UseVirtualDestructor>, public ComBase<UseVirtualDestructor>
{
private:
	std::shared_ptr<FileWriteCache<UseVirtualDestructor>> cache;

public:
	CachedFileExtractStream(std::shared_ptr<FileWriteCache<UseVirtualDestructor>> cache) : cache(cache) {}

	UNKNOWN_IMPL_BEGIN
	UNKNOWN_IMPL_ITF(ISequentialOutStream)
	UNKNOWN_IMPL_END

	STDMETHODIMP Write(const void *data, UInt32 size, UInt32 *processedSize) noexcept override
	{
		COM_ERROR_HANDLER_BEGIN
		if (processedSize)
			*processedSize = 0;
		cache->store_data(static_cast<const unsigned char *>(data), size);
		if (processedSize)
			*processedSize = size;
		return S_OK;
		COM_ERROR_HANDLER_END
	}
};

template<bool UseVirtualDestructor>
class ArchiveExtractor : public IArchiveExtractCallback<UseVirtualDestructor>,
						 public ICryptoGetTextPassword<UseVirtualDestructor>, public ComBase<UseVirtualDestructor>
{
private:
	std::wstring file_path;
	ArcFileInfo file_info;
	UInt32 src_dir_index;
	std::wstring dst_dir;
	std::shared_ptr<Archive<UseVirtualDestructor>> archive;
	HardlinkIndexMap &hlmap;
	std::shared_ptr<OverwriteAction> overwrite_action;
	std::shared_ptr<bool> ignore_errors;
	std::shared_ptr<ErrorLog> error_log;
	std::shared_ptr<FileWriteCache<UseVirtualDestructor>> cache;
	std::shared_ptr<ExtractProgress> progress;
	std::shared_ptr<std::set<UInt32>> skipped_indices;

public:
	ArchiveExtractor(UInt32 src_dir_index, const std::wstring &dst_dir, std::shared_ptr<Archive<UseVirtualDestructor>> archive,
			HardlinkIndexMap &hlmap, std::shared_ptr<OverwriteAction> overwrite_action, std::shared_ptr<bool> ignore_errors,
			std::shared_ptr<ErrorLog> error_log, std::shared_ptr<FileWriteCache<UseVirtualDestructor>> cache,
			std::shared_ptr<ExtractProgress> progress, std::shared_ptr<std::set<UInt32>> skipped_indices)
		: src_dir_index(src_dir_index),
		  dst_dir(dst_dir),
		  archive(archive),
		  hlmap(hlmap),
		  overwrite_action(overwrite_action),
		  ignore_errors(ignore_errors),
		  error_log(error_log),
		  cache(cache),
		  progress(progress),
		  skipped_indices(skipped_indices)
	{
	}

	UNKNOWN_IMPL_BEGIN
	UNKNOWN_IMPL_ITF(IProgress)
	UNKNOWN_IMPL_ITF(IArchiveExtractCallback)
	UNKNOWN_IMPL_ITF(ICryptoGetTextPassword)
	UNKNOWN_IMPL_END

	STDMETHODIMP SetTotal(UInt64 total) noexcept override
	{
		CriticalSectionLock lock(GetSync());
		COM_ERROR_HANDLER_BEGIN
		progress->set_extract_total(total);
		return S_OK;
		COM_ERROR_HANDLER_END
	}
	STDMETHODIMP SetCompleted(const UInt64 *completeValue) noexcept override
	{
		CriticalSectionLock lock(GetSync());
		COM_ERROR_HANDLER_BEGIN
		if (completeValue)
			progress->update_extract_completed(*completeValue);
		return S_OK;
		COM_ERROR_HANDLER_END
	}

	STDMETHODIMP
	GetStream(UInt32 index, ISequentialOutStream<UseVirtualDestructor> **outStream, Int32 askExtractMode) noexcept override
	{
		COM_ERROR_HANDLER_BEGIN
		*outStream = nullptr;
		file_info = archive->file_list[index];
		if (file_info.is_dir)
			return S_OK;

		if (file_info.hl_group != (uint32_t)-1 && !file_info.is_altstream) {
			auto it_hlmap = hlmap.find(file_info.hl_group);
			if (it_hlmap != hlmap.end()) {
				index = it_hlmap->second;
				file_info = archive->file_list[index];
			}
			else {
			}
		}

		file_path = archive->build_extract_path(index, dst_dir, src_dir_index);

		if (askExtractMode != NArchive::NExtract::NAskMode::kExtract)
			return S_OK;

		FindData dst_file_info;
		OverwriteAction overwrite;
		if (File::get_find_data_nt(file_path, dst_file_info)) {
			if (*overwrite_action == oaAsk) {
				OverwriteFileInfo src_ov_info, dst_ov_info;
				src_ov_info.is_dir = file_info.is_dir;
				src_ov_info.size = archive->get_size(index);
				src_ov_info.mtime = archive->get_mtime(index);
				dst_ov_info.is_dir = dst_file_info.is_dir();
				dst_ov_info.size = dst_file_info.size();
				dst_ov_info.mtime = dst_file_info.ftLastWriteTime;
				ProgressSuspend ps(*progress);
				OverwriteOptions ov_options;
				if (!overwrite_dialog(file_path, src_ov_info, dst_ov_info, odkExtract, ov_options))
					return E_ABORT;
				if (g_options.strict_case && ov_options.action == oaOverwrite) {
					auto dst_len = std::wcslen(dst_file_info.cFileName);
					if (file_path.size() > dst_len
							&& file_path.substr(file_path.size() - dst_len) != dst_file_info.cFileName)
						ov_options.action = oaOverwriteCase;
				}
				overwrite = ov_options.action;
				if (ov_options.all)
					*overwrite_action = ov_options.action;
			} else
				overwrite = *overwrite_action;

			if (overwrite == oaSkip) {
				if (skipped_indices) {
					skipped_indices->insert(index);
					for (UInt32 idx = file_info.parent; idx != c_root_index;
							idx = archive->file_list[idx].parent) {
						skipped_indices->insert(idx);
					}
				}
				return S_OK;
			}
		} else
			overwrite = oaAsk;

		if (archive->get_anti(index)) {

			if (File::exists(file_path))
				File::delete_file(file_path);
			return S_OK;
		}

		progress->update_extract_file(file_path);
		cache->store_file(file_path, index, overwrite);
		ComObject<ISequentialOutStream<UseVirtualDestructor>> out_stream(new CachedFileExtractStream<UseVirtualDestructor>(cache));
		out_stream.detach(outStream);

		return S_OK;
		COM_ERROR_HANDLER_END
	}
	STDMETHODIMP PrepareOperation(Int32 askExtractMode) noexcept override
	{
		CriticalSectionLock lock(GetSync());
		COM_ERROR_HANDLER_BEGIN
		return S_OK;
		COM_ERROR_HANDLER_END
	}
	STDMETHODIMP SetOperationResult(Int32 resultEOperationResult) noexcept override
	{
		CriticalSectionLock lock(GetSync());
		COM_ERROR_HANDLER_BEGIN
		RETRY_OR_IGNORE_BEGIN
		bool encrypted = !archive->m_password.empty();
		Error error;
		switch (resultEOperationResult) {
			case NArchive::NExtract::NOperationResult::kOK:
			case NArchive::NExtract::NOperationResult::kDataAfterEnd:
				return S_OK;
			case NArchive::NExtract::NOperationResult::kUnsupportedMethod:
				error.messages.emplace_back(Far::get_msg(MSG_ERROR_EXTRACT_UNSUPPORTED_METHOD));
				break;
			case NArchive::NExtract::NOperationResult::kDataError:
				archive->m_password.clear();
				error.messages.emplace_back(Far::get_msg(
						encrypted ? MSG_ERROR_EXTRACT_DATA_ERROR_ENCRYPTED : MSG_ERROR_EXTRACT_DATA_ERROR));
				break;
			case NArchive::NExtract::NOperationResult::kCRCError:
				archive->m_password.clear();
				error.messages.emplace_back(Far::get_msg(
						encrypted ? MSG_ERROR_EXTRACT_CRC_ERROR_ENCRYPTED : MSG_ERROR_EXTRACT_CRC_ERROR));
				break;
			case NArchive::NExtract::NOperationResult::kUnavailable:
				error.messages.emplace_back(Far::get_msg(MSG_ERROR_EXTRACT_UNAVAILABLE_DATA));
				break;
			case NArchive::NExtract::NOperationResult::kUnexpectedEnd:
				error.messages.emplace_back(Far::get_msg(MSG_ERROR_EXTRACT_UNEXPECTED_END_DATA));
				break;
			// case NArchive::NExtract::NOperationResult::kDataAfterEnd:
			//   error.messages.emplace_back(Far::get_msg(MSG_ERROR_EXTRACT_DATA_AFTER_END));
			//   break;
			case NArchive::NExtract::NOperationResult::kIsNotArc:
				error.messages.emplace_back(Far::get_msg(MSG_ERROR_EXTRACT_IS_NOT_ARCHIVE));
				break;
			case NArchive::NExtract::NOperationResult::kHeadersError:
				error.messages.emplace_back(Far::get_msg(MSG_ERROR_EXTRACT_HEADERS_ERROR));
				break;
			case NArchive::NExtract::NOperationResult::kWrongPassword:
				error.messages.emplace_back(Far::get_msg(MSG_ERROR_EXTRACT_WRONG_PASSWORD));
				break;
			default:
				error.messages.emplace_back(Far::get_msg(MSG_ERROR_EXTRACT_UNKNOWN));
				break;
		}
		error.code = E_MESSAGE;
		error.messages.emplace_back(file_path);
		error.messages.emplace_back(archive->arc_path);
		throw error;
		IGNORE_END(*ignore_errors, *error_log, *progress)
		COM_ERROR_HANDLER_END
	}

	STDMETHODIMP CryptoGetTextPassword(BSTR *password) noexcept override
	{
		CriticalSectionLock lock(GetSync());
		COM_ERROR_HANDLER_BEGIN
		if (archive->m_password.empty()) {
			ProgressSuspend ps(*progress);
			if (!password_dialog(archive->m_password, archive->arc_path))
				FAIL(E_ABORT);
		}
		BStr(archive->m_password).detach(password);
		return S_OK;
		COM_ERROR_HANDLER_END
	}
};

template<bool UseVirtualDestructor>
void Archive<UseVirtualDestructor>::prepare_dst_dir(const std::wstring &path)
{
	if (!is_root_path(path)) {
		prepare_dst_dir(extract_file_path(path));
		File::create_dir_nt(path);
	}
}

template<bool UseVirtualDestructor>
class PrepareExtract : public ProgressMonitor
{
private:
	Archive<UseVirtualDestructor> &archive;
	std::list<UInt32> &indices;
	HardlinkIndexMap &hlmap;
	PendingHardlinks &phl;
	PendingSymlinks &psl;
	PendingSPFiles &psf;
	Far::FileFilter *filter;
	bool &ignore_errors;
	ErrorLog &error_log;
	const std::wstring *file_path{};

	void do_update_ui() override
	{
		const unsigned c_width = 60;
		std::wostringstream st;
		st << std::left << std::setw(c_width) << fit_str(*file_path, c_width) << L'\n';
		progress_text = st.str();
	}

	void update_progress(const std::wstring &file_path_value)
	{
		CriticalSectionLock lock(GetSync());
		file_path = &file_path_value;
		update_ui();
	}

	void prepare_extract(const FileIndexRange &index_range, const std::wstring &parent_dir)
	{
		//fprintf(stderr, "prepare_extract()\n" );
		const auto cmode = static_cast<int>(g_options.correct_name_mode);
		std::for_each(index_range.first, index_range.second, [&](UInt32 file_index) {
			const ArcFileInfo &file_info = archive.file_list[file_index];
			DWORD attr = 0, posixattr = 0;
			attr = archive.get_attr(file_index, &posixattr);

			if (filter) {
				PluginPanelItem filter_data;
				memset(&filter_data, 0, sizeof(PluginPanelItem));

				DWORD farattr = SetFARAttributes(attr, posixattr);

				if (file_info.name.length()) {
					if (file_info.name[0] == L'.') {
						if (file_info.name.length() == 1) { // skip .
							return;
						}
						farattr |= FILE_ATTRIBUTE_HIDDEN;
					}
				}
				else { // no name ?
					farattr |= FILE_ATTRIBUTE_HIDDEN;
				}

				if (archive.get_encrypted(file_index))
					farattr |= FILE_ATTRIBUTE_ENCRYPTED;

				filter_data.NumberOfLinks = file_info.num_links;
				if (!file_info.num_links)
					farattr |= FILE_ATTRIBUTE_BROKEN;
				else if (file_info.num_links > 1)
					farattr |= FILE_ATTRIBUTE_HARDLINKS;

				filter_data.FindData.dwFileAttributes = attr | farattr;
				filter_data.FindData.dwUnixMode = posixattr;
				filter_data.FindData.nFileSize = archive.get_size(file_index);
				filter_data.FindData.nPhysicalSize = archive.get_psize(file_index);
				filter_data.FindData.ftCreationTime = archive.get_ctime(file_index);
				filter_data.FindData.ftLastAccessTime = archive.get_atime(file_index);
				filter_data.FindData.ftLastWriteTime = archive.get_mtime(file_index);
				filter_data.FindData.lpwszFileName = const_cast<wchar_t *>(file_info.name.c_str());

				filter_data.CRC32 = archive.get_crc(file_index);
				filter_data.Owner = const_cast<wchar_t *>(file_info.owner.c_str());
				filter_data.Group = const_cast<wchar_t *>(file_info.group.c_str());
				filter_data.Description = const_cast<wchar_t *>(file_info.desc.c_str());

				if (!filter->match(filter_data)) {
					return;
				}
			}

			if (file_info.is_dir) {
				std::wstring dir_path = add_trailing_slash(parent_dir) + correct_filename(file_info.name, cmode, file_info.is_altstream);
				update_progress(dir_path);

				RETRY_OR_IGNORE_BEGIN
				try {
					File::create_dir(dir_path);
				} catch (const Error &e) {
					if (e.code != HRESULT_FROM_WIN32(ERROR_ALREADY_EXISTS))
						throw;
				}
				RETRY_OR_IGNORE_END(ignore_errors, error_log, *this)

				FileIndexRange dir_list = archive.get_dir_list(file_index);
				prepare_extract(dir_list, dir_path);
			} else {

				if (!file_info.num_links) // broken hard link
					return;

				std::wstring link_path;
				if (archive.get_symlink(file_index, link_path)) {
					PendingSymlink syml = { link_path, file_index };
					psl.push_back(syml);
					return;
				}

				uint32_t ftype = posixattr & S_IFMT;
				if (ftype == S_IFCHR || ftype == S_IFBLK || ftype == S_IFIFO) {
					psf.push_back(file_index);
					return;
				}

				if (file_info.hl_group == (uint32_t)-1 || file_info.is_altstream) {
					//fprintf(stderr, "ALTSTREAM! %ls group %u, file_index %u;\n", file_info.name.c_str(), file_info.hl_group, file_index);
					indices.push_back(file_index);
					return;
				}

				const HardLinkGroup &group = archive.hard_link_groups[file_info.hl_group];
				if (group.empty()) {
					indices.push_back(file_index);
					return;
				}

				auto it_hlmap = hlmap.find(file_info.hl_group);
				if (it_hlmap == hlmap.end()) {
					hlmap[file_info.hl_group] = file_index;
					indices.push_back(group[0]);
					return;
				} else {
					UInt32 ex_index = it_hlmap->second;
					PendingHardlink hardl = { ex_index, file_index };
					phl.push_back(hardl);
					return;
				}

				indices.push_back(file_index);
			}

		});
	}

public:
	PrepareExtract(const FileIndexRange &index_range, const std::wstring &parent_dir, Archive<UseVirtualDestructor> &archive,
			std::list<UInt32> &indices, HardlinkIndexMap &hlmap, PendingHardlinks &phl, PendingSymlinks &psl, PendingSPFiles &psf, 
			Far::FileFilter *filter, bool &ignore_errors, ErrorLog &error_log)
		: ProgressMonitor(Far::get_msg(MSG_PROGRESS_CREATE_DIRS), false),
		  archive(archive),
		  indices(indices),
		  hlmap(hlmap),
		  phl(phl),
		  psl(psl),
		  psf(psf),
		  filter(filter),
		  ignore_errors(ignore_errors),
		  error_log(error_log)
	{
		if (filter)
			filter->start();

		prepare_extract(index_range, parent_dir);
	}
};

template<bool UseVirtualDestructor>
class SetDirAttr : public ProgressMonitor
{
private:
	Archive<UseVirtualDestructor> &archive;
	Far::FileFilter *filter;
	bool &ignore_errors;
	const ExtractOptions &options;
	ErrorLog &error_log;
	const std::wstring *m_file_path{};
	FILETIME crft;

	void do_update_ui() override
	{
		const unsigned c_width = 60;
		std::wostringstream st;
		st << std::left << std::setw(c_width) << fit_str(*m_file_path, c_width) << L'\n';
		progress_text = st.str();
	}

	void update_progress(const std::wstring &file_path_value)
	{
		CriticalSectionLock lock(GetSync());
		m_file_path = &file_path_value;
		update_ui();
	}

	void set_dir_attr(const FileIndexRange &index_range, const std::wstring &parent_dir)
	{
		const auto cmode = static_cast<int>(g_options.correct_name_mode);

		for_each(index_range.first, index_range.second, [&](UInt32 file_index) {
			const ArcFileInfo &file_info = archive.file_list[file_index];
			std::wstring file_path = add_trailing_slash(parent_dir)
					+ correct_filename(file_info.name, cmode, file_info.is_altstream);
			update_progress(file_path);

			if (!file_info.is_dir)
				return;

			FileIndexRange dir_list = archive.get_dir_list(file_index);
			set_dir_attr(dir_list, file_path);
			RETRY_OR_IGNORE_BEGIN

			if (archive.get_anti(file_index)) {
				if (File::exists(file_path)) {
					File::remove_dir(file_path);
				}

			} else {
				DWORD attr = 0, posixattr = 0;
				attr = archive.get_attr(file_index, &posixattr);

				if (filter) {
					PluginPanelItem filter_data;
					memset(&filter_data, 0, sizeof(PluginPanelItem));
					DWORD farattr = SetFARAttributes(attr, posixattr);

					if (file_info.name.length()) {
						if (file_info.name[0] == L'.') {
							if (file_info.name.length() == 1) { // skip .
								return;
							}
							farattr |= FILE_ATTRIBUTE_HIDDEN;
						}
					}
					else { // no name ?
						farattr |= FILE_ATTRIBUTE_HIDDEN;
					}

					if (archive.get_encrypted(file_index))
						farattr |= FILE_ATTRIBUTE_ENCRYPTED;

					{
						uint32_t n = archive.get_links(file_index);
						filter_data.NumberOfLinks = n;
						if (n > 1)
							attr |= FILE_ATTRIBUTE_HARDLINKS;
					}

					filter_data.FindData.dwFileAttributes = attr | farattr;
					filter_data.FindData.dwUnixMode = posixattr;
					filter_data.FindData.nFileSize = archive.get_size(file_index);
					filter_data.FindData.nPhysicalSize = archive.get_psize(file_index);
					filter_data.FindData.ftCreationTime = archive.get_ctime(file_index);
					filter_data.FindData.ftLastAccessTime = archive.get_atime(file_index);
					filter_data.FindData.ftLastWriteTime = archive.get_mtime(file_index);
					filter_data.FindData.lpwszFileName = const_cast<wchar_t *>(file_info.name.c_str());

					filter_data.CRC32 = archive.get_crc(file_index);
					filter_data.Owner = const_cast<wchar_t *>(file_info.owner.c_str());
					filter_data.Group = const_cast<wchar_t *>(file_info.group.c_str());
					filter_data.Description = const_cast<wchar_t *>(file_info.desc.c_str());

					if (!filter->match(filter_data)) {
						return;
					}
				}

				std::string mb_path = StrWide2MB(file_path);

				if (options.extract_access_rights && posixattr) {
					if (sdc_chmod(mb_path.c_str(), posixattr & 0xFFF) != 0) {
						FAIL(errno);
					}
				}

				if (options.extract_owners_groups) {
					if (set_file_owner_group(mb_path, file_info, options.extract_owners_groups)) {
						FAIL(errno);
					}
				}

				FILETIME atime_ft = archive.get_atime(file_index);
				FILETIME mtime_ft = archive.get_mtime(file_index);
				if (set_symlink_times(mb_path, atime_ft, mtime_ft)) {
					FAIL(errno);
				}
			}

			RETRY_OR_IGNORE_END(ignore_errors, error_log, *this)

		});

	}

public:
	SetDirAttr(const FileIndexRange &index_range, const std::wstring &parent_dir, Archive<UseVirtualDestructor> &archive, Far::FileFilter *filter,
			bool &ignore_errors, const ExtractOptions &options, ErrorLog &error_log)
		: ProgressMonitor(Far::get_msg(MSG_PROGRESS_SET_ATTR), false),
		  archive(archive),
		  filter(filter),
		  ignore_errors(ignore_errors),
		  options(options),
		  error_log(error_log)
	{
		WINPORT(GetSystemTimeAsFileTime)(&crft);

		if (filter)
			filter->start();

		set_dir_attr(index_range, parent_dir);
	}
};

template<bool UseVirtualDestructor>
class SimpleExtractor : public IArchiveExtractCallback<UseVirtualDestructor>,
						public ICryptoGetTextPassword<UseVirtualDestructor>, public ComBase<UseVirtualDestructor>
{
private:
	std::shared_ptr<Archive<UseVirtualDestructor>> archive;
	ComObject<ISequentialOutStream<UseVirtualDestructor>> mem_stream;

public:
	SimpleExtractor(std::shared_ptr<Archive<UseVirtualDestructor>> archive, ISequentialOutStream<UseVirtualDestructor> *stream = nullptr)
	:	archive(archive),
		mem_stream(stream)
	{}

	UNKNOWN_IMPL_BEGIN
	UNKNOWN_IMPL_ITF(IProgress)
	UNKNOWN_IMPL_ITF(IArchiveExtractCallback)
	UNKNOWN_IMPL_ITF(ICryptoGetTextPassword)
	UNKNOWN_IMPL_END

	STDMETHODIMP SetTotal(UInt64 total) noexcept override
	{
//		CriticalSectionLock lock(GetSync());
		COM_ERROR_HANDLER_BEGIN
		return S_OK;
		COM_ERROR_HANDLER_END
	}

	STDMETHODIMP SetCompleted(const UInt64 *completeValue) noexcept override
	{
//		CriticalSectionLock lock(GetSync());
		COM_ERROR_HANDLER_BEGIN
		return S_OK;
		COM_ERROR_HANDLER_END
	}

	STDMETHODIMP
	GetStream(UInt32 index, ISequentialOutStream<UseVirtualDestructor> **outStream, Int32 askExtractMode) noexcept override
	{
		COM_ERROR_HANDLER_BEGIN
		if (mem_stream) {
			mem_stream.detach(outStream);
		}
		return S_OK;
		COM_ERROR_HANDLER_END
	}

	STDMETHODIMP PrepareOperation(Int32 askExtractMode) noexcept override
	{
//		CriticalSectionLock lock(GetSync());
		COM_ERROR_HANDLER_BEGIN
		return S_OK;
		COM_ERROR_HANDLER_END
	}

	STDMETHODIMP SetOperationResult(Int32 resultEOperationResult) noexcept override
	{
		COM_ERROR_HANDLER_BEGIN
		return S_OK;
		COM_ERROR_HANDLER_END
/**
		CriticalSectionLock lock(GetSync());
		COM_ERROR_HANDLER_BEGIN
		RETRY_OR_IGNORE_BEGIN
		bool encrypted = !archive->m_password.empty();
		Error error;
		switch (resultEOperationResult) {
			case NArchive::NExtract::NOperationResult::kOK:
			case NArchive::NExtract::NOperationResult::kDataAfterEnd:
				return S_OK;
			case NArchive::NExtract::NOperationResult::kUnsupportedMethod:
				error.messages.emplace_back(Far::get_msg(MSG_ERROR_EXTRACT_UNSUPPORTED_METHOD));
				break;
			case NArchive::NExtract::NOperationResult::kDataError:
				archive->m_password.clear();
				error.messages.emplace_back(Far::get_msg(
						encrypted ? MSG_ERROR_EXTRACT_DATA_ERROR_ENCRYPTED : MSG_ERROR_EXTRACT_DATA_ERROR));
				break;
			case NArchive::NExtract::NOperationResult::kCRCError:
				archive->m_password.clear();
				error.messages.emplace_back(Far::get_msg(
						encrypted ? MSG_ERROR_EXTRACT_CRC_ERROR_ENCRYPTED : MSG_ERROR_EXTRACT_CRC_ERROR));
				break;
			case NArchive::NExtract::NOperationResult::kUnavailable:
				error.messages.emplace_back(Far::get_msg(MSG_ERROR_EXTRACT_UNAVAILABLE_DATA));
				break;
			case NArchive::NExtract::NOperationResult::kUnexpectedEnd:
				error.messages.emplace_back(Far::get_msg(MSG_ERROR_EXTRACT_UNEXPECTED_END_DATA));
				break;
			// case NArchive::NExtract::NOperationResult::kDataAfterEnd:
			//   error.messages.emplace_back(Far::get_msg(MSG_ERROR_EXTRACT_DATA_AFTER_END));
			//   break;
			case NArchive::NExtract::NOperationResult::kIsNotArc:
				error.messages.emplace_back(Far::get_msg(MSG_ERROR_EXTRACT_IS_NOT_ARCHIVE));
				break;
			case NArchive::NExtract::NOperationResult::kHeadersError:
				error.messages.emplace_back(Far::get_msg(MSG_ERROR_EXTRACT_HEADERS_ERROR));
				break;
			case NArchive::NExtract::NOperationResult::kWrongPassword:
				error.messages.emplace_back(Far::get_msg(MSG_ERROR_EXTRACT_WRONG_PASSWORD));
				break;
			default:
				error.messages.emplace_back(Far::get_msg(MSG_ERROR_EXTRACT_UNKNOWN));
				break;
		}
		error.code = E_MESSAGE;
		error.messages.emplace_back(file_path);
		error.messages.emplace_back(archive->arc_path);
		throw error;
		IGNORE_END(*ignore_errors, *error_log, *progress)
		COM_ERROR_HANDLER_END
**/
	}

	STDMETHODIMP CryptoGetTextPassword(BSTR *password) noexcept override
	{
//		CriticalSectionLock lock(GetSync());
		COM_ERROR_HANDLER_BEGIN
		if (archive->m_password.empty()) {
		//			ProgressSuspend ps(*progress);
		//			if (!password_dialog(archive->m_password, archive->arc_path))
			return S_OK;
		//			FAIL(E_ABORT);
		}
		BStr(archive->m_password).detach(password);
		return S_OK;
		COM_ERROR_HANDLER_END
	}
};

class CreateHardlinksProgress : public ProgressMonitor
{
private:
	std::wstring arc_path;
	UInt64 completed;
	UInt64 total;
	std::wstring current_file;

	void do_update_ui() override
	{
		const unsigned c_width = 60;

		percent_done = calc_percent(completed, total);

		std::wostringstream st;
		st << fit_str(arc_path, c_width) << L'\n';
		st << L"\x1\n";
		st << fit_str(current_file, c_width) << L'\n';
		st << completed << L" / " << total << L" hardlinks\n";
		st << Far::get_progress_bar_str(c_width, percent_done, 100) << L'\n';
		progress_text = st.str();
	}

public:
	CreateHardlinksProgress(const std::wstring &arc_path)
//		: ProgressMonitor(Far::get_msg(MSG_PROGRESS_CREATE_HARDLINKS)),
		: ProgressMonitor(L"Create hardlinks"),
		  arc_path(arc_path),
		  completed(0),
		  total(0)
	{}

	void set_total(UInt64 total_count)
	{
		CriticalSectionLock lock(GetSync());
		total = total_count;
		update_ui();
	}

	void update_current_file(const std::wstring &file_path)
	{
		CriticalSectionLock lock(GetSync());
		current_file = file_path;
		update_ui();
	}

	void update_completed(UInt64 completed_count)
	{
		CriticalSectionLock lock(GetSync());
		completed = completed_count;
		update_ui();
	}
};

static void create_hardlink_copy(const std::string &src_path, 
										 const std::string &dst_path, 
										 off_t total_size,
										 std::shared_ptr<CreateHardlinksProgress> progress)
{
	int src_fd = sdc_open(src_path.c_str(), O_RDONLY);
	if (src_fd == -1) {
		FAIL(errno);
	}

	auto src_guard = std::shared_ptr<void>(nullptr, [src_fd](void*) {
		if (src_fd != -1) sdc_close(src_fd);
	});

	struct stat st;
	if (sdc_fstat(src_fd, &st) != 0) {
		FAIL(errno);
	}

	int dst_fd = sdc_open(dst_path.c_str(), O_WRONLY | O_CREAT | O_TRUNC, st.st_mode & 0777);
	if (dst_fd == -1) {
		FAIL(HRESULT_FROM_WIN32(errno));
	}

	auto dst_guard = std::shared_ptr<void>(nullptr, [dst_fd](void*) {
		if (dst_fd != -1) sdc_close(dst_fd);
	});

	const size_t buf_size = 64 * 1024;
	std::vector<char> buffer(buf_size);

	off_t total_copied = 0;
	ssize_t bytes_read;

	std::wstring original_filename = extract_file_name(StrMB2Wide(src_path));

	while ((bytes_read = sdc_read(src_fd, buffer.data(), buf_size)) > 0) {
		ssize_t bytes_written = sdc_write(dst_fd, buffer.data(), bytes_read);
		if (bytes_written != bytes_read) {
			FAIL(errno);
		}

		total_copied += bytes_written;

		if (total_size > 1024 * 1024) {
			if (total_copied % (512 * 1024) == 0 || total_copied == total_size) {
				unsigned percent = calc_percent(total_copied, total_size);
				std::wstring progress_text = original_filename + 
											L" (" + int_to_str(percent) + L"%)";
				progress->update_current_file(progress_text);
			}
		}
	}

	if (bytes_read == -1) {
		FAIL(errno);
	}

	struct timespec times[2];
	times[0] = st.st_atim; // access time
	times[1] = st.st_mtim; // modification time

	if (sdc_futimens(dst_fd, times) != 0) {
		fprintf(stderr, "Warning: failed to set timestamps for %s: %s\n", dst_path.c_str(), strerror(errno));
	}

	dst_guard.reset();
	src_guard.reset();

	if (sdc_chown(dst_path.c_str(), st.st_uid, st.st_gid) != 0) {
		fprintf(stderr, "Warning: failed to set owner/group for %s: %s\n",
			dst_path.c_str(), strerror(errno));
	}

	if (sdc_chmod(dst_path.c_str(), st.st_mode & 07777) != 0) {
		fprintf(stderr, "Warning: failed to set permissions for %s: %s\n", 
			dst_path.c_str(), strerror(errno));
	}
}

template<bool UseVirtualDestructor>
void create_hardlinks(
	Archive<UseVirtualDestructor>& archive,
	const std::wstring& dst_dir,
	UInt32 src_dir_index,
	const PendingHardlinks &phl,
	const PendingSymlinks &psl,
	const PendingSPFiles &psf,
	const HardlinkIndexMap &hardlink_map,
	const ExtractOptions &options,
	OverwriteAction &overwrite_action,
	bool &ignore_errors,
	ErrorLog &error_log)
{
	fprintf(stderr, "create_hardlinks()\n");

	bool bCreateHardLinks = !options.duplicate_hardlinks;

	size_t total_files = phl.size() + psl.size() + (options.restore_special_files ? psf.size() : 0);

	if (total_files == 0) {
		return;
	}

	auto progress = std::make_shared<CreateHardlinksProgress>(archive.arc_path);
	progress->set_total(total_files);

	UInt64 completed = 0;

	for (const auto &link : phl) {
		try {
			std::wstring src_path = archive.build_extract_path(link.src_index, dst_dir, src_dir_index);
			std::wstring dst_path = archive.build_extract_path(link.dst_index, dst_dir, src_dir_index);
			const ArcFileInfo &file_info = archive.file_list[link.dst_index];

			std::wstring current_file = extract_file_name(dst_path);
			if (bCreateHardLinks) {
				current_file = L"[Hardlink] " + current_file;
			} else {
				current_file = L"[Copy] " + current_file;
			}
			progress->update_current_file(current_file);

			RETRY_OR_IGNORE_BEGIN
			std::string src_mb = StrWide2MB(src_path);
			std::string dst_mb = StrWide2MB(dst_path);
			struct stat st;
			if (sdc_stat(src_mb.c_str(), &st) != 0) {
				FAIL_MSG(L"Source file for hardlink/copy does not exist: " + src_path);
			}

			struct stat dst_st;
			std::wstring file_path = dst_path;
			OverwriteAction overwrite = overwrite_action;
			if (sdc_stat(dst_mb.c_str(), &dst_st) == 0) {

				if (overwrite_action == oaAsk) {
					OverwriteFileInfo src_ov_info, dst_ov_info;

					src_ov_info.is_dir = file_info.is_dir;
					src_ov_info.size = archive.get_size(link.dst_index);
					src_ov_info.mtime = archive.get_mtime(link.dst_index);

					dst_ov_info.is_dir = S_ISDIR(st.st_mode);
					dst_ov_info.size = st.st_size;
					WINPORT(FileTime_UnixToWin32)(st.st_mtim, &dst_ov_info.mtime);

					OverwriteOptions ov_options;
					if (!overwrite_dialog(dst_path, src_ov_info, dst_ov_info, odkExtract, ov_options))
						throw E_ABORT;

					overwrite = ov_options.action;
					if (ov_options.all)
						overwrite_action = ov_options.action;
				} else
					overwrite = overwrite_action;

				if (overwrite == oaSkip)
					break;

				if (overwrite == oaRename) {
					file_path = auto_rename(dst_path);
				}
				else if (sdc_unlink(dst_mb.c_str()) != 0) {
					FAIL(errno);
				}
			}

			std::string filepath_mb = StrWide2MB(file_path);

			if (bCreateHardLinks) {
				if (sdc_link(src_mb.c_str(), filepath_mb.c_str()) != 0) {
					FAIL(errno);
				}
			}
			else {
				create_hardlink_copy(src_mb, filepath_mb, st.st_size, progress);
			}
			RETRY_OR_IGNORE_END(ignore_errors, error_log, *progress)

			completed++;
			progress->update_completed(completed);
		} catch (const Error& error) {
			retry_or_ignore_error(error, ignore_errors, ignore_errors, error_log, *progress, true, true);
			if (!ignore_errors) throw;
		}
	}

	for (const auto &symlink_info : psl) {
		try {
			std::wstring link_path = archive.build_extract_path(symlink_info.dst_index, dst_dir, src_dir_index);
			std::wstring target_path = symlink_info.src_path;
			const ArcFileInfo &file_info = archive.file_list[symlink_info.dst_index];

			std::wstring current_file = L"[Symlink] " + extract_file_name(link_path);
			progress->update_current_file(current_file);

			RETRY_OR_IGNORE_BEGIN
			std::string link_mb = StrWide2MB(link_path);
			std::wstring file_path = link_path;
			std::string target_mb = StrWide2MB(target_path);
			struct stat st;
			OverwriteAction overwrite = overwrite_action;
			if (sdc_lstat(link_mb.c_str(), &st) == 0) {

				if (overwrite_action == oaAsk) {
					OverwriteFileInfo src_ov_info, dst_ov_info;

					src_ov_info.is_dir = file_info.is_dir;
					src_ov_info.size = archive.get_size(symlink_info.dst_index);
					src_ov_info.mtime = archive.get_mtime(symlink_info.dst_index);

					dst_ov_info.is_dir = S_ISDIR(st.st_mode);
					dst_ov_info.size = st.st_size;
					WINPORT(FileTime_UnixToWin32)(st.st_mtim, &dst_ov_info.mtime);

					OverwriteOptions ov_options;
					if (!overwrite_dialog(link_path, src_ov_info, dst_ov_info, odkExtract, ov_options))
						throw E_ABORT;

					overwrite = ov_options.action;
					if (ov_options.all)
						overwrite_action = ov_options.action;
				} else
					overwrite = overwrite_action;

				if (overwrite == oaSkip)
					break;

				if (overwrite == oaRename) {
					file_path = auto_rename(link_path);
				}
				else if (sdc_unlink(link_mb.c_str()) != 0) {
					FAIL(errno);
				}
			}

			std::string filepath_mb = StrWide2MB(file_path);

			if (sdc_symlink(target_mb.c_str(), filepath_mb.c_str()) != 0) {
				FAIL(errno);
			}

			if (options.extract_owners_groups) {
				set_file_owner_group(filepath_mb, file_info, options.extract_owners_groups);
			}

			FILETIME atime_ft = archive.get_atime(symlink_info.dst_index);
			FILETIME mtime_ft = archive.get_mtime(symlink_info.dst_index);
			if (set_symlink_times(filepath_mb, atime_ft, mtime_ft)) {
				FAIL(errno);
			}

			RETRY_OR_IGNORE_END(ignore_errors, error_log, *progress)

			completed++;
			progress->update_completed(completed);
		} catch (const Error& error) {
			retry_or_ignore_error(error, ignore_errors, ignore_errors, error_log, *progress, true, true);
			if (!ignore_errors) throw;
		}
	}

	if (!options.restore_special_files) {
		progress->clean();
		return;
	}

	for (const auto &dst_index : psf) {
		try {
			std::wstring dev_path = archive.build_extract_path(dst_index, dst_dir, src_dir_index);
			const ArcFileInfo &file_info = archive.file_list[dst_index];

			std::wstring current_file = L"[Device] " + extract_file_name(dev_path);
			progress->update_current_file(current_file);

			DWORD attr = 0, posixattr = 0;
			attr = archive.get_attr(dst_index, &posixattr);
			(void)attr;
			uint32_t ftype = posixattr & S_IFMT;
			dev_t _device;

			if (ftype == S_IFCHR || ftype == S_IFBLK) {
				if (!archive.get_device(dst_index, _device)) {
					completed++; progress->update_completed(completed);
					continue;
				}
			}
			else if (ftype != S_IFIFO) {
				completed++; progress->update_completed(completed);
				continue;
			}

			RETRY_OR_IGNORE_BEGIN
			std::string dev_mb = StrWide2MB(dev_path);
			std::wstring file_path = dev_path;
			struct stat st;
			OverwriteAction overwrite = overwrite_action;
			if (sdc_lstat(dev_mb.c_str(), &st) == 0) {

				if (overwrite_action == oaAsk) {
					OverwriteFileInfo src_ov_info, dst_ov_info;

					src_ov_info.is_dir = file_info.is_dir;
					src_ov_info.size = archive.get_size(dst_index);
					src_ov_info.mtime = archive.get_mtime(dst_index);

					dst_ov_info.is_dir = S_ISDIR(st.st_mode);
					dst_ov_info.size = st.st_size;
					WINPORT(FileTime_UnixToWin32)(st.st_mtim, &dst_ov_info.mtime);

					OverwriteOptions ov_options;
					if (!overwrite_dialog(dev_path, src_ov_info, dst_ov_info, odkExtract, ov_options))
						throw E_ABORT;

					overwrite = ov_options.action;
					if (ov_options.all)
						overwrite_action = ov_options.action;
				} else
					overwrite = overwrite_action;

				if (overwrite == oaSkip)
					break;

				if (overwrite == oaRename) {
					file_path = auto_rename(dev_path);
				}
				else if (sdc_unlink(dev_mb.c_str()) != 0) {
					FAIL(errno);
				}
			}

			std::string filepath_mb = StrWide2MB(file_path);

			switch(ftype) {
			case S_IFCHR:
			case S_IFBLK:
				if (sdc_mknod(filepath_mb.c_str(), posixattr, _device) != 0) {
					FAIL(errno);
				}
			break;
			case S_IFIFO:
				if (sdc_mkfifo(filepath_mb.c_str(), posixattr) != 0) {
					FAIL(errno);
				}
			break;

			default:
			break;
			}

			if (options.extract_owners_groups) {
				set_file_owner_group(filepath_mb, file_info, options.extract_owners_groups);
			}

			FILETIME atime_ft = archive.get_atime(dst_index);
			FILETIME mtime_ft = archive.get_mtime(dst_index);
			if (set_file_times(filepath_mb, atime_ft, mtime_ft)) {
				FAIL(errno);
			}

			RETRY_OR_IGNORE_END(ignore_errors, error_log, *progress)

			completed++;
			progress->update_completed(completed);
		} catch (const Error& error) {
			retry_or_ignore_error(error, ignore_errors, ignore_errors, error_log, *progress, true, true);
			if (!ignore_errors) throw;
		}
	}

	progress->clean();
}

template<bool UseVirtualDestructor>
void Archive<UseVirtualDestructor>::extract(UInt32 src_dir_index, const std::vector<UInt32> &src_indices,
		const ExtractOptions &options, std::shared_ptr<ErrorLog> error_log,
		std::vector<UInt32> *extracted_indices)
{
	DisableSleepMode dsm;
	HardlinkIndexMap hlmap;
	PendingHardlinks phl;
	PendingSymlinks psl;
	PendingSPFiles psf;

	fprintf(stderr, ">>> Archive::extract():\n" );

	const auto ignore_errors = std::make_shared<bool>(options.ignore_errors);
	const auto overwrite_action = std::make_shared<OverwriteAction>(options.overwrite);

	prepare_dst_dir(options.dst_dir);

	std::list<UInt32> file_indices;
	PrepareExtract(FileIndexRange(src_indices.begin(), src_indices.end()), options.dst_dir, *this,
			file_indices, hlmap, phl, psl, psf, options.filter.get(), *ignore_errors, *error_log);

	std::vector<UInt32> indices;
	indices.reserve(file_indices.size());
	std::copy(file_indices.begin(), file_indices.end(),
			std::back_insert_iterator<std::vector<UInt32>>(indices));
	std::sort(indices.begin(), indices.end());

	bool bDoubleBuffering = (options.double_buffering == triTrue);
	if (options.double_buffering == triUndef) {

		struct stat src_stat, dst_stat;
		const std::string &src_path_mb = StrWide2MB(get_root()->arc_path);
		const std::string &dst_path_mb = StrWide2MB(options.dst_dir);

		if (sdc_stat(src_path_mb.c_str(), &src_stat) == 0 &&
			sdc_stat(dst_path_mb.c_str(), &dst_stat) == 0 ) {

			bDoubleBuffering = (src_stat.st_dev != dst_stat.st_dev);
		}
	}

	const auto progress = std::make_shared<ExtractProgress>(arc_path, bDoubleBuffering);
	const auto cache = std::make_shared<FileWriteCache<UseVirtualDestructor>>(this->shared_from_this(), ignore_errors, 
							options, error_log, progress, bDoubleBuffering);
	const auto skipped_indices = extracted_indices ? std::make_shared<std::set<UInt32>>() : nullptr;

	ComObject<IArchiveExtractCallback<UseVirtualDestructor>> extractor(new ArchiveExtractor<UseVirtualDestructor>(src_dir_index, options.dst_dir,
			this->shared_from_this(), hlmap, overwrite_action, ignore_errors, 
			error_log, cache, progress,
			skipped_indices));

	UInt64 bFullSizeStream = 0;
	if (ex_stream) {
		ex_stream->Seek(0, STREAM_CTL_GETFULLSIZE, &bFullSizeStream);
	}

	if (ex_stream && !bFullSizeStream) {
		ex_stream->Seek(0, STREAM_CTL_RESET, nullptr);
		UInt32 indices2[2] = { 0, 0 };
		ComObject<IArchiveExtractCallback<UseVirtualDestructor>> extractor2(new SimpleExtractor<UseVirtualDestructor>(parent, ex_out_stream));

		std::promise<int> promise1;
		std::future<int> future1 = promise1.get_future();
		std::promise<int> promise2;
		std::future<int> future2 = promise2.get_future();

		std::thread ex_thread1([&]() {
			int errc = parent->in_arc->Extract(indices2, 1, 0, extractor2);
			ex_stream->Seek(0, STREAM_CTL_FINISH, nullptr);
			promise1.set_value(errc);
		});

		std::thread ex_thread2([this, &indices, &extractor, &promise2]() {
			int errc = in_arc->Extract(indices.data(), static_cast<UInt32>(indices.size()), 0, extractor);
			ex_stream->Seek(0, STREAM_CTL_FINISH, nullptr);
			promise2.set_value(errc);
		});

		bool thread1_done = false;
		bool thread2_done = false;
		bool cache_finished = false;

		while (!thread1_done || !thread2_done || !cache_finished) {
			Far::g_fsf.DispatchInterThreadCalls();

			if (!thread1_done && future1.wait_for(std::chrono::seconds(0)) == std::future_status::ready) {
				thread1_done = true;
			}

			if (!thread2_done && future2.wait_for(std::chrono::seconds(0)) == std::future_status::ready) {
				thread2_done = true;
			}

			if (thread2_done) {
				if (cache) {
					cache_finished = !cache->has_background_work();
				}
				else {
					cache_finished = true;
				}
			}

			std::this_thread::sleep_for(std::chrono::milliseconds(10));
		}

		ex_thread1.join();
		ex_thread2.join();
		int errc1 = future1.get();
		int errc2 = future2.get();

//		COM_ERROR_CHECK(errc1);
		COM_ERROR_CHECK(errc2);

		(void)errc1;
	}
	else {
		std::promise<int> promise2;
		std::future<int> future2 = promise2.get_future();
		std::thread ex_thread2([this, &indices, &extractor, &promise2]() {
			int errc = in_arc->Extract(indices.data(), static_cast<UInt32>(indices.size()), 0, extractor);
			promise2.set_value(errc);
		});

		bool thread2_done = false;
		bool cache_finished = false;

		while (!thread2_done || !cache_finished) {
			Far::g_fsf.DispatchInterThreadCalls();

			if (!thread2_done && future2.wait_for(std::chrono::seconds(0)) == std::future_status::ready) {
				thread2_done = true;
			}

			if (thread2_done) {
				if (cache) {
					cache_finished = !cache->has_background_work();
				}
				else {
					cache_finished = true;
				}
			}

			std::this_thread::sleep_for(std::chrono::milliseconds(10));
		}

		ex_thread2.join();
		int errc2 = future2.get();
		COM_ERROR_CHECK(errc2);
//		COM_ERROR_CHECK(in_arc->Extract(indices.data(), static_cast<UInt32>(indices.size()), 0, extractor));
	}

	cache->finalize();
	progress->clean();

	SetDirAttr(FileIndexRange(src_indices.begin(), src_indices.end()), options.dst_dir, *this, options.filter.get(), *ignore_errors,
			options, *error_log);

	if (!phl.empty() || !psl.empty() || !psf.empty()) {
		::create_hardlinks(*this, options.dst_dir, src_dir_index, phl, psl, psf, hlmap, options, *overwrite_action, *ignore_errors, *error_log);
	}

	if (extracted_indices) {
		std::vector<UInt32> sorted_src_indices(src_indices);
		std::sort(sorted_src_indices.begin(), sorted_src_indices.end());
		extracted_indices->clear();
		extracted_indices->reserve(src_indices.size());
		std::set_difference(sorted_src_indices.begin(), sorted_src_indices.end(), skipped_indices->begin(),
				skipped_indices->end(), back_inserter(*extracted_indices));
		extracted_indices->shrink_to_fit();
	}
}

template<bool UseVirtualDestructor>
void Archive<UseVirtualDestructor>::delete_archive()
{
	File::delete_file_nt(arc_path);
	std::for_each(volume_names.begin(), volume_names.end(), [&](const std::wstring &volume_name) {
		File::delete_file_nt(add_trailing_slash(arc_dir()) + volume_name);
	});
}

template class Archive<true>;
template class Archive<false>;
