#include <stdio.h>
#include <wchar.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <assert.h>
#include <signal.h>
#include <fcntl.h>
#include <string>
#include <vector>
#include <ScopeHelpers.h>
#include <Threaded.h>
#include <StringConfig.h>
#include <CheckedCast.hpp>

#include "../SitesConfig.h"

#include "HostRemote.h"
#include "Protocol/Protocol.h"
#include "IPC.h"
#include "Globals.h"
#include "PooledStrings.h"

#include "UI/Activities/InteractiveLogin.h"
#include "UI/Activities/ConfirmNewServerIdentity.h"

////////////////////////////////////////////

HostRemote::HostRemote(const std::string &site)
	: _site(site)
{
	SitesConfig sc;
	_identity.protocol = sc.GetProtocol(_site);
	_identity.host = sc.GetHost(_site);
	_identity.port = sc.GetPort(_site);
	_identity.username = sc.GetUsername(_site);
	_login_mode = sc.GetLoginMode(_site);
	_password = sc.GetPassword(_site);
	_options = sc.GetProtocolOptions(_site, _identity.protocol);

	if (_login_mode == 0) {
		_password.clear();
	}
}

HostRemote::HostRemote(const std::string &protocol, const std::string &host,
	unsigned int port, const std::string &username, const std::string &password)
	:
	_login_mode( ( (username.empty() || username == "anonymous") && password.empty()) ? 0 : (password.empty() ? 1 : 2)),
	_password(password)
{
	_identity.protocol = protocol;
	_identity.host = host;
	_identity.port = port;
	_identity.username = username;
}

HostRemote::~HostRemote()
{
	AssertNotBusy();
}


std::shared_ptr<IHost> HostRemote::Clone()
{
	auto cloned = std::make_shared<HostRemote>();

	std::unique_lock<std::mutex> locker(_mutex);
	cloned->_site = _site;
	cloned->_identity = _identity;
	cloned->_login_mode = _login_mode;
	cloned->_password = _password;
	cloned->_options = _options;
	cloned->_peer = 0;
	cloned->_busy = false;
	cloned->_cloning = true;

	return cloned;
}

std::string HostRemote::SiteName()
{
	if (!_site.empty()) {
		std::string out;
		out+= '<';
		out+= _site;
		out+= '>';
		return out;
	}

	const auto *pi = ProtocolInfoLookup(_identity.protocol.c_str());

	std::unique_lock<std::mutex> locker(_mutex);
	std::string out = _identity.protocol;
	out+= ":";
	if (!_identity.username.empty()) {
		out+= _identity.username;
		out+= "@";
	}
	out+= _identity.host;
	if (_identity.port && pi &&
	 pi->default_port != -1 && pi->default_port != (int)_identity.port) {
		out+= StrPrintf(":%u", _identity.port);
	}

	for (auto &c : out) {
		if (c == '/') c = '\\';
	}
	return out;
}

void HostRemote::GetIdentity(Identity &identity)
{
	std::unique_lock<std::mutex> locker(_mutex);
	identity = _identity;
}

void HostRemote::BusySet()
{
	_busy = true;
}

void HostRemote::BusyReset()
{
	if (!_busy) {
		fprintf(stderr, "HostRemote::BusyReset while not busy\n");
		abort();
	}
	_busy = false;
}

void HostRemote::AssertNotBusy()
{
	if (_busy) {
		fprintf(stderr, "HostRemote::AssertNotBusy while busy\n");
		abort();
	}
}

void HostRemote::CheckReady()
{
	AssertNotBusy();

	if (_cloning) {
		_cloning = false;
		ReInitialize();
	}

	if (_peer == 0) {
		throw std::runtime_error("IPC broken");
	}
}

void HostRemote::OnBroken()
{
	IPCRecver::SetFD(-1);
	IPCSender::SetFD(-1);
	_peer = 0;
}

void HostRemote::ReInitialize() throw (std::runtime_error)
{
	OnBroken();

	AssertNotBusy();

	const auto *pi = ProtocolInfoLookup(_identity.protocol.c_str());
	if (!pi) {
		throw std::runtime_error(std::string("Wrong protocol: ").append(_identity.protocol));
	}

	if (_identity.host.empty() && pi->require_server) {
		throw std::runtime_error("No server specified");
	}

	int master2broker[2] = {-1, -1};
	int broker2master[2] = {-1, -1};
	int r = pipe(master2broker);
	if (r == 0) {
		r = pipe(broker2master);
		if (r != 0)
			CheckedCloseFDPair(master2broker);
	}
	if (r != 0)
		throw IPCError("pipe() error", errno);

	fcntl(master2broker[1], F_SETFD, FD_CLOEXEC);
	fcntl(broker2master[0], F_SETFD, FD_CLOEXEC);


	std::string broker_path = StrWide2MB(G.plugin_path);
	CutToSlash(broker_path, true);
	broker_path+= pi->broker;
	broker_path+= ".broker";
	char arg1[32], arg2[32];
	itoa(master2broker[0], arg1, 10);
	itoa(broker2master[1], arg2, 10);

	fprintf(stderr, "NetRocks: starting broker '%s' '%s' '%s'\n", broker_path.c_str(), arg1, arg2);
	const bool use_tsocks = G.GetGlobalConfigBool("UseProxy", false);
	pid_t pid = fork();
	if (pid == 0) {
		if (use_tsocks) {
			setenv("LD_PRELOAD", "libtsocks.so", 1);
			setenv("TSOCKS_CONFFILE", G.tsocks_config.c_str(), 1);
		}
		if (fork() == 0) {
			execl(broker_path.c_str(), broker_path.c_str(), arg1, arg2, NULL);
			_exit(-1);
			exit(-2);
		}
		_exit(0);
		exit(0);

	} else if (pid > 0) {
		waitpid(pid, 0, 0);
	}
//	G.info.FSF->Execute(cmdstr.c_str(), EF_HIDEOUT | EF_NOWAIT); //_interactive

	CheckedCloseFD(master2broker[0]);
	CheckedCloseFD(broker2master[1]);

	IPCRecver::SetFD(broker2master[0]);
	IPCSender::SetFD(master2broker[1]);

	uint32_t ipc_ver_magic = 0;

	try {
		RecvPOD(ipc_ver_magic);
		RecvPOD(_peer);

	} catch (std::exception &) {
		OnBroken();
		throw std::runtime_error(StrPrintf("Failed to start '%s' '%s' '%s'", broker_path.c_str(), arg1, arg2));
	}

	if (ipc_ver_magic != IPC_VERSION_MAGIC) {
		OnBroken();
		throw std::runtime_error(StrPrintf("Wrong version of '%s' '%s' '%s'", broker_path.c_str(), arg1, arg2));
	}

	std::unique_lock<std::mutex> locker(_mutex);
	for (unsigned int auth_failures = 0;;) {
//		fprintf(stderr, "login_mode=%d retry=%d\n", login_mode, retry);
		if (_login_mode == 1) {
			std::string tmp_username = _identity.username, tmp_password = _password;
			locker.unlock();
			if (!InteractiveLogin(SiteName(), auth_failures, tmp_username, tmp_password)) {
				SendString(std::string());
				OnBroken();
				throw AbortError();
			}
			locker.lock();
			_identity.username = tmp_username;
			_password = tmp_password;
		}

		SendString(_identity.protocol);
		SendString(_identity.host);
		SendPOD(_identity.port);
		SendPOD(_login_mode);
		SendString(_identity.username);
		SendString(_password);
		SendString(_options);

		locker.unlock();

		IPCProtocolInitStatus status;
		RecvPOD(status);

		locker.lock();

		if (status == IPC_PI_OK) {
			if (_login_mode == 1) {
				// on next reinitialization try to autouse same password that now succeeded
				_login_mode = 2;
			}
			break;
		}

		std::string info;
		RecvString(info);
		fprintf(stderr, "HostRemote::ReInitialize: status=%d info='%s'\n", status, info.c_str());

		switch (status) {
			case IPC_PI_SERVER_IDENTITY_CHANGED:
				if (!OnServerIdentityChanged(info))  {
					OnBroken();
					throw ProtocolError("Server identity mismatch", info.c_str());
				}
				if (_login_mode == 1) {
					// on next reinitialization try to autouse same password that was already entered
					_login_mode = 2;
				}

				break;

			case IPC_PI_AUTHORIZATION_FAILED:
				if (auth_failures >= 3) {
					OnBroken();
					throw ProtocolError("Authorization failed", info.c_str());
				}

				++auth_failures;
				_login_mode = 1;
				break;

			case IPC_PI_PROTOCOL_ERROR:
				OnBroken();
				throw ProtocolError(info);

			case IPC_PI_GENERIC_ERROR:
				OnBroken();
				throw std::runtime_error(info);

			default:
				OnBroken();
				throw IPCError("Unexpected protocol init status", status);
		}
	}
}

bool HostRemote::OnServerIdentityChanged(const std::string &new_identity)
{
	StringConfig protocol_options(_options);
	const std::string &prev_identity = protocol_options.GetString("ServerIdentity");
	protocol_options.SetString("ServerIdentity", new_identity);

	if (!prev_identity.empty()) {
		switch (ConfirmNewServerIdentity(_site, new_identity, !_site.empty()).Ask()) {
			case ConfirmNewServerIdentity::R_ALLOW_ONCE: {
				_options = protocol_options.Serialize();
				return true;
			}

			case ConfirmNewServerIdentity::R_ALLOW_ALWAYS:
				break;

			default:
				return false;
		}
	}

	_options = protocol_options.Serialize();

	if (!_site.empty()) {
		SitesConfig().PutProtocolOptions(_site, _identity.protocol, _options);
	}
	return true;
}

void HostRemote::Abort()
{
	AbortReceiving();
	if (_peer != 0) {
		kill(_peer, SIGQUIT);
		_peer = 0;
	}
}

void HostRemote::RecvReply(IPCCommand cmd)
{
	IPCCommand reply = RecvCommand();
	if (reply != cmd) {
		if (reply == IPC_ERROR) {
			std::string str;
			RecvString(str);
			throw ProtocolError(str);

		} else if (reply == IPC_UNSUPPORTED) {
			std::string str;
			RecvString(str);
			throw ProtocolUnsupportedError(str);
		}

		throw IPCError("Wrong command reply", (unsigned int)cmd);
	}
}

mode_t HostRemote::GetMode(const std::string &path, bool follow_symlink) throw (std::runtime_error)
{
	CheckReady();

	SendCommand(IPC_GET_MODE);
	SendString(path);
	SendPOD(follow_symlink);
	RecvReply(IPC_GET_MODE);
	mode_t out;
	RecvPOD(out);
	return out;
}

unsigned long long HostRemote::GetSize(const std::string &path, bool follow_symlink) throw (std::runtime_error)
{
	CheckReady();

	SendCommand(IPC_GET_SIZE);
	SendString(path);
	SendPOD(follow_symlink);
	RecvReply(IPC_GET_SIZE);
	unsigned long long out;
	RecvPOD(out);
	return out;
}

void HostRemote::GetInformation(FileInformation &file_info, const std::string &path, bool follow_symlink) throw (std::runtime_error)
{
	CheckReady();

	SendCommand(IPC_GET_INFORMATION);
	SendString(path);
	SendPOD(follow_symlink);
	RecvReply(IPC_GET_INFORMATION);
	RecvPOD(file_info);
}

void HostRemote::FileDelete(const std::string &path) throw (std::runtime_error)
{
	CheckReady();

	SendCommand(IPC_FILE_DELETE);
	SendString(path);
	RecvReply(IPC_FILE_DELETE);
}

void HostRemote::DirectoryDelete(const std::string &path) throw (std::runtime_error)
{
	CheckReady();

	SendCommand(IPC_DIRECTORY_DELETE);
	SendString(path);
	RecvReply(IPC_DIRECTORY_DELETE);
}

void HostRemote::DirectoryCreate(const std::string &path, mode_t mode) throw (std::runtime_error)
{
	CheckReady();

	SendCommand(IPC_DIRECTORY_CREATE);
	SendString(path);
	SendPOD(mode);
	RecvReply(IPC_DIRECTORY_CREATE);
}

void HostRemote::Rename(const std::string &path_old, const std::string &path_new) throw (std::runtime_error)
{
	CheckReady();

	SendCommand(IPC_RENAME);
	SendString(path_old);
	SendString(path_new);
	RecvReply(IPC_RENAME);
}


void HostRemote::SetTimes(const std::string &path, const timespec &access_time, const timespec &modification_time) throw (std::runtime_error)
{
	CheckReady();

	SendCommand(IPC_SET_TIMES);
	SendString(path);
	SendPOD(access_time);
	SendPOD(modification_time);
	RecvReply(IPC_SET_TIMES);
}

void HostRemote::SetMode(const std::string &path, mode_t mode) throw (std::runtime_error)
{
	CheckReady();

	SendCommand(IPC_SET_MODE);
	SendString(path);
	SendPOD(mode);
	RecvReply(IPC_SET_MODE);
}

void HostRemote::SymlinkCreate(const std::string &link_path, const std::string &link_target) throw (std::runtime_error)
{
	CheckReady();

	SendCommand(IPC_SYMLINK_CREATE);
	SendString(link_path);
	SendString(link_target);
	RecvReply(IPC_SYMLINK_CREATE);
}

void HostRemote::SymlinkQuery(const std::string &link_path, std::string &link_target) throw (std::runtime_error)
{
	CheckReady();

	SendCommand(IPC_SYMLINK_QUERY);
	SendString(link_path);
	RecvReply(IPC_SYMLINK_QUERY);
	RecvString(link_target);
}

////////////////////////////////////////


class HostRemoteDirectoryEnumer : public IDirectoryEnumer
{
	std::shared_ptr<HostRemote> _conn;
	bool _complete = false;

public:
	HostRemoteDirectoryEnumer(std::shared_ptr<HostRemote> conn, const std::string &path)
		: _conn(conn)
	{
		_conn->BusySet();
	}

	virtual ~HostRemoteDirectoryEnumer()
	{
		if (!_complete) try {
			_conn->SendCommand(IPC_STOP);
			_conn->RecvReply(IPC_STOP);
		} catch (std::exception &) {
		}
		_conn->BusyReset();
	}

	virtual bool Enum(std::string &name, std::string &owner, std::string &group, FileInformation &file_info) throw (std::runtime_error)
	{
		if (_complete)
			return false;

		try {
			_conn->SendCommand(IPC_DIRECTORY_ENUM);
			_conn->RecvReply(IPC_DIRECTORY_ENUM);

			_conn->RecvString(name);
			if (name.empty()) {
				_complete = true;
				return false;
			}

			_conn->RecvString(owner);
			_conn->RecvString(group);
			_conn->RecvPOD(file_info);
			return true;

		} catch (...) {
			_complete = true;
			throw;
		}
  	}
};

std::shared_ptr<IDirectoryEnumer> HostRemote::DirectoryEnum(const std::string &path) throw (std::runtime_error)
{
	CheckReady();

	SendCommand(IPC_DIRECTORY_ENUM);
	SendString(path);
	RecvReply(IPC_DIRECTORY_ENUM);

	return std::make_shared<HostRemoteDirectoryEnumer>(shared_from_this(), path);
}


class HostRemoteFileIO : public IFileReader, public IFileWriter
{
	std::shared_ptr<HostRemote> _conn;
	bool _complete = false, _writing;

	void EnsureComplete()
	{
		if (!_complete) {
			_complete = true;
			_conn->SendPOD((size_t)0); // zero length mean stop requested
			_conn->RecvReply(IPC_STOP);
		}
	}

public:
	HostRemoteFileIO(std::shared_ptr<HostRemote> conn, bool writing)
		: _conn(conn), _writing(writing)
	{
	}

	virtual ~HostRemoteFileIO()
	{
		try {
			EnsureComplete();

		} catch (std::exception &ex) {
			fprintf(stderr, "~HostRemoteFileIO(_writing=%d): %s\n", _writing, ex.what());
//			_conn->Abort();
		}
	}

	virtual size_t Read(void *buf, size_t len) throw (std::runtime_error)
	{
		assert(!_writing);
		if (_complete || len == 0) {
			return 0;
		}

		try {
			_conn->SendPOD(len);
			_conn->RecvReply(IPC_FILE_GET);
			size_t recv_len = len;
			_conn->RecvPOD(recv_len);
			if (recv_len == 0) {
				_complete = true;
				return 0;
			}
			if (recv_len > len) {
				fprintf(stderr, "HostRemoteFileIO::Read: IPC gonna mad\n");
				_conn->Abort();
				throw ProtocolError("Read: IPC gonna mad");
			}
			_conn->Recv(buf, recv_len);
			return recv_len;

		} catch (...) {
			_complete = true;
			throw;
		}
	}

	virtual void Write(const void *buf, size_t len) throw (std::runtime_error)
	{
		assert(_writing);
		if (len == 0) {
			return;
		}
		if (_complete) {
			throw std::runtime_error("Write: already complete");
		}

		try {
			_conn->SendPOD(len);
			_conn->Send(buf, len);
			_conn->RecvReply(IPC_FILE_PUT);

		} catch (...) {
			_complete = true;
			throw;
		}
	}

	virtual void WriteComplete() throw (std::runtime_error)
	{
		EnsureComplete();
	}
};


std::shared_ptr<IFileReader> HostRemote::FileGet(const std::string &path, unsigned long long resume_pos) throw (std::runtime_error)
{
	CheckReady();

	SendCommand(IPC_FILE_GET);
	SendString(path);
	SendPOD(resume_pos);
	RecvReply(IPC_FILE_GET);

	return std::make_shared<HostRemoteFileIO>(shared_from_this(), false);
}

std::shared_ptr<IFileWriter> HostRemote::FilePut(const std::string &path, mode_t mode, unsigned long long size_hint, unsigned long long resume_pos) throw (std::runtime_error)
{
	CheckReady();

	SendCommand(IPC_FILE_PUT);
	SendString(path);
	SendPOD(mode);
	SendPOD(size_hint);
	SendPOD(resume_pos);
	RecvReply(IPC_FILE_PUT);

	return std::make_shared<HostRemoteFileIO>(shared_from_this(), true);
}


void HostRemote::ExecuteCommand(const std::string &working_dir, const std::string &command_line, const std::string &fifo) throw (std::runtime_error)
{
	CheckReady();

	SendCommand(IPC_EXECUTE_COMMAND);
	SendString(working_dir);
	SendString(command_line);
	SendString(fifo);
	RecvReply(IPC_EXECUTE_COMMAND);
}

bool HostRemote::Alive()
{
	return _peer != 0;
}
