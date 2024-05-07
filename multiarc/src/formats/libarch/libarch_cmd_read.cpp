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

static bool PartsMatchesWanted(const PathParts &wanted, const PathParts &parts)
{
	size_t i = 0;
	while (i != wanted.size() && i != parts.size() && (wanted[i] == parts[i] || wanted[i] == "*")) {
		++i;
	}

	return (i == wanted.size());
}

static bool PartsMatchesAnyOfWanteds(const std::vector<PathParts > &wanteds, const PathParts &parts)
{
	for (const auto &w : wanteds) {
		if (PartsMatchesWanted(w, parts)) {
			return true;
		}
	}

	return false;
}

static bool LIBARCH_CommandReadWanteds(const char *cmd, LibArchOpenRead &arc,
	const size_t root_count, const std::vector<PathParts > &wanteds)
{
	std::string src_path, extract_path;
	PathParts parts;

	bool out = true;
	for (;;) {
		struct archive_entry *entry = arc.NextHeader();
		if (!entry) {
			break;
		}

		const char *pathname = LibArch_EntryPathname(entry);
		src_path = pathname ? pathname : "";
		parts.clear();
		parts.Traverse(src_path);

		if (parts.empty()) {
			fprintf(stderr, "Empty path: '%s' '%ls'\n",
				pathname, archive_entry_pathname_w(entry));
			arc.SkipData();
			continue;
		}

		if (!wanteds.empty() && !PartsMatchesAnyOfWanteds(wanteds, parts)) {
//			fprintf(stderr, "Not matching: '%s' '%ls'\n", pathname, archive_entry_pathname_w(entry));
			arc.SkipData();
			continue;
		}

		switch (*cmd) {
			case 'X': {
				extract_path = '.';
				size_t root_dismiss_counter = root_count;
				for (const auto &p : parts) {
					if (root_dismiss_counter == 0) {
						mkdir(extract_path.c_str(),
							S_IWUSR | S_IRUSR | S_IRGRP | S_IROTH | S_IXUSR | S_IXGRP | S_IXOTH);
						extract_path+= '/';
						extract_path+= p;
					} else {
						--root_dismiss_counter;
					}
				}
			} break;

			case 'x': {
				extract_path = "./";
				extract_path+= parts.back();
			} break;

			case 't':
			default:
				extract_path.clear();
		}

		int r = ARCHIVE_OK;
		if (extract_path.empty()) {
			// don't use 'extraction' to /dev/null as libarchive 'recreates' it when running under root (#2182)
			const __LA_INT64_T size = (archive_entry_filetype(entry) == AE_IFREG) ? archive_entry_size(entry) : 0;
			for (__LA_INT64_T offset = 0, zeroreads = 0; offset < size;) {
				const void *buf = nullptr;
				size_t size_cur = 0;
				__LA_INT64_T offset_cur = offset;
				r = LibArchCall(archive_read_data_block, arc.Get(), &buf, &size_cur, &offset_cur);
				if (r != ARCHIVE_OK  && r != ARCHIVE_WARN) {
					break;
				}
				if (size_cur == 0) {
					// sometimes libarchive returns zero length blocks, but next time it return non-zero
					// not sure how to handle that right, so roughly giving it 1000 attempts..
					if (zeroreads > 1000) {
						r = ARCHIVE_RETRY;
						break;
					}
					++zeroreads;
				} else {
					zeroreads = 0;
				}
				offset+= size_cur;
			}

		} else {
			archive_entry_set_pathname(entry, extract_path.c_str() );
			r = archive_read_extract(arc.Get(), entry, ARCHIVE_EXTRACT_TIME); // ARCHIVE_EXTRACT_PERM???
		}
		if (r != ARCHIVE_OK && r != ARCHIVE_WARN) {
			fprintf(stderr, "Error %d (%s): '%s' -> '%s'\n",
				r, archive_error_string(arc.Get()),
				src_path.c_str(), extract_path.c_str());
			out = false;

		} else if (extract_path.empty()) {
			fprintf(stderr, "Tested: '%s'\n", src_path.c_str());
			if (wanteds.size() == 1 && wanteds[0] == parts && archive_entry_filetype(entry) != AE_IFDIR) {
				break;
			}

		} else {
			fprintf(stderr, "Extracted: '%s' -> '%s'\n",
				src_path.c_str(), extract_path.c_str());

			struct stat s;
			if (wanteds.size() == 1 && wanteds[0] == parts
			  && stat(extract_path.c_str(), &s) == 0 && !S_ISDIR(s.st_mode)) {
				break; // nothing to search more here
			}
		}
	}

	return out;
}

bool LIBARCH_CommandRead(const char *cmd, const char *arc_path, const LibarchCommandOptions &arc_opts, int files_cnt, char *files[])
{
	std::vector<PathParts > wanteds;
	wanteds.reserve(files_cnt);

	PathParts root;
	if (!arc_opts.root_path.empty()) {
		root.Traverse(arc_opts.root_path);
	}

	for (int i = 0; i < files_cnt; ++i) {
		wanteds.emplace_back();
		if (files[i]) {
			if (*files[i]) {
				wanteds.back().Traverse(std::string(files[i]));
			}
			if (!root.empty() && !wanteds.back().Starts(root)) {
				fprintf(stderr, "Fixup root for path: '%s'\n", files[i]);
				wanteds.back().insert(wanteds.back().begin(), root.begin(), root.end());
			}
		}

		if (wanteds.back().empty()) {
			fprintf(stderr, "Skipping empty path: '%s'\n", files[i]);
			wanteds.pop_back();
		}
	}

	if (wanteds.empty() && files_cnt > 0) {
		return false;
	}

	LibArchOpenRead arc(arc_path, cmd, arc_opts.charset.c_str());
	return LIBARCH_CommandReadWanteds(cmd, arc, root.size(), wanteds);
}
