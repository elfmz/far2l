#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <fstream>
#include <random>
#include <RandomString.h>
#include <base64.h>
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

	// The ready marker cannot appear as one contiguous run of bytes anywhere in
	// the encoded blob: a terminal echoing this very command back would then
	// look like the shell answering. base64 output cannot contain quotes on its
	// own, so splitting the marker with a doubled PowerShell single-quote (which
	// the PowerShell parser reads as one literal quote) both breaks the string
	// and stays inside the '...' literal.
	//
	// The pattern is extraordinarily unlikely - the marker is nine ASCII
	// characters and the base64 alphabet is A-Za-z0-9+/= - but the collision
	// would be hard to reproduce and even harder to debug, so it is filtered
	// out unconditionally, matching what f4's Go client does.
	static std::string SplitReadyMarkerInB64(const std::string &s, const std::string &token)
	{
		const std::string marker = ReadyMarker(token);
		const std::string replacement = "F''4RDY" + token;
		std::string out;
		out.reserve(s.size());
		size_t pos = 0;
		for (;;) {
			const size_t p = s.find(marker, pos);
			if (p == std::string::npos) {
				out.append(s, pos, std::string::npos);
				break;
			}
			out.append(s, pos, p - pos);
			out.append(replacement);
			pos = p + marker.size();
		}
		return out;
	}

	std::string BootstrapLinePwshB64(const std::string &token,
		const std::string &compact_helper)
	{
		// The prefix is a PowerShell comment that decodes to nothing but lets
		// a wire dump identify what the base64 payload is.
		std::string payload = "# F4B64" + token + "\n" + compact_helper;

		std::string encoded;
		base64_encode(encoded, (const unsigned char *)payload.c_str(), payload.size());
		encoded = SplitReadyMarkerInB64(encoded, token);

		std::string out;
		out.reserve(encoded.size() + 256);
		out += "$F4B='";
		out += encoded;
		out += "'; Write-Output ('F4R'+'DY'+'";
		out += token;
		out += "'); try { $F4S=[System.Text.Encoding]::UTF8.GetString"
		       "([System.Convert]::FromBase64String($F4B)); "
		       "Remove-Variable F4B -Force -ErrorAction SilentlyContinue; "
		       "Invoke-Expression $F4S } catch { Write-Output ('.' + '";
		out += token;
		out += "' + ' 0 err bootstrap ' + $_.Exception.Message"
		       ".Replace([char]10,' ').Replace([char]13,' ')) }\n";
		return out;
	}
}
