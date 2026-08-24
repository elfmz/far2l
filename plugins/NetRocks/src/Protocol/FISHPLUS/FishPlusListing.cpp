#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <utils.h>
#include "FishPlusListing.h"

namespace FishPlus
{
	static bool SplitN(const std::string &line, size_t n, std::vector<std::string> &out)
	{
		// Splits on single spaces into at most n fields, the last one keeping
		// whatever is left - which is how a name with spaces survives.
		out.clear();
		size_t pos = 0;
		while (out.size() + 1 < n) {
			const size_t sp = line.find(' ', pos);
			if (sp == std::string::npos) {
				break;
			}
			out.push_back(line.substr(pos, sp - pos));
			pos = sp + 1;
		}
		out.push_back(line.substr(pos));
		return out.size() == n;
	}

	// Understands both plain seconds ("1785869231") and the fractional form
	// GNU find prints ("1785869231.1084370490").
	static bool ParseEpoch(const std::string &s, timespec &ts)
	{
		if (s.empty()) {
			return false;
		}
		char *end = nullptr;
		const long long sec = strtoll(s.c_str(), &end, 10);
		if (end == s.c_str()) {
			return false;
		}
		long nsec = 0;
		if (*end == '.') {
			std::string frac(end + 1);
			// Keep at most nanosecond resolution, pad shorter fractions.
			if (frac.size() > 9) {
				frac.resize(9);
			}
			while (frac.size() < 9) {
				frac += '0';
			}
			nsec = strtol(frac.c_str(), nullptr, 10);
		}
		ts.tv_sec = (time_t)sec;
		ts.tv_nsec = nsec;
		return true;
	}

	static mode_t TypeCharMode(char c)
	{
		switch (c) {
			case 'f': return S_IFREG;
			case 'd': return S_IFDIR;
			case 'l': return S_IFLNK;
			case 'b': return S_IFBLK;
			case 'c': return S_IFCHR;
			case 'p': return S_IFIFO;
			case 's': return S_IFSOCK;
		}
		return 0;
	}

	static std::string BaseName(const std::string &path)
	{
		const size_t p = path.rfind('/');
		return (p == std::string::npos) ? path : path.substr(p + 1);
	}

	// Applies the trailing-slash and base-name rules shared by the stat and ls
	// backends.
	static void FinishName(std::string name, bool keep_path, Entry &e)
	{
		while (name.size() > 1 && name[name.size() - 1] == '/') {
			name.resize(name.size() - 1);
		}
		if (name.empty()) {
			name = "/";
		} else if (!keep_path) {
			name = BaseName(name);
		}
		e.name = name;
	}

	// find -printf '%y %Y %s %T@ %A@ %C@ %m %U %G %f\n'
	static bool ParseFindEntry(const std::string &line, bool keep_path, Entry &e)
	{
		std::vector<std::string> f;
		if (!SplitN(line, 10, f) || f[0].empty()) {
			return false;
		}
		const mode_t type = TypeCharMode(f[0][0]);
		const unsigned long perm = strtoul(f[6].c_str(), nullptr, 8);
		if (!ParseEpoch(f[3], e.mtime) || !ParseEpoch(f[4], e.atime) || !ParseEpoch(f[5], e.ctime)) {
			return false;
		}
		e.size = strtoull(f[2].c_str(), nullptr, 10);
		e.mode = (mode_t)perm | type;
		e.uid = atoi(f[7].c_str());
		e.gid = atoi(f[8].c_str());
		e.target_is_dir = (f[1] == "d");
		// find prints %f, already a bare name; keep_path callers get the path
		// the search handed back instead.
		FinishName(f[9], keep_path, e);
		return !e.name.empty();
	}

	// GNU  stat -c '%f %s %Y %X %Z %u %g %n'   - mode in hex
	// BSD  stat -f '%p %z %m %a %c %u %g %N'   - mode in octal
	static bool ParseStatLike(const std::string &line, int mode_base, bool keep_path, Entry &e)
	{
		std::vector<std::string> f;
		if (!SplitN(line, 8, f)) {
			return false;
		}
		std::string mode_text = f[0];
		if (mode_base == 16 && mode_text.size() > 2
				&& mode_text[0] == '0' && (mode_text[1] == 'x' || mode_text[1] == 'X')) {
			mode_text.erase(0, 2);
		}
		char *end = nullptr;
		const unsigned long mode = strtoul(mode_text.c_str(), &end, mode_base);
		if (end == mode_text.c_str()) {
			return false;
		}
		if (!ParseEpoch(f[2], e.mtime) || !ParseEpoch(f[3], e.atime) || !ParseEpoch(f[4], e.ctime)) {
			return false;
		}
		e.size = strtoull(f[1].c_str(), nullptr, 10);
		e.mode = (mode_t)mode;
		e.uid = atoi(f[5].c_str());
		e.gid = atoi(f[6].c_str());
		FinishName(f[7], keep_path, e);
		return !e.name.empty();
	}

	////////////////////////////////////////////////////////////////////////
	// The ls backend exists for hosts that have neither GNU find nor either
	// stat: OpenWrt with BusyBox is the first one reported. ls is the one
	// listing tool that is always there, and the one whose output was never
	// meant to be parsed.
	//
	// Two things make it parseable at all. The timestamp is asked for in full,
	// because the short form drops the year on anything older than six months
	// and there is no way to guess it back. And the ids are asked for
	// numerically, because a user name can contain a space and would then be
	// indistinguishable from the column after it. What is left is a fixed
	// number of fields and then the name, however many spaces it has in it.

	static unsigned SpecialBit(int triple)
	{
		switch (triple) {
			case 0: return 04000;
			case 1: return 02000;
			default: return 01000;
		}
	}

	// Turns "drwxr-sr-t" into the bits a stat would have reported. A trailing
	// marker for extended attributes or an ACL, which macOS and Linux both add,
	// is ignored.
	static bool ParseLsMode(const std::string &s, mode_t &out)
	{
		if (s.size() < 10) {
			return false;
		}
		mode_t mode = 0;
		switch (s[0]) {
			case 'd': mode = S_IFDIR; break;
			case 'l': mode = S_IFLNK; break;
			case '-': mode = S_IFREG; break;
			case 'c': mode = S_IFCHR; break;
			case 'b': mode = S_IFBLK; break;
			case 'p': mode = S_IFIFO; break;
			case 's': mode = S_IFSOCK; break;
			default: return false;
		}
		for (int triple = 0; triple < 3; ++triple) {
			const unsigned shift = (unsigned)(6 - 3 * triple);
			if (s[1 + 3 * triple] == 'r') {
				mode |= (mode_t)(4u << shift);
			}
			if (s[2 + 3 * triple] == 'w') {
				mode |= (mode_t)(2u << shift);
			}
			switch (s[3 + 3 * triple]) {
				case 'x':
					mode |= (mode_t)(1u << shift);
					break;
				case 's': case 't':
					mode |= (mode_t)(1u << shift);
					mode |= (mode_t)SpecialBit(triple);
					break;
				case 'S': case 'T':
					mode |= (mode_t)SpecialBit(triple);
					break;
			}
		}
		out = mode;
		return true;
	}

	// How many fields come before the name in each dialect: mode, links, uid,
	// gid, size, and then the timestamp - one field as an epoch, three as a
	// full iso date and four in the BSD form.
	static size_t LsFieldCount(const std::string &variant)
	{
		if (variant == "bsd") {
			return 9;
		}
		if (variant == "iso") {
			return 8;
		}
		return 6;
	}

	static bool ParseLsTime(const std::vector<std::string> &fields, size_t from, size_t to,
		const std::string &variant, timespec &ts)
	{
		struct tm tm {};
		if (variant == "iso") {
			if (to - from != 3) {
				return false;
			}
			// "2026-05-14 09:11:02[.123456789] +0300"
			const std::string joined = fields[from] + " " + fields[from + 1];
			if (!strptime(joined.c_str(), "%Y-%m-%d %H:%M:%S", &tm)) {
				return false;
			}
			long zone_sec = 0;
			const std::string &zone = fields[from + 2];
			if (zone.size() >= 5 && (zone[0] == '+' || zone[0] == '-')) {
				const long hh = strtol(zone.substr(1, 2).c_str(), nullptr, 10);
				const long mm = strtol(zone.substr(3, 2).c_str(), nullptr, 10);
				zone_sec = (hh * 3600 + mm * 60) * ((zone[0] == '-') ? -1 : 1);
			}
			tm.tm_isdst = 0;
			ts.tv_sec = timegm(&tm) - zone_sec;
			ts.tv_nsec = 0;
			return true;
		}
		if (variant == "bsd") {
			if (to - from != 4) {
				return false;
			}
			// "Jan  2 15:04:05 2006" - the only dialect without a zone in it,
			// so it is read in the client's zone. A host elsewhere shows times
			// off by the difference, which is the price of that format.
			std::string joined = fields[from];
			for (size_t i = from + 1; i < to; ++i) {
				joined += " ";
				joined += fields[i];
			}
			if (!strptime(joined.c_str(), "%b %d %H:%M:%S %Y", &tm)) {
				return false;
			}
			tm.tm_isdst = -1;
			ts.tv_sec = mktime(&tm);
			ts.tv_nsec = 0;
			return true;
		}
		if (to - from != 1) {
			return false;
		}
		ts.tv_sec = (time_t)strtoll(fields[from].c_str(), nullptr, 10);
		ts.tv_nsec = 0;
		return true;
	}

	// Finds where the name starts by walking past the fixed fields in the
	// original line, so that a name made of several spaces survives.
	static std::string LsNameOf(const std::string &line,
		const std::vector<std::string> &fields, size_t n)
	{
		size_t at = 0;
		for (size_t i = 0; i < n; ++i) {
			const size_t idx = line.find(fields[i], at);
			if (idx == std::string::npos) {
				return std::string();
			}
			at = idx + fields[i].size();
		}
		// Exactly one space separates the last fixed column from the name: ls
		// pads inside a column, never between them. Trimming everything here
		// would eat the leading spaces of a name that begins with them.
		std::string rest = line.substr(at);
		if (!rest.empty() && (rest[0] == ' ' || rest[0] == '\t')) {
			rest.erase(0, 1);
		}
		return rest;
	}

	static bool ParseLsEntry(const std::string &line, const std::string &variant,
		bool keep_path, Entry &e)
	{
		if (StrStartsFrom(line, "total ")) {
			return false;
		}
		const size_t n = LsFieldCount(variant);
		std::vector<std::string> fields;
		StrExplode(fields, line, " \t");
		if (fields.size() <= n) {
			return false;
		}
		if (!ParseLsMode(fields[0], e.mode)) {
			return false;
		}
		e.uid = atoi(fields[2].c_str());
		e.gid = atoi(fields[3].c_str());
		e.size = strtoull(fields[4].c_str(), nullptr, 10);
		timespec when {};
		if (!ParseLsTime(fields, 5, n, variant, when)) {
			return false;
		}
		e.mtime = e.atime = e.ctime = when;

		std::string name = LsNameOf(line, fields, n);
		if (name.empty()) {
			return false;
		}
		// A symlink is printed with its target, which is not part of the name.
		if ((e.mode & S_IFMT) == S_IFLNK) {
			const size_t at = name.find(" -> ");
			if (at != std::string::npos) {
				name.resize(at);
			}
		}
		FinishName(name, keep_path, e);
		return !e.name.empty();
	}

	////////////////////////////////////////////////////////////////////////

	std::string ParseListing(const std::vector<std::string> &lines,
		std::vector<Entry> &out, bool keep_path)
	{
		if (lines.empty() || !StrStartsFrom(lines[0], "M ")) {
			return std::string();
		}
		std::string marker = lines[0].substr(2);
		StrTrim(marker, " \t\r\n");
		if (marker.empty()) {
			return std::string();
		}
		// The ls backend carries its time dialect in the marker, because the
		// dialects print timestamps differently and nothing else says which.
		std::string mode = marker, variant;
		const size_t sp = marker.find(' ');
		if (sp != std::string::npos) {
			mode = marker.substr(0, sp);
			variant = marker.substr(sp + 1);
			StrTrim(variant, " \t\r\n");
		}

		for (size_t i = 1; i < lines.size(); ++i) {
			const std::string &line = lines[i];
			if (line.empty()) {
				continue;
			}
			Entry e;
			bool ok;
			if (mode == "find") {
				ok = ParseFindEntry(line, keep_path, e);
			} else if (mode == "stat") {
				ok = ParseStatLike(line, 16, keep_path, e);
			} else if (mode == "statbsd") {
				ok = ParseStatLike(line, 8, keep_path, e);
			} else if (mode == "ls") {
				ok = ParseLsEntry(line, variant, keep_path, e);
			} else {
				return mode;
			}
			// A stray diagnostic line must not cost the user the panel.
			if (!ok || e.name.empty() || e.name == "." || e.name == "..") {
				continue;
			}
			out.push_back(e);
		}
		return mode;
	}

	bool ParseSingle(const std::vector<std::string> &lines, Entry &out)
	{
		std::vector<Entry> entries;
		// keep_path, because info answers about a path rather than about a
		// directory entry, and the caller may want to know what it got.
		if (ParseListing(lines, entries, true).empty() || entries.empty()) {
			return false;
		}
		out = entries[0];
		return true;
	}
}
