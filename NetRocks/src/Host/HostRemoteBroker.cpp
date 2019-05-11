#include <vector>
#include <StringConfig.h>
#include "IPC.h"
#include "Protocol/ProtocolSFTP.h"

static const std::string s_empty_string;

class HostRemoteBroker : protected IPCEndpoint
{
	std::shared_ptr<IProtocol> _protocol;
	struct {
		std::string str1, str2, str3;
		FileInformation file_info;
		unsigned long long ull;
		mode_t mode;
		bool b;
	} _args;

	std::vector<char> _io_buf;

	void InitConnection()
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

		StringConfig protocol_options(options);
		if (strcasecmp(protocol.c_str(), "sftp") == 0) {
			_protocol = std::make_shared<ProtocolSFTP>(host, port, username, password, protocol_options);
       		} else {
			throw std::runtime_error(std::string("Wrong protocol: ").append(protocol));
		}
	}

	void OnDirectoryEnum()
	{
		RecvString(_args.str1);
		std::shared_ptr<IDirectoryEnumer> enumer = _protocol->DirectoryEnum(_args.str1);
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
			auto cmd = RecvCommand();
			SendCommand(cmd);
			if (cmd != IPC_DIRECTORY_ENUM) {
				break;
			}

			if (!cont) {
				SendString(s_empty_string);
				break;
			}

			if (!_args.str1.empty()) {
				SendString(_args.str1);
				SendString(_args.str2);
				SendString(_args.str3);
				SendPOD(_args.file_info);

			} else {
				fprintf(stderr, "OnDirectoryEnum: skipped empty name\n");
			}
		}
	}

	void OnFileGet()
	{
		RecvString(_args.str1);
		RecvPOD(_args.ull);
		std::shared_ptr<IFileReader> reader = _protocol->FileGet(_args.str1, _args.ull);
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
		RecvPOD(_args.ull);
		std::shared_ptr<IFileWriter> writer = _protocol->FilePut(_args.str1, _args.mode, _args.ull);
		SendCommand(IPC_FILE_PUT);
		// Trick to improve IO parallelization: instead of sending status reply on operation,
		// send preliminary OK and if error will occur - do error reply on next operation.
		std::string error_str;
		for (;;) {
			size_t len = 0;
			RecvPOD(len);
			if (!error_str.empty()) {
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

			if (_io_buf.size() < len) {
				_io_buf.resize(len);
			}

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

	void OnCommand(IPCCommand c)
	{
		switch (c) {
			case IPC_IS_BROKEN: {
					const bool out = _protocol->IsBroken();
					SendCommand(IPC_IS_BROKEN);
					SendPOD(out);
				} break;

			case IPC_GET_MODE: OnGetModeOrSize<IPC_GET_MODE>(&IProtocol::GetMode); break;
			case IPC_GET_SIZE: OnGetModeOrSize<IPC_GET_SIZE>(&IProtocol::GetSize); break;
			case IPC_GET_INFORMATION: OnGetInformation(); break;
			case IPC_FILE_DELETE: OnDelete<IPC_FILE_DELETE>(&IProtocol::FileDelete); break;
			case IPC_DIRECTORY_DELETE: OnDelete<IPC_DIRECTORY_DELETE>(&IProtocol::DirectoryDelete); break;
			case IPC_RENAME: OnRename(); break;
			case IPC_DIRECTORY_CREATE: OnDirectoryCreate(); break;
			case IPC_DIRECTORY_ENUM: OnDirectoryEnum(); break;
			case IPC_FILE_GET: OnFileGet(); break;	
			case IPC_FILE_PUT: OnFilePut(); break;
				
			default:
				throw IPCError("HostRemoteBroker: bad command", (unsigned int)c); 
		}
	}

public:
	HostRemoteBroker(int fd_recv, int fd_send) :
		IPCEndpoint(fd_recv, fd_send)
		
	{
		for (;;) try {
			InitConnection();
			SendPOD(IPC_PI_OK);
			break;
		} catch (ServerIdentityMismatchError &ex) {
			SendPOD(IPC_PI_SERVER_IDENTITY_CHANGED);
			SendString(ex.what());

		} catch (ProtocolAuthFailedError &ex) {
			SendPOD(IPC_PI_AUTHORIZATION_FAILED);
			SendString(ex.what());
			break;

		} catch (ProtocolError &ex) {
			SendPOD(IPC_PI_PROTOCOL_ERROR);
			SendString(ex.what());
			break;

		} catch (std::exception &ex) {
			SendPOD(IPC_PI_GENERIC_ERROR);
			SendString(ex.what());
			break;
		}
	}

	void Loop()
	{
		for (;;) {
			IPCCommand c = RecvCommand();
			try {
				OnCommand(c);

			} catch (ProtocolError &e) {
				c = IPC_ERROR;
				SendPOD(c);
				SendString(e.what());
			}
		}
	}

};

extern "C" __attribute__ ((visibility("default"))) int HostRemoteBrokerMain(int argc, char *argv[])
{
	fprintf(stderr, "HostRemoteBrokerMain: BEGIN\n");
	try {
		if (argc == 2) {
			HostRemoteBroker(atoi(argv[0]), atoi(argv[1])).Loop();
		}

	} catch (std::exception &e) {
		fprintf(stderr, "HostRemoteBrokerMain: %s\n", e.what());
	}
	fprintf(stderr, "HostRemoteBrokerMain: END\n");
	return 0;
}
