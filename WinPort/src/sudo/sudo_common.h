#pragma once
#include <string>
#include <utils.h>

namespace Sudo
{
	enum SudoCommand
	{
		SUDO_CMD_PING = 1,
		SUDO_CMD_CLOSE,
		SUDO_CMD_OPEN,
		SUDO_CMD_LSEEK,
		SUDO_CMD_WRITE,
		SUDO_CMD_READ,
		SUDO_CMD_STAT,
		SUDO_CMD_LSTAT,
		SUDO_CMD_FSTAT,
		SUDO_CMD_FTRUNCATE,
		SUDO_CMD_CLOSEDIR,
		SUDO_CMD_OPENDIR,
		SUDO_CMD_READDIR,
	};

	class BaseTransaction
	{
		public:
		void SendBuf(const void *buf, size_t len);
		void SendStr(const char *sz);
		template <class POD> void SendPOD(const POD &v) { SendBuf(&v, sizeof(v)); }

		void RecvBuf(void *buf, size_t len);
		void RecvStr(std::string &str);
		template <class POD>  void RecvPOD(POD &v) { RecvBuf(&v, sizeof(v)); }
		void RecvErrno();
	};

	class ClientTransaction : public BaseTransaction
	{
		SudoCommand _cmd;
		
		public:
		ClientTransaction(SudoCommand cmd);
		~ClientTransaction();
	};

	bool TouchClientConnection();
	void OnSudoDispatch(SudoCommand cmd, BaseTransaction &bt);

}
