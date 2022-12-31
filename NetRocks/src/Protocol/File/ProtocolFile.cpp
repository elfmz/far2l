#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <algorithm>
#include <StringConfig.h>
#include <utils.h>
#include "ProtocolFile.h"


std::shared_ptr<IProtocol> CreateProtocol(const std::string &protocol, const std::string &host, unsigned int port,
	const std::string &username, const std::string &password, const std::string &options)
{
	return std::make_shared<ProtocolFile>(host, port, username, password, options);
}

ProtocolFile::ProtocolFile(const std::string &host, unsigned int port,
	const std::string &username, const std::string &password, const std::string &options)
{
	StringConfig protocol_options(options);
	const std::string &cmd = protocol_options.GetString("Command");
	if (!cmd.empty()) {
		const std::string &extra = protocol_options.GetString("Extra");
		char port_sz[32];
		snprintf(port_sz, sizeof(port_sz) - 1, "%d", port);
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

			setenv("HOST", host.c_str(), 1);
			setenv("PORT", port_sz, 1);
			setenv("USER", username.c_str(), 1);
			setenv("PASSWORD", password.c_str(), 1);
			setenv("EXTRA", extra.c_str(), 1);

			execlp("sh", "sh", "-c", cmd.c_str(), nullptr);
			int err = errno ? errno : -1;
			fprintf(stderr, "exec shell error %d\n", err);
			_exit(err);
			exit(err);
		}
		close(child_stdout[1]);

		time_t time_limit = time(NULL) + std::max(3, protocol_options.GetInt("CommandTimeLimit", 30));
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

		int status;
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
}

ProtocolFile::~ProtocolFile()
{
}

