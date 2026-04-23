#include <fstream>
#include <memory>

#include <sys/ioctl.h>
#include <signal.h>
#include <fcntl.h>
#include <termios.h>
#include <dlfcn.h>
#include <time.h>

#ifdef __linux__
# include <termios.h>
# include <linux/kd.h>
# include <linux/keyboard.h>
#elif defined(__FreeBSD__) || defined(__DragonFly__)
# include <sys/ioctl.h>
# include <sys/kbio.h>
#endif

#include <ScopeHelpers.h>
#include <TestPath.h>

#include "TTY/TTYCaps.h"
#include "TTY/TTYRevive.h"

#include <os_call.hpp>

#include "Backend.h"
#include "WinPortRGB.h"
#include "ConsoleOutput.h"
#include "ConsoleInput.h"
#include "WinPortHandle.h"
#include "ExtClipboardBackend.h"
#include "PathHelpers.h"
#include "../sudo/sudo_askpass_ipc.h"
#include "SudoAskpassImpl.h"
#ifdef TESTING
# include "TestController.h"
#endif

#define STDERR_BUFFER_SIZE 0x1000

IConsoleOutput *g_winport_con_out = nullptr;
IConsoleInput *g_winport_con_in = nullptr;
static char *s_stderr_trace = nullptr;
static BOOL s_winport_testing = FALSE;

bool WinPortMainTTY(const char *full_exe_path, int std_in, int std_out,
	bool ext_clipboard, TTYRestrict restrict,
	unsigned int esc_expiration, int notify_pipe, int argc, char **argv,
	int(*AppMain)(int argc, char **argv), int *result);

extern "C" void WinPortInitRegistry();

extern "C" BOOL WinPortTesting()
{
	return s_winport_testing;
}

extern "C" const char *WinPortStderrTrace(size_t *len)
{
	if (s_stderr_trace) {
		static unsigned int s_cnt = 0;
		unsigned int cnt = ++s_cnt;
		*len = STDERR_BUFFER_SIZE;
		// there is no ducumented way to obtain size of buffered content,
		// so here is little trick: write 'magic' string to stderr and then look
		// for it
		char magic[32];
		const int magic_len = snprintf(magic, sizeof(magic), "EndOfSTDERR:%x@%llx\n", cnt, (unsigned long long)time(NULL));
		if (magic_len > 0) {
			fwrite(magic, magic_len, 1, stderr);
			for (int l = 0; l + magic_len <= STDERR_BUFFER_SIZE; ++l) {
				if (memcmp(s_stderr_trace + l, magic, magic_len) == 0) {
					*len = l;
					break;
				}
			}
		}
	} else {
		*len = 0;
	}
	return s_stderr_trace;
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
	fflush(stdout);
	fflush(stderr);
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
			if (err == DEVNULL) {
				if (!s_stderr_trace) {
					s_stderr_trace = (char *)calloc(STDERR_BUFFER_SIZE, 1);
				}
				setvbuf(stderr, s_stderr_trace, _IOFBF, STDERR_BUFFER_SIZE);
			} else {
				setvbuf(stderr, NULL, _IONBF, 0);
			}
		} else
			perror("freopen stderr");
	}

	if ( reopened == 3 && out == DEVNULL && err == DEVNULL) {
		if (!freopen(DEVNULL, "r", stdin)) {
			perror("freopen stdin");
		}
#if !defined(__CYGWIN__) && !defined(__HAIKU__) //TODO
		ioctl(0, TIOCNOTTY, NULL);
#endif
	}
}

static int TTYTryReviveSome(int std_in, int std_out, pid_t &pid)
{
	for (;;) {
		std::vector<TTYRevivableInstance> instances;
		TTYRevivableEnum(instances);
		if (instances.empty()) {
			return -1;
		}

		FScope f_out(fdopen(dup(std_out), "w"));

		fprintf(f_out, "\n\x1b[1;31mSome far2l-s lost in space-time nearby:\x1b[39;22m\n");
		for (size_t i = 0; i < instances.size(); ++i) {
			fprintf(f_out, " \x1b[1;31m%lu\x1b[39;22m: %s\n", (unsigned long)(i + 1), instances[i].info.c_str());
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

		int notify_pipe = TTYReviveIt(instances[index - 1].pid, std_in, std_out);

		if (notify_pipe != -1) {
			pid = instances[index - 1].pid;
			return notify_pipe;
		}

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
	printf("FAR2L backend-specific options:\n"
			"\t--tty - force using TTY backend only (disable GUI/TTY autodetection)\n"
			"\t--notty - don't fallback to TTY backend if GUI backend failed\n"
			"\t--nodetect or --nodetect=[x|xi][f][w][a][k][e] - don't detect if TTY backend supports X11/Xi input and clipboard interaction extensions and/or disable detect f=FAR2l terminal extensions, w=win32, a=apple iTerm2, k=kovidgoyal's kitty input modes, e=emodjie VS16 suffix\n"
			"\t--norgb - don't use true (24-bit) colors\n"
			"\t--mortal - terminate instead of going to background on getting SIGHUP (default if in Linux TTY)\n"
			"\t--immortal - go to background instead of terminating on getting SIGHUP (default if not in Linux TTY)\n"
			"\t--x11 - force GUI backend to run on X11/Xwayland (force make GDK_BACKEND=x11)\n"
			"\t--wayland - force GUI backend to run on Wayland (force make GDK_BACKEND=wayland)\n"
			"\t--ee=N - ESC expiration in msec (default is 100, 0 to disable) to avoid need for double ESC presses (valid only in TTY mode without FAR2L extensions)\n"
			"\t--primary-selection - use PRIMARY selection instead of CLIPBOARD X11 selection (only for GUI backend)\n"
			"\t--maximize - force maximize window upon launch (only for GUI backend)\n"
			"\t--nomaximize - dont maximize window upon launch even if its has saved maximized state (only for GUI backend)\n"
			"\t--size=WxH - set initial window size in characters (only for GUI backend)\n"
			"\t--clipboard=SCRIPT - use external clipboard handler script that implements get/set text clipboard data via its stdin/stdout\n"
			"\n"
			"All options (except -h and -u) also can be set via the FAR2L_ARGS environment variable\n"
			" (for example: export FAR2L_ARGS=\"--tty\" to start far2l in tty mode by default)\n");
}

struct ArgOptions
{
	TTYRestrict restrict{};
	bool tty = false, notty = false;
	bool mortal = false;
	bool x11 = false;
	bool wayland = false;
	std::string ext_clipboard;
	unsigned int esc_expiration = 100;
	std::vector<char *> filtered_argv;

	ArgOptions() = default;

	void ParseArg(char *a, bool need_strdup)
	{
		if (strcmp(a, "--immortal") == 0) {
			mortal = false;

		} else if (strcmp(a, "--mortal") == 0) {
			mortal = true;

		} else if (strcmp(a, "--x11") == 0) {
			x11 = true;

		} else if (strcmp(a, "--wayland") == 0) {
			wayland = true;

		} else if (strcmp(a, "--notty") == 0) {
			notty = true;
			tty = false;

		} else if (strcmp(a, "--tty") == 0) {
			tty = true;

		} else if (strcmp(a, "--norgb") == 0) {
			restrict.rgb = true;

		} else if (strcmp(a, "--nodetect") == 0) {
			restrict.xi = true;
			restrict.x11 = true;
			restrict.far2l = true;
			restrict.apple = true;
			restrict.kitty = true;
			restrict.win32 = true;
			restrict.emoji = true;

		} else if (strstr(a, "--nodetect=") == a) {
			if(strstr(a+11, "xi")) {
				restrict.xi = true;
			} else if (strchr(a+11, 'x')) {
				restrict.x11 = true;
			}
			if(strchr(a+11, 'f')) {
				restrict.far2l = true;
			}
			if(strchr(a+11, 'a')) {
				restrict.apple = true;
			}
			if(strchr(a+11, 'k')) {
				restrict.kitty = true;
			}
			if(strchr(a+11, 'w')) {
				restrict.win32 = true;
			}
			if(strchr(a+11, 'e')) {
				restrict.emoji = true;
			}
		} else if (strstr(a, "--clipboard=") == a) {
			ext_clipboard = a + 12;

		} else if (strstr(a, "--ee") == a) {
			if (a[4] == '=')
				esc_expiration = atoi(&a[5]);

		} else if (need_strdup) {
			char *a_dup = strdup(a);
			if (a_dup) {
				_strdupeds.emplace_back(a_dup);
				filtered_argv.emplace_back(a_dup);
			}
		} else {
			filtered_argv.emplace_back(a);
		}
	}

	~ArgOptions()
	{
		for (auto p : _strdupeds) {
			free(p);
		}
	}

private:
	std::vector<char *> _strdupeds;

	ArgOptions(const ArgOptions&) = delete;
};

static bool IsFar2lTerminal()
{
	const char *env = getenv("TERM_PROGRAM");
	if (env && strcmp(env, "far2l") == 0) {
		return true;
	}
	env = getenv("FISH");
	return (env && strstr(env, "far2l") != NULL);
}

extern "C" int WinPortMain(const char *full_exe_path, int argc, char **argv, int(*AppMain)(int argc, char **argv))
{
	std::unique_ptr<ConsoleOutput> winport_con_out(new ConsoleOutput);
	std::unique_ptr<ConsoleInput> winport_con_in(new ConsoleInput);
	g_winport_con_out = winport_con_out.get();
	g_winport_con_in = winport_con_in.get();
	ArgOptions arg_opts;

	InitPalette();

	if (IsFar2lTerminal()) {
		arg_opts.tty = true; // run far2l helper apps etc in TTY mode, if started from far2l terminal
#if defined(__linux__) || defined(__FreeBSD__) || defined(__DragonFly__)
	} else {
		unsigned long int leds = 0;
		if (ioctl(0, KDGETLED, &leds) == 0) {
			// running under linux 'real' TTY, such kind of terminal cannot be dropped due to lost connection etc
			// also detachable session makes impossible using of ioctl(_stdin, TIOCLINUX, &state) in child (#653),
			// so lets default to mortal mode in Linux/BSD TTY
			arg_opts.mortal = true;
			arg_opts.tty = true;
		}
#endif
	}

	char *ea = getenv("FAR2L_ARGS");
	if (ea != nullptr && *ea) {
		std::string str;
		for (const char *begin = ea;;) {
			++ea;
			if (*ea == 0 || *ea == ' ' || *ea == '\t') {
				if (ea > begin) {
					str.assign(begin, ea - begin);
					arg_opts.ParseArg((char *)str.c_str(), true);
				}
				if (*ea == 0) {
					break;
				}
				begin = ea + 1;
			}
		}
	}

	for (int i = 1; i < argc; ++i) { // from 1 = skip self name here
		arg_opts.ParseArg(argv[i], false);
	}

	//const char *xdg_st = getenv("XDG_SESSION_TYPE");
	//bool on_wayland = ((xdg_st && strcasecmp(xdg_st, "wayland") == 0) || getenv("WAYLAND_DISPLAY"));
	if (arg_opts.x11) {
	//if (((on_wayland && getenv("WSL_DISTRO_NAME")) && !arg_opts.wayland && !getenv("FAR2L_WSL_NATIVE")) || arg_opts.x11) {
		// on wslg stay on x11 by default until remaining upstream wayland-related clipboard bug is fixed
		// https://github.com/microsoft/wslg/issues/1216
		setenv("GDK_BACKEND", "x11", TRUE);
	} else if (arg_opts.wayland) {
		setenv("GDK_BACKEND", "wayland", TRUE);
	}

	if (!arg_opts.tty && !arg_opts.notty) {
		const char *xdg_st = getenv("XDG_SESSION_TYPE");
		if (xdg_st && strcasecmp(xdg_st, "tty") == 0) {
			arg_opts.tty = true;
		}
	}

	if (argc>0)
		arg_opts.filtered_argv.emplace(arg_opts.filtered_argv.begin(), argv[0]); // self name should be always first
	if (!arg_opts.filtered_argv.empty()) {
		argv = &arg_opts.filtered_argv[0];
	}
	argc = (int)arg_opts.filtered_argv.size();

	FDScope std_in(dup(0));
	FDScope std_out(dup(1));

	MakeFDCloexec(std_in);
	MakeFDCloexec(std_out);

//	tcgetattr(std_out, &g_ts_tstp);
	SetupStdHandles();
	if (!arg_opts.mortal) {
		signal(SIGHUP, SIG_IGN);
	}

	WinPortInitRegistry();
	WinPortInitWellKnownEnv();
	ClipboardBackendSetter ext_clipboard_backend_setter;
	if (arg_opts.ext_clipboard.empty()) {
		const std::string &ext_clipboard = InMyConfig("clipboard");
		if (TestPath(ext_clipboard).Executable()) {
			arg_opts.ext_clipboard = ext_clipboard;
		}
	}
	if (!arg_opts.ext_clipboard.empty()) {
		ext_clipboard_backend_setter.Set<ExtClipboardBackend>(arg_opts.ext_clipboard.c_str());
	}
//	g_winport_con_out->WriteString(L"Hello", 5);
#ifdef TESTING
	std::unique_ptr<TestController> test_ctl;
#endif

	const char *test_ctl_id = getenv("FAR2L_TESTCTL");
	if (test_ctl_id && *test_ctl_id) {
#ifdef TESTING
		// derive console output size from terminal now to avoid smoketest race condition on getting terminal size
		struct winsize w{};
		int r = ioctl(std_out, TIOCGWINSZ, &w);
		if (r == 0 && w.ws_col > 80 && w.ws_row > 25) {
			winport_con_out->SetSize(w.ws_col, w.ws_row);
		}
		test_ctl.reset(new TestController(test_ctl_id));
		s_winport_testing = TRUE;
		unsetenv("FAR2L_TESTCTL");
#else
		fprintf(stderr, "Testing facilities not enabled, rebuild with -DTESTING=YES to use FAR2L_TESTCTL environment variable\n");
		return -1;
#endif
	}

	bool wsl_clipboard_workaround = (arg_opts.ext_clipboard.empty()
		&& getenv("WSL_DISTRO_NAME")
		&& !getenv("FAR2L_WSL_NATIVE"));
	if (wsl_clipboard_workaround) {
		arg_opts.ext_clipboard = full_exe_path;
		if (TranslateInstallPath_Bin2Share(arg_opts.ext_clipboard)) {
			ReplaceFileNamePart(arg_opts.ext_clipboard, APP_BASENAME "/wslgclip.sh");
		} else {
			ReplaceFileNamePart(arg_opts.ext_clipboard, "wslgclip.sh");
		}
		if (TestPath(arg_opts.ext_clipboard).Executable()) {
			fprintf(stderr, "WSL cliboard workaround: '%s'\n", arg_opts.ext_clipboard.c_str());
			ext_clipboard_backend_setter.Set<ExtClipboardBackend>(arg_opts.ext_clipboard.c_str());
		} else {
			fprintf(stderr, "Can't use WSL cliboard workaround: '%s'\n", arg_opts.ext_clipboard.c_str());
			arg_opts.ext_clipboard.clear();
		}
	}

	int result = -1;
	if (!arg_opts.tty) {
		std::string gui_path = full_exe_path;
		ReplaceFileNamePart(gui_path, "far2l_gui.so");
		TranslateInstallPath_Bin2Lib(gui_path);
		void *gui_so = dlopen(gui_path.c_str(), RTLD_GLOBAL | RTLD_NOW);
		if (gui_so) {
			typedef bool (*WinPortMainBackend_t)(WinPortMainBackendArg *a);
			WinPortMainBackend_t WinPortMainBackend_p = (WinPortMainBackend_t)dlsym(gui_so, "WinPortMainBackend");
			if (WinPortMainBackend_p) {
				SudoAskpassImpl askass_impl;
				SudoAskpassServer askpass_srv(&askass_impl);

				WinPortMainBackendArg a{FAR2L_BACKEND_ABI_VERSION,
					argc, argv, AppMain, &result, g_winport_con_out, g_winport_con_in, !arg_opts.ext_clipboard.empty(), arg_opts.restrict.rgb};
				if (!WinPortMainBackend_p(&a) ) {
					fprintf(stderr, "Cannot use GUI backend\n");
					arg_opts.tty = !arg_opts.notty;
					if (wsl_clipboard_workaround) {
						//arg_opts.ext_clipboard.clear();
					}
				}
			} else {
				fprintf(stderr, "Cannot find backend entry point, error %s\n", dlerror());
				arg_opts.tty = true;
			}
		} else {
			fprintf(stderr, "Failed to load %s error %s\n", gui_path.c_str(), dlerror());
			arg_opts.tty = true;
		}
	}

	if (arg_opts.tty) {
		if (arg_opts.mortal) {
			SudoAskpassImpl askass_impl;
			SudoAskpassServer askpass_srv(&askass_impl);
			if (!WinPortMainTTY(full_exe_path, std_in, std_out,
					!arg_opts.ext_clipboard.empty(), arg_opts.restrict,
					arg_opts.esc_expiration, -1, argc, argv, AppMain, &result)) {
				fprintf(stderr, "Cannot use TTY backend\n");
			}

		} else {
			int notify_pipe = TTYTryReviveSome(std_in, std_out, g_sigwinch_pid);
			if (notify_pipe == -1) {
				int new_notify_pipe[2] {-1, -1};
				if (pipe(new_notify_pipe) == -1) {
					perror("notify_pipe");
					return -1;
				}
				MakeFDCloexec(new_notify_pipe[0]);
				MakeFDNonCloexec(std_in);
				MakeFDNonCloexec(std_out);
				g_sigwinch_pid = fork();
				MakeFDCloexec(std_in);
				MakeFDCloexec(std_out);
				MakeFDCloexec(new_notify_pipe[1]);
				if (g_sigwinch_pid == 0) {
					{
						setsid();
						SudoAskpassImpl askass_impl;
						SudoAskpassServer askpass_srv(&askass_impl);
						if (!WinPortMainTTY(full_exe_path, std_in, std_out,
								!arg_opts.ext_clipboard.empty(), arg_opts.restrict,
								arg_opts.esc_expiration, new_notify_pipe[1], argc, argv, AppMain, &result)) {
							fprintf(stderr, "Cannot use TTY backend\n");
						}
					}
					_exit(result);
				}
				close(new_notify_pipe[1]);
				notify_pipe = new_notify_pipe[0];
			}

			strncpy(argv[0], "shim.far2l", strlen(argv[0]));
			auto prev_sigwinch = signal(SIGWINCH, ShimSigWinch);
			ReadAll(notify_pipe, (void *)&result, sizeof(result));
			signal(SIGWINCH, prev_sigwinch);
			g_sigwinch_pid = 0;
			CheckedCloseFD(notify_pipe);
		}
	}

	g_winport_con_out = nullptr;
	g_winport_con_in = nullptr;

	return result;
}
