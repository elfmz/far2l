#include <unistd.h>
#include <stdio.h>
#include <stdarg.h>
#include <dirent.h>
#include <fcntl.h>
#include <string.h>
#include <dlfcn.h>
#include <sys/stat.h>
#include <map>
#include <mutex>
#include "sudo_common.h"

namespace Sudo {

template <class LOCAL, class REMOTE> class Client2Server
{
	struct Map : std::map<LOCAL, REMOTE> {} _map;
	std::mutex _mutex;

public:
	typedef Client2Server<LOCAL, REMOTE> Client2ServerBase;

	void Register(LOCAL local, REMOTE remote)
	{
		std::lock_guard<std::mutex> lock(_mutex);
		_map[local] = remote;
	}

	bool Deregister(LOCAL local, REMOTE &remote)
	{
		std::lock_guard<std::mutex> lock(_mutex);
		auto i = _map.find(local);
		if (i == _map.end())
			return false;

		remote = i->second;
		_map.erase(i);
		return true;

	}

	bool Lookup(LOCAL local, REMOTE &remote)
	{
		std::lock_guard<std::mutex> lock(_mutex);
		auto i = _map.find(local);
		if (i == _map.end())
			return false;

		remote = i->second;
		return true;
	}

};


static class Client2ServerFD : protected Client2Server<int, int>
{
	public:

	int Register(int remote)
	{
		int local = open("/dev/null", O_RDWR, 0);
		if (local != -1)
			Client2ServerBase::Register(local, remote);
		return local;
	}

	int Deregister(int local)
	{
		int remote;
		if (!Client2ServerBase::Deregister(local, remote)) 
			return -1;

		close(local);
		return remote;

	}

	int Lookup(int local)
	{
		int remote;
		if (!Client2ServerBase::Lookup(local, remote))
			return -1;

		return remote;
	}

} s_c2s_fd;


static class Client2ServerDIR : protected Client2Server<DIR *, void *>
{
	public:

	DIR *Register(void *remote)
	{
		DIR *local = opendir("/");
		if (local)
			Client2ServerBase::Register(local, remote);
		return local;
	}

	void *Deregister(DIR *local)
	{
		void *remote;
		if (!Client2ServerBase::Deregister(local, remote)) 
			return nullptr;

		closedir(local);
		return remote;

	}

	void *Lookup(DIR *local)
	{
		void *remote;
		if (!Client2ServerBase::Lookup(local, remote))
			return nullptr;

		return remote;
	}

} s_c2s_dir;

////////////////////////////////////////////


static int send_remote_fd_close(int fd)
{
	ClientTransaction ct(SUDO_CMD_CLOSE);
	ct.SendPOD(fd);
	ct.RecvPOD(fd);
	return fd;
}



inline bool IsAccessDeniedErrno()
{
	return (errno==EACCES || errno==EPERM);
}

extern "C" __attribute__ ((visibility("default"))) int sdc_open(const char* pathname, int flags, ...)
{
	int saved_errno = errno;
	mode_t mode = 0;
	
	if (flags & O_CREAT) {
		va_list va;
		va_start(va, flags);
		mode = va_arg(va, mode_t);
		va_end(va);
	}
	
	int r = open(pathname, flags, mode);
	if (r!=-1 || !IsAccessDeniedErrno() || !TouchClientConnection())
		return r;
		
	try {
		ClientTransaction ct(SUDO_CMD_OPEN);
		ct.SendStr(pathname);
		ct.SendPOD(flags);
		ct.SendPOD(mode);

		int remote_fd;
		ct.RecvPOD(remote_fd);

		if (remote_fd!=-1) {
			r = s_c2s_fd.Register(remote_fd);

			if (r==-1) {
				ct.NewTransaction(SUDO_CMD_CLOSE);
				ct.SendPOD(remote_fd);
				ct.RecvPOD(r);
				throw "register";
			}
			errno = saved_errno;
		} else {
			ct.RecvErrno();
		}
		
		return r;
	} catch(const char *what) {
		fprintf(stderr, "sudo_client: open(%s, 0x%x, 0x%x) - error %s\n", pathname, flags, mode, what);
		return -1;
	}
}

extern "C" __attribute__ ((visibility("default"))) int sdc_close(int fd)
{
	int remote_fd = s_c2s_fd.Deregister(fd);
	if (remote_fd==-1) {
		return close(fd);
	}

	try {
		ClientTransaction ct(SUDO_CMD_CLOSE);
		ct.SendPOD(remote_fd);
		return ct.RecvInt();
	} catch(const char *what) {
		fprintf(stderr, "sudo_client: close(0x%x -> 0x%x) - error %s\n", fd, remote_fd, what);
		return 0;
	}
}


extern "C" __attribute__ ((visibility("default"))) off_t sdc_lseek(int fd, off_t offset, int whence)
{
	int remote_fd = s_c2s_fd.Lookup(fd);
	if (remote_fd==-1) {
		return lseek(fd, offset, whence);
	}

	try {
		ClientTransaction ct(SUDO_CMD_LSEEK);
		ct.SendPOD(remote_fd);
		ct.SendPOD(offset);
		ct.SendPOD(whence);

		off_t r;
		ct.RecvPOD(r);
		if (r==-1)
			ct.RecvErrno();
		return r;
	} catch(const char *what) {
		fprintf(stderr, "sudo_client: close(0x%x) - error %s\n", fd, what);
		return -1;
	}
}

extern "C" __attribute__ ((visibility("default"))) ssize_t sdc_write(int fd, const void *buf, size_t count)
{
	int remote_fd = s_c2s_fd.Lookup(fd);
	if (remote_fd==-1)
		return write(fd, buf, count);

	try {
		ClientTransaction ct(SUDO_CMD_WRITE);
		ct.SendPOD(remote_fd);
		ct.SendPOD(count);
		if (count) ct.SendBuf(buf, count);

		ssize_t r;
		ct.RecvPOD(r);
		if (r == -1)
			ct.RecvErrno();

		return r;
	} catch(const char *what) {
		fprintf(stderr, "sudo_client: write(0x%x) - error %s\n", fd, what);
		return -1;
	}
}

extern "C" __attribute__ ((visibility("default"))) ssize_t sdc_read(int fd, void *buf, size_t count)
{
	int remote_fd = s_c2s_fd.Lookup(fd);
	if (remote_fd==-1)
		return read(fd, buf, count);

	try {
		ClientTransaction ct(SUDO_CMD_READ);
		ct.SendPOD(remote_fd);
		ct.SendPOD(count);

		ssize_t r;
		ct.RecvPOD(r);
		if (r ==-1) {
			ct.RecvErrno();
		} else if ( r > 0) {
			if (r > (ssize_t)count)
				throw "too many bytes";

			ct.RecvBuf(buf, r);
		}

		return r;
	} catch(const char *what) {
		fprintf(stderr, "sudo_client: read(0x%x) - error %s\n", fd, what);
		return -1;
	}
}

static int send_stat_cmd(SudoCommand cmd, const char *path, struct stat *buf)
{
	try {
		ClientTransaction ct(cmd);
		ct.SendStr(path);

		int r = ct.RecvInt();
		if (r == 0)
			ct.RecvPOD(*buf);

		return r;
	} catch(const char *what) {
		fprintf(stderr, "sudo_client: send_stat_cmd(%u, '%s') - error %s\n", cmd, path, what);
		return -1;
	}
}

extern "C" __attribute__ ((visibility("default"))) int sdc_stat(const char *path, struct stat *buf)
{
	int saved_errno = errno;
	int r = stat(path, buf);
	if (r==-1 && IsAccessDeniedErrno() && TouchClientConnection()) {
		r = send_stat_cmd(SUDO_CMD_STAT, path, buf);
		if (r==0)
			errno = saved_errno;
	}

	return r;
}

extern "C" __attribute__ ((visibility("default"))) int sdc_lstat(const char *path, struct stat *buf)
{
	int saved_errno = errno;
	int r = lstat(path, buf);
	if (r==-1 && IsAccessDeniedErrno() && TouchClientConnection()) {
		r = send_stat_cmd(SUDO_CMD_LSTAT, path, buf);
		if (r==0)
			errno = saved_errno;
	}
	return r;
}

extern "C" __attribute__ ((visibility("default"))) int sdc_fstat(int fd, struct stat *buf)
{
	int remote_fd = s_c2s_fd.Lookup(fd);
	if (remote_fd==-1)
		return fstat(fd, buf);


	try {
		ClientTransaction ct(SUDO_CMD_FSTAT);
		ct.SendPOD(remote_fd);

		int r = ct.RecvInt();
		if (r == 0)
			ct.RecvPOD(*buf);
		else
			ct.RecvErrno();

		return r;
	} catch(const char *what) {
		fprintf(stderr, "sudo_client: fstat(0x%x) - error %s\n", fd, what);
		return -1;
	}
}

extern "C" __attribute__ ((visibility("default"))) int sdc_ftruncate(int fd, off_t length)
{
	int remote_fd = s_c2s_fd.Lookup(fd);
	if (remote_fd==-1)
		return ftruncate(fd, length);
	
	try {
		ClientTransaction ct(SUDO_CMD_FTRUNCATE);
		ct.SendPOD(remote_fd);
		ct.SendPOD(length);
		int r = ct.RecvInt();
		if (r != 0)
			ct.RecvErrno();
		return r;
	} catch(const char *what) {
		fprintf(stderr, "sdc_ftruncate(0x%x) - error %s\n", fd, what);
		return -1;
	}
}

extern "C" __attribute__ ((visibility("default"))) DIR *sdc_opendir(const char *name)
{
	int saved_errno = errno;
	DIR *dir = opendir(name);
	if (dir==NULL && IsAccessDeniedErrno() && TouchClientConnection()) {
		try {
			ClientTransaction ct(SUDO_CMD_OPENDIR);
			ct.SendStr(name);
			void *remote;
			ct.RecvPOD(remote);
			if (remote) {
				dir = s_c2s_dir.Register(remote);
				if (dir) {
					errno = saved_errno;
				} else {
					ct.NewTransaction(SUDO_CMD_CLOSEDIR);
					ct.SendPOD(remote);
					ct.RecvInt();
				}
			}

		} catch(const char *what) {
			fprintf(stderr, "sudo_client: opendir('%s') - error %s\n", name, what);
			return nullptr;
		}
	}
	return dir;
}

extern "C" __attribute__ ((visibility("default"))) int sdc_closedir(DIR *dir)
{
	void *remote_dir = s_c2s_dir.Deregister(dir);
	if (!remote_dir) {
		return closedir(dir);
	}

	try {
		ClientTransaction ct(SUDO_CMD_CLOSEDIR);
		ct.SendPOD(remote_dir);
		return ct.RecvInt();
	} catch(const char *what) {
		fprintf(stderr, "sudo_client: closedir(%p -> %p) - error %s\n", dir, remote_dir, what);
		return 0;
	}
}


thread_local struct dirent sudo_client_dirent;

extern "C" __attribute__ ((visibility("default"))) struct dirent *sdc_readdir(DIR *dir)
{
	void *remote_dir = s_c2s_dir.Lookup(dir);
	if (!remote_dir)
		return readdir(dir);

	try {
		ClientTransaction ct(SUDO_CMD_READDIR);
		ct.SendPOD(remote_dir);
		int err = ct.RecvInt();
		if (err==0) {
			ct.RecvPOD(sudo_client_dirent);
			return &sudo_client_dirent;
		} else
			errno = err;
	} catch(const char *what) {
		fprintf(stderr, "sudo_client: readdir(%p -> %p) - error %s\n", dir, remote_dir, what);
	}
	return nullptr;
}


} //namespace Sudo
