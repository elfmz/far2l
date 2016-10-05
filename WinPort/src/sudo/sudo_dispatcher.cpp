#include <unistd.h>
#include <stdio.h>
#include <stdarg.h>
#include <dirent.h>
#include <fcntl.h>
#include <string.h>
#include <sys/stat.h>
#include <set>
#include <vector>
#include <mutex>
#include "sudo_common.h"

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
	
	static Opened<int> g_fds;
	static Opened<DIR *> g_dirs;

	static void OnSudoDispatch_Close(BaseTransaction &bt)
	{
		int fd;
		bt.RecvPOD(fd);
		int r = g_fds.Remove(fd) ? close(fd) : -1;
		bt.SendPOD(r);
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
		bt.SendPOD(r);
		if (r!=-1) 
			g_fds.Put(r);
		else
			bt.SendErrno();
	}
	
	static void OnSudoDispatch_LSeek(BaseTransaction &bt)
	{		
		int fd;
		off_t offset;
		int whence;

		bt.RecvPOD(fd);
		bt.RecvPOD(offset);
		bt.RecvPOD(whence);
		
		off_t r = g_fds.Check(fd) ? lseek(fd, offset, whence) : -1;
		bt.SendPOD(r);
		if (r==-1)
			bt.SendErrno();
	}
	
	static void OnSudoDispatch_Write(BaseTransaction &bt)
	{		
		int fd;
		size_t count;

		bt.RecvPOD(fd);
		bt.RecvPOD(count);
		
		std::vector<char> buf(count + 1);
		if (count)
			bt.RecvBuf(&buf[0], count);
		
		ssize_t r = g_fds.Check(fd) ? write(fd, &buf[0], count) : -1;
		bt.SendPOD(r);
		if (r==-1)
			bt.SendErrno();
	}
	
	static void OnSudoDispatch_Read(BaseTransaction &bt)
	{
		int fd;
		size_t count;

		bt.RecvPOD(fd);
		bt.RecvPOD(count);
		
		std::vector<char> buf(count + 1);
		
		ssize_t r = g_fds.Check(fd) ? read(fd, &buf[0], count) : -1;
		bt.SendPOD(r);
		if (r==-1) {
			bt.SendErrno();
		} else if (r > 0) {
			bt.SendBuf(&buf[0], r);
		}
	}
	
	static void OnSudoDispatch_Stat(bool l, BaseTransaction &bt)
	{		
		std::string path;
		bt.RecvStr(path);
		struct stat s;
		int r = l ? lstat(path.c_str(), &s) : stat(path.c_str(), &s);
		bt.SendPOD(r);
		if (r==0)
			bt.SendPOD(s);
	}

	
	static void OnSudoDispatch_FStat(BaseTransaction &bt)
	{
		int fd;
		bt.RecvPOD(fd);
		
		struct stat s;
		int r = g_fds.Check(fd) ? fstat(fd, &s) : -1;
		bt.SendPOD(r);
		if (r == 0)
			bt.SendPOD(s);
		else
			bt.SendErrno();
	}
	
	static void OnSudoFTruncate(BaseTransaction &bt)
	{
		int fd;
		off_t length;
		bt.RecvPOD(fd);
		bt.RecvPOD(length);
		
		int r = g_fds.Check(fd) ? ftruncate(fd, length) : -1;
		bt.SendPOD(r);
		if (r != 0)
			bt.SendErrno();
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
	}
	
	static void OnSudoDispatch_ReadDir(BaseTransaction &bt)
	{
		DIR *d;
		bt.RecvPOD(d);
		struct dirent *de = g_dirs.Check(d) ? readdir(d) : nullptr;
		if (de) {
			bt.SendInt(0);
			bt.SendPOD(*de);
		} else {
			int err = errno;
			bt.SendInt(err ? err : -1);
		}
	}
	
	void OnSudoDispatch(SudoCommand cmd, BaseTransaction &bt)
	{
		fprintf(stderr, "OnSudoDispatch: %u\n", cmd);
		switch (cmd) {
			case SUDO_CMD_PING:
				break;
		
			case SUDO_CMD_CLOSE:
				OnSudoDispatch_Close(bt);
				break;
				
			case SUDO_CMD_OPEN:
				OnSudoDispatch_Open(bt);
				break;
				
			case SUDO_CMD_LSEEK:
				OnSudoDispatch_LSeek(bt);
				break;

			case SUDO_CMD_WRITE:
				OnSudoDispatch_Write(bt);
				break;
			
			case SUDO_CMD_READ:
				OnSudoDispatch_Read(bt);
				break;
				
			case SUDO_CMD_STAT:
				OnSudoDispatch_Stat(false, bt);
				break;
				
			case SUDO_CMD_LSTAT:
				OnSudoDispatch_Stat(true, bt);
				break;
				
			
			case SUDO_CMD_FSTAT:
				OnSudoDispatch_FStat(bt);
				break;
				
			case SUDO_CMD_FTRUNCATE:
				OnSudoFTruncate(bt);
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
				
			default:
				throw "OnSudoDispatch - bad command";
		}
	}
	
	extern "C" __attribute__ ((visibility("default"))) void sudo_dispatcher(int pipe_request, int pipe_reply)
	{
		fprintf(stderr, "sudo_dispatcher(%d, %d)\n", pipe_request, pipe_reply);
		
		try {
			for (;;) {
				BaseTransaction bt(pipe_reply, pipe_request);
				SudoCommand cmd;
				bt.RecvPOD(cmd);
				OnSudoDispatch(cmd, bt);
				bt.SendPOD(cmd);
			}
		} catch (const char *what) {
			fprintf(stderr, "sudo_dispatcher - %s (errno=%u)\n", what, errno);
		}
	}	
}

