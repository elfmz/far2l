#include <string.h>
#include <stdlib.h>
#include <time.h>

#include "FTPParseMLST.h"
#include "../../Erroring.h"
#include <utils.h>

static const char match__unix_mode[] = "UNIX.mode";
static const char match__unix_uid[] = "UNIX.uid";
static const char match__unix_gid[] = "UNIX.gid";

static const char match__type[] = "type";
static const char match__type_os_unix[] = "type=OS.unix";
static const char match__perm[] = "perm";
static const char match__size[] = "size";
static const char match__sizd[] = "sizd";

static const char match__modify[] = "modify";
static const char match__create[] = "create";

static const char match__file[] = "file";
static const char match__dir[] = "dir";
static const char match__cdir[] = "cdir";
static const char match__pdir[] = "pdir";

#define MATCH_SUBSTR(str, len, match) (sizeof(match) == (len) + 1 && CaseIgnoreEngStrMatch(str, match, sizeof(match) - 1))

static time_t GetDefaultTime()
{
	static time_t s_out = time(NULL);
	return s_out;
}

static void ParseTime(timespec &ts, const char *str, size_t len)
{
/*
    time-val       = 14DIGIT [ "." 1*DIGIT ]
   The leading, mandatory, fourteen digits are to be interpreted as, in
   order from the leftmost, four digits giving the year, with a range of
   1000--9999, two digits giving the month of the year, with a range of
   01--12, two digits giving the day of the month, with a range of
   01--31, two digits giving the hour of the day, with a range of
   00--23, two digits giving minutes past the hour, with a range of
   00--59, and finally, two digits giving seconds past the minute, with
   a range of 00--60 (with 60 being used only at a leap second).  Years
    The optional digits, which are preceded by a period, give decimal
   fractions of a second.  These may be given to whatever precision is
   appropriate to the circumstance, however implementations MUST NOT add
   precision to time-vals where that precision does not exist in the
   underlying value being transmitted.

   Symbolically, a time-val may be viewed as: YYYYMMDDHHMMSS.sss
*/

	struct tm t{};

	if (len >= 4) {
		t.tm_year = DecToULong(str, 4) - 1900;
		if (len >= 6) {
			t.tm_mon = DecToULong(str + 4, 2) - 1;
			if (len >= 8) {
				t.tm_mday = DecToULong(str + 6, 2);
				if (len >= 10) {
					t.tm_hour = DecToULong(str + 8, 2);
					if (len >= 12) {
						t.tm_min = DecToULong(str + 10, 2);
						if (len >= 14) {
							t.tm_sec = DecToULong(str + 12, 2);
						}
					}
				}
			}
		}
	}

	ts.tv_sec = mktime(&t);

	if (len > 15 && str[14] == '.') {
		size_t deci_len = std::min(len - 15, (size_t)9);
		ts.tv_nsec = DecToULong(str + 15, deci_len);
		for (size_t i = deci_len; i < 9; ++i) {
			ts.tv_nsec*= 10;
		}

	} else {
		ts.tv_nsec = 0;
	}
}

bool ParseMLsxLine(const char *line, const char *end, FileInformation &file_info, uid_t *uid, gid_t *gid, std::string *name, std::string *lnkto)
{
	file_info = FileInformation();

	bool has_mode = false, has_type = false, has_any_known = false, has_modify = false, has_create = false;
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

			} else if (MATCH_SUBSTR(fact, name_len, match__create)) {
				ParseTime(file_info.status_change_time, eq + 1, value_len);
				has_any_known = has_create = true;

			} else if (MATCH_SUBSTR(fact, name_len, match__modify)) {
				ParseTime(file_info.modification_time, eq + 1, value_len);
				has_any_known = has_modify = true;

			} else if (MATCH_SUBSTR(fact, name_len, match__type)) {
				if (MATCH_SUBSTR(eq + 1, value_len, match__file)) {
					file_info.mode|= S_IFREG;
					has_type = true;
					has_any_known = true;

				} else if (MATCH_SUBSTR(eq + 1, value_len, match__dir)
					|| MATCH_SUBSTR(eq + 1, value_len, match__cdir)
					|| MATCH_SUBSTR(eq + 1, value_len, match__pdir))
				{
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
				|| MATCH_SUBSTR(fact, name_len, match__sizd))
			{
				file_info.size = strtol(eq + 1, nullptr, 10);
				has_any_known = true;

			} else if (MATCH_SUBSTR(fact, name_len, match__type_os_unix)) {
				if (value_len >= 6 && CaseIgnoreEngStrMatch(eq + 1, "slink:", 6)) {
					file_info.mode|= S_IFLNK;
					has_type = true;
					has_any_known = true;
					if (lnkto) {
						lnkto->assign(eq + 1 + 6, value_len - 6);
					}
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

	if (!has_type) {
		if (perm) {
			if (CaseIgnoreEngStrChr('c', perm, perm_end - perm) != nullptr
				|| CaseIgnoreEngStrChr('l', perm, perm_end - perm) != nullptr)
			{
				file_info.mode|= S_IFDIR;

			} else {
				file_info.mode|= S_IFREG;
			}

		} else {
			file_info.mode|= S_IFREG; // last resort
		}
	}
//	fprintf(stderr, "type:%s\n", ((file_info.mode & S_IFMT) == S_IFDIR) ? "DIR" : "FILE");


	if (!has_mode) {
		if (perm) {
			if (CaseIgnoreEngStrChr('r', perm, perm_end - perm) != nullptr
				|| CaseIgnoreEngStrChr('l', perm, perm_end - perm) != nullptr
				|| CaseIgnoreEngStrChr('e', perm, perm_end - perm) != nullptr)
			{
				file_info.mode|= 0444;
				if ((file_info.mode & S_IFMT) == S_IFDIR) {
					file_info.mode|= 0111;
				}
			}
			if (CaseIgnoreEngStrChr('w', perm, perm_end - perm) != nullptr
				|| CaseIgnoreEngStrChr('a', perm, perm_end - perm) != nullptr
				|| CaseIgnoreEngStrChr('c', perm, perm_end - perm) != nullptr)
			{
				file_info.mode|= 0220;
			}
			if (CaseIgnoreEngStrChr('x', perm, perm_end - perm) != nullptr
				|| CaseIgnoreEngStrChr('e', perm, perm_end - perm) != nullptr)
			{
				file_info.mode|= 0111;
			}

		} else if ((file_info.mode & S_IFMT) == S_IFDIR) {
			file_info.mode|= DEFAULT_ACCESS_MODE_DIRECTORY; // last resort

		} else {
			file_info.mode|= DEFAULT_ACCESS_MODE_FILE; // last resort
		}
	}

	if (has_modify) {
		file_info.access_time.tv_sec = file_info.modification_time.tv_sec;
		if (!has_create) {
			file_info.status_change_time.tv_sec = file_info.modification_time.tv_sec;
		}

	} else if (has_create) {
		file_info.access_time.tv_sec = file_info.status_change_time.tv_sec;
		if (!has_modify) {
			file_info.modification_time.tv_sec = file_info.status_change_time.tv_sec;
		}

	} else {
		file_info.status_change_time.tv_sec =
			file_info.modification_time.tv_sec =
				file_info.access_time.tv_sec = GetDefaultTime();
	}

	return has_any_known;
}
