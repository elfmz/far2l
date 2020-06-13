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

bool LIBARCH_CommandHandler(LibArchOpenRead &arc, const char *cmd, const char *wanted_path = nullptr)
{
	std::string src_path, extract_path;
	std::vector<std::string> parts, wanted;
	
	if (wanted_path && *wanted_path) {
		StrExplode(wanted, std::string(wanted_path), "/\\");
	}

	bool out = true;
	for (;;) {
		struct archive_entry *entry = arc.NextHeader();
		if (!entry) {
			break;
		}

		const char *pathname = archive_entry_pathname(entry);
		
		src_path = pathname ? pathname : "";
		parts.clear();
		StrExplode(parts, src_path, "/\\");

		if (parts.empty()) {
			fprintf(stderr, "Empty path: '%s'\n", pathname);
			continue;
		}

		size_t i = 0;
		while (i != wanted.size() && i != parts.size() && (wanted[i] == parts[i] || wanted[i] == "*")) {
			++i;
		}

		if (i != wanted.size()) {
			printf("Skip: '%s'\n", pathname);
			continue;
		}

		switch (*cmd) {
			case 'X': {
				extract_path = '.';
				for (const auto &p : parts) {
					mkdir(extract_path.c_str(),
						S_IWUSR | S_IRUSR | S_IRGRP | S_IROTH | S_IXUSR | S_IXGRP | S_IXOTH);
					extract_path+= '/';
					extract_path+= p;
				}
			} break;

			case 'x': {
				extract_path = "./";
				extract_path+= parts.back();
			} break;

			case 't':
			default:
				extract_path = "/dev/null";
		}

		archive_entry_set_pathname(entry, extract_path.c_str() );
		int r = archive_read_extract(arc.Get(), entry, 0);
		if (r != ARCHIVE_OK) {
			fprintf(stderr, "Error %d: '%s' -> '%s'\n",
				r, src_path.c_str(), extract_path.c_str());
			out = false;

		} else {
			fprintf(stderr, "Extracted: '%s' -> '%s'\n",
				src_path.c_str(), extract_path.c_str());
		}
	}

	return out;
}
