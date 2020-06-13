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


bool LIBARCH_CommandDelete(const char *arc_path, int files_cnt, char *files[])
{
	std::string tmp_path = arc_path;
	tmp_path+= ".tmp";

	std::string str;
	std::vector<std::vector<std::string> > files_parts;
	for (int i = 0; i < files_cnt; ++i) if (files[i]) {
		files_parts.emplace_back();
		str = files[i];
		StrExplode(files_parts.back(), str, "/\\");
	}

	std::vector<std::string> parts;

	try {
		LibArchOpenRead arc_src(arc_path);
		LibArchOpenWrite arc_dst(tmp_path.c_str(), arc_src.Get());
		for (;;) {
			struct archive_entry *entry = arc_src.NextHeader();
			if (!entry) {
				break;
			}
			const char *pathname = archive_entry_pathname(entry);
			if (pathname) {
				parts.clear();
				str = pathname;
				StrExplode(parts, str, "/\\");
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
			}

			int r = LibArchCall(archive_write_header, arc_dst.Get(), entry);
			if (r != ARCHIVE_OK) {
				throw std::runtime_error(StrPrintf(
					"Error %d writing header: %s", r, pathname));
			}

			if (archive_entry_filetype(entry) != AE_IFREG || archive_entry_size(entry) == 0) {
				continue;
			}

			for (__LA_INT64_T offset = 0, sz = archive_entry_size(entry); offset < sz;) {
				const void *buf = nullptr;
				size_t size = 0;
				int r = LibArchCall(archive_read_data_block, arc_src.Get(), &buf, &size, &offset);
				if (r != ARCHIVE_OK || size == 0) {
					throw std::runtime_error(StrPrintf("Error %d reading at 0x%llx : %s",
						r, (unsigned long long)offset, archive_entry_pathname(entry)));
				}
				if (!arc_dst.WriteData(buf, size)) {
					throw std::runtime_error("write data failed");
				}
				offset+= size;
			}
		}

	} catch (std::exception &e) {
		remove(tmp_path.c_str());
		fprintf(stderr, "Exception: %s\n", e.what());
		return false;
	}

	int r = rename(tmp_path.c_str(), arc_path);
	if (r == -1) {
		fprintf(stderr, "Rename error %u: %s -> %s\n", errno, tmp_path.c_str(), arc_path);
		remove(tmp_path.c_str());
		return false;
	}

	return true;
}

