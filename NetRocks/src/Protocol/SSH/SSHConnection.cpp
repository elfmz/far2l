#include <sys/stat.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <time.h>
#include <unistd.h>
#include <netinet/tcp.h>
#include <fcntl.h>
#include <signal.h>
#include <assert.h>
#include <string.h>
#include <utils.h>
#include <string>
#include <vector>
#include <map>
#include <set>
#include <deque>
#include <algorithm>
#include <os_call.hpp>
#include <ScopeHelpers.h>
#include <Environment.h>
#include <Threaded.h>
#include "../Protocol.h"
#include "SSHConnection.h"


void SSHSessionDeleter(ssh_session res)
{
	if (res) {
		ssh_disconnect(res);
		ssh_free(res);
	}
}


void SSHChannelDeleter(ssh_channel res)
{
	if (res) {
		ssh_channel_close(res);
		ssh_channel_free(res);
	}
}

static std::string GetSSHPubkeyHash(ssh_session ssh)
{
	ssh_key pub_key = {};
	int rc = ssh_get_publickey(ssh, &pub_key);
	if (rc != SSH_OK)
		throw ProtocolError("Pubkey", ssh_get_error(ssh), rc);

	unsigned char *hash = nullptr;
	size_t hlen = 0;
	rc = ssh_get_publickey_hash(pub_key, SSH_PUBLICKEY_HASH_SHA1, &hash, &hlen);
	ssh_key_free(pub_key);

	if (rc != SSH_OK)
		throw ProtocolError("Pubkey hash", ssh_get_error(ssh), rc);

	std::string out;
	for (size_t i = 0; i < hlen; ++i) {
		out+= StrPrintf("%02x", (unsigned int)hash[i]);
	}
	ssh_clean_pubkey_hash(&hash);

	return out;
}


SSHConnection::SSHConnection(const std::string &host, unsigned int port, const std::string &username,
		const std::string &password, const StringConfig &protocol_options)
	:
	ssh(ssh_new())
{
	if (!ssh)
		throw ProtocolError("SSH session");

	ssh_options_set(ssh, SSH_OPTIONS_HOST, host.c_str());
	if (port > 0)
		ssh_options_set(ssh, SSH_OPTIONS_PORT, &port);

	ssh_options_set(ssh, SSH_OPTIONS_USER, username.c_str());

	if (protocol_options.GetInt("UseOpenSSHConfigs", 0) ) {
		struct stat s{};
		if (stat("/etc/ssh/ssh_config", &s) == 0) {
			ssh_options_parse_config(ssh, "/etc/ssh/ssh_config");
		}
		std::string per_user_config = GetMyHome();
		per_user_config+= "/.ssh/config";
		if (stat(per_user_config.c_str(), &s) == 0) {
			ssh_options_parse_config(ssh, per_user_config.c_str());
		}
	}

#if (LIBSSH_VERSION_INT >= SSH_VERSION_INT(0, 8, 0))
	if (protocol_options.GetInt("TcpNoDelay", 1) ) {
		int nodelay = 1;
		ssh_options_set(ssh, SSH_OPTIONS_NODELAY, &nodelay);
	}
#endif

	int ssh_verbosity = SSH_LOG_NOLOG;
	if (g_netrocks_verbosity == 2) {
		ssh_verbosity = SSH_LOG_WARNING;
	} else if (g_netrocks_verbosity > 2) {
		ssh_verbosity = SSH_LOG_PROTOCOL;
	}
	ssh_options_set(ssh, SSH_OPTIONS_LOG_VERBOSITY, &ssh_verbosity);

	const int compression = protocol_options.GetInt("Compression", 0);
	if (compression & 1) {
		ssh_options_set(ssh, SSH_OPTIONS_COMPRESSION_S_C, "yes");
	}
	if (compression & 2) {
		ssh_options_set(ssh, SSH_OPTIONS_COMPRESSION_C_S, "yes");
	}

	ssh_key priv_key {};
	std::string key_path_spec;
	if (protocol_options.GetInt("PrivKeyEnable", 0) != 0) {
		key_path_spec = protocol_options.GetString("PrivKeyPath");
		Environment::ExplodeCommandLine ecl(key_path_spec);
		if (ecl.empty()) {
			throw std::runtime_error(StrPrintf("No key file specified: \'%s\'", key_path_spec.c_str()));
		}
		int key_import_result = -1;
		for (const auto &key_path : ecl) {
			key_import_result = ssh_pki_import_privkey_file(key_path.c_str(), password.c_str(), nullptr, nullptr, &priv_key);
			if (key_import_result == SSH_ERROR && password.empty()) {
				key_import_result = ssh_pki_import_privkey_file(key_path.c_str(), nullptr, nullptr, nullptr, &priv_key);
			}
			if (key_import_result == SSH_OK) {
				break;
			}
		}
		switch (key_import_result) {
			case SSH_EOF:
				throw std::runtime_error(StrPrintf("Cannot read key file \'%s\'", key_path_spec.c_str()));

			case SSH_ERROR:
				throw ProtocolAuthFailedError();

			case SSH_OK:
				break;

			default:
				throw std::runtime_error(
					StrPrintf("Unexpected error %u while loading key from \'%s\'",
					key_import_result, key_path_spec.c_str()));
		}
	}

	int retries = std::max(protocol_options.GetInt("ConnectRetries", 2), 1);
	long timeout = std::max(protocol_options.GetInt("ConnectTimeout", 20), 1);

	ssh_options_set(ssh, SSH_OPTIONS_TIMEOUT, &timeout);

	// TODO: seccomp: if (protocol_options.GetInt("Sandbox") ) ...

	for (int attempt = 1; ; ++attempt) {
		time_t connect_attemp_ts = time(NULL);
		int rc = ssh_connect(ssh);
		if (rc == SSH_OK)
			break;

		if (attempt >= retries)
			throw ProtocolError("Connection", ssh_get_error(ssh), rc);

		// otherwise next connect complains that session is already connected
		ssh_disconnect(ssh);

		// retry with increasing up to 5 seconds delay
		time_t min_delay = (attempt > 5) ? 5 : attempt;
		time_t ts = time(NULL);
		if (ts >= connect_attemp_ts && ts - connect_attemp_ts < min_delay) {
			sleep(min_delay - (ts - connect_attemp_ts));
		}
	}


	int socket_fd = ssh_get_fd(ssh);
	if (socket_fd != -1) {
#if (LIBSSH_VERSION_INT < SSH_VERSION_INT(0, 8, 0))
		if (protocol_options.GetInt("TcpNoDelay", 1) ) {
			int nodelay = 1;
			if (setsockopt(socket_fd, IPPROTO_TCP, TCP_NODELAY, (void *)&nodelay, sizeof(nodelay)) == -1) {
				perror("SSHConnection - TCP_NODELAY");
			}
		}
#endif
		if (protocol_options.GetInt("TcpQuickAck") ) {
#ifdef TCP_QUICKACK
			int quickack = 1;
			if (setsockopt(socket_fd, IPPROTO_TCP, TCP_QUICKACK , (void *)&quickack, sizeof(quickack)) == -1) {
				perror("SSHConnection - TCP_QUICKACK ");
			}
#else
			fprintf(stderr, "SSHConnection: TCP_QUICKACK requested but not supported\n");
#endif
		}
	}

	const std::string &pub_key_hash = GetSSHPubkeyHash(ssh);
	if (pub_key_hash != protocol_options.GetString("ServerIdentity"))
		throw ServerIdentityMismatchError(pub_key_hash);

	if (priv_key) {
		int rc = ssh_userauth_publickey(ssh, username.empty() ? nullptr : username.c_str(), priv_key);
		if (rc != SSH_AUTH_SUCCESS) {
			fprintf(stderr, "ssh_userauth_publickey: %d '%s'\n" , rc, ssh_get_error(ssh));
			throw std::runtime_error("Key file authentication failed");
		}

	} else if (protocol_options.GetInt("InteractiveLogin", 0) != 0) {
		int rc;
		for (int loop = 0; loop < 3; ++loop) {
			rc = ssh_userauth_kbdint(ssh, username.empty() ? nullptr : username.c_str(), NULL);
			if (g_netrocks_verbosity > 0) {
				fprintf(stderr, "kbdint: %d\n", rc);
			}
			if (rc != SSH_AUTH_INFO) {
				break;
			}
			for (unsigned int i = 0, n = ssh_userauth_kbdint_getnprompts(ssh); i < n; ++i) {
				char echo[2] = {0, 0};
				const char *prompt = ssh_userauth_kbdint_getprompt(ssh, i, echo);
				int ans_rc = (i + 1 == n) ? ssh_userauth_kbdint_setanswer(ssh, i, password.c_str()) : -1;
				if (g_netrocks_verbosity > 0) {
					fprintf(stderr, "kbdint_setanswer[%d]: %d for '%s'\n", i, ans_rc, prompt);
				}
			}
		}
		if (rc != SSH_AUTH_SUCCESS) {
			throw ProtocolAuthFailedError();//"Authentication failed", ssh_get_error(ssh), rc);
		}

	} else {
		if (protocol_options.GetInt("SSHAgentEnable", 0) != 0) {
			const char *ssh_agent_sock = getenv("SSH_AUTH_SOCK");
			if (ssh_agent_sock && *ssh_agent_sock) {
				fprintf(stderr, "Using ssh-agent cuz SSH_AUTH_SOCK='%s'\n", ssh_agent_sock);
				int rc = ssh_userauth_agent(ssh, NULL);
				if (rc == SSH_AUTH_SUCCESS) {
					return;
				}
				throw std::runtime_error("SSH-agent authentication failed");
//				throw ProtocolAuthFailedError("SSH-agent");//"Authentication failed", ssh_get_error(ssh), rc);
			}
		}

		int rc = ssh_userauth_password(ssh, username.empty() ? nullptr : username.c_str(), password.c_str());
  		if (rc != SSH_AUTH_SUCCESS)
			throw ProtocolAuthFailedError();//"Authentication failed", ssh_get_error(ssh), rc);
	}
}

SSHConnection::~SSHConnection()
{
}

void SSHExecutedCommand::OnReadFDIn(const char *buf, size_t len)
{
	for (size_t ofs = 0; ofs < len; ) {
		uint32_t piece = (len - ofs >= 32768) ? 32768 : (uint32_t)len - ofs;
		ssize_t slen = ssh_channel_write(_channel, &buf[ofs], piece);
		if (slen <= 0) {
			throw std::runtime_error("channel write failed");
		}
		ofs+= std::min((size_t)slen, (size_t)piece);
	}
}

void SSHExecutedCommand::SendSignal(int sig)
{
	const char *sig_name;
	switch (sig) {
		case SIGALRM: sig_name = "ALRM"; break;
		case SIGFPE: sig_name = "FPE"; break;
		case SIGHUP: sig_name = "HUP"; break;
		case SIGILL: sig_name = "ILL"; break;
		case SIGINT: sig_name = "INT"; break;
		case SIGKILL: sig_name = "KILL"; break;
		case SIGPIPE: sig_name = "PIPE"; break;
		case SIGQUIT: sig_name = "QUIT"; break;
		case SIGSEGV: sig_name = "SEGV"; break;
		case SIGTERM: sig_name = "TERM"; break;
		case SIGUSR1: sig_name = "USR1"; break;
		case SIGUSR2: sig_name = "USR2"; break;
		case SIGABRT: default: sig_name = "ABRT"; break;
	}
	int rc = ssh_channel_request_send_signal(_channel, sig_name);
	if (rc == SSH_ERROR ) {
		throw ProtocolError("ssh send signal",  ssh_get_error(_conn->ssh));
	}
}


void SSHExecutedCommand::OnReadFDCtl(int fd)
{
	ExecFIFO_CtlMsg m;
	if (ReadAll(fd, &m, sizeof(m)) != sizeof(m)) {
		throw std::runtime_error("ctl read failed");
	}

	switch (m.cmd) {
		case ExecFIFO_CtlMsg::CMD_PTY_SIZE: {
			if (_pty) {
				ssh_channel_change_pty_size(_channel, m.u.pty_size.cols, m.u.pty_size.rows);
			}
		} break;

		case ExecFIFO_CtlMsg::CMD_SIGNAL: {
			SendSignal(m.u.signum);
		} break;
	}
}

void SSHExecutedCommand::IOLoop()
{
	FDScope fd_err(open((_fifo + ".err").c_str(), O_WRONLY));
	FDScope fd_out(open((_fifo + ".out").c_str(), O_WRONLY));
	FDScope fd_in(open((_fifo + ".in").c_str(), O_RDONLY));
	FDScope fd_ctl(open((_fifo + ".ctl").c_str(), O_RDONLY));

	if (!fd_err.Valid() || !fd_out.Valid() || !fd_in.Valid() || !fd_ctl.Valid())
		throw ProtocolError("fifo");

	// get PTY size that is sent immediately
	OnReadFDCtl(fd_ctl);

	if (_pty) {
		for (const auto &i : _conn->env_set) {
			int rc_env = ssh_channel_request_env(_channel, i.first.c_str(), i.second.c_str());
			if (rc_env != SSH_OK) {
				fprintf(stderr, "SSHExecutedCommand: error %d setting env '%s'\n", rc_env, i.first.c_str());
			}
		}
	}

	std::string cmd;
	if (!_working_dir.empty() && _working_dir != "." && _working_dir != "./") {
		cmd = _working_dir;
		QuoteCmdArg(cmd);
		cmd.insert(0, "cd ");
		cmd+= " && ";
	}

	cmd+= _command_line;

	int rc = ssh_channel_request_exec(_channel, cmd.c_str());
	if (rc != SSH_OK) {
		throw ProtocolError("ssh execute",  ssh_get_error(_conn->ssh));
	}

	fcntl(fd_in, F_SETFL, fcntl(fd_in, F_GETFL, 0) | O_NONBLOCK);

	for (unsigned int idle = 0;;) {
		char buf[0x8000];
		fd_set fdr, fde;
		FD_ZERO(&fdr);
		FD_ZERO(&fde);
		FD_SET(fd_ctl, &fdr);
		FD_SET(fd_in, &fdr);
		FD_SET(_kickass[0], &fdr);
		FD_SET(fd_ctl, &fde);
		FD_SET(fd_in, &fde);
		FD_SET(_kickass[0], &fde);
		struct timeval tv = {0, (idle > 1000) ? 100000 : ((idle > 0) ? 10000 : 0)};
		if (idle < 100000) {
			++idle;
		}
		int r = select(std::max(std::max((int)fd_ctl, (int)fd_in), _kickass[0]) + 1, &fdr, nullptr, &fde, &tv);
		if ( r < 0) {
			if (errno == EAGAIN || errno == EINTR)
				continue;

			throw std::runtime_error("select failed");
		}

		if (FD_ISSET(_kickass[0], &fde)) {
			throw std::runtime_error("kickass exception");
		}

		if (FD_ISSET(_kickass[0], &fdr)) {
			char c = 0;
			if (os_call_ssize(read, _kickass[0], (void*)&c, sizeof(c)) != 1) {
				throw std::runtime_error("kickass read failed");
			}
			if (c == 0) {
				throw std::runtime_error("kickass-driven exit");
			}

			// TODO: somehow keepalive
		}

		if (FD_ISSET(fd_in, &fdr)) { // || FD_ISSET(fd_in, &fde)
			ssize_t rlen = read(fd_in, buf, sizeof(buf));
			if (rlen <= 0) {
				if (errno != EAGAIN) {
					throw std::runtime_error("fd_in read failed");
				}
			} else {
				OnReadFDIn(buf, (size_t)rlen);
				idle = 0;
			}
		}
		if (FD_ISSET(fd_ctl, &fdr)) {// || FD_ISSET(fd_ctl, &fde)
			OnReadFDCtl(fd_ctl);
			idle = 0;
		}

		if (ssh_channel_is_eof(_channel)) {
			int status = ssh_channel_get_exit_status(_channel);
			if (status == 0) {
				_succeess = true;
			}
			FDScope fd_status(open((_fifo + ".status").c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0600));
			if (fd_status.Valid()) {
				WriteAll(fd_status, &status, sizeof(status));
			}
			throw std::runtime_error("channel EOF");
		}

		int rlen = ssh_channel_read_nonblocking(_channel, buf, sizeof(buf), 0);
		if (rlen > 0) {
			if (WriteAll(fd_out, buf, rlen) != (size_t)rlen) {
				throw std::runtime_error("output write failed");
			}
			idle = 0;
		}

		rlen = ssh_channel_read_nonblocking(_channel, buf, sizeof(buf), 1);
		if (rlen > 0) {
			if (WriteAll(fd_err, buf, rlen) != (size_t)rlen) {
				throw std::runtime_error("error write failed");
			}
			idle = 0;
		}
	}
}

void *SSHExecutedCommand::ThreadProc()
{
	try {
		fprintf(stderr, "SSHExecutedCommand: ENTERING [%s]\n", _command_line.c_str());
		IOLoop();
		fprintf(stderr, "SSHExecutedCommand: LEAVING\n");
	} catch (std::exception &ex) {
		fprintf(stderr, "SSHExecutedCommand: %s\n", ex.what());
	}

	return nullptr;
}

SSHExecutedCommand::SSHExecutedCommand(std::shared_ptr<SSHConnection> conn, const std::string &working_dir, const std::string &command_line, const std::string &fifo, bool pty)
	:
	_conn(conn),
	_working_dir(working_dir),
	_command_line(command_line),
	_fifo(fifo),
	_channel(ssh_channel_new(conn->ssh))
{
	if (!_channel)
		throw ProtocolError("ssh channel",  ssh_get_error(_conn->ssh));

	int rc = ssh_channel_open_session(_channel);
	if (rc != SSH_OK) {
		throw ProtocolError("ssh channel session",  ssh_get_error(_conn->ssh));
	}

	if (pty) {
		rc = ssh_channel_request_pty(_channel);
		if (rc == SSH_OK) {
			_pty = true;
		}
	}

	if (pipe_cloexec(_kickass) == -1) {
		throw ProtocolError("pipe",  ssh_get_error(_conn->ssh));
	}

	fcntl(_kickass[1], F_SETFL, fcntl(_kickass[1], F_GETFL, 0) | O_NONBLOCK);

	if (!StartThread()) {
		CheckedCloseFDPair(_kickass);
		throw std::runtime_error("start thread");
	}
}

SSHExecutedCommand::~SSHExecutedCommand()
{
	if (!WaitThread(0)) {
		char c = 0;
		if (os_call_ssize(write, _kickass[1], (const void*)&c, sizeof(c)) != 1) {
			perror("~SSHExecutedCommand: write kickass");
		}
		WaitThread();
	}
	CheckedCloseFDPair(_kickass);

	if (_succeess) {
		std::vector<std::string> parts;
		StrExplode(parts, _command_line, " ");
		if (parts.size() > 1) {
			if (parts[0] == "export") {
				std::vector<std::string> var_parts;
				StrExplode(var_parts, parts[1], "=");
				if (var_parts.size() > 0) {
					StrTrim(var_parts[0]);
					if (var_parts.size() > 1) {
						StrTrim(var_parts[1]);
						_conn->env_set[var_parts[0]] = var_parts[1];
					} else {
						_conn->env_set[var_parts[0]].clear();
					}
				}

			} else if (parts[0] == "unset") {
				StrTrim(parts[1]);
				_conn->env_set.erase(parts[1]);
			}
		}
	}
}

void SSHExecutedCommand::KeepAlive()
{
	char c = 1;
	if (os_call_ssize(write, _kickass[1], (const void*)&c, sizeof(c)) != 1) {
		perror("SSHExecutedCommand::KeepAlive: write kickass");
	}
}
