#define __USE_BSD 
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <stdio.h>
#include <signal.h>
#include <stdlib.h>
#include <errno.h>
#include <termios.h>

static volatile int s_group_signal_propogated = -1;

static void SignalPropogatorHandler(int sig)
{
	if (s_group_signal_propogated != sig) {
		s_group_signal_propogated = sig;
		killpg(0, sig);
	}
	if (sig == SIGTERM) {
		_exit(sig);
	}
}

static void SetupSignalHandlers(bool propogation)
{
	signal(SIGHUP, propogation ? SignalPropogatorHandler : SIG_DFL);
	signal(SIGINT, propogation ? SignalPropogatorHandler : SIG_DFL);
	signal(SIGQUIT, propogation ? SignalPropogatorHandler : SIG_DFL);
	signal(SIGTERM, propogation ? SignalPropogatorHandler : SIG_DFL);
	signal(SIGPIPE, SIG_DFL);
	signal(SIGCHLD, SIG_DFL);
	signal(SIGSTOP, SIG_DFL);
}

////////////

static int VTShell_ExecShell(const char *shell)
{
	int r = (*shell == '/')
		? execl(shell, shell, "-i", NULL)
		: execlp(shell, shell, "-i", NULL);
	fprintf(stderr, "%s: execl('%s') returned %d errno %u\n",
		__FUNCTION__, shell, r, errno);
	return -1;
}


int VTShell_Leader(const char *shell, const char *pty)
{
	pid_t parent_grp = getpgid(getpid());
	if (setsid() == -1) {
		perror("VT: setsid");
	}

	pid_t child_grp = getpgid(getpid());

	fprintf(stderr, "VT: parent_grp=%lx child_grp=%lx\n", (unsigned long)parent_grp, (unsigned long)child_grp);

	SetupSignalHandlers(child_grp != parent_grp);

	if (pty && *pty) {
		int r = open(pty, O_RDWR);
		if (r == -1) {
			perror("VT: open slave");
			return errno;
		}

		dup2(r, STDIN_FILENO);
		dup2(r, STDOUT_FILENO);
		dup2(r, STDERR_FILENO);
		close(r);

		if ( ioctl( STDIN_FILENO, TIOCSCTTY, 0 ) == -1 ) {
			perror( "VT: ioctl(TIOCSCTTY)" );
		}
	}

	return VTShell_ExecShell(shell);
}
