#include "TTYRevive.h"
#include <sys/param.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/un.h>
#include <sys/wait.h>
#include <signal.h>
#include <dirent.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#include <utils.h>
#include <ScopeHelpers.h>


static bool unixdomain_send_fd(int sock, int fd)
{
	std::vector<char> buf(CMSG_SPACE(sizeof(int)));
	std::fill(buf.begin(), buf.end(), 0x0b);

	struct cmsghdr *cmsghdr = (struct cmsghdr *)&buf[0];
	cmsghdr->cmsg_len = CMSG_LEN(sizeof(int));
	cmsghdr->cmsg_level = SOL_SOCKET;
	cmsghdr->cmsg_type = SCM_RIGHTS;
 
	struct iovec iov[1] {};
	char c = '*';
	iov[0].iov_base = &c;
	iov[0].iov_len = sizeof(c);

	struct msghdr msg {};
	msg.msg_name = NULL;
	msg.msg_namelen = 0;
	msg.msg_iov = iov;
	msg.msg_iovlen = sizeof(iov) / sizeof(iov[0]);
	msg.msg_control = cmsghdr;
	msg.msg_controllen = CMSG_LEN(sizeof(int));
	msg.msg_flags = 0;

	*(int *)CMSG_DATA(&buf[0]) = fd;
 
	if (sendmsg(sock, &msg, 0) == -1)
		return false;
 
	return true;
}

static int unixdomain_recv_fd(int sock)
{
	std::vector<char> buf(CMSG_SPACE(sizeof(int)));
	std::fill(buf.begin(), buf.end(), 0x0d);

	struct cmsghdr *cmsghdr = (struct cmsghdr *)&buf[0];
	cmsghdr->cmsg_len = CMSG_LEN(sizeof(int));
	cmsghdr->cmsg_level = SOL_SOCKET;
	cmsghdr->cmsg_type = SCM_RIGHTS;

	struct iovec iov[1] {};
	char c;
	iov[0].iov_base = &c;
	iov[0].iov_len = sizeof(c);


	struct msghdr msg {};
	msg.msg_name = NULL;
	msg.msg_namelen = 0;
	msg.msg_iov = iov;
	msg.msg_iovlen = sizeof(iov) / sizeof(iov[0]);
	msg.msg_control = cmsghdr;
	msg.msg_controllen = CMSG_LEN(sizeof(int));
	msg.msg_flags = 0;

	if (recvmsg(sock, &msg, 0) == -1)
		return -1;

	return *(int *)CMSG_DATA(&buf[0]);
}


int TTYReviveMe(int std_in, int std_out, bool far2l_tty, int kickass, const std::string &info)
{
	char sz[64] = {};
	snprintf(sz, sizeof(sz) - 1, "TTY/%u-%lu.", far2l_tty ? 1 : 0, (unsigned long)getpid());
	std::string ipc_path = InMyTemp(sz);
	std::string info_path = ipc_path;
	ipc_path+= "ipc";
	info_path+= "info";

	UnlinkScope us_ipc_path(ipc_path);
	UnlinkScope us_info_path(info_path);

	FDScope sock(socket(PF_UNIX, SOCK_DGRAM, 0));
	if (!sock.Valid()) {
		perror("TTYReviveMe: socket");
		return -1;
	}

	{
		FDScope info_fd(open(info_path.c_str(), O_RDWR | O_CREAT | O_TRUNC, 0600));
		if (info_fd.Valid()) {
			WriteAll(info_fd, info.c_str(), info.size());
		}
	}

	struct sockaddr_un sa = {};
	sa.sun_family = AF_UNIX;
	strncpy(sa.sun_path, ipc_path.c_str(), sizeof(sa.sun_path));
	unlink(sa.sun_path);
	if (bind(sock, (struct sockaddr *)&sa, sizeof(sa)) < 0) {
		perror("TTYReviveMe: bind");
		return -1;
	}

	fd_set fdr, fde;
	int maxfd = (kickass > (int)sock) ? kickass : (int)sock;

	for (;;) {
		FD_ZERO(&fdr);
		FD_ZERO(&fde);
		FD_SET(kickass, &fdr); 
		FD_SET(sock, &fdr); 
		FD_SET(kickass, &fde); 
		FD_SET(sock, &fde); 

		if (select(maxfd + 1, &fdr, NULL, &fde, NULL) == -1) {
			perror("TTYReviveMe: select");
			return -1;
		}

		if (FD_ISSET(kickass, &fde) || FD_ISSET(kickass, &fdr)) {
			perror("TTYReviveMe: kickass");
			char c;
			if (read(kickass, &c, 1) < 0) {
				perror("read kickass");
			}
			return -1;
		}

		if (FD_ISSET(sock, &fde)) {
			perror("TTYReviveMe: sock error");
			return -1;
		}

		if (FD_ISSET(sock, &fdr)) {
			break;
		}
	}

	FDScope new_in(unixdomain_recv_fd(sock));
	if (!new_in.Valid())
		return -1;

	FDScope new_out(unixdomain_recv_fd(sock));
	if (!new_out.Valid())
		return -1;

	int notify_pipe = unixdomain_recv_fd(sock);

	dup2(new_in, std_in);
	dup2(new_out, std_out);

	return notify_pipe;
}

///////////////////////////////////////////////////////////////


struct TTYRevivableInstance
{
	std::string info;
	pid_t pid;
	bool far2l_tty;
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
		if (MatchWildcard(de->d_name, "*-*.info")) {
			unsigned int far2l_tty = 0;
			unsigned long pid = 0;
			sscanf(de->d_name, "%u-%lu.info", &far2l_tty, &pid);
			if (far2l_tty <= 1 && pid > 1) {
				std::string info_file = tty_dir;
				info_file+= '/';
				info_file+= de->d_name;

				FDScope info_fd(open(info_file.c_str(), O_RDONLY));
				char info[0x1000] = {};
				if (info_fd.Valid()) {
					ReadAll(info_fd, info, sizeof(info) - 1);
				}

				instances.emplace_back(TTYRevivableInstance{info, (pid_t)pid, far2l_tty != 0});
			}
		}
	}

	closedir(d);
}

int TTYReviveIt(const TTYRevivableInstance &instance, int std_in, int std_out)
{
	char sz[64] = {};
	snprintf(sz, sizeof(sz) - 1, "TTY/%u-%lu.ipc", instance.far2l_tty ? 1 : 0, (unsigned long)instance.pid);
	const std::string &ipc_path = InMyTemp(sz);

	snprintf(sz, sizeof(sz) - 1, "TTY/clnt-%lu.ipc", (unsigned long)getpid());
	const std::string &ipc_path_clnt = InMyTemp(sz);

	UnlinkScope us_ipc_path_clnt(ipc_path_clnt);

	FDScope sock(socket(PF_UNIX, SOCK_DGRAM, 0));
	if (!sock.Valid()) {
		perror("TTYRevive: socket");
		return -1;
	}

	struct sockaddr_un sa = {};
	sa.sun_family = AF_UNIX;
	strncpy(sa.sun_path, ipc_path_clnt.c_str(), sizeof(sa.sun_path));
	unlink(sa.sun_path);

	if (bind(sock, (struct sockaddr *)&sa, sizeof(sa)) < 0) {
		perror("TTYRevive: bind");
		return -1;
	}

	memset(&sa, 0, sizeof(sa));
	sa.sun_family = AF_UNIX;
	strncpy(sa.sun_path, ipc_path.c_str(), sizeof(sa.sun_path));
	if (connect(sock, (struct sockaddr *)&sa, sizeof(sa)) == -1) {
		perror("TTYRevive: connect");
		unlink(ipc_path.c_str());
		snprintf(sz, sizeof(sz) - 1, "TTY/%u-%lu.info", instance.far2l_tty ? 1 : 0, (unsigned long)instance.pid);
		unlink(InMyTemp(sz).c_str());
		return -1;
	}

	int notify_pipe[2];
	if (pipe_cloexec(notify_pipe) == -1) {
		perror("TTYRevive: pipe");
		return -1;
	}

	unixdomain_send_fd(sock, std_in);
	unixdomain_send_fd(sock, std_out);
	unixdomain_send_fd(sock, notify_pipe[1]);
	CheckedCloseFD(notify_pipe[1]);
	return notify_pipe[0];
}


class FScope
{
	FILE *_f;
public:
	FScope(const FScope &) = delete;
	inline FScope &operator = (const FScope &) = delete;

	FScope(FILE *f) : _f(f) {}
	~FScope()
	{
		if (_f)
			fclose(_f);
	}

	operator FILE *() const
	{
		return _f;
	}
};


static pid_t g_revided_pid = 0;

static void revived_proxy(int sig)
{
	if (g_revided_pid) {
		kill(g_revided_pid, sig);
	}
}

bool TTYTryReviveSome(int std_in, int std_out, bool far2l_tty)
{
	for (;;) {
		std::vector<TTYRevivableInstance> instances;
		TTYRevivableEnum(instances);
		if (instances.empty())
			return false;

		FScope f_out(fdopen(dup(std_out), "w"));

		fprintf(f_out, "\nThere're some far2l-s lost in space-time nearby:\n");
		bool have_compats = false;
		for (size_t i = 0; i < instances.size(); ++i) if (far2l_tty || !instances[i].far2l_tty) {
			fprintf(f_out, " %lu: %s\n", i, instances[i].info.c_str());
			have_compats = true;
		} else {
			fprintf(f_out, " <NEED_FARL_TTY>: %s\n", instances[i].info.c_str());
		}

		if (!have_compats)
			return false;

		fprintf(f_out, "Input instance index to revive or empty string to skip revival and spawn new far2l\n");

		char buf[32] = {};
		{
			FScope f_in(fdopen(dup(std_in), "r"));
			if (!fgets(buf, 31, f_in)) {
				return false;
			}
		}
		if (buf[0] == 0 || buf[0] == '\r' || buf[0] == '\n') {
			return false;
		}

		size_t index = atoi(buf);
		if (buf[0] < '0' || buf[0] > '9' || index >= instances.size()) {
			fprintf(f_out, "Wrong input\n");

		} else if (instances[index].far2l_tty && far2l_tty) {
			fprintf(f_out, "Selected instance requires current terminal tu support far2l extensions but it doesnt\n");

		} else {
			FDScope notify_pipe(TTYReviveIt(instances[index], std_in, std_out));
			if (notify_pipe.Valid()) {
				g_revided_pid = instances[index].pid;
				auto prev_sigwinch = signal(SIGWINCH,  revived_proxy);
				for (;;) {
					char c;
					ssize_t r = read(notify_pipe, &c, 1);
					if (r == 0 || (r < 0 && (errno != EAGAIN && errno != EINTR))) {
						break;
					}
				}
				signal(SIGWINCH,  prev_sigwinch);
				g_revided_pid = 0;
				return true;
			}
			fprintf(f_out, "Revival failed\n");
		}
	}
}

