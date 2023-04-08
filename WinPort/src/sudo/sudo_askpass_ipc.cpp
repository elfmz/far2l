#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <stdexcept>
#include <utils.h>
#include <os_call.hpp>
#include <ScopeHelpers.h>

#include "sudo_askpass_ipc.h"
#include "sudo_private.h"

static std::string AskpassIpcFile(const char *ipc_id, const char *purpose)
{
	char sz[128] = {};
	snprintf(sz, sizeof(sz) - 1, "askpass_ipc/%s.%s", ipc_id, purpose);
	return InMyCache(sz);
}

static std::string AskpassIpcFile(const unsigned long ipc_id, const char *purpose)
{
	char sz[128] = {};
	snprintf(sz, sizeof(sz) - 1, "askpass_ipc/%lx.%s", ipc_id, purpose);
	return InMyCache(sz);
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

	try {
		_srv = AskpassIpcFile(ipc_srv, "srv");
		_sock.reset(new LocalSocketServer(LocalSocket::DATAGRAM, _srv));

		if (pthread_create(&_trd, NULL, sThread, this) != 0) {
			throw std::runtime_error("pthread_create");
		}

		_active = true;
		setenv("sdc_askpass_ipc", ipc_srv, 1);

	} catch (std::exception &e) {
		fprintf(stderr, "SudoAskpassServer: %s\n", e.what());
	}
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
	_sock.reset();

	if (!_srv.empty())
		unlink(_srv.c_str());
}

/////////////////////////////////////////////////////////////////////

void SudoAskpassServer::Thread()
{
	for (;;) try {
		_sock->WaitForClient(_kickass[0]);
		Buffer buf;
		struct sockaddr_un sa;
		ssize_t rlen = _sock->RecvFrom(&buf, sizeof(buf), sa);
		if (rlen > 0) {
			size_t slen = OnRequest(buf, (size_t)rlen);
			if (slen > 0) {
				_sock->SendTo(&buf, slen, sa);
			}
		}
	} catch (LocalSocketCancelled &) {
		fprintf(stderr, "SudoAskpassServer::Thread finished\n");
		break;

	} catch (std::exception &e) {
		fprintf(stderr, "SudoAskpassServer::Thread: %s", e.what());
	}
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
//	fprintf(stderr, "sRequestToServer: ipc_srv='%s'\n", ipc_srv);

	SudoAskpassResult out = SAR_FAILED;
	try {
		const std::string &srv = AskpassIpcFile(ipc_srv, "srv");
		const std::string &clnt = AskpassIpcFile(getpid(), "clnt");

		UnlinkScope us(clnt);
		LocalSocketClient sock(LocalSocket::DATAGRAM, srv, clnt);

		Buffer buf = {};
		size_t slen = sFillBuffer(buf, code, str);
		sock.Send(&buf, slen);
		int rlen = sock.Recv(&buf, sizeof(buf));
		if (rlen > 0) {
			out = (SudoAskpassResult)buf.code;
			if (out == SAR_OK)
				str.assign(buf.str, rlen - 1);
		}
	} catch (std::exception &e) {
		fprintf(stderr, "sRequestToServer: %s\n", e.what());
	}

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

