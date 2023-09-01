#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <signal.h>
#include <time.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <utils.h>
#include <crc64.h>
#include <SharedResource.h>
#include <memory>
#include "../Erroring.h"
#include "ProtocolInitDeinitCmd.h"

#define NRLOCK_GROUP	"nrlock"

ProtocolInitDeinitCmd::~ProtocolInitDeinitCmd()
{
}

class ProtocolInitDeinitCmdImpl : public ProtocolInitDeinitCmd
{
	StringConfig _protocol_options;
	uint64_t _lock_id;
	SharedResource _sr_local, _sr_global;
	std::unique_ptr<SharedResource::Reader> _sr_global_rlock;
	std::string _host, _username, _password;
	char _port_sz[32];

	static uint64_t CalcLockID(const char *proto, const std::string &host, unsigned int port, const std::string &username)
	{
		const std::string &nrlock_id_std = StrPrintf("%s:%s@%s:%u", proto, username.c_str(), host.c_str(), port);
		return crc64(0, (const unsigned char *)nrlock_id_std.data(), nrlock_id_std.size());
	}

	void Run(const char *cmd_name, bool singular)
	{
		const std::string &cmd = _protocol_options.GetString(cmd_name);
		if (cmd.empty()) {
			return;
		}

		const std::string &extra = _protocol_options.GetString("Extra");
		const std::string &stg = InMyCache(
			StrPrintf("nrstg/%llx", (unsigned long long)_lock_id).c_str());

		int child_stdout[2] = {-1, -1};
		if (pipe(child_stdout) < 0) {
			throw std::runtime_error("Can't create pipe");
		}

		MakeFDCloexec(child_stdout[0]);
		pid_t pid = fork();
		if (pid == 0) {
			dup2(child_stdout[1], 1);
			dup2(child_stdout[1], 2);
			close(child_stdout[1]);

			setenv("HOST", _host.c_str(), 1);
			setenv("PORT", _port_sz, 1);
			setenv("USER", _username.c_str(), 1);
			setenv("PASSWORD", _password.c_str(), 1);
			setenv("EXTRA", extra.c_str(), 1);
			setenv("SINGULAR", singular ? "1" : "0", 1);
			setenv("STORAGE", stg.c_str(), 1);

			execlp("sh", "sh", "-c", cmd.c_str(), nullptr);
			int err = errno ? errno : -1;
			fprintf(stderr, "exec shell error %d\n", err);
			_exit(err);
			exit(err);
		}
		close(child_stdout[1]);

		time_t time_limit = time(NULL) + std::max(3, _protocol_options.GetInt("CommandTimeLimit", 30));
		char input_buf[0x1000] = {}; // must be greater than 0x100!
		size_t input_buf_len = 0;
		bool done = false;
		for (;;) {
			fd_set fdr, fde;
			FD_ZERO(&fdr);
			FD_ZERO(&fde);
			FD_SET(child_stdout[0], &fdr);
			FD_SET(child_stdout[0], &fde);

			time_t time_now = time(NULL);
			if (time_now >= time_limit) {
				break;
			}

			struct timeval tv = {time_limit - time_now, 0};
			int sv = select(child_stdout[0] + 1, &fdr, nullptr, &fde, &tv);
			if (sv < 0) {
				if (errno == EAGAIN || errno == EINTR)
					continue;

				break;
			}

			if (FD_ISSET(child_stdout[0], &fdr)) {
				if (input_buf_len >= sizeof(input_buf)) {
					input_buf_len = sizeof(input_buf) - 0x100;
					memcpy(&input_buf[input_buf_len], "...", 3);
					input_buf_len+= 3;
				}
				ssize_t r = read(child_stdout[0], &input_buf[input_buf_len], sizeof(input_buf) - input_buf_len);
				if (r <= 0) {
					done = true;
					break;
				}

				input_buf_len+= (size_t)r;
			}

			if (FD_ISSET(child_stdout[0], &fde)) {
				done = true;
				break;
			}
		}

		int status = -1;
		if (!done) {
			kill(pid, 9);
			waitpid(pid, &status, 0);
			throw ProtocolError("Timeout", std::string(input_buf, input_buf_len).c_str(), ETIMEDOUT);
		}

		waitpid(pid, &status, 0);
		if (status != 0) {
			throw ProtocolError("Error", std::string(input_buf, input_buf_len).c_str(), status);
		}
	}

	bool IsSingular() noexcept
	{
		if (!_sr_global.LockWrite(0))
			return false;

		_sr_global.UnlockWrite();
		return true;
	}

public:
	ProtocolInitDeinitCmdImpl(const char *proto, const std::string &host, unsigned int port,
			const std::string &username, const std::string &password, const StringConfig &protocol_options)
		:
		_protocol_options(protocol_options),
		_lock_id(CalcLockID(proto, host, port, username)),
		_sr_local(NRLOCK_GROUP, _lock_id & (~(uint64_t)1)),
		_sr_global(NRLOCK_GROUP, _lock_id & (uint64_t)1),
		_host(host),
		_username(username),
		_password(password)
	{
		snprintf(_port_sz, sizeof(_port_sz) - 1, "%d", port);
		SharedResource::Writer srl_w(_sr_local);
		const bool singular = IsSingular();
		_sr_global_rlock.reset(new SharedResource::Reader(_sr_global));
		Run("Command", singular);
	}

	virtual ~ProtocolInitDeinitCmdImpl()
	{
		SharedResource::Writer srl_w(_sr_local);
		_sr_global_rlock.reset(new SharedResource::Reader(_sr_global));
		const bool singular = IsSingular();

		try {
			Run("CommandDeinit", singular);

		} catch (std::exception &e) {
			fprintf(stderr, "~ProtocolInitDeinitCmdImpl: '%s'\n", e.what());

		} catch (...) {
			fprintf(stderr, "~ProtocolInitDeinitCmdImpl: ???\n");
		}
	}
};

ProtocolInitDeinitCmd *ProtocolInitDeinitCmd::Make(const char *proto, const std::string &host, unsigned int port,
		const std::string &username, const std::string &password, const StringConfig &protocol_options)
{
	if (protocol_options.GetString("Command").empty()
			&& protocol_options.GetString("CommandDeinit").empty()) {
		return nullptr;
	}
	return new ProtocolInitDeinitCmdImpl(proto, host, port, username, password, protocol_options);
}
