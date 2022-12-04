#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <dirent.h>
#include <fcntl.h>
#include <string.h>
#include <sys/stat.h>
#if defined(__APPLE__) || defined(__FreeBSD__)
  #include <sys/mount.h>
#elif !defined(__HAIKU__)
  #include <sys/statfs.h>
  #include <sys/ioctl.h>
#  if !defined(__CYGWIN__)
#   include <linux/fs.h>
#  endif
#endif
#include <sys/statvfs.h>
#include <sys/time.h>
#include <sys/types.h>
#ifndef __FreeBSD__
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
		std::mutex _mutex;
		
	public:
		void Put(OBJ obj)
		{
			std::lock_guard<std::mutex> lock(_mutex);
			_set.insert(obj);
		}

		bool Check(OBJ obj)
		{
			std::lock_guard<std::mutex> lock(_mutex);
			return (_set.find(obj) != _set.end());
		}
		
		bool Remove(OBJ obj)
		{
			std::lock_guard<std::mutex> lock(_mutex);
			return (_set.erase(obj) != 0);
		}
	};
	
	static Opened<DIR *> g_dirs;

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

	static void OnSudoDispatch_CloseDir(BaseTransaction &bt)
	{		
		DIR *d;
		bt.RecvPOD(d);
		int r = g_dirs.Remove(d) ? closedir(d) : -1;
		bt.SendPOD(r);
	}
	
	static void OnSudoDispatch_OpenDir(BaseTransaction &bt)
	{
		std::string path;
		bt.RecvStr(path);
		DIR *d = opendir(path.c_str());
		g_dirs.Put(d);
		bt.SendPOD(d);
		if (!d)
			bt.SendErrno();
	}
	
	static void OnSudoDispatch_ReadDir(BaseTransaction &bt)
	{
		DIR *d;
		bt.RecvPOD(d);
		bt.RecvErrno();
		struct dirent *de = g_dirs.Check(d) ? readdir(d) : nullptr;
		if (de) {
			bt.SendInt(0);
			bt.SendPOD(*de);
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

	static void OnSudoDispatch_TwoPathes(int (*pfn)(const char *, const char *), BaseTransaction &bt)
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
#if !defined(__APPLE__) && !defined(__FreeBSD__) && !defined(__CYGWIN__) && !defined(__HAIKU__)
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
		
#if defined(__APPLE__) || defined(__FreeBSD__)
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
	
	void OnSudoDispatch(SudoCommand cmd, BaseTransaction &bt)
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
				OnSudoDispatch_CloseDir(bt);
				break;
				
			case SUDO_CMD_OPENDIR:
				OnSudoDispatch_OpenDir(bt);
				break;
				
			case SUDO_CMD_READDIR:
				OnSudoDispatch_ReadDir(bt);
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
			
			case SUDO_CMD_RENAME:
				OnSudoDispatch_TwoPathes(&rename, bt);
				break;

			case SUDO_CMD_SYMLINK:
				OnSudoDispatch_TwoPathes(&symlink, bt);
				break;
				
			case SUDO_CMD_LINK:
				OnSudoDispatch_TwoPathes(&link, bt);
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
				
			default:
				throw std::runtime_error("OnSudoDispatch - bad command");
		}
	}
	
	static void sudo_dispatcher_with_socket(LocalSocket &sock)
	{
		fprintf(stderr, "sudo_dispatcher\n");
		
		SudoCommand cmd = SUDO_CMD_INVALID;
		try {
			for (;;) {
				BaseTransaction bt(sock);
				bt.RecvPOD(cmd);
				OnSudoDispatch(cmd, bt);
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

		std::string ipc_client = InMyTemp(StrPrintf("sudo/%u", getpid()).c_str());

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
