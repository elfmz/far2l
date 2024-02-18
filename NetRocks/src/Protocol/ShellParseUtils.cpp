#include <sys/stat.h>
#include <time.h>
#include <string.h>
#include <utils.h>

#include "ShellParseUtils.h"

namespace ShellParseUtils
{
	std::string ExtractStringTail(std::string &line, const char *separators)
	{
		std::string out;
		size_t p = line.find_last_of(separators);
		if (p != std::string::npos) {
			out = line.substr(p + 1);
			line.resize(p);
		} else {
			out.swap(line);
		}

		return out;
	}

	std::string ExtractStringHead(std::string &line, const char *separators)
	{
		std::string out;
		size_t p = line.find_first_of(separators);
		if (p != std::string::npos) {
			out = line.substr(0, p);
			while (p < line.size() && strchr(separators, line[p])) {
				++p;
			}
			line.erase(0, p);

		} else {
			out.swap(line);
		}

		return out;
	}

////////////////////
	unsigned int Char2FileType(char c)
	{
		switch (c) {
			case 'l':
				return S_IFLNK;

			case 'd':
				return S_IFDIR;

			case 'c':
				return S_IFCHR;

			case 'b':
				return S_IFBLK;

			case 'p':
				return S_IFIFO;

			case 's':
				return S_IFSOCK;

			case 'f':
			default:
				return S_IFREG;
		}
	}

	unsigned int Triplet2FileMode(const char *c)
	{
		unsigned int out = 0;
		if (c[0] == 'r') out|= 4;
		if (c[1] == 'w') out|= 2;
		if (c[2] == 'x' || c[1] == 's' || c[1] == 't') out|= 1;
		return out;
	}

	unsigned int Str2Mode(const char *str, size_t len)
	{
		unsigned int mode = 0;
		if (len >= 1) {
			mode = Char2FileType(*str);
		}

		if (len >= 4) {
			mode|= Triplet2FileMode(str + 1) << 6;
		}

		if (len >= 7) {
			mode|= Triplet2FileMode(str + 4) << 3;
		}

		if (len >= 10) {
			mode|= Triplet2FileMode(str + 7);
		}

		return mode;
	}

	bool ParseLineFromLS(std::string &line,
		std::string &name, std::string &owner, std::string &group,
		timespec &access_time, timespec &modification_time, timespec &status_change_time,
		unsigned long long &size, mode_t &mode)
	{
		const std::string &str_mode = ExtractStringHead(line, " \t");
		if (line.empty())
			return false;

		if (line[0] >= '0' && line[0] <= '9') {
			ExtractStringHead(line, " \t"); // skip nlinks
			if (line.empty())
				return false;
		}
		std::string str_owner = ExtractStringHead(line, " \t");
		if (line.empty())
			return false;

		std::string str_group = ExtractStringHead(line, " \t");
		if (line.empty())
			return false;

		const std::string &str_size = ExtractStringHead(line, " \t");
		if (line.empty())
			return false;

		bool size_is_not_size = false;
		if (!str_size.empty() && str_size.back() == ',') {
			// block/char device, size is major, followed by minor
			ExtractStringHead(line, " \t"); // really dont care
			if (line.empty())
				return false;
			size_is_not_size = true;
		}

		const std::string &str_month = ExtractStringHead(line, " \t");

		if (line.empty())
			return false;

		const std::string &str_day = ExtractStringHead(line, " \t");
		if (line.empty())
			return false;

		const std::string &str_yt = ExtractStringHead(line, " \t");
		if (line.empty())
			return false;

		mode = ShellParseUtils::Str2Mode(str_mode.c_str(), str_mode.size());
		if (S_ISLNK(mode)) {
			size_t p = line.find(" -> ");
			if (p != std::string::npos)
				line.resize(p);
		}

		static const time_t now = time(NULL);
		struct tm t{}, tn{};
		struct tm *tnow = gmtime(&now);
		if (tnow) {
			tn = *tnow;
			t = *tnow;
		}
		bool has_year = false;
		if (str_yt.find(':') == std::string::npos) {
			has_year = true;
			t.tm_year = DecToULong(str_yt.c_str(), str_yt.length()) - 1900;
		} else if (sscanf(str_yt.c_str(), "%d:%d", &t.tm_hour, &t.tm_min) <= -1) {
			perror("scanf(str_yt)");
		}

		if (strcasecmp(str_month.c_str(), "jan") == 0) t.tm_mon = 0;
		else if (strcasecmp(str_month.c_str(), "feb") == 0) t.tm_mon = 1;
		else if (strcasecmp(str_month.c_str(), "mar") == 0) t.tm_mon = 2;
		else if (strcasecmp(str_month.c_str(), "apr") == 0) t.tm_mon = 3;
		else if (strcasecmp(str_month.c_str(), "may") == 0) t.tm_mon = 4;
		else if (strcasecmp(str_month.c_str(), "jun") == 0) t.tm_mon = 5;
		else if (strcasecmp(str_month.c_str(), "jul") == 0) t.tm_mon = 6;
		else if (strcasecmp(str_month.c_str(), "aug") == 0) t.tm_mon = 7;
		else if (strcasecmp(str_month.c_str(), "sep") == 0) t.tm_mon = 8;
		else if (strcasecmp(str_month.c_str(), "oct") == 0) t.tm_mon = 9;
		else if (strcasecmp(str_month.c_str(), "nov") == 0) t.tm_mon = 10;
		else if (strcasecmp(str_month.c_str(), "dec") == 0) t.tm_mon = 11;
		if (!has_year && t.tm_mon > tn.tm_mon && t.tm_year) {
			--t.tm_year;
		}

		t.tm_mday = atoi(str_day.c_str());

		status_change_time.tv_sec
			= modification_time.tv_sec
				= access_time.tv_sec = mktime(&t);
		status_change_time.tv_nsec
			= modification_time.tv_nsec
				= access_time.tv_nsec = 0;

		size = size_is_not_size ? 0 : atoll(str_size.c_str());

		owner.swap(str_owner);
		group.swap(str_group);

		size_t p = line.rfind('/');
		if (p != std::string::npos && line.size() > 1) {
			line.erase(0, p + 1);
		}
		name.swap(line);

		return true;
	}
}
