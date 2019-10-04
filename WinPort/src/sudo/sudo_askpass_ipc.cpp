#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#include <assert.h>
#include <utils.h>
#include <os_call.hpp>
#include <ScopeHelpers.h>

#include "sudo_askpass_ipc.h"
#include "sudo_private.h"

static std::string AskpassIpcFile(const char *ipc_id, const char *purpose)
{
	char sz[128] = {};
	snprintf(sz, sizeof(sz) - 1, "askpass_ipc/%s.%s", ipc_id, purpose);
	return InMyConfig(sz);
}

static std::string AskpassIpcFile(const unsigned long ipc_id, const char *purpose)
{
	char sz[128] = {};
	snprintf(sz, sizeof(sz) - 1, "askpass_ipc/%lx.%s", ipc_id, purpose);
	return InMyConfig(sz);
}

static void set_nonblock(int sc)
{
    int flags = fcntl(sc, F_GETFL, 0);
    assert(flags != -1);
    fcntl(sc, F_SETFL, flags | O_NONBLOCK);
}

static void set_block(int sc)
{
    int flags = fcntl(sc, F_GETFL, 0);
    assert(flags != -1);
    fcntl(sc, F_SETFL, flags & (~O_NONBLOCK));
}

size_t SudoAskpassServer::sFillBuffer(Buffer &buf, unsigned char code, const std::string &str)
{
	buf.code = code;
	size_t len = std::min(str.size(), sizeof(buf.str));
	memcpy(buf.str, str.c_str(), len );
	return len + 1;
}

SudoAskpassServer::SudoAskpassServer(ISudoAskpass *isa)
	: _isa(isa)
{
	if (pipe_cloexec(_kickass) == -1) {
		_kickass[0] = _kickass[1] = -1;
		return;
	}

	char ipc_srv[32];
	snprintf(ipc_srv, sizeof(ipc_srv) - 1, "%lx", (unsigned long)getpid());

	_srv = AskpassIpcFile(ipc_srv, "srv");

	_srv_fd = socket(PF_UNIX, SOCK_DGRAM, 0);
	if (_srv_fd < 0) {
		perror("SudoAskpassServer: socket");
		return;
	}

	struct sockaddr_un sa = {};
	sa.sun_family = AF_UNIX;
	strncpy(sa.sun_path, _srv.c_str(), sizeof(sa.sun_path));
	unlink(sa.sun_path);
	if (bind(_srv_fd, (struct sockaddr *)&sa, sizeof(sa)) < 0) {
		perror("SudoAskpassServer: bind");
		return;
	}

	if (pthread_create(&_trd, NULL, sThread, this) != 0) {
		perror("SudoAskpassServer: pthread_create");
		return;
	}

	_active = true;

	setenv("sdc_askpass_ipc", ipc_srv, 1);
}

SudoAskpassServer::~SudoAskpassServer()
{
	if (_active) {
		unsetenv("sdc_askpass_ipc");

		if (_kickass[1] != -1) {
			if (os_call_ssize(write, _kickass[1], (const void*)&_kickass[1], (size_t)1) != 1) {
				perror("~SudoAskpassServer - write");
			}
		}
		pthread_join(_trd, nullptr);
	}

	CheckedCloseFDPair(_kickass);
	CheckedCloseFD(_srv_fd);

	if (!_srv.empty())
		unlink(_srv.c_str());
}

/////////////////////////////////////////////////////////////////////

void SudoAskpassServer::Thread()
{
	set_nonblock(_srv_fd);
	for (;;) {
		fd_set fds, fde;
		int maxfd = (_kickass[0] > _srv_fd) ? _kickass[0] : _srv_fd;

		FD_ZERO(&fds);
		FD_ZERO(&fde);
		FD_SET(_kickass[0], &fds); 
		FD_SET(_kickass[0], &fde); 
		FD_SET(_srv_fd, &fds); 

		if (os_call_int(select, maxfd + 1, &fds, (fd_set *)nullptr, &fde, (timeval *)nullptr) == -1) {
			break;
		}

		if (_srv_fd != -1 && FD_ISSET(_srv_fd, &fds)) {
			struct sockaddr_un sa = {};
			socklen_t  sa_len = sizeof(sa);
			Buffer buf = {};
			ssize_t rlen = recvfrom(_srv_fd, &buf, sizeof(buf), 0, (sockaddr *)&sa, &sa_len);
			if (rlen > 0) {
				size_t slen = OnRequest(buf, (size_t)rlen);
				if (slen > 0) {
					if (sendto(_srv_fd, &buf, slen, 0, (sockaddr *)&sa, sa_len) != (ssize_t)slen) {
						perror("SudoAskpassServer::Thread: write reply");
					}
				}
			}
		}

		if (FD_ISSET(_kickass[0], &fde)) {
			break;
		}

		if (FD_ISSET(_kickass[0], &fds)) {
			char cmd = 0;
			if (read(_kickass[0], &cmd, 1) != 1) {
				perror("SudoAskpassServer::Thread: read kickass");
			}
			break;
		}

	}

	fprintf(stderr, "SudoAskpassServer::Thread exiting\n");
}

size_t SudoAskpassServer::OnRequest(Buffer &buf, size_t len)
{
	if (len == 0) {
		fprintf(stderr, "SudoAskpassServer::OnRequest: empty request\n");
		return 0;
	}

	switch (buf.code) {
		case 1: {
			std::string password;
			if (_isa->OnSudoAskPassword(Sudo::g_sudo_title, Sudo::g_sudo_prompt, password))
				return sFillBuffer(buf, SAR_OK, password);

			buf.code = SAR_CANCEL;
			return 1;
		}

		case 2: {
			if (_isa->OnSudoConfirm(Sudo::g_sudo_title, Sudo::g_sudo_confirm))
				buf.code = SAR_OK;
			else
				buf.code = SAR_CANCEL;
			return 1;
		}

		default: {
			fprintf(stderr, "SudoAskpassServer::OnRequest: wrong request, code=0x%x len=0x%x\n",
				(unsigned int)buf.code, (unsigned int)len);
			return 0;
		}
	}
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	static SudoAskpassResult sRequestConfirmation();


SudoAskpassResult SudoAskpassServer::sRequestToServer(unsigned char code, std::string &str)
{
	const char *ipc_srv = getenv("sdc_askpass_ipc");
	if (!ipc_srv || !*ipc_srv) {
		fprintf(stderr, "sRequestToServer: no ipc\n");
		return SAR_FAILED;
	}
	fprintf(stderr, "sRequestToServer: ipc_srv='%s'\n", ipc_srv);

	const std::string &srv = AskpassIpcFile(ipc_srv, "srv");
	const std::string &clnt = AskpassIpcFile(getpid(), "clnt");

	FDScope fd(socket(PF_UNIX, SOCK_DGRAM, 0));
	if (!fd.Valid()) {
		perror("sRequestToServer: socket");
		return SAR_FAILED;
	}

	struct sockaddr_un sa = {};
	sa.sun_family = AF_UNIX;
	strncpy(sa.sun_path, clnt.c_str(), sizeof(sa.sun_path));
	unlink(sa.sun_path);

	UnlinkScope us(sa.sun_path);

	if (bind(fd, (struct sockaddr *)&sa, sizeof(sa)) < 0) {
		perror("sRequestToServer: bind");
		return SAR_FAILED;
	}

	memset(&sa, 0, sizeof(sa));
	sa.sun_family = AF_UNIX;
	strncpy(sa.sun_path, srv.c_str(), sizeof(sa.sun_path));
	if (connect(fd, (struct sockaddr *)&sa, sizeof(sa)) == -1) {
		perror("sRequestToServer: connect");
		return SAR_FAILED;
	}

	Buffer buf = {};
	size_t slen = sFillBuffer(buf, code, str);
	if (send(fd, &buf, slen, 0) != (ssize_t)slen) {
		perror("sRequestToServer: send");
		return SAR_FAILED;
	}

	int rlen = recv(fd, &buf, sizeof(buf), 0);
	if (rlen <= 0) {
		perror("sRequestToServer: recv");
		return SAR_FAILED;
	}

	SudoAskpassResult out = (SudoAskpassResult)buf.code;
	if (out == SAR_OK)
		str.assign(buf.str, rlen - 1);

	return out;
}

SudoAskpassResult SudoAskpassRequestPassword(std::string &password)
{
	password.clear();
	return SudoAskpassServer::sRequestToServer(1, password);
}

SudoAskpassResult SudoAskpassRequestConfirmation()
{
	std::string dummy_str;
	return SudoAskpassServer::sRequestToServer(2, dummy_str);
}

