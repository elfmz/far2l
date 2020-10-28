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
#include <utils.h>
#include <os_call.hpp>
#include <ScopeHelpers.h>
#include <LocalSocket.h>


int TTYReviveMe(int std_in, int std_out, bool &far2l_tty, int kickass, const std::string &info)
{
	char sz[64] = {};
	snprintf(sz, sizeof(sz) - 1, "TTY/srv-%lu.", (unsigned long)getpid());
	std::string ipc_path = InMyTemp(sz);
	std::string info_path = ipc_path;
	ipc_path+= "ipc";
	info_path+= "info";

	UnlinkScope us_ipc_path(ipc_path);
	UnlinkScope us_info_path(info_path);

	try {
		{
			FDScope info_fd(open(info_path.c_str(), O_RDWR | O_CREAT | O_TRUNC, 0600));
			if (info_fd.Valid()) {
				WriteAll(info_fd, info.c_str(), info.size());
			}
		}
		LocalSocketServer sock(LocalSocket::DATAGRAM, ipc_path);
		sock.WaitForClient(kickass);

		char x_far2l_tty = -1;
		sock.Recv(&x_far2l_tty, 1);

		if (x_far2l_tty != 0 && x_far2l_tty != 1)
			return -1;

		FDScope new_in(sock.RecvFD());
		FDScope new_out(sock.RecvFD());
		int notify_pipe = sock.RecvFD();

		dup2(new_in, std_in);
		dup2(new_out, std_out);

		far2l_tty = x_far2l_tty != 0;
		return notify_pipe;

	} catch (LocalSocketCancelled &e) {
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

				FDScope info_fd(open(info_file.c_str(), O_RDONLY));
				char info[0x1000] = {};
				if (info_fd.Valid()) {
					ReadAll(info_fd, info, sizeof(info) - 1);
				}

				instances.emplace_back(TTYRevivableInstance{info, (pid_t)pid});
			}
		}
	}

	closedir(d);
}

int TTYReviveIt(pid_t pid, int std_in, int std_out, bool far2l_tty)
{
	char sz[64] = {};
	snprintf(sz, sizeof(sz) - 1, "TTY/srv-%lu.ipc", (unsigned long)pid);
	const std::string &ipc_path = InMyTemp(sz);

	snprintf(sz, sizeof(sz) - 1, "TTY/clnt-%lu.ipc", (unsigned long)getpid());
	const std::string &ipc_path_clnt = InMyTemp(sz);

	UnlinkScope us_ipc_path_clnt(ipc_path_clnt);

	int notify_pipe[2];
	if (pipe_cloexec(notify_pipe) == -1) {
		perror("TTYRevive: pipe");
		return -1;
	}

	try {
		LocalSocketClient sock(LocalSocket::DATAGRAM, ipc_path, ipc_path_clnt);

		char x_far2l_tty = far2l_tty ? 1 : 0;
		sock.Send(&x_far2l_tty, 1);
		sock.SendFD(std_in);
		sock.SendFD(std_out);
		sock.SendFD(notify_pipe[1]);
		CheckedCloseFD(notify_pipe[1]);
		return notify_pipe[0];

	} catch (LocalSocketConnectError &e) {
		fprintf(stderr, "TTYRevive: %s - discarding %lu\n", e.what(), (unsigned long)pid);
		unlink(ipc_path.c_str());
		snprintf(sz, sizeof(sz) - 1, "TTY/srv-%lu.info", (unsigned long)pid);
		unlink(InMyTemp(sz).c_str());

	} catch (std::exception &e) {
		fprintf(stderr, "TTYRevive: %s\n", e.what());
	}

	CheckedCloseFDPair(notify_pipe);

	return -1;
}
