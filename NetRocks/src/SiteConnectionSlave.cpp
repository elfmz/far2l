#include "IPC.h"
#include "Protocol/ProtocolSFTP.h"

static const std::string s_empty_string;

class SiteConnectionSlave : protected IPCEndpoint
{
	std::shared_ptr<IProtocol> _protocol;
	struct {
		std::string str1, str2, str3;
		FileInformation file_info;
		unsigned long long ull;
		mode_t mode;
		bool b;
	} _args;

	void InitConnection()
	{
		std::string protocol, host, username, password, directory, options;
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
		RecvString(directory);
		RecvString(options);

		if (strcasecmp(protocol.c_str(), "sftp") == 0) {
			_protocol = std::make_shared<ProtocolSFTP>(host, port, options, username, password, directory);
       		} else {
			throw std::runtime_error(std::string("Wrong protocol: ").append(protocol));
		}
	}

	bool OnContinueOrAbort(bool may_continue = true)
	{
		IPCCommand c = RecvCommand();
		SendCommand(may_continue ? c : IPC_ABORT);
		return (may_continue && c == IPC_CONTINUE);
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
				OnContinueOrAbort(false);
				break;
			}
			if (!OnContinueOrAbort())
				break;

			if (!cont) {
				SendString(s_empty_string);
				break;

			} else if (!_args.str1.empty()) {
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
		char buf[0x10000];
		while (OnContinueOrAbort()) {
			size_t piece;
			try {
				piece = reader->Read(buf, sizeof(buf));
			} catch (ProtocolError &ex) {
				fprintf(stderr, "OnFileGet: %s\n", ex.what());
				piece = (size_t)-1;
			}
			SendPOD(piece);
			if (!piece || piece == (size_t)-1)
				break;
			Send(buf, piece);

		}
	}

	void OnFilePut()
	{
		RecvString(_args.str1);
		RecvPOD(_args.mode);
		RecvPOD(_args.ull);
		std::shared_ptr<IFileWriter> writer = _protocol->FilePut(_args.str1, _args.mode, _args.ull);
		SendCommand(IPC_FILE_PUT);
		char buf[0x10000];
		while (OnContinueOrAbort()) {
			size_t piece = 0;
			RecvPOD(piece);
			if (!piece)
				break;
			Recv(buf, piece);
			try {
				writer->Write(buf, sizeof(buf));
			} catch (ProtocolError &ex) {
				fprintf(stderr, "OnFilePut: %s\n", ex.what());
				OnContinueOrAbort(false);
				break;
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
			case IPC_FILE_DELETE: OnDelete<IPC_FILE_DELETE>(&IProtocol::FileDelete); break;
			case IPC_DIRECTORY_DELETE: OnDelete<IPC_DIRECTORY_DELETE>(&IProtocol::DirectoryDelete); break;
			case IPC_RENAME: OnRename(); break;
			case IPC_DIRECTORY_CREATE: OnDirectoryCreate(); break;
			case IPC_DIRECTORY_ENUM: OnDirectoryEnum(); break;
			case IPC_FILE_GET: OnFileGet(); break;	
			case IPC_FILE_PUT: OnFilePut(); break;
				
			default:
				throw IPCError("SiteConnectionSlave: bad command", (unsigned int)c); 
		}
	}

public:
	SiteConnectionSlave(int fd_recv, int fd_send) :
		IPCEndpoint(fd_recv, fd_send)
		
	{
		for (;;) try {
			InitConnection();
			SendPOD((unsigned int)0);
			break;

		} catch (ProtocolAuthFailedError &) {
			SendPOD((unsigned int)1);

		} catch (ProtocolError &ex) {
			SendPOD((unsigned int)2);
			SendString(ex.what());
			break;

		} catch (std::exception &ex) {
			SendPOD((unsigned int)3);
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

extern "C" __attribute__ ((visibility("default"))) void SiteConnectionSlaveMain(int argc, char *argv[])
{
	fprintf(stderr, "SiteConnectionSlaveMain: BEGIN\n");
	try {
		if (argc == 2) {
			SiteConnectionSlave(atoi(argv[0]), atoi(argv[1])).Loop();
		}

	} catch (std::exception &e) {
		fprintf(stderr, "SiteConnectionSlaveMain: %s\n", e.what());
	}
	fprintf(stderr, "SiteConnectionSlaveMain: END\n");
}
