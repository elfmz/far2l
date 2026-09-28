#include "libarch_crutches.h"

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

uint64_t LibArch_UnpackedSizeOfGZ(LibArchOpenRead *arc)
{
	const off_t len = arc->RawSize();
	uint32_t out = 0;
	if (len > (off_t)sizeof(out)) {
		arc->RawRead(&out, sizeof(out), len - sizeof(out));
	}
	return out;
}

static uint64_t RawReadXZVarInt(LibArchOpenRead *arc, off_t &ofs)
{
	uint8_t buf[9]{};
	ssize_t size_max = arc->RawRead(&buf, sizeof(buf), ofs);
	if (size_max <= 0)
		return 0;

	uint64_t num = buf[0] & 0x7F;
	ssize_t i = 0;

	while (buf[i++] & 0x80) {
		if (i >= size_max || buf[i] == 0x00) {
			return 0;
		}

		num|= (uint64_t)(buf[i] & 0x7F) << (i * 7);
	}
	ofs+= i;

	return num;
}

uint64_t LibArch_UnpackedSizeOfXZ(LibArchOpenRead *arc)
{
	struct {
		uint32_t crc32;
		uint32_t index_size;
		uint8_t flags[2];
		uint8_t magic[2];
	} footer;
	const off_t len = arc->RawSize();

	if (len <= (off_t)sizeof(footer))
		return 0;
	if (arc->RawRead(&footer, sizeof(footer), len - sizeof(footer)) != sizeof(footer))
		return 0;
	if (footer.magic[0] != 'Y' || footer.magic[1] != 'Z')
		return 0;

	off_t real_index_size = (off_t(footer.index_size) + 1) * 4;

	if (len < (off_t)(sizeof(footer) + real_index_size))
		return 0;

	uint64_t out = 0;
	off_t ofs = len - sizeof(footer) - real_index_size;
	uint8_t index_indicator{};
	if (arc->RawRead(&index_indicator, sizeof(index_indicator), ofs) != sizeof(index_indicator) || index_indicator != 0) {
		return 0;
	}
	ofs+= sizeof(index_indicator);
	for (uint64_t nrecs = RawReadXZVarInt(arc, ofs); nrecs; --nrecs) {
		auto saved_ofs = ofs;
		RawReadXZVarInt(arc, ofs); // skip Unpadded Size
		out+= RawReadXZVarInt(arc, ofs); // read Uncompressed Size
		if (saved_ofs == ofs) { // IO error
			break;
		}
	}

	return out;
}

