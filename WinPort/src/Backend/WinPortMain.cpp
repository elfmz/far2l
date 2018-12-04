#include <sys/ioctl.h>
#include <signal.h>
#include <fcntl.h>
#include <termios.h> 
#include "Backend.h"
#include "ConsoleOutput.h"
#include "ConsoleInput.h"
#include "WinPortHandle.h"
#include "PathHelpers.h"
#include "../sudo/sudo_askpass_ipc.h"
#include "SudoAskpassImpl.h"


ConsoleOutput g_winport_con_out;
ConsoleInput g_winport_con_in;

#ifndef NOWX
bool WinPortMainWX(int argc, char **argv, int(*AppMain)(int argc, char **argv), int *result);
#endif

bool WinPortMainTTY(int std_in, int std_out, bool far2l_tty, int argc, char **argv, int(*AppMain)(int argc, char **argv), int *result);

extern "C" void WinPortInitRegistry();


static std::string GetStdPath(const char *env1, const char *env2)
{
	const char *path = getenv(env1);
	if (!path)
		path = getenv(env2);
	if (path) {
		std::string s;
		if (*path == '~') {
			const char *home = getenv("HOME");
			s = home ? home : "/tmp";
			s+= path + 1;
		} else
			s = path;

		return s;
	}

	return DEVNULL;
}

static void SetupStdHandles(bool ignore_hup)
{
	const std::string &out = GetStdPath("FAR2L_STDOUT", "FAR2L_STD");
	const std::string &err = GetStdPath("FAR2L_STDERR", "FAR2L_STD");
	unsigned char reopened = 0;
	if (!out.empty() && out != "-") {
		if (freopen(out.c_str(), "a", stdout)) {
			reopened|= 1;
		} else
			perror("freopen stdout");
	}
	if (!err.empty() && err != "-") {
		if (freopen(err.c_str(), "a", stderr)) {
			reopened|= 2;
		} else
			perror("freopen stderr");
	}
	
	if ( reopened == 3 && out == DEVNULL && err == DEVNULL) {
		ioctl(0, TIOCNOTTY, NULL);
		if (!freopen(DEVNULL, "r", stdin)) {
			perror("freopen stdin");
		}
		if (ignore_hup)
			signal(SIGHUP, SIG_IGN);
	}
}

static bool NegotiateFar2lTTY(int fdin, int fdout, bool enable)
{
	char buf[64] = {};
	snprintf(buf, sizeof(buf) - 1, "\x1b_far2l%d\x07\x1b[5n\r\n", enable ? 1 : 0);
	if (write(fdout, buf, strlen(buf)) != (ssize_t)strlen(buf))
		return false;

	std::string s;
	for (bool status_replied = false; !status_replied;) {
		fd_set fds, fde;
		FD_ZERO(&fds);
		FD_ZERO(&fde);
		FD_SET(fdin, &fds); 
		FD_SET(fdin, &fde); 
		struct timeval tv = {10, 0};

		if (select(fdin + 1, &fds, NULL, &fde, &tv) <= 0) {
			break;
		}

		char c = 0;
		if (read(fdin, &c, 1) <= 0)
			break;
		s+= c;
		if (s.find("\x1b\x1b") != std::string::npos // ..if terminal doesnt report status and user in panic
		|| s.find_first_of("\r\n") != std::string::npos) {
			break;
		}

		// status reply typically looks like "\x1b[0n"
		// but parse anything that is "\x1b[???n" where ??? is empty or sequence of nubers with ;
		for (size_t p = s.find("\x1b["); p != std::string::npos; p = s.find("\x1b[", p + 1)) {
			size_t e = s.find('n', p + 1);
			if (e != std::string::npos) {
				status_replied = true;
				for (size_t i = p + 2; i < e; ++i) {
					if (!isdigit(s[i]) && s[i] != ';') {
						status_replied = false;
					}
				}
				if (status_replied) break;
			}
		}
	}

	return (s.find("\x1b_far2lok\x07") != std::string::npos);
}

extern "C" int WinPortMain(int argc, char **argv, int(*AppMain)(int argc, char **argv))
{
	bool tty = false, far2l_tty = false, nodetect = false, help = false;

	for (int i = 0; i < argc; ++i) {
		if (strstr(argv[i], "--tty") == argv[i]) {
			tty = true;

		} else if (strstr(argv[i], "--nodetect") == argv[i]) {
			nodetect = true;

		} else if (strcmp(argv[i], "/?") == 0 || strcmp(argv[i], "--help") == 0){
			help = true;
		}
	}

	int std_in = dup(0);
	int std_out = dup(1);
	fcntl(std_in, F_SETFD, FD_CLOEXEC);
	fcntl(std_out, F_SETFD, FD_CLOEXEC);

	struct termios ts = {};
	if (!nodetect) {
		int r = tcgetattr(std_out, &ts);
		if (r == 0) {
			struct termios ts_ne = ts;
			cfmakeraw(&ts_ne);
			if (tcsetattr( std_out, TCSADRAIN, &ts_ne ) == 0) {
				far2l_tty = NegotiateFar2lTTY(std_in, std_out, true);
				if (!far2l_tty)
					tcsetattr( std_out, TCSADRAIN, &ts);
			}
		}
	}
		
	if (far2l_tty) {
		tty = true;
	}

	if (!help) {
		SetupStdHandles(!tty);
	}

	WinPortInitRegistry();
	WinPortInitWellKnownEnv();
//      g_winport_con_out.WriteString(L"Hello", 5);

	SudoAskpassImpl askass_impl;
	SudoAskpassServer askpass_srv(&askass_impl);

	int result = -1;
#ifndef NOWX
	if (!tty) {
		if (!WinPortMainWX(argc, argv, AppMain, &result) ) {
			fprintf(stderr, "Cannot use WX backend\n");
			tty = true;
		}
	}
#else
	tty = true;
#endif

	if (tty) {
		if (!WinPortMainTTY(std_in, std_out, far2l_tty, argc, argv, AppMain, &result)) {
			fprintf(stderr, "Cannot use TTY backend\n");
		}
		if (far2l_tty) {
			NegotiateFar2lTTY(std_in, std_out, false);
			tcsetattr( std_out, TCSADRAIN, &ts);
		}
	}

	WinPortHandle_FinalizeApp();
	close(std_in);
	close(std_out);
	return result;
}
