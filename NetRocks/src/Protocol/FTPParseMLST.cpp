#include <string.h>

#include "FTPParseMLST.h"
#include "../Erroring.h"
#include <utils.h>

static const char match__unix_mode[] = "UNIX.mode";
static const char match__unix_uid[] = "UNIX.uid";
static const char match__unix_gid[] = "UNIX.gid";

static const char match__type[] = "type";
static const char match__type_os_unix[] = "type=OS.unix";
static const char match__perm[] = "perm";
static const char match__size[] = "size";
static const char match__sizd[] = "sizd";

static const char match__file[] = "file";
static const char match__dir[] = "dir";
static const char match__cdir[] = "cdir";
static const char match__pdir[] = "pdir";

#define MATCH_SUBSTR(str, len, match) (sizeof(match) == (len) + 1 && CaseIgnoreEngStrMatch(str, match, sizeof(match) - 1))

bool ParseMLsxLine(const char *line, const char *end, FileInformation &file_info, uid_t *uid, gid_t *gid, std::string *name)
{
	file_info = FileInformation();

	bool has_mode = false, has_type = false, has_any_known = false;
	const char *perm = nullptr, *perm_end = nullptr;

	for (; line != end && *line == ' '; ++line) {;}

	const char *fact, *eq;
	for (fact = eq = line; line != end; ++line) if (*line == '=') {
		eq = line;

	} else if (*line ==';') {
		if (eq > fact) {
			const size_t name_len = eq - fact;
			const size_t value_len = line - (eq + 1);

			if (MATCH_SUBSTR(fact, name_len, match__unix_mode)) {
				file_info.mode|= strtol(eq + 1, nullptr, (*(eq + 1) == '0') ? 8 : 10);
				has_any_known = has_mode = true;

			} else if (MATCH_SUBSTR(fact, name_len, match__unix_uid)) {
				if (uid) *uid = strtol(eq + 1, nullptr, 10);
				has_any_known = true;

			} else if (MATCH_SUBSTR(fact, name_len, match__unix_gid)) {
				if (gid) *gid = strtol(eq + 1, nullptr, 10);
				has_any_known = true;

			} else if (MATCH_SUBSTR(fact, name_len, match__type)) {
				if (MATCH_SUBSTR(eq + 1, value_len, match__file)) {
					file_info.mode|= S_IFREG;
					has_type = true;
					has_any_known = true;

				} else if (MATCH_SUBSTR(eq + 1, value_len, match__dir)
				  || MATCH_SUBSTR(eq + 1, value_len, match__cdir)
				  || MATCH_SUBSTR(eq + 1, value_len, match__pdir)) {
					file_info.mode|= S_IFDIR;
					has_type = true;
					has_any_known = true;

				} else if (g_netrocks_verbosity > 0) {
					fprintf(stderr, "ParseMLsxLine: unknown type='%s'\n",
						std::string(eq + 1, value_len).c_str());
				}

			} else if (MATCH_SUBSTR(fact, name_len, match__perm)) {
				perm = eq + 1;
				perm_end = line;
				has_any_known = true;

			} else if (MATCH_SUBSTR(fact, name_len, match__size)
			  || MATCH_SUBSTR(fact, name_len, match__sizd)) {
				file_info.size = strtol(eq + 1, nullptr, 10);
				has_any_known = true;

			} else if (MATCH_SUBSTR(fact, name_len, match__type_os_unix)) {
				if (value_len >= 6 && CaseIgnoreEngStrMatch(eq + 1, "slink:", 6)) {
					file_info.mode|= S_IFLNK;
					has_type = true;
					has_any_known = true;
				}
			}
		}

		fact = line + 1;
	}

	if (name) {
		if (fact != end && *fact == ' ') {
			++fact;
		}
		if (fact != end) {
			name->assign(fact, end - fact);
		} else {
			name->clear();
		}
	}

	if (!has_type)  {
		if (perm) {
			if (CaseIgnoreEngStrChr('c', perm, perm_end - perm) != nullptr
			  || CaseIgnoreEngStrChr('l', perm, perm_end - perm) != nullptr) {
				file_info.mode|= S_IFDIR;

			} else {
				file_info.mode|= S_IFREG;
			}

		} else {
			file_info.mode|= S_IFREG; // last resort
		}
	}
//	fprintf(stderr, "type:%s\n", ((file_info.mode & S_IFMT) == S_IFDIR) ? "DIR" : "FILE");


	if (!has_mode)  {
		if (perm) {
			if (CaseIgnoreEngStrChr('r', perm, perm_end - perm) != nullptr
			  || CaseIgnoreEngStrChr('l', perm, perm_end - perm) != nullptr
			  || CaseIgnoreEngStrChr('e', perm, perm_end - perm) != nullptr) {
				file_info.mode|= 0444;
				if ((file_info.mode & S_IFMT) == S_IFDIR) {
					file_info.mode|= 0111;
				}
			}
			if (CaseIgnoreEngStrChr('w', perm, perm_end - perm) != nullptr
			  || CaseIgnoreEngStrChr('a', perm, perm_end - perm) != nullptr
			  || CaseIgnoreEngStrChr('c', perm, perm_end - perm) != nullptr) {
				file_info.mode|= 0220;
			}
			if (CaseIgnoreEngStrChr('x', perm, perm_end - perm) != nullptr
			  || CaseIgnoreEngStrChr('e', perm, perm_end - perm) != nullptr) {
				file_info.mode|= 0111;
			}

		} else if ((file_info.mode & S_IFMT) == S_IFDIR) {
			file_info.mode|= DEFAULT_ACCESS_MODE_DIRECTORY; // last resort

		} else {
			file_info.mode|= DEFAULT_ACCESS_MODE_FILE; // last resort
		}
	}

	return has_any_known;
}
