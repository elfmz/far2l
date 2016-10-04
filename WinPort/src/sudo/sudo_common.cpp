#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <mutex>
#include <fcntl.h>
#include "sudo_common.h"

namespace Sudo
{
	static std::mutex s_mutex;
	static int s_pipe_send = -1;
	static int s_pipe_recv = -1;


	static void CloseConnection()
	{
		CheckedCloseFD(s_pipe_send);
		CheckedCloseFD(s_pipe_recv);
	}
	
	static void WriteToPeer(const void *buf, size_t len)
	{
		ErrnoSaver _es;
		if (write(s_pipe_send, buf, len) != (ssize_t)len) {
			CloseConnection();
			throw "WriteToPeer";
		}		
	}
	
	static void ReadFromPeer(void *buf, size_t len)
	{
		ErrnoSaver _es;
		while (len) {
			ssize_t r = read(s_pipe_recv, buf, len);
			//fprintf(stderr, "reading %p %d from %u: %d\n", buf, len, s_pipe_recv, r);
			if (r <= 0 || r > (ssize_t)len) {
				CloseConnection();
				throw "ReadFromPeer";
			}

			buf = (char *)buf + r;
			len-= r;
		}
	}
	
	
/////////////////////////
	static int (*g_sudo_launcher)(int pipe_request, int pipe_reply) = nullptr;
	
	static int global_client_region_counter = 0;
	thread_local int thread_client_region_counter = 0;

	extern "C" {
		__attribute__ ((visibility("default"))) void sudo_client(int (*p_sudo_launcher)(int pipe_request, int pipe_reply))
		{
			std::lock_guard<std::mutex> lock(s_mutex);
			g_sudo_launcher = p_sudo_launcher;
		}
		
		
		__attribute__ ((visibility("default"))) void sudo_dispatcher(int pipe_request, int pipe_reply)
		{
			fprintf(stderr, "sudo_dispatcher(%d, %d)\n", pipe_request, pipe_reply);
				
			std::lock_guard<std::mutex> lock(s_mutex);
			CloseConnection();
			s_pipe_recv = pipe_request;
			s_pipe_send = pipe_reply;
			try {
				for (;;) {
					BaseTransaction bt;
					SudoCommand cmd;
					bt.RecvPOD(cmd);
					OnSudoDispatch(cmd, bt);
					bt.SendPOD(cmd);
				}
			} catch (const char *what) {
				fprintf(stderr, "sudo_dispatcher - %s (errno=%u)\n", what, errno);
			}
		}

		__attribute__ ((visibility("default"))) void sudo_client_region_enter()
		{
			std::lock_guard<std::mutex> lock(s_mutex);
			++global_client_region_counter;
			++thread_client_region_counter;
		}
		
		__attribute__ ((visibility("default"))) void sudo_client_region_leave()
		{
			std::lock_guard<std::mutex> lock(s_mutex);
			--thread_client_region_counter;
			--global_client_region_counter;

			if (!global_client_region_counter)
				CloseConnection();
		}
	}

	bool TouchClientConnection()
	{
		if (!thread_client_region_counter)
			return false;

		ErrnoSaver es;
		std::lock_guard<std::mutex> lock(s_mutex);
		if (s_pipe_send==-1 || s_pipe_recv==-1) {
			CloseConnection();
			if (g_sudo_launcher) {
				int req[2] = {-1, -1}, rep[2] = {-1, -1};
				if (pipe(req)==-1)
					return false;
				if (pipe(rep)==-1) {
					CheckedCloseFDPair(req);
					return false;
				}
				fcntl(req[1], F_SETFD, FD_CLOEXEC);
				fcntl(rep[0], F_SETFD, FD_CLOEXEC);
				
				if (g_sudo_launcher(req[0], rep[1])==-1) {
					perror("g_sudo_launcher\n");
					CheckedCloseFDPair(req);
					CheckedCloseFDPair(rep);
					return false;
				}
				CheckedCloseFD(req[0]);
				CheckedCloseFD(rep[1]);
				s_pipe_send = req[1];
				s_pipe_recv = rep[0];
				
				SudoCommand cmd = SUDO_CMD_PING;
				WriteToPeer(&cmd, sizeof(cmd));
				ReadFromPeer(&cmd, sizeof(cmd));
				if (cmd!=SUDO_CMD_PING) {
					CloseConnection();
					return false;
				}
			}
		}
			
		return true;
	}


/////////////////////

	void BaseTransaction::SendBuf(const void *buf, size_t len)
	{
		WriteToPeer(buf, len);
	}
	
	void BaseTransaction::SendStr(const char *sz)
	{
		size_t len = strlen(sz);
		SendBuf(&len, sizeof(len));
		SendBuf(sz, len);
	}

	void BaseTransaction::RecvBuf(void *buf, size_t len)
	{
		ReadFromPeer(buf, len);
	}
	
	void BaseTransaction::RecvStr(std::string &str)
	{
		size_t len;
		RecvBuf(&len, sizeof(len));
		str.resize(len);
		if (len)
			RecvBuf(&str[0], len);
	}
	
	void BaseTransaction::RecvErrno()
	{//can't do RecvPOD(errno) cuz errno is saved across RecvPOD
		int err;
		RecvPOD(err);
		errno = err;
	}
	
/////////////////////

	ClientTransaction::ClientTransaction(SudoCommand cmd) 
		: _cmd(cmd) 
	{
		s_mutex.lock();
		SendPOD(_cmd);
	}
	
	ClientTransaction::~ClientTransaction()
	{
		SudoCommand reply;
		try {
			RecvPOD(reply);
			if (reply!=_cmd)
				CloseConnection();
		} catch (const char *) {
		}
		s_mutex.unlock();
	}


}