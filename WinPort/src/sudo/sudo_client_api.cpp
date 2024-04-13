#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <dirent.h>
#include <fcntl.h>
#include <string.h>
#include <dlfcn.h>
#include <sys/stat.h>
#include <sys/statvfs.h>
#if defined(__APPLE__) || defined(__FreeBSD__) || defined(__DragonFly__) || defined(__CYGWIN__)
# include <sys/mount.h>
#elif !defined(__HAIKU__)
# include <sys/statfs.h>
#endif
#include <sys/time.h>
#include <sys/types.h>
#if !defined(__FreeBSD__) && !defined(__DragonFly__)
# include <sys/xattr.h>
#endif
#include <map>
#include <mutex>
#include <utimens_compat.h>
#include "sudo_private.h"
#include "sudo.h"

#if !defined(__APPLE__) and !defined(__FreeBSD__) && !defined(__DragonFly__) && !defined(__CYGWIN__) && !defined(__HAIKU__)
# include <sys/ioctl.h>
# include <linux/fs.h>
#endif

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


inline bool IsAccessDeniedErrno()
{
	return (errno==EACCES || errno==EPERM);
}

extern "C" int sudo_client_execute(const char *cmd, bool modify, bool no_wait)
{
	//this call doesnt require outside region demarkation, so ensure it now
	SudoClientRegion scr;
			
	if (!TouchClientConnection(modify))
		return -2;
	
	int r;
	try {
		ClientTransaction ct(SUDO_CMD_EXECUTE);
		ct.SendStr(cmd);
		ct.SendInt(no_wait ? 1 : 0);
		r = ct.RecvInt();
		if (r==-1) {
			ct.RecvErrno();
		}
	} catch(std::exception &e) {
		fprintf(stderr, "sudo_client: sudo_client_execute('%s', %u) - error %s\n", cmd, modify, e.what());
		r = -3;
	}
	return r;
}		

extern "C" __attribute__ ((visibility("default"))) int sudo_client_is_required_for(const char *pathname, bool modify)
{
	ClientReconstructCurDir crcd(pathname);
	struct stat s;
	
	int r = stat(pathname, &s);
	if (r == 0) {
		r = access(pathname, modify ? R_OK|W_OK : R_OK);
		if (r==0)
			return 0;
			
		return IsAccessDeniedErrno() ? 1 : -1;
	}

	if (IsAccessDeniedErrno())
		return 1;
		
	if (errno != ENOENT) {
		//fprintf(stderr, "stat: error %u on path %s\n", errno, pathname);
		return -1;
	}
	
	std::string tmp(pathname);
	size_t p = tmp.rfind('/');
	if (p == std::string::npos)
		tmp = ".";
	else if (p > 0)
		tmp.resize(p - 1);
	else
		tmp = "/";
	
	r = access(tmp.c_str(), modify ? R_OK|W_OK : R_OK);
	if (r==0) {
		//fprintf(stderr, "access: may %s path %s\n", modify ? "modify" : "read", tmp.c_str());
		return 0;
	}
		
	if (IsAccessDeniedErrno())
		return 1;

	//fprintf(stderr, "access: error %u on path %s\n", errno, tmp.c_str());
	return -1;
}

extern "C" __attribute__ ((visibility("default"))) int sdc_open(const char* pathname, int flags, ...)
{
	int saved_errno = errno;
	mode_t mode = 0;
	
	if (flags & O_CREAT) {
		va_list va;
		va_start(va, flags);
		mode = (mode_t)va_arg(va, unsigned int);
		va_end(va);
	}
	
	ClientReconstructCurDir crcd(pathname);
	
	int r = open(pathname, flags, mode);
	if (r!=-1 || !pathname || !IsAccessDeniedErrno() || 
		!TouchClientConnection((flags & (O_CREAT | O_RDWR | O_TRUNC | O_WRONLY))!=0)) {
		return r;
	}
		
	try {
		ClientTransaction ct(SUDO_CMD_OPEN);
		ct.SendStr(pathname);
		ct.SendPOD(flags);
		ct.SendPOD(mode);

		int remote_errno = -1;
		ct.RecvPOD(remote_errno);
		if (remote_errno == 0) {
			r = ct.RecvFD();
			errno = saved_errno;

		} else {
//			r = -1;
			errno = remote_errno;
		}

	} catch(std::exception &e) {
		r = -1;
		fprintf(stderr, "sudo_client: open(%s, 0x%x, 0x%x) - error %s\n", pathname, flags, mode, e.what());
	}
	return r;
}

extern "C" __attribute__ ((visibility("default"))) int sdc_close(int fd)
{
	return close(fd);
}


extern "C" __attribute__ ((visibility("default"))) off_t sdc_lseek(int fd, off_t offset, int whence)
{
	return lseek(fd, offset, whence);
}

extern "C" __attribute__ ((visibility("default"))) ssize_t sdc_write(int fd, const void *buf, size_t count)
{
	return write(fd, buf, count);
}

extern "C" __attribute__ ((visibility("default"))) ssize_t sdc_read(int fd, void *buf, size_t count)
{
	return read(fd, buf, count);
}


extern "C" __attribute__ ((visibility("default"))) ssize_t sdc_pwrite(int fd, const void *buf, size_t count, off_t offset)
{
	return pwrite(fd, buf, count, offset);
}

extern "C" __attribute__ ((visibility("default"))) ssize_t sdc_pread(int fd, void *buf, size_t count, off_t offset)
{
	return pread(fd, buf, count, offset);
}

template <class STAT_STRUCT>
	static int common_stat(SudoCommand cmd, const char *path, STAT_STRUCT *buf)
{
	try {
		ClientTransaction ct(cmd);
		ct.SendStr(path);

		int r = ct.RecvInt();
		if (r == 0)
			ct.RecvPOD(*buf);

		return r;
	} catch(std::exception &e) {
		fprintf(stderr, "sudo_client: common_stat(%u, '%s') - error %s\n", cmd, path, e.what());
		return -1;
	}
}
extern "C" __attribute__ ((visibility("default"))) int sdc_statfs(const char *path, struct statfs *buf)
{
	int saved_errno = errno;
	ClientReconstructCurDir crcd(path);
	int r = statfs(path, buf);
	if (r==-1 && IsAccessDeniedErrno() && TouchClientConnection(false)) {
		r = common_stat(SUDO_CMD_STATFS, path, buf);
		if (r==0)
			errno = saved_errno;
	}

	return r;
}
extern "C" __attribute__ ((visibility("default"))) int sdc_statvfs(const char *path, struct statvfs *buf)
{
	int saved_errno = errno;
	ClientReconstructCurDir crcd(path);
	int r = statvfs(path, buf);
	if (r==-1 && IsAccessDeniedErrno() && TouchClientConnection(false)) {
		r = common_stat(SUDO_CMD_STATVFS, path, buf);
		if (r==0)
			errno = saved_errno;
	}

	return r;
}

extern "C" __attribute__ ((visibility("default"))) int sdc_stat(const char *path, struct stat *buf)
{
	int saved_errno = errno;
	ClientReconstructCurDir crcd(path);
	int r = stat(path, buf);
	if (r==-1 && IsAccessDeniedErrno() && TouchClientConnection(false)) {
		r = common_stat(SUDO_CMD_STAT, path, buf);
		if (r==0)
			errno = saved_errno;
	}

	return r;
}

extern "C" __attribute__ ((visibility("default"))) int sdc_lstat(const char *path, struct stat *buf)
{
	int saved_errno = errno;
	ClientReconstructCurDir crcd(path);
	int r = lstat(path, buf);
	if (r==-1 && IsAccessDeniedErrno() && TouchClientConnection(false)) {
		r = common_stat(SUDO_CMD_LSTAT, path, buf);
		if (r==0)
			errno = saved_errno;
	}
	return r;
}

extern "C" __attribute__ ((visibility("default"))) int sdc_fstat(int fd, struct stat *buf)
{
	return fstat(fd, buf);
}

extern "C" __attribute__ ((visibility("default"))) int sdc_ftruncate(int fd, off_t length)
{
	return ftruncate(fd, length);
}

extern "C" __attribute__ ((visibility("default"))) int sdc_fchmod(int fd, mode_t mode)
{
	int saved_errno = errno;
	int r = fchmod(fd, mode);
	if (r == -1 && IsAccessDeniedErrno() && TouchClientConnection(true)) {
		// this fails even if fd opened for write, but process has no access to its path
		try {
			ClientTransaction ct(SUDO_CMD_FCHMOD);
			ct.SendFD(fd);
			ct.SendPOD(mode);
			r = ct.RecvInt();
			if (r == 0) {
				errno = saved_errno;
			}
		} catch(std::exception &e) {
			fprintf(stderr, "sdc_fchmod(%u, %o) - error %s\n", fd, mode, e.what());
		}
	}
	return r;
}

extern "C" __attribute__ ((visibility("default"))) DIR *sdc_opendir(const char *path)
{
	int saved_errno = errno;
	ClientReconstructCurDir crcd(path);
	DIR *dir = opendir(path);
	if (dir==NULL && IsAccessDeniedErrno() && TouchClientConnection(false)) {
		try {
			ClientTransaction ct(SUDO_CMD_OPENDIR);
			ct.SendStr(path);
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
			} else
				ct.RecvErrno();

		} catch(std::exception &e) {
			fprintf(stderr, "sudo_client: opendir('%s') - error %s\n", path, e.what());
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
	} catch(std::exception &e) {
		fprintf(stderr, "sudo_client: closedir(%p -> %p) - error %s\n", dir, remote_dir, e.what());
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
		ct.SendErrno();
		int err = ct.RecvInt();
		if (err==0) {
			ct.RecvPOD(sudo_client_dirent);
			return &sudo_client_dirent;
		} else
			errno = err;
	} catch(std::exception &e) {
		fprintf(stderr, "sudo_client: readdir(%p -> %p) - error %s\n", dir, remote_dir, e.what());
	}
	return nullptr;
}

static int common_path_and_mode(SudoCommand cmd, int (*pfn)(const char *, mode_t), const char *path, mode_t mode, bool want_modify)
{
	int saved_errno = errno;
	ClientReconstructCurDir crcd(path);
	int r = pfn(path, mode);
	if (r==-1 && IsAccessDeniedErrno() && TouchClientConnection(want_modify)) {
		try {
			ClientTransaction ct(cmd);
			ct.SendStr(path);
			ct.SendPOD(mode);
			r = ct.RecvInt();
			if (r==-1)
				ct.RecvErrno();
			else
				errno = saved_errno;
		} catch(std::exception &e) {
			fprintf(stderr, "sudo_client: common_path_and_mode(%u, '%s', 0%04lo) - error %s\n", cmd, path, (unsigned long)mode, e.what());
			r = -1;
		}
	}
	return r;	
}

static int common_one_path(SudoCommand cmd, int (*pfn)(const char *), const char *path, bool want_modify)
{
	int saved_errno = errno;
	ClientReconstructCurDir crcd(path);
	int r = pfn(path);
	
	if (r==-1 && IsAccessDeniedErrno() && TouchClientConnection(want_modify)) {
		try {
			ClientTransaction ct(cmd);
			ct.SendStr(path);
			r = ct.RecvInt();
			if (r==-1)
				ct.RecvErrno();
			else
				errno = saved_errno;
		} catch(std::exception &e) {
			fprintf(stderr, "sudo_client: common_one_path(%u, '%s') - error %s\n", cmd, path, e.what());
			r = -1;
		}
	}
	return r;
}

extern "C" __attribute__ ((visibility("default"))) int sdc_mkdir(const char *path, mode_t mode)
{
	return common_path_and_mode(SUDO_CMD_MKDIR, &mkdir, path, mode, true);
}


extern "C" __attribute__ ((visibility("default"))) int sdc_chdir(const char *path)
{
	int saved_errno = errno;
	//fprintf(stderr, "sdc_chdir: %s\n", path);
	ClientReconstructCurDir crcd(path);
	int r = chdir(path);
	const bool access_denied = (r==-1 && IsAccessDeniedErrno());	
	ClientCurDirOverrideReset();
	if (IsSudoRegionActive() || (access_denied && TouchClientConnection(false))) {
		int r2;
		std::string cwd;
		try {
			ClientTransaction ct(SUDO_CMD_CHDIR);
			ct.SendStr(path);
			r2 = ct.RecvInt();
			if (r2 == -1)
				ct.RecvErrno();
			else
				ct.RecvStr(cwd);

		} catch(std::exception &e) {
			fprintf(stderr, "sudo_client: sdc_chdir('%s') - error %s\n", path, e.what());
			r2 = -1;
		}

		if (!cwd.empty())
			ClientCurDirOverrideSet(cwd.c_str());
		
		if (r != 0 && r2 == 0) {
			r = 0;
			errno = saved_errno;
		}

	} else if (access_denied && !IsSudoRegionActive()) {
		//Workaround to avoid excessive sudo prompt on TAB panel switch:
		//set override if path likely to be existing but not accessible 
		//cuz we're out of sudo region now
		//Right solution would be putting TAB worker code into sudo region
		//but showing sudo just to switch focus between panels is bad UX
		if (ClientCurDirOverrideSetIfRecent(path)) {
			r = 0;
			errno = saved_errno;
		} else {
			fprintf(stderr, "sdc_chdir: access denied for unknown - '%s'\n", path);
		}

	} else if (r == 0 && *path == '/') {
		//workaround for chdir(symlink-to-somedir) following getcwd returns somedir but not symlink to it
		ClientCurDirOverrideSet(path);
	}
	
	//if (r!=0)
	//	fprintf(stderr, "FAILED sdc_chdir: %s\n", path);
	
	return r;
}

extern "C" __attribute__ ((visibility("default"))) int sdc_rmdir(const char *path)
{
	return common_one_path(SUDO_CMD_RMDIR, &rmdir, path, true);
}

extern "C" __attribute__ ((visibility("default"))) int sdc_remove(const char *path)
{
	return common_one_path(SUDO_CMD_REMOVE, &remove, path, true);	
}

extern "C" __attribute__ ((visibility("default"))) int sdc_unlink(const char *path)
{
	return common_one_path(SUDO_CMD_UNLINK, &unlink, path, true);
}

extern "C" __attribute__ ((visibility("default"))) int sdc_chmod(const char *path, mode_t mode)
{
	return common_path_and_mode(SUDO_CMD_CHMOD, &chmod, path, mode, true);
}

extern "C" __attribute__ ((visibility("default"))) int sdc_chown(const char *path, uid_t owner, gid_t group)
{
	int saved_errno = errno;
	ClientReconstructCurDir crcd(path);
	int r = chown(path, owner, group);
	if (r==-1 && IsAccessDeniedErrno() && TouchClientConnection(true)) {
		try {
			ClientTransaction ct(SUDO_CMD_CHOWN);
			ct.SendStr(path);
			ct.SendPOD(owner);
			ct.SendPOD(group);
			r = ct.RecvInt();
			if (r==-1)
				ct.RecvErrno();
			else
				errno = saved_errno;
		} catch(std::exception &e) {
			fprintf(stderr, "sudo_client: sdc_chown('%s') - error %s\n", path, e.what());
			r = -1;
		}
	}
	return r;	
}

extern "C" __attribute__ ((visibility("default"))) int sdc_utimens(const char *filename, const struct timespec times[2])
{
	int saved_errno = errno;
	ClientReconstructCurDir crcd(filename);
	int r = utimens(filename, times);
	if (r==-1 && IsAccessDeniedErrno() && TouchClientConnection(true)) {
		try {
			ClientTransaction ct(SUDO_CMD_UTIMENS);
			ct.SendStr(filename);
			ct.SendPOD(times[0]);
			ct.SendPOD(times[1]);
			r = ct.RecvInt();
			if (r==-1)
				ct.RecvErrno();
			else
				errno = saved_errno;
		} catch(std::exception &e) {
			fprintf(stderr, "sudo_client: sdc_utimens('%s') - error %s\n", filename, e.what());
			r = -1;
		}
	}
	return r;	
}


extern "C" __attribute__ ((visibility("default"))) int sdc_futimens(int fd, const struct timespec times[2])
{
    int saved_errno = errno;
    int r = futimens(fd, times);
    if (r==-1 && IsAccessDeniedErrno() && TouchClientConnection(true)) {
        try {
            ClientTransaction ct(SUDO_CMD_FUTIMENS);
            ct.SendFD(fd);
            ct.SendPOD(times[0]);
            ct.SendPOD(times[1]);
            r = ct.RecvInt();
            if (r==-1)
                ct.RecvErrno();
            else
                errno = saved_errno;
        } catch(std::exception &e) {
            fprintf(stderr, "sudo_client: sdc_futimens('%d') - error %s\n", fd, e.what());
            r = -1;
        }
    }
    return r;
}

static int common_two_paths(SudoCommand cmd,
	int (*pfn)(const char *, const char *), const char *path1, const char *path2, bool modify)
{
	int saved_errno = errno;
	int r = pfn(path1, path2);
	if (r==-1 && IsAccessDeniedErrno() && TouchClientConnection(modify)) {
		try {
			ClientTransaction ct(cmd);
			ct.SendStr(path1);
			ct.SendStr(path2);
			r = ct.RecvInt();
			if (r==-1)
				ct.RecvErrno();
			else
				errno = saved_errno;
		} catch(std::exception &e) {
			fprintf(stderr, "sudo_client: common_two_paths(%u, '%s', '%s') - error %s\n", cmd, path1, path2, e.what());
			r = -1;
		}
	}
	return r;
}

extern "C" __attribute__ ((visibility("default"))) int sdc_rename(const char *path1, const char *path2)
{
	ClientReconstructCurDir crcd(path1);
	return common_two_paths(SUDO_CMD_RENAME, &rename, path1, path2, true);
}

extern "C" __attribute__ ((visibility("default"))) int sdc_symlink(const char *path1, const char *path2)
{
	ClientReconstructCurDir crcd(path2);
	return common_two_paths(SUDO_CMD_SYMLINK, &symlink, path1, path2, true);
}

extern "C" __attribute__ ((visibility("default"))) int sdc_link(const char *path1, const char *path2)
{
	ClientReconstructCurDir crcd(path2);
	return common_two_paths(SUDO_CMD_LINK, &link, path1, path2, true);
}

extern "C" __attribute__ ((visibility("default"))) char *sdc_realpath(const char *path, char *resolved_path)
{
	int saved_errno = errno;
	ClientReconstructCurDir crcd(path);
	char *r = realpath(path, resolved_path);
	if (!r && IsAccessDeniedErrno() && TouchClientConnection(false)) {
		try {
			ClientTransaction ct(SUDO_CMD_REALPATH);
			ct.SendStr(path);
			int err = ct.RecvInt();
			if (err == 0) {
				std::string str;
				ct.RecvStr(str);
				if (str.size() >= PATH_MAX)
					str.resize(PATH_MAX - 1);
				if (!resolved_path)
					resolved_path = (char *)malloc(PATH_MAX);
				if (resolved_path)
					strcpy(resolved_path, str.c_str());
				r = resolved_path;
				errno = saved_errno;
			} else
				errno = err;
		} catch(std::exception &e) {
			fprintf(stderr, "sudo_client: sdc_realpath('%s') - error %s\n", path, e.what());
			r = nullptr;
		}
	}
	return r;
}

extern "C" __attribute__ ((visibility("default"))) ssize_t sdc_readlink(const char *path, char *buf, size_t bufsiz)
{
	int saved_errno = errno;
	ClientReconstructCurDir crcd(path);
	ssize_t r = readlink(path, buf, bufsiz);
	if (r <= 0 && IsAccessDeniedErrno() && TouchClientConnection(false)) {
		try {
			ClientTransaction ct(SUDO_CMD_READLINK);
			ct.SendStr(path);
			ct.SendPOD(bufsiz);
			ct.RecvPOD(r);
			if (r >= 0 && r <= (ssize_t)bufsiz) {
				ct.RecvBuf(buf, r);
				errno = saved_errno;
			} else
				ct.RecvErrno();
		} catch(std::exception &e) {
			fprintf(stderr, "sudo_client: sdc_readlink('%s') - error %s\n", path, e.what());
			r = -1;
		}
	}
	return r;
}


extern "C" __attribute__ ((visibility("default"))) char *sdc_getcwd(char *buf, size_t size)
{
	if (!ClientCurDirOverrideQuery(buf, size))
		return NULL;

	if (*buf)
		return buf;

	return getcwd(buf, size);
}


extern "C" __attribute__ ((visibility("default"))) ssize_t sdc_flistxattr(int fd, char *namebuf, size_t size)
{
#if defined(__FreeBSD__) || defined(__DragonFly__) || defined(__CYGWIN__)
		return -1;
#elif defined(__APPLE__)
		return flistxattr(fd, namebuf, size, 0);
#else
		return flistxattr(fd, namebuf, size);
#endif
}

extern "C" __attribute__ ((visibility("default"))) ssize_t sdc_fgetxattr(int fd, const char *name,void *value, size_t size)
{
#if defined(__FreeBSD__) || defined(__DragonFly__) || defined(__CYGWIN__)
	return -1;
#elif defined(__APPLE__)
	return fgetxattr(fd, name, value, size, 0, 0);
#else
	return fgetxattr(fd, name, value, size);
#endif
}

extern "C" __attribute__ ((visibility("default"))) int sdc_fsetxattr(int fd, const char *name, const void *value, size_t size, int flags)
{
#if defined(__FreeBSD__) || defined(__DragonFly__) || defined(__CYGWIN__)
	return -1;
#elif defined(__APPLE__)
	return fsetxattr(fd, name, value, size, 0, flags);
#else
	return fsetxattr(fd, name, value, size, flags);
#endif
}


 extern "C" __attribute__ ((visibility("default"))) int sdc_fs_flags_get(const char *path, unsigned long *flags)
 {
	ClientReconstructCurDir crcd(path);

#if defined(__CYGWIN__) || defined(__HAIKU__)
	//TODO
	*flags = 0;
	return 0;

#elif defined(__APPLE__) || defined(__FreeBSD__) || defined(__DragonFly__)
	struct stat s{};
	int r = sdc_stat(path, &s);
	if (r == 0) {
		*flags = s.st_flags;
	}
	return r;

#else
	int r = -1;
	int fd = open(path, O_RDONLY);
	if (fd != -1) {
		r = bugaware_ioctl_pint(fd, FS_IOC_GETFLAGS, flags);
		close(fd);
	}
	if (r == 0 || !IsAccessDeniedErrno() || !TouchClientConnection(false))
		return r;
	 
	try {
		ClientTransaction ct(SUDO_CMD_FSFLAGSGET);
		ct.SendStr(path);

		r = ct.RecvInt();
		if (r == 0)
			ct.RecvPOD(*flags);
		else
			ct.RecvErrno();

	} catch(std::exception &e) {
		fprintf(stderr, "sudo_client: sdc_fs_flags_get('%s') - error %s\n", path, e.what());
		r = -1;
	}
	return r;
#endif
}
 
extern "C" __attribute__ ((visibility("default"))) int sdc_fs_flags_set(const char *path, unsigned long flags)
{
#if defined(__CYGWIN__) || defined(__HAIKU__)
	//TODO
	return 0;

#else
	ClientReconstructCurDir crcd(path);
	int r;
# if defined(__APPLE__) || defined(__FreeBSD__) || defined(__DragonFly__)
	r = chflags(path, flags);
# else
	int fd = r = open(path, O_RDONLY);
	if (fd != -1) {
		r = bugaware_ioctl_pint(fd, FS_IOC_SETFLAGS, &flags);
		close(fd);
	}
# endif
	if (r == 0 || !IsAccessDeniedErrno() || !TouchClientConnection(true)) {
		return r;
	}

	try {
		ClientTransaction ct(SUDO_CMD_FSFLAGSSET);
		ct.SendStr(path);
		ct.SendPOD(flags);

		r = ct.RecvInt();
		if (r != 0)
			ct.RecvErrno();
			
	} catch(std::exception &e) {
		fprintf(stderr, "sudo_client: sdc_fs_flags_set('%s', 0x%lx) - error %s\n", path, flags, e.what());
		r = -1;
	}
	 
	return r;
#endif
}

extern "C" __attribute__ ((visibility("default"))) int sdc_mkfifo(const char *path, mode_t mode)
{
	ClientReconstructCurDir crcd(path);
	int r = mkfifo(path, mode);
	if (r == 0 || !IsAccessDeniedErrno() || !TouchClientConnection(true)) {
		return r;
	}

	try {
		ClientTransaction ct(SUDO_CMD_MKFIFO);
		ct.SendStr(path);
		ct.SendPOD(mode);

		r = ct.RecvInt();
		if (r != 0)
			ct.RecvErrno();

	} catch(std::exception &e) {
		fprintf(stderr, "sudo_client: sdc_mkfifo('%s', 0x%lx) - error %s\n",
			path, (unsigned long)mode, e.what());
		r = -1;
	}

	return r;
}

extern "C" __attribute__ ((visibility("default"))) int sdc_mknod(const char *path, mode_t mode, dev_t dev)
{
	ClientReconstructCurDir crcd(path);
	int r = mknod(path, mode, dev);
	if (r == 0 || !IsAccessDeniedErrno() || !TouchClientConnection(true)) {
		return r;
	}

	try {
		ClientTransaction ct(SUDO_CMD_MKNOD);
		ct.SendStr(path);
		ct.SendPOD(mode);
		ct.SendPOD(dev);

		r = ct.RecvInt();
		if (r != 0)
			ct.RecvErrno();
	} catch(std::exception &e) {
		fprintf(stderr, "sudo_client: sdc_mknod('%s', 0x%lx, 0x%lx) - error %s\n",
			path, (unsigned long)mode, (unsigned long)dev, e.what());
		r = -1;
	}

	return r;
}


} //namespace Sudo
