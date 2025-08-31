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

template <bool UseVirtualDestructor>
class FileWriteCache {
private:
	static constexpr size_t c_min_cache_size = 32 * 1024 * 1024; // x2 for double buffer
	static constexpr size_t c_max_cache_size = 128 * 1024 * 1024; // x2 for double buffer
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
	std::shared_ptr<bool> extract_access_rights;
	std::shared_ptr<bool> extract_owners_groups;
	std::shared_ptr<bool> extract_attributes;
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
		//fprintf(stderr, "FWC: close_file() [TID: %lu]\n", static_cast<unsigned long>(pthread_self()));
		if (!file.is_open())
			return;

		if (error_state) {
			file.close();
			return;
		}

		RETRY_OR_IGNORE_BEGIN
		file.set_end();

		DWORD attr, posixattr = 0;
		attr = archive->get_attr(current_rec.file_id, &posixattr);
		(void)attr;

		if ((posixattr & S_IFMT) == S_IFLNK) {
			size_t slsize = file.getsymlinksize();
			if (slsize) {
				if (slsize >= PATH_MAX)
					slsize = PATH_MAX - 1;
				char* symlinkaddr = file.getsymlink();
				symlinkaddr[slsize] = 0;
				int res = sdc_symlink(symlinkaddr, StrWide2MB(current_rec.file_path).c_str());
				if (!res) {
				}
			}
		} else {
			if (*extract_access_rights && posixattr) {
				int res = Far::g_fsf.ESetFileMode(current_rec.file_path.c_str(), posixattr,
					*ignore_errors ? SETATTR_RET_SKIPALL : SETATTR_RET_UNKNOWN);
				if (res != SETATTR_RET_OK) {
					FAIL(res);
				}
			}
			if (*extract_owners_groups && current_rec.file_id < archive->m_num_indices) {
				std::wstring &owner = archive->file_list[current_rec.file_id].owner,
							 &group = archive->file_list[current_rec.file_id].group;
				if (owner.size()) {
					int res = Far::g_fsf.ESetFileOwner(current_rec.file_path.c_str(), owner.c_str(),
						*ignore_errors ? SETATTR_RET_SKIPALL : SETATTR_RET_UNKNOWN);
					if (res != SETATTR_RET_OK)
						FAIL(res);
				}
				if (group.size()) {
					int res = Far::g_fsf.ESetFileGroup(current_rec.file_path.c_str(), group.c_str(),
						*ignore_errors ? SETATTR_RET_SKIPALL : SETATTR_RET_UNKNOWN);
					if (res != SETATTR_RET_OK)
						FAIL(res);
				}
			}
			file.set_time_nt(archive->get_ctime(current_rec.file_id),
				archive->get_atime(current_rec.file_id), archive->get_mtime(current_rec.file_id));
		}
		IGNORE_END(*ignore_errors, *error_log, *progress)

		if (error_ignored)
			error_state = true;

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
		std::shared_ptr<bool> ignore_errors, std::shared_ptr<bool> extract_access_rights,
		std::shared_ptr<bool> extract_owners_groups, std::shared_ptr<bool> extract_attributes,
		std::shared_ptr<ErrorLog> error_log, std::shared_ptr<ExtractProgress> progress,
		bool double_buffering = false )
		: archive(archive)
		, ignore_errors(ignore_errors)
		, extract_access_rights(extract_access_rights)
		, extract_owners_groups(extract_owners_groups)
		, extract_attributes(extract_attributes)
		, error_log(error_log)
		, progress(progress)
		, bDouble_buffering(double_buffering)
		, buffer_size(get_max_cache_size())
	{
		progress->set_cache_total(buffer_size);
		_buffer = (unsigned char *)mmap(NULL, buffer_size * 2, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
		if (!_buffer) {
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
	std::shared_ptr<OverwriteAction> overwrite_action;
	std::shared_ptr<bool> ignore_errors;
	std::shared_ptr<ErrorLog> error_log;
	std::shared_ptr<FileWriteCache<UseVirtualDestructor>> cache;
	std::shared_ptr<ExtractProgress> progress;
	std::shared_ptr<std::set<UInt32>> skipped_indices;

public:
	ArchiveExtractor(UInt32 src_dir_index, const std::wstring &dst_dir, std::shared_ptr<Archive<UseVirtualDestructor>> archive,
			std::shared_ptr<OverwriteAction> overwrite_action, std::shared_ptr<bool> ignore_errors,
			std::shared_ptr<ErrorLog> error_log, std::shared_ptr<FileWriteCache<UseVirtualDestructor>> cache,
			std::shared_ptr<ExtractProgress> progress, std::shared_ptr<std::set<UInt32>> skipped_indices)
		: src_dir_index(src_dir_index),
		  dst_dir(dst_dir),
		  archive(archive),
		  overwrite_action(overwrite_action),
		  ignore_errors(ignore_errors),
		  error_log(error_log),
		  cache(cache),
		  progress(progress),
		  skipped_indices(skipped_indices)
	{}

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

		const auto cmode = static_cast<int>(g_options.correct_name_mode);
		file_path = correct_filename(file_info.name, cmode, file_info.is_altstream);
		UInt32 parent_index = file_info.parent;
		while (parent_index != src_dir_index && parent_index != c_root_index) {
			const ArcFileInfo &parent_file_info = archive->file_list[parent_index];
			file_path.insert(0, 1, L'/')
					.insert(0, correct_filename(parent_file_info.name, cmode & ~(0x10 | 0x40), false));
			parent_index = parent_file_info.parent;
		}
		file_path.insert(0, add_trailing_slash(dst_dir));

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

			if (filter) {
				PluginPanelItem filter_data;
				memset(&filter_data, 0, sizeof(PluginPanelItem));

				DWORD attr = 0, posixattr = 0;
				attr = archive.get_attr(file_index, &posixattr);
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
				indices.push_back(file_index);
			}

		});
	}

public:
	PrepareExtract(const FileIndexRange &index_range, const std::wstring &parent_dir, Archive<UseVirtualDestructor> &archive,
			std::list<UInt32> &indices, Far::FileFilter *filter, bool &ignore_errors, ErrorLog &error_log)
		: ProgressMonitor(Far::get_msg(MSG_PROGRESS_CREATE_DIRS), false),
		  archive(archive),
		  indices(indices),
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
	bool &extract_access_rights;
	bool &extract_owners_groups;
	bool &extract_attributes;
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

				if (extract_access_rights && posixattr) {
					int res = Far::g_fsf.ESetFileMode(file_path.c_str(), posixattr, ignore_errors ? SETATTR_RET_SKIPALL : SETATTR_RET_UNKNOWN);
					if (res != SETATTR_RET_OK) FAIL(res);
				}

				if (extract_owners_groups) {
					std::wstring owner, group;

					owner = archive.get_user(file_index);
					if (owner.size()) {
						int res = Far::g_fsf.ESetFileOwner(file_path.c_str(), owner.c_str(), ignore_errors ? SETATTR_RET_SKIPALL : SETATTR_RET_UNKNOWN);
						if (res != SETATTR_RET_OK) FAIL(res);
					}

					group = archive.get_group(file_index);
					if (group.size()) {
						int res = Far::g_fsf.ESetFileGroup(file_path.c_str(), group.c_str(), ignore_errors ? SETATTR_RET_SKIPALL : SETATTR_RET_UNKNOWN);
						if (res != SETATTR_RET_OK) FAIL(res);
					}
					//fprintf(stderr, "[!!!] owner and group set to: = [%ls](%lu) [%ls](%lu)\n", owner.c_str(), owner.size(), group.c_str(), group.size() );
				}

				{
					FILETIME AccessTime = archive.get_atime(file_index),
							 ModifyTime = archive.get_mtime(file_index);

					if (AccessTime.dwHighDateTime || AccessTime.dwLowDateTime ||
						ModifyTime.dwHighDateTime || ModifyTime.dwLowDateTime) {

						if (!AccessTime.dwHighDateTime && !AccessTime.dwLowDateTime)
							AccessTime = crft;

						if (!ModifyTime.dwHighDateTime && !ModifyTime.dwLowDateTime)
							ModifyTime = crft;

						const std::string &mb_name = Wide2MB(file_path.c_str());
						struct stat s{};
						if (sdc_stat(mb_name.c_str(), &s) != 0)
							memset(&s, 0, sizeof(s));

						WINPORT(FileTime_Win32ToUnix)(&AccessTime, &s.st_atim);
						WINPORT(FileTime_Win32ToUnix)(&ModifyTime, &s.st_mtim);

						struct timespec times[2] = {s.st_atim, s.st_mtim};
						if (sdc_utimens(mb_name.c_str(), times) != 0)
							FAIL(errno);
					}
				}

				///File::set_attr(file_path, FILE_ATTRIBUTE_NORMAL);
//				File file(file_path, FILE_WRITE_ATTRIBUTES, FILE_SHARE_READ, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS);
				///File::set_attr_nt(file_path, archive.get_attr(file_index));
//				file.set_time_nt(archive.get_ctime(file_index), archive.get_atime(file_index), archive.get_mtime(file_index));
///				File::set_attr_nt(file_path, archive.get_attr(file_index));
///				File::set_attr_nt(file_path, attr);
//				File::set_attr_posix(file_path, posixattr);
			}
			RETRY_OR_IGNORE_END(ignore_errors, error_log, *this)
		});

	}

public:
	SetDirAttr(const FileIndexRange &index_range, const std::wstring &parent_dir, Archive<UseVirtualDestructor> &archive, Far::FileFilter *filter,
			bool &ignore_errors, bool &extract_access_rights, bool &extract_owners_groups, bool &extract_attributes, ErrorLog &error_log)
		: ProgressMonitor(Far::get_msg(MSG_PROGRESS_SET_ATTR), false),
		  archive(archive),
		  filter(filter),
		  ignore_errors(ignore_errors),
		  extract_access_rights(extract_access_rights),
		  extract_owners_groups(extract_owners_groups),
		  extract_attributes(extract_attributes),
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

template<bool UseVirtualDestructor>
void Archive<UseVirtualDestructor>::extract(UInt32 src_dir_index, const std::vector<UInt32> &src_indices,
		const ExtractOptions &options, std::shared_ptr<ErrorLog> error_log,
		std::vector<UInt32> *extracted_indices)
{
	DisableSleepMode dsm;

	fprintf(stderr, ">>> Archive::extract():\n" );

	const auto ignore_errors = std::make_shared<bool>(options.ignore_errors);
	const auto overwrite_action = std::make_shared<OverwriteAction>(options.overwrite);
	const auto extract_access_rights = std::make_shared<bool>(options.extract_access_rights);
	const auto extract_owners_groups = std::make_shared<bool>(options.extract_owners_groups);
	const auto extract_attributes = std::make_shared<bool>(options.extract_attributes);

	prepare_dst_dir(options.dst_dir);

	std::list<UInt32> file_indices;
	PrepareExtract(FileIndexRange(src_indices.begin(), src_indices.end()), options.dst_dir, *this,
			file_indices, options.filter.get(), *ignore_errors, *error_log);

	std::vector<UInt32> indices;
	indices.reserve(file_indices.size());
	std::copy(file_indices.begin(), file_indices.end(),
			std::back_insert_iterator<std::vector<UInt32>>(indices));
	std::sort(indices.begin(), indices.end());

//	bool bDoubleBuffering = true;

	bool bDoubleBuffering = (options.double_buffering == triTrue);
	if (options.double_buffering == triUndef) {

		struct stat src_stat, dst_stat;
		const std::string src_path_mb = StrWide2MB(get_root()->arc_path);
		const std::string dst_path_mb = StrWide2MB(options.dst_dir);

		if (sdc_stat(src_path_mb.c_str(), &src_stat) == 0 &&
			sdc_stat(dst_path_mb.c_str(), &dst_stat) == 0 ) {

			bDoubleBuffering = (src_stat.st_dev != dst_stat.st_dev);
		}
	}

//	bDoubleBuffering = false;

	const auto progress = std::make_shared<ExtractProgress>(arc_path, bDoubleBuffering);
	const auto cache = std::make_shared<FileWriteCache<UseVirtualDestructor>>(this->shared_from_this(), ignore_errors, 
							extract_access_rights, extract_owners_groups, extract_attributes, error_log, progress, bDoubleBuffering);
	const auto skipped_indices = extracted_indices ? std::make_shared<std::set<UInt32>>() : nullptr;

	ComObject<IArchiveExtractCallback<UseVirtualDestructor>> extractor(new ArchiveExtractor<UseVirtualDestructor>(src_dir_index, options.dst_dir,
			this->shared_from_this(), overwrite_action, ignore_errors, 
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
			*extract_access_rights, *extract_owners_groups, *extract_attributes, *error_log);

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
