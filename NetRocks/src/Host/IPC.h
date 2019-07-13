#pragma once
#include <string>
#include <atomic>
#include <unistd.h>
#include <string.h>
#include "Erroring.h"
#include "utils.h"

enum IPCCommand
{
	IPC_ERROR = 0,
	IPC_UNSUPPORTED,
	IPC_STOP,
	IPC_RESERVED = 10,
	IPC_GET_MODE,
	IPC_GET_SIZE,
	IPC_GET_INFORMATION,
	IPC_FILE_DELETE,
	IPC_DIRECTORY_DELETE,
	IPC_RENAME,
	IPC_SET_TIMES,
	IPC_SET_MODE,
	IPC_SYMLINK_CREATE,
	IPC_SYMLINK_QUERY,
	IPC_DIRECTORY_CREATE,
	IPC_DIRECTORY_ENUM,
	IPC_FILE_GET,
	IPC_FILE_PUT,
	IPC_EXECUTE_COMMAND
};

enum IPCProtocolInitStatus
{
	IPC_PI_OK = 0,
	IPC_PI_SERVER_IDENTITY_CHANGED,
	IPC_PI_AUTHORIZATION_FAILED,
	IPC_PI_PROTOCOL_ERROR,
	IPC_PI_GENERIC_ERROR
};

class IPCSender
{
	int _fd;

protected:
	void SetFD(int fd);

public:
	IPCSender(int fd = -1);
	~IPCSender();

	void Send(const void *data, size_t len) throw(IPCError);
	void SendString(const char *s) throw(IPCError);
	void SendString(const std::string &s) throw(IPCError);
	template <class POD_T>
		inline void SendPOD(const POD_T &pod) throw(IPCError)
	{
		Send(&pod, sizeof(pod));
	}
	inline void SendCommand(IPCCommand cmd) throw(IPCError)
	{
		SendPOD(cmd);
	}
};

class IPCRecver
{
	int _fd, _kickass[2];
	std::atomic<bool> _aborted;

protected:
	void SetFD(int fd);

public:
	IPCRecver(int fd = -1);
	~IPCRecver();

	void AbortReceiving();

	void Recv(void *data, size_t len) throw(IPCError);
	void RecvString(std::string &s) throw(IPCError);

	template <class POD_T>
		inline void RecvPOD(POD_T &pod) throw(IPCError)
	{
		Recv(&pod, sizeof(pod));
	}

	inline IPCCommand RecvCommand() throw(IPCError)
	{
		IPCCommand out;
		RecvPOD(out);
		return out;
	}
};

class IPCEndpoint : public IPCRecver, public IPCSender
{
	public:
	IPCEndpoint(int fd_recv, int fd_send);
};


#define IPC_VERSION_MAGIC  0xbabe0001
