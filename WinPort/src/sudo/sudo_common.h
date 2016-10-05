#pragma once
#include <string>
#include <mutex>
#include <utils.h>

namespace Sudo
{
	enum SudoCommand
	{
		SUDO_CMD_PING = 1,
		SUDO_CMD_OPEN,
		SUDO_CMD_CLOSE,
		SUDO_CMD_LSEEK,
		SUDO_CMD_WRITE,
		SUDO_CMD_READ,
		SUDO_CMD_STAT,
		SUDO_CMD_LSTAT,
		SUDO_CMD_FSTAT,
		SUDO_CMD_FTRUNCATE,
		SUDO_CMD_OPENDIR,
		SUDO_CMD_CLOSEDIR,
		SUDO_CMD_READDIR,
	};

	class BaseTransaction
	{
		int _pipe_send, _pipe_recv;
		bool _failed;
		
	public:
		BaseTransaction(int pipe_send, int pipe_recv);
		void SendBuf(const void *buf, size_t len);
		void SendStr(const char *sz);
		template <class POD> void SendPOD(const POD &v) { SendBuf(&v, sizeof(v)); }
		inline void SendInt(int v) { SendPOD(v); }
		inline void SendErrno() { SendInt(errno); }

		void RecvBuf(void *buf, size_t len);
		void RecvStr(std::string &str);
		template <class POD>  void RecvPOD(POD &v) { RecvBuf(&v, sizeof(v)); }
		int RecvInt();
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

	bool TouchClientConnection();
	void OnSudoDispatch(SudoCommand cmd, BaseTransaction &bt);

}
