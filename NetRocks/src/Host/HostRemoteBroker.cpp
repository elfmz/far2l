#include <memory>
#include <vector>
#include <unistd.h>
#include <stdlib.h>
#include <signal.h>
#include "IPC.h"
#include "Protocol/Protocol.h"

static const std::string s_empty_string;

std::shared_ptr<IProtocol> CreateProtocol(
	const std::string &protocol, // protocol name e.g. "ftp", "sftp", "scp" etc
	const std::string &host,
	unsigned int port,
	const std::string &username,
	const std::string &password,
	const std::string &options, // StringConfig representing various other options, most of which are protocol-specific
	int fd_ipc_recv // DONT read this fd, use it only errors checking by poll/select (to know that host process exited)
);

class HostRemoteBroker : protected IPCEndpoint
{
	int _keepalive = -1;
	std::string _keepalive_path = ".";
	std::shared_ptr<IProtocol> _protocol;
	struct {
		std::string str1, str2, str3;
		timespec ts1, ts2;
		FileInformation file_info;
		unsigned long long ull1, ull2;
		mode_t mode;
		bool b;
	} _args;

	std::vector<char> _io_buf;

	void InitConnection(int fd_recv)
	{
		std::string protocol, host, username, password, options;
		unsigned int port, login_mode;
		RecvString(protocol);
		if (protocol.empty()) {
			throw AbortError();
		}

		RecvString(host);
		RecvPOD(port);
		RecvPOD(login_mode);
		RecvString(username);
		RecvString(password);
		RecvString(options);

		_protocol = CreateProtocol(protocol, host, port, username, password, options, fd_recv);

		if (!_protocol){
			throw std::runtime_error(std::string("Failed to create protocol: ").append(protocol));
		}
	}

	void OnDirectoryEnum()
	{
		RecvString(_args.str1);
		std::shared_ptr<IDirectoryEnumer> enumer = _protocol->DirectoryEnum(_args.str1);
		_keepalive_path = _args.str1;
		SendCommand(IPC_DIRECTORY_ENUM);
		for (;;) {
			bool cont;
			try {
				cont = enumer->Enum(_args.str1, _args.str2, _args.str3, _args.file_info);

			} catch (ProtocolError &ex) {
				fprintf(stderr, "OnDirectoryEnum: %s\n", ex.what());
				RecvCommand();
				SendCommand(IPC_ERROR);
				SendString(ex.what());
				break;
			}
			if (cont && _args.str1.empty()) {
				fprintf(stderr, "OnDirectoryEnum: skipped empty name\n");
				continue;
			}
			auto cmd = RecvCommand();
			SendCommand(cmd);
			if (cmd != IPC_DIRECTORY_ENUM) {
				break;
			}

			if (!cont) {
				SendString(s_empty_string);
				break;
			}

			SendString(_args.str1);
			SendString(_args.str2);
			SendString(_args.str3);
			SendPOD(_args.file_info);
		}
	}

	void OnFileGet()
	{
		RecvString(_args.str1);
		RecvPOD(_args.ull1);
		std::shared_ptr<IFileReader> reader = _protocol->FileGet(_args.str1, _args.ull1);
		SendCommand(IPC_FILE_GET);

		for (;;) {
			size_t len = 0;
			RecvPOD(len);
			if (len == 0) {
				SendCommand(IPC_STOP);
				break;
			}
			try {
				if (_io_buf.size() < len) {
					_io_buf.resize(len);
				}
				len = reader->Read(&_io_buf[0], len);
			} catch (std::exception &ex) {
				fprintf(stderr, "OnFileGet: %s\n", ex.what());
				SendCommand(IPC_ERROR);
				SendString(ex.what());
				break;
			}
			SendCommand(IPC_FILE_GET);
			SendPOD(len);
			if (len == 0) {
				break;
			}
			Send(&_io_buf[0], len);
		}
	}

	void OnFilePut()
	{
		RecvString(_args.str1);
		RecvPOD(_args.mode);
		RecvPOD(_args.ull1);
		RecvPOD(_args.ull2);
		std::shared_ptr<IFileWriter> writer = _protocol->FilePut(_args.str1, _args.mode, _args.ull1, _args.ull2);
		SendCommand(IPC_FILE_PUT);
		// Trick to improve IO parallelization: instead of sending status reply on operation,
		// send preliminary OK and if error will occur - do error reply on next operation.
		std::string error_str;
		for (;;) {
			size_t len = 0;
			RecvPOD(len);
			if (_io_buf.size() < len) {
				_io_buf.resize(len);
			}

			if (!error_str.empty()) {
				if (len) {
					// still have to fetch buffer to ensure proper IPC sequencing
					Recv(&_io_buf[0], len);
				}
				SendCommand(IPC_ERROR);
				SendString(error_str);
				break;
			}
			if (!len) {
				writer->WriteComplete();
				SendCommand(IPC_STOP);
				break;
			}
			SendCommand(IPC_FILE_PUT);

			Recv(&_io_buf[0], len);
			try {
				writer->Write(&_io_buf[0], len);
			} catch (ProtocolError &ex) {
				fprintf(stderr, "OnFilePut: %s\n", ex.what());
				error_str = ex.what();
				if (error_str.empty())
					error_str = "Unknown error";
			}
		}
	}

	void OnGetModes()
	{
		bool follow_symlink = true;
		size_t count = 0;
		RecvPOD(follow_symlink);
		RecvPOD(count);
		std::vector<std::string> paths(count);
		std::vector<mode_t> modes(count);
		for (auto &path : paths) {
			RecvString(path);
		}
		if (!paths.empty()) {
			_keepalive_path = paths.back();
			_protocol->GetModes(follow_symlink, count, paths.data(), modes.data());
		}
		SendCommand(IPC_GET_MODES);
		for (const auto &mode : modes) {
			SendPOD(mode);
		}
	}

	void OnKeepAlive()
	{
		try {
			if (g_netrocks_verbosity >= 1) {
				fprintf(stderr, "OnKeepAlive for '%s'\n", _keepalive_path.c_str());
			}

			_protocol->KeepAlive(_keepalive_path);

		} catch (std::exception &e) {
			fprintf(stderr, "OnKeepAlive: <%s> for '%s'\n", e.what(), _keepalive_path.c_str());
			_keepalive_path = ".";
		}
	}

	template <IPCCommand C, class MethodT>
		void OnGetModeOrSize(MethodT pGet)
	{
		RecvString(_args.str1);
		RecvPOD(_args.b);
		const auto &out = ((*_protocol).*(pGet))(_args.str1, _args.b);
		SendCommand(C);
		SendPOD(out);
	}

	template <IPCCommand C, class MethodT>
		void OnDelete(MethodT pDelete)
	{
		RecvString(_args.str1);
		((*_protocol).*(pDelete))(_args.str1);
		SendCommand(C);
	}

	void OnGetInformation()
	{
		RecvString(_args.str1);
		RecvPOD(_args.b);
		FileInformation file_info;
		_protocol->GetInformation(file_info, _args.str1, _args.b);
		SendCommand(IPC_GET_INFORMATION);
		SendPOD(file_info);
	}

	void OnRename()
	{
		RecvString(_args.str1);
		RecvString(_args.str2);
		_protocol->Rename(_args.str1, _args.str2);
		SendCommand(IPC_RENAME);
	}

	void OnDirectoryCreate()
	{
		RecvString(_args.str1);
		RecvPOD(_args.mode);
		_protocol->DirectoryCreate(_args.str1, _args.mode);
		SendCommand(IPC_DIRECTORY_CREATE);
	}

	void OnSetTimes()
	{
		RecvString(_args.str1);
		RecvPOD(_args.ts1);
		RecvPOD(_args.ts2);
		_protocol->SetTimes(_args.str1, _args.ts1, _args.ts2);
		SendCommand(IPC_SET_TIMES);
	}

	void OnSetMode()
	{
		RecvString(_args.str1);
		RecvPOD(_args.mode);
		_protocol->SetMode(_args.str1, _args.mode);
		SendCommand(IPC_SET_MODE);
	}

	void OnSymLinkCreate()
	{
		RecvString(_args.str1);
		RecvString(_args.str2);
		_protocol->SymlinkCreate(_args.str1, _args.str2);
		SendCommand(IPC_SYMLINK_CREATE);
	}

	void OnSymLinkQuery()
	{
		RecvString(_args.str1);
		_args.str2.clear();
		_protocol->SymlinkQuery(_args.str1, _args.str2);
		SendCommand(IPC_SYMLINK_QUERY);
		SendString(_args.str2);
	}

	void OnExecuteCommand()
	{
		RecvString(_args.str1);
		RecvString(_args.str2);
		RecvString(_args.str3);
		_protocol->ExecuteCommand(_args.str1, _args.str2, _args.str3);
		SendCommand(IPC_EXECUTE_COMMAND);
	}

	void OnCommand(IPCCommand c)
	{
		switch (c) {
			case IPC_GET_MODES: OnGetModes(); break;
			case IPC_GET_MODE: OnGetModeOrSize<IPC_GET_MODE>(&IProtocol::GetMode); break;
			case IPC_GET_SIZE: OnGetModeOrSize<IPC_GET_SIZE>(&IProtocol::GetSize); break;
			case IPC_GET_INFORMATION: OnGetInformation(); break;
			case IPC_FILE_DELETE: OnDelete<IPC_FILE_DELETE>(&IProtocol::FileDelete); break;
			case IPC_DIRECTORY_DELETE: OnDelete<IPC_DIRECTORY_DELETE>(&IProtocol::DirectoryDelete); break;
			case IPC_RENAME: OnRename(); break;
			case IPC_DIRECTORY_CREATE: OnDirectoryCreate(); break;
			case IPC_SET_TIMES: OnSetTimes(); break;
			case IPC_SET_MODE: OnSetMode(); break;
			case IPC_SYMLINK_CREATE: OnSymLinkCreate(); break;
			case IPC_SYMLINK_QUERY: OnSymLinkQuery(); break;
			case IPC_DIRECTORY_ENUM: OnDirectoryEnum(); break;
			case IPC_FILE_GET: OnFileGet(); break;
			case IPC_FILE_PUT: OnFilePut(); break;
			case IPC_EXECUTE_COMMAND: OnExecuteCommand(); break;

			default:
				throw PipeIPCError("HostRemoteBroker: bad command", (unsigned int)c);
		}
	}

public:
	HostRemoteBroker(int fd_recv, int fd_send, int keepalive) :
		IPCEndpoint(fd_recv, fd_send),
		_keepalive(keepalive)
	{
		SendPOD((uint32_t)IPC_VERSION_MAGIC);
		SendPOD((pid_t)getpid());

		for (;;) try {
			InitConnection(fd_recv);
			SendPOD(IPC_PI_OK);
			break;

		} catch (PipeIPCError &) {
			throw;

		} catch (AbortError &) {
			throw;

		} catch (ServerIdentityMismatchError &ex) {
			SendPOD(IPC_PI_SERVER_IDENTITY_CHANGED);
			SendString(ex.what());

		} catch (ProtocolAuthFailedError &ex) {
			SendPOD(IPC_PI_AUTHORIZATION_FAILED);
			SendString(ex.what());

		} catch (ProtocolError &ex) {
			SendPOD(IPC_PI_PROTOCOL_ERROR);
			SendString(ex.what());

		} catch (std::exception &ex) {
			SendPOD(IPC_PI_GENERIC_ERROR);
			SendString(ex.what());
		}
	}

	void Loop()
	{
		for (;;) {
			if (_keepalive > 0) {
				if (!WaitForRecv(_keepalive * 1000)) {
					OnKeepAlive();
					continue;
				}
			}
			IPCCommand c = RecvCommand();
			try {
				OnCommand(c);

			} catch (ProtocolUnsupportedError &e) {
				c = IPC_UNSUPPORTED;
				SendPOD(c);
				SendString(e.what());

			} catch (ProtocolError &e) {
				c = IPC_ERROR;
				SendPOD(c);
				SendString(e.what());
			}
		}
	}

};

void handle_sigquit(int) {
	_exit(0);  // fast exit, no crash report
}

int main(int argc, char *argv[])
{
	if (argc != 4) {
		fprintf(stderr, "Its a NetRocks protocol broker and must be started by NetRocks only\n");
		return -1;
	}

	setsid();
	//survive terminal death
	signal(SIGHUP, SIG_IGN);
	//signal(SIGTERM, SIG_IGN);
	signal(SIGQUIT, handle_sigquit);

	fprintf(stderr, "%d: HostRemoteBrokerMain: BEGIN\n", getpid());
	try {
		HostRemoteBroker(atoi(argv[1]), atoi(argv[2]), atoi(argv[3])).Loop();

	} catch (std::exception &e) {
		fprintf(stderr, "%d HostRemoteBrokerMain: %s\n", getpid(), e.what());
	}
	fprintf(stderr, "%d: HostRemoteBrokerMain: END\n", getpid());
	return 0;
}
