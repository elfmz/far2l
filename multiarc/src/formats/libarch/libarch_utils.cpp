#include <string.h>

#include <string>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <sudo.h>
#include <utils.h>

#include "libarch_utils.h"


#if (ARCHIVE_VERSION_NUMBER >= 3002000)
static std::string s_passprhase;
static bool s_passprhase_is_set = false;

void LibArch_SetPassprhase(const char *passprhase)
{
	s_passprhase = passprhase;
	s_passprhase_is_set = true;
}

static const char *LibArch_PassprhaseCallback(struct archive *, void *_client_data)
{
	return s_passprhase_is_set ? s_passprhase.c_str() : getpass("Password please:");
}
#else

void LibArch_SetPassprhase(const char *passprhase)
{
	fprintf(stderr, "Used libarchive doesn't support passworded archives, please rebuild with libarchive version 3.2.0 or higher.\n");
}

#endif

const char *LibArch_EntryPathname(struct archive_entry *e)
{
#if (ARCHIVE_VERSION_NUMBER >= 3002000)
	const char *utf8 = archive_entry_pathname_utf8(e);
	if (utf8) {
		return utf8;
	}
#endif
	return archive_entry_pathname(e);
}

bool LibArch_DetectedFormatHasCompression(struct archive *a)
{
	unsigned int fmt = archive_format(a);
	if (fmt != ARCHIVE_FORMAT_RAW) {
		return true;
	}

	for (int i = 0, ii = archive_filter_count(a); i < ii; ++i) {
		auto fc = archive_filter_code(a, i);
//		fprintf(stderr, "LibArch_DetectedFormatHasCompression: fc[%d]=0x%x\n", i, fc);
		if (fc != 0) {
			return true;
		}
	}

	return false;
}


void LibArch_ParsePathToParts(std::vector<std::string> &parts, const std::string &path)
{
	size_t i = parts.size();
	StrExplode(parts, path, "/");
	while (i < parts.size()) {
		if (parts[i] == ".") {
			parts.erase(parts.begin() + i);
		} else if (parts[i] == "..") {
			parts.erase(parts.begin() + i);
			if (i != 0) {
				parts.erase(parts.begin() + i - 1);
				--i;
			} else {
				fprintf(stderr, "LibArch_ParsePathToParts: impossible <..> in '%s'\n", path.c_str());
			}
		} else {
			++i;
		}
	}
}

LibArchOpenRead::LibArchOpenRead(const char *name, const char *cmd, const char *charset)
{
	Open(name);
	LibArchCall(archive_read_support_filter_all, _arc);

	/// Workaround for #710:
	// if request supporting all formats then librchive somewhy detects archive
	// format incorrectly, like detecting some raw gz archives as 'mtree' format,
	// so workaround invented - first open archive while allowing only those formats
	// that we primariliy support, and only if this will fail- try to open
	// using full list of supported formats
	LibArchCall(archive_read_support_format_raw, _arc);
	LibArchCall(archive_read_support_format_tar, _arc);
	LibArchCall(archive_read_support_format_iso9660, _arc);
	LibArchCall(archive_read_support_format_gnutar, _arc);
	LibArchCall(archive_read_support_format_cpio, _arc);
	LibArchCall(archive_read_support_format_cab, _arc);
	PrepareForOpen(charset);

	int r = LibArchCall(archive_read_open1, _arc);
	if (r == ARCHIVE_OK || r == ARCHIVE_WARN) {
		_ae = NextHeader();
		if (!LibArch_DetectedFormatHasCompression(_arc)) {
			r = ARCHIVE_EOF;
		}
	}

	if (r != ARCHIVE_OK && r != ARCHIVE_WARN) {
		EnsureClosed();
		Open(name);

		LibArchCall(archive_read_support_filter_all, _arc);
		LibArchCall(archive_read_support_format_all, _arc);
		PrepareForOpen(charset);

		// already tried this: LibArchCall(archive_read_support_format_raw, _arc);
		r = LibArchCall(archive_read_open1, _arc);
		if (r != ARCHIVE_OK && r != ARCHIVE_WARN) {
			EnsureClosed();
			throw std::runtime_error(StrPrintf("error %d (%s) opening archive '%s'",
				r, archive_error_string(_arc), name));
		}

		_ae = NextHeader();
	}

	_fmt = archive_format(_arc);
	if (_fmt == ARCHIVE_FORMAT_RAW && _ae != nullptr) {
		const char *name_part = strrchr(name, '/');
		std::string arc_raw_name(name_part ? name_part + 1 : name);
		size_t p = arc_raw_name.rfind('.');
		if (p != std::string::npos) {
			arc_raw_name.resize(p);
		} else {
			arc_raw_name+= ".data";
		}
		archive_entry_set_pathname(_ae, arc_raw_name.c_str() );
	}
}

LibArchOpenRead::~LibArchOpenRead()
{
	EnsureClosed();
}

void LibArchOpenRead::PrepareForOpen(const char *charset)
{
	if (charset && *charset)  {
		char opt_hdrcharset[0x100] = {0};
		snprintf(opt_hdrcharset, sizeof(opt_hdrcharset) - 1, "hdrcharset=%s", charset);
		int r = LibArchCall(archive_read_set_options, _arc, (const char *)opt_hdrcharset);
		if (r != 0) {
			fprintf(stderr, "LibArchOpenRead::PrepareForOpen('%s') hdrcharset error %d (%s)\n",
				charset, r, archive_error_string(_arc));
		}/* else {
			fprintf(stderr, "LibArchOpenRead::PrepareForOpen('%s') hdrcharset OK\n",
				charset);
		}*/
	}/* else {
		fprintf(stderr, "LibArchOpenRead::PrepareForOpen('%s') hdrcharset NOPE\n",
			charset);
	} */
}

void LibArchOpenRead::Open(const char *name)
{
	_arc = archive_read_new();
	_fd = sdc_open(name, O_RDONLY);
	if (!_arc || _fd == -1) {
		EnsureClosed();
		throw std::runtime_error(StrPrintf("error %d opening archive '%s'",
			errno, name));
	}

	LibArchCall(archive_read_set_callback_data, _arc, (void *)this);
	LibArchCall(archive_read_set_read_callback, _arc, sReadCallback);
	LibArchCall(archive_read_set_seek_callback, _arc, sSeekCallback);
	LibArchCall(archive_read_set_skip_callback, _arc, sSkipCallback);
	LibArchCall(archive_read_set_close_callback, _arc, sCloseCallback);

#if (ARCHIVE_VERSION_NUMBER >= 3002000)
	archive_read_set_passphrase_callback(_arc, nullptr, LibArch_PassprhaseCallback);
#endif
	_eof = false;
}

struct archive_entry *LibArchOpenRead::NextHeader()
{
	if (_eof) {
		return nullptr;
	}

	struct archive_entry *entry = nullptr;
	if (_ae != nullptr) {
		entry = _ae;
		_ae = nullptr;

	} else {
		int r = LibArchCall(archive_read_next_header, _arc, &entry);

		if (r == ARCHIVE_EOF) {
			_eof = true;
			return nullptr;
		}

		if (r != ARCHIVE_OK && r != ARCHIVE_WARN) {
			throw std::runtime_error(StrPrintf("archive_read_next_header - error %d (%s)\n",
				r, archive_error_string(_arc)));
		}
	}

	return entry;
}

void LibArchOpenRead::SkipData()
{
	LibArchCall(archive_read_data_skip, _arc);
}

void LibArchOpenRead::EnsureClosed()
{
	if (_arc != nullptr) {
		archive_read_free(_arc);
		_arc = nullptr;
	}

	_ae = nullptr;

	EnsureClosedFD();
}

void LibArchOpenRead::EnsureClosedFD()
{
	if (_fd != -1) {
		sdc_close(_fd);
		_fd = -1;
		_pos = 0;
	}
}

__LA_SSIZE_T LibArchOpenRead::sReadCallback(struct archive *, void *it, const void **_buffer)
{
	*_buffer = ((LibArchOpenRead *)it)->_buf;
	ssize_t r = sdc_read(((LibArchOpenRead *)it)->_fd, ((LibArchOpenRead *)it)->_buf, sizeof(((LibArchOpenRead *)it)->_buf));
	if (r > 0) {
		((LibArchOpenRead *)it)->_pos+= r;
	}
	return (r < 0) ? ARCHIVE_FATAL : r;
}

__LA_INT64_T LibArchOpenRead::sSkipCallback(struct archive *a, void *it, __LA_INT64_T request)
{
	__LA_INT64_T prev_pos = ((LibArchOpenRead *)it)->_pos;
	sSeekCallback(a, it, request, SEEK_CUR);
	return ((LibArchOpenRead *)it)->_pos - prev_pos;
}

__LA_INT64_T LibArchOpenRead::sSeekCallback(struct archive *, void *it, __LA_INT64_T offset, int whence)
{
	off_t r = sdc_lseek(((LibArchOpenRead *)it)->_fd, offset, whence);
	if (r < 0) {
		r = sdc_lseek(((LibArchOpenRead *)it)->_fd, 0, SEEK_CUR);
		if (r >= 0) {
			((LibArchOpenRead *)it)->_pos = r;
		}
		return ARCHIVE_FATAL;
	}
	((LibArchOpenRead *)it)->_pos = r;
	return r;
}

int LibArchOpenRead::sCloseCallback(struct archive *, void *it)
{
	((LibArchOpenRead *)it)->EnsureClosedFD();
	return 0;
}

///////////////////////////////////////////////

static const char *NameExt(const char *name)
{
	const char *slash = strrchr(name, '/');

	const char *ext = strchr(slash ? slash + 1 : name, '.');

	return ext ? ext : (slash ? slash + 1 : name);
}

LibArchOpenWrite::LibArchOpenWrite(const char *name, const char *cmd, const char *charset)
{
	const char *ne = NameExt(name);

	int format = ARCHIVE_FORMAT_TAR, filter = ARCHIVE_FILTER_GZIP; // defaults

	if (strstr(cmd, ":cpio")) { format = ARCHIVE_FORMAT_CPIO;
	} else if (strstr(cmd, ":zip")) { format = ARCHIVE_FORMAT_ZIP; filter = 0;
	} else if (strstr(cmd, ":cab")) { format = ARCHIVE_FORMAT_CAB; filter = 0;
	} else if (strstr(cmd, ":iso")) { format = ARCHIVE_FORMAT_ISO9660;
	} else if (strstr(cmd, ":rar")) { format = ARCHIVE_FORMAT_RAR; filter = 0;
	} else if (strstr(ne, ".cpio")) { format = ARCHIVE_FORMAT_CPIO;
	} else if (strstr(ne, ".zip")) { format = ARCHIVE_FORMAT_ZIP; filter = 0;
	} else if (strstr(ne, ".cab")) { format = ARCHIVE_FORMAT_CAB; filter = 0;
	} else if (strstr(ne, ".iso")) { format = ARCHIVE_FORMAT_ISO9660;
	} else if (strstr(ne, ".rar")) { format = ARCHIVE_FORMAT_RAR; filter = 0;
	}

	if (strstr(cmd, ":plain")) { filter = 0;
	} else if (strstr(cmd, ":bz")) { filter = ARCHIVE_FILTER_BZIP2;
	} else if (strstr(cmd, ":lzip")) { filter = ARCHIVE_FILTER_LZIP;
	} else if (strstr(cmd, ":xz")) { filter = ARCHIVE_FILTER_XZ;
	} else if (strstr(cmd, ":uu")) { filter = ARCHIVE_FILTER_UU;
	} else if (strstr(cmd, ":rpm")) { filter = ARCHIVE_FILTER_RPM;
	} else if (strstr(cmd, ":lz")) { filter = ARCHIVE_FILTER_LZMA;
	} else if (strstr(ne, ".bz")) { filter = ARCHIVE_FILTER_BZIP2;
	} else if (strstr(ne, ".lzip")) { filter = ARCHIVE_FILTER_LZIP;
	} else if (strstr(ne, ".uue")) { filter = ARCHIVE_FILTER_UU;
	} else if (strstr(ne, ".rpm")) { filter = ARCHIVE_FILTER_RPM;
	} else if (strstr(ne, ".lz")) { filter = ARCHIVE_FILTER_LZMA;
	}

	if (format == ARCHIVE_FORMAT_ZIP && (!charset || !*charset)) {
		charset = "UTF-8";
	}

	_arc = archive_write_new();
	if (!_arc) {
		throw std::runtime_error("failed to init archiver");
	}
	archive_write_set_format(_arc, format);
	if (filter) {
		archive_write_add_filter(_arc, filter);
	}
	PrepareForOpen(charset, format);

	int r = LibArchCall(archive_write_open_filename, _arc, name);
	if (r != ARCHIVE_OK && r != ARCHIVE_WARN) {
		archive_write_free(_arc);
		throw std::runtime_error(StrPrintf("error %d (%s) opening archive %s",
			r, archive_error_string(_arc), name));
	}
}

LibArchOpenWrite::LibArchOpenWrite(const char *name, struct archive *arc_template, const char *charset)
{
	_arc = archive_write_new();
	if (!_arc) {
		throw std::runtime_error("failed to init archiver");
	}

	auto format = archive_format(arc_template);

	if (format == ARCHIVE_FORMAT_ZIP && (!charset || !*charset)) {
		charset = "UTF-8";
	}

	archive_write_set_format(_arc, format);
	for (int i = 0, ii = archive_filter_count(arc_template); i < ii; ++i) {
		int fc = archive_filter_code(arc_template, i);
		if (fc != 0) {
			archive_write_add_filter(_arc, fc);
		}
	}

	PrepareForOpen(charset, format);

	int r = LibArchCall(archive_write_open_filename, _arc, name);
	if (r != ARCHIVE_OK && r != ARCHIVE_WARN) {
		archive_write_free(_arc);
		throw std::runtime_error(StrPrintf("error %d (%s) opening archive %s",
			r, archive_error_string(_arc), name));
	}
}

LibArchOpenWrite::~LibArchOpenWrite()
{
	archive_write_close(_arc);
	archive_write_free(_arc);
}

void LibArchOpenWrite::PrepareForOpen(const char *charset, unsigned int format)
{
	if (charset && *charset) {
		char opt_hdrcharset[0x100] = {0};
		snprintf(opt_hdrcharset, sizeof(opt_hdrcharset) - 1, "hdrcharset=%s", charset);
		int r = LibArchCall(archive_write_set_options, _arc, (const char *)opt_hdrcharset);
		if (r != 0) {
			fprintf(stderr, "LibArchOpenWrite::PrepareForOpen('%s') hdrcharset error %d (%s)\n",
				charset, r, archive_error_string(_arc));
		}
	}

#if (ARCHIVE_VERSION_NUMBER >= 3002000)
	if (s_passprhase_is_set) {
		if (format == ARCHIVE_FORMAT_ZIP) {
			int r = archive_write_set_options(_arc, "zip:encryption=aes256");
			if (r != ARCHIVE_OK) {
				r = archive_write_set_options(_arc, "zip:encryption=zipcrypt");
				if (r != ARCHIVE_OK) {
					fprintf(stderr, "Cannot use any encryption, error %d (%s)\n",
						r, archive_error_string(_arc));
				} else {
					fprintf(stderr, "Using ZipCrypto encryption\n");
				}
			} else {
				fprintf(stderr, "Using AES encryption\n");
			}

		} else {
			fprintf(stderr, "Encryption not supported for archive format %u\n", format);
		}

		int r = archive_write_set_passphrase(_arc, s_passprhase.c_str());
		if (r != ARCHIVE_OK) {
			fprintf(stderr, "LibArchOpenWrite::PrepareForOpen('%s') setting password error %d (%s)\n",
				charset, r, archive_error_string(_arc));
		}
	}
#endif
}


bool LibArchOpenWrite::WriteData(const void *data, size_t len)
{
	while (len) {
		ssize_t r = archive_write_data(_arc, data, len);
		if (r <= 0) {
			return false;
		}
		len-= (size_t)r;
	}

	return true;
}
