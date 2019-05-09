#include <stdio.h>
#include <wchar.h>
#include <unistd.h>
#include <assert.h>
#include <fcntl.h>
#include <string>
#include <vector>
#include <KeyFileHelper.h>
#include <ScopeHelpers.h>
#include <CheckedCast.hpp>

#include "HostRemote.h"
#include "Protocol/Protocol.h"
#include "Protocol/ProtocolOptions.h"
#include "IPC.h"
#include "Globals.h"
#include "PooledStrings.h"

#include "UI/InteractiveLogin.h"
#include "UI/ConfirmNewServerIdentity.h"

HostRemote::HostRemote(const std::string &site, int OpMode) throw (std::runtime_error)
	: _site(site)
{
	KeyFileHelper kfh(G.config.c_str());
	_protocol = kfh.GetString(_site.c_str(), "Protocol");
	_host = kfh.GetString(_site.c_str(), "Host");
	_port = (unsigned int)kfh.GetInt(_site.c_str(), "Port");
	_login_mode = (unsigned int)kfh.GetInt(_site.c_str(), "LoginMode", 2);
	_username = kfh.GetString(_site.c_str(), "Username");
	_password = kfh.GetString(_site.c_str(), "Password"); // TODO: de/obfuscation
	_directory = kfh.GetString(_site.c_str(), "Directory");
	_options = kfh.GetString(_site.c_str(), "Options");
	if (_protocol.empty() || _host.empty())
		throw std::runtime_error("Bad site configuration");

	_site_info = _protocol;
	_site_info+= "://";
	if (!_username.empty()) {
		_site_info+= _username;
		_site_info+= '@';
	}

	if (_login_mode == 0) {
		_password.clear();
	}

	_site_info+= _host;
	if (_port > 0) {
		char sz[64];
		snprintf(sz, sizeof(sz), ":%d", _port);
		_site_info+= sz;
	}
}

std::string HostRemote::SiteName() const
{
	return _site;
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

void HostRemote::OnBroken()
{
	_broken = true;
}

void HostRemote::CheckReady()
{
	AssertNotBusy();
	if (_broken) {
		throw std::runtime_error("IPC broken");
	}
}


void HostRemote::ReInitialize() throw (std::runtime_error)
{
	AssertNotBusy();

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

	wchar_t lib_cmdline[0x200] = {};
	swprintf(lib_cmdline, ARRAYSIZE(lib_cmdline) - 1, L"%d %d", master2broker[0], broker2master[1]);

	fprintf(stderr, "G.plugin_path.c_str()='%ls'\n", G.plugin_path.c_str());
	G.info.FSF->ExecuteLibrary(G.plugin_path.c_str(),
		L"HostRemoteBrokerMain", lib_cmdline, EF_HIDEOUT | EF_NOWAIT);

	close(master2broker[0]);
	close(broker2master[1]);

	IPCRecver::SetFD(broker2master[0]);
	IPCSender::SetFD(master2broker[1]);

	for (unsigned int auth_failures = 0;;) {
//		fprintf(stderr, "login_mode=%d retry=%d\n", login_mode, retry);
		if (_login_mode == 1) {
			if (!InteractiveLogin(_site, auth_failures, _username, _password)) {
				SendString(std::string());
				throw AbortError();
			}
		}

		SendString(_protocol);
		SendString(_host);
		SendPOD(_port);
		SendPOD(_login_mode);
		SendString(_username);
		SendString(_password);
		SendString(_directory);
		SendString(_options);

		IPCProtocolInitStatus status;
		RecvPOD(status);
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
				if (!OnServerIdentityChanged(info)) 
					throw ProtocolError("Server identity mismatch", info.c_str());
				if (_login_mode == 1) {
					// on next reinitialization try to autouse same password that was already entered
					_login_mode = 2;
				}

				break;

			case IPC_PI_AUTHORIZATION_FAILED:
				if (auth_failures >= 3)
					throw ProtocolError("Authorization failed", info.c_str());

				++auth_failures;
				_login_mode = 1;
				break;

			case IPC_PI_PROTOCOL_ERROR:
				throw ProtocolError(info);

			case IPC_PI_GENERIC_ERROR:
				throw std::runtime_error(info);

			default:
				throw IPCError("Unexpected protocol init status", status);
		}
	}
	_broken = false;
}

bool HostRemote::OnServerIdentityChanged(const std::string &new_identity)
{
	ProtocolOptions protocol_options(_options);
	const std::string &prev_identity = protocol_options.GetString("ServerIdentity");
	protocol_options.SetString("ServerIdentity", new_identity);

	if (!prev_identity.empty()) {
		switch (ConfirmNewServerIdentity(_site, new_identity).Ask()) {
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

	KeyFileHelper kfh(G.config.c_str());
	kfh.PutString(_site.c_str(), "Options", _options.c_str());
	return true;
}

HostRemote::~HostRemote()
{
	AssertNotBusy();
}

void HostRemote::Abort()
{
	AbortReceiving();
}

void HostRemote::RecvReply(IPCCommand cmd)
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

bool HostRemote::IsBroken()
{
	AssertNotBusy();
	if (_broken)
		return true;

	try {
		SendCommand(IPC_IS_BROKEN);
		RecvReply(IPC_IS_BROKEN);
		bool out;
		RecvPOD(out);
		return out;

	} catch (std::exception &ex) {
		fprintf(stderr, "HostRemote::IsBroken: %s\n", ex.what());
		OnBroken();
		return true;
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

	return std::shared_ptr<IDirectoryEnumer>(new HostRemoteDirectoryEnumer(shared_from_this(), path));
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

	return std::shared_ptr<IFileReader>((IFileReader *)new HostRemoteFileIO(shared_from_this(), false));
}

std::shared_ptr<IFileWriter> HostRemote::FilePut(const std::string &path, mode_t mode, unsigned long long resume_pos) throw (std::runtime_error)
{
	CheckReady();

	SendCommand(IPC_FILE_PUT);
	SendString(path);
	SendPOD(mode);
	SendPOD(resume_pos);
	RecvReply(IPC_FILE_PUT);

	return std::shared_ptr<IFileWriter>((IFileWriter *)new HostRemoteFileIO(shared_from_this(), true));
}
