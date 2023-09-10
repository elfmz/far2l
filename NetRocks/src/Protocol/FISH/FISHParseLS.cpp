#include <stdio.h>
#include <stdlib.h>

#include "FISHParse.h"
#include "../ShellParseUtils.h"

static void FISHParseLSPermissions(FileInfo &fi, const char *line, size_t line_len)
{
//	drwxr-xr-x 0 0
	fi.mode = ShellParseUtils::Str2Mode(line, line_len);
	for (size_t i = line_len, j = line_len; i--;) {
		if (line[i] == ' ') {
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

static void FISHParseLSLine(std::vector<FileInfo> &files, const char *line, size_t line_len)
{
fprintf(stderr, "FISHLS: '%.*s'\n", (int)line_len, line);
	switch (*line) {
		case ':':
			files.back().path.assign(line + 1, line_len - 1);
			files.emplace_back();
			break;

		case 'S':
			files.back().size = (uint64_t)strtoull(line + 1, nullptr, 10);
			break;

		case 'P':
			FISHParseLSPermissions(files.back(), line + 1, line_len - 1);
			break;
		// ... другие case ...
		default:
			;
	}
}

void FISHParseLS(std::vector<FileInfo> &files, const std::string &buffer)
{
	files.emplace_back();
	for (size_t ofs = 0; ofs < buffer.size();) {
		size_t eol = buffer.find('\n', ofs);
		if (eol == std::string::npos) {
			eol = buffer.size();
		}
		if (eol > ofs) {
			FISHParseLSLine(files, buffer.c_str() + ofs, eol - ofs);
		}
		while (eol < buffer.size() && buffer[eol] == '\n') {
			++eol;
		}
		ofs = eol;
	}
	if (files.back().path.empty()) {
		files.pop_back();
	}
}
