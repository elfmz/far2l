#include <string>
#include <vector>
#include <all_far.h>
#include <fstdlib.h>
#include <KeyFileHelper.h>
#include <ScopeHelpers.h>
#include <CheckedCast.hpp>

#include "SiteConnection.h"
#include "Protocol/Protocol.h"
#include "IPC.h"
#include "Globals.h"
#include "PooledStrings.h"

#include "UI/InteractiveLogin.h"

SiteConnection::SiteConnection(const std::string &site, int OpMode) throw (std::runtime_error)
	: _site(site)
{
}

void SiteConnection::ReInitialize() throw (std::runtime_error)
{
	KeyFileHelper kfh(G.config.c_str());
	const std::string &protocol = kfh.GetString(_site.c_str(), "Protocol");
	const std::string &host = kfh.GetString(_site.c_str(), "Host");
	unsigned int port = (unsigned int)kfh.GetInt(_site.c_str(), "Port");
	unsigned int login_mode = (unsigned int)kfh.GetInt(_site.c_str(), "LoginMode", 2);
	std::string username = kfh.GetString(_site.c_str(), "Username");
	std::string password = kfh.GetString(_site.c_str(), "Password"); // TODO: de/obfuscation
	const std::string &directory = kfh.GetString(_site.c_str(), "Directory");
	const std::string &options = kfh.GetString(_site.c_str(), "Options");
	if (protocol.empty() || host.empty())
		throw std::runtime_error("Bad site configuration");

	_site_info = protocol;
	_site_info+= "://";
	if (!username.empty()) {
		_site_info+= username;
		_site_info+= '@';
	}
	if (login_mode == 0) {
		password.clear();
	}

	_site_info+= host;
	if (port > 0) {
		char sz[64];
		snprintf(sz, sizeof(sz), ":%d", port);
		_site_info+= sz;
	}

	int master2slave[2] = {-1, -1};
	int slave2master[2] = {-1, -1};
	int r = pipe(master2slave);
	if (r == 0) {
		r = pipe(slave2master);
		if (r != 0)
			CheckedCloseFDPair(master2slave);
	}
	if (r != 0)
		throw IPCError("pipe() error", errno);

	fcntl(master2slave[1], F_SETFD, FD_CLOEXEC);
	fcntl(slave2master[0], F_SETFD, FD_CLOEXEC);

	char lib_cmdline[0x200];
	snprintf(lib_cmdline, sizeof(lib_cmdline), "%d %d", master2slave[0], slave2master[1]);

	G.info.FSF->ExecuteLibrary(G.plugin_path.c_str(),
		"SiteConnectionSlaveMain", lib_cmdline, EF_HIDEOUT | EF_NOWAIT);

	close(master2slave[0]);
	close(slave2master[1]);

	IPCRecver::SetFD(slave2master[0]);
	IPCSender::SetFD(master2slave[1]);

	for (unsigned int retry = 0;; ++retry) {

		if (retry > 0) {
			if (retry == 3) {
				throw ProtocolAuthFailedError();
			}
			login_mode = 1;
		}

//		fprintf(stderr, "login_mode=%d retry=%d\n", login_mode, retry);
		if (login_mode == 1) {
			if (!InteractiveLogin(_site, retry, username, password)) {
				SendString(std::string());
				throw AbortError();
			}
		}

		SendString(protocol);
		SendString(host);
		SendPOD(port);
		SendPOD(login_mode);
		SendString(username);
		SendString(password);
		SendString(directory);
		SendString(options);

		unsigned int status;
		RecvPOD(status);
		if (status == 0) {
			break;
		}
		if (status == 2 || status == 3) {
			std::string what;
			RecvString(what);
			if (status == 2)
				throw ProtocolError(what);

			throw std::runtime_error(what);
		}
	}
}

SiteConnection::~SiteConnection()
{
}

const std::string &SiteConnection::Site() const
{
	return _site;
}

const std::string &SiteConnection::SiteInfo() const
{
	return _site_info;
}

void SiteConnection::RecvReply(IPCCommand cmd)
{
	IPCCommand reply = RecvCommand();
	if (reply != cmd) {
		if (reply == IPC_ERROR) {
			std::string str;
			RecvString(str);
			throw ProtocolError(str);
		}

		throw IPCError("Wrong command reply", (unsigned int)cmd);
	}
}

void SiteConnection::TransactContinueOrAbort()
{
	if (_user_requesting_abort) {
		_user_requesting_abort = false;
		SendCommand(IPC_ABORT);
		RecvReply(IPC_ABORT);
		throw AbortError();
	}

	SendCommand(IPC_CONTINUE);
	RecvReply(IPC_CONTINUE);
}

bool SiteConnection::IsBroken()
{
	try {
		SendCommand(IPC_IS_BROKEN);
		RecvReply(IPC_IS_BROKEN);
		bool out;
		RecvPOD(out);
		return out;

	} catch (std::exception &e) {
		fprintf(stderr, "SiteConnection::IsBroken: %s\n", e.what());
		return true;
	}
}

mode_t SiteConnection::GetMode(const std::string &path, bool follow_symlink) throw (std::runtime_error)
{
	SendCommand(IPC_GET_MODE);
	SendString(path);
	SendPOD(follow_symlink);
	RecvReply(IPC_GET_MODE);
	mode_t out;
	RecvPOD(out);
	return out;
}

unsigned long long SiteConnection::GetSize(const std::string &path, bool follow_symlink) throw (std::runtime_error)
{
	SendCommand(IPC_GET_SIZE);
	SendString(path);
	SendPOD(follow_symlink);
	RecvReply(IPC_GET_SIZE);
	unsigned long long out;
	RecvPOD(out);
	return out;
}

void SiteConnection::DirectoryEnum(const std::string &path, FP_SizeItemList &il, int OpMode, EnumStatusCallback *cb) throw (std::runtime_error)
{
	_user_requesting_abort = false;

	SendCommand(IPC_DIRECTORY_ENUM);
	SendString(path);
	RecvReply(IPC_DIRECTORY_ENUM);

	std::string name, owner, group;
	FileInformation file_info;
	for (;;) {
		TransactContinueOrAbort();
		RecvString(name);
		if (name.empty()) break;
		RecvString(owner);
		RecvString(group);
		RecvPOD(file_info);
		PluginPanelItem tmp = {};
		strncpy(tmp.FindData.cFileName, name.c_str(), sizeof(tmp.FindData.cFileName) - 1);
		tmp.FindData.nFileSizeLow = (DWORD)(file_info.size & 0xffffffff);
		tmp.FindData.nFileSizeHigh = (DWORD)(file_info.size >> 32);
		tmp.FindData.dwUnixMode = file_info.mode;
		tmp.FindData.dwFileAttributes = WINPORT(EvaluateAttributesA)(file_info.mode, name.c_str());
		tmp.Owner = (char *)PooledString(owner);
		tmp.Group = (char *)PooledString(group);
		if (!il.Add(&tmp) || (cb && !cb->OnEnumEntry())) {
			_user_requesting_abort = true;
		}
	}
}

void SiteConnection::DirectoryEnum(const std::string &path, UnixFileList &ufl, int OpMode, EnumStatusCallback *cb) throw (std::runtime_error)
{
	_user_requesting_abort = false;

	SendCommand(IPC_DIRECTORY_ENUM);
	SendString(path);
	RecvReply(IPC_DIRECTORY_ENUM);

	std::string owner, group;
	UnixFileEntry e;
	for (;;) {
		TransactContinueOrAbort();
		RecvString(e.name);
		if (e.name.empty()) break;
		RecvString(owner);
		RecvString(group);
		e.owner = (char *)PooledString(owner);
		e.group = (char *)PooledString(group);
		RecvPOD(e.info);
		ufl.emplace_back(e);
		if (cb && !cb->OnEnumEntry()) {
			_user_requesting_abort = true;
		}
	}
}

void SiteConnection::FileDelete(const std::string &path) throw (std::runtime_error)
{
	SendCommand(IPC_FILE_DELETE);
	SendString(path);
	RecvReply(IPC_FILE_DELETE);
}

void SiteConnection::DirectoryDelete(const std::string &path) throw (std::runtime_error)
{
	SendCommand(IPC_DIRECTORY_DELETE);
	SendString(path);
	RecvReply(IPC_DIRECTORY_DELETE);
}

void SiteConnection::DirectoryCreate(const std::string &path, mode_t mode) throw (std::runtime_error)
{
	SendCommand(IPC_DIRECTORY_CREATE);
	SendString(path);
	SendPOD(mode);
	RecvReply(IPC_DIRECTORY_CREATE);
}

void SiteConnection::Rename(const std::string &path_old, const std::string &path_new) throw (std::runtime_error)
{
	SendCommand(IPC_RENAME);
	SendString(path_old);
	SendString(path_new);
	RecvReply(IPC_RENAME);
}

void SiteConnection::FileGet(const std::string &path_remote, const std::string &path_local, mode_t mode, unsigned long long resume_pos, IOStatusCallback *cb) throw (std::runtime_error)
{
	_user_requesting_abort = false;
	SendCommand(IPC_FILE_GET);
	SendString(path_remote);
	SendPOD(resume_pos);
	RecvReply(IPC_FILE_GET);

	FDScope fd(open(path_local.c_str(), O_RDWR | O_CREAT, mode));
	const char *err = nullptr;
	if (!fd.Valid()) {
		err = "Create file error";

	} else  if (ftruncate(fd, resume_pos) == -1) {
		err = "Truncate file error";

	} else if (lseek(fd, resume_pos, SEEK_SET) != CheckedCast<off_t>(resume_pos) ) {
		err = "Seek file error";
	}

	if (err != nullptr) {
		fprintf(stderr, "SiteConnection::FileGet: %s\n", err);
		SendCommand(IPC_ABORT);
		RecvReply(IPC_ABORT);
		throw std::runtime_error(err);
	}

	std::vector<char> buf;
	for (;;) {
		TransactContinueOrAbort();
		size_t piece;
		RecvPOD(piece);
		if (piece == 0)
			break;

		if (piece == (size_t)-1)
			throw ProtocolError("Protocol read error");

		if (buf.size() < piece)
			buf.resize(piece);

		Recv(&buf[0], piece);
		for (size_t i = 0; i < piece; ) {
			ssize_t wr = write(fd, &buf[i], piece - i);
			if (wr <= 0) {
				fprintf(stderr, "SiteConnection::FileGet: IO error!\n");
				SendCommand(IPC_ABORT);
				RecvReply(IPC_ABORT);
				throw ProtocolError("Write file IO error");
			}

			i+= (size_t)wr;
		}
		if (cb && !cb->OnIOStatus(piece))
			_user_requesting_abort = true;
	}
}

void SiteConnection::FilePut(const std::string &path_remote, const std::string &path_local, mode_t mode, unsigned long long resume_pos, IOStatusCallback *cb) throw (std::runtime_error)
{
	_user_requesting_abort = false;

	FDScope fd(open(path_local.c_str(), O_RDONLY));
	if (!fd.Valid())
		throw std::runtime_error("Open file error");

	if (lseek(fd, resume_pos, SEEK_SET) != CheckedCast<off_t>(resume_pos) )
		throw std::runtime_error("Seek file error");

	SendCommand(IPC_FILE_PUT);
	SendString(path_remote);
	SendPOD(mode);
	SendPOD(resume_pos);
	RecvReply(IPC_FILE_PUT);

	char buf[0x10000];
	for (;;) {
		ssize_t piece = read(fd, buf, sizeof(buf));
		if (piece < 0) {
			SendCommand(IPC_ABORT);
			RecvReply(IPC_ABORT);
			throw std::runtime_error("Read file error");
		}
		TransactContinueOrAbort();

		SendPOD(piece);
		if (piece == 0)
			break;

		Send(buf, piece);
		if (cb && !cb->OnIOStatus(piece))
			_user_requesting_abort = true;
	}
}
