#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <fstream>
#include <random>
#include <RandomString.h>
#include "FishPlusScript.h"
#include "../../Erroring.h"

namespace FishPlus
{
	const char TOKEN_PLACEHOLDER[] = "__F4_TOKEN__";
	const char HELPER_END_MARKER[] = "F4EOF";

	std::string NewToken()
	{
		std::string out;
		RandomStringAppend(out, 16, 16, RNDF_DIGITS);
		return out;
	}

	std::string ReadyMarker(const std::string &token)
	{
		return std::string("F4RDY") + token;
	}

	std::string Compact(const std::string &src)
	{
		std::string out;
		out.reserve(src.size());
		for (size_t pos = 0; pos < src.size();) {
			size_t eol = src.find('\n', pos);
			size_t end = (eol == std::string::npos) ? src.size() : eol;
			std::string line = src.substr(pos, end - pos);
			pos = (eol == std::string::npos) ? src.size() : eol + 1;

			// A CRLF checkout would otherwise send the carriage return to the
			// remote shell as part of every command.
			if (!line.empty() && line[line.size() - 1] == '\r') {
				line.resize(line.size() - 1);
			}
			size_t at = line.find_first_not_of(" \t");
			if (at == std::string::npos) {
				continue;
			}
			line.erase(0, at);
			if (line[0] == '#') {
				continue;
			}
			out += line;
			out += '\n';
		}
		return out;
	}

	static void SubstituteAll(std::string &str, const char *pattern, const std::string &replacement)
	{
		const size_t pattern_len = strlen(pattern);
		for (size_t ofs = 0; ofs < str.size();) {
			const size_t p = str.find(pattern, ofs);
			if (p == std::string::npos) {
				break;
			}
			str.replace(p, pattern_len, replacement);
			ofs = p + replacement.size();
		}
	}

	std::string LoadHelperScript(const char *path, const std::string &token)
	{
		std::ifstream ifs;
		ifs.open(path, std::ios::binary);
		if (!ifs.is_open()) {
			throw ProtocolError("can't open FISH+ helper", path, errno);
		}
		std::string src((std::istreambuf_iterator<char>(ifs)), std::istreambuf_iterator<char>());
		if (src.empty()) {
			throw ProtocolError("empty FISH+ helper", path);
		}
		if (src.find(TOKEN_PLACEHOLDER) == std::string::npos) {
			// Refuse rather than upload a helper whose terminator we cannot
			// recognize: every reply would then look like payload.
			throw ProtocolError("FISH+ helper carries no token placeholder", path);
		}
		SubstituteAll(src, TOKEN_PLACEHOLDER, token);
		return Compact(src);
	}

	std::string BootstrapLine(const std::string &token)
	{
		std::string out;
		out += "echo F4R\"DY\"";
		out += token;
		out += "; F4NL=$(printf '\\nx'); F4NL=${F4NL%x}; F4S=; ";
		out += "while IFS= read -r F4L; do [ \"$F4L\" = ";
		out += HELPER_END_MARKER;
		out += " ] && break; ";
		out += "F4S=$F4S$F4L$F4NL; done; eval \"$F4S\"\n";
		return out;
	}
}
