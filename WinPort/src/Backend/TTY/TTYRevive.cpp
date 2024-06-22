#include "TTYRevive.h"
#include <sys/param.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/un.h>
#include <sys/wait.h>
#include <signal.h>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <utils.h>
#include <os_call.hpp>
#include <ScopeHelpers.h>
#include <LocalSocket.h>

#define TTY_FLAG_FAR2L           0x01
#define TTY_FLAG_FEATS           0x02
#define TTY_FLAGS_ALL            (TTY_FLAG_FAR2L | TTY_FLAG_FEATS)

#define TTY_INFO_MAXTEXT         0x1000
#define TTY_INFO_FEAT_XENV       0x00000001

int TTYReviveMe(int std_in, int std_out, bool &far2l_tty, int kickass, const std::string &info)
{
	std::string ipc_path = InMyTempFmt("TTY/srv-%lu.", (unsigned long)getpid());
	std::string info_path = ipc_path;
	ipc_path+= "ipc";
	info_path+= "info";

	UnlinkScope us_ipc_path(ipc_path);
	UnlinkScope us_info_path(info_path);

	try {
		{
			FDScope info_fd(info_path.c_str(), O_RDWR | O_CREAT | O_TRUNC | O_CLOEXEC, 0600);
			if (info_fd.Valid()) {
				if (info.size() >= TTY_INFO_MAXTEXT) {
					WriteAll(info_fd, info.c_str(), TTY_INFO_MAXTEXT);
					const char zero = 0;
					WriteAll(info_fd, &zero, sizeof(zero));
				} else {
					WriteAll(info_fd, info.c_str(), info.size() + 1);
				}
				const uint64_t feats = TTY_INFO_FEAT_XENV;
				WriteAll(info_fd, &feats, sizeof(feats));
			}
		}
		LocalSocketServer sock(LocalSocket::DATAGRAM, ipc_path);
		sock.WaitForClient(kickass);

		unsigned char flags = -1;
		sock.Recv(&flags, 1);
		if (flags & ~TTY_FLAGS_ALL) {
			return -1;
		}
		uint64_t intersected_feats = 0;
		if (flags & TTY_FLAG_FEATS) {
			// we and peer know about extended feats, so peer send us intersection - lets get it
			sock.Recv(&intersected_feats, sizeof(intersected_feats));
		}

		FDScope new_in(sock.RecvFD());
		FDScope new_out(sock.RecvFD());
		int notify_pipe = sock.RecvFD();

		dup2(new_in, std_in);
		dup2(new_out, std_out);

		far2l_tty = ((flags & TTY_FLAG_FAR2L) != 0);

		if (intersected_feats & TTY_INFO_FEAT_XENV) {
			std::vector<char> v;
			char env[0x80]{};
			for (;;) {
				uint32_t l;
				sock.Recv(&l, sizeof(l));
				if (l == 0 || l >= sizeof(env)) {
					break;
				}
				sock.Recv(env, l);
				env[l] = 0;
				sock.Recv(&l, sizeof(l));
				if (l == (uint32_t)-1) {
					unsetenv(env);

				} else if (l < 0x400000) { // 4 megs ought to be enough for everyone
					v.resize(l + 1);
					if (l) {
						sock.Recv(v.data(), l);
					}
					v[l] = 0;
					setenv(env, v.data(), 1);
				} else {
					break;
				}
			}
		}

		return notify_pipe;

	} catch (LocalSocketCancelled &e) {
		(void)e;
		fprintf(stderr, "TTYReviveMe: kickass signalled\n");
		char c;
		if (read(kickass, &c, 1) < 0) {
			perror("read kickass");
		}

	} catch (std::exception &e) {
		fprintf(stderr, "TTYReviveMe: %s\n", e.what());
	}

	return -1;
}

///////////////////////////////////////////////////////////////

class TTYInfoReader
{
	char _buf[TTY_INFO_MAXTEXT + sizeof(uint16_t) + 2] {};
	uint64_t _feats = 0;

public:
	TTYInfoReader(const std::string &info_file)
	{
		FDScope info_fd(info_file.c_str(), O_RDONLY | O_CLOEXEC);
		if (info_fd.Valid()) {
			const size_t rd_len = ReadAll(info_fd, _buf, sizeof(_buf) - 1);
			const size_t text_len = strlen(_buf);
			if (rd_len >= text_len + 1 + sizeof(_feats)) {
				memcpy(&_feats, &_buf[text_len + 1], sizeof(_feats));
			}
		}
	}

	inline const char *Text() const { return _buf; }
	inline uint64_t Feats() const { return _feats; }
};

void TTYRevivableEnum(std::vector<TTYRevivableInstance> &instances)
{
	std::string ipc_path = InMyTemp("TTY/");

	instances.clear();

	const std::string &tty_dir = InMyTemp("TTY");
	DIR *d = opendir(tty_dir.c_str());
	if (!d) {
		return;
	}

	for (;;) {
		dirent *de = readdir(d);
		if (!de) break;
		if (MatchWildcard(de->d_name, "srv-*.info")) {
			unsigned long pid = 0;
			sscanf(de->d_name, "srv-%lu.info", &pid);
			if (pid > 1) {
				std::string info_file = tty_dir;
				info_file+= '/';
				info_file+= de->d_name;
				TTYInfoReader info_reader(info_file);
				instances.emplace_back(TTYRevivableInstance{info_reader.Text(), (pid_t)pid});
			}
		}
	}

	closedir(d);
}

int TTYReviveIt(pid_t pid, int std_in, int std_out, bool far2l_tty)
{
	const std::string &info_file = InMyTempFmt("TTY/srv-%lu.info", (unsigned long)pid);
	const std::string &ipc_path = InMyTempFmt("TTY/srv-%lu.ipc", (unsigned long)pid);
	const std::string &ipc_path_clnt = InMyTempFmt("TTY/clnt-%lu.ipc", (unsigned long)getpid());

	UnlinkScope us_ipc_path_clnt(ipc_path_clnt);

	int notify_pipe[2];
	if (pipe_cloexec(notify_pipe) == -1) {
		perror("TTYRevive: pipe");
		return -1;
	}

	try {
		TTYInfoReader info_reader(info_file);
		LocalSocketClient sock(LocalSocket::DATAGRAM, ipc_path, ipc_path_clnt);

		// if peer and we have common extended feats - let him know we understood that
		const uint64_t intersected_feats = (info_reader.Feats() & TTY_INFO_FEAT_XENV);
		const unsigned char flags =
			(far2l_tty ? TTY_FLAG_FAR2L : 0) |
			( (intersected_feats != 0) ? TTY_FLAG_FEATS : 0);

		sock.Send(&flags, 1);
		if (intersected_feats != 0) {
			sock.Send(&intersected_feats, sizeof(intersected_feats));
		}
		sock.SendFD(std_in);
		sock.SendFD(std_out);
		sock.SendFD(notify_pipe[1]);
		CheckedCloseFD(notify_pipe[1]);
		if (intersected_feats & TTY_INFO_FEAT_XENV) {
			const char *envs[] = { "DISPLAY", "ICEAUTHORITY", "SESSION_MANAGER",
				"XAPPLRESDIR", "XCMSDB", "XENVIRONMENT", "XFILESEARCHPATH", "XKEYSYMDB",
				"XLOCALEDIR", "XMODIFIERS", "XUSERFILESEARCHPATH", "XWTRACE", "XWTRACELC"
			};
			uint32_t l;
			for (const auto &env : envs) {
				l = strlen(env);
				sock.Send(&l, sizeof(l));
				if (l) {
					sock.Send(env, l);
				}
				const char *v = getenv(env);
				l = v ? strlen(v) : (uint32_t)-1;
				sock.Send(&l, sizeof(l));
				if (v) {
					sock.Send(v, l);
				}
			}
			l = 0;
			sock.Send(&l, sizeof(l));
		}

		return notify_pipe[0];

	} catch (LocalSocketConnectError &e) {
		fprintf(stderr, "TTYRevive: %s - discarding %lu\n", e.what(), (unsigned long)pid);
		unlink(ipc_path.c_str());
		unlink(info_file.c_str());

	} catch (std::exception &e) {
		fprintf(stderr, "TTYRevive: %s\n", e.what());
	}

	CheckedCloseFDPair(notify_pipe);

	return -1;
}
