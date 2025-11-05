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

	BaseTransaction::BaseTransaction(LocalSocket &sock)
		: _sock(sock), _failed(false)
	{
	}

	void BaseTransaction::SendBuf(const void *buf, size_t len)
	{
		ErrnoSaver _es;
		try {
			while (len) {
				size_t r = _sock.Send(buf, len);
				len-= r;
				buf = (const char *)buf + r;
			}
		} catch (...) {
			_failed = true;
			throw;
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
		try {
			while (len) {
				size_t r = _sock.Recv(buf, len);
				len-= r;
				buf = (char *)buf + r;
			}

		} catch (...) {
			_failed = true;
			throw;
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

#if !defined(__APPLE__) && !defined(__FreeBSD__)  && !defined(__DragonFly__) && !defined(__HAIKU__)
# include <sys/ioctl.h>

	int bugaware_ioctl_pint(int fd, unsigned long req, unsigned long *v)
	{
		// workaround for stupid FUSE/NTFS-3G bug with wrong FS_IOC_GETFLAGS' arg size,
		// that modifies more than sizeof(int) near to arg ptr, corrupting memory
		unsigned long placeholder[8] = {*v, 0};
		int out = ioctl(fd, req, &placeholder[0], NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL);
		*v = placeholder[0];
		return out;
	}
#endif

}
