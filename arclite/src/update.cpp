#include "headers.hpp"

#include "msg.hpp"
#include "utils.hpp"
#include "sysutils.hpp"
#include "farutils.hpp"
#include "common.hpp"
#include "ui.hpp"
#include "archive.hpp"
#include "options.hpp"
#include "sfx.hpp"

#if defined(__FreeBSD__) || defined(__DragonFly__) || defined(__APPLE__)
#include <sys/types.h>
#else
#include <sys/sysmacros.h>	  // major / minor
#endif

#include <sys/stat.h>
#include <pwd.h>
#include <grp.h>

static std::wstring format_time(UInt64 t)
{
	UInt64 s = t % 60;
	UInt64 m = (t / 60) % 60;
	UInt64 h = t / 60 / 60;
	std::wostringstream st;
	st << std::setfill(L'0') << std::setw(2) << h << L":" << std::setw(2) << m << L":" << std::setw(2) << s;
	return st.str();
}

class ArchiveUpdateProgress : public ProgressMonitor
{
private:
	UInt64 total;
	UInt64 completed;
	std::wstring arc_path;
	std::wstring file_path;
	UInt64 file_total;
	UInt64 file_completed;
	UInt64 total_data_read;
	UInt64 total_data_written;
	bool silent = false;

	void do_update_ui() override
	{
		const unsigned c_width = 60;

		percent_done = calc_percent(completed, total);

		UInt64 time = time_elapsed();
		UInt64 speed;
		if (time == 0)
			speed = 0;
		else
			speed = al_round(static_cast<double>(completed) / static_cast<double>(time)
					* static_cast<double>(ticks_per_sec()));

		UInt64 total_time;
		if (completed)
			total_time = static_cast<UInt64>(
					static_cast<double>(total) / static_cast<double>(completed) * static_cast<double>(time));
		else
			total_time = 0;
		if (total_time < time)
			total_time = time;

		std::wostringstream st;
		st << fit_str(arc_path, c_width) << L'\n';
		st << L"\x1\n";
		st << fit_str(file_path, c_width) << L'\n';
		st << std::setw(7) << format_data_size(file_completed, get_size_suffixes()) << L" / "
		   << format_data_size(file_total, get_size_suffixes()) << L'\n';
		st << Far::get_progress_bar_str(c_width, calc_percent(file_completed, file_total), 100) << L'\n';
		st << L"\x1\n";
		st << std::setw(7) << format_data_size(completed, get_size_suffixes()) << L" / "
		   << format_data_size(total, get_size_suffixes()) << L" @ " << std::setw(9)
		   << format_data_size(speed, get_speed_suffixes()) << L" -"
		   << format_time((total_time - time) / ticks_per_sec()) << L'\n';
		st << std::setw(7) << format_data_size(total_data_read, get_size_suffixes()) << L" \x2192 "
		   << std::setw(7) << format_data_size(total_data_written, get_size_suffixes()) << L" = "
		   << std::setw(2) << calc_percent(total_data_written, total_data_read) << L"%" << L'\n';
		st << Far::get_progress_bar_str(c_width, percent_done, 100) << L'\n';
		progress_text = st.str();
	}

public:
	ArchiveUpdateProgress(bool new_arc, const std::wstring &arcpath, bool silent = false)
		: ProgressMonitor(Far::get_msg(new_arc ? MSG_PROGRESS_CREATE : MSG_PROGRESS_UPDATE)),
		  total(0),
		  completed(0),
		  arc_path(arcpath),
		  file_total(0),
		  file_completed(0),
		  total_data_read(0),
		  total_data_written(0),
		  silent(silent)
	{}

	void on_open_file(const std::wstring &file_path_value, UInt64 size)
	{
		if (silent) return;
		CriticalSectionLock lock(GetSync());
		file_path = file_path_value;
		file_total = size;
		file_completed = 0;
		update_ui();
	}

	void on_read_file(unsigned size)
	{
		if (silent) return;
		CriticalSectionLock lock(GetSync());
		file_completed += size;
		total_data_read += size;
		update_ui();
	}

	void on_write_archive(unsigned size)
	{
		if (silent) return;
		CriticalSectionLock lock(GetSync());
		total_data_written += size;
		update_ui();
	}

	void on_total_update(UInt64 total_value)
	{
		if (silent) return;
		CriticalSectionLock lock(GetSync());
		total = total_value;
		update_ui();
	}

	void on_completed_update(UInt64 completed_value)
	{
		if (silent) return;
		CriticalSectionLock lock(GetSync());
		completed = completed_value;
		update_ui();
	}
};

DWORD translate_seek_method(UInt32 seek_origin)
{
	DWORD method;
	switch (seek_origin) {
		case STREAM_SEEK_SET:
			method = FILE_BEGIN;
			break;
		case STREAM_SEEK_CUR:
			method = FILE_CURRENT;
			break;
		case STREAM_SEEK_END:
			method = FILE_END;
			break;
		default:
			FAIL(E_INVALIDARG);
	}
	return method;
}

template<bool UseVirtualDestructor>
class UpdateStream : public IOutStream<UseVirtualDestructor>
{
protected:
	std::shared_ptr<ArchiveUpdateProgress> progress;

public:
	UpdateStream(std::shared_ptr<ArchiveUpdateProgress> progress) : progress(progress) {}
	virtual ~UpdateStream() {}
	virtual void clean_files() noexcept = 0;
};

template<bool UseVirtualDestructor>
class SimpleUpdateStream : public UpdateStream<UseVirtualDestructor>, public ComBase<UseVirtualDestructor>, private File
{
public:
	SimpleUpdateStream(const std::wstring &file_path, std::shared_ptr<ArchiveUpdateProgress> progress)
		: UpdateStream<UseVirtualDestructor>(progress)
	{
		RETRY_OR_IGNORE_BEGIN
		open(file_path, GENERIC_WRITE, FILE_SHARE_READ, CREATE_ALWAYS, 0);
		RETRY_END(*this->progress)
	}

	UNKNOWN_IMPL_BEGIN
	UNKNOWN_IMPL_ITF(ISequentialOutStream)
	UNKNOWN_IMPL_ITF(IOutStream)
	UNKNOWN_IMPL_END

	STDMETHODIMP Write(const void *data, UInt32 size, UInt32 *processedSize) noexcept override
	{
		COM_ERROR_HANDLER_BEGIN
		if (processedSize)
			*processedSize = 0;
		unsigned size_written;
		RETRY_OR_IGNORE_BEGIN
		size_written = static_cast<unsigned>(write(data, size));
		RETRY_END(*this->progress)
		this->progress->on_write_archive(size_written);

		if (processedSize)
			*processedSize = size_written;
		return S_OK;
		COM_ERROR_HANDLER_END
	}

	STDMETHODIMP Seek(Int64 offset, UInt32 seekOrigin, UInt64 *newPosition) noexcept override
	{
		COM_ERROR_HANDLER_BEGIN
		if (newPosition)
			*newPosition = 0;
		UInt64 new_position = set_pos(offset, translate_seek_method(seekOrigin));
		if (newPosition)
			*newPosition = new_position;

		return S_OK;
		COM_ERROR_HANDLER_END
	}

	STDMETHODIMP SetSize(UInt64 newSize) noexcept override
	{
		COM_ERROR_HANDLER_BEGIN
		RETRY_OR_IGNORE_BEGIN
		set_pos(newSize, FILE_BEGIN);
		set_end();
		RETRY_END(*this->progress)

		return S_OK;
		COM_ERROR_HANDLER_END
	}

	virtual void clean_files() noexcept override
	{
		close();
		File::delete_file_nt(m_file_path);
	}

	using File::copy_ctime_from;
};

template<bool UseVirtualDestructor>
class SfxUpdateStream : public UpdateStream<UseVirtualDestructor>, public ComBase<UseVirtualDestructor>, private File
{
private:
	UInt64 start_offset;

public:
	SfxUpdateStream(const std::wstring &file_path, const SfxOptions &sfx_options,
			std::shared_ptr<ArchiveUpdateProgress> progress)
		: UpdateStream<UseVirtualDestructor>(progress)
	{
		RETRY_OR_IGNORE_BEGIN
		try {
			create_sfx_module(file_path, sfx_options);
			open(file_path, FILE_WRITE_DATA, FILE_SHARE_READ, OPEN_EXISTING, 0);
			start_offset = set_pos(0, FILE_END);
		} catch (...) {
			File::delete_file_nt(file_path);
			throw;
		}
		RETRY_END(*this->progress)
	}

	UNKNOWN_IMPL_BEGIN
	UNKNOWN_IMPL_ITF(ISequentialOutStream)
	UNKNOWN_IMPL_ITF(IOutStream)
	UNKNOWN_IMPL_END

	STDMETHODIMP Write(const void *data, UInt32 size, UInt32 *processedSize) noexcept override
	{
		COM_ERROR_HANDLER_BEGIN
		if (processedSize)
			*processedSize = 0;
		unsigned size_written;
		RETRY_OR_IGNORE_BEGIN
		size_written = static_cast<unsigned>(write(data, size));
		RETRY_END(*this->progress)
		this->progress->on_write_archive(size_written);
		if (processedSize)
			*processedSize = size_written;
		return S_OK;
		COM_ERROR_HANDLER_END
	}

	STDMETHODIMP Seek(Int64 offset, UInt32 seekOrigin, UInt64 *newPosition) noexcept override
	{
		COM_ERROR_HANDLER_BEGIN
		if (newPosition)
			*newPosition = 0;
		Int64 real_offset = offset;
		if (seekOrigin == STREAM_SEEK_SET)
			real_offset += start_offset;
		UInt64 new_position = set_pos(real_offset, translate_seek_method(seekOrigin));
		if (new_position < start_offset)
			FAIL(E_INVALIDARG);
		new_position -= start_offset;
		if (newPosition)
			*newPosition = new_position;
		return S_OK;
		COM_ERROR_HANDLER_END
	}

	STDMETHODIMP SetSize(UInt64 newSize) noexcept override
	{
		COM_ERROR_HANDLER_BEGIN
		RETRY_OR_IGNORE_BEGIN
		set_pos(newSize + start_offset);
		set_end();
		RETRY_END(*this->progress)
		return S_OK;
		COM_ERROR_HANDLER_END
	}

	void clean_files() noexcept override
	{
		close();
		File::delete_file_nt(m_file_path);
	}
};

template<bool UseVirtualDestructor>
class MultiVolumeUpdateStream : public UpdateStream<UseVirtualDestructor>, public ComBase<UseVirtualDestructor>
{
private:
	std::wstring file_path;
	UInt64 volume_size;

	UInt64 stream_pos;
	UInt64 seek_stream_pos;
	UInt64 stream_size;
	bool next_volume;
	File volume;

	std::wstring get_volume_path(UInt64 volume_idx)
	{
		std::wstring volume_ext = uint_to_str(volume_idx + 1);
		if (volume_ext.size() < 3)
			volume_ext.insert(0, 3 - volume_ext.size(), L'0');
		volume_ext.insert(0, 1, L'.');

		size_t pos = file_path.find_last_of(L'.');
		if (pos != std::wstring::npos && pos != 0) {
			std::wstring ext = file_path.substr(pos);
			if (StrCmpI(ext.c_str(), c_volume_ext) == 0)
				return file_path.substr(0, pos) + volume_ext;
		}
		return file_path + volume_ext;
	}

	UInt64 get_last_volume_idx() { return stream_size ? (stream_size - 1) / volume_size : 0; }

public:
	MultiVolumeUpdateStream(const std::wstring &file_path, UInt64 volume_size,
			std::shared_ptr<ArchiveUpdateProgress> progress)
		: UpdateStream<UseVirtualDestructor>(progress),
		  file_path(file_path),
		  volume_size(volume_size),
		  stream_pos(0),
		  seek_stream_pos(0),
		  stream_size(0),
		  next_volume(false)
	{
		RETRY_OR_IGNORE_BEGIN
		volume.open(get_volume_path(0), GENERIC_WRITE, FILE_SHARE_READ, CREATE_ALWAYS, 0);
		RETRY_END(*this->progress)
	}

	UNKNOWN_IMPL_BEGIN
	UNKNOWN_IMPL_ITF(ISequentialOutStream)
	UNKNOWN_IMPL_ITF(IOutStream)
	UNKNOWN_IMPL_END

	STDMETHODIMP Write(const void *data, UInt32 size, UInt32 *processedSize) noexcept override
	{
		SudoRegionGuard sudo_guard;
		COM_ERROR_HANDLER_BEGIN
		if (processedSize)
			*processedSize = 0;
		if (seek_stream_pos != stream_pos) {
			UInt64 volume_idx = seek_stream_pos / volume_size;
			UInt64 last_volume_idx = get_last_volume_idx();
			while (last_volume_idx + 1 < volume_idx) {
				last_volume_idx += 1;
				RETRY_OR_IGNORE_BEGIN
				volume.open(get_volume_path(last_volume_idx), GENERIC_WRITE, FILE_SHARE_READ, CREATE_ALWAYS,
						0);
				volume.set_pos(volume_size);
				volume.set_end();
				RETRY_END(*this->progress)
			}
			if (last_volume_idx < volume_idx) {
				last_volume_idx += 1;
				assert(last_volume_idx == volume_idx);
				RETRY_OR_IGNORE_BEGIN
				volume.open(get_volume_path(last_volume_idx), GENERIC_WRITE, FILE_SHARE_READ, CREATE_ALWAYS,
						0);
				RETRY_END(*this->progress)
			} else {
				RETRY_OR_IGNORE_BEGIN
				volume.open(get_volume_path(volume_idx), GENERIC_WRITE, FILE_SHARE_READ, OPEN_EXISTING, 0);
				RETRY_END(*this->progress)
			}
			volume.set_pos(seek_stream_pos - volume_idx * volume_size);
			stream_pos = seek_stream_pos;
			next_volume = false;
		}

		unsigned data_off = 0;
		do {
			UInt64 volume_idx = stream_pos / volume_size;

			if (next_volume) {	  // advance to next volume
				if (volume_idx > get_last_volume_idx()) {
					RETRY_OR_IGNORE_BEGIN
					volume.open(get_volume_path(volume_idx), GENERIC_WRITE, FILE_SHARE_READ, CREATE_ALWAYS,
							0);
					RETRY_END(*this->progress)
				} else {
					RETRY_OR_IGNORE_BEGIN
					volume.open(get_volume_path(volume_idx), GENERIC_WRITE, FILE_SHARE_READ, OPEN_EXISTING,
							0);
					RETRY_END(*this->progress)
				}
				next_volume = false;
			}

			UInt64 volume_upper_bound = (volume_idx + 1) * volume_size;
			unsigned write_size;
			if (stream_pos + (size - data_off) >= volume_upper_bound) {
				write_size = static_cast<unsigned>(volume_upper_bound - stream_pos);
				next_volume = true;
			} else
				write_size = size - data_off;
			RETRY_OR_IGNORE_BEGIN
			write_size = static_cast<unsigned>(
					volume.write(reinterpret_cast<const unsigned char *>(data) + data_off, write_size));
			RETRY_END(*this->progress)
			CHECK(write_size != 0);
			data_off += write_size;
			stream_pos += write_size;
			seek_stream_pos = stream_pos;
			if (stream_size < stream_pos)
				stream_size = stream_pos;
		} while (data_off < size);
		this->progress->on_write_archive(size);
		if (processedSize)
			*processedSize = size;
		return S_OK;
		COM_ERROR_HANDLER_END
	}

	STDMETHODIMP Seek(Int64 offset, UInt32 seekOrigin, UInt64 *newPosition) noexcept override
	{
		COM_ERROR_HANDLER_BEGIN
		if (newPosition)
			*newPosition = 0;
		switch (seekOrigin) {
			case STREAM_SEEK_SET:
				seek_stream_pos = offset;
				break;
			case STREAM_SEEK_CUR:
				seek_stream_pos += offset;
				break;
			case STREAM_SEEK_END:
				if (offset < 0 && static_cast<unsigned>(-offset) > stream_size)
					FAIL(E_INVALIDARG);
				seek_stream_pos = stream_size + offset;
				break;
			default:
				FAIL(E_INVALIDARG);
		}
		if (newPosition)
			*newPosition = seek_stream_pos;
		return S_OK;
		COM_ERROR_HANDLER_END
	}

	STDMETHODIMP SetSize(UInt64 newSize) noexcept override
	{
		SudoRegionGuard sudo_guard;
		COM_ERROR_HANDLER_BEGIN
		if (stream_size == newSize)
			return S_OK;

		UInt64 last_volume_idx = get_last_volume_idx();
		UInt64 volume_idx = static_cast<unsigned>(newSize / volume_size);
		while (last_volume_idx + 1 < volume_idx) {
			last_volume_idx += 1;
			RETRY_OR_IGNORE_BEGIN
			volume.open(get_volume_path(last_volume_idx), GENERIC_WRITE, FILE_SHARE_READ, CREATE_ALWAYS, 0);
			volume.set_pos(volume_size);
			volume.set_end();
			RETRY_END(*this->progress)
		}
		RETRY_OR_IGNORE_BEGIN
		if (last_volume_idx < volume_idx) {
			last_volume_idx += 1;
			assert(last_volume_idx == volume_idx);
			volume.open(get_volume_path(last_volume_idx), GENERIC_WRITE, FILE_SHARE_READ, CREATE_ALWAYS, 0);
		} else {
			volume.open(get_volume_path(volume_idx), GENERIC_WRITE, FILE_SHARE_READ, OPEN_EXISTING, 0);
		}
		volume.set_pos(newSize - volume_idx * volume_size);
		volume.set_end();
		RETRY_END(*this->progress)

		for (UInt64 extra_idx = volume_idx + 1; extra_idx <= last_volume_idx; extra_idx++) {
			File::delete_file(get_volume_path(extra_idx));
		}

		stream_size = newSize;

		return S_OK;
		COM_ERROR_HANDLER_END
	}

	void clean_files() noexcept override
	{
		volume.close();
		UInt64 last_volume_idx = get_last_volume_idx();
		for (UInt64 volume_idx = 0; volume_idx <= last_volume_idx; volume_idx++) {
			File::delete_file_nt(get_volume_path(volume_idx));
		}
	}
};

template<bool UseVirtualDestructor>
class FileReadStream : public IInStream<UseVirtualDestructor>, public IStreamGetSize<UseVirtualDestructor>, public ComBase<UseVirtualDestructor>, private File
{
private:
	std::shared_ptr<ArchiveUpdateProgress> progress;
	bool dereference_symlinks;
	uint32_t fileattr;
public:
	FileReadStream(const std::wstring &file_path, bool open_shared,
			std::shared_ptr<ArchiveUpdateProgress> progress, bool dereference_symlinks, uint32_t fileattr, uint32_t smp)
		: progress(progress),
		  dereference_symlinks(dereference_symlinks),
		  fileattr(fileattr)
	{
		SudoRegionGuard sudo_guard;
		uint32_t flags = FILE_FLAG_SEQUENTIAL_SCAN;

		//fprintf(stderr, "FileReadStream( ) %ls \n", file_path.c_str() );

		if (fileattr & FILE_ATTRIBUTE_REPARSE_POINT) {
			if (!dereference_symlinks) {
				flags |= FILE_FLAG_OPEN_REPARSE_POINT;
				open(file_path, FILE_READ_DATA, FILE_SHARE_READ | (open_shared ? FILE_SHARE_WRITE | FILE_SHARE_DELETE : 0), smp, flags);
				return;
			}
		}

		open(file_path, FILE_READ_DATA, FILE_SHARE_READ | (open_shared ? FILE_SHARE_WRITE | FILE_SHARE_DELETE : 0), OPEN_EXISTING, flags);
	}

	UNKNOWN_IMPL_BEGIN
	UNKNOWN_IMPL_ITF(ISequentialInStream)
	UNKNOWN_IMPL_ITF(IInStream)
	UNKNOWN_IMPL_ITF(IStreamGetSize)
	UNKNOWN_IMPL_END

	STDMETHODIMP Read(void *data, UInt32 size, UInt32 *processedSize) noexcept override
	{
		COM_ERROR_HANDLER_BEGIN
		if (processedSize)
			*processedSize = 0;
		unsigned size_read = static_cast<unsigned>(read(data, size));
		this->progress->on_read_file(size_read);
		if (processedSize)
			*processedSize = size_read;
		return S_OK;
		COM_ERROR_HANDLER_END
	}

	STDMETHODIMP Seek(Int64 offset, UInt32 seekOrigin, UInt64 *newPosition) noexcept override
	{
		COM_ERROR_HANDLER_BEGIN
		if (newPosition)
			*newPosition = 0;
		UInt64 new_position = set_pos(offset, translate_seek_method(seekOrigin));
		if (newPosition)
			*newPosition = new_position;
		return S_OK;
		COM_ERROR_HANDLER_END
	}

	STDMETHODIMP GetSize(UInt64 *pSize) noexcept override
	{
		COM_ERROR_HANDLER_BEGIN
		if (!pSize) {
			FAIL(E_INVALIDARG);
		} else {
			*pSize = size();
			return S_OK;
		}
		COM_ERROR_HANDLER_END
	}
};

template<bool UseVirtualDestructor>
class AcmRelayStream : public ISequentialOutStream<UseVirtualDestructor>,
						public ISequentialInStream<UseVirtualDestructor>, public ComBase<UseVirtualDestructor> {
private:
	static constexpr size_t c_min_buff_size = 10 * 1024 * 1024;
	static constexpr size_t c_max_buff_size = 100 * 1024 * 1024;
	uint8_t *buffer;
	size_t buffer_size;
	size_t read_pos = 0;
	size_t write_pos = 0;
	size_t data_size = 0;
	bool writing_finished = false;
	bool reading_finished = false;
	std::mutex mtx;
	std::condition_variable cv;

public:
	AcmRelayStream(size_t capacity = 16)
		: buffer(nullptr), buffer_size(capacity << 20) {

		if (buffer_size < c_min_buff_size)
			buffer_size = c_min_buff_size;

		if (buffer_size > c_max_buff_size)
			buffer_size = c_max_buff_size;

		buffer = (unsigned char *)mmap(NULL, buffer_size, PROT_READ | PROT_WRITE,
				MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);

		if (buffer == MAP_FAILED) {
			FAIL(E_OUTOFMEMORY);
		}
	}

	~AcmRelayStream() {
		if (buffer)
			munmap(buffer, buffer_size);
	}

	UNKNOWN_IMPL_BEGIN
	UNKNOWN_IMPL_ITF(ISequentialOutStream)
	UNKNOWN_IMPL_ITF(ISequentialInStream)
	UNKNOWN_IMPL_END

	STDMETHODIMP Write(const void *data, UInt32 size, UInt32 *processedSize) noexcept override {
		std::unique_lock<std::mutex> lock(mtx);
		COM_ERROR_HANDLER_BEGIN

		cv.wait(lock, [this, size]() {
			return ( (buffer_size - data_size) >= size) || writing_finished || reading_finished;
		});

		if (writing_finished || reading_finished) {
			if (processedSize) {
				*processedSize = 0;
			}
			//	return E_ABORT;
			return S_OK;
		}

		size_t bytes_to_write = std::min(size, static_cast<UInt32>(buffer_size - data_size));
		size_t first_chunk = std::min(bytes_to_write, buffer_size - write_pos);
		memcpy(buffer + write_pos, data, first_chunk);
		if (first_chunk < bytes_to_write) {
			memcpy(buffer, (BYTE*)data + first_chunk, bytes_to_write - first_chunk);
		}

		write_pos = (write_pos + bytes_to_write) % buffer_size;
		data_size += bytes_to_write;

		if (processedSize) {
			*processedSize = static_cast<UInt32>(bytes_to_write);
		}

		cv.notify_all();
		return S_OK;
		COM_ERROR_HANDLER_END
	}

	STDMETHODIMP Read(void *data, UInt32 size, UInt32 *processedSize) noexcept override {
		std::unique_lock<std::mutex> lock(mtx);
		COM_ERROR_HANDLER_BEGIN

		cv.wait(lock, [this]() { return data_size > 0 || writing_finished; });

		size_t bytes_to_read = std::min(size, static_cast<UInt32>(data_size));
		size_t first_chunk = std::min(bytes_to_read, buffer_size - read_pos);
		memcpy(data, buffer + read_pos, first_chunk);
		if (first_chunk < bytes_to_read) {
			memcpy((BYTE*)data + first_chunk, buffer, bytes_to_read - first_chunk);
		}

		read_pos = (read_pos + bytes_to_read) % buffer_size;
		data_size -= bytes_to_read;

		if (processedSize) {
			*processedSize = static_cast<UInt32>(bytes_to_read);
		}

		cv.notify_all();
		return S_OK;
		COM_ERROR_HANDLER_END
	}

	void FinishWriting() {
		std::unique_lock<std::mutex> lock(mtx);
		writing_finished = true;
		cv.notify_all();
	}

	void FinishReading() {
		std::unique_lock<std::mutex> lock(mtx);
		reading_finished = true;
		cv.notify_all();
	}

};

struct FileIndexInfo
{
	std::wstring rel_path;
	FindData find_data;
};
typedef std::map<UInt32, FileIndexInfo> FileIndexMap;

struct PairHash {
	std::size_t operator()(const std::pair<uint64_t, uint64_t>& p) const {
		return p.first + p.second;
	}
};

struct PairEqual {
	bool operator()(const std::pair<uint64_t, uint64_t>& a, 
				   const std::pair<uint64_t, uint64_t>& b) const {
		return a.first == b.first && a.second == b.second;
	}
};

typedef std::unordered_map<std::pair<uint64_t, uint64_t>, uint32_t, PairHash, PairEqual> HardLinkMap;
typedef std::set<std::pair<dev_t, ino_t>> DirVisitedSet;

template<bool UseVirtualDestructor>
class PrepareUpdate : private ProgressMonitor
{
private:
	std::wstring src_dir;
	Archive<UseVirtualDestructor> &archive;
	FileIndexMap &file_index_map;
	HardLinkMap &hlmap;
	UInt32 &new_index;
	bool &ignore_errors;
	ErrorLog &error_log;
	OverwriteAction overwrite_action;
	Far::FileFilter *filter;
	bool &skipped_files;
	const UpdateOptions &options;
	const std::wstring *file_path;
	DirVisitedSet vdirs;

	void do_update_ui() override
	{
		const unsigned c_width = 60;
		std::wostringstream st;
		st << std::left << std::setw(c_width) << fit_str(*file_path, c_width) << L'\n';
		progress_text = st.str();
	}

	void update_progress(const std::wstring &file_path_value)
	{
		file_path = &file_path_value;
		update_ui();
	}

	bool process_file(const std::wstring &sub_dir, FindData &src_find_data, UInt32 dst_dir_index,
			UInt32 &file_index)
	{
		if (src_find_data.dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT) {
			if (options.skip_symlinks)
				return false;

			if (!options.dereference_symlinks) {
				struct stat s{};
				int r = sdc_lstat( StrWide2MB(add_trailing_slash(sub_dir) + src_find_data.cFileName).c_str(), &s);
				if (!r) {
					WINPORT(FileTime_UnixToWin32)(s.st_ctim, &src_find_data.ftCreationTime);
					WINPORT(FileTime_UnixToWin32)(s.st_atim, &src_find_data.ftLastAccessTime);
					WINPORT(FileTime_UnixToWin32)(s.st_mtim, &src_find_data.ftLastWriteTime);

					src_find_data.UnixOwner = s.st_uid;
					src_find_data.UnixGroup = s.st_gid;
					src_find_data.nFileSize = s.st_size;
					src_find_data.dwUnixMode = s.st_mode;
					src_find_data.nHardLinks = (DWORD)s.st_nlink;
				}
			}
			else if (src_find_data.dwFileAttributes & FILE_ATTRIBUTE_BROKEN) {
				return false;
			}
		}

		uint32_t ftype = src_find_data.dwUnixMode & S_IFMT;
		if (ftype == S_IFCHR || ftype == S_IFBLK) {

			struct stat s{};
			int r = sdc_stat( StrWide2MB(add_trailing_slash(sub_dir) + src_find_data.cFileName).c_str(), &s);
			if (!r) {
				src_find_data.UnixDevice = s.st_rdev;
			}
		}

		if (filter) {
			PluginPanelItem filter_data;
			memset(&filter_data, 0, sizeof(PluginPanelItem));

			filter_data.FindData.ftCreationTime = src_find_data.ftCreationTime;
			filter_data.FindData.ftLastAccessTime = src_find_data.ftLastAccessTime;
			filter_data.FindData.ftLastWriteTime = src_find_data.ftLastWriteTime;
			filter_data.FindData.dwFileAttributes = src_find_data.dwFileAttributes;
			filter_data.FindData.dwUnixMode = src_find_data.dwUnixMode;
			filter_data.NumberOfLinks = src_find_data.nHardLinks;

			if (filter_data.NumberOfLinks > 1 )
				filter_data.FindData.dwFileAttributes |= FILE_ATTRIBUTE_HARDLINKS;

			filter_data.FindData.nFileSize = src_find_data.size();
			filter_data.FindData.nPhysicalSize = 0;
			filter_data.FindData.lpwszFileName = const_cast<wchar_t *>(src_find_data.cFileName);

			if (!filter->match(filter_data))
				return false;
		}

		if (!src_find_data.is_dir() && src_find_data.nHardLinks > 1 && options.skip_hardlinks)
			return false;

		FileIndexInfo file_index_info;
		file_index_info.rel_path = sub_dir;
		file_index_info.find_data = src_find_data;

		ArcFileInfo file_info;
		file_info.is_dir = src_find_data.is_dir();
		file_info.parent = dst_dir_index;
		file_info.name = src_find_data.cFileName;

		FileIndexRange fi_range = std::equal_range(archive.file_list_index.begin(),
				archive.file_list_index.end(), -1, [&](UInt32 left, UInt32 right) -> bool {
					const ArcFileInfo &fi_left = left == (UInt32)-1 ? file_info : archive.file_list[left];
					const ArcFileInfo &fi_right = right == (UInt32)-1 ? file_info : archive.file_list[right];
					return fi_left < fi_right;
				});

		if (fi_range.first == fi_range.second) {
			// new file
			file_index = new_index;
			file_index_map[new_index] = file_index_info;
			new_index++;
		} else {
			// updated file
			file_index = *fi_range.first;
			if (file_index >= archive.m_num_indices) {	  // fake index
				file_index_map[new_index] = file_index_info;
				new_index++;
			} else if (!file_info.is_dir) {
				OverwriteAction overwrite;
				if (overwrite_action == oaAsk) {
					OverwriteFileInfo src_ov_info, dst_ov_info;

					src_ov_info.is_dir = src_find_data.is_dir();
					src_ov_info.size = src_find_data.size();
					src_ov_info.mtime = src_find_data.ftLastWriteTime;

					dst_ov_info.is_dir = file_info.is_dir;
					dst_ov_info.size = archive.get_size(file_index);
					dst_ov_info.mtime = archive.get_mtime(file_index);

					ProgressSuspend ps(*this);
					OverwriteOptions ov_options;
					if (!overwrite_dialog(add_trailing_slash(sub_dir) + file_info.name, src_ov_info,
								dst_ov_info, odkUpdate, ov_options))
						FAIL(E_ABORT);
					overwrite = ov_options.action;
					if (ov_options.all)
						overwrite_action = ov_options.action;
				} else
					overwrite = overwrite_action;
				if (overwrite == oaSkip) {
					skipped_files = true;
					return false;
				}
				file_index_map[file_index] = file_index_info;
			}
		}

		if (!src_find_data.is_dir() && src_find_data.nHardLinks > 1 && !options.duplicate_hardlinks) {
			std::pair<uint64_t, uint64_t> key = {src_find_data.UnixDevice, src_find_data.UnixNode};
			auto it = hlmap.find(key);
			if (it == hlmap.end()) {
				hlmap[key] = file_index;
			}
		}

		return true;
	}

	bool process_file_enum(FileEnum &file_enum, const std::wstring &sub_dir, UInt32 dst_dir_index)
	{
		bool not_empty = false;
		while (true) {
			bool more = false;
			RETRY_OR_IGNORE_BEGIN
			more = file_enum.next();
			RETRY_OR_IGNORE_END(ignore_errors, error_log, *this)
			if (error_ignored || !more)
				break;
			UInt32 saved_new_index = new_index;
			UInt32 file_index;

			if (process_file(sub_dir, file_enum.data(), dst_dir_index, file_index)) {

				if (file_enum.data().is_dir() && ( !(file_enum.data().dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT) || options.dereference_symlinks ) ) {
					std::wstring rel_path = add_trailing_slash(sub_dir) + file_enum.data().cFileName;
					std::wstring full_path = add_trailing_slash(src_dir) + rel_path;
					update_progress(full_path);

					bool added_to_visited = false;

					if (options.dereference_symlinks) {
						auto key = std::make_pair(file_enum.data().UnixDevice, file_enum.data().UnixNode);
						if (vdirs.find(key) != vdirs.end()) {
							fprintf(stderr, "process_file_enum( ) break loop %ls \n", full_path.c_str() );
							continue;
						}
						vdirs.insert(key);
						added_to_visited = true;
					}

					DirList dir_list(full_path);
					if (!process_file_enum(dir_list, rel_path, file_index)) {
						if (filter) {
							file_index_map.erase(file_index);
							new_index = saved_new_index;
						}
					} else
						not_empty = true;

					if (added_to_visited) {
						vdirs.erase({file_enum.data().UnixDevice, file_enum.data().UnixNode});
					}

				} else
					not_empty = true;
			}
		}
		return not_empty;
	}

public:
	PrepareUpdate(const std::wstring &src_dir, const std::vector<std::wstring> &file_names,
			UInt32 dst_dir_index, Archive<UseVirtualDestructor> &archive, FileIndexMap &file_index_map,
			HardLinkMap &hlmap, UInt32 &new_index,
			OverwriteAction overwrite_action, bool &ignore_errors, ErrorLog &error_log,
			Far::FileFilter *filter, bool &skipped_files, const UpdateOptions &options)
		: ProgressMonitor(Far::get_msg(MSG_PROGRESS_SCAN_DIRS), false),
		  src_dir(src_dir),
		  archive(archive),
		  file_index_map(file_index_map),
		  hlmap(hlmap),
		  new_index(new_index),
		  ignore_errors(ignore_errors),
		  error_log(error_log),
		  overwrite_action(overwrite_action),
		  filter(filter),
		  skipped_files(skipped_files),
		  options(options),
		  vdirs()
	{
		skipped_files = false;
		if (filter)
			filter->start();

		for (unsigned i = 0; i < file_names.size(); i++) {
			std::wstring full_path = add_trailing_slash(src_dir) + file_names[i];
			FileEnum file_enum(full_path);
			process_file_enum(file_enum, extract_file_path(file_names[i]), dst_dir_index);
		}
	}
};

template<bool UseVirtualDestructor>
class ArchiveUpdater : public IArchiveUpdateCallback<UseVirtualDestructor>,
						public ICryptoGetTextPassword2<UseVirtualDestructor>, public ComBase<UseVirtualDestructor>
{
private:
	std::wstring src_dir;
	std::wstring dst_dir;
	UInt32 num_indices;
	std::shared_ptr<FileIndexMap> file_index_map;
	HardLinkMap &hlmap;
	const UpdateOptions &options;
	std::shared_ptr<bool> ignore_errors;
	std::shared_ptr<ErrorLog> error_log;
	std::shared_ptr<ArchiveUpdateProgress> progress;
	ComObject<ISequentialInStream<UseVirtualDestructor>> mem_stream;
	bool use_mem_stream = false;
	FILETIME crft;
	std::wstring tmp_str;

public:
	ArchiveUpdater(const std::wstring &src_dir, const std::wstring &dst_dir, UInt32 num_indices,
			std::shared_ptr<FileIndexMap> file_index_map, HardLinkMap &hlmap, const UpdateOptions &options,
			std::shared_ptr<bool> ignore_errors, std::shared_ptr<ErrorLog> error_log,
			std::shared_ptr<ArchiveUpdateProgress> progress, ISequentialInStream<UseVirtualDestructor> *stream = nullptr, 
			bool use_mem_stream = false)
		: src_dir(src_dir),
		  dst_dir(dst_dir),
		  num_indices(num_indices),
		  file_index_map(file_index_map),
		  hlmap(hlmap),
		  options(options),
		  ignore_errors(ignore_errors),
		  error_log(error_log),
		  progress(progress),
		  mem_stream(stream),
		  use_mem_stream(use_mem_stream)
	{
		WINPORT(GetSystemTimeAsFileTime)(&crft);
	}

	UNKNOWN_IMPL_BEGIN
	UNKNOWN_IMPL_ITF(IProgress)
	UNKNOWN_IMPL_ITF(IArchiveUpdateCallback)
	UNKNOWN_IMPL_ITF(ICryptoGetTextPassword2)
	UNKNOWN_IMPL_END

	void SetMemoryStream(ISequentialInStream<UseVirtualDestructor>* stream) {
		mem_stream = stream;
		use_mem_stream = true;
	}

	STDMETHODIMP SetTotal(UInt64 total) noexcept override
	{
		CriticalSectionLock lock(GetSync());
		COM_ERROR_HANDLER_BEGIN
		this->progress->on_total_update(total);
		return S_OK;
		COM_ERROR_HANDLER_END
	}

	STDMETHODIMP SetCompleted(const UInt64 *completeValue) noexcept override
	{
		CriticalSectionLock lock(GetSync());
		COM_ERROR_HANDLER_BEGIN
		if (completeValue)
			this->progress->on_completed_update(*completeValue);
		return S_OK;
		COM_ERROR_HANDLER_END
	}

	STDMETHODIMP GetUpdateItemInfo(UInt32 index, Int32 *newData, Int32 *newProperties,
			UInt32 *indexInArchive) noexcept override
	{
		COM_ERROR_HANDLER_BEGIN
		auto found = file_index_map->find(index);
		if (found == file_index_map->end()) {
			*newData = 0;
			*newProperties = 0;
			*indexInArchive = index;
		} else {
			*newData = 1;
			*newProperties = 1;
			*indexInArchive = found->first < num_indices ? found->first : -1;
		}
		return S_OK;
		COM_ERROR_HANDLER_END
	}

	STDMETHODIMP GetProperty(UInt32 index, PROPID propID, PROPVARIANT *value) noexcept override
	{
		COM_ERROR_HANDLER_BEGIN
		const FileIndexInfo &file_index_info = file_index_map->at(index);
		constexpr DWORD c_valid_export_attributes =
			FILE_ATTRIBUTE_READONLY |
			FILE_ATTRIBUTE_HIDDEN |
			FILE_ATTRIBUTE_SYSTEM |
			FILE_ATTRIBUTE_DIRECTORY |
			FILE_ATTRIBUTE_ARCHIVE |
			FILE_ATTRIBUTE_DEVICE_BLOCK |
			FILE_ATTRIBUTE_NORMAL |
			FILE_ATTRIBUTE_TEMPORARY |
			FILE_ATTRIBUTE_SPARSE_FILE |
			FILE_ATTRIBUTE_REPARSE_POINT |
			FILE_ATTRIBUTE_COMPRESSED |
			FILE_ATTRIBUTE_OFFLINE |
			FILE_ATTRIBUTE_NOT_CONTENT_INDEXED |
			FILE_ATTRIBUTE_ENCRYPTED |
			FILE_ATTRIBUTE_INTEGRITY_STREAM;
			//#define FILE_ATTRIBUTE_INTEGRITY_STREAM	 0x00008000
			//#define FILE_ATTRIBUTE_VIRTUAL			  0x00010000 // 65536
			//#define FILE_ATTRIBUTE_NO_SCRUB_DATA		0x00020000
			//#define FILE_ATTRIBUTE_EA				   0x00040000
			//#define FILE_ATTRIBUTE_PINNED			   0x00080000
			//#define FILE_ATTRIBUTE_UNPINNED			 0x00100000

		PropVariant prop;
//		wchar_t wtmp[256];

		switch (propID) {
			case kpidPath:
				prop = add_trailing_slash(add_trailing_slash(dst_dir) + file_index_info.rel_path)
						+ file_index_info.find_data.cFileName;
				break;
			case kpidName:
				prop = file_index_info.find_data.cFileName;
				break;
			case kpidIsDir:
				if (use_mem_stream) {
					prop = (bool)false;
					break;
				}
				if (file_index_info.find_data.dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT) {
					if (!options.dereference_symlinks && (file_index_info.find_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) ) {
						prop = (bool)false;
						break;
					}
				}
				prop = file_index_info.find_data.is_dir();
				break;

			case kpidSize: {
				if (use_mem_stream) {
					prop = (UInt64)100ull;
					break;
				}
				uint32_t ftype = file_index_info.find_data.dwUnixMode & S_IFMT;
				if (ftype == S_IFSOCK || ftype == S_IFCHR || ftype == S_IFBLK || ftype == S_IFIFO) {
					prop = (UInt64)0ull;
					break;
				}

				prop = file_index_info.find_data.size();
			}
			break;

			case kpidCTime: {
				const FILETIME *ptime = &file_index_info.find_data.ftCreationTime;
				if (options.use_export_settings && options.export_options.export_creation_time != triUndef) {
					if (options.export_options.export_creation_time) {
						if (options.export_options.custom_creation_time) {
							if (options.export_options.current_creation_time)
								ptime = &crft;
							else
								ptime = &options.export_options.ftCreationTime;
						}
						else
							ptime = &file_index_info.find_data.ftCreationTime;
					}
				}
#if IS_BIG_ENDIAN
				prop = FILETIME{ptime->dwLowDateTime, ptime->dwHighDateTime};
#else
				prop = *ptime;
#endif
			}
			break;
			case kpidATime: {
				const FILETIME *ptime = &file_index_info.find_data.ftLastAccessTime;
				if (options.use_export_settings && options.export_options.export_last_access_time != triUndef) {
					if (options.export_options.export_last_access_time) {
						if (options.export_options.custom_last_access_time) {
							if (options.export_options.current_last_access_time)
								ptime = &crft;
							else
								ptime = &options.export_options.ftLastAccessTime;
						}
						else
							ptime = &file_index_info.find_data.ftLastAccessTime;
					}
				}
#if IS_BIG_ENDIAN
				prop = FILETIME{ptime->dwLowDateTime, ptime->dwHighDateTime};
#else
				prop = *ptime;
#endif
			}
			break;
			case kpidMTime: {
				const FILETIME *ptime = &file_index_info.find_data.ftLastWriteTime;
				if (options.use_export_settings && options.export_options.export_last_write_time != triUndef) {
					if (options.export_options.export_last_write_time) {
						if (options.export_options.custom_last_write_time) {
							if (options.export_options.current_last_write_time)
								ptime = &crft;
							else
								ptime = &options.export_options.ftLastWriteTime;
						}
						else
							ptime = &file_index_info.find_data.ftLastWriteTime;
					}
				}
#if IS_BIG_ENDIAN
				prop = FILETIME{ptime->dwLowDateTime, ptime->dwHighDateTime};
#else
				prop = *ptime;
#endif
			}
			break;
			case kpidChangeTime: {
			}
			break;
			case kpidAttrib: {
				uint32_t attributes = 0;
				uint32_t unixmode = 0;
				if (options.use_export_settings) {
					if (options.export_options.export_file_attributes) {
						if (options.export_options.custom_file_attributes) {
							attributes = file_index_info.find_data.dwFileAttributes & options.export_options.dwExportAttributesMask;
							attributes |= options.export_options.dwFileAttributes;
							attributes &= 0x0FFFFFFF;
						}
						else
							attributes = file_index_info.find_data.dwFileAttributes & c_valid_export_attributes;
					}
					else
						attributes = 0;

					if (options.export_options.export_unix_mode) {
						attributes &= 0x0000FFFF;
						if (options.export_options.custom_unix_mode)
							unixmode = static_cast<UInt32>(options.export_options.UnixNode);
						else
							unixmode = file_index_info.find_data.dwUnixMode & 0xFFF;
					}
					else {
						unixmode = 0;
					}
				}
				else {
					attributes = file_index_info.find_data.dwFileAttributes & c_valid_export_attributes;
					unixmode = file_index_info.find_data.dwUnixMode & 0xFFF;
				}

				if (attributes & FILE_ATTRIBUTE_REPARSE_POINT) {
					if (options.dereference_symlinks) {
						attributes &= ~(FILE_ATTRIBUTE_REPARSE_POINT);
						if (options.use_export_settings && !options.export_options.export_unix_mode)
							unixmode = 0;
						else
							unixmode |= file_index_info.find_data.dwUnixMode & 0xF000;
					}
					else {
						unixmode |= S_IFLNK;
					}
				}
				else {
					if (options.use_export_settings && !options.export_options.export_unix_mode)
						unixmode = 0;
					else
						unixmode |= file_index_info.find_data.dwUnixMode & 0xF000;
				}

				prop = static_cast<UInt32>( attributes | (unixmode << 16) );
//				fprintf(stderr, "----------PUT ATTRIB = %X | %u\n", (UInt32)file_index_info.find_data.dwUnixMode, (UInt32)file_index_info.find_data.dwUnixMode);
			}
			break;

			case kpidPosixAttrib: {

				uint32_t unixmode = file_index_info.find_data.dwUnixMode;
				if (options.use_export_settings) {
					if (options.export_options.export_unix_mode) {
						if (options.export_options.custom_unix_mode) {
							unixmode &= 0xF000;
							unixmode |= static_cast<UInt32>(options.export_options.UnixNode);
						}
					}
					else
						unixmode &= 0xF000;
				}

				prop = static_cast<UInt32>(unixmode);
			}
			break;

			case kpidSymLink: {
				if ( !(file_index_info.find_data.dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT) || options.dereference_symlinks)
					break;

				char symlinkdata[PATH_MAX + 1];
				std::wstring file_path = add_trailing_slash(add_trailing_slash(src_dir) + file_index_info.rel_path) + file_index_info.find_data.cFileName;
				int r = sdc_readlink(Wide2MB(file_path.c_str()).c_str(), symlinkdata, PATH_MAX);
				if (r <= 0 || r > PATH_MAX)
					break;
				symlinkdata[r] = 0;
				std::wstring symtp;
				MB2Wide(symlinkdata, symtp);

				if (options.symlink_fix_path_mode == 1) {
					prop = make_absolute_symlink_target(file_path, symtp);
				} else if (options.symlink_fix_path_mode == 2) {
					prop = make_relative_symlink_target(file_path, symtp);
				} else {
					prop = symtp;
				}
			}
			break;

			case kpidHardLink: {

//				prop = L"Broken Hardlink";
//				break;

				if (file_index_info.find_data.is_dir() || file_index_info.find_data.nHardLinks < 2)
					break;

				std::pair<uint64_t, uint64_t> key = {file_index_info.find_data.UnixDevice, file_index_info.find_data.UnixNode};
				HardLinkMap::iterator it = hlmap.find(key);

				if (it == hlmap.end())
					break;
				if (it->second == index)
					break;

				prop = add_trailing_slash(add_trailing_slash(dst_dir) + file_index_map->at(it->second).rel_path)
						+ file_index_map->at(it->second).find_data.cFileName;
			}
			break;

			case kpidUserId:
				if (options.use_export_settings) {
					if (options.export_options.export_user_id) {
						if (options.export_options.custom_user_id)
							prop = static_cast<UInt32>(options.export_options.UnixOwner);
						else
							prop = static_cast<UInt32>(file_index_info.find_data.UnixOwner);
					}
				}
				else
					prop = static_cast<UInt32>(file_index_info.find_data.UnixOwner);
			break;

			case kpidGroupId:
				if (options.use_export_settings) {
					if (options.export_options.export_group_id) {
						if (options.export_options.custom_group_id)
							prop = static_cast<UInt32>(options.export_options.UnixGroup);
						else
							prop = static_cast<UInt32>(file_index_info.find_data.UnixGroup);
					}
				}
				else
					prop = static_cast<UInt32>(file_index_info.find_data.UnixGroup);
			break;

			case kpidUser: {
				struct passwd *pw = getpwuid(file_index_info.find_data.UnixOwner);
				if (pw && pw->pw_name) {
					MB2Wide(pw->pw_name, tmp_str);
//					WINPORT_MultiByteToWideChar(CP_UTF8, 0, pw->pw_name, -1, wtmp, 256);
//					wtmp[255] = L'\0';
				} else {
					tmp_str = L"-";
				}

				if (options.use_export_settings) {
					if (options.export_options.export_user_name) {
						if (options.export_options.custom_user_name)
							prop = options.export_options.Owner;
						else
							prop = tmp_str;
					}
				}
				else
					prop = tmp_str;
			}
			break;

			case kpidGroup: {
				struct group *gr = getgrgid(file_index_info.find_data.UnixGroup);
				if (gr && gr->gr_name) {
					MB2Wide(gr->gr_name, tmp_str);
				} else {
					tmp_str = L"-";
				}

				if (options.use_export_settings) {
					if (options.export_options.export_group_name) {
						if (options.export_options.custom_group_name)
							prop = options.export_options.Group;
						else
							prop = tmp_str;
					}
				}
				else
					prop = tmp_str;
			}
			break;

			case kpidComment:
//				if (options.use_export_settings && options.export_options.export_file_descriptions) {
//					prop = L"TODO:";
//				}
			break;

			case kpidDevMajor:
//					prop = (UInt32)999;
			break;

			case kpidDevMinor:
//					prop = (UInt32)999;
			break;

			case kpidDeviceMajor: {
				uint32_t ftype = file_index_info.find_data.dwUnixMode & S_IFMT;
				if (ftype != S_IFCHR && ftype != S_IFBLK) {
					break;
				}

				if (options.use_export_settings && options.export_options.export_unix_device &&
						options.export_options.custom_unix_device) {
					if (options.export_options.custom_unix_device)
						prop = (UInt32)major(options.export_options.UnixDevice);
				}
				else
					prop = (UInt32)major(file_index_info.find_data.UnixDevice);
			}
			break;

			case kpidDeviceMinor: {
				uint32_t ftype = file_index_info.find_data.dwUnixMode & S_IFMT;
				if (ftype != S_IFCHR && ftype != S_IFBLK) {
					break;
				}

				if (options.use_export_settings && options.export_options.export_unix_device &&
						options.export_options.custom_unix_device) {
					if (options.export_options.custom_unix_device)
						prop = (UInt32)minor(options.export_options.UnixDevice);
				}
				else
					prop = (UInt32)minor(file_index_info.find_data.UnixDevice);
			}
			break;
		}
		prop.detach(value);
		return S_OK;
		COM_ERROR_HANDLER_END
	}

	STDMETHODIMP GetStream(UInt32 index, ISequentialInStream<UseVirtualDestructor> **inStream) noexcept override
	{
		COM_ERROR_HANDLER_BEGIN
		*inStream = nullptr;

		if (use_mem_stream) {
			if (mem_stream) {
				mem_stream.detach(inStream);
			}
			return S_OK;
		}

		FileIndexInfo &file_index_info = file_index_map->at(index);
		if (file_index_info.find_data.is_dir() && (!(file_index_info.find_data.dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT) || options.dereference_symlinks) ) {
			return S_OK;
		}

		uint32_t ftype = file_index_info.find_data.dwUnixMode & S_IFMT;
		if (ftype == S_IFSOCK || ftype == S_IFCHR || ftype == S_IFBLK || ftype == S_IFIFO) {
			return S_OK;
		}

		std::wstring file_path = add_trailing_slash(add_trailing_slash(src_dir) + file_index_info.rel_path)
				+ file_index_info.find_data.cFileName;
		this->progress->on_open_file(file_path, file_index_info.find_data.size());

		ComObject<ISequentialInStream<UseVirtualDestructor>> stream;
		RETRY_OR_IGNORE_BEGIN
		stream = new FileReadStream<UseVirtualDestructor>(file_path, options.open_shared, progress, options.dereference_symlinks, 
															file_index_info.find_data.dwFileAttributes, options.symlink_fix_path_mode );

		RETRY_OR_IGNORE_END(*ignore_errors, *error_log, *this->progress)
		if (error_ignored)
			return S_FALSE;

		stream.detach(inStream);
		return S_OK;

		COM_ERROR_HANDLER_END
	}

	STDMETHODIMP SetOperationResult(Int32 operationResult) noexcept override
	{
		CriticalSectionLock lock(GetSync());
		COM_ERROR_HANDLER_BEGIN
		if (operationResult == NArchive::NUpdate::NOperationResult::kOK)
			return S_OK;
		/*
		  if (operationResult == NArchive::NUpdate::NOperationResult::kError)
			FAIL_MSG(Far::get_msg(MSG_ERROR_UPDATE_ERROR));
			else
		*/
		FAIL_MSG(Far::get_msg(MSG_ERROR_UPDATE_UNKNOWN));
		COM_ERROR_HANDLER_END
	}

	STDMETHODIMP CryptoGetTextPassword2(Int32 *passwordIsDefined, BSTR *password) noexcept override
	{
		CriticalSectionLock lock(GetSync());
		COM_ERROR_HANDLER_BEGIN
		*passwordIsDefined = !options.password.empty();
		BStr(options.password).detach(password);
		return S_OK;
		COM_ERROR_HANDLER_END
	}
};

template<bool UseVirtualDestructor>
void Archive<UseVirtualDestructor>::set_properties(IOutArchive<UseVirtualDestructor> *out_arc, const UpdateOptions &options)
{
	ComObject<ISetProperties<UseVirtualDestructor>> set_props;
	if ( SUCCEEDED(out_arc->QueryInterface((REFIID)IID_ISetProperties, reinterpret_cast<void **>(&set_props))) ) {
		static ExternalCodec defopts{L"", 1, 9, 0, 1, 3, 5, 7, 9, false};
		defopts.reset();
//		fprintf(stderr, " +++ Archive::set_properties +++\n");

		std::wstring method;
		bool ignore_method = false;

		if (options.arc_type == c_7z || options.arc_type == c_zip || options.arc_type == c_tar) {
			if (options.method == c_method_default)
				ignore_method = true;
			else
				method = options.method;
		} else if (ArcAPI::formats().count(options.arc_type)) {
			method = ArcAPI::formats().at(options.arc_type).name;
		}

		auto method_params = &defopts;
		for (size_t i = 0; i < g_options.codecs.size(); ++i) {
			if (method == g_options.codecs[i].name) {
				method_params = &g_options.codecs[i];
				break;
			}
		}

		const auto is_digit = [](const std::wstring &n, const wchar_t up) {
			return n.size() == 1 && n[0] >= L'0' && n[0] <= up;
		};

		const auto variant = [](const std::wstring &v) {
			PropVariant var;
			if (!v.empty()) {
				wchar_t *endptr = nullptr;
				UINT64 v64 = wcstoull(v.c_str(), &endptr, 10);

				if (endptr && !*endptr) {
					if (v64 <= UINT_MAX)
						var = static_cast<UInt32>(v64);
					else
						var = v64;
				} else
					var = v;
			}
			return var;
		};

		auto adv = options.advanced;
		//	adv.clear();

		if (!adv.empty() && adv[0] == L'-') {
			adv.erase(0, 1);
			if (!adv.empty() && adv[0] == L'-') {
				adv.erase(0, 1);
				ignore_method = true;
			}
		} else {
			if (!method_params->adv.empty()) {
				adv = method_params->adv + L' ' + adv;
			}
		}

		if (options.arc_type != c_tar && options.repack) ignore_method = true;

		auto adv_params = split(adv, L' ');
		int adv_level = -1;
		bool adv_have_0 = false, adv_have_1 = false, adv_have_bcj = false, adv_have_qs = false;

		for (auto it = adv_params.begin(); it != adv_params.end();) {
			auto param = *it;

			if (param == L"s" || param == L"s1" || param == L"s+") {	// s s+ s1
				*it = param = L"s=on";
			} else if (param == L"s0" || param == L"s-") {
				*it = param = L"s=off";
			} else if (param == L"qs" || param == L"qs1" || param == L"qs+" || param == L"qs=on") {
				*it = param = L"qs=on";
				adv_have_qs = true;
			} else if (param == L"qs0" || param == L"qs-" || param == L"qs=off") {
				*it = param = L"qs=off";
				adv_have_qs = true;
			} else if (param.size() >= 2 && param[0] == L'x' && param[1] >= L'0'
					&& param[1] <= L'9') {	  // xN
				*it = param.insert(1, 1, L'=');
			} else if (param.size() >= 3 && param[0] == L'm' && param[1] == 't' && param[2] >= L'1'
					&& param[2] <= L'9') {		   // mtN
				*it = param.insert(2, 1, L'=');	   // mt=N
			} else if (param.size() >= 3 && param[0] == L'y' && param[1] == 'x' && param[2] >= L'1'
					&& param[2] <= L'9') {		   // yxN
				*it = param.insert(2, 1, L'=');	   // yx=N
			}

			auto sep = param.find(L'=');
			if (param.empty() || sep == 0) {
				it = adv_params.erase(it);
				continue;
			} else if (sep != std::wstring::npos) {
				auto name = param.substr(0, sep);
				auto value = param.substr(sep + 1);

				if (0 == StrCmpI(name.c_str(), L"x")) {
					it = adv_params.erase(it);
					if (!value.empty() && value[0] >= L'0' && value[0] <= L'9') {
						adv_level = static_cast<int>(str_to_uint(value));
					}
					continue;
				}
				adv_have_0 = adv_have_0 || name == L"0";
				adv_have_1 = adv_have_1 || name == L"1";
				if (is_digit(name, L'9') && value.size() >= 3)
					adv_have_bcj = adv_have_bcj || 0 == StrCmpI(value.substr(0, 3).c_str(), L"BCJ");
			}
			++it;
		}
		if (!adv_have_qs && g_options.qs_by_default && options.arc_type == c_7z)
			adv_params.emplace_back(L"qs=on");

		auto level = options.level;
		if (adv_level < 0) {
			if (level == 1)
				level = method_params->L1;
			else if (level == 3)
				level = method_params->L3;
			else if (level == 5)
				level = method_params->L5;
			else if (level == 7)
				level = method_params->L7;
			else if (level == 9)
				level = method_params->L9;
		} else {
			level = adv_level;
			if (method_params->mod0L && level % method_params->mod0L == 0)
				level = 0;
			else if (level && level < method_params->minL)
				level = method_params->minL;
			else if (level > method_params->maxL)
				level = method_params->maxL;
		}

		std::vector<std::wstring> names;
		std::vector<PropVariant> values;
		int n_01 = 0;

		if (options.arc_type == c_7z) {
			if (!ignore_method) {
				names.push_back(L"0");
				values.push_back(method);
				++n_01;
			}
			if (method_params->bcj_only && !adv_have_bcj && !ignore_method) {
				names.push_back(L"1");
				values.push_back(L"BCJ");
				++n_01;
			}
			names.push_back(L"x");
			values.push_back(level);
			if (level != 0) {
				names.push_back(L"s");
				values.push_back(options.solid);
			}
			if (options.encrypt) {
				if (options.encrypt_header != triUndef) {
					names.push_back(L"he");
					values.push_back(options.encrypt_header == triTrue);
				}
			}
		} else if (options.arc_type == c_zip) {
			if (!ignore_method) {
				names.push_back(L"0");
				values.push_back(method);
				++n_01;
			}
			names.push_back(L"x");
			values.push_back(level);
		} else if (options.arc_type == c_tar) {
			if (!ignore_method) {
				names.push_back(L"m");
				values.push_back(method);
				++n_01;
			}
			names.push_back(L"x");
			values.push_back(level);
		} else if (options.arc_type != c_bzip2 || level != 0) {
			names.push_back(L"x");
			values.push_back(level);
		}

		if (options.arc_type == c_7z || options.arc_type == c_zip) {
			if (options.multithreading) {
				names.push_back(L"mt");
				if (!options.threads_num)
					values.push_back(L"on");
				else
					values.push_back(options.threads_num);
			}
			else {
				names.push_back(L"mt");
				values.push_back(L"off");
			}
		}

		if (options.arc_type == c_tar || options.arc_type == c_7z || options.arc_type == c_zip || options.arc_type == c_wim) {
			if (options.use_export_settings) {
				if (options.export_options.export_last_write_time != triUndef) {
					names.push_back(L"tm");

					if (options.export_options.export_last_write_time)
						values.push_back(L"on");
					else
						values.push_back(L"off");
				}

				if (options.export_options.export_creation_time != triUndef) {
					names.push_back(L"tc");

					if (options.export_options.export_creation_time)
						values.push_back(L"on");
					else
						values.push_back(L"off");
				}

				if (options.export_options.export_last_access_time != triUndef) {
					names.push_back(L"ta");

					if (options.export_options.export_last_access_time)
						values.push_back(L"on");
					else
						values.push_back(L"off");
				}
			}
		}

		int n_shift = (adv_have_0 || adv_have_1) ? n_01 : 0;
		for (const auto &param : adv_params) {
			auto sep = param.find(L'=');
			std::wstring name = sep != std::wstring::npos ? param.substr(0, sep) : param;
			std::wstring value = sep != std::wstring::npos ? param.substr(sep + 1) : std::wstring();

			if (n_shift && is_digit(name, L'7'))
				name[0] = static_cast<wchar_t>(name[0] + n_shift);
			bool found = false;
			unsigned i = 0;
			for (const auto &n : names) {
				std::wstring v = values[i].is_str() ? values[i].get_str() : std::wstring();
				if (0 == StrCmpI(n.c_str(), name.c_str())
						|| ((int)i < n_01 && is_digit(n, L'9') && is_digit(name, L'9') && !v.empty()
								&& substr_match(upcase(value), 0, upcase(v).c_str()))) {
					found = true;
					values[i] = variant(value);
					break;
				}
				++i;
			}
			if (!found) {
				names.emplace_back(name);
				values.emplace_back(value);
			}
		}

		// normalize {N}=... parameter names (start from '0', no gaps): {'1' '5' '3'} -> {'0' '2' '1'}
		//
		int gaps[10] = {1, 1, 1, 1, 1, 1, 1, 1, 1, 1};
		for (const auto &n : names)
			if (is_digit(n, L'9'))
				gaps[n[0] - L'0'] = 0;
		int gap = 0;
		for (int i = 0; i < 10; ++i) {
			auto t = gaps[i];
			gaps[i] = gap;
			gap += t;
		}
		for (size_t i = 0; i < names.size(); ++i)
			if (is_digit(names[i], L'9'))
				names[i][0] = static_cast<wchar_t>(names[i][0] - gaps[names[i][0] - L'0']);

		std::vector<const wchar_t *> name_ptrs;
		name_ptrs.reserve(names.size());
		for (unsigned i = 0; i < names.size(); i++) {
			name_ptrs.push_back(names[i].c_str());
		}

		typedef uint64_t (*converter_t)(const wchar_t *, wchar_t **, int);
		const auto string_to_integer = [](const std::wstring &str, converter_t converter, PropVariant &v) {
			wchar_t *endptr{};
			const auto str_end = str.data() + str.size();

			//	  const auto value = converter(str.c_str(), &endptr, 10);
			const uint64_t value = converter(str.c_str(), &endptr, 10);

			if (endptr != str_end) {
				return false;
			}
			if (value > std::numeric_limits<UInt32>::max())
				v = static_cast<UInt64>(value);
			else
				v = static_cast<UInt32>(value);

			return true;
		};

		for (auto &i : values) {
			if (!i.is_str())
				continue;

			const auto str = i.get_str();
			if (str.empty())
				continue;

			string_to_integer(str, (converter_t)&wcstoull, i)
					|| string_to_integer(str, (converter_t)&wcstoll, i);
		}

		CHECK_COM(set_props->SetProperties(name_ptrs.data(), values.data(), static_cast<UInt32>(values.size())));
	}
}

class DeleteSrcFiles : public ProgressMonitor
{
private:
	bool &ignore_errors;
	ErrorLog &error_log;

	const std::wstring *m_file_path;

	void do_update_ui() override
	{
		const unsigned c_width = 60;
		std::wostringstream st;
		st << std::left << std::setw(c_width) << fit_str(*m_file_path, c_width) << L'\n';
		progress_text = st.str();
	}

	void update_progress(const std::wstring &file_path_value)
	{
		m_file_path = &file_path_value;
		update_ui();
	}

	void delete_src_file(const std::wstring &file_path)
	{
		update_progress(file_path);
		RETRY_OR_IGNORE_BEGIN
		try {
			File::delete_file(file_path);
		} catch (const Error &e) {
			if (e.code != HRESULT_FROM_WIN32(ERROR_ACCESS_DENIED))
				throw;
			File::set_attr_nt(file_path, FILE_ATTRIBUTE_NORMAL);
			File::delete_file(file_path);
		}
		RETRY_OR_IGNORE_END(ignore_errors, error_log, *this)
	}

	void delete_src_dir(const std::wstring &dir_path)
	{
		{
			DirList dir_list(dir_path);
			while (dir_list.next()) {
				std::wstring path = add_trailing_slash(dir_path) + dir_list.data().cFileName;
				update_progress(path);
				if (dir_list.data().is_dir())
					delete_src_dir(path);
				else
					delete_src_file(path);
			}
		}

		RETRY_OR_IGNORE_BEGIN
		try {
			File::remove_dir(dir_path);
		} catch (const Error &e) {
			if (e.code != HRESULT_FROM_WIN32(ERROR_ACCESS_DENIED))
				throw;
			File::set_attr_nt(dir_path, FILE_ATTRIBUTE_NORMAL);
			File::remove_dir(dir_path);
		}
		RETRY_OR_IGNORE_END(ignore_errors, error_log, *this)
	}

public:
	DeleteSrcFiles(const std::wstring &src_dir, const std::vector<std::wstring> &file_names,
			bool &ignore_errors, ErrorLog &error_log)
		: ProgressMonitor(Far::get_msg(MSG_PROGRESS_DELETE_FILES), false),
		  ignore_errors(ignore_errors),
		  error_log(error_log)
	{
		for (unsigned i = 0; i < file_names.size(); i++) {
			std::wstring file_path = add_trailing_slash(src_dir) + file_names[i];
			FindData find_data = File::get_find_data(file_path);
			if (find_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
				delete_src_dir(file_path);
			else
				delete_src_file(file_path);
		}
	}
};

template<bool UseVirtualDestructor>
void Archive<UseVirtualDestructor>::create(const std::wstring &src_dir, const std::vector<std::wstring> &file_names,
		UpdateOptions &options, std::shared_ptr<ErrorLog> error_log)
{
	DisableSleepMode dsm;
	const auto ignore_errors = std::make_shared<bool>(options.ignore_errors);

	fprintf(stderr, ">>> Archive::create():\n" );

	if (options.arc_type == c_tar && options.repack) {

		UInt32 new_index = 0, new_index2 = 0;
		bool skipped_files = false;

		if (options.enable_volumes) {
			if (extract_file_ext(options.arc_path) == L".001") {
				removeExtension(options.arc_path);
			}
		}

		UpdateOptions repack_options = options;
		const auto tar_file_index_map = std::make_shared<FileIndexMap>();
		const auto arc_file_index_map = std::make_shared<FileIndexMap>();
		HardLinkMap hlmap;

		ComObject<IOutArchive<UseVirtualDestructor>> out_tar;
		ArcAPI::create_out_archive(options.arc_type, (void **)out_tar.ref());
		set_properties(out_tar, options);

		std::wstring tar_filename = extract_file_name(options.arc_path);
		std::wstring arc_suff = ArcAPI::formats().at(options.repack_arc_type).default_extension();
		repack_options.arc_path.append(arc_suff);

		FindData src_find_data;
		memset(&src_find_data, 0, sizeof(FindData));
		memcpy(src_find_data.cFileName, tar_filename.c_str(), (tar_filename.length() + 1) * sizeof(wchar_t));

		FileIndexInfo file_index_info;
		file_index_info.rel_path = L"";
		file_index_info.find_data = src_find_data;
		FileIndexMap &_file_index_map = *arc_file_index_map;
		_file_index_map[0] = file_index_info;
		new_index2++;
		repack_options.arc_type = options.repack_arc_type;

		ComObject<IOutArchive<UseVirtualDestructor>> out_arc;
		ArcAPI::create_out_archive(repack_options.arc_type, (void **)out_arc.ref());
		set_properties(out_arc, repack_options);

		const auto progress = std::make_shared<ArchiveUpdateProgress>(true, repack_options.arc_path);
		const auto progress2 = std::make_shared<ArchiveUpdateProgress>(true, repack_options.arc_path, true);

		PrepareUpdate<UseVirtualDestructor>(src_dir, file_names, c_root_index, *this, *tar_file_index_map, hlmap, new_index, oaOverwrite,
				*ignore_errors, *error_log, options.filter.get(), skipped_files, options);

		ComObject<IArchiveUpdateCallback<UseVirtualDestructor>> tar_updater(new ArchiveUpdater<UseVirtualDestructor>(src_dir, std::wstring(), 0,
				tar_file_index_map, hlmap, options, ignore_errors, error_log, progress));

		ComObject<AcmRelayStream<UseVirtualDestructor>> mem_stream(new AcmRelayStream<UseVirtualDestructor>( (size_t)g_options.relay_buffer_size ));

		std::promise<int> promise1;
		std::future<int> future1 = promise1.get_future();
		std::promise<int> promise2;
		std::future<int> future2 = promise2.get_future();

		std::thread tar_thread([&]() {
			int errc = out_tar->UpdateItems(mem_stream, new_index, tar_updater);
			mem_stream->FinishWriting();
			promise1.set_value(errc);
		});

		ComObject<IArchiveUpdateCallback<UseVirtualDestructor>> arc_updater(new ArchiveUpdater<UseVirtualDestructor>(src_dir, std::wstring(), 0,
				arc_file_index_map, hlmap, repack_options, ignore_errors, error_log, progress2, mem_stream, true));

		UpdateStream<UseVirtualDestructor> *arc_wstream_impl;
		if (options.enable_volumes)
			arc_wstream_impl = new MultiVolumeUpdateStream<UseVirtualDestructor>(repack_options.arc_path, parse_size_string(repack_options.volume_size), progress2);
		else
			arc_wstream_impl = new SimpleUpdateStream<UseVirtualDestructor>(repack_options.arc_path, progress2);

		ComObject<IOutStream<UseVirtualDestructor>> arc_update_stream(arc_wstream_impl);

		std::thread arc_thread([&]() {
			int errc = out_arc->UpdateItems(arc_update_stream, new_index2, arc_updater);
			promise2.set_value(errc);
			mem_stream->FinishReading();
		});

		bool thread1_done = false;
		bool thread2_done = false;

		while (!thread1_done || !thread2_done) {
			Far::g_fsf.DispatchInterThreadCalls();

			if (!thread1_done && future1.wait_for(std::chrono::seconds(0)) == std::future_status::ready)
				thread1_done = true;

			if (!thread2_done && future2.wait_for(std::chrono::seconds(0)) == std::future_status::ready)
				thread2_done = true;

			std::this_thread::sleep_for(std::chrono::milliseconds(10));
		}

		tar_thread.join();
		arc_thread.join();

		int errc1 = future1.get(); // out_tar->UpdateItems
		int errc2 = future2.get(); // out_arc->UpdateItems

		try {
			COM_ERROR_CHECK(errc1);
			COM_ERROR_CHECK(errc2);
		} catch (...) {
			arc_wstream_impl->clean_files();
			throw;
		}

		return;
	}

	UInt32 new_index = 0;
	bool skipped_files = false;
	const auto file_index_map = std::make_shared<FileIndexMap>();
	HardLinkMap hlmap;

	PrepareUpdate<UseVirtualDestructor>(src_dir, file_names, c_root_index, *this, *file_index_map, hlmap, new_index, oaOverwrite,
			*ignore_errors, *error_log, options.filter.get(), skipped_files, options);

	ComObject<IOutArchive<UseVirtualDestructor>> out_arc;
	ArcAPI::create_out_archive(options.arc_type, (void **)out_arc.ref());

	set_properties(out_arc, options);

	const auto progress = std::make_shared<ArchiveUpdateProgress>(true, options.arc_path);
	ComObject<IArchiveUpdateCallback<UseVirtualDestructor>> updater(new ArchiveUpdater<UseVirtualDestructor>(src_dir, std::wstring(), 0,
			file_index_map, hlmap, options, ignore_errors, error_log, progress));

	prepare_dst_dir(extract_file_path(options.arc_path));
	UpdateStream<UseVirtualDestructor> *stream_impl;

	if (options.enable_volumes) {
		stream_impl = new MultiVolumeUpdateStream<UseVirtualDestructor>(options.arc_path, parse_size_string(options.volume_size),progress);
	}
	else if (options.create_sfx && options.arc_type == c_7z)
		stream_impl = new SfxUpdateStream<UseVirtualDestructor>(options.arc_path, options.sfx_options, progress);
	else
		stream_impl = new SimpleUpdateStream<UseVirtualDestructor>(options.arc_path, progress);

	ComObject<IOutStream<UseVirtualDestructor>> update_stream(stream_impl);

	std::promise<int> promise;
	std::future<int> future = promise.get_future();

	std::thread arc_thread([&]() {
		int errc = out_arc->UpdateItems(update_stream, new_index, updater);
		promise.set_value(errc);
	});

	bool thread_done = false;

	while (!thread_done) {
		Far::g_fsf.DispatchInterThreadCalls();

		if (!thread_done && future.wait_for(std::chrono::seconds(0)) == std::future_status::ready)
			thread_done = true;

		std::this_thread::sleep_for(std::chrono::milliseconds(10));
	}

	arc_thread.join();
	int errc1 = future.get();

	try {
		COM_ERROR_CHECK(errc1);
	} catch (...) {
		stream_impl->clean_files();
		throw;
	}

	if (options.move_files && error_log->empty() && !options.filter && !skipped_files)
		DeleteSrcFiles(src_dir, file_names, *ignore_errors, *error_log);
}

template<bool UseVirtualDestructor>
void Archive<UseVirtualDestructor>::update(const std::wstring &src_dir, const std::vector<std::wstring> &file_names,
		const std::wstring &dst_dir, const UpdateOptions &options, std::shared_ptr<ErrorLog> error_log)
{
	DisableSleepMode dsm;

	const auto ignore_errors = std::make_shared<bool>(options.ignore_errors);
	UInt32 new_index = m_num_indices;	 // starting index for new files
	bool skipped_files = false;

	const auto file_index_map = std::make_shared<FileIndexMap>();
	HardLinkMap hlmap;

	PrepareUpdate<UseVirtualDestructor>(src_dir, file_names, find_dir(dst_dir), *this, *file_index_map, hlmap, new_index,
			options.overwrite, *ignore_errors, *error_log, options.filter.get(), skipped_files, options);

	std::wstring temp_arc_name = get_temp_file_name();
	try {
		ComObject<IOutArchive<UseVirtualDestructor>> out_arc;
		CHECK_COM(in_arc->QueryInterface((REFIID)IID_IOutArchive, reinterpret_cast<void **>(&out_arc)));
		set_properties(out_arc, options);

		const auto progress = std::make_shared<ArchiveUpdateProgress>(false, arc_path);
		ComObject<IArchiveUpdateCallback<UseVirtualDestructor>> updater(new ArchiveUpdater<UseVirtualDestructor>(src_dir, dst_dir, m_num_indices,
				file_index_map, hlmap, options, ignore_errors, error_log, progress));
		ComObject<SimpleUpdateStream<UseVirtualDestructor>> update_stream(new SimpleUpdateStream<UseVirtualDestructor>(temp_arc_name, progress));

		COM_ERROR_CHECK(copy_prologue(update_stream));

		std::promise<int> promise;
		std::future<int> future = promise.get_future();

		std::thread arc_thread([&]() {
			int errc = out_arc->UpdateItems(update_stream, new_index, updater);
			promise.set_value(errc);
		});

		bool thread_done = false;
		while (!thread_done) {
			Far::g_fsf.DispatchInterThreadCalls();
			if (!thread_done && future.wait_for(std::chrono::seconds(0)) == std::future_status::ready)
				thread_done = true;
			std::this_thread::sleep_for(std::chrono::milliseconds(10));
		}

		arc_thread.join();
		int errc1 = future.get();

		COM_ERROR_CHECK(errc1);
//		COM_ERROR_CHECK(out_arc->UpdateItems(update_stream, new_index, updater));
		update_stream->copy_ctime_from(arc_path);

		close();
		update_stream.Release();
		File::move_file(temp_arc_name, arc_path, MOVEFILE_REPLACE_EXISTING);
	} catch (...) {
		File::delete_file_nt(temp_arc_name);
		throw;
	}

	reopen();

	if (options.move_files && error_log->empty() && !options.filter && !skipped_files)
		DeleteSrcFiles(src_dir, file_names, *ignore_errors, *error_log);
}

template<bool UseVirtualDestructor>
void Archive<UseVirtualDestructor>::create_dir(const std::wstring &dir_name, const std::wstring &dst_dir)
{
	DisableSleepMode dsm;

	const auto file_index_map = std::make_shared<FileIndexMap>();
	HardLinkMap hlmap;
	FileIndexInfo file_index_info{};
	file_index_info.find_data.dwFileAttributes = FILE_ATTRIBUTE_DIRECTORY;
	SYSTEMTIME sys_time;
	WINPORT_GetSystemTime(&sys_time);
	FILETIME file_time;
	WINPORT_SystemTimeToFileTime(&sys_time, &file_time);
	file_index_info.find_data.ftCreationTime = file_time;
	file_index_info.find_data.ftLastAccessTime = file_time;
	file_index_info.find_data.ftLastWriteTime = file_time;
	std::wcscpy(file_index_info.find_data.cFileName, dir_name.c_str());
	(*file_index_map)[m_num_indices] = file_index_info;

	UpdateOptions options;
	options.arc_type = arc_chain.back().type;
	load_update_props(options.arc_type);
	options.level = m_level;
	options.method = m_method;
	options.solid = m_solid;
	options.encrypt = m_encrypted;
	options.password = m_password;
	options.overwrite = oaOverwrite;

	std::wstring temp_arc_name = get_temp_file_name();
	try {
		ComObject<IOutArchive<UseVirtualDestructor>> out_arc;
		CHECK_COM(in_arc->QueryInterface((REFIID)IID_IOutArchive, reinterpret_cast<void **>(&out_arc)));

		const auto error_log = std::make_shared<ErrorLog>();
		const auto ignore_errors = std::make_shared<bool>(options.ignore_errors);

		const auto progress = std::make_shared<ArchiveUpdateProgress>(false, arc_path);
		ComObject<IArchiveUpdateCallback<UseVirtualDestructor>> updater(new ArchiveUpdater<UseVirtualDestructor>(std::wstring(), dst_dir, m_num_indices,
				file_index_map, hlmap, options, ignore_errors, error_log, progress));
		ComObject<IOutStream<UseVirtualDestructor>> update_stream(new SimpleUpdateStream<UseVirtualDestructor>(temp_arc_name, progress));

		COM_ERROR_CHECK(out_arc->UpdateItems(update_stream, m_num_indices + 1, updater));
		close();
		update_stream.Release();
		File::move_file(temp_arc_name, arc_path, MOVEFILE_REPLACE_EXISTING);
	} catch (...) {
		File::delete_file_nt(temp_arc_name);
		throw;
	}

	reopen();
}

template class Archive<true>;
template class Archive<false>;
