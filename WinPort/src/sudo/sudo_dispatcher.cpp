#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <dirent.h>
#include <fcntl.h>
#include <string.h>
#include <sys/stat.h>
#if defined(__APPLE__) || defined(__FreeBSD__) || defined(__DragonFly__)
	#include <sys/mount.h>
#elif !defined(__HAIKU__)
	#include <sys/statfs.h>
	#include <sys/ioctl.h>
#  if !defined(__CYGWIN__)
#    include <linux/fs.h>
#  endif
#endif
#include <sys/statvfs.h>
#include <sys/time.h>
#include <sys/types.h>
#if !defined(__FreeBSD__) && !defined(__DragonFly__)
# include <sys/xattr.h>
#endif
#include <stdexcept>
#include <set>
#include <vector>
#include <mutex>
#include <locale.h>
#include <LocalSocket.h>
#include <utimens_compat.h>
#include "sudo_private.h"

namespace Sudo 
{
	template <class OBJ> class Opened
	{
		std::set<OBJ> _set;
		
	public:
		void Put(OBJ obj)
		{
			_set.insert(obj);
		}

		bool Check(OBJ obj)
		{
			return (_set.find(obj) != _set.end());
		}
		
		bool Remove(OBJ obj)
		{
			return (_set.erase(obj) != 0);
		}
	};

	typedef Opened<DIR *> OpenedDirs;

	static void OnSudoDispatch_Execute(BaseTransaction &bt)
	{
		std::string cmd;
		bt.RecvStr(cmd);
		int no_wait = bt.RecvInt();
		int r;
		if (no_wait) {
			r = fork();
			if (r == 0) {
				r = system(cmd.c_str());
				_exit(r);
				exit(r);
			}
			if (r != -1) {
				PutZombieUnderControl(r);
				r = 0;
			}
		} else {
			r = system(cmd.c_str());
		}
		bt.SendInt(r);
		if (r==-1)
			bt.SendErrno();
	}

	static void OnSudoDispatch_Open(BaseTransaction &bt)
	{
		std::string path;
		int flags;
		mode_t mode;

		bt.RecvStr(path);
		bt.RecvPOD(flags);
		bt.RecvPOD(mode);
		int r = open(path.c_str(), flags, mode);
		if (r != -1) {
			bt.SendInt(0);
			bt.SendFD(r);
			close(r);

		} else {
			r = errno;
			bt.SendInt(r ? r : -1);
		}
	}
	
	template <class STAT_STRUCT>
		static void OnSudoDispatch_StatCommon(int (*pfn)(const char *path, STAT_STRUCT *buf), BaseTransaction &bt)
	{		
		std::string path;
		bt.RecvStr(path);
		STAT_STRUCT s = {};
		int r = pfn(path.c_str(), &s);
		bt.SendPOD(r);
		if (r==0)
			bt.SendPOD(s);
	}

	static void OnSudoDispatch_CloseDir(BaseTransaction &bt, OpenedDirs &dirs)
	{		
		DIR *d;
		bt.RecvPOD(d);
		int r = dirs.Remove(d) ? closedir(d) : -1;
		bt.SendPOD(r);
	}
	
	static void OnSudoDispatch_OpenDir(BaseTransaction &bt, OpenedDirs &dirs)
	{
		std::string path;
		bt.RecvStr(path);
		DIR *d = opendir(path.c_str());
		dirs.Put(d);
		bt.SendPOD(d);
		if (!d)
			bt.SendErrno();
	}
	
	static void OnSudoDispatch_ReadDir(BaseTransaction &bt, OpenedDirs &dirs)
	{
		DIR *d;
		bt.RecvPOD(d);
		bt.RecvErrno();
		struct dirent *de = dirs.Check(d) ? readdir(d) : nullptr;
		if (de) {
			// It appears that actual size of allocated memory for de
			// can be less than sizeof(*de), so direct send sometimes
			// fails with EFAULT, so here is workaround: use full-sized
			// local dirent and copy into it information by two parts:
			// fixed-sized part of dirent then strnlen(path) of path
			struct dirent dex{};
			memcpy(&dex, de, sizeof(dex) - sizeof(dex.d_name));
			strncpy(dex.d_name, de->d_name, sizeof(dex.d_name));
			bt.SendInt(0);
			bt.SendPOD(dex);
		} else {
			int err = errno;
			bt.SendInt(err ? err : -1);
		}
	}
	
	static void OnSudoDispatch_MkDir(BaseTransaction &bt)
	{
		std::string path;
		mode_t mode;
		
		bt.RecvStr(path);
		bt.RecvPOD(mode);
		
		int r = mkdir(path.c_str(), mode);
		bt.SendInt(r);
		if (r==-1)
			bt.SendErrno();
	}
	
	static void OnSudoDispatch_OnePathCommon(int (*pfn)(const char *path), BaseTransaction &bt)
	{
		std::string path;
		bt.RecvStr(path);
		int r = pfn(path.c_str());
		bt.SendInt(r);
		if (r==-1)
			bt.SendErrno();
	}
	
	static void OnSudoDispatch_ChDir(BaseTransaction &bt)
	{
		std::string path;
		bt.RecvStr(path);
		int r = chdir(path.c_str());
		bt.SendInt(r);
		if (r==-1) {
			bt.SendErrno();
		} else {
			char cwd[PATH_MAX + 1];
			if (!getcwd(cwd, sizeof(cwd)-1))
				cwd[0] = 0;
			bt.SendStr(cwd);
		}
	}
	
	static void OnSudoDispatch_ChMod(BaseTransaction &bt)
	{
		std::string path;
		mode_t mode;
		
		bt.RecvStr(path);
		bt.RecvPOD(mode);
		
		int r = chmod(path.c_str(), mode);
		bt.SendInt(r);
		if (r==-1)
			bt.SendErrno();
	}
				
	static void OnSudoDispatch_ChOwn(BaseTransaction &bt)
	{
		std::string path;
		uid_t owner;
		gid_t group;
		
		bt.RecvStr(path);
		bt.RecvPOD(owner);
		bt.RecvPOD(group);
		
		int r = chown(path.c_str(), owner, group);
		bt.SendInt(r);
		if (r==-1)
			bt.SendErrno();
	}
				
	static void OnSudoDispatch_UTimens(BaseTransaction &bt)
	{
		std::string path;
		struct timespec times[2];
		
		bt.RecvStr(path);
		bt.RecvPOD(times[0]);
		bt.RecvPOD(times[1]);
		
		int r = utimens(path.c_str(), times);
		bt.SendInt(r);
		if (r==-1)
			bt.SendErrno();
	}

    static void OnSudoDispatch_FUTimens(BaseTransaction &bt)
    {
        struct timespec times[2];
        int fd = bt.RecvFD();
		int r = -1;
        bt.RecvPOD(times[0]);
        bt.RecvPOD(times[1]);

		if (fd != -1) {
			r = futimens(fd, times);
			close(fd);
		}

        bt.SendInt(r);
        if (r==-1)
            bt.SendErrno();
    }

	static void OnSudoDispatch_TwoPaths(int (*pfn)(const char *, const char *), BaseTransaction &bt)
	{
		std::string path1, path2;
		
		bt.RecvStr(path1);
		bt.RecvStr(path2);
		
		int r = pfn(path1.c_str(), path2.c_str());
		bt.SendInt(r);
		if (r==-1)
			bt.SendErrno();
	}
	
	static void OnSudoDispatch_RealPath(BaseTransaction &bt)
	{
		std::string path;
		bt.RecvStr(path);
		char resolved_path[PATH_MAX + 1] = { 0 };
		if (realpath(path.c_str(), resolved_path)) {
			bt.SendInt(0);
			bt.SendStr(resolved_path);
		} else {
			int err = errno;
			bt.SendInt( err ? err : -1 );
		}
	}
	
	static void OnSudoDispatch_ReadLink(BaseTransaction &bt)
	{
		std::string path;
		size_t bufsiz;
		bt.RecvStr(path);
		bt.RecvPOD(bufsiz);
		std::vector<char> buf(bufsiz + 1);
		ssize_t r = readlink(path.c_str(), &buf[0], bufsiz);
		bt.SendPOD(r);
		if (r >= 0 && r<= (ssize_t)bufsiz) {
			bt.SendBuf(&buf[0], r);
		} else
			bt.SendErrno();
	}
	
	static void OnSudoDispatch_FSFlagsGet(BaseTransaction &bt)
	{
#if !defined(__APPLE__) && !defined(__FreeBSD__)  && !defined(__DragonFly__) && !defined(__CYGWIN__) && !defined(__HAIKU__)
		std::string path;
		bt.RecvStr(path);
		int r = -1;
		int fd = open(path.c_str(), O_RDONLY);
		if (fd != -1) {
			unsigned long flags = 0;
			r = bugaware_ioctl_pint(fd, FS_IOC_GETFLAGS, &flags);
			close(fd);
			if (r == 0) {
				bt.SendInt(0);
				bt.SendPOD(flags);
				return;
			}
		}
		bt.SendInt(-1);
		bt.SendErrno();
#endif
	}
	
	static void OnSudoDispatch_FSFlagsSet(BaseTransaction &bt)
	{
		std::string path;
		unsigned long flags;
		bt.RecvStr(path);
		bt.RecvPOD(flags);
		
#if defined(__APPLE__) || defined(__FreeBSD__) || defined(__DragonFly__)
		if (chflags(path.c_str(), flags) == 0) {
			bt.SendInt(0);
			return;
		}

#elif defined(__HAIKU__)
// ???
#elif !defined(__CYGWIN__)
		int fd = open(path.c_str(), O_RDONLY);
		if (fd != -1) {
			int r = bugaware_ioctl_pint(fd, FS_IOC_SETFLAGS, &flags);
			close(fd);
			if (r == 0) {
				bt.SendInt(0);
				return;
			}
		}
#endif
		bt.SendInt(-1);
		bt.SendErrno();
	}

	static void OnSudoDispatch_FChMod(BaseTransaction &bt)
	{
		int r = -1;
		int fd = bt.RecvFD();
		mode_t mode;
		bt.RecvPOD(mode);
		if (fd != -1) {
			r = fchmod(fd, mode);
			close(fd);
		}
		bt.SendInt(r);
	}

	static void OnSudoDispatch_MkFifo(BaseTransaction &bt)
	{
		std::string path;
		mode_t mode;
		bt.RecvStr(path);
		bt.RecvPOD(mode);
		int r = mkfifo(path.c_str(), mode);
		bt.SendInt(r);
		if (r != 0) {
			bt.SendErrno();
		}
	}

	static void OnSudoDispatch_MkNod(BaseTransaction &bt)
	{
		std::string path;
		mode_t mode;
		dev_t dev;
		bt.RecvStr(path);
		bt.RecvPOD(mode);
		bt.RecvPOD(dev);
		int r = mknod(path.c_str(), mode, dev);
		bt.SendInt(r);
		if (r != 0) {
			bt.SendErrno();
		}
	}
	
	void OnSudoDispatch(SudoCommand cmd, BaseTransaction &bt, OpenedDirs &dirs)
	{
		//fprintf(stderr, "OnSudoDispatch: %u\n", cmd);
		switch (cmd) {
			case SUDO_CMD_PING:
				break;
				
			case SUDO_CMD_EXECUTE:
				OnSudoDispatch_Execute(bt);
				break;
				
			case SUDO_CMD_OPEN:
				OnSudoDispatch_Open(bt);
				break;
#ifndef __HAIKU__
			case SUDO_CMD_STATFS:
				OnSudoDispatch_StatCommon<struct statfs>(&statfs, bt);
				break;
#endif
			case SUDO_CMD_STATVFS:
				OnSudoDispatch_StatCommon<struct statvfs>(&statvfs, bt);
				break;
				
			case SUDO_CMD_STAT:
				OnSudoDispatch_StatCommon<struct stat>(&stat, bt);
				break;
				
			case SUDO_CMD_LSTAT:
				OnSudoDispatch_StatCommon<struct stat>(&lstat, bt);
				break;

			case SUDO_CMD_CLOSEDIR:
				OnSudoDispatch_CloseDir(bt, dirs);
				break;
				
			case SUDO_CMD_OPENDIR:
				OnSudoDispatch_OpenDir(bt, dirs);
				break;
				
			case SUDO_CMD_READDIR:
				OnSudoDispatch_ReadDir(bt, dirs);
				break;
				
			case SUDO_CMD_MKDIR:
				OnSudoDispatch_MkDir(bt);
				break;
				
			case SUDO_CMD_CHDIR:
				OnSudoDispatch_ChDir(bt);
				break;
				
			case SUDO_CMD_RMDIR:
				OnSudoDispatch_OnePathCommon(&rmdir, bt);
				break;
			
			case SUDO_CMD_REMOVE:
				OnSudoDispatch_OnePathCommon(&remove, bt);
				break;
			
			case SUDO_CMD_UNLINK:
				OnSudoDispatch_OnePathCommon(&unlink, bt);
				break;
			
			case SUDO_CMD_CHMOD:
				OnSudoDispatch_ChMod(bt);
				break;
				
			case SUDO_CMD_CHOWN:
				OnSudoDispatch_ChOwn(bt);
				break;
				
			case SUDO_CMD_UTIMENS:
				OnSudoDispatch_UTimens(bt);
				break;

            case SUDO_CMD_FUTIMENS:
                OnSudoDispatch_FUTimens(bt);
                break;

            case SUDO_CMD_RENAME:
				OnSudoDispatch_TwoPaths(&rename, bt);
				break;

			case SUDO_CMD_SYMLINK:
				OnSudoDispatch_TwoPaths(&symlink, bt);
				break;
				
			case SUDO_CMD_LINK:
				OnSudoDispatch_TwoPaths(&link, bt);
				break;

			case SUDO_CMD_REALPATH:
				OnSudoDispatch_RealPath(bt);
				break;

			case SUDO_CMD_READLINK:
				OnSudoDispatch_ReadLink(bt);
				break;
				
			case SUDO_CMD_FSFLAGSGET:
				OnSudoDispatch_FSFlagsGet(bt);
				break;
				
			case SUDO_CMD_FSFLAGSSET:
				OnSudoDispatch_FSFlagsSet(bt);
				break;

			case SUDO_CMD_FCHMOD:
				OnSudoDispatch_FChMod(bt);
				break;

			case SUDO_CMD_MKFIFO:
				OnSudoDispatch_MkFifo(bt);
				break;

			case SUDO_CMD_MKNOD:
				OnSudoDispatch_MkNod(bt);
				break;
				
			default:
				throw std::runtime_error("OnSudoDispatch - bad command");
		}
	}
	
	static void sudo_dispatcher_with_socket(LocalSocket &sock)
	{
		fprintf(stderr, "sudo_dispatcher\n");
		
		SudoCommand cmd = SUDO_CMD_INVALID;
		try {
			OpenedDirs dirs;
			for (;;) {
				BaseTransaction bt(sock);
				bt.RecvPOD(cmd);
				OnSudoDispatch(cmd, bt, dirs);
				bt.SendPOD(cmd);
			}
		} catch (std::exception &e) {
			fprintf(stderr, "sudo_dispatcher - %s (cmd=%u errno=%u)\n", e.what(), cmd, errno);

		}
	}
	
	
	extern "C" __attribute__ ((visibility("default"))) int sudo_main_dispatcher(int argc, char *argv[])
	{
		setlocale(LC_ALL, "");//otherwise non-latin keys missing with XIM input method
		if (argc != 1) {
			fprintf(stderr, "sudo_main_dispatcher: bad args\n");
			return -1;
		}

		const auto &ipc_client = InMyTempFmt("sudo/%u", (unsigned int)getpid());

		try {
			LocalSocketClient udc(LocalSocket::STREAM, argv[0], ipc_client);
			fprintf(stderr, "sudo_main_dispatcher: CONNECTED on '%s'\n", argv[0]);
			sudo_dispatcher_with_socket(udc);

		} catch (std::exception &e) {
			fprintf(stderr, "sudo_main_dispatcher: %s on '%s'\n", e.what(), argv[0]);
		}

		unlink(ipc_client.c_str());
		unlink(argv[0]);

		return 0;	
	}
}
