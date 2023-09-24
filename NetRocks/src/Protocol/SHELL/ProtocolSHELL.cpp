#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>
#include <vector>
#include <map>
#include <string>
#include <fstream>
#include "ProtocolSHELL.h"
#include "Parse.h"
#include <StringConfig.h>
#include <base64.h>
#include <utils.h>

#include <fstream>
#include <cstdlib> // для getenv

#define GREETING_MSG "far2l is ready for fishing"

static std::vector<const char *> s_prompt{">(((^>\n"};
static std::vector<const char *> s_prompt_or_error{">(((^>\n", "+ERROR:*\n"};
static std::vector<const char *> s_ok_or_error{"+OK:*\n", "+ERROR:*\n"};
static std::vector<const char *> s_read_replies{"+NEXT:*\n", "+DONE\n", "+FAIL\n", "+ABORTED\n", "+ERROR:*\n"};
static std::vector<const char *> s_write_replies{"+OK\n", "+ERROR:*\n"};

std::shared_ptr<IProtocol> CreateProtocol(const std::string &protocol, const std::string &host, unsigned int port,
	const std::string &username, const std::string &password, const std::string &options)
{
	fprintf(stderr, "*1\n");
	return std::make_shared<ProtocolSHELL>(host, port, username, password, options);
}

static uint64_t GetIntegerBeforeStatusReplyLine(const std::string &data, size_t pos)
{
	size_t prev_line_start = pos;
	if (prev_line_start) do {
		--prev_line_start;
	} while (prev_line_start && data[prev_line_start] != '\n');
	if (data[prev_line_start] == '\n') {
		++prev_line_start;
	}
	return (uint64_t)strtoull(data.c_str() + prev_line_start, nullptr, 10);
}

////////////////////////////

static std::string LoadHelper(const char *helper)
{
	std::ifstream helper_ifs;
	helper_ifs.open(helper);
	std::string out, tmp_str, varname;
	if (!helper_ifs.is_open() ) {
		throw ProtocolError("can't open helper", helper, errno);
	}
	std::map<std::string, unsigned int> renamed_vars;
	size_t orig_len = 0;
	while (std::getline(helper_ifs, tmp_str)) {
		orig_len+= tmp_str.size() + 1;
		// do some compactization
		StrTrim(tmp_str);
		// skip no-code lines unless enabled logging (to keep line numbers)
		if (tmp_str.empty() || tmp_str.front() == '#') {
			if (g_netrocks_verbosity <= 0) {
				continue;
			}
			tmp_str.clear();
		}
		// rename variables
		for (;;) {
			size_t p = tmp_str.find("SHELLVAR_");
			if (p == std::string::npos) {
				p = tmp_str.find("SHELLFCN_");
				if (p == std::string::npos) {
					break;
				}
			}
			size_t e = p + 8;
			while (e < tmp_str.size() && (isalnum(tmp_str[e]) || tmp_str[e] == '_')) {
				++e;
			}
			varname = tmp_str.substr(p, e - p);
			auto ir = renamed_vars.emplace(varname, renamed_vars.size());
			tmp_str.replace(p, e - p, StrPrintf("F%x", ir.first->second));
		}
		out+= ' '; // prepend each line with space to avoid history pollution (as HISTCONTROL=ignorespace)
		out+= tmp_str;
		out+= '\n';
	}
	fprintf(stderr, "LoadHelper('%s'): %lu -> %lu bytes, %lu tokens renamed\n",
		helper, (unsigned long)orig_len, (unsigned long)out.size(), (unsigned long)renamed_vars.size());
	if (g_netrocks_verbosity > 2) {
		fprintf(stderr, "---\n");
		fprintf(stderr, "%s\n", out.c_str());
		fprintf(stderr, "---\n");
	}
	return out;
}

ProtocolSHELL::ProtocolSHELL(const std::string &host, unsigned int port,
	const std::string &username, const std::string &password, const std::string &options)
	:
	_app(std::make_shared<ClientApp>())
{
	StringConfig protocol_options(options);
	// ---------------------------------------------

	if (g_netrocks_verbosity > 0) {
		fprintf(stderr, "*** host from config: %s\n", host.c_str());
		fprintf(stderr, "*** port from config: %i\n", port);
		fprintf(stderr, "*** username from config: %s\n", username.c_str());
		if (g_netrocks_verbosity > 1) {
			fprintf(stderr, "*** password from config: {%lu}\n", password.size());
		}
	}

	std::string ssh_arg = host;
	if (!username.empty()) {
		ssh_arg.insert(0, 1, '@');
		ssh_arg.insert(0, username);
	}

	fprintf(stderr, "*** SHELL CONNECT: %s\n", ssh_arg.c_str());

	char ssh_prog[] = "ssh";
	char ssh_cmd[] = "echo '" GREETING_MSG "';HISTCONTROL=ignorespace sh";
	char *argv[] = {ssh_prog, (char *)ssh_arg.c_str(), ssh_cmd, NULL};
	if (!_app->Open(ssh_prog, argv)) {
		throw ProtocolError("ssh exec failed");
	}

	std::string helper = LoadHelper("SHELL/remote.sh");

	// Мы таки залогинены, спрашивают пароль, ключ незнакомый?
	std::vector<const char *> login_replies {
		GREETING_MSG "\n",
		"Permission denied*\n",
		"*password: ",
		"*Enter passphrase for key*",
		"Are you sure*",
		"Host key verification failed"
	};

	auto wr = _app->WaitReply(login_replies);
	while (wr.index > 0) {
		if (wr.index == 1) {
			throw ProtocolAuthFailedError();
		}
		if (wr.index == 2 || wr.index == 3) {
			wr = _app->SendAndWaitReply(password + "\n", login_replies, true);
		}
		if (wr.index == 4) {
			wr = _app->SendAndWaitReply("yes\n", login_replies);
		}
		if (wr.index == 5) {
			std::string s;
			for (const auto &line : wr.stderr_lines) {
				s+= line;
			}
			if (s.back() == '\n') {
				s.pop_back();
			}
			throw ProtocolError(s);
		}
	}

	wr = _app->SendAndWaitReply(helper, s_prompt);
	// если мы сюда добрались, значит, уже залогинены
	// fixme: таймайт? ***
	fprintf(stderr, "*** SHELL CONNECTED\n");

	if (wr.stdout_lines.size() > 1) {
		auto &info_line = wr.stdout_lines[wr.stdout_lines.size() - 2];
		while (!info_line.empty() && (info_line.back() == '\r' || info_line.back() == '\n') ) {
			info_line.pop_back();
		}
		_feats.using_stat = (info_line.find(" STAT ") != std::string::npos);
		_feats.using_find = (info_line.find(" FIND ") != std::string::npos);
		_feats.using_ls = (info_line.find(" LS ") != std::string::npos);
		if (info_line.find(" READ_RESUME ") != std::string::npos) {
			_feats.support_read = _feats.support_read_resume = true;
		}
		if (!_feats.support_read && info_line.find(" READ ") != std::string::npos) {
			_feats.support_read = true;
		}
		if (info_line.find(" WRITE_RESUME ") != std::string::npos) {
			_feats.support_write = _feats.support_write_resume = true;
		}
		if (info_line.find(" WRITE_BASE64 ") != std::string::npos) {
			 _feats.support_write = _feats.require_write_base64 = true;
		}
		if (!_feats.support_write && info_line.find(" WRITE ") != std::string::npos) {
			_feats.support_write = true;
		}
		size_t p = info_line.find(" WRITE_BLOCK=");
		if (p != std::string::npos) {
			size_t require_write_block = atoi(info_line.c_str() + p + 13);
			_feats.require_write_block = require_write_block;
			const size_t px = info_line.find('*', p + 13);
			if (px != std::string::npos && px < info_line.find(' ', p + 13)) {
				_feats.limit_max_blocks = atoi(info_line.c_str() + px + 1);
			}
		}
	}
	fprintf(stderr, "*** stat=%u find=%u ls=%u read=%u r/resume:%u write:%u w/resume:%u w/base64:%u w/block:%u*%u\n",
		_feats.using_stat, _feats.using_find, _feats.using_ls, _feats.support_read, _feats.support_read_resume,
		_feats.support_write, _feats.support_write_resume, _feats.require_write_base64,
		_feats.require_write_block, _feats.limit_max_blocks);
}

ProtocolSHELL::~ProtocolSHELL()
{
	fprintf(stderr, "*** ProtocolSHELL::%s\n", __FUNCTION__);
	_exec_cmd.reset();
}

void ProtocolSHELL::KeepAlive(const std::string &path_to_check)
{
	if (_exec_cmd) {
		if (!_exec_cmd->KeepAlive()) {
			_exec_cmd.reset();
		}
	}
	if (!_exec_cmd) {
		_app->SendAndWaitReply("noop\n", s_prompt);
	}
}

void ProtocolSHELL::GetModes(bool follow_symlink, size_t count, const std::string *pathes, mode_t *modes) noexcept
{
	_exec_cmd.reset();
	fprintf(stderr, "*** ProtocolSHELL::GetModes follow_symlink=%d count=%lu\n", follow_symlink, (unsigned long)count);
	try {
		std::string request = follow_symlink ? "mode\n" : "lmode\n";
		for (size_t i = 0; i != count; ++i) {
			request+= pathes[i];
			request+= '\n';
		}
		request+= '\n';
		auto wr = _app->SendAndWaitReply(request, s_prompt);
		for (size_t i = 0; i < count; ++i) {
			if (wr.stdout_lines.size() - 1 >= count - i) {
				auto &line = wr.stdout_lines[wr.stdout_lines.size() - 1 - (count - i)];
				if (StrStartsFrom(line, "+ERROR:")) {
					modes[i] = ~(mode_t)0;
				} else if (_feats.using_stat || _feats.using_find) {
					modes[i] = SHELLParseModeByStatOrFind(line);
				} else {
					modes[i] = SHELLParseModeByLS(line);
				}
			}
		}
	} catch (...) {
		fprintf(stderr, "*** ProtocolSHELL::GetModes excpt\n");
		for (size_t i = 0; i != count; ++i) {
			modes[i] = ~(mode_t)0;
		}
	}
}

mode_t ProtocolSHELL::GetMode(const std::string &path, bool follow_symlink)
{
	fprintf(stderr, "*** ProtocolSHELL::%s\n", __FUNCTION__);
	_exec_cmd.reset();
	auto wr = _app->SendAndWaitReply(
		MultiLineRequest(follow_symlink ? "mode" : "lmode", path, std::string()),
		s_prompt_or_error);
	if (wr.index != 0 || wr.stdout_lines.size() < 2) {
		if (wr.index != 0) {
			_app->WaitReply(s_prompt);
		}
		throw ProtocolError("mode query error");
	}

	if (_feats.using_stat || _feats.using_find) {
		return SHELLParseModeByStatOrFind(wr.stdout_lines[wr.stdout_lines.size() - 2]);
	}

	return SHELLParseModeByLS(wr.stdout_lines[wr.stdout_lines.size() - 2]);
}

unsigned long long ProtocolSHELL::GetSize(const std::string &path, bool follow_symlink)
{
	fprintf(stderr, "*** ProtocolSHELL::%s\n", __FUNCTION__);
	_exec_cmd.reset();
	auto wr = _app->SendAndWaitReply(
		MultiLineRequest(follow_symlink ? "size" : "lsize", path, std::string()),
		s_prompt_or_error);

	if (wr.index != 0 || wr.stdout_lines.size() < 2) {
		if (wr.index != 0) {
			_app->WaitReply(s_prompt);
		}
		throw ProtocolError("mode query error");
	}

	if (_feats.using_stat || _feats.using_find) {
		return SHELLParseSizeByStatOrFind(wr.stdout_lines[wr.stdout_lines.size() - 2]);
	}

	return SHELLParseSizeByLS(wr.stdout_lines[wr.stdout_lines.size() - 2]);
}

void ProtocolSHELL::GetInformation(FileInformation &file_info, const std::string &path, bool follow_symlink)
{
	fprintf(stderr, "*** ProtocolSHELL::%s\n", __FUNCTION__);
	_exec_cmd.reset();
	auto wr = _app->SendAndWaitReply(
		MultiLineRequest(follow_symlink ? "info" : "linfo", path, std::string()),
		s_prompt_or_error);

	if (wr.index != 0 || wr.stdout_lines.size() < 2) {
		if (wr.index != 0) {
			_app->WaitReply(s_prompt);
		}
		throw ProtocolError("mode query error");
	}

	if (_feats.using_stat || _feats.using_find) {
		SHELLParseInfoByStatOrFind(file_info, wr.stdout_lines[wr.stdout_lines.size() - 2]);

	} else {
		SHELLParseInfoByLS(file_info, wr.stdout_lines[wr.stdout_lines.size() - 2]);
	}
}

void ProtocolSHELL::FileDelete(const std::string &path)
{
	fprintf(stderr, "*** ProtocolSHELL::%s\n", __FUNCTION__);
	_exec_cmd.reset();
	auto wr = _app->SendAndWaitReply(
		MultiLineRequest("rmfile", path),
		s_prompt_or_error);
	if (wr.index != 0) {
		_app->WaitReply(s_prompt);
		throw ProtocolError("rmfile error");
	}
}

void ProtocolSHELL::DirectoryDelete(const std::string &path)
{
	fprintf(stderr, "*** ProtocolSHELL::%s\n", __FUNCTION__);
	_exec_cmd.reset();
	auto wr = _app->SendAndWaitReply(
		MultiLineRequest("rmdir", path),
		s_prompt_or_error);
	if (wr.index != 0) {
		_app->WaitReply(s_prompt);
		throw ProtocolError("rmdir error");
	}
}

void ProtocolSHELL::DirectoryCreate(const std::string &path, mode_t mode)
{
	fprintf(stderr, "*** ProtocolSHELL::%s\n", __FUNCTION__);
	_exec_cmd.reset();
	auto wr = _app->SendAndWaitReply(
		MultiLineRequest("mkdir", path, StrPrintf("0%o", mode & 0777)),
		s_prompt_or_error);
	if (wr.index != 0) {
		_app->WaitReply(s_prompt);
		throw ProtocolError("mkdir error");
	}
}

void ProtocolSHELL::Rename(const std::string &path_old, const std::string &path_new)
{
	fprintf(stderr, "*** ProtocolSHELL::%s\n", __FUNCTION__);
	_exec_cmd.reset();
	auto wr = _app->SendAndWaitReply(
		MultiLineRequest("rename", path_old, path_new),
		s_prompt_or_error);
	if (wr.index != 0) {
		_app->WaitReply(s_prompt);
		throw ProtocolError("rename error");
	}
}


void ProtocolSHELL::SetTimes(const std::string &path, const timespec &access_time, const timespec &modification_time)
{
	fprintf(stderr, "*** ProtocolSHELL::%s\n", __FUNCTION__);
	_exec_cmd.reset();
}

void ProtocolSHELL::SetMode(const std::string &path, mode_t mode)
{
	fprintf(stderr, "*** ProtocolSHELL::%s\n", __FUNCTION__);
	_exec_cmd.reset();
	auto wr = _app->SendAndWaitReply(
		MultiLineRequest("chmod", path, StrPrintf("0%o", mode & 0777)),
		s_prompt_or_error);
	if (wr.index != 0) {
		_app->WaitReply(s_prompt);
		throw ProtocolError("chmod error");
	}
}

void ProtocolSHELL::SymlinkCreate(const std::string &link_path, const std::string &link_target)
{
	fprintf(stderr, "*** ProtocolSHELL::%s\n", __FUNCTION__);
	_exec_cmd.reset();
	auto wr = _app->SendAndWaitReply(
		MultiLineRequest("mksym", link_path, link_target),
		s_prompt_or_error);
	if (wr.index != 0) {
		_app->WaitReply(s_prompt);
		throw ProtocolError("mksym error");
	}
}

void ProtocolSHELL::SymlinkQuery(const std::string &link_path, std::string &link_target)
{
	fprintf(stderr, "*** ProtocolSHELL::%s\n", __FUNCTION__);
	auto wr = _app->SendAndWaitReply(
		MultiLineRequest("rdsym", link_path),
		s_ok_or_error);
	_app->WaitReply(s_prompt);
	if (wr.index != 0) {
		throw ProtocolError("mksym error");
	}
	link_target = wr.stdout_lines.back();
	size_t p = link_target.find(':');
	if (p != std::string::npos) {
		link_target.erase(0, p + 1);
	}
	p = link_target.find('\n');
	if (p != std::string::npos) {
		link_target.resize(p);
	}
}

class SHELLDirectoryEnumer : public IDirectoryEnumer {
private:
	std::shared_ptr<ClientApp> _app;

	std::vector<FileInfo> _files;
	size_t _index = 0;

public:
	SHELLDirectoryEnumer(std::shared_ptr<ClientApp> &app, const std::string &path, const RemoteFeats &feats)
		: _app(app)
	{
		auto wr = _app->SendAndWaitReply( MultiLineRequest("enum", path), s_prompt_or_error);
		if (wr.index != 0) {
			_app->WaitReply(s_prompt);
			throw ProtocolError("dir query error");
		}
		if (feats.using_stat || feats.using_find) {
			wr.stdout_lines.pop_back(); // get rid of reply
			SHELLParseEnumByStatOrFind(_files, wr.stdout_lines);

		} else {
			wr.stdout_lines.pop_back(); // get rid of reply
			SHELLParseEnumByLS(_files, wr.stdout_lines);
		}

		fprintf(stderr, "*** LIST READ\n");
    }

	virtual bool Enum(std::string &name, std::string &owner, std::string &group, FileInformation &file_info) override
	{
		const FileInfo *file;
		do {
			if (_index >= _files.size()) {
				return false;
			}
			file = &_files[_index++];
			name = file->path;
			if (S_ISLNK(file->mode)) {
				size_t p = name.rfind(" -> ");
				if (p != std::string::npos) {
					name.resize(p);
				}
			}
//			if (_unquote && name.size() > 2 && name.front() == '\"' && name.back() == '\"') {
//				name.pop_back();
//				name.erase(0, 1);
//			}

		} while (!FILENAME_ENUMERABLE(name));

		owner = file->owner;
		group = file->group;
		file_info.access_time = file->access_time;
		file_info.modification_time = file->modification_time;
		file_info.status_change_time = file->status_change_time;
		file_info.size = file->size;
		file_info.mode = file->mode;
		return true;
	}
};

std::shared_ptr<IDirectoryEnumer> ProtocolSHELL::DirectoryEnum(const std::string &path)
{
	fprintf(stderr, "Enum '%s'\n", path.c_str());
	_exec_cmd.reset();
	return std::shared_ptr<IDirectoryEnumer>(new SHELLDirectoryEnumer(_app, path, _feats));
}

class SHELLFileReader : public IFileReader
{
	std::shared_ptr<ClientApp> _app;

	unsigned long _chunk {0};
	bool _finished{false};
	bool _aborting{false};

	void RecvChunkHeader()
	{
		WaitResult wr = _app->SendAndWaitReply(_aborting ? "abort\n" : "cont\n", s_read_replies);
		//{"+NEXT:*\n", "+DONE\n", "+FAIL\n", "+ABORTED\n", "+ERROR:*\n"};
		if (wr.index == 4) {
			std::string resync = wr.stdout_lines.back();
			resync.replace(0, 1, "SHELL_RESYNCHRONIZATION_");
			_app->Send(resync);
			_finished = true;
			throw ProtocolError("read error");
		}
		if (wr.index == 0) {
			_chunk = strtoul(wr.stdout_lines.back().c_str() + 6, nullptr, 10);
		} else {
			_chunk = 0;
		}
		if (!_chunk) {
			_finished = true;
		}
	}


public:
	SHELLFileReader(std::shared_ptr<ClientApp> &app, const std::string &path, unsigned long long resume_pos)
		: _app(app)
	{
		_app->Send( MultiLineRequest("read", StrPrintf("%llu %s", resume_pos, path.c_str()), "cont"));
	}

	virtual ~SHELLFileReader()
	{
		_aborting = true;
		try {
			while (!_finished) {
				char tmp[0x1000];
				Read(tmp, sizeof(tmp));
			}
		} catch (std::exception &e) {
			fprintf(stderr, "~SHELLFileReader: read %s\n", e.what());
		} catch (...) {
			fprintf(stderr, "~SHELLFileReader: read ...\n");
		}
		try {
			_app->WaitReply(s_prompt);
		} catch (std::exception &e) {
			fprintf(stderr, "~SHELLFileReader: prompt %s\n", e.what());
		} catch (...) {
			fprintf(stderr, "~SHELLFileReader: prompt ...\n");
		}
	}

	virtual size_t Read(void *buf, size_t len)
	{
		size_t rv = 0;
		while (rv < len && !_finished) {
			if (!_chunk) {
				RecvChunkHeader();
				if (_finished) {
					break;
				}
			}
			const size_t piece = std::min(len - rv, _chunk);
			_app->ReadStdout((char *)buf + rv, piece);
			rv+= piece;
			_chunk-= piece;
		}
		return rv;
	}
};

class SHELLFileWriter : public IFileWriter
{
	std::shared_ptr<ClientApp> _app;
	RemoteFeats _feats;
	unsigned int _pending_replies{0};
	bool _write_completed{false};
	std::vector<char> _padded_data;
	size_t _seq{0};

	void WaitReply()
	{
		--_pending_replies;
		const auto &wr = _app->WaitReply(s_write_replies); // {"+OK\n", "+ERROR:*\n"};
		if (wr.index == 1) {
			std::string resync = wr.stdout_lines.back();
			resync.replace(0, 1, "SHELL_RESYNCHRONIZATION_");
			_app->Send(resync);
			throw ProtocolError("write error");
		}
	}

public:
	SHELLFileWriter(std::shared_ptr<ClientApp> &app, const std::string &path, mode_t mode, unsigned long long size_hint, unsigned long long resume_pos, const RemoteFeats &feats)
		: _app(app), _feats(feats)
	{
		_app->Send( MultiLineRequest("write", StrPrintf("%llu %s", resume_pos, path.c_str())));
		++_pending_replies;
	}

	virtual ~SHELLFileWriter()
	{
		if (!_write_completed) try {
			_app->Send(". .\n");
		} catch (std::exception &e) {
			fprintf(stderr, "~SHELLFileWriter: dot %s\n", e.what());
		} catch (...) {
			fprintf(stderr, "~SHELLFileWriter: dot ...\n");
		}
		while (_pending_replies) try {
			WaitReply();
		} catch (std::exception &e) {
			fprintf(stderr, "~SHELLFileWriter: reply %s\n", e.what());
		} catch (...) {
			fprintf(stderr, "~SHELLFileWriter: reply ...\n");
		}
		try {
			_app->WaitReply(s_prompt);
		} catch (std::exception &e) {
			fprintf(stderr, "~SHELLFileReader: prompt %s\n", e.what());
		} catch (...) {
			fprintf(stderr, "~SHELLFileReader: prompt ...\n");
		}
	}

	virtual void WriteComplete()
	{
		if (!_write_completed) {
			_write_completed = true;
			_app->Send(". .\n");
		}
		while (_pending_replies) {
			WaitReply();
		}
	}

	virtual void Write(const void *buf, size_t len)
	{
		if (_feats.require_write_base64) {
			std::string line;
			for (size_t ofs = 0; ofs < len; ) {
				// avoid too long lines cuz receiver should fit it into env variable
				const size_t piece = std::min(size_t(0x1000), len - ofs);
				line.clear();
				base64_encode(line, (const unsigned char *)buf + ofs, piece);
				line+= '\n';
				WritePiece(piece, line.data(), line.size());
				ofs+= piece;
			}
		} else if (_feats.require_write_block > 1) {
			size_t ofs = 0;
			for (;;) {
				size_t piece = ((len - ofs) / _feats.require_write_block) * _feats.require_write_block;
				if (piece == 0) {
					break;
				}
				if (_feats.limit_max_blocks && piece > _feats.require_write_block * _feats.limit_max_blocks) {
					piece = _feats.require_write_block * _feats.limit_max_blocks;
				}
				WritePiece(piece, (const char *)buf + ofs, piece);
				ofs+= piece;
			}
			_padded_data.resize(_feats.require_write_block);
			// Pad data with extra spaces finalized with \n and with amount that ensures
			// total sent size aligned by require_write_block plus extra require_write_block.
			// This is to workaround issue with head of busybox that consumes more than it was
			// asked for due to it uses stdin that has internal buffer.
			// Note that remote sh will handle this by skipping empty lines got from read until
			// getting unempty line with subsequent directive.
			if (ofs < len) {
				const size_t piece = len - ofs;
				memcpy(_padded_data.data(), (const char *)buf + ofs, piece);
				memset(_padded_data.data() + piece, ' ', _feats.require_write_block - piece);
				WritePiece(piece, (const char *)buf, _padded_data.size());
			}
			memset(_padded_data.data(), ' ', _padded_data.size());
//			No need as read skips leading spaces so they can arrive as part of subsequent directive
//			_padded_data.back() = '\n';
			_app->Send(_padded_data.data(), _padded_data.size());

		} else {
			WritePiece(len, (const char *)buf, len);
		}
	}

	void WritePiece(size_t target_len, const char *data, size_t len)
	{
		while (_pending_replies > 1) {
			WaitReply();
		}
		++_seq;
		_app->Send(StrPrintf("%lu %lu\n", (unsigned long)_seq, (unsigned long)target_len));
		_app->Send(data, len);
		++_pending_replies;
	}
};

std::shared_ptr<IFileReader> ProtocolSHELL::FileGet(const std::string &path, unsigned long long resume_pos)
{
	fprintf(stderr, "*24\n");
	_exec_cmd.reset();

	if (!_feats.support_read) {
		throw ProtocolUnsupportedError("read unsupported");
	}
	if (resume_pos != 0 && !_feats.support_read_resume) {
		throw ProtocolUnsupportedError("read-resume unsupported");
	}

	return std::make_shared<SHELLFileReader>(_app, path, resume_pos);
}

std::shared_ptr<IFileWriter> ProtocolSHELL::FilePut(const std::string &path, mode_t mode, unsigned long long size_hint, unsigned long long resume_pos)
{
	fprintf(stderr, "*25\n");
	_exec_cmd.reset();

	if (!_feats.support_write) {
		throw ProtocolUnsupportedError("write unsupported");
	}
	if (resume_pos != 0 && !_feats.support_write_resume) {
		throw ProtocolUnsupportedError("write-resume unsupported");
	}

	return std::make_shared<SHELLFileWriter>(_app, path, mode, size_hint, resume_pos, _feats);
}
