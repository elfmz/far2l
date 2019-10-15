#include <utils.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <termios.h>
#include <fcntl.h>
#include <signal.h>

#include <ScopeHelpers.h>
#include <TTYRawMode.h>

#include "../Globals.h"
#include "OpExecute.h"

static int g_fd_ctl = -1;

static bool FDCtl_VTSize()
{
	ExecFIFO_CtlMsg m = {};
	m.cmd = ExecFIFO_CtlMsg::CMD_PTY_SIZE;
	m.u.pty_size.cols = 80;
	m.u.pty_size.rows = 25;
	struct winsize w = {};
	if (ioctl(1, TIOCGWINSZ, &w) == 0) {
		m.u.pty_size.cols = w.ws_col;
		m.u.pty_size.rows = w.ws_row;
	}

	return (WriteAll(g_fd_ctl, &m, sizeof(m)) == sizeof(m));
}


static void OpExecute_SIGWINCH(int)
{
	FDCtl_VTSize();
}

static void OpExecute_SignalProxy(int signum)
{
	ExecFIFO_CtlMsg m = {};
	m.cmd = ExecFIFO_CtlMsg::CMD_SIGNAL;
	m.u.signum = signum;

	WriteAll(g_fd_ctl, &m, sizeof(m));
}

static void SetSignalHandler(int sugnum, void (*handler)(int))
{
	struct sigaction sa = {};
	sa.sa_handler = handler;
	sa.sa_flags = SA_RESTART;
	sigaction(sugnum,  &sa, nullptr);
}

SHAREDSYMBOL int OpExecute_Shell(int argc, char *argv[])
{
	if (argc < 1) {
		fprintf(stderr, "Missing argument\n");
		return -1;
	}

	std::string fifo = argv[0];

	try {
		int fd_stdin = 0, fd_stdout = 1, fd_stderr = 2;

		TTYRawMode tty_raw_mode(fd_stdout);

		FDScope fd_err(open((fifo + ".err").c_str(), O_RDONLY));
		FDScope fd_out(open((fifo + ".out").c_str(), O_RDONLY));
		FDScope fd_in(open((fifo + ".in").c_str(), O_WRONLY));
		FDScope fd_ctl(open((fifo + ".ctl").c_str(), O_WRONLY));

		if (!fd_ctl.Valid() || !fd_in.Valid() || !fd_out.Valid() || !fd_err.Valid()) {
			throw std::runtime_error("Can't open FIFO");
		}

		g_fd_ctl = fd_ctl;

		if (!FDCtl_VTSize()) {
			throw std::runtime_error("Initial FDCtl_VTSize failed");
		}

		SetSignalHandler(SIGWINCH, OpExecute_SIGWINCH);
		SetSignalHandler(SIGINT, OpExecute_SignalProxy);
		SetSignalHandler(SIGTSTP, OpExecute_SignalProxy);

		fd_set fdr, fde;
		for (;;) {
			FD_ZERO(&fdr);
			FD_ZERO(&fde);
			FD_SET(fd_out, &fdr);
			FD_SET(fd_err, &fdr);
			FD_SET(fd_stdin, &fdr);
			FD_SET(fd_out, &fde);
			FD_SET(fd_err, &fde);
			FD_SET(fd_stdin, &fde);
			int r = select(std::max(fd_stdin, std::max((int)fd_out, (int)fd_err)) + 1, &fdr, nullptr, &fde, NULL);
			if ( r < 0) {
				if (errno == EAGAIN || errno == EINTR)
					continue;

				throw std::runtime_error("select failed");
			}

			if (FD_ISSET(fd_stdin, &fdr) || FD_ISSET(fd_stdin, &fde)) {
				if (ReadWritePiece(fd_stdin, fd_in) <= 0)
					throw std::runtime_error("stdin failed");
			}

			unsigned int out_fails = 0;
			if (FD_ISSET(fd_out, &fdr) || FD_ISSET(fd_out, &fde)) {
				switch (ReadWritePiece(fd_out, fd_stdout)) {
					case 0: out_fails|= 1; break;
					case -1: throw std::runtime_error("stdout failed");
				}
			}

			if (FD_ISSET(fd_err, &fdr) || FD_ISSET(fd_err, &fde)) {
				switch (ReadWritePiece(fd_err, fd_stderr)) {
					case 0: out_fails|= 2; break;
					case -1: throw std::runtime_error("stderr failed");
				}
			}

			
			if (out_fails) {
				if (out_fails == 3) {
					break;
				}
				usleep(10000);
			}
		}



	} catch (std::exception &ex) {
		fprintf(stderr, "OpExecute_Shell: %s\n", ex.what());
		g_fd_ctl = -1;
		return -1;
	}

//	fprintf(stderr, "OpExecute_Shell: LEAVE\n");
	g_fd_ctl = -1;

	return ExecCommandFIFO::sReadStatus(fifo);
}


OpExecute::OpExecute(std::shared_ptr<IHost> &host, const std::string &dir, const std::string &command)
	:
	_host(host),
	_dir(dir),
	_command(command)
{
}

OpExecute::~OpExecute()
{
}

void OpExecute::Do()
{
	_host->ExecuteCommand(_dir, _command, _fifo.FileName());
	int r = G.info.FSF->ExecuteLibrary(G.plugin_path.c_str(), L"OpExecute_Shell", StrMB2Wide(_fifo.FileName()).c_str(), EF_NOCMDPRINT);

	if (G.GetGlobalConfigBool("EnableDesktopNotifications", true)) {
		std::wstring display_action = G.GetMsgWide((r == 0) ? MNotificationSuccess : MNotificationFailed);
		size_t p = display_action.find(L"()");
		if (p != std::wstring::npos)
			display_action.replace(p, 2, G.GetMsgWide(MCommandNotificationTitle));

		// display only first argument of command to avoid leakage of secret information, like passwords etc
		p = _command.find(' ');
		std::string first_arg = (p != std::string::npos) ? _command.substr(0, p) : _command;

		G.info.FSF->DisplayNotification(display_action.c_str(), StrMB2Wide(first_arg).c_str());
	}
}

