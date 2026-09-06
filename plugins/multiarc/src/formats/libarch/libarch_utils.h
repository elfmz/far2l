#pragma once
#include <vector>
#include <unistd.h>
#include <vector>
#include <stdexcept>

#include <archive.h>
#include <archive_entry.h>
#include <PathParts.h>

template <typename ... ARGS>
	static int LibArchCall(int (*pfn)(ARGS ... args), ARGS ... args)
{
	for (unsigned int i = 0;; ++i) {
		int r = pfn(args ...);
		if (r != ARCHIVE_RETRY || i == 1000) {
			return r;
		}
		usleep(10000);
	}
};

class LibArchTempEntry
{
	struct archive_entry *_entry;
public:
	LibArchTempEntry();
	~LibArchTempEntry();
	struct archive_entry *Get() { return _entry; }
	operator struct archive_entry *() { return _entry; }
	//struct archive_entry *entry = archive_entry_new();
};

void LibArch_SetPassprhase(const char *passprhase);

const char *LibArch_EntryPathname(struct archive_entry *e);

bool LibArch_DetectedFormatHasCompression(struct archive *a);

struct LibArchOpenRead
{
	LibArchOpenRead(const char *name, const char *cmd, const char *charset);
	~LibArchOpenRead();

	off_t RawSize();
	ssize_t RawRead(void *data, size_t len, off_t ofs);

	inline unsigned int Format() const { return _fmt; }
	inline struct archive *Get() { return _arc; }

	struct archive_entry *NextHeader();
	void SkipData();

protected:
	struct archive *_arc = nullptr;

private:
	int _fd = -1;
	off_t _pos = 0;
	struct archive_entry *_ae = nullptr;
	unsigned int _fmt = 0;
	char _buf[0x2000];
	bool _eof = false;

	LibArchOpenRead(const LibArchOpenRead&) = delete;
	void Open(const char *name);
	void EnsureClosed();
	void EnsureClosedFD();
	void PrepareForOpen(const char *charset);

	static __LA_SSIZE_T sReadCallback(struct archive *, void *it, const void **_buffer);
	static __LA_INT64_T sSkipCallback(struct archive *a, void *it, __LA_INT64_T request);
	static __LA_INT64_T sSeekCallback(struct archive *, void *it, __LA_INT64_T offset, int whence);
	static int sCloseCallback(struct archive *, void *it);
};

struct LibArchOpenWrite
{
	LibArchOpenWrite(const char *name, const char *cmd, const char *charset, int compression_level = -1);
	LibArchOpenWrite(const char *name, struct archive *arc_template, const char *charset, int compression_level = -1);

	~LibArchOpenWrite();

	inline struct archive *Get() { return _arc; }
	bool WriteData(const void *data, size_t len);

protected:
	struct archive *_arc = nullptr;

private:
	LibArchOpenWrite(const LibArchOpenWrite&) = delete;
	void PrepareForOpen(const char *charset, unsigned int format, int compression_level);
};
