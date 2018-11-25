#include <sys/ioctl.h>
#include <signal.h>
#include <fcntl.h>
#include "Backend.h"
#include "ConsoleOutput.h"
#include "ConsoleInput.h"
#include "WinPortHandle.h"
#include "PathHelpers.h"
#include "../sudo/sudo_askpass_ipc.h"
#include "SudoAskpassImpl.h"
#include "TTY/TTYFar2lExts.h"

ConsoleOutput g_winport_con_out;
ConsoleInput g_winport_con_in;

bool WinPortMainWX(int argc, char **argv, int(*AppMain)(int argc, char **argv), int *result);
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

static void SetupStdHandles(bool nohup)
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
		signal(SIGHUP, SIG_IGN);
	}
}

extern "C" int WinPortMain(int argc, char **argv, int(*AppMain)(int argc, char **argv))
{
	bool tty = false, notty = false, far2l_tty = false, help = false;
	for (int i = 0; i < argc; ++i) {
		if (strcmp(argv[i], "--tty") == 0) {
			tty = true;

		} else if (strcmp(argv[i], "--notty") == 0) {
			notty = true;

		} else if (strcmp(argv[i], "/?") == 0 || strcmp(argv[i], "--help") == 0){
			help = true;
		}
	}

	int std_in = dup(0);
	int std_out = dup(1);
	fcntl(std_in, F_SETFD, FD_CLOEXEC);
	fcntl(std_out, F_SETFD, FD_CLOEXEC);

	if (!notty) {
		far2l_tty = TTYFar2lExt_Negotiate(std_in, std_out, true);
		if (far2l_tty) {
			tty = true;
		}
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
	if (!tty) {
		if (!WinPortMainWX(argc, argv, AppMain, &result) ) {
			fprintf(stderr, "Cannot use WX backend\n");
			if (!notty)
				tty = true;
		}
	}
	if (tty) {
		if (!WinPortMainTTY(std_in, std_out, far2l_tty, argc, argv, AppMain, &result)) {
			fprintf(stderr, "Cannot use TTY backend\n");
		}
		if (far2l_tty)
			TTYFar2lExt_Negotiate(std_in, std_out, false);
	}

	WinPortHandle_FinalizeApp();
	close(std_in);
	close(std_out);
	return result;
}
