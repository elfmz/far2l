#include "headers.hpp"

#include "msg.hpp"
#include "utils.hpp"
#include "sysutils.hpp"
#include "farutils.hpp"
#include "common.hpp"
#include "ui.hpp"
#include "msearch.hpp"
#include "archive.hpp"
#include "options.hpp"

#include <sys/stat.h>
#include <sys/statvfs.h>
#include <fcntl.h>
#include <errno.h>

#if defined(__linux__)
#include <sys/ioctl.h>
#include <linux/fs.h>
#elif defined(__APPLE__)
#include <sys/disk.h>
#elif defined(__FreeBSD__)
#include <sys/disk.h>
#include <sys/disklabel.h>
#elif defined(__DragonFly__)
#include <sys/diskslice.h>
#elif defined(__HAIKU__)
#include <Drivers.h>
#endif

#if defined(__APPLE__) || defined(__FreeBSD__) || defined(__DragonFly__) || defined(__CYGWIN__)
#include <sys/mount.h>
#elif !defined(__HAIKU__)
#include <sys/statfs.h>
#endif

OpenOptions::OpenOptions()
	: detect(false), open_ex(true), nochain(false), open_password_len(nullptr), recursive_panel(false), delete_on_close('\0')
{}

template<bool UseVirtualDestructor>
class DataRelayStream : public IInStream<UseVirtualDestructor>,
						public ISequentialOutStream<UseVirtualDestructor>,
						public ComBase<UseVirtualDestructor> {
private:
	UInt64 file_size;
	UInt64 read_pos = 0;
	UInt64 write_pos = 0;
	uint8_t *buffer;
	size_t buffer_size;
	size_t buffer_max_size;
	size_t data_size = 0;
	bool full_size = true;
	bool writing_finished = false;
	bool reading_finished = false;
	std::mutex mtx;
	std::condition_variable cv;

public:
    DataRelayStream(size_t max_size = 64, UInt64 filesize = 0xFFFFFFFFFFFFFFFF)
	:	file_size(filesize),
		buffer_max_size(max_size << 20) {

		if (!filesize) { // unknown size ?
			file_size = 0xFFFFFFFFFFFFFFFF;
			buffer_size = buffer_max_size;
		}
		else {
			if (file_size < 32768)
				file_size = 32768;

			uintptr_t pagesize = sysconf(_SC_PAGESIZE);
			size_t aligned_size = (file_size + pagesize - 1) & ~(pagesize - 1);

			if (aligned_size > buffer_max_size)
				buffer_size = buffer_max_size;
			else
				buffer_size = aligned_size;
		}

		buffer = (unsigned char *)mmap(NULL, buffer_size, PROT_READ | PROT_WRITE,
				MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);

		if (buffer == MAP_FAILED) {
			FAIL(E_OUTOFMEMORY);
		}
	}

	~DataRelayStream() {
		if (buffer)
			munmap(buffer, buffer_size);
	}

	UNKNOWN_IMPL_BEGIN
	UNKNOWN_IMPL_ITF(IInStream)
	UNKNOWN_IMPL_ITF(ISequentialOutStream)
	UNKNOWN_IMPL_END

	STDMETHODIMP Seek(Int64 offset, UInt32 seekOrigin, UInt64 *newPosition) noexcept override {
		//fprintf(stderr, "DataRelayStream: Seek: offset %" PRId64 " from %X\n", offset, seekOrigin );

		std::unique_lock<std::mutex> lock(mtx);
		COM_ERROR_HANDLER_BEGIN

		switch (seekOrigin) {
			case STREAM_CTL_RESET: {
				/// reset stream
				read_pos = 0;
				write_pos = 0;
				data_size = 0;
				writing_finished = false;
				reading_finished = false;
				break;
			}
			case STREAM_CTL_FINISH: {
				writing_finished = true;
				cv.notify_all();
				break;
			}
			case STREAM_CTL_GETFULLSIZE: {
				*newPosition = full_size;
				break;
			}
			case STREAM_SEEK_SET: {
				if (offset < 0) {
					fprintf(stderr, "DataRelayStream: Error: neg offset %" PRId64 "\n", offset);
					return E_INVALIDARG;
				}
				if ((UInt64)offset < write_pos && (write_pos - (UInt64)offset) > buffer_size ) {
					fprintf(stderr, "DataRelayStream: Error: offset out of buffer range %" PRId64 "\n", offset);
					return E_INVALIDARG;
				}
				read_pos = offset;
				data_size = (write_pos > read_pos) ? (size_t)(write_pos - read_pos) : 0;
				cv.notify_all();

				if (newPosition) {
					*newPosition = read_pos;
				}
				break;
			}
            case STREAM_SEEK_CUR: {
				Int64 new_offset = (Int64)read_pos + offset;
				if (new_offset < 0)
					new_offset = 0;

				if ((UInt64)new_offset < write_pos && (write_pos - (UInt64)new_offset) > buffer_size ) {
					fprintf(stderr, "DataRelayStream: Error: offset out of buffer range %" PRId64 "\n", offset);
					return E_INVALIDARG;
				}

				read_pos = new_offset;
				data_size = (write_pos > read_pos) ? (size_t)(write_pos - read_pos) : 0;
				cv.notify_all();

				if (newPosition) {
					*newPosition = read_pos;
				}

				break;
			}
			case STREAM_SEEK_END: {
				Int64 new_offset = (file_size != 0xFFFFFFFFFFFFFFFF) ? (Int64)file_size + offset : file_size;

				if (file_size < buffer_size) {
					read_pos = new_offset;
					data_size = (write_pos > read_pos) ? (size_t)(write_pos - read_pos) : 0;
					cv.notify_all();
				}

				if (newPosition) {
					*newPosition = new_offset;
				}
				break;
			}
		}

		return S_OK;
		COM_ERROR_HANDLER_END
	}

	STDMETHODIMP Read(void *data, UInt32 size, UInt32 *processedSize) noexcept override {
		//fprintf(stderr, "DataRelayStream: Read: %u bytes from pos %" PRId64 "\n", size, read_pos );
		std::unique_lock<std::mutex> lock(mtx);
		COM_ERROR_HANDLER_BEGIN

		if (writing_finished && data_size == 0) {
			if (processedSize) {
				*processedSize = 0;
			}
			return S_OK;
		}

		cv.wait(lock, [this, size] {
			return (data_size >= size) || writing_finished;
		});

		//		if (reading_finished) {
		//			if (processedSize) {
		//				*processedSize = 0;
		//			}
		//			return E_ABORT;
		//      }

		if (writing_finished && data_size == 0) {
			if (processedSize) {
				*processedSize = 0;
			}
			return S_OK;
		}

		size_t available = std::min<size_t>(size, data_size);
		size_t read_ptr = (size_t)(read_pos % buffer_size);

		size_t firstChunk = std::min(available, buffer_size - read_ptr);
		memcpy(data, buffer + read_ptr, firstChunk);
		if (firstChunk < available) {
			memcpy(static_cast<BYTE*>(data) + firstChunk, buffer, available - firstChunk);
		}

		read_pos += available;
		data_size = (write_pos > read_pos) ? (write_pos - read_pos) : 0;

		if (processedSize) {
			*processedSize = static_cast<UInt32>(available);
		}
		cv.notify_all();

		return S_OK;
		COM_ERROR_HANDLER_END
	}

	int32_t Peek(void *data, UInt32 size, UInt32 *processedSize) noexcept {
		//fprintf(stderr, "DataRelayStream: Peek: %u bytes from pos %" PRId64 "\n", size, read_pos );

		std::unique_lock<std::mutex> lock(mtx);
		COM_ERROR_HANDLER_BEGIN

		if (writing_finished && data_size == 0) {
			if (processedSize) {
				*processedSize = 0;
			}
			return S_OK;
		}

		cv.wait(lock, [this, size] {
			return (data_size >= size) || writing_finished;
		});

		if (writing_finished && data_size == 0) {
			if (processedSize) {
				*processedSize = 0;
			}
			return S_OK;
		}

		size_t available = std::min<size_t>(size, data_size);
		size_t read_ptr = (size_t)(read_pos % buffer_size);

		size_t firstChunk = std::min(available, buffer_size - read_ptr);
		memcpy(data, buffer + read_ptr, firstChunk);
		if (firstChunk < available) {
			memcpy(static_cast<BYTE*>(data) + firstChunk, buffer, available - firstChunk);
		}

		if (processedSize) {
			*processedSize = static_cast<UInt32>(available);
		}
		cv.notify_all();

		return S_OK;
		COM_ERROR_HANDLER_END
	}

	STDMETHODIMP Write(const void *data, UInt32 size, UInt32 *processedSize) noexcept override {
//		fprintf(stderr, "DataRelayStream: Write: size %u, read_pos %" PRIu64 "\n", size, read_pos );

		if (buffer_size < size) {
			size = buffer_size;
		}
		if ((buffer_size - data_size) < size && buffer_size >= file_size) {
			fprintf(stderr, "DataRelayStream: Write: buffer_size overflow! \n" );
			full_size = false;
			return E_ABORT;
		}

		std::unique_lock<std::mutex> lock(mtx);
		COM_ERROR_HANDLER_BEGIN

//		fprintf(stderr, "DataRelayStream: Write: wait (buffer_size - data_size) >= size) >= %u in buffer or writing_finished || reading_finished\n", size );
		cv.wait(lock, [this, size] {
			return ((buffer_size - data_size) >= size) || writing_finished || reading_finished;
		});

		if (writing_finished) {
			if (processedSize) {
				*processedSize = 0;
			}
			full_size = false;
			return E_ABORT;
		}

		if ((buffer_size - data_size) < size && reading_finished) {
			if (processedSize) {
				*processedSize = 0;
			}
			full_size = false;
			return E_ABORT;
		}

		size_t write_ptr = (size_t)(write_pos % buffer_size);
		size_t bytesToWrite = std::min<size_t>(size, buffer_size - data_size);
		size_t firstChunk = std::min(bytesToWrite, buffer_size - write_ptr);

		memcpy(buffer + write_ptr, data, firstChunk);
		if (firstChunk < bytesToWrite)  {
			memcpy(buffer, static_cast<const BYTE*>(data) + firstChunk, bytesToWrite - firstChunk);
		}

		write_pos += bytesToWrite;
		data_size = (write_pos > read_pos) ? (size_t)(write_pos - read_pos) : 0;
		if (write_pos > buffer_size) {
			full_size = false;
		}

		if (processedSize) {
			*processedSize = size;
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

	bool GetFullSize() {
		return full_size;
	}
};

template<bool UseVirtualDestructor>
class SimpleRelExtractor : public IArchiveExtractCallback<UseVirtualDestructor>,
							public ICryptoGetTextPassword<UseVirtualDestructor>,
							public ComBase<UseVirtualDestructor>, public ProgressMonitor
{
private:
	std::shared_ptr<Archive<UseVirtualDestructor>> archive;
	UInt32 e_index;
	ComObject<ISequentialOutStream<UseVirtualDestructor>> mem_stream;
	UInt64 total_files;
	UInt64 total_bytes;
	UInt64 completed_files;
	UInt64 completed_bytes;

	void do_update_ui() override
	{
		const unsigned c_width = 60;
		std::wostringstream st;
		st << format_data_size(completed_bytes, get_size_suffixes()) << L" / "
		   << format_data_size(total_bytes, get_size_suffixes()) << L'\n';
		st << Far::get_progress_bar_str(c_width, completed_bytes, total_bytes) << L'\n';
		progress_text = st.str();
		percent_done = calc_percent(completed_bytes, total_bytes);
	}

public:
	SimpleRelExtractor(std::shared_ptr<Archive<UseVirtualDestructor>> archive, UInt32 e_index,
						ISequentialOutStream<UseVirtualDestructor> *stream = nullptr, size_t sizetotal = 0xFFFFFFFFFFFFFFFF)
	: ProgressMonitor(Far::get_msg(MSG_PROGRESS_OPEN)),
	archive(archive),
	e_index(e_index),
	mem_stream(stream),
	total_bytes(sizetotal)
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
		total_bytes = total;
		update_ui();
		return S_OK;
		COM_ERROR_HANDLER_END
	}

	STDMETHODIMP SetCompleted(const UInt64 *completeValue) noexcept override
	{
		CriticalSectionLock lock(GetSync());
		COM_ERROR_HANDLER_BEGIN
		if (completeValue)
			completed_bytes = *completeValue;
		update_ui();
		return S_OK;
		COM_ERROR_HANDLER_END
	}

	STDMETHODIMP
	GetStream(UInt32 index, ISequentialOutStream<UseVirtualDestructor> **outStream, Int32 askExtractMode) noexcept override
	{
		COM_ERROR_HANDLER_BEGIN
		*outStream = nullptr;

		if (index != e_index)
			return S_OK;

		if (mem_stream) {
			mem_stream.detach(outStream);
		}
		return S_OK;
		COM_ERROR_HANDLER_END
	}

	STDMETHODIMP PrepareOperation(Int32 askExtractMode) noexcept override
	{
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
		CriticalSectionLock lock(GetSync());
		COM_ERROR_HANDLER_BEGIN
		if (archive->m_password.empty()) {
#if 1
			ProgressSuspend ps(*this);
			if (!password_dialog(archive->m_password, archive->arc_path))
				FAIL(E_ABORT);
#else
			FAIL(E_ABORT);
#endif
		}
		BStr(archive->m_password).detach(password);
		return S_OK;
		COM_ERROR_HANDLER_END
	}
};

template<bool UseVirtualDestructor>
class ArchiveSubStream : public IInStream<UseVirtualDestructor>, private ComBase<UseVirtualDestructor>
{
private:
	ComObject<IInStream<UseVirtualDestructor>> base_stream;
	size_t start_offset;

public:
	ArchiveSubStream(IInStream<UseVirtualDestructor> *stream, size_t offset) : base_stream(stream), start_offset(offset)
	{
		base_stream->Seek(static_cast<Int64>(start_offset), STREAM_SEEK_SET, nullptr);
	}

	UNKNOWN_IMPL_BEGIN
	UNKNOWN_IMPL_ITF(ISequentialInStream)
	UNKNOWN_IMPL_ITF(IInStream)
	UNKNOWN_IMPL_END

	STDMETHODIMP Read(void *data, UInt32 size, UInt32 *processedSize) noexcept override
	{
		return base_stream->Read(data, size, processedSize);
	}

	STDMETHODIMP Seek(Int64 offset, UInt32 seekOrigin, UInt64 *newPosition) noexcept override
	{
		if (seekOrigin == STREAM_SEEK_SET)
			offset += start_offset;
		UInt64 newPos = 0;
		auto res = base_stream->Seek(offset, seekOrigin, &newPos);
		if (res == S_OK && newPosition)
			*newPosition = newPos - start_offset;
		return res;
	}
};

class BlockDeviceInfo {
public:
	uint64_t size = 0;
	unsigned sector_size = 4096;

	bool get_info(int fd) {
		bool success = false;

#if defined(__linux__)
		success = get_info_linux(fd);
#elif defined(__APPLE__)
		success = get_info_apple(fd);
#elif defined(__FreeBSD__) || defined(__DragonFly__)
		success = get_info_bsd(fd);
#elif defined(__HAIKU__)
		success = get_info_haiku(fd);
#else
		success = get_info_fallback(fd);
#endif
		if (sector_size == 0 || (sector_size & (sector_size - 1)) != 0) {
			 fprintf(stderr, "BlockDeviceInfo: Invalid sector size %u, using default 512\n", sector_size);
			 sector_size = 512;
		} else if (sector_size < 512) {
			 fprintf(stderr, "BlockDeviceInfo: Sector size %u < 512, using 512\n", sector_size);
			 sector_size = 512;
		}

		if (sector_size > 65536)
			sector_size = 65536;

		return success;
	}

private:
#if defined(__linux__)
	bool get_info_linux(int fd) {
		uint64_t blk_size;
		if (ioctl(fd, BLKGETSIZE64, &blk_size) == 0) {
			size = blk_size;
		} else {
			off_t sz = lseek(fd, 0, SEEK_END);
			if (sz != (off_t)-1) {
				size = sz;
				lseek(fd, 0, SEEK_SET);
			} else {
				return false;
			}
		}

		unsigned phys_sector_size;
		if (ioctl(fd, BLKPBSZGET, &phys_sector_size) == 0) {
			sector_size = phys_sector_size;
			// fprintf(stderr, "BLKPBSZGET: %u\n", sector_size);
		}
		else if (ioctl(fd, BLKSSZGET, &sector_size) == 0) {
			// fprintf(stderr, "BLKSSZGET: %u\n", sector_size);
		} else {
			sector_size = 4096;
		}
		return true;
	}
#endif

#if defined(__APPLE__)
	bool get_info_apple(int fd) {
		uint64_t block_count;
		uint32_t logical_block_size;

		if (ioctl(fd, DKIOCGETBLOCKCOUNT, &block_count) == 0 &&
			ioctl(fd, DKIOCGETBLOCKSIZE, &logical_block_size) == 0) {
			size = block_count * logical_block_size;
		} else {
			off_t sz = lseek(fd, 0, SEEK_END);
			if (sz != (off_t)-1) {
				size = sz;
				lseek(fd, 0, SEEK_SET);
			} else {
				return false;
			}
		}

		sector_size = 512;
		uint32_t phys_block_size;
		if (ioctl(fd, DKIOCGETPHYSICALBLOCKSIZE, &phys_block_size) == 0) {
			sector_size = phys_block_size;
			// fprintf(stderr, "DKIOCGETPHYSICALBLOCKSIZE: %u\n", sector_size);
		}
		else if (ioctl(fd, DKIOCGETBLOCKSIZE, &sector_size) == 0) {
			// fprintf(stderr, "DKIOCGETBLOCKSIZE: %u\n", sector_size);
		} else {
			 // fprintf(stderr, "DKIOC*: using default %u\n", sector_size);
		}
		return true;
	}
#endif

#if defined(__FreeBSD__) || defined(__DragonFly__)
	bool get_info_bsd(int fd) {
		off_t media_size;
		if (ioctl(fd, DIOCGMEDIASIZE, &media_size) == 0) {
			size = media_size;
		} else {
			off_t sz = lseek(fd, 0, SEEK_END);
			if (sz != (off_t)-1) {
				size = sz;
				lseek(fd, 0, SEEK_SET);
			} else {
				return false;
			}
		}

		if (ioctl(fd, DIOCGSECTORSIZE, &sector_size) != 0) {
			sector_size = 512;
		}
		return true;
	}
#endif

#if defined(__HAIKU__)
	bool get_info_haiku(int fd) {
		struct device_geometry geometry;
		if (ioctl(fd, B_GET_GEOMETRY, &geometry) == 0) {
			sector_size = geometry.bytes_per_sector;
			size = (uint64_t)geometry.bytes_per_sector *
				   geometry.sectors_per_track *
				   geometry.cylinder_count *
				   geometry.head_count;
			return true;
		}

		off_t device_size;
		if (ioctl(fd, B_GET_DEVICE_SIZE, &device_size) == 0) {
			size = device_size;
			sector_size = 512;
			return true;
		}

		off_t sz = lseek(fd, 0, SEEK_END);
		if (sz != (off_t)-1) {
			 size = sz;
			 lseek(fd, 0, SEEK_SET);
			 sector_size = 512;
			 return true;
		}
		return false;
	}
#endif

	bool get_info_fallback(int fd) {
		off_t device_size = lseek(fd, 0, SEEK_END);
		if (device_size != (off_t)-1) {
			size = device_size;
			lseek(fd, 0, SEEK_SET);
			sector_size = 512;
			return true;
		}
		return false;
	}
};

template<bool UseVirtualDestructor>
class ArchiveOpenStream : public IInStream<UseVirtualDestructor>, private ComBase<UseVirtualDestructor>, private File
{
private:
	bool device_file;
	BlockDeviceInfo bdinfo;
	uint64_t device_pos = 0;
	uint64_t device_size = 1024ULL * 1024 * 1024;
	unsigned device_sector_size = 4096;

	uint8_t *cached_header;
	uint32_t cached_size;

	uint8_t *shared_buffer = nullptr;
	size_t shared_buffer_size = 0;

	bool allocate_shared_buffer() {
		size_t requested_size = g_options.relay_buffer_size > 0 ? g_options.relay_buffer_size : (1024 * 1024);

		size_t page_size = sysconf(_SC_PAGESIZE);
		if (page_size == (size_t)-1) {
			page_size = 4096;
		}

		shared_buffer_size = (requested_size + page_size - 1) & ~(page_size - 1);
		if (shared_buffer_size < requested_size) shared_buffer_size = requested_size;

		shared_buffer = (uint8_t *)mmap(NULL, shared_buffer_size, PROT_READ | PROT_WRITE,
										MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);

		if (shared_buffer == MAP_FAILED) {
			shared_buffer = nullptr;
			shared_buffer_size = 0;
			return false;
		}

		return true;
	}

	void deallocate_shared_buffer() {
		if (shared_buffer && shared_buffer != MAP_FAILED) {
			if (munmap(shared_buffer, shared_buffer_size) != 0) {
			} else {
			}
		}
		shared_buffer = nullptr;
		shared_buffer_size = 0;
	}

public:
	ArchiveOpenStream(const std::wstring &file_path)
		: shared_buffer(nullptr), shared_buffer_size(0) // Инициализируем в списке инициализации
	{
		open(file_path, FILE_READ_DATA | FILE_READ_ATTRIBUTES,
				FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, OPEN_EXISTING,
				FILE_FLAG_SEQUENTIAL_SCAN);

		int fd = WINPORT_GetFileDescriptor(handle());
		if (fd == -1) {
			 FAIL(EBADF);
		}

		struct stat s{};
		if (sdc_fstat(fd, &s) != 0) {
			FAIL(errno);
		}

		device_file = S_ISBLK(s.st_mode);
		if (!S_ISREG(s.st_mode) && !device_file) {
			FAIL(E_ABORT);
		}
		if (S_ISREG(s.st_mode) && s.st_size < 8) {
			FAIL(E_ABORT);
		}

		if (device_file) {
			if (!allocate_shared_buffer()) {
			}

			if (bdinfo.get_info(fd)) {
				device_size = bdinfo.size;
				device_sector_size = bdinfo.sector_size;
			} else {
				 fprintf(stderr, "Failed to get device info, using defaults\n");
			}
		} else {
			device_size = s.st_size;
			fprintf(stderr, "Regular file: Size=%lu\n", device_size);
		}

		cached_header = nullptr;
		cached_size = 0;
	}

	~ArchiveOpenStream() noexcept {
		if (device_file)
			deallocate_shared_buffer();
	}

	void CacheHeader(Byte *buffer, UInt32 size)
	{
		if (!device_file) {
			cached_header = buffer;
			cached_size = size;
		}
	}

	UNKNOWN_IMPL_BEGIN
	UNKNOWN_IMPL_ITF(ISequentialInStream)
	UNKNOWN_IMPL_ITF(IInStream)
	UNKNOWN_IMPL_END

	STDMETHODIMP Read(void *data, UInt32 size, UInt32 *processedSize) noexcept override
	{
		COM_ERROR_HANDLER_BEGIN
		if (processedSize)
			*processedSize = 0;
		unsigned size_read = 0;

		if (device_file) {
			UInt64 aligned_pos = device_pos / device_sector_size * device_sector_size;
			unsigned aligned_offset = static_cast<unsigned>(device_pos - aligned_pos);
			unsigned aligned_size = aligned_offset + size;

			aligned_size = ((aligned_size + device_sector_size - 1) / device_sector_size) * device_sector_size;

			uint8_t *aligned_buffer = nullptr;
			if (shared_buffer && shared_buffer_size >= (aligned_size + device_sector_size)) {
				const uintptr_t buffer_addr = reinterpret_cast<uintptr_t>(shared_buffer);
				const size_t alignment_mask = device_sector_size - 1;
				aligned_buffer = reinterpret_cast<uint8_t*>((buffer_addr + alignment_mask) & ~alignment_mask);

				if ((aligned_buffer + aligned_size) > (shared_buffer + shared_buffer_size)) {
					aligned_buffer = nullptr;
				}
			}

			if (!aligned_buffer) {
				Buffer<unsigned char> fallback_buffer(aligned_size + device_sector_size);
				const auto buffer_addr = reinterpret_cast<uintptr_t>(fallback_buffer.data());
				const size_t alignment_mask = device_sector_size - 1;
				aligned_buffer = reinterpret_cast<unsigned char*>((buffer_addr + alignment_mask) & ~alignment_mask);

				set_pos(aligned_pos, FILE_BEGIN);
				size_t bytes_read = read(aligned_buffer, aligned_size);
				size_read = (bytes_read < aligned_offset) ? 0 : (bytes_read - aligned_offset);
				if (size_read > size) size_read = size;
				device_pos += size_read;
				memcpy(data, aligned_buffer + aligned_offset, size_read);

				if (processedSize) *processedSize = size_read;
				return S_OK;
			}

			set_pos(aligned_pos, FILE_BEGIN);
			size_t bytes_read = read(aligned_buffer, aligned_size);

			if (bytes_read < aligned_offset) {
				size_read = 0;
			} else {
				size_read = bytes_read - aligned_offset;
				if (size_read > size) size_read = size;
			}
			device_pos += size_read;
			memcpy(data, aligned_buffer + aligned_offset, size_read);

		} else {
			size_read = 0;
			if (cached_size) {
				auto cur_pos = set_pos(0, FILE_CURRENT);
				if (cur_pos < cached_size) {
					UInt32 off = static_cast<UInt32>(cur_pos);
					UInt32 siz = std::min(size, cached_size - off);
					memcpy(data, cached_header + off, siz);
					size -= (size_read = siz);
					data = static_cast<void *>(static_cast<Byte *>(data) + siz);
					set_pos(static_cast<Int64>(siz), FILE_CURRENT);
				}
			}
			if (size > 0) {
				size_read += static_cast<unsigned>(read(data, size));
			}
		}
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
		UInt64 new_position;
		if (device_file) {
			switch (seekOrigin) {
				case STREAM_SEEK_SET:
					device_pos = offset;
					break;
				case STREAM_SEEK_CUR:
					device_pos += offset;
					break;
				case STREAM_SEEK_END:
					device_pos = device_size + offset;
					break;
				default:
					FAIL(E_INVALIDARG);
			}
			new_position = device_pos;
		} else {
			DWORD method;
			switch (seekOrigin) {
				case STREAM_SEEK_SET:
					method = FILE_BEGIN;
					break;
				case STREAM_SEEK_CUR:
					method = FILE_CURRENT;
					break;
				case STREAM_SEEK_END:
					method = FILE_END;
					break;
				default: {
					FAIL(E_INVALIDARG);
				}
			}
			new_position = set_pos(offset, method);
		}
		if (newPosition)
			*newPosition = new_position;
		return S_OK;
		COM_ERROR_HANDLER_END
	}

	FindData get_info()
	{
		FindData file_info{};
		std::wstring file_name = extract_file_name(path());
		if (file_name.empty())
			file_name = path();
		std::wcscpy(file_info.cFileName, file_name.c_str());
		BY_HANDLE_FILE_INFORMATION fi;
		if (get_info_nt(fi)) {
			file_info.dwFileAttributes = fi.dwFileAttributes;
			file_info.ftCreationTime = fi.ftCreationTime;
			file_info.ftLastAccessTime = fi.ftLastAccessTime;
			file_info.ftLastWriteTime = fi.ftLastWriteTime;
			file_info.nFileSize = fi.nFileSize;
		}
		return file_info;
	}
};


template<bool UseVirtualDestructor>
class ArchiveOpener
	: public IArchiveOpenCallback<UseVirtualDestructor>,
	  public IArchiveOpenVolumeCallback<UseVirtualDestructor>,
	  public ICryptoGetTextPassword<UseVirtualDestructor>,
	  public ComBase<UseVirtualDestructor>,
	  public ProgressMonitor
{
private:
	std::shared_ptr<Archive<UseVirtualDestructor>> archive;
	FindData volume_file_info;

	UInt64 total_files;
	UInt64 total_bytes;
	UInt64 completed_files;
	UInt64 completed_bytes;
	bool bShowProgress;

	void do_update_ui() override
	{
		const unsigned c_width = 60;
		std::wostringstream st;
		st << fit_str(volume_file_info.cFileName, c_width) << L'\n';
		st << completed_files << L" / " << total_files << L'\n';
		st << Far::get_progress_bar_str(c_width, completed_files, total_files) << L'\n';
		st << L"\x01\n";
		st << format_data_size(completed_bytes, get_size_suffixes()) << L" / "
		   << format_data_size(total_bytes, get_size_suffixes()) << L'\n';
		st << Far::get_progress_bar_str(c_width, completed_bytes, total_bytes) << L'\n';
		progress_text = st.str();

		if (total_files)
			percent_done = calc_percent(completed_files, total_files);
		else
			percent_done = calc_percent(completed_bytes, total_bytes);
	}

public:
	ArchiveOpener(std::shared_ptr<Archive<UseVirtualDestructor>> archive, bool bShowProgress = true)
		: ProgressMonitor(Far::get_msg(MSG_PROGRESS_OPEN)),
		  archive(archive),
		  volume_file_info(archive->arc_info),
		  total_files(0),
		  total_bytes(0),
		  completed_files(0),
		  completed_bytes(0),
		  bShowProgress(bShowProgress)
	{
	}

	UNKNOWN_IMPL_BEGIN
	UNKNOWN_IMPL_ITF(IArchiveOpenCallback)
	UNKNOWN_IMPL_ITF(IArchiveOpenVolumeCallback)
	UNKNOWN_IMPL_ITF(ICryptoGetTextPassword)
	UNKNOWN_IMPL_END

	STDMETHODIMP SetTotal(const UInt64 *files, const UInt64 *bytes) noexcept override
	{
		CriticalSectionLock lock(GetSync());
		COM_ERROR_HANDLER_BEGIN
		if (files)
			total_files = *files;
		if (bytes)
			total_bytes = *bytes;

		if (bShowProgress)
			update_ui();

		return S_OK;
		COM_ERROR_HANDLER_END
	}

	STDMETHODIMP SetCompleted(const UInt64 *files, const UInt64 *bytes) noexcept override
	{
		CriticalSectionLock lock(GetSync());
		COM_ERROR_HANDLER_BEGIN
		if (files)
			completed_files = *files;
		if (bytes)
			completed_bytes = *bytes;

		if (bShowProgress)
			update_ui();

		return S_OK;
		COM_ERROR_HANDLER_END
	}

	STDMETHODIMP GetProperty(PROPID propID, PROPVARIANT *value) noexcept override
	{
		COM_ERROR_HANDLER_BEGIN
		PropVariant prop;
		switch (propID) {
			case kpidName:
				prop = volume_file_info.cFileName;
				break;
			case kpidIsDir:
				prop = volume_file_info.is_dir();
				break;
			case kpidSize:
				prop = volume_file_info.size();
				break;
			case kpidAttrib:
				prop = static_cast<UInt32>(volume_file_info.dwFileAttributes);
				break;
			case kpidCTime:
				prop = volume_file_info.ftCreationTime;
				break;
			case kpidATime:
				prop = volume_file_info.ftLastAccessTime;
				break;
			case kpidMTime:
				prop = volume_file_info.ftLastWriteTime;
				break;
			case kpidChangeTime:
//				prop = volume_file_info.ftChangeTime;
				break;
		}
		prop.detach(value);
		return S_OK;
		COM_ERROR_HANDLER_END
	}

	STDMETHODIMP GetStream(const wchar_t *name, IInStream<UseVirtualDestructor> **inStream) noexcept override
	{
		COM_ERROR_HANDLER_BEGIN
		std::wstring file_path = add_trailing_slash(archive->arc_dir()) + name;
		FindData find_data;
		if (!File::get_find_data_nt(file_path, find_data))
			return S_FALSE;
		if (find_data.is_dir())
			return S_FALSE;
		archive->volume_names.insert(name);
		volume_file_info = find_data;
		ComObject<IInStream<UseVirtualDestructor>> file_stream(new ArchiveOpenStream<UseVirtualDestructor>(file_path));
		file_stream.detach(inStream);
		//	CriticalSectionLock lock(GetSync());

		if (bShowProgress)
			update_ui();

		return S_OK;
		COM_ERROR_HANDLER_END
	}

	STDMETHODIMP CryptoGetTextPassword(BSTR *password) noexcept override
	{
		COM_ERROR_HANDLER_BEGIN
		if (archive->m_password.empty()) {
			if (archive->m_open_password == -'A') {	 // open from AnalyzeW
				FAIL(E_PENDING);
			}
			ProgressSuspend ps(*this);
			if (!password_dialog(archive->m_password, archive->arc_path)) {
				archive->m_open_password = -3;
				FAIL(E_ABORT);
			} else {
				archive->m_open_password = static_cast<int>(archive->m_password.size());
			}
		}
		BStr(archive->m_password).detach(password);
		return S_OK;
		COM_ERROR_HANDLER_END
	}
};

template<bool UseVirtualDestructor>
bool Archive<UseVirtualDestructor>::get_stream(UInt32 index, IInStream<UseVirtualDestructor> **stream)
{
	UInt32 num_indices = 0;

	if (in_arc->GetNumberOfItems(&num_indices) != S_OK) {
		return false;
	}

	if (index >= num_indices) {
		return false;
	}

	ComObject<IInArchiveGetStream<UseVirtualDestructor>> get_stream;
	if (in_arc->QueryInterface((REFIID)IID_IInArchiveGetStream, reinterpret_cast<void **>(&get_stream))
					!= S_OK
			|| !get_stream) {
		return false;
	}

	ComObject<ISequentialInStream<UseVirtualDestructor>> seq_stream;
	if (get_stream->GetStream(index, seq_stream.ref()) != S_OK || !seq_stream) {
		return false;
	}

	if (seq_stream->QueryInterface((REFIID)IID_IInStream, reinterpret_cast<void **>(stream)) != S_OK
			|| !stream) {
		return false;
	}

	return true;
}

template<bool UseVirtualDestructor>
HRESULT Archive<UseVirtualDestructor>::copy_prologue(IOutStream<UseVirtualDestructor> *out_stream)
{
	auto prologue_size = arc_chain.back().sig_pos;
	if (prologue_size <= 0)
		return S_OK;

	if (!base_stream)
		return E_FAIL;

	auto res = base_stream->Seek(0, STREAM_SEEK_SET, nullptr);
	if (res != S_OK)
		return res;

	while (prologue_size > 0) {
		char buf[16 * 1024];
		UInt32 nr = 0, nw = 0, nb = static_cast<UInt32>(sizeof(buf));
		if (prologue_size < nb)
			nb = static_cast<UInt32>(prologue_size);
		res = base_stream->Read(buf, nb, &nr);
		if (res != S_OK || nr == 0)
			break;
		res = out_stream->Write(buf, nr, &nw);
		if (res != S_OK || nr != nw)
			break;
		prologue_size -= nr;
	}

	if (res == S_OK && prologue_size > 0)
		res = E_FAIL;
	return res;
}

template<bool UseVirtualDestructor>
bool Archive<UseVirtualDestructor>::open(IInStream<UseVirtualDestructor> *stream, const ArcType &type, const bool allow_tail, const bool show_progress)
{
	ArcAPI::create_in_archive(type, (void **)in_arc.ref());
	ComObject<IArchiveOpenCallback<UseVirtualDestructor>> opener(new ArchiveOpener<UseVirtualDestructor>(this->shared_from_this(), show_progress));

//	allow_tail = true;

//	if (allow_tail && ArcAPI::formats().at(type).Flags_PreArc()) {
	if (allow_tail && ArcAPI::formats().at(type).Flags_PreArc()) {
		ComObject<IArchiveAllowTail<UseVirtualDestructor>> allowTail;
		in_arc->QueryInterface((REFIID)IID_IArchiveAllowTail, (void **)&allowTail);

		if (allowTail)
			allowTail->AllowTail(TRUE);
	}

	const UInt64 max_check_start_position = 0;

	auto res = in_arc->Open(stream, &max_check_start_position, opener);
//	auto res = in_arc->Open(nullptr, &max_check_start_position, opener);
//  auto res = HRESULT_FROM_WIN32(ERROR_INVALID_DATA);

	if (res == HRESULT_FROM_WIN32(ERROR_INVALID_DATA))	  // unfriendly eDecode
		res = S_FALSE;

	COM_ERROR_CHECK(res);
	return res == S_OK;
}

static void prioritize(std::list<ArcEntry> &arc_entries, const ArcType &first, const ArcType &second)
{
	std::list<ArcEntry>::iterator iter = arc_entries.end();
	for (std::list<ArcEntry>::iterator arc_entry = arc_entries.begin(); arc_entry != arc_entries.end();
			++arc_entry) {
		if (arc_entry->type == second) {
			iter = arc_entry;
		} else if (arc_entry->type == first) {
			if (iter != arc_entries.end()) {
				arc_entries.insert(iter, *arc_entry);
				arc_entries.erase(arc_entry);
			}
			break;
		}
	}
}

static void prioritize_before_any(std::list<ArcEntry>& arc_entries, const ArcType& to_move, std::initializer_list<ArcType> targets) {
	std::vector<ArcEntry> entries_to_move;
	for (auto it = arc_entries.begin(); it != arc_entries.end();) {
		if (it->type == to_move) {
			entries_to_move.push_back(std::move(*it));
			it = arc_entries.erase(it);
		} else {
			++it;
		}
	}

	if (entries_to_move.empty()) {
		return;
	}

	auto insert_pos = arc_entries.end();
	for (const auto& target : targets) {
		for (auto pos = arc_entries.begin(); pos != arc_entries.end(); ++pos) {
			if (pos->type == target) {
				if (insert_pos == arc_entries.end() ||
					std::distance(arc_entries.begin(), pos) < std::distance(arc_entries.begin(), insert_pos)) {
					insert_pos = pos;
				}
				break;
			}
		}
	}

	arc_entries.insert(insert_pos, 
					   std::make_move_iterator(entries_to_move.begin()),
					   std::make_move_iterator(entries_to_move.end()));
}

//??? filter multi-volume .zip archives to avoid wrong opening.
//??? if someone can find better solution - welcome...
//
static Byte zip_LOCAL_sig[] = {0x50, 0x4B, 0x03, 0x04};
// static const std::string_view zip_EOCD_sig = "\x50\x4B\x05\x06"sv;
static const std::string zip_EOCD_sig = "\x50\x4B\x05\x06";
static const UInt32 check_size = 16 * 1024, min_LOCAL = 30, min_EOCD = 22;
//
template<bool UseVirtualDestructor>
static bool accepted_signature(size_t pos, const SigData &sig, const Byte *buffer, size_t size,
		IInStream<UseVirtualDestructor> *stream, int eof_i)
{
	if (!pos || sig.signature.size() != sizeof(zip_LOCAL_sig)
			|| !std::equal(zip_LOCAL_sig, zip_LOCAL_sig + sizeof(zip_LOCAL_sig), buffer + pos))
		return true;
	if (eof_i < 0)
		return false;

	std::unique_ptr<Byte[]> buf;
	//  std::string_view tail;
	std::string tail;
	if (eof_i) {
		pos += min_LOCAL;
		size -= (min_EOCD - zip_EOCD_sig.size());
		if (pos >= size)
			return true;
		if (pos + check_size < size)
			pos = size - check_size;
		tail = {(const char *)buffer + pos, size - pos};
	} else {
		//	buf = std::make_unique<Byte[]>(check_size);
		buf.reset(new Byte[check_size]);
		pos = 0;
		buffer = buf.get();
		UInt64 cur_pos;
		if (S_OK != stream->Seek(0, STREAM_SEEK_CUR, &cur_pos))
			return false;
		if (S_OK == stream->Seek(-(Int64)check_size, STREAM_SEEK_END, nullptr)) {
			UInt32 nr;
			if (S_OK == stream->Read(buf.get(), check_size, &nr) && nr == check_size)
				tail = {(const char *)buffer, check_size - min_EOCD + zip_EOCD_sig.size()};
		}
		stream->Seek((Int64)cur_pos, STREAM_SEEK_SET, nullptr);
	}
	if (!tail.empty()) {
		auto eocd = tail.rfind(zip_EOCD_sig);
		if (eocd != std::string::npos) {
			pos += eocd + zip_EOCD_sig.size();
			if (buffer[pos] != 0 || buffer[pos + 1] != 0)	 // This disk (aka Volume) number
				return false;
		}
	}
	return true;
}

#include "format_priority.hpp"

template<bool UseVirtualDestructor>
ArcEntries Archive<UseVirtualDestructor>::detect(Byte *buffer, UInt32 size, bool eof, const std::wstring &file_ext,
		const ArcTypes &arc_types, IInStream<UseVirtualDestructor> *stream)
{
	ArcEntries arc_entries;
	std::set<ArcType> found_types;

	// 1. find formats by signature
	//
	std::vector<SigData> signatures;
	signatures.reserve(2 * arc_types.size());
	std::for_each(arc_types.begin(), arc_types.end(), [&](const ArcType &arc_type) {
		const auto &format = ArcAPI::formats().at(arc_type);
		if (format.ClassID != c_dmg) {
			std::for_each(format.Signatures.begin(), format.Signatures.end(),
					[&](const ByteVector &signature) {
						signatures.emplace_back(SigData(signature, format));
					});
		}
	});
	std::vector<StrPos> sig_positions = msearch(buffer, size, signatures, eof);

	int eof_i = eof ? 1 : 0;
	std::for_each(sig_positions.begin(), sig_positions.end(), [&](const StrPos &sig_pos) {
		const auto &signature = signatures[sig_pos.idx];
		const auto &format = signature.format;
		if (accepted_signature(sig_pos.pos, signature, buffer, size, stream, eof_i)) {
			found_types.insert(format.ClassID);
			arc_entries.emplace_back(format.ClassID, sig_pos.pos - format.SignatureOffset);
		} else {
			eof_i = -1;
		}
	});

	arc_entries.sort([&](const ArcEntry &a, const ArcEntry &b) {
		int prio_a = g_format_priority.count(a.type) ? g_format_priority.at(a.type) : 100;
		int prio_b = g_format_priority.count(b.type) ? g_format_priority.at(b.type) : 100;
		return prio_a < prio_b;
	});

	// 2. find formats by file extension
	//
	ArcTypes types_by_ext = ArcAPI::formats().find_by_ext(file_ext);
	std::for_each(types_by_ext.begin(), types_by_ext.end(), [&](const ArcType &arc_type) {
		if (found_types.count(arc_type) == 0
				&& std::find(arc_types.begin(), arc_types.end(), arc_type) != arc_types.end()) {
			found_types.insert(arc_type);
			arc_entries.emplace_front(arc_type, 0);
		}
	});

	// 3. all other formats
	//
	std::for_each(arc_types.begin(), arc_types.end(), [&](const ArcType &arc_type) {
		if (found_types.count(arc_type) == 0) {
			const auto &format = ArcAPI::formats().at(arc_type);
			if (!format.Flags_ByExtOnlyOpen())
				arc_entries.emplace_back(arc_type, 0);
		}
	});

#if 0
	// special case: Dmg must go before HFS
	prioritize(arc_entries, c_dmg, c_hfs);

	// special case: UDF must go before ISO
	prioritize(arc_entries, c_udf, c_iso);

	// special case: Rar must go before Split
	prioritize(arc_entries, c_rar, c_split);

//	prioritize_before_any(arc_entries, c_sqfs, {c_xz, c_zstd, c_gzip, c_bzip2, c_lzma }); // c_lz4, c_lzo, с_lzma86 ??
#endif

	return arc_entries;
}

template<bool UseVirtualDestructor>
UInt64 Archive<UseVirtualDestructor>::get_physize()
{
	UInt64 physize = 0;
	PropVariant prop;
	auto res = in_arc->GetArchiveProperty(kpidPhySize, prop.ref());
	if (res == S_OK && prop.is_uint())
		physize = prop.get_uint();
	return physize;
}

template<bool UseVirtualDestructor>
UInt64 Archive<UseVirtualDestructor>::archive_filesize()
{
	auto arc_size = arc_info.size();
#if 0
  for (const auto& volume_name : volume_names) {
	auto volume_path = add_trailing_slash(arc_dir()) + volume_name;
	FindData find_data;
	if (File::get_find_data_nt(volume_path, find_data))
	  arc_size += find_data.size();
  }
#endif
	return arc_size;
}

template<bool UseVirtualDestructor>
UInt64 Archive<UseVirtualDestructor>::get_skip_header(IInStream<UseVirtualDestructor> *stream, const ArcType &type)
{
	if (ArcAPI::formats().at(type).Flags_PreArc()) {
		ComObject<IArchiveAllowTail<UseVirtualDestructor>> allowTail;
		in_arc->QueryInterface((REFIID)IID_IArchiveAllowTail, (void **)&allowTail);
		if (allowTail)
			allowTail->AllowTail(TRUE);
	}

	auto res = stream->Seek(0, STREAM_SEEK_SET, nullptr);
	if (S_OK == res) {
		const UInt64 max_check_start_position = ArchiveGlobals::max_check_size;
		res = in_arc->Open(stream, &max_check_start_position, nullptr);
		if (res == S_OK) {
			auto physize = get_physize();
			if (physize < arc_info.size())
				return physize;
		}
	}
	return 0;
}

template<bool UseVirtualDestructor>
void Archive<UseVirtualDestructor>::open(const OpenOptions &options, Archives<UseVirtualDestructor> &archives, UInt32 entry_index)
{
	fprintf(stderr,"Archive::open(): > entry_index = %u\n", entry_index);

	size_t parent_idx = -1;
	if (!archives.empty())
		parent_idx = archives.size() - 1;

	ArchiveOpenStream<UseVirtualDestructor> *stream_impl = nullptr;
	ComObject<IInStream<UseVirtualDestructor>> stream;
	FindData arc_info{};

	if (parent_idx == (size_t)-1) {
		fprintf(stderr,"Opening root archive from file path %ls\n", options.arc_path.c_str());
		stream_impl = new ArchiveOpenStream<UseVirtualDestructor>(options.arc_path);
		stream = stream_impl;
		fprintf(stderr, "Stream get info\n" );
		arc_info = stream_impl->get_info();
		fprintf(stderr,"Root archive info: name='%ls', size=%" PRIu64 "\n", arc_info.cFileName, arc_info.size());
	} else {
		if (parent_idx > 0 && archives[parent_idx]->flags == 2) {
			fprintf(stderr,"Parent is a sequential stream (partial seek) - STOPPING\n");
			return;
		}
		const ArcType &pArcType = archives[parent_idx]->arc_chain.back().type;
		UInt32 main_file_index = 0, num_indices = 0;
		archives[parent_idx]->in_arc->GetNumberOfItems(&num_indices);
		if (!num_indices) {
			fprintf(stderr,"Parent archive has no items - STOPPING\n");
			return;
		}
		if (entry_index >= num_indices && entry_index != 0xFFFFFFFF) {
			fprintf(stderr,"Provided entry_index (%u) out of range, setting to 0xFFFFFFFF\n", entry_index);
			entry_index = 0xFFFFFFFF;
		}
		if (archives[parent_idx]->get_main_file(main_file_index)) {
			fprintf(stderr, "Parent archive reports main file index: %u\n", main_file_index);
			if (entry_index == 0xFFFFFFFF) {
				fprintf(stderr, "Using main file index as entry_index\n");
				entry_index = main_file_index;
			}
		}
		if (entry_index == 0xFFFFFFFF && !ArcAPI::is_single_file_format(pArcType)) {
#if 0
			if (num_indices > 1) {
				//fprintf(stderr,"Archive::open(): no entry_index & nultifile format - STOP!\n");
				return;
			}
#else
			fprintf(stderr, "No entry_index & multi-file format - STOPPING\n");
			return;
#endif
		}
		if (entry_index == 0xFFFFFFFF) {
			fprintf(stderr, "entry_index still unset, defaulting to 0\n");
			entry_index = 0;
		}

		fprintf(stderr, "Attempting to get stream for entry_index: %u\n", entry_index);
		bool bInArcStream = archives[parent_idx]->get_stream(entry_index, stream.ref());

		if (!bInArcStream) {
			fprintf(stderr, "Failed to get direct stream, falling back to extraction method\n");

			UInt32 indices[2] = { entry_index, 0 };
			FindData ef = archives[parent_idx]->get_file_info(entry_index);
			fprintf(stderr, "Entry file info: name='%ls', size=%" PRIu64 ", is_dir=%s\n", ef.cFileName, ef.size(), ef.is_dir() ? "true" : "false");
			if (ef.is_dir()) {
				fprintf(stderr, "Entry is a directory - STOPPING\n");
				return;
			}

			size_t fsize = ef.size();
			if (pArcType == c_gzip) { // Do not rely on gzip size data.
				fsize = 0;
			}
			fprintf(stderr,"entry filename = %ls\n" , ef.cFileName);
			bool exFullSize = (fsize && fsize <= ((size_t)g_options.max_arc_cache_size) << 20);
			fprintf(stderr,"fsize = %zu, g_options.max_arc_cache_size = %zu MB, exFullSize = %s\n",
						  fsize, (size_t)g_options.max_arc_cache_size, exFullSize ? "true" : "false");

			ComObject<DataRelayStream<UseVirtualDestructor>> mem_stream(new DataRelayStream<UseVirtualDestructor>(
						(size_t)g_options.max_arc_cache_size, fsize));

			//ComObject<DataRelayStream<UseVirtualDestructor>> mem_stream(new DataRelayStream<UseVirtualDestructor>(4, 8, fsize));
			ComObject<IArchiveExtractCallback<UseVirtualDestructor>> extractor(new SimpleRelExtractor<UseVirtualDestructor>(archives[parent_idx], entry_index, mem_stream, fsize));

			const auto archive = std::make_shared<Archive<UseVirtualDestructor>>();
			if (options.open_password_len && *options.open_password_len == -'A')
				archive->m_open_password = *options.open_password_len;

			//		archive->flags = 2 + bFullSize;
			archive->arc_path = options.arc_path;
//			archive->arc_path = extract_file_name(ef.cFileName);
			archive->arc_info = arc_info;
			archive->m_password = options.password;
			archives[parent_idx]->m_chain_file_index = entry_index;
			archive->parent = archives[parent_idx];
			archive->volume_names = archives[parent_idx]->volume_names;
			stream = mem_stream;

			std::promise<int> promise;
			std::future<int> future = promise.get_future();
			std::promise<int> promise2;
			std::future<int> future2 = promise2.get_future();

			std::thread extract_thread([&]() {
				fprintf(stderr,"Starting Extraction Thread...\n");
				int errc = archives[parent_idx]->in_arc->Extract(indices, 1, 0, extractor);
				promise.set_value(errc);
				fprintf(stderr,"Extraction finished with code: 0x%X\n", errc);
			});

			if (!exFullSize) {
				fprintf(stderr,"NOT exFullSize: Using streaming detection/open method\n");

				ArcEntry arc_entry(c_tar, 0, 2);
				std::thread open_thread([&]() {
					fprintf(stderr,"Starting Streaming Detection/Open Thread...\n");

					uint8_t buffer[32768];
					UInt32 size = 0;
					fprintf(stderr,"Peeking 32KB from extracted stream...\n");
					int32_t hr = mem_stream->Peek(buffer, sizeof(buffer), &size);
					if (!size || hr != S_OK) {
						fprintf(stderr,"Peek failed or no data: size=%u, hr=0x%X\n", size, hr);
						promise2.set_value(hr);
						return;
					}

					ArcTypes e_arc_types = {c_tar, c_xar, c_ar, c_cpio};
					ArcEntries arc_entries = detect(buffer, size, true,
												extract_file_ext(ef.cFileName), e_arc_types, stream);
					if (arc_entries.empty()) {
						fprintf(stderr,"arc_entries.empty()\n" );
						promise2.set_value(hr);
						return;
					}

					arc_entry = arc_entries.front();
					ArcAPI::create_in_archive(arc_entry.type, (void **)archive->in_arc.ref());
					ComObject<IArchiveOpenCallback<UseVirtualDestructor>> opener(new ArchiveOpener<UseVirtualDestructor>(archive, false));
					const UInt64 max_check_start_position = 0;
					int errc = archive->in_arc->Open(stream, &max_check_start_position, opener);
					promise2.set_value(errc);
					fprintf(stderr,"archive->in_arc->Open() finished with code: 0x%X\n", errc);
				});

				bool thread1_done = false;
				bool thread2_done = false;
				int errc1 = 0;// = future.get(); // extract
				int errc2 = 0;// = future2.get(); // open

				while (!thread1_done || !thread2_done) {
					Far::g_fsf.DispatchInterThreadCalls();

					if (!thread1_done && future.wait_for(std::chrono::seconds(0)) == std::future_status::ready) { // extract
						thread1_done = true;
						mem_stream->FinishWriting();
						errc1 = future.get();
					}

					if (!thread2_done && future2.wait_for(std::chrono::seconds(0)) == std::future_status::ready) { // open
						errc2 = future2.get(); // open
						thread2_done = true;
						HRESULT res = errc2;
						if (res == HRESULT_FROM_WIN32(ERROR_INVALID_DATA))	  // unfriendly eDecode
							res = S_FALSE;
						if (FAILED(res))
							mem_stream->FinishWriting();
						else
							mem_stream->FinishReading();
					}
					std::this_thread::sleep_for(std::chrono::milliseconds(10));
				}

				fprintf(stderr,"joininbg threads......\n" );

				open_thread.join();
				extract_thread.join();

				HRESULT res = errc2;
				if (res == HRESULT_FROM_WIN32(ERROR_INVALID_DATA))	  // unfriendly eDecode
					res = S_FALSE;

				fprintf(stderr,"ex res = %i S_OK = %i\n", res, S_OK );
				COM_ERROR_CHECK(res);
				fprintf(stderr,"ex errc1 = %i S_OK = %i archive->in_arc = %p\n", errc1, S_OK, (void *)archive->in_arc );
				(void)errc1;
				bool opened = (res == S_OK);

				//if (opened) {
				if (opened && archive->is_open()) {
				//if (archive->is_open()) {
					bool bFullSize = mem_stream->GetFullSize();
					arc_entry.flags = 2 + bFullSize;
					archive->ex_stream = stream;
					archive->ex_out_stream = mem_stream;
					archive->flags = 2 + bFullSize;
					archive->arc_chain.assign(archives[parent_idx]->arc_chain.begin(), archives[parent_idx]->arc_chain.end());
					archive->arc_chain.push_back(arc_entry);
					archives.push_back(archive);
				}
				return;
			} // !exFullSize
			else {
				fprintf(stderr,"exFullSize = true ?\n" );
				bool thread1_done = false;
				int errc1 = 0;// = future.get(); // extract

				fprintf(stderr,"wait loop..\n" );

				while (!thread1_done) {
					Far::g_fsf.DispatchInterThreadCalls();
					if (!thread1_done && future.wait_for(std::chrono::seconds(0)) == std::future_status::ready) { // extract
						thread1_done = true;
						mem_stream->FinishWriting();
						errc1 = future.get();
					}
					std::this_thread::sleep_for(std::chrono::milliseconds(10));
				}
				extract_thread.join();
				HRESULT res = errc1;
				COM_ERROR_CHECK(res);
			}
		}
	}

	Buffer<unsigned char> buffer(ArchiveGlobals::max_check_size);
	UInt32 size;
	CHECK_COM(stream->Read(buffer.data(), static_cast<UInt32>(buffer.size()), &size));

	if (stream_impl) {
		stream_impl->CacheHeader(buffer.data(), size);
	}

	UInt64 skip_header = 0;
	bool first_open = true;
	ArcEntries arc_entries = detect(buffer.data(), size, size < ArchiveGlobals::max_check_size,
			extract_file_ext(arc_info.cFileName), options.arc_types, stream);

#if 0
	fprintf(stderr, "Total arc_entries list: %lu\n", static_cast<unsigned long>(arc_entries.size()));
	unsigned int entry_index = 0;
	for (const ArcEntry& entry : arc_entries) {
		entry_index++;
		const auto& format_info = ArcAPI::formats().at(entry.type);
		fprintf(stderr, "  [%u] ArcEntry:\n", entry_index);
		fprintf(stderr, "	  Type (GUID): "); // GUID
		for (size_t i = 0; i < entry.type.size(); ++i) {
			 fprintf(stderr, "%02X", static_cast<unsigned char>(entry.type[i]));
		}
		fprintf(stderr, "\n");
		fprintf(stderr, "	  Signature Position: %zu\n", entry.sig_pos);
		fprintf(stderr, "	  Flags: %zu\n", entry.flags);

		fprintf(stderr, "	  Format Name: %ls\n", format_info.name.c_str());
		fprintf(stderr, "	  Default Extension: %ls\n", format_info.default_extension().c_str());

		if (!format_info.Signatures.empty()) {
			fprintf(stderr, "	  Signatures:\n");
			for(const auto& sig_data : format_info.Signatures) {
//				fprintf(stderr, "		- Offset: %d, Signature: ", sig_data.offset);
				// sig hex
				for (size_t i = 0; i < sig_data.size(); ++i) {
					fprintf(stderr, "%02X ", static_cast<unsigned char>(sig_data[i]));
				}
				fprintf(stderr, "\n");
			}
		} else {
			 fprintf(stderr, "	  Signatures: (none listed)\n");
		}
		fprintf(stderr, "  --------------------------------------------------------\n");
	}
	fprintf(stderr, "========== End of arc_entries list ==========\n");
#endif

	for (ArcEntries::const_iterator arc_entry = arc_entries.cbegin(); arc_entry != arc_entries.cend();
			++arc_entry) {

		const auto archive = std::make_shared<Archive>();
		if (options.open_password_len && *options.open_password_len == -'A')
			archive->m_open_password = *options.open_password_len;
		archive->arc_path = options.arc_path;
		archive->arc_info = arc_info;
		archive->m_password = options.password;
		if (parent_idx != (size_t)-1) {
			archive->volume_names = archives[parent_idx]->volume_names;
		}

		const ArcFormat &format = ArcAPI::formats().at(arc_entry->type);

		bool opened = false, have_tail = false;
//		fprintf(stderr,"Seeking to signature position: %zu\n", arc_entry->sig_pos);
		CHECK_COM(stream->Seek(arc_entry->sig_pos, STREAM_SEEK_SET, nullptr));

//		fprintf(stderr,"Attempting to open at sig_pos=%zu as format '%ls'\n", arc_entry->sig_pos, format.name.c_str());

		if (!arc_entry->sig_pos) {
			opened = archive->open(stream, arc_entry->type);
			if (archive->m_open_password && options.open_password_len)
				*options.open_password_len = archive->m_open_password;
			if (!opened && first_open) {
				if (format.Flags_PreArc()) {
					stream->Seek(0, STREAM_SEEK_SET, nullptr);
					opened = have_tail = archive->open(stream, arc_entry->type, true);
				}
				if (!opened) {
					auto next_entry = arc_entry;
					++next_entry;
					if (next_entry != arc_entries.cend() && next_entry->sig_pos > 0) {
						skip_header = archive->get_skip_header(stream, arc_entry->type);
					}
				}
			}
		} else if (arc_entry->sig_pos >= skip_header) {
			archive->arc_info.set_size(arc_info.size() - arc_entry->sig_pos);
			ComObject<IInStream<UseVirtualDestructor>> substream(new ArchiveSubStream<UseVirtualDestructor>(stream, arc_entry->sig_pos));
			opened = archive->open(substream, arc_entry->type);
			if (archive->m_open_password && options.open_password_len)
				*options.open_password_len = archive->m_open_password;
			if (opened)
				archive->base_stream = stream;
		}

		if (opened) {
			fprintf(stderr,"SUCCESSFULLY OPENED as format: %ls\n", format.name.c_str());
			if (parent_idx != (size_t)-1) {
				archives[parent_idx]->m_chain_file_index = entry_index;
				archive->parent = archives[parent_idx];
//				FindData ef = archives[parent_idx]->get_file_info(entry_index);
//				archive->arc_path = extract_file_name(ef.cFileName);
				archive->arc_path = options.arc_path;
				archive->arc_chain.assign(archives[parent_idx]->arc_chain.begin(),
						archives[parent_idx]->arc_chain.end());
			}
			archive->arc_chain.push_back(*arc_entry);
			archives.push_back(archive);

			if (!options.nochain) {
				open(options, archives);
			}
			if (!options.detect && !have_tail) {
				break;
			}

			skip_header = arc_entry->sig_pos + std::min(archive->arc_info.size(), archive->get_physize());
		}
		else {
			fprintf(stderr,"OPEN FAILED as %ls !\n", format.name.c_str());
		}

		first_open = false;
	}

	fprintf(stderr,"========== archives size = %lu\n", archives.size());
	if (stream_impl)
		stream_impl->CacheHeader(nullptr, 0);
}

template<bool UseVirtualDestructor>
std::unique_ptr<Archives<UseVirtualDestructor>> Archive<UseVirtualDestructor>::open(const OpenOptions &options)
{
	//  auto archives = std::make_unique<Archives>();
	std::unique_ptr<Archives<UseVirtualDestructor>> archives(new Archives<UseVirtualDestructor>());

	open(options, *archives);
	if (!options.detect && !archives->empty())
		archives->erase(archives->begin(), archives->end() - 1);
	return archives;
}

template<bool UseVirtualDestructor>
void Archive<UseVirtualDestructor>::reopen()
{
	assert(!in_arc);
	volume_names.clear();
	ArchiveOpenStream<UseVirtualDestructor> *stream_impl = new ArchiveOpenStream<UseVirtualDestructor>(arc_path);
	ComObject<IInStream<UseVirtualDestructor>> stream(stream_impl);
	arc_info = stream_impl->get_info();
	ArcChain::const_iterator arc_entry = arc_chain.begin();
	if (arc_entry->sig_pos > 0) {
		auto opened = open(new ArchiveSubStream<UseVirtualDestructor>(stream, arc_entry->sig_pos), arc_entry->type);
		if (opened)
			base_stream = stream;
		else
			FAIL(E_FAIL);
	} else {
		if (!open(stream, arc_entry->type))
			FAIL(E_FAIL);
	}
	++arc_entry;
	while (arc_entry != arc_chain.end()) {
		UInt32 main_file;
		CHECK(get_main_file(main_file));
		ComObject<IInStream<UseVirtualDestructor>> sub_stream;
		CHECK(get_stream(main_file, sub_stream.ref()));
		arc_info = get_file_info(main_file);
		sub_stream->Seek(arc_entry->sig_pos, STREAM_SEEK_SET, nullptr);
		CHECK(open(sub_stream, arc_entry->type));
		++arc_entry;
	}
}

template<bool UseVirtualDestructor>
void Archive<UseVirtualDestructor>::close()
{
	//fprintf(stderr,"Archive<UseVirtualDestructor>::close()\n");
	base_stream = nullptr;
	if (in_arc) {
		in_arc->Close();
		in_arc.Release();
	}
	file_list.clear();
	file_list_index.clear();
}

template class Archive<true>;
template class Archive<false>;
