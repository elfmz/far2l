#include <sys/ioctl.h>
#include <signal.h>
#include <fcntl.h>
#include <termios.h> 
#include <ScopeHelpers.h>
#include <TTYRawMode.h>
#include "TTY/TTYRevive.h"
#include "TTY/TTYNegotiateFar2l.h"

#include <os_call.hpp>

#include "Backend.h"
#include "ConsoleOutput.h"
#include "ConsoleInput.h"
#include "WinPortHandle.h"
#include "PathHelpers.h"
#include "../sudo/sudo_askpass_ipc.h"
#include "SudoAskpassImpl.h"

#include <memory>

ConsoleOutput g_winport_con_out;
ConsoleInput g_winport_con_in;

#ifndef NOWX
bool WinPortMainWX(int argc, char **argv, int(*AppMain)(int argc, char **argv), int *result);
#endif

bool WinPortMainTTY(int std_in, int std_out, bool far2l_tty, int notify_pipe, int argc, char **argv, int(*AppMain)(int argc, char **argv), int *result);

extern "C" void WinPortInitRegistry();

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

static void SetupStdHandles()
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
		if (!freopen(DEVNULL, "r", stdin)) {
			perror("freopen stdin");
		}
#if !defined(__CYGWIN__) //TODO
		ioctl(0, TIOCNOTTY, NULL);
#endif
	}
}


class NormalizeTerminalState
{
	int _std_in, _std_out;
	bool _far2l_tty;
	std::unique_ptr<TTYRawMode> &_tty_raw_mode;
	bool _raw;
	bool _far2l_tty_actual;

public:
	NormalizeTerminalState(int std_in, int std_out, bool far2l_tty, std::unique_ptr<TTYRawMode> &tty_raw_mode)
		: _std_in(std_in), _std_out(std_out),
		_far2l_tty(far2l_tty),
		_tty_raw_mode(tty_raw_mode),
		_raw(tty_raw_mode),
		_far2l_tty_actual(far2l_tty)
	{
		Apply();
	}

	~NormalizeTerminalState()
	{
		Revert();
	}

	void Apply()
	{
		if (_far2l_tty && _far2l_tty_actual) {
			TTYNegotiateFar2l(_std_in, _std_out, false);
			_far2l_tty_actual = false;
		}
		_tty_raw_mode.reset();
	}

	void Revert()
	{
		if (_raw && !_tty_raw_mode) {
			_tty_raw_mode.reset(new TTYRawMode(_std_out));
		}

		if (_far2l_tty && !_far2l_tty_actual) {
			_far2l_tty_actual = TTYNegotiateFar2l(_std_in, _std_out, true);
		}
	}
};

static int TTYTryReviveSome(int std_in, int std_out, bool far2l_tty, std::unique_ptr<TTYRawMode> &tty_raw_mode, pid_t &pid)
{
	for (;;) {
		std::vector<TTYRevivableInstance> instances;
		TTYRevivableEnum(instances);
		if (instances.empty()) {
			return -1;
		}

		NormalizeTerminalState nts(std_in, std_out, far2l_tty, tty_raw_mode);
		FScope f_out(fdopen(dup(std_out), "w"));

		fprintf(f_out, "\n\x1b[1;31mSome far2l-s lost in space-time nearby:\x1b[39;22m\n");
		for (size_t i = 0; i < instances.size(); ++i) {
			fprintf(f_out, " \x1b[1;31m%lu\x1b[39;22m: %s\n", i + 1, instances[i].info.c_str());
		}

		fprintf(f_out, "\x1b[1;31mInput instance index to revive or empty string to spawn new far2l\x1b[39;22m\n");

		char buf[32] = {};
		{
			FScope f_in(fdopen(dup(std_in), "r"));
			if (!fgets(buf, sizeof(buf) - 1, f_in)) {
				return -1;
			}
		}
		if (buf[0] == 0 || buf[0] == '\r' || buf[0] == '\n') {
			return -1;
		}

		size_t index = atoi(buf);
		if (buf[0] < '0' || buf[0] > '9' || index == 0 || index > instances.size()) {
			fprintf(f_out, "\x1b[1;31mWrong input\x1b[39;22m\n");
			continue;
		}

		nts.Revert();
		int notify_pipe = TTYReviveIt(instances[index - 1].pid, std_in, std_out, far2l_tty);

		if (notify_pipe != -1) {
			pid = instances[index - 1].pid;
			return notify_pipe;
		}

		nts.Apply();

		fprintf(f_out, "\x1b[1;31mRevival failed\x1b[39;22m\n");
		sleep(1);
	}
}


static pid_t g_sigwinch_pid = 0;

static void ShimSigWinch(int sig)
{
	if (g_sigwinch_pid != 0) {
		kill(g_sigwinch_pid, sig);
	}
}

extern "C" void WinPortHelp()
{
	printf("FAR2L backend-specific options:\n");
	printf("\t--tty - force using TTY backend only (disable GUI/TTY autodetection)\n");
	printf("\t--notty - don't fallback to TTY backend if GUI backend failed\n");
	printf("\t--nodetect - don't detect if TTY backend supports FAR2L extensions\n");
	printf("\t--mortal - terminate instead of going to background on getting SIGHUP\n");
}

extern "C" int WinPortMain(int argc, char **argv, int(*AppMain)(int argc, char **argv))
{
	bool tty = false, far2l_tty = false, nodetect = false, notty = false;
	bool mortal = false;

	std::vector<char *> filtered_argv;
	for (int i = 0; i < argc; ++i) {

		if (strstr(argv[i], "--mortal") == argv[i]) {
			mortal = true;

		} else if (strstr(argv[i], "--notty") == argv[i]) {
			notty = true;

		} else if (strstr(argv[i], "--tty") == argv[i]) {
			tty = true;

		} else if (strstr(argv[i], "--nodetect") == argv[i]) {
			nodetect = true;

		} else {
			filtered_argv.push_back(argv[i]);
		}
	}

	if (!filtered_argv.empty()) {
		argv = &filtered_argv[0];
	}
	argc = (int)filtered_argv.size();

	FDScope std_in(dup(0));
	FDScope std_out(dup(1));

	fcntl(std_in, F_SETFD, FD_CLOEXEC);
	fcntl(std_out, F_SETFD, FD_CLOEXEC);

//	tcgetattr(std_out, &g_ts_tstp);

	std::unique_ptr<TTYRawMode> tty_raw_mode(new TTYRawMode(std_out));
	if (!nodetect) {
//		tty_raw_mode.reset(new TTYRawMode(std_out));
		if (tty_raw_mode->Applied()) {
			far2l_tty = TTYNegotiateFar2l(std_in, std_out, true);
			if (far2l_tty) {
				tty = true;
			}

		} else {
			notty = true;
		}
	}

	SetupStdHandles();
	if (!mortal) {
		signal(SIGHUP, SIG_IGN);
	}

	WinPortInitRegistry();
	WinPortInitWellKnownEnv();
//  g_winport_con_out.WriteString(L"Hello", 5);

	int result = -1;
#ifndef NOWX
	if (!tty) {
		tty_raw_mode.reset();
		SudoAskpassImpl askass_impl;
		SudoAskpassServer askpass_srv(&askass_impl);
		if (!WinPortMainWX(argc, argv, AppMain, &result) ) {
			fprintf(stderr, "Cannot use WX backend\n");
			tty = !notty;
		}
	}
#else
	tty = true;
#endif

	if (tty) {
		if (!tty_raw_mode) {
			tty_raw_mode.reset(new TTYRawMode(std_out));
		}
		if (mortal) {
			SudoAskpassImpl askass_impl;
			SudoAskpassServer askpass_srv(&askass_impl);
			if (!WinPortMainTTY(std_in, std_out, far2l_tty, -1, argc, argv, AppMain, &result)) {
				fprintf(stderr, "Cannot use TTY backend\n");
			}

		} else {
			int notify_pipe = TTYTryReviveSome(std_in, std_out, far2l_tty, tty_raw_mode, g_sigwinch_pid);
			if (notify_pipe == -1) {
				int new_notify_pipe[2] {-1, -1};
				if (pipe(new_notify_pipe) == -1) {
					perror("notify_pipe");
					return -1;
				}
				fcntl(new_notify_pipe[0], F_SETFD, FD_CLOEXEC);
				fcntl(std_in, F_SETFD, 0);
				fcntl(std_out, F_SETFD, 0);
				g_sigwinch_pid = fork();
				fcntl(std_in, F_SETFD, FD_CLOEXEC);
				fcntl(std_out, F_SETFD, FD_CLOEXEC);
				fcntl(new_notify_pipe[1], F_SETFD, FD_CLOEXEC);
				if (g_sigwinch_pid == 0) {
					{
						setsid();
						SudoAskpassImpl askass_impl;
						SudoAskpassServer askpass_srv(&askass_impl);
						if (!WinPortMainTTY(std_in, std_out, far2l_tty, new_notify_pipe[1], argc, argv, AppMain, &result)) {
							fprintf(stderr, "Cannot use TTY backend\n");
						}
					}
					WinPortHandle_FinalizeApp();
					_exit(0);
				}
				close(new_notify_pipe[1]);
				notify_pipe = new_notify_pipe[0];
			}

			strncpy(argv[0], "shim.far2l", strlen(argv[0]));
			auto prev_sigwinch = signal(SIGWINCH, ShimSigWinch);
			char c;
			os_call_ssize(read, notify_pipe, (void *)&c, sizeof(c));
			signal(SIGWINCH, prev_sigwinch);
			g_sigwinch_pid = 0;
			CheckedCloseFD(notify_pipe);
		}

		if (far2l_tty) {
			TTYNegotiateFar2l(std_in, std_out, false);
		}
	}

	WinPortHandle_FinalizeApp();

	return result;
}
