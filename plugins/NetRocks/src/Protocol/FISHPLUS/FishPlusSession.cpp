#include <stdlib.h>
#include <string.h>
#include <base64.h>
#include <utils.h>
#include "FishPlusSession.h"
#include "FishPlusScript.h"
#include "../../Erroring.h"

namespace FishPlus
{
	// A motd is long; it is not endless.
	static const size_t MAX_BOOTSTRAP_LINES = 1000;

	static std::string TrimEOL(const std::string &s)
	{
		size_t end = s.size();
		while (end && (s[end - 1] == '\n' || s[end - 1] == '\r')) {
			--end;
		}
		return s.substr(0, end);
	}

	////////////////////////////////////////////////////////////////////////

	bool Response::HasLine(const char *what) const
	{
		for (const auto &line : lines) {
			if (line == what) {
				return true;
			}
		}
		return false;
	}

	////////////////////////////////////////////////////////////////////////

	void Features::Parse(const std::string &tail)
	{
		_raw = tail;
		_names.clear();
		std::vector<std::string> parts;
		StrExplode(parts, tail, " \t");
		for (const auto &p : parts) {
			if (!p.empty()) {
				_names.insert(p);
			}
		}
	}

	bool Features::Has(const char *name) const
	{
		return _names.find(name) != _names.end();
	}

	std::string Features::Valued(const char *prefix) const
	{
		const size_t len = strlen(prefix);
		for (const auto &n : _names) {
			if (n.size() > len && memcmp(n.c_str(), prefix, len) == 0) {
				return n.substr(len);
			}
		}
		return std::string();
	}

	////////////////////////////////////////////////////////////////////////

	Session::Session(std::shared_ptr<WayToShell> way)
		: _way(way), _token(NewToken())
	{
	}

	Session::~Session()
	{
		// Nothing to say goodbye with: when the client closes its side the
		// helper's read hits EOF and the remote shell exits on its own, so a
		// hung remote can never block teardown. WayToShell does the closing.
	}

	void Session::SendRaw(const std::string &data)
	{
		try {
			_way->Send(data.c_str(), data.size());
		} catch (...) {
			_broken = true;
			throw;
		}
	}

	std::string Session::EncodePathLine(const std::string &path)
	{
		bool needs_escape = path.empty() || path[0] == '~';
		if (!needs_escape) {
			needs_escape = (path.find('\n') != std::string::npos
				|| path.find('\r') != std::string::npos);
		}
		if (!needs_escape) {
			return path;
		}
		std::string out("~");
		base64_encode(out, (const unsigned char *)path.c_str(), path.size());
		return out;
	}

	void Session::Handshake(const char *helper_path, bool tty_transport)
	{
		const std::string script = LoadHelperScript(helper_path, _token);

		SendRaw(BootstrapLine(_token));

		// Everything printed before the marker - motd, shell warnings, login
		// banners - is noise and gets discarded. The marker carries the session
		// token so that noise cannot be mistaken for it.
		const std::string marker = ReadyMarker(_token);
		std::string ready_pattern = "*" + marker + "*";
		std::vector<const char *> ready_replies{ready_pattern.c_str()};
		for (size_t i = 0;; ++i) {
			if (i >= MAX_BOOTSTRAP_LINES) {
				_broken = true;
				throw ProtocolError("FISH+: the remote shell never reported being ready");
			}
			auto wr = _way->WaitReply(ready_replies);
			if (wr.index == 0) {
				break;
			}
		}

		// Nothing is in flight while the shell's parser is working, so the
		// script can now be fed in through the bootstrap's read loop.
		SendRaw(script + HELPER_END_MARKER + "\n");

		Response resp = ReadResponse(0, false);
		if (!resp.ok) {
			_broken = true;
			throw ProtocolError("FISH+ handshake refused by remote host", resp.msg.c_str());
		}

		// Banner: "FISHPLUS <version> <feature> ..."
		std::string banner = resp.msg;
		StrTrim(banner, " \t\r\n");
		std::vector<std::string> fields;
		StrExplode(fields, banner, " \t");
		if (fields.size() < 2 || fields[0] != "FISHPLUS") {
			_broken = true;
			throw ProtocolError("FISH+: unexpected handshake banner", banner.c_str());
		}
		const int proto = atoi(fields[1].c_str());
		if (proto != PROTOCOL_VERSION) {
			_broken = true;
			throw ProtocolError("FISH+: unsupported protocol version on remote host", banner.c_str());
		}
		std::string tail;
		for (size_t i = 2; i < fields.size(); ++i) {
			if (!tail.empty()) {
				tail += ' ';
			}
			tail += fields[i];
		}
		_feats.Parse(tail);

		// A shell that ended up on a pseudo terminal echoes back everything it
		// is fed and turns every \n on the way out into \r\n, which destroys
		// binary frames. The helper tames such a terminal with POSIX stty and
		// announces "tty" when it managed to. A terminal backed transport whose
		// helper did not manage it cannot carry raw payload at all.
		_raw_payload_safe = (!tty_transport || _feats.Has("tty"));

		fprintf(stderr, "[FISH+] connected, proto %d, feats:%s%s\n", proto,
			_feats.Raw().empty() ? " (none)" : "", _feats.Raw().c_str());
		if (!_raw_payload_safe) {
			fprintf(stderr, "[FISH+] terminal transport not tamed - binary payload will be base64 encoded\n");
		}
	}

	Response Session::ReadResponse(unsigned long long id, bool binary)
	{
		const std::string prefix = std::string(".") + _token + " " + ToDec(id) + " ";
		// The handshake is the one place where the terminator may not start its
		// line: a motd, a shell warning or the echo of the uploaded script on a
		// pseudo terminal can end without a newline and glue itself to the
		// banner. Later responses are strict - the helper controls every byte
		// by then.
		const std::string term_pattern = (id == 0) ? ("*" + prefix + "*") : (prefix + "*");
		Response resp;

		for (;;) {
			std::vector<const char *> replies;
			replies.push_back(term_pattern.c_str());
			if (binary) {
				replies.push_back("#*");
			}

			WaitResult wr;
			try {
				wr = _way->WaitReply(replies);
			} catch (...) {
				_broken = true;
				throw;
			}

			std::vector<std::string> &got =
				(wr.output_type == STDERR) ? wr.stderr_lines : wr.stdout_lines;
			if (got.empty()) {
				_broken = true;
				throw ProtocolError("FISH+: empty reply from remote host");
			}

			// Everything before the match is payload; the matched line is last.
			const std::string matched = TrimEOL(got.back());
			got.pop_back();
			if (id != 0) {
				for (auto &line : got) {
					resp.lines.push_back(TrimEOL(line));
				}
			}

			if (wr.index == 0) {
				// id 0 tolerates leading noise glued to the terminator, so the
				// prefix is looked up rather than assumed to start the line.
				const size_t at = matched.find(prefix);
				if (at == std::string::npos) {
					_broken = true;
					throw ProtocolError("FISH+: bad terminator from remote host", matched.c_str());
				}
				const std::string tail = matched.substr(at + prefix.size());
				std::string status = tail, msg;
				const size_t sp = tail.find(' ');
				if (sp != std::string::npos) {
					status = tail.substr(0, sp);
					msg = tail.substr(sp + 1);
				}
				StrTrim(status, " \t\r\n");
				StrTrim(msg, " \t\r\n");
				if (status != "ok" && status != "err") {
					_broken = true;
					throw ProtocolError("FISH+: bad terminator from remote host", matched.c_str());
				}
				resp.ok = (status == "ok");
				resp.msg = msg;
				return resp;
			}

			// A binary frame: header line then exactly n raw bytes. WaitReply
			// consumed the header and nothing past it, so what follows in the
			// transport buffer is the payload itself.
			const long long n = atoll(matched.c_str() + 1);
			if (n < 0 || (size_t)n > MAX_FRAME_LEN) {
				_broken = true;
				throw ProtocolError("FISH+: bad data frame header", matched.c_str());
			}
			if (n > 0) {
				const size_t was = resp.data.size();
				resp.data.resize(was + (size_t)n);
				try {
					_way->ReadStdout(&resp.data[was], (size_t)n);
				} catch (...) {
					_broken = true;
					throw;
				}
			}
		}
	}

	Response Session::ExecFull(const char *cmd, const std::vector<std::string> &paths,
		const std::vector<std::string> &args,
		const void *payload, size_t payload_len, bool encoded, bool binary)
	{
		if (_broken) {
			throw ProtocolError("FISH+ session is out of sync");
		}
		for (const auto &arg : args) {
			// Short arguments are bare tokens by construction. Anything else
			// would shift the request line and desynchronize the stream.
			if (arg.empty() || arg.find_first_of(" \t\r\n") != std::string::npos) {
				throw ProtocolError("FISH+: invalid argument for command", cmd);
			}
		}

		++_seq;
		const unsigned long long id = _seq;

		std::string req = ToDec(id);
		req += ' ';
		req += cmd;
		for (const auto &arg : args) {
			req += ' ';
			req += arg;
		}
		req += '\n';
		for (const auto &p : paths) {
			req += EncodePathLine(p);
			req += '\n';
		}
		if (encoded) {
			// The line is written even for an empty payload: the helper reads
			// one line per encoded request and would otherwise wait for a line
			// that never comes.
			if (payload_len) {
				base64_encode(req, (const unsigned char *)payload, payload_len);
			}
			req += '\n';
		}

		SendRaw(req);

		if (!encoded && payload_len) {
			// The raw payload carries no terminator of its own: the helper
			// reads exactly as many bytes as the request announced, so a stray
			// byte here would end up at the head of the next request.
			try {
				_way->Send((const char *)payload, payload_len);
			} catch (...) {
				_broken = true;
				throw;
			}
		}

		return ReadResponse(id, binary);
	}

	Response Session::Exec(const char *cmd, const std::vector<std::string> &args)
	{
		return ExecFull(cmd, {}, args, nullptr, 0, false, false);
	}

	Response Session::ExecPath(const char *cmd, const std::string &path,
		const std::vector<std::string> &args)
	{
		return ExecFull(cmd, {path}, args, nullptr, 0, false, false);
	}

	Response Session::ExecPaths(const char *cmd, const std::vector<std::string> &paths,
		const std::vector<std::string> &args)
	{
		return ExecFull(cmd, paths, args, nullptr, 0, false, false);
	}

	Response Session::ExecPathData(const char *cmd, const std::string &path,
		const std::vector<std::string> &args)
	{
		return ExecFull(cmd, {path}, args, nullptr, 0, false, true);
	}

	Response Session::ExecPayload(const char *cmd, const std::vector<std::string> &paths,
		const std::vector<std::string> &args,
		const void *payload, size_t payload_len, bool encoded)
	{
		return ExecFull(cmd, paths, args, payload, payload_len, encoded, false);
	}

	void Session::ThrowIfFailed(const Response &resp, const char *what, const std::string &path)
	{
		if (resp.ok) {
			return;
		}
		std::string info = path;
		if (!resp.msg.empty()) {
			info += ": ";
			info += resp.msg;
		}
		throw ProtocolError(what, info.c_str());
	}
}
