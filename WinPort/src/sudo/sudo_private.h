#pragma once
#include <string>
#include <mutex>
#include <utils.h>
#include <limits.h>
#include <LocalSocket.h>

namespace Sudo
{
	extern std::string g_sudo_title, g_sudo_prompt, g_sudo_confirm;

	enum SudoCommand
	{
		SUDO_CMD_INVALID = 0,
		SUDO_CMD_PING = 1,
		SUDO_CMD_EXECUTE,
		SUDO_CMD_OPEN,
		SUDO_CMD_STATFS,
		SUDO_CMD_STATVFS,
		SUDO_CMD_STAT,
		SUDO_CMD_LSTAT,
		SUDO_CMD_OPENDIR,
		SUDO_CMD_CLOSEDIR,
		SUDO_CMD_READDIR,
		SUDO_CMD_MKDIR,
		SUDO_CMD_CHDIR,
		SUDO_CMD_RMDIR,
		SUDO_CMD_REMOVE,
		SUDO_CMD_UNLINK,
		SUDO_CMD_CHMOD,
		SUDO_CMD_CHOWN,
		SUDO_CMD_UTIMENS,
        SUDO_CMD_FUTIMENS,
		SUDO_CMD_RENAME,
		SUDO_CMD_SYMLINK,
		SUDO_CMD_LINK,
		SUDO_CMD_REALPATH,
		SUDO_CMD_READLINK,
		SUDO_CMD_FSFLAGSGET,
		SUDO_CMD_FSFLAGSSET,
		SUDO_CMD_FCHMOD,
		SUDO_CMD_MKFIFO,
		SUDO_CMD_MKNOD
	};

	class BaseTransaction
	{
		LocalSocket &_sock;
		bool _failed;
		
	public:
		BaseTransaction(LocalSocket &sock);
		void SendBuf(const void *buf, size_t len);
		void SendStr(const char *sz);
		template <class POD> void SendPOD(const POD &v) { SendBuf(&v, sizeof(v)); }
		inline void SendInt(int v) { SendPOD(v); }
		inline void SendFD(int fd) { _sock.SendFD(fd); }
		inline void SendErrno() { SendInt(errno); }

		void RecvBuf(void *buf, size_t len);
		void RecvStr(std::string &str);
		template <class POD> void RecvPOD(POD &v) { RecvBuf(&v, sizeof(v)); }
		int RecvInt();
		inline int RecvFD() { return _sock.RecvFD(); }
		inline void RecvErrno() { errno = RecvInt(); }
		
		inline bool IsFailed() const { return _failed; }
	};

	class ClientTransaction : protected std::lock_guard<std::mutex>, public BaseTransaction
	{
		SudoCommand _cmd;
		
		void Finalize();
		public:
		ClientTransaction(SudoCommand cmd);
		~ClientTransaction();
		
		void NewTransaction(SudoCommand cmd);
	};

	bool TouchClientConnection(bool want_modify);
	bool IsSudoRegionActive();
	
	void ClientCurDirOverrideReset();
	void ClientCurDirOverrideSet(const char *path);
	bool ClientCurDirOverrideSetIfRecent(const char *path);
	bool ClientCurDirOverrideQuery(char *path, size_t size);
	
	class ClientReconstructCurDir
	{
		char *_free_ptr;
		const char * _initial_path;
		const char * &_path;
	public:
		ClientReconstructCurDir(const char * &path);
		ClientReconstructCurDir(const ClientReconstructCurDir&) = delete;
		ClientReconstructCurDir& operator=(const ClientReconstructCurDir&) = delete;
		~ClientReconstructCurDir();
	};

#if !defined(__APPLE__) && !defined(__FreeBSD__) && !defined(__DragonFly__)
	int bugaware_ioctl_pint(int fd, unsigned long req, unsigned long *v);
#endif

}
