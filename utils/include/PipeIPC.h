#pragma once
#include <string>
#include <atomic>
#include <unistd.h>
#include <string.h>
#include <stdexcept>
#include "utils.h"

struct PipeIPCError : std::runtime_error
{
	PipeIPCError(const char *msg, unsigned int code = (unsigned int)-1);
};

class PipeIPCSender
{
	int _fd{-1};

protected:
	void SetFD(int fd);

public:
	PipeIPCSender(int fd = -1);
	~PipeIPCSender();

	void Send(const void *data, size_t len);
	void SendString(const char *s);
	void SendString(const std::string &s);
	template <class POD_T>
		inline void SendPOD(const POD_T &pod)
	{
		Send(&pod, sizeof(pod));
	}
};

class PipeIPCRecver
{
	int _fd, _kickass[2];
	std::atomic<bool> _aborted{false};

protected:
	void SetFD(int fd);

public:
	PipeIPCRecver(int fd = -1);
	~PipeIPCRecver();

	void AbortReceiving();

	bool WaitForRecv(int msec = -1);

	void Recv(void *data, size_t len);
	void RecvString(std::string &s);

	template <class POD_T>
		inline void RecvPOD(POD_T &pod)
	{
		Recv(&pod, sizeof(pod));
	}
};

template <class COMMAND_T>
	struct PipeIPCEndpoint : PipeIPCRecver, PipeIPCSender
{
	PipeIPCEndpoint(int fd_recv = -1, int fd_send = -1)
		: PipeIPCRecver(fd_recv), PipeIPCSender(fd_send)
	{
	}

	void SetFD(int fd_recv, int fd_send)
	{
		PipeIPCRecver::SetFD(fd_recv);
		PipeIPCSender::SetFD(fd_send);
	}

	inline void SendCommand(COMMAND_T cmd)
	{
		SendPOD(cmd);
	}

	inline COMMAND_T RecvCommand()
	{
		COMMAND_T out;
		RecvPOD(out);
		return out;
	}
};

struct PipeIPCFD
{
	PipeIPCFD(const PipeIPCFD&) = delete;

	// fills all data members with valid pipe FDs
	PipeIPCFD();

	// closes all FDs if wasn't detached before
	~PipeIPCFD();

	// closes remote FDs, sets all FDs to -1
	void Detach();

	int master2broker[2]{-1, -1};
	int broker2master[2]{-1, -1};

	char broker_arg_r[32]{}; // decimal representation of master2broker[0]
	char broker_arg_w[32]{}; // decimal representation of broker2master[1]
};
