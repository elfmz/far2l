#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <mutex>
#include <fcntl.h>
#include <assert.h>
#include "sudo_private.h"

namespace Sudo
{

/////////////////////

	BaseTransaction::BaseTransaction(int pipe_send, int pipe_recv)
		: _pipe_send(pipe_send), _pipe_recv(pipe_recv), _failed(false)
	{
	}

	void BaseTransaction::SendBuf(const void *buf, size_t len)
	{
		ErrnoSaver _es;
		if (write(_pipe_send, buf, len) != (ssize_t)len) {
			_failed = true;
			throw "BaseTransaction::SendBuf";
		}		
	}
	
	void BaseTransaction::SendStr(const char *sz)
	{
		size_t len = strlen(sz);
		SendBuf(&len, sizeof(len));
		SendBuf(sz, len);
	}

	void BaseTransaction::RecvBuf(void *buf, size_t len)
	{
		ErrnoSaver _es;
		while (len) {
			ssize_t r = read(_pipe_recv, buf, len);
			//fprintf(stderr, "reading %p %d from %u: %d\n", buf, len, s_client_pipe_recv, r);
			if (r <= 0 || r > (ssize_t)len) {
				_failed = true;
				throw "BaseTransaction::RecvBuf";
			}

			buf = (char *)buf + r;
			len-= r;
		}
	}
	
	void BaseTransaction::RecvStr(std::string &str)
	{
		size_t len;
		RecvBuf(&len, sizeof(len));
		str.resize(len);
		if (len)
			RecvBuf(&str[0], len);
	}
	
	int BaseTransaction::RecvInt()
	{
		int r;
		RecvPOD(r);
		return r;
	}
	
/////////////////////

}
