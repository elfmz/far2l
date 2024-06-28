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
#include <UtfConvert.hpp>
#include <StringConfig.h>
#include <CheckedCast.hpp>

#include "HostRemote.h"
#include "Protocol/Protocol.h"
#include "IPC.h"
#include "Globals.h"
#include "PooledStrings.h"

#include "UI/Activities/InteractiveLogin.h"
#include "UI/Activities/ConfirmNewServerIdentity.h"

////////////////////////////////////////////

HostRemote::HostRemote(const SiteSpecification &site_specification)
	: _site_specification(site_specification)
{
	SitesConfig sc(_site_specification.sites_cfg_location);
	_identity.protocol = sc.GetProtocol(_site_specification.site);
	_identity.host = sc.GetHost(_site_specification.site);
	_identity.port = sc.GetPort(_site_specification.site);
	_identity.username = sc.GetUsername(_site_specification.site);
	_login_mode = sc.GetLoginMode(_site_specification.site);
	_password = sc.GetPassword(_site_specification.site);
	_options = sc.GetProtocolOptions(_site_specification.site, _identity.protocol);

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

const std::string &HostRemote::CodepageLocal2Remote(const std::string &str)
{
	if (_codepage == CP_UTF8 || str.empty()) {
		return str;
	}

	_codepage_wstr.clear();
	size_t uc_len = str.size();
	const unsigned wide_cvt_rv = UtfConvertStd(str.data(), uc_len, _codepage_wstr, false);
	if (wide_cvt_rv != 0) {
		fprintf(stderr, "%s(%s): wide_cvt_rv=%u\n", __FUNCTION__, str.c_str(), wide_cvt_rv);
		return str;
	}

	_codepage_str.resize(_codepage_wstr.size() * 8); // ought to should be enough for any encoding
	const int cp_cvt_rv = WINPORT(WideCharToMultiByte)(_codepage, 0, _codepage_wstr.c_str(),
			_codepage_wstr.size(), &_codepage_str[0], (int)_codepage_str.size(), nullptr, nullptr);
	if (cp_cvt_rv <= 0) {
		fprintf(stderr, "%s(%s): cp_cvt_rv=%u\n", __FUNCTION__, str.c_str(), cp_cvt_rv);
		return str;
	}

	_codepage_str.resize(cp_cvt_rv);
	return _codepage_str;
}

void HostRemote::CodepageRemote2Local(std::string &str)
{
	if (_codepage == CP_UTF8 || str.empty()) {
		return;
	}

	_codepage_wstr.resize(str.size() * 2);
	const int cp_cvt_rv = WINPORT(MultiByteToWideChar)(_codepage,
		0, str.c_str(), str.size(), &_codepage_wstr[0], (int)_codepage_wstr.size());
	if (cp_cvt_rv <= 0) {
		fprintf(stderr, "%s(%s): cp_cvt_rv=%u\n", __FUNCTION__, str.c_str(), cp_cvt_rv);
		return;
	}
	_codepage_wstr.resize(cp_cvt_rv);

	str.clear();
	size_t uc_len = _codepage_wstr.size();
	UtfConvertStd(_codepage_wstr.c_str(), uc_len, str, false);
}

static void TimespecAdjust(timespec &ts, int adjust)
{
	if (ts.tv_sec == 0) {
		; // dont adjust zero timestamps

	} else if (adjust > 0) {
		ts.tv_sec+= adjust;

	} else if (adjust < 0) {
		ts.tv_sec-= -adjust;
	}
}

const timespec &HostRemote::TimespecLocal2Remote(const timespec &ts)
{
	_ts_2_remote = ts;
	TimespecAdjust(_ts_2_remote, -_timeadjust);
	return _ts_2_remote;
}

void HostRemote::TimespecRemote2Local(timespec &ts)
{
	TimespecAdjust(ts, _timeadjust);
}


std::shared_ptr<IHost> HostRemote::Clone()
{
	auto cloned = std::make_shared<HostRemote>();

	std::unique_lock<std::mutex> locker(_mutex);
	cloned->_site_specification = _site_specification;
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
	if (_site_specification.IsValid()) {
		std::string out;
		out+= '<';
		out+= _site_specification.ToString();
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
	ASSERT_MSG(_busy, "not busy");
	_busy = false;
}

void HostRemote::AssertNotBusy()
{
	ASSERT_MSG(!_busy, "busy");
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
	IPCEndpoint::SetFD(-1, -1);
	_peer = 0;
	_init_deinit_cmd.reset();
}

void HostRemote::ReInitialize()
{
	OnBroken();
	_aborted = false;
	AssertNotBusy();

	const auto *pi = ProtocolInfoLookup(_identity.protocol.c_str());
	if (!pi) {
		throw std::runtime_error(std::string("Wrong protocol: ").append(_identity.protocol));
	}

	if (_identity.host.empty() && pi->require_server) {
		throw std::runtime_error("No server specified");
	}

	std::string broker_path = StrWide2MB(G.plugin_path);
	CutToSlash(broker_path, true);
	std::string broker_pathname = broker_path;
	broker_pathname+= pi->broker;
	broker_pathname+= ".broker";

	PipeIPCFD ipc_fd;

	StringConfig sc_options(_options);
	_codepage = sc_options.GetInt("CodePage", CP_UTF8);
	_timeadjust = sc_options.GetInt("TimeAdjust", 0);

	char keep_alive_arg[32];
	sprintf(keep_alive_arg, "%d", sc_options.GetInt("KeepAlive", 0));

	std::string work_path = broker_path;
	TranslateInstallPath_Lib2Share(work_path);

	fprintf(stderr, "NetRocks: starting broker '%s' '%s' '%s' in '%s'\n",
		broker_pathname.c_str(), ipc_fd.broker_arg_r, ipc_fd.broker_arg_w, work_path.c_str());

	UnlinkScope prxf_cfg;
	std::string prxf = sc_options.GetString("Proxifier");
	if (!prxf.empty()) {
		prxf_cfg = InMyTempFmt("NetRocks/proxy/run_%u.cfg", getpid());
		const std::string &cfg_content = sc_options.GetString(std::string("Proxifier_").append(prxf).c_str());
		if (!WriteWholeFile(prxf_cfg.c_str(), cfg_content)) {
			fprintf(stderr, "NetRocks: error %d creating proxifier config '%s'\n", errno, prxf_cfg.c_str());
			prxf.clear();
		}
	}

	pid_t pid = fork();
	if (pid == 0) {
		// avoid holding arbitrary dir for broker's whole runtime
		if (chdir(work_path.c_str()) == -1) {
			fprintf(stderr, "chdir '%s' error %u\n", work_path.c_str(), errno);
			if (chdir(broker_path.c_str()) == -1) {
				fprintf(stderr, "chdir '%s' error %u\n", broker_path.c_str(), errno);
				if (chdir("/tmp") == -1) {
					fprintf(stderr, "chdir '/tmp' error %u\n", errno);
				}
			}
		}
		if (prxf == "tsocks") {
			setenv("LD_PRELOAD", "libtsocks.so", 1);
			setenv("TSOCKS_CONFFILE", prxf_cfg.c_str(), 1);
		}
		if (fork() == 0) {
			if (prxf == "proxychains") {
				execlp("proxychains", "proxychains", "-f", prxf_cfg.c_str(),
					broker_pathname.c_str(), ipc_fd.broker_arg_r, ipc_fd.broker_arg_w, keep_alive_arg, NULL);
			} else {
				execl(broker_pathname.c_str(),
					broker_pathname.c_str(), ipc_fd.broker_arg_r, ipc_fd.broker_arg_w, keep_alive_arg, NULL);
			}
			_exit(-1);
			exit(-2);
		}
		_exit(0);
		exit(0);

	} else if (pid != -1) {
		_init_deinit_cmd.reset(InitDeinitCmd::sMake(_identity.protocol,
			_identity.host, _identity.port, _identity.username, _password, sc_options, _aborted));
		waitpid(pid, 0, 0);
	} else {
		perror("fork");
	}
//	G.info.FSF->Execute(cmdstr.c_str(), EF_HIDEOUT | EF_NOWAIT); //_interactive

	IPCEndpoint::SetFD(ipc_fd.broker2master[0], ipc_fd.master2broker[1]);

	// so far so good - avoid automatic closing of pipes FDs in ipc_fd's d-tor
	ipc_fd.Detach();

	uint32_t ipc_ver_magic = 0;

	try {
		pid_t peer = 0;
		RecvPOD(ipc_ver_magic);
		RecvPOD(peer);
		_peer = peer;

	} catch (std::exception &) {
		OnBroken();
		throw std::runtime_error(StrPrintf("Failed to start '%s' '%s' '%s'", broker_path.c_str(), ipc_fd.broker_arg_r, ipc_fd.broker_arg_w));
	}

	if (ipc_ver_magic != IPC_VERSION_MAGIC) {
		OnBroken();
		throw std::runtime_error(StrPrintf("Wrong version of '%s' '%s' '%s'", broker_path.c_str(), ipc_fd.broker_arg_r, ipc_fd.broker_arg_w));
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
		SendString(CodepageLocal2Remote(_identity.username));
		SendString(CodepageLocal2Remote(_password));
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
				throw PipeIPCError("Unexpected protocol init status", status);
		}
	}
}

bool HostRemote::OnServerIdentityChanged(const std::string &new_identity)
{
	StringConfig protocol_options(_options);
	const std::string &prev_identity = protocol_options.GetString("ServerIdentity");
	protocol_options.SetString("ServerIdentity", new_identity);

	if (!prev_identity.empty()) {
		switch (ConfirmNewServerIdentity(_site_specification.ToString(), new_identity, _site_specification.IsValid()).Ask()) {
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

	if (_site_specification.IsValid()) {
		SitesConfig(_site_specification.sites_cfg_location).SetProtocolOptions(_site_specification.site, _identity.protocol, _options);
	}

	return true;
}

void HostRemote::Abort()
{
	_aborted = true;
	pid_t peer = _peer.exchange(0);
	AbortReceiving();
	if (peer != 0) {
		kill(peer, SIGQUIT);
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

		throw PipeIPCError("Wrong command reply", (unsigned int)cmd);
	}
}

mode_t HostRemote::GetMode(const std::string &path, bool follow_symlink)
{
	CheckReady();

	SendCommand(IPC_GET_MODE);
	SendString(CodepageLocal2Remote(path));
	SendPOD(follow_symlink);
	RecvReply(IPC_GET_MODE);
	mode_t out;
	RecvPOD(out);
	return out;
}

void HostRemote::GetModes(bool follow_symlink, size_t count, const std::string *paths, mode_t *modes) noexcept
{
	size_t j = 0;
	try {
		CheckReady();

		SendCommand(IPC_GET_MODES);
		SendPOD(follow_symlink);
		SendPOD(count);
		for (size_t i = 0; i < count; ++i) {
			SendString(CodepageLocal2Remote(paths[i]));
		}
		RecvReply(IPC_GET_MODES);
		for (; j < count; ++j) {
			RecvPOD(modes[j]);
		}

	} catch (std::exception &e) {
		fprintf(stderr, "%s: %s\n", __FUNCTION__, e.what());
	} catch (...) {
		fprintf(stderr, "%s: ...\n", __FUNCTION__);
	}
	for (; j < count; ++j) {
		modes[j] = ~(mode_t)0;
	}
}


unsigned long long HostRemote::GetSize(const std::string &path, bool follow_symlink)
{
	CheckReady();

	SendCommand(IPC_GET_SIZE);
	SendString(CodepageLocal2Remote(path));
	SendPOD(follow_symlink);
	RecvReply(IPC_GET_SIZE);
	unsigned long long out;
	RecvPOD(out);
	return out;
}

void HostRemote::GetInformation(FileInformation &file_info, const std::string &path, bool follow_symlink)
{
	CheckReady();

	SendCommand(IPC_GET_INFORMATION);
	SendString(CodepageLocal2Remote(path));
	SendPOD(follow_symlink);
	RecvReply(IPC_GET_INFORMATION);
	RecvPOD(file_info);
	TimespecRemote2Local(file_info.access_time);
	TimespecRemote2Local(file_info.modification_time);
	TimespecRemote2Local(file_info.status_change_time);
}

void HostRemote::FileDelete(const std::string &path)
{
	CheckReady();

	SendCommand(IPC_FILE_DELETE);
	SendString(CodepageLocal2Remote(path));
	RecvReply(IPC_FILE_DELETE);
}

void HostRemote::DirectoryDelete(const std::string &path)
{
	CheckReady();

	SendCommand(IPC_DIRECTORY_DELETE);
	SendString(CodepageLocal2Remote(path));
	RecvReply(IPC_DIRECTORY_DELETE);
}

void HostRemote::DirectoryCreate(const std::string &path, mode_t mode)
{
	CheckReady();

	SendCommand(IPC_DIRECTORY_CREATE);
	SendString(CodepageLocal2Remote(path));
	SendPOD(mode);
	RecvReply(IPC_DIRECTORY_CREATE);
}

void HostRemote::Rename(const std::string &path_old, const std::string &path_new)
{
	CheckReady();

	SendCommand(IPC_RENAME);
	SendString(CodepageLocal2Remote(path_old));
	SendString(CodepageLocal2Remote(path_new));
	RecvReply(IPC_RENAME);
}


void HostRemote::SetTimes(const std::string &path, const timespec &access_time, const timespec &modification_time)
{
	CheckReady();

	SendCommand(IPC_SET_TIMES);
	SendString(CodepageLocal2Remote(path));
	SendPOD(TimespecLocal2Remote(access_time));
	SendPOD(TimespecLocal2Remote(modification_time));
	RecvReply(IPC_SET_TIMES);
}

void HostRemote::SetMode(const std::string &path, mode_t mode)
{
	CheckReady();

	SendCommand(IPC_SET_MODE);
	SendString(CodepageLocal2Remote(path));
	SendPOD(mode);
	RecvReply(IPC_SET_MODE);
}

void HostRemote::SymlinkCreate(const std::string &link_path, const std::string &link_target)
{
	CheckReady();

	SendCommand(IPC_SYMLINK_CREATE);
	SendString(CodepageLocal2Remote(link_path));
	SendString(CodepageLocal2Remote(link_target));
	RecvReply(IPC_SYMLINK_CREATE);
}

void HostRemote::SymlinkQuery(const std::string &link_path, std::string &link_target)
{
	CheckReady();

	SendCommand(IPC_SYMLINK_QUERY);
	SendString(CodepageLocal2Remote(link_path));
	RecvReply(IPC_SYMLINK_QUERY);
	RecvString(link_target);
	CodepageRemote2Local(link_target);
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

	virtual bool Enum(std::string &name, std::string &owner, std::string &group, FileInformation &file_info)
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
			_conn->CodepageRemote2Local(name);
			_conn->CodepageRemote2Local(owner);
			_conn->CodepageRemote2Local(group);
			_conn->TimespecRemote2Local(file_info.access_time);
			_conn->TimespecRemote2Local(file_info.modification_time);
			_conn->TimespecRemote2Local(file_info.status_change_time);
			return true;

		} catch (...) {
			_complete = true;
			throw;
		}
	}
};

std::shared_ptr<IDirectoryEnumer> HostRemote::DirectoryEnum(const std::string &path)
{
	CheckReady();

	SendCommand(IPC_DIRECTORY_ENUM);
	SendString(CodepageLocal2Remote(path));
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

	virtual size_t Read(void *buf, size_t len)
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

	virtual void Write(const void *buf, size_t len)
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

	virtual void WriteComplete()
	{
		EnsureComplete();
	}
};


std::shared_ptr<IFileReader> HostRemote::FileGet(const std::string &path, unsigned long long resume_pos)
{
	CheckReady();

	SendCommand(IPC_FILE_GET);
	SendString(CodepageLocal2Remote(path));
	SendPOD(resume_pos);
	RecvReply(IPC_FILE_GET);

	return std::make_shared<HostRemoteFileIO>(shared_from_this(), false);
}

std::shared_ptr<IFileWriter> HostRemote::FilePut(const std::string &path, mode_t mode, unsigned long long size_hint, unsigned long long resume_pos)
{
	CheckReady();

	SendCommand(IPC_FILE_PUT);
	SendString(CodepageLocal2Remote(path));
	SendPOD(mode);
	SendPOD(size_hint);
	SendPOD(resume_pos);
	RecvReply(IPC_FILE_PUT);

	return std::make_shared<HostRemoteFileIO>(shared_from_this(), true);
}


void HostRemote::ExecuteCommand(const std::string &working_dir, const std::string &command_line, const std::string &fifo)
{
	CheckReady();

	SendCommand(IPC_EXECUTE_COMMAND);
	SendString(CodepageLocal2Remote(working_dir));
	SendString(CodepageLocal2Remote(command_line));
	SendString(fifo);
	RecvReply(IPC_EXECUTE_COMMAND);
}

bool HostRemote::Alive()
{
	return _peer != 0;
}
