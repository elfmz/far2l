#include <stdio.h>
#include <stdlib.h>
#include <utils.h>

#include "Parse.h"
#include "../ShellParseUtils.h"
#include "../Protocol.h"

static std::string FetchTail(std::string &line)
{
	return ShellParseUtils::ExtractStringTail(line, " \t");
}

static std::string FetchHead(std::string &line)
{
	return ShellParseUtils::ExtractStringHead(line, " \t");
}

////

static void SHELLParseLSPermissions(FileInfo &fi, const char *line, size_t line_len)
{
//	drwxr-xr-x 0 0
	fi.mode = ShellParseUtils::Str2Mode(line, line_len);
	for (size_t i = line_len, j = line_len; i--;) {
		if (line[i] == ' ' || line[i] == '.') {
			if (j > i) {
				if (fi.group.empty()) {
					fi.group.assign(&line[i] + 1, j - i - 1);
				} else {
					fi.owner.assign(&line[i] + 1, j - i - 1);
					break;
				}
				j = i;
			}
		}
	}
}

static unsigned int ParseModeByStatOrFindLine(const std::string &s)
{
	bool hexadec = !s.empty();
	for (const auto &c : s) {
		if ( (c < '0' || c > '9') && (c < 'a' || c > 'f') && (c < 'A' || c > 'F')) {
			hexadec = false;
			break;
		}
	}

	if (hexadec) {
		return strtoul(s.c_str(), nullptr, 16);
	}

	return ShellParseUtils::Str2Mode(s.c_str(), s.size());
}

static bool SHELLParseEnumByStatOrFindLine(FileInfo &fi, std::string line)
{
	StrTrim(line, " \t\n");
	// NAME MODE SIZE TIME_ACC TIME_MOD TIME_ST OWNER GROUP
	//. 41ed 4096 1694886719 1691156361 1691156361 root root
	fi.group = FetchTail(line);
	fi.owner = FetchTail(line);
	fi.status_change_time = timespec{(time_t)strtoull(FetchTail(line).c_str(), nullptr, 10), 0};
	fi.modification_time = timespec{(time_t)strtoull(FetchTail(line).c_str(), nullptr, 10), 0};
	fi.access_time = timespec{(time_t)strtoull(FetchTail(line).c_str(), nullptr, 10), 0};
	fi.size = (uint64_t)strtoull(FetchTail(line).c_str(), nullptr, 10);
	fi.mode = ParseModeByStatOrFindLine(FetchTail(line));
	StrTrim(line, " \t\n");
	const size_t p = line.rfind('/');
	if (p != std::string::npos) {
		fi.path = line.substr(p + 1);
	} else {
		fi.path.swap(line);
	}
	return !fi.path.empty() && fi.path != "." && fi.path != "..";
}

void SHELLParseEnumByStatOrFind(std::vector<FileInfo> &files, const std::vector<std::string> &lines)
{
	for (const auto &line : lines) {
		if (!line.empty()) {
			files.emplace_back();
			if (!SHELLParseEnumByStatOrFindLine(files.back(), line)) {
				files.pop_back();
			}
		}
	}
}

void SHELLParseInfoByStatOrFind(FileInformation &fi, std::string &line)
{
	StrTrim(line, " \t\n");
	// MODE SIZE TIME_ACC TIME_MOD TIME_ST
	// 41ed 4096 1694886719 1691156361 1691156361
	fi.status_change_time = timespec{(time_t)strtoull(FetchTail(line).c_str(), nullptr, 10), 0};
	fi.modification_time = timespec{(time_t)strtoull(FetchTail(line).c_str(), nullptr, 10), 0};
	fi.access_time = timespec{(time_t)strtoull(FetchTail(line).c_str(), nullptr, 10), 0};
	fi.size = (uint64_t)strtoull(FetchTail(line).c_str(), nullptr, 10);
	fi.mode = ParseModeByStatOrFindLine(FetchTail(line));
}

uint64_t SHELLParseSizeByStatOrFind(std::string &line)
{
	StrTrim(line, " \t\n");
	return (uint64_t)strtoull(FetchTail(line).c_str(), nullptr, 10);
}

uint32_t SHELLParseModeByStatOrFind(std::string &line)
{
	StrTrim(line, " \t\n");
	return ParseModeByStatOrFindLine(FetchTail(line));
}

////////////////
// drwxr-xr-x 2 root root 73728 Sep 16 06:59 "/bin"
void SHELLParseEnumByLS(std::vector<FileInfo> &files, const std::vector<std::string> &lines)
{
	for (std::string line : lines) {
		files.emplace_back();
		auto &fi = files.back();
		StrTrim(line, " \t\n");
		if (!ShellParseUtils::ParseLineFromLS(line,
				fi.path, fi.owner, fi.group,
				fi.access_time, fi.modification_time, fi.status_change_time,
				fi.size, fi.mode) || !FILENAME_ENUMERABLE(fi.path)) {
			files.pop_back();
		}
	}
}

void SHELLParseInfoByLS(FileInformation &fi, std::string &line)
{
	StrTrim(line, " \t\n");
	std::string name, owner, group;
	ShellParseUtils::ParseLineFromLS(line, name, owner, group,
		fi.access_time, fi.modification_time, fi.status_change_time, fi.size, fi.mode);
}

uint64_t SHELLParseSizeByLS(std::string &line)
{
	StrTrim(line, " \t\n");
	return (uint64_t)strtoull(line.c_str(), nullptr, 10);
}

uint32_t SHELLParseModeByLS(std::string &line)
{
	StrTrim(line, " \t\n");
	return ParseModeByStatOrFindLine(line);
}

void AppendTrimmedLines(std::string &s, const std::vector<std::string> &lines)
{
	for (auto line : lines) {
		StrTrim(line, " \t\n\r");
		if (!line.empty()) {
			if (!s.empty()) {
				s+= '\n';
			}
			s+= line;
		}
	}
}

void Substitute(std::string &str, const char *pattern, const std::string &replacement)
{
	for (size_t ofs = 0; ofs < str.size();) {
		const size_t p = str.find(pattern, ofs);
		if (p == std::string::npos) {
			break;
		}
		str.replace(p, strlen(pattern), replacement);
		ofs = p + replacement.size();
	}
}
