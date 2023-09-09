#include <stdio.h>
#include <stdlib.h>

#include "FISHParse.h"

static void FISHParseLSLine(std::vector<FileInfo> &files, const char *line, size_t line_len)
{
	switch (*line) {
		case ':': {
			files.emplace_back();
			// Парсинг имени файла и пути символической ссылки
			files.back().path.assign(line + 1, line_len - 1);
			size_t arrow_pos = files.back().path.rfind(" -> ");
			if (arrow_pos != std::string::npos) {
				files.back().symlink_path = files.back().path.substr(arrow_pos + 4);
				files.back().path.resize(arrow_pos);
			}
			break;
		}

		case 'S':
			if (!files.empty()) {
				files.back().size = (uint64_t)strtoull(line + 1, nullptr, 10);
			}
			break;
		// ... другие case ...
		default:
			;
	}
}

void FISHParseLS(std::vector<FileInfo> &files, const std::string &buffer)
{
	for (size_t ofs = 0; ofs < buffer.size();) {
		size_t eol = buffer.find_first_of("\r\n", ofs);
		if (eol == std::string::npos) {
			eol = buffer.size();
		}
		if (eol > ofs) {
			FISHParseLSLine(files, buffer.c_str() + ofs, eol - ofs);
		}
		while (eol < buffer.size() && (buffer[eol] == '\r' || buffer[eol] == '\n')) {	
			++eol;
		}
		ofs = eol;
	}
}
