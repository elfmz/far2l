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
#include <atomic>
#include <memory>
#include "../Erroring.h"
#include "InitDeinitCmd.h"

#define NRLOCK_PRIVATE	"nrlock/prv"
#define NRLOCK_GROUP	"nrlock"

//////////////////////////////////////////////////////////////
InitDeinitCmd::~InitDeinitCmd()
{
}

class InitDeinitCmdImpl : public InitDeinitCmd
{
	struct Cleanup
	{
		~Cleanup()
		{
			// cleanup ALL unused private locks, not only own
			// this is to avoid cache dir pollution by dangling lock files
			std::vector<uint64_t> ids;
			SharedResource::sEnum(NRLOCK_PRIVATE, ids);
			for (const auto &id : ids) {
				SharedResource sr_group(NRLOCK_GROUP, GroupLockID(id));
				SharedResource::Writer srw_group(sr_group, 0);
				if (srw_group.Locked()) {
					SharedResource sr_id(NRLOCK_PRIVATE, id);
					if (sr_id.LockWrite(0)) {
						sr_id.UnlockWrite();
						SharedResource::sCleanup(NRLOCK_PRIVATE, id);
					}
				}
			}
		}
	} _cleanup; // its first field to be destroyed last, after all SharedResource fields released

	StringConfig _protocol_options;
	uint64_t _lock_id;
	SharedResource _sr_group;
	std::unique_ptr<SharedResource> _sr_private; // created only after _sr_group locked
	std::unique_ptr<SharedResource::Reader> _sr_private_rlock;
	std::string _host, _username, _password;
	char _port_sz[32];
	std::atomic<bool> &_aborted;

	static uint64_t CalcLockID(const std::string &proto, const std::string &host, unsigned int port,
		const std::string &username, const StringConfig &protocol_options)
	{
		// derive lock id from connection's host specification, credentials and creation timestamp (if any)
		const std::string &nrlock_id_str = StrPrintf("%llx %s:%s@%s:%x",
			protocol_options.GetHexULL("TS"), proto.c_str(), username.c_str(), host.c_str(), port);
		return crc64(0, (const unsigned char *)nrlock_id_str.data(), nrlock_id_str.size());
	}

	static uint64_t GroupLockID(uint64_t lock_id)
	{
		// more bits makes more uncleanable 'group' lock files
		// however less 'group' lock files causing more probability of unrelated connections to block each other
		return lock_id & 0xff; // up to 256 group locks is reasonable amount
	}

	void Run(const char *cmd_name, bool singular)
	{
		const std::string &cmd = _protocol_options.GetString(cmd_name);
		if (cmd.empty()) {
			return;
		}

		if (_aborted) {
			throw AbortError();
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

			struct timeval tv = {std::min((time_t)1, time_limit - time_now), 0};
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
			if (_aborted) {
				break;
			}
		}

		int status = -1;
		if (!done) {
			kill(pid, 9);
			waitpid(pid, &status, 0);
			if (_aborted) {
				throw AbortError();
			}
			throw ProtocolError("Timeout", std::string(input_buf, input_buf_len).c_str(), ETIMEDOUT);
		}

		waitpid(pid, &status, 0);
		if (status != 0) {
			throw ProtocolError("Error", std::string(input_buf, input_buf_len).c_str(), status);
		}
	}

	bool PrivateMayTakeWriteLock() noexcept
	{
		if (!_sr_private->LockWrite(0))
			return false;

		_sr_private->UnlockWrite();
		return true;
	}

public:
	InitDeinitCmdImpl(const std::string &proto, const std::string &host, unsigned int port,
			const std::string &username, const std::string &password,
			const StringConfig &protocol_options, std::atomic<bool> &aborted)
		:
		_protocol_options(protocol_options),
		_lock_id(CalcLockID(proto, host, port, username, protocol_options)),
		_sr_group(NRLOCK_GROUP, GroupLockID(_lock_id)),
		_host(host),
		_username(username),
		_password(password),
		_aborted(aborted)
	{
		fprintf(stderr, "InitDeinitCmdImpl(%s:%s@%s:%u): _lock_id=%llx\n",
			proto.c_str(), username.c_str(), host.c_str(), port, (unsigned long long)_lock_id);
		snprintf(_port_sz, sizeof(_port_sz) - 1, "%d", port);
		SharedResource::Writer srw(_sr_group);
		_sr_private.reset(new SharedResource(NRLOCK_PRIVATE, _lock_id));
		Run("Command", PrivateMayTakeWriteLock());
		_sr_private_rlock.reset(new SharedResource::Reader(*_sr_private));
	}

	virtual ~InitDeinitCmdImpl()
	{
		try {
			SharedResource::Writer srw(_sr_group);
			_sr_private_rlock.reset();
			Run("CommandDeinit", PrivateMayTakeWriteLock());
			_sr_private.reset();
		} catch (std::exception &e) {
			fprintf(stderr, "~InitDeinitCmdImpl: '%s'\n", e.what());

		} catch (...) {
			fprintf(stderr, "~InitDeinitCmdImpl: ???\n");
		}
	}

	virtual void Abort()
	{
		_aborted = true;
	}
};

InitDeinitCmd *InitDeinitCmd::sMake(const std::string &proto, const std::string &host, unsigned int port,
		const std::string &username, const std::string &password, const StringConfig &protocol_options, std::atomic<bool> &aborted)
{
	if (protocol_options.GetString("Command").empty()
			&& protocol_options.GetString("CommandDeinit").empty()) {
		return nullptr;
	}
	return new InitDeinitCmdImpl(proto, host, port, username, password, protocol_options, aborted);
}
