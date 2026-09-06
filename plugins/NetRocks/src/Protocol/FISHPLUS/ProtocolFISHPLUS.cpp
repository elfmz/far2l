#include <sys/stat.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <algorithm>
#include <KeyFileHelper.h>
#include <utils.h>
#include "ProtocolFISHPLUS.h"
#include "FishPlusListing.h"
#include "FishPlusScript.h"
#include "../SHELL/Parse.h"		// Substitute()
#include "../SHELL/WayToShellConfig.h"
#include "../../Erroring.h"

#define FISHPLUS_WAYS_INI	"FISHPLUS/ways.ini"
#define FISHPLUS_HELPER		"FISHPLUS/helper.sh"

std::shared_ptr<IProtocol> CreateProtocol(const std::string &protocol, const std::string &host, unsigned int port,
	const std::string &username, const std::string &password, const std::string &options, int fd_ipc_recv)
{
	return std::make_shared<ProtocolFISHPLUS>(host, port, username, password, options, fd_ipc_recv);
}

////////////////////////////////////////////////////////////////////////////

namespace
{
	// Both halves of the transfer path talk to one session, and the session
	// answers one request at a time, so a reader and a writer are simply two
	// users of the same shell.

	class FishPlusFileReader : public IFileReader
	{
		std::shared_ptr<FishPlus::Session> _sess;
		std::string _path;
		unsigned long long _pos;

	public:
		FishPlusFileReader(std::shared_ptr<FishPlus::Session> sess,
				const std::string &path, unsigned long long resume_pos)
			: _sess(sess), _path(path), _pos(resume_pos)
		{
		}

		virtual size_t Read(void *buf, size_t len)
		{
			if (len == 0) {
				return 0;
			}
			len = std::min(len, FishPlus::READ_CHUNK);

			auto resp = _sess->ExecPathData("read", _path,
				{ToDec(_pos), ToDec((unsigned long long)len)});
			FishPlus::Session::ThrowIfFailed(resp, "FISH+ read error", _path);

			// A short frame is the end of the file: the helper clamps the range
			// against the size it just read, so the frame length is exact.
			const size_t got = std::min(resp.data.size(), len);
			if (got) {
				memcpy(buf, resp.data.data(), got);
				_pos += got;
			}
			return got;
		}
	};

	class FishPlusFileWriter : public IFileWriter
	{
		std::shared_ptr<FishPlus::Session> _sess;
		std::string _path;
		unsigned long long _pos;
		bool _encoded;
		size_t _chunk;
		std::vector<unsigned char> _pending;

		void Flush(size_t keep_below)
		{
			size_t ofs = 0;
			while (_pending.size() - ofs >= keep_below && ofs < _pending.size()) {
				const size_t piece = std::min(_chunk, _pending.size() - ofs);
				auto resp = _sess->ExecPayload("write", {_path},
					{ToDec(_pos), ToDec((unsigned long long)piece), _encoded ? "b64" : "raw"},
					_pending.data() + ofs, piece, _encoded);
				if (!resp.ok) {
					// A refusal the helper saw coming is answered only after
					// the payload has been read into /dev/null, and says so
					// with a "D" line. Without that line an unknown number of
					// bytes is still on the wire and the next request would be
					// parsed out of the middle of a file.
					if (!resp.HasLine("D")) {
						_sess->MarkBroken();
					}
					FishPlus::Session::ThrowIfFailed(resp, "FISH+ write error", _path);
				}
				_pos += piece;
				ofs += piece;
			}
			_pending.erase(_pending.begin(), _pending.begin() + ofs);
		}

	public:
		FishPlusFileWriter(std::shared_ptr<FishPlus::Session> sess, const std::string &path,
				unsigned long long resume_pos, bool encoded, size_t chunk)
			: _sess(sess), _path(path), _pos(resume_pos), _encoded(encoded), _chunk(chunk)
		{
		}

		virtual void Write(const void *buf, size_t len)
		{
			if (!len) {
				return;
			}
			const unsigned char *p = (const unsigned char *)buf;
			_pending.insert(_pending.end(), p, p + len);
			// Whole chunks go out now, the tail waits for more or for the close.
			Flush(_chunk);
		}

		virtual void WriteComplete()
		{
			Flush(1);
		}
	};

	class FishPlusDirectoryEnumer : public IDirectoryEnumer
	{
		std::vector<FishPlus::Entry> _entries;
		size_t _at{0};

	public:
		FishPlusDirectoryEnumer(std::shared_ptr<FishPlus::Session> sess, const std::string &path)
		{
			auto resp = sess->ExecPath("enum", path);
			FishPlus::Session::ThrowIfFailed(resp, "FISH+ enum error", path);
			const std::string mode = FishPlus::ParseListing(resp.lines, _entries);
			if (mode.empty()) {
				throw ProtocolError("FISH+: unparsable directory listing", path.c_str());
			}
		}

		virtual bool Enum(std::string &name, std::string &owner, std::string &group, FileInformation &file_info)
		{
			for (;;) {
				if (_at >= _entries.size()) {
					name.clear();
					owner.clear();
					group.clear();
					return false;
				}
				const FishPlus::Entry &e = _entries[_at++];
				if (!FILENAME_ENUMERABLE(e.name.c_str())) {
					continue;
				}
				name = e.name;
				// The helper reports ids numerically on purpose: a user name
				// with a space in it cannot be told from the column after it.
				owner = (e.uid >= 0) ? ToDec(e.uid) : std::string();
				group = (e.gid >= 0) ? ToDec(e.gid) : std::string();
				file_info.mode = e.mode;
				file_info.size = e.size;
				file_info.access_time = e.atime;
				file_info.modification_time = e.mtime;
				file_info.status_change_time = e.ctime;
				return true;
			}
		}
	};
}

////////////////////////////////////////////////////////////////////////////

ProtocolFISHPLUS::ProtocolFISHPLUS(const std::string &host, unsigned int port,
	const std::string &username, const std::string &password,
	const std::string &options, int fd_ipc_recv)
	:
	_fd_ipc_recv(fd_ipc_recv),
	_protocol_options(options),
	_host(host),
	_port(port),
	_username(username),
	_password(password)
{
	if (_username.empty()) {
		const char *user = getenv("USER");
		_username = (user && *user) ? user : "root";
	}
	Initialize();
}

ProtocolFISHPLUS::~ProtocolFISHPLUS()
{
	// The session goes first: when our side of the stream is gone the helper's
	// read hits EOF and the remote shell leaves on its own, so there is no
	// farewell command that a hung remote could stall on.
	_sess.reset();
	_way.reset();
}

void ProtocolFISHPLUS::SubstituteCreds(std::string &str)
{
	Substitute(str, "$HOST", _host);
	Substitute(str, "$PORT", ToDec(_port));
	Substitute(str, "$USER", _username);
	Substitute(str, "$PASSWORD", _password);
}

void ProtocolFISHPLUS::OpenWay()
{
	_way.reset();
	WayToShellConfig cfg(FISHPLUS_WAYS_INI, _way_name);
	if (!cfg.command.empty()) {
		SubstituteCreds(cfg.command);
	}
	if (!cfg.serial.empty()) {
		SubstituteCreds(cfg.serial);
	}
	_way = std::make_shared<WayToShell>(_fd_ipc_recv, cfg, _protocol_options);
	fprintf(stderr, "[FISH+] WAY OPENED\n");
}

void ProtocolFISHPLUS::PerformLogin()
{
	// Identical in shape to the SHELL protocol's login: ways.ini describes what
	// the transport prints and what to answer it with, so a password prompt, a
	// host key question and a sudo challenge are configuration rather than code.
	KeyFileReadSection rules(FISHPLUS_WAYS_INI, _way_name + "/LOGIN");
	if (rules.empty()) {
		fprintf(stderr, "[FISH+] LOGIN NOT REQUIRED\n");
		return;
	}

	std::vector<const char *> replies;
	for (const auto &it : rules) {
		if (it.first.empty()) { // special case - initial string
			std::string line = it.second;
			SubstituteCreds(line);
			_way->Send(line);
		} else {
			replies.emplace_back(it.first.c_str());
		}
	}
	if (replies.empty()) {
		fprintf(stderr, "[FISH+] NON-INTERACTIVE LOGIN DONE\n");
		return;
	}
	auto wr = _way->WaitReply(replies);
	for (;;) {
		std::string reply = rules.GetString(replies[wr.index]);
		if (reply == "^") {
			break;
		}
		if (reply == "^STDERR" || reply == "^STDOUT" || reply == "^STD") {
			std::string s;
			if (reply == "^STDERR" || reply == "^STD") {
				AppendTrimmedLines(s, wr.stderr_lines);
			}
			if (reply == "^STDOUT" || reply == "^STD") {
				AppendTrimmedLines(s, wr.stdout_lines);
			}
			throw ProtocolError(s);
		}
		if (reply == "^AUTH") {
			throw ProtocolAuthFailedError();
		}

		SubstituteCreds(reply);
		wr = _way->SendAndWaitReply(reply, replies);
	}
	fprintf(stderr, "[FISH+] INTERACTIVE LOGIN DONE\n");
}

void ProtocolFISHPLUS::Initialize()
{
	_way_name = _protocol_options.GetString("Way");
	if (_way_name.empty()) { // defaults to the very first root section
		WaysToShell ways(FISHPLUS_WAYS_INI);
		if (!ways.empty()) {
			_way_name = ways.front();
		}
		if (_way_name.empty()) {
			throw ProtocolError("Way not specified");
		}
	}
	fprintf(stderr, "[FISH+] INITIALIZE: '%s'\n", _way_name.c_str());

	OpenWay();
	PerformLogin();

	_sess = std::make_shared<FishPlus::Session>(_way);
	// Every way in ways.ini reaches the remote shell through a pseudo terminal,
	// which is why the helper is told to expect one; it tames the line
	// discipline with POSIX stty and reports "tty" when it managed to.
	_sess->Handshake(FISHPLUS_HELPER, true);

	// The directory an interactive login would have landed in.
	auto resp = _sess->Exec("pwd");
	if (resp.ok && !resp.lines.empty()) {
		_home = resp.lines.front();
		StrTrim(_home, " \t\r\n");
	}
	if (_home.empty()) {
		_home = "/";
	}
	fprintf(stderr, "[FISH+] READY, home '%s'\n", _home.c_str());
}

void ProtocolFISHPLUS::KeepAlive(const std::string &path_to_check)
{
	auto resp = _sess->Exec("noop");
	if (!resp.ok) {
		throw ProtocolError("FISH+ keepalive failed", resp.msg.c_str());
	}
}

////////////////////////////////////////////////////////////////////////////

void ProtocolFISHPLUS::QueryInformation(FileInformation &file_info, const std::string &path, bool follow_symlink)
{
	auto resp = _sess->ExecPath(follow_symlink ? "info" : "linfo", path);
	FishPlus::Session::ThrowIfFailed(resp, "FISH+ stat error", path);

	FishPlus::Entry e;
	if (!FishPlus::ParseSingle(resp.lines, e)) {
		throw ProtocolError("FISH+: unparsable file information", path.c_str());
	}
	file_info.mode = e.mode;
	file_info.size = e.size;
	file_info.access_time = e.atime;
	file_info.modification_time = e.mtime;
	file_info.status_change_time = e.ctime;
}

mode_t ProtocolFISHPLUS::GetMode(const std::string &path, bool follow_symlink)
{
	FileInformation fi{};
	QueryInformation(fi, path, follow_symlink);
	return fi.mode;
}

unsigned long long ProtocolFISHPLUS::GetSize(const std::string &path, bool follow_symlink)
{
	FileInformation fi{};
	QueryInformation(fi, path, follow_symlink);
	return fi.size;
}

void ProtocolFISHPLUS::GetInformation(FileInformation &file_info, const std::string &path, bool follow_symlink)
{
	QueryInformation(file_info, path, follow_symlink);
}

void ProtocolFISHPLUS::FileDelete(const std::string &path)
{
	auto resp = _sess->ExecPath("rm", path);
	FishPlus::Session::ThrowIfFailed(resp, "FISH+ delete error", path);
}

void ProtocolFISHPLUS::DirectoryDelete(const std::string &path)
{
	// rmdir rather than rmtree: NetRocks enumerates and removes the contents
	// itself so that it can show progress and let the user abort. The helper's
	// one-round-trip recursive delete waits for a place in IProtocol to live
	// in; see INTEGRATION.md.
	auto resp = _sess->ExecPath("rmdir", path);
	FishPlus::Session::ThrowIfFailed(resp, "FISH+ rmdir error", path);
}

void ProtocolFISHPLUS::DirectoryCreate(const std::string &path, mode_t mode)
{
	auto resp = _sess->ExecPath("mkdir", path);
	FishPlus::Session::ThrowIfFailed(resp, "FISH+ mkdir error", path);
	if (mode != 0 && mode != (mode_t)-1) {
		try {
			SetMode(path, mode);
		} catch (std::exception &) {
			// A host that will not let us set the mode of a directory we just
			// created is not a reason to fail the whole operation.
		}
	}
}

void ProtocolFISHPLUS::Rename(const std::string &path_old, const std::string &path_new)
{
	auto resp = _sess->ExecPaths("mv", {path_old, path_new});
	FishPlus::Session::ThrowIfFailed(resp, "FISH+ rename error", path_old);
}

void ProtocolFISHPLUS::FileCopy(const std::string &path_src, const std::string &path_dst)
{
	// cp -R -f on the far side: recursive, so a whole tree costs one round
	// trip and the data never touches the network.
	auto resp = _sess->ExecPaths("cp", {path_src, path_dst});
	FishPlus::Session::ThrowIfFailed(resp, "FISH+ copy error", path_src);
}

void ProtocolFISHPLUS::SetTimes(const std::string &path, const timespec &access_time, const timespec &modification_time)
{
	// utime <mtime> <atime>, either of them '-' for "leave it alone". The epoch
	// is converted to the host's own local time on the far side, because BSD
	// and macOS touch want -t YYYYMMDDhhmm.SS in a zone we cannot compute here.
	auto resp = _sess->ExecPath("utime", path,
		{ToDec((long long)modification_time.tv_sec), ToDec((long long)access_time.tv_sec)});
	FishPlus::Session::ThrowIfFailed(resp, "FISH+ set times error", path);
}

void ProtocolFISHPLUS::SetMode(const std::string &path, mode_t mode)
{
	char octal[32];
	snprintf(octal, sizeof(octal), "%o", (unsigned)(mode & 07777));
	auto resp = _sess->ExecPath("chmod", path, {octal});
	FishPlus::Session::ThrowIfFailed(resp, "FISH+ chmod error", path);
}

void ProtocolFISHPLUS::SymlinkCreate(const std::string &link_path, const std::string &link_target)
{
	// A host whose helper predates the command, or which has no ln, says so by
	// leaving it out of the banner. Refusing here is better than sending a
	// request that would come back as "unknown command".
	if (!_sess->Feats().Has("ln")) {
		throw ProtocolUnsupportedError("FISH+: the remote host cannot create symlinks");
	}
	// The link comes first and is the only one guarded on the far side: the
	// target is a string to store, not a path on that host, so a relative or
	// not-yet-existing target is ordinary rather than suspect.
	auto resp = _sess->ExecPaths("mklink", {link_path, link_target});
	FishPlus::Session::ThrowIfFailed(resp, "FISH+ symlink error", link_path);
}

void ProtocolFISHPLUS::SymlinkQuery(const std::string &link_path, std::string &link_target)
{
	auto resp = _sess->ExecPath("rdlink", link_path);
	FishPlus::Session::ThrowIfFailed(resp, "FISH+ readlink error", link_path);
	if (resp.lines.empty()) {
		throw ProtocolError("FISH+: empty symlink target", link_path.c_str());
	}
	link_target = resp.lines.front();
}

std::string ProtocolFISHPLUS::RealPath(const std::string &path)
{
	return RealPathFromHome(_home, path);
}

std::shared_ptr<IDirectoryEnumer> ProtocolFISHPLUS::DirectoryEnum(const std::string &path)
{
	return std::make_shared<FishPlusDirectoryEnumer>(_sess, path);
}

std::shared_ptr<IFileReader> ProtocolFISHPLUS::FileGet(const std::string &path, unsigned long long resume_pos)
{
	if (_sess->Feats().ReadMode().empty()) {
		throw ProtocolUnsupportedError("FISH+: the remote host cannot read files");
	}
	return std::make_shared<FishPlusFileReader>(_sess, path, resume_pos);
}

std::shared_ptr<IFileWriter> ProtocolFISHPLUS::FilePut(const std::string &path, mode_t mode,
	unsigned long long size_hint, unsigned long long resume_pos)
{
	std::string write_mode = _sess->Feats().WriteMode();
	if (write_mode.empty()) {
		// Sending a payload the remote side cannot take off the wire would
		// leave it in the stream to be read as the next request.
		throw ProtocolUnsupportedError("FISH+: the remote host cannot write files");
	}

	bool encoded = (write_mode == "b64");
	if (!encoded && !_sess->RawPayloadSafe()) {
		// A terminal that could not be tamed mangles raw bytes, so the payload
		// has to travel as base64 even though the host would prefer dd.
		auto resp = _sess->Exec("wmode", {"b64"});
		FishPlus::Session::ThrowIfFailed(resp, "FISH+ cannot select a safe write mode", path);
		encoded = true;
	}

	if (resume_pos == 0) {
		// Create truncates to zero and then writes forward, which is the one
		// path that works everywhere: zero is a shell redirection, while any
		// other size needs the truncate utility.
		auto resp = _sess->ExecPath("trunc", path, {"0"});
		FishPlus::Session::ThrowIfFailed(resp, "FISH+ create error", path);
		if (mode != 0 && mode != (mode_t)-1) {
			try {
				SetMode(path, mode);
			} catch (std::exception &) {
				// Best effort: a host that refuses chmod must not turn a
				// successful transfer into a failed one.
			}
		}
	}

	return std::make_shared<FishPlusFileWriter>(_sess, path, resume_pos, encoded,
		encoded ? FishPlus::WRITE_CHUNK_B64 : FishPlus::WRITE_CHUNK);
}
