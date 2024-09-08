#include "headers.hpp"
#include "clipboard.hpp"
#include "ctrlobj.hpp"
#include "scrbuf.hpp"
#include "cmdline.hpp"
#include "fileedit.hpp"
#include "fileview.hpp"
#include "interf.hpp"
#include "clipboard.hpp"
#include "message.hpp"
#include "mix.hpp"
#include <signal.h>
#include <mutex>
#include <list>
#include <atomic>
#include <memory>
#include <fcntl.h>
#include <errno.h>
#include <iostream>
#include <fstream>
#include <sys/ioctl.h> 
#include <sys/wait.h> 
#include <condition_variable>
#include <base64.h> 
#include <StackSerializer.h>
#include <ScopeHelpers.h>
#include "dirmix.hpp"
#include "vtansi.h"
#include "vtlog.h"
#include "vtshell.h"
#include "VTFar2lExtensios.h"
#include "InterThreadCall.hpp"
#include "vtshell_compose.h"
#include "vtshell_ioreaders.h"
#include "vtshell_mouse.h"
#include "../WinPort/src/SavedScreen.h"
#define __USE_BSD 
#include <termios.h> 
#include "palette.hpp"
#include "AnsiEsc.hpp"
#include "TestPath.h"

#define BRACKETED_PASTE_SEQ_START "\x1b[200~"
#define BRACKETED_PASTE_SEQ_STOP  "\x1b[201~"

const char *VT_TranslateSpecialKey(const WORD key, bool ctrl, bool alt, bool shift, unsigned char keypad = 0,
	WCHAR uc = 0);

#if 0 //change to 1 to enable verbose I/O reports to stderr
static void DbgPrintEscaped(const char *info, const char *s, size_t l)
{
	std::string msg;
	for (;l;++s, --l) {
		char c = *s;
		if (c=='\\') {
			msg+= "\\\\";
		} else if (c <= 32 || c > 127) {
			char zz[64]; sprintf(zz, "\\%02x", (unsigned int)(unsigned char)c);
			msg+= zz;
		} else 
			msg+= (char)(unsigned char)c;
	}
	fprintf(stderr, "VT %s: '%s'\n", info, msg.c_str());
}
#else
# define DbgPrintEscaped(i, s, l) 
#endif

int VTShell_Leader(char *const shell_argv[], const char *pty);

std::string VTSanitizeHistcontrol()
{
	std::string hc_override;
	const char *hc = getenv("HISTCONTROL");
	if (!hc || (!strstr(hc, "ignorespace") && !strstr(hc, "ignoreboth"))) {
		hc_override = "ignorespace";
		if (hc && *hc) {
			hc_override+= ':';
			hc_override+= hc;
		}
		fprintf(stderr, "Override HISTCONTROL='%s'\n", hc_override.c_str());
	}
	return hc_override;
}

class VTShell : VTOutputReader::IProcessor, VTInputReader::IProcessor, IVTShell
{
	HANDLE _console_handle = NULL;
	std::atomic<bool> _console_switch_requested{false};
	std::atomic<bool> _console_kill_requested{false};

	VTAnsi _vta;
	VTInputReader _input_reader;
	VTOutputReader _output_reader;
	std::mutex _inout_control_mutex;
	int _fd_out, _fd_in;
	int _pipes_fallback_in, _pipes_fallback_out;
	pid_t _leader_pid;
	std::string _slavename;
	std::atomic<unsigned char> _keypad{0};
	std::atomic<bool> _bracketed_paste_expected{false};
	std::atomic<bool> _win32_input_mode_expected{false};
	std::atomic<int> _kitty_kb_flags{0};
	INPUT_RECORD _last_window_info_ir;
	std::unique_ptr<VTFar2lExtensios> _far2l_exts;
	std::unique_ptr<VTMouse> _mouse;
	std::mutex _read_state_mutex, _write_term_mutex;
	uint32_t _mouse_expectations{0};

	std::string _start_marker, _exit_marker;
	std::string _host_id;
	unsigned int _exit_code;
	bool _may_notify{false};
	std::atomic<bool> _allow_osc_clipset{false};
	std::string _init_user_profile;

	static const char *GetSystemShell()
	{
		const char *env_shell = getenv("SHELL");
		if (!env_shell || !*env_shell) {
			return "/bin/sh";
		}

		const char *slash = strrchr(env_shell, '/');
		// avoid using fish and csh for a while, it requires changes in Opt.strQuotedSymbols and some others
		if (strcmp(slash ? slash + 1 : env_shell, "fish") == 0
		 || strcmp(slash ? slash + 1 : env_shell, "csh") == 0
		 || strcmp(slash ? slash + 1 : env_shell, "tcsh") == 0 ) {
			return "bash";
		}

		return env_shell;
	}

	int ExecLeaderProcess()
	{
		std::string home = GetMyHome();

		std::string far2l_exename = g_strFarModuleName.GetMB();

		Environment::ExplodeCommandLine shell_exploded;
		if (Opt.CmdLine.UseShell) {
			shell_exploded.Parse(Opt.CmdLine.strShell.GetMB());
		}

		if (shell_exploded.empty() || shell_exploded.front().empty()) {
			shell_exploded.Parse(GetSystemShell());
			shell_exploded.emplace_back("-i");
		}

		std::vector<char *> shell_argv;
		for (const auto &arg : shell_exploded) {
			shell_argv.emplace_back((char *)arg.c_str());
		}
		shell_argv.emplace_back(nullptr);

		// Will need to ensure that HISTCONTROL prevents adding to history commands that start by space
		// to avoid shell history pollution by far2l's intermediate script execution commands
		const std::string &hc_override = VTSanitizeHistcontrol();

		const BYTE col = static_cast<BYTE>(FarColorToReal(COL_COMMANDLINEUSERSCREEN));
		char colorfgbg[32];
		snprintf(colorfgbg, sizeof(colorfgbg), "%u;%u",
			 AnsiEsc::ConsoleColorToAnsi(col & 0xf),
			 AnsiEsc::ConsoleColorToAnsi((col >> 4) & 0xf));

		const auto color_bpp = WINPORT(GetConsoleColorPalette)(ConsoleHandle());
		std::string askpass_app;
		if (Opt.SudoEnabled) {
			askpass_app = GetHelperPathName("far2l_askpass");
		}
#ifdef __APPLE__ // workaround for Macos
		if (strstr(shell_argv.front(), "zsh")) {
			_init_user_profile = ".zprofile";
		} else if (strstr(shell_argv.front(), "bash")) {
			_init_user_profile = ".bash_profile";;
		}
#endif
		std::cin.sync();
		std::cout.flush();
		std::cerr.flush();
		std::clog.flush();
		int r = fork();
		if (r != 0) {
			return r;
		}

		switch (color_bpp) {
			case 24:
				setenv("TERM", "xterm-256color", 1);
				setenv("COLORTERM", "truecolor", 1);
				break;

			case 8:
				setenv("TERM", "xterm-256color", 1);
				setenv("COLORTERM", "256color", 1);
				break;

			default:
				setenv("TERM", "xterm", 1);
				unsetenv("COLORTERM");
		}

		if (!askpass_app.empty()) {
			setenv("SUDO_ASKPASS", askpass_app.c_str(), 1);
		}

		setenv("COLORFGBG", colorfgbg, 1);

		if (!hc_override.empty()) {
			setenv("HISTCONTROL", hc_override.c_str(), 1);
		}

		// avoid locking current directory
		if (chdir(home.c_str()) != 0) {
			if (chdir("/") != 0) {
				perror("chdir /");
			}
		}

		if (_slavename.empty()) {
			dup2(_pipes_fallback_in, STDIN_FILENO);
			dup2(_pipes_fallback_out, STDOUT_FILENO);
			dup2(_pipes_fallback_out, STDERR_FILENO);
			CheckedCloseFD(_pipes_fallback_in);
			CheckedCloseFD(_pipes_fallback_out);
		}

		r = VTShell_Leader(shell_argv.data(), _slavename.c_str());
		fprintf(stderr, "%s: VTShell_Leader('%s', '%s') returned %d errno %u\n",
			__FUNCTION__, shell_argv[0], _slavename.c_str(), r, errno);

		int err = errno;
		_exit(err);
		exit(err);
		return -1;
	}
	
	void UpdateTerminalSize(int fd_term)
	{
		CONSOLE_SCREEN_BUFFER_INFO csbi = { };
		if (WINPORT(GetConsoleScreenBufferInfo)(ConsoleHandle(), &csbi )
					&& csbi.dwSize.X && csbi.dwSize.Y) {
			fprintf(stderr, "UpdateTerminalSize: %u x %u\n", csbi.dwSize.X, csbi.dwSize.Y);
			struct winsize ws = {(unsigned short)csbi.dwSize.Y, 
				(unsigned short)csbi.dwSize.X, 0, 0};
			if (ioctl( fd_term, TIOCSWINSZ, &ws )==-1)
				perror("VT: ioctl(TIOCSWINSZ)");
		}
	}
	
	bool InitTerminal()
	{
		int fd_term = posix_openpt( O_RDWR | O_NOCTTY ); //use -1 to verify pipes fallback functionality
		_slavename.clear();
		if (fd_term!=-1) {
			MakeFDCloexec(fd_term);
			
			if (grantpt(fd_term)==0 && unlockpt(fd_term)==0) {
				UpdateTerminalSize(fd_term);
				const char *slavename = ptsname(fd_term);
				if (slavename && *slavename)
					_slavename = slavename;
				else
					perror("VT: ptsname");				
			} else
				perror("VT: grantpt/unlockpt");
				
			if (_slavename.empty()) {
				CheckedCloseFD(fd_term);
			}
		} else
			perror("VT: posix_openpt");

		if (fd_term==-1) {
			fprintf(stderr, "VT: fallback to pipes\n");
			int fd_in[2] = {-1, -1}, fd_out[2] = {-1, -1};
			if (pipe(fd_in) < 0) {
				perror("VT: fds_in");
				return false;
			}

			if (pipe(fd_out) < 0) {
				perror("VT: fds_out");
				CheckedCloseFDPair(fd_in);
				return false;
			}
			MakeFDCloexec(fd_in[1]);
			MakeFDCloexec(fd_out[0]);
			
			_pipes_fallback_in = fd_in[0];
			_pipes_fallback_out = fd_out[1];
			_fd_in = fd_in[1];
			_fd_out = fd_out[0];

		} else {
			_pipes_fallback_in = _pipes_fallback_out = -1;
			struct termios ts = {};
			if (tcgetattr(fd_term, &ts) == 0) {
				ts.c_lflag |= ISIG | ICANON | ECHO;
#ifdef IUTF8
				ts.c_iflag |= IUTF8;
#endif
				//ts.c_lflag&= ~ECHO;
				ts.c_cc[VINTR] = 003;
				// ts.c_cc[VQUIT] = 034;
				if (tcsetattr( fd_term, TCSAFLUSH, &ts ) == -1) {
					perror("InitTerminal: tcsetattr");
				}
			} else {
				perror("InitTerminal: tcgetattr");
			}
			_fd_in = fd_term;
			_fd_out = dup(fd_term);
			MakeFDCloexec(_fd_out);
		}

		return true;
	}

	bool Startup()
	{
		if (!InitTerminal())
			return false;
		
		int r = ExecLeaderProcess();
		if (r == -1) {
			perror("VT: exec leader");
			return false;
		}

		_leader_pid = r;
		usleep(300000);//give it time to initialize, otherwise additional command copy will be echoed
		return true;
	}
	

	virtual bool OnProcessOutput(const char *buf, int len) //called from worker thread
	{
		DbgPrintEscaped("OUTPUT", buf, len);
		if (_slavename.empty()) {
			// pipes fallback mode
			for (int i = 0, ii = 0; i <= len; ++i) {
				if (i == len || buf[i] == '\n') {
					if (i > ii) {
						_vta.Write(&buf[ii], i - ii);
					}
					if (i == len) {
						break;
					}
					char cr = '\r';
					_vta.Write(&cr, 1);
					ii = i;
				}
			}
		} else {
			_vta.Write(buf, len);
		}
		return !_exit_marker.empty();
	}
	
	virtual void OnTerminalResized()
	{
		if (!_slavename.empty())
			UpdateTerminalSize(_fd_out);
		if (_far2l_exts)
			_far2l_exts->OnTerminalResized();
	}

	virtual void OnInputResized(const INPUT_RECORD &ir) //called from worker thread
	{
		OnTerminalResized();
		_last_window_info_ir = ir;
	}
	
	virtual void OnInputMouse(const MOUSE_EVENT_RECORD &MouseEvent)
	{
		//fprintf(stderr, "OnInputMouse: %x\n", MouseEvent.dwEventFlags);
		{
			std::lock_guard<std::mutex> lock(_read_state_mutex);
			if (_far2l_exts && _far2l_exts->OnInputMouse(MouseEvent))
				return;

			if (_mouse && _mouse->OnInputMouse(MouseEvent))
				return;
		}

		if (MouseEvent.dwEventFlags & MOUSE_WHEELED) {
			if (short(HIWORD(MouseEvent.dwButtonState)) > 0) {
				OnConsoleLog(CLK_VIEW_AUTOCLOSE);
			}
		} else if ( (MouseEvent.dwButtonState&FROM_LEFT_1ST_BUTTON_PRESSED) != 0 &&
				(MouseEvent.dwEventFlags & (MOUSE_HWHEELED | MOUSE_MOVED | DOUBLE_CLICK)) == 0 ) {
			WINPORT(BeginConsoleAdhocQuickEdit)();
		}
	}


	bool WriteTerm(const char *str, size_t len)
	{
		if (len == 0)
			return true;

		std::lock_guard<std::mutex> lock(_write_term_mutex);
		if (WriteAll(_fd_in, (const void *)str, len) != len) {
			perror("WriteTerm - write");
			return false;
		}

		return true;
	}

	virtual void OnBracketedPaste(bool start) //called from worker thread
	{
		if (_bracketed_paste_expected) {
			const char *seq = start ? BRACKETED_PASTE_SEQ_START : BRACKETED_PASTE_SEQ_STOP;
			if (!WriteTerm(seq, strlen(seq))) {
				fprintf(stderr, "VT: OnBracketedPaste - write error %d\n", errno);
			}
		}
	}

	virtual void OnInputInjected(const std::string &str) //called from worker thread
	{
		if (!WriteTerm(str.c_str(), str.size())) {
			fprintf(stderr, "VT: OnInputInjected - write error %d\n", errno);
		}
	}

	virtual void OnInputKey(const KEY_EVENT_RECORD &KeyEvent) //called from worker thread
	{
		{
			std::lock_guard<std::mutex> lock(_read_state_mutex);
			if (_far2l_exts && _far2l_exts->OnInputKey(KeyEvent))
				return;
		}

		DWORD dw;
		const std::string &translated = TranslateKeyEvent(KeyEvent);

		if (!KeyEvent.bKeyDown && !_win32_input_mode_expected && !(_kitty_kb_flags & 2))
			return;

		if (!translated.empty()) {
			if (_slavename.empty() && KeyEvent.uChar.UnicodeChar) {//pipes fallback
				WINPORT(WriteConsole)(ConsoleHandle(), &KeyEvent.uChar.UnicodeChar, 1, &dw, NULL );
			}
			DbgPrintEscaped("INPUT", translated.c_str(), translated.size());
			if (!WriteTerm(translated.c_str(), translated.size())) {
				fprintf(stderr, "VT: OnInputKeyDown - write error %d\n", errno);
			}
		} else {
			fprintf(stderr, "VT: not translated keydown: VK=0x%x MODS=0x%x char=0x%x\n", 
				KeyEvent.wVirtualKeyCode, KeyEvent.dwControlKeyState,
				KeyEvent.uChar.UnicodeChar );
		}		
	}

	void OnCtrlC(bool alt)
	{
		if (alt) {
			_console_kill_requested = true;
			fprintf(stderr, "VT: Ctrl+Alt+C - killing them hardly...\n");
			SendSignalToVT(SIGKILL);
			DetachTerminal();
			
		} else if (_slavename.empty()) {//pipes fallback
			SendSignalToVT(SIGINT);
		}
	}

	enum ConsoleLogKind
	{
		CLK_EDIT,
		CLK_VIEW,
		CLK_VIEW_AUTOCLOSE
	};

	static int sShowConsoleLog(ConsoleLogKind kind)
	{
		if (!CtrlObject || !CtrlObject->CmdLine)
			return 0;

		ScrBuf.FillBuf();
		CtrlObject->CmdLine->SaveBackground();

		SetFarConsoleMode(TRUE);
		if (kind == CLK_EDIT)
			EditConsoleHistory(NULL, true);
		else
			ViewConsoleHistory(NULL, true, kind == CLK_VIEW_AUTOCLOSE);

		CtrlObject->CmdLine->ShowBackground();
		ScrBuf.Flush();
		return 1;
	}

	void OnConsoleLog(ConsoleLogKind kind)//NB: called not from main thread!
	{
		std::unique_lock<std::mutex> lock(_inout_control_mutex, std::try_to_lock);
		if (!lock) {
			fprintf(stderr, "VTShell::OnConsoleLog: SKIPPED\n");
			return;
		}

		//called in input thread context
		//we're input, stop output and remember _vta state
		
		StopAndStart<VTOutputReader> sas(_output_reader);
		VTAnsiSuspend vta_suspend(_vta);
		if (!vta_suspend)
			return;

		DeliverPendingWindowInfo();
		InterThreadCall<int>(std::bind(sShowConsoleLog, kind));

		if (!_slavename.empty())
			UpdateTerminalSize(_fd_out);
		if (_far2l_exts)
			_far2l_exts->OnTerminalResized();
	}

	void OnConsoleSwitch()
	{
		InterThreadLockAndWake ittlaw;
		_console_switch_requested = true;
	}

	virtual void OnKeypadChange(unsigned char keypad)
	{
//		fprintf(stderr, "VTShell::OnKeypadChange: %u\n", keypad);
		_keypad = keypad;
	}

	virtual void OnMouseExpectation(MouseExpectation mex, bool enable)
	{
		fprintf(stderr, "VT::OnMouseExpectation: %u\n", mex);

		std::lock_guard<std::mutex> lock(_read_state_mutex);
		const auto prev_mouse_expectations = _mouse_expectations;
		if (enable) {
			_mouse_expectations|= mex;
		} else {
			_mouse_expectations&= ~(uint32_t)mex;
		}
		if (prev_mouse_expectations != _mouse_expectations) {
			_mouse.reset();
			_mouse.reset(new VTMouse(this, _mouse_expectations));
		}
	}

	virtual void OnBracketedPasteExpectation(bool enabled)
	{
		_bracketed_paste_expected = enabled;
	}

	virtual void OnWin32InputMode(bool enabled)
	{
		_win32_input_mode_expected = enabled;
	}

	virtual void SetKittyFlags(int flags)
	{
		_kitty_kb_flags = flags;
	}

	virtual int GetKittyFlags()
	{
		return _kitty_kb_flags;
	}

	virtual void OnApplicationProtocolCommand(const char *str)//NB: called not from main thread!
	{
		if (strncmp(str, "far2l", 5) == 0) {
			std::string reply;
			switch (str[5]) {
				case '1': {
					std::lock_guard<std::mutex> lock(_read_state_mutex);
					if (!_far2l_exts)
						_far2l_exts.reset(new VTFar2lExtensios(this, _host_id));

					reply = "\x1b_far2lok\x07";
				} break;

				case '0': {
					std::lock_guard<std::mutex> lock(_read_state_mutex);
					_far2l_exts.reset();
				} break;

				case ':': {
					std::lock_guard<std::mutex> lock(_read_state_mutex);
					if (str[6] && _far2l_exts) {
						VTAnsiSuspend vta_suspend(_vta);
						StackSerializer stk_ser;
						uint8_t id = 0;
						try {
							stk_ser.FromBase64(str + 6, strlen(str + 6));
							id = stk_ser.PopU8();
							_far2l_exts->OnInteract(stk_ser);

						} catch (std::exception &e) {
							fprintf(stderr, "_far2l_exts->OnInteract: %s\n", e.what());
							stk_ser.Clear();
						}

						if (id) try {
							stk_ser.PushNum(id);
							reply = "\x1b_far2l";
							reply+= stk_ser.ToBase64();
							reply+= '\x07';

						} catch (std::exception &) {
							reply.clear();
						}
					}

				} break;

				case '#': { // NetRocks host identity, used to fortify clipboard auth
					if (_host_id.empty()) {
						_host_id = str + 6;
					} else {
						fprintf(stderr, "VT: superfluous host_id='%s', prev='%s'\n", str + 6, _host_id.c_str());
					}
				} break;

				case '_': { // internal markers control
					if (!_start_marker.empty() && _start_marker == &str[6]) {
						_start_marker.clear();
						_vta.EnableOutput();
					}
					else if (!_exit_marker.empty()
						&& strncmp(&str[6], _exit_marker.c_str(), _exit_marker.size()) == 0)
					{
						_exit_code = atoi(&str[6 + _exit_marker.size()]);
						_exit_marker.clear();
						if (_may_notify) {
							DisplayNotification(
								_exit_code ? Msg::ConsoleCommandFailed : Msg::ConsoleCommandComplete,
									GetTitle().c_str());
						}
//						fprintf(stderr, "_exit_marker=%s _exit_code=%d\n", &str[6], _exit_code);
					} else {
						fprintf(stderr,
							"OnApplicationProtocolCommand - bad marker: '%s' while _start_marker='%s' _exit_marker='%s'\n",
							&str[6], _start_marker.c_str(), _exit_marker.c_str());
					}
				} break;

			}
			if (!reply.empty())
				_input_reader.InjectInput(reply.c_str(), reply.size());
		}
	}

	virtual bool OnOSCommand(int id, std::string &str)
	{
		if (id == 52) try {
			OnOSC_ClipboardSet(str);
			return true;

		} catch (std::exception &e) {
			fprintf(stderr, "%s: %s\n", "OnOSCommand", e.what());
		}

		return false;
	}

	void OnOSC_ClipboardSet(std::string &str)
	{
		StrTrim(str, "; \t");
		if (!_allow_osc_clipset) {
			{
				VTAnsiSuspend vta_suspend(_vta); // preserve console state
				std::lock_guard<std::mutex> lock(_read_state_mutex); // stop input readout
				SavedScreen saved_scr;
				ScrBuf.FillBuf();
				int choice;
				do { // prevent quick thoughtless tap Enter or Space or Esc in dialog
					choice = Message(MSG_KEEPBACKGROUND, 4,
						Msg::TerminalClipboardAccessTitle,
						Msg::TerminalClipboardSetText,
						L"...",	// 0 - stub select for thoughtless tap
						Msg::TerminalClipboardAccessBlock,		// 1
						Msg::TerminalClipboardSetAllowOnce,		// 2
						Msg::TerminalClipboardSetAllowForCommand);	// 3
				} while (choice <= 0 );
				if (choice != 2 && choice != 3) {
					return;
				}
				if (choice == 3) {
					_allow_osc_clipset = true;
				}
			}
			OnTerminalResized(); // window could resize during dialog box processing
		}

		// remove "c;" prefix if any
		size_t pos = str.rfind(';');
		if (pos != std::string::npos) {
			str.erase(0, pos + 1);
		}

		std::vector<unsigned char> plain;
		base64_decode(plain, str);
		{ // release no more needed memory
			std::string().swap(str);
		}
		std::wstring ws;
		MB2Wide((char *)plain.data(), strnlen((char *)plain.data(), plain.size()), ws);
		{// release no more needed memory
			std::vector<unsigned char>().swap(plain);
		}
		CopyToClipboard(ws.c_str());
	}
	
	virtual void InjectInput(const char *str)
	{
		_input_reader.InjectInput(str, strlen(str));
	}

	std::string StringFromClipboard()
	{
		std::string out;
		wchar_t *wz = PasteFromClipboard();
		if (wz) {
			out = Wide2MB(&wz[0]);
			free(wz);
			// #1424, convert: "\n" -> "\r",  "\r\n" -> "\r"
			for (size_t i = out.size(); i > 0; ) {
				--i;
				if (out[i] == '\n') {
					if (i == 0 || out[i - 1] != '\r') {
						out[i] = '\r';
					} else {
						out.erase(i, 1);
					}
				}
			}
		}

		if (_bracketed_paste_expected) {
			out.insert(0, BRACKETED_PASTE_SEQ_START);
			out.append(BRACKETED_PASTE_SEQ_STOP);
		}

		return out;
	}
	
	std::string TranslateKeyEvent(const KEY_EVENT_RECORD &KeyEvent)
	{
		if (KeyEvent.wVirtualKeyCode) {

			const bool ctrl = (KeyEvent.dwControlKeyState & (LEFT_CTRL_PRESSED|RIGHT_CTRL_PRESSED)) != 0;
			const bool alt = (KeyEvent.dwControlKeyState & (RIGHT_ALT_PRESSED|LEFT_ALT_PRESSED)) != 0;
			const bool shift = (KeyEvent.dwControlKeyState & (SHIFT_PRESSED)) != 0;

			if (KeyEvent.bKeyDown) {

				if (!ctrl && !shift && !alt && KeyEvent.wVirtualKeyCode==VK_BACK && !_kitty_kb_flags) {
					//WCM has a setting for that, so probably in some cases backspace should be returned as is
					char backspace[] = {127, 0};
					return backspace;
				}

				if ((ctrl && shift && !alt && KeyEvent.wVirtualKeyCode=='V') ||
						(!ctrl && shift && !alt && KeyEvent.wVirtualKeyCode==VK_INSERT) ) {
					return StringFromClipboard();
				}

				if (ctrl && !shift && KeyEvent.wVirtualKeyCode=='C') {
					OnCtrlC(alt);
				}

				if (ctrl && !shift && alt && KeyEvent.wVirtualKeyCode=='Z') {
					OnConsoleSwitch();
//					WINPORT(ConsoleBackgroundMode)(TRUE);
					return "";
				}

				if (ctrl && shift && KeyEvent.wVirtualKeyCode==VK_F4) {
					OnConsoleLog(CLK_EDIT);
					return "";
				}

				if (ctrl && shift && KeyEvent.wVirtualKeyCode==VK_F3) {
					OnConsoleLog(CLK_VIEW);
					return "";
				}
			}

			if (_kitty_kb_flags) {

				// References:
				// https://sw.kovidgoyal.net/kitty/keyboard-protocol/
				// https://learn.microsoft.com/en-us/windows/win32/inputdev/virtual-key-codes

				char buffer[64] = {0};

				// fixme: KEYPAD 5 работает как F5, а надо чтоб как F3

				int keycode = towlower(KeyEvent.uChar.UnicodeChar);

				int base = 0;
				if ((KeyEvent.wVirtualKeyCode >= 'A') && (KeyEvent.wVirtualKeyCode <= 'Z')) {
					base = towlower(KeyEvent.wVirtualKeyCode);
					if (base == keycode) { base = 0; }
				}

				// workaround for tty backend
				if (base && !keycode) { keycode = base; }

				int shifted = 0;

				// (KeyEvent.uChar.UnicodeChar && iswupper(KeyEvent.uChar.UnicodeChar))
				// is workaround for far2l wx as it is not sending Shift state for Char events
				// See
				// ir.Event.KeyEvent.wVirtualKeyCode = VK_OEM_PERIOD;
				// and below in wxMain.cpp: dwControlKeyState not set
				if (shift || (KeyEvent.uChar.UnicodeChar && iswupper(KeyEvent.uChar.UnicodeChar))) {
					shifted = KeyEvent.uChar.UnicodeChar;
				}

				int modifiers = 0;

				if (shift) modifiers |= 1;
				if (alt)   modifiers |= 2;
				if (ctrl)  modifiers |= 4;
				if (KeyEvent.dwControlKeyState & CAPSLOCK_ON) modifiers |= 64;
				if (KeyEvent.dwControlKeyState & NUMLOCK_ON)  modifiers |= 128;

				modifiers += 1; // as spec requres

				char suffix = 'u';

				// apply modifications for special keys

				switch (KeyEvent.wVirtualKeyCode) {
					case VK_ESCAPE:    keycode = 27;  break;
					case VK_RETURN:    keycode = 13;  break;

					case VK_TAB:       keycode = 9;   break;
					case VK_BACK:      keycode = 127; break;

					case VK_INSERT:    keycode = 2;   suffix = '~'; break;
					case VK_DELETE:    keycode = 3;   suffix = '~'; break;

					case VK_LEFT:      keycode = 1;   suffix = 'D'; break;
					case VK_RIGHT:     keycode = 1;   suffix = 'C'; break;
					case VK_UP:        keycode = 1;   suffix = 'A'; break;
					case VK_DOWN:      keycode = 1;   suffix = 'B'; break;

					case VK_PRIOR:     keycode = 5;   suffix = '~'; break;
					case VK_NEXT:      keycode = 6;   suffix = '~'; break;

					case VK_HOME:      keycode = 1;   suffix = 'H'; break;
					case VK_END:       keycode = 1;   suffix = 'F'; break;

					case VK_F1:        keycode = 11;  suffix = '~'; break;
					case VK_F2:        keycode = 12;  suffix = '~'; break;
					case VK_F3:        keycode = 13;  suffix = '~'; break;
					case VK_F4:        keycode = 14;  suffix = '~'; break;
					case VK_F5:        keycode = 15;  suffix = '~'; break;

					case VK_F6:        keycode = 17;  suffix = '~'; break;
					case VK_F7:        keycode = 18;  suffix = '~'; break;
					case VK_F8:        keycode = 19;  suffix = '~'; break;
					case VK_F9:        keycode = 20;  suffix = '~'; break;
					case VK_F10:       keycode = 21;  suffix = '~'; break;

					case VK_F11:       keycode = 23;  suffix = '~'; break;
					case VK_F12:       keycode = 24;  suffix = '~'; break;

					case VK_MENU:
					{
						if (KeyEvent.dwControlKeyState & ENHANCED_KEY) {
							// right
							keycode = 57449; suffix = 'u';
						} else {
							// left
							keycode = 57443; suffix = 'u';
						}

						break;
					}

					case VK_CONTROL:
					{
						if ((KeyEvent.dwControlKeyState & ENHANCED_KEY)) {
							// right
							keycode = 57448; suffix = 'u';
						} else {
							// left
							keycode = 57442; suffix = 'u';
						}

						break;
					}

					case VK_SHIFT:
					{
						if (KeyEvent.wVirtualScanCode == RIGHT_SHIFT_VSC) {
							// right
							keycode = 57447; suffix = 'u';
						} else {
							// left
							keycode = 57441; suffix = 'u';
						}

						break;
					}

				}

				// avoid sending base char if it is equal to keycode
				if (base == keycode) { base = 0; }

				int flags = _kitty_kb_flags;


				if (!(flags & 8)) { // "Report all keys as escape codes" disabled
					// do not sent modifiers themselfs
					if (!keycode && (modifiers > 1)) {
						return "";
					}
				}

				// Записываем ESC-последовательность

				// CSI unicode-key-code:shifted-key:base-layout-key ; modifiers:event-type ; text-as-codepoints u

				// Переменная для отслеживания текущей длины строки в буфере
				int len = 0;

				// Старт последовательности
				len += snprintf(buffer + len, sizeof(buffer) - len, "\x1B[");

				// Часть 1

				// We are not able to generate proper sequence for this key for now, sorry
				if (!keycode) { return ""; }

				// Добавляем значение keycode
				len += snprintf(buffer + len, sizeof(buffer) - len, "%i", keycode);

				if ((flags & 4) && (shifted || base)) { // "report alternative keys" enabled

					len += snprintf(buffer + len, sizeof(buffer) - len, ":");

					if (shifted) {
						// Добавляем значение shifted
						len += snprintf(buffer + len, sizeof(buffer) - len, "%i", shifted);
					}

					if (base) {

						// Добавляем значение base
						len += snprintf(buffer + len, sizeof(buffer) - len, ":%i", base);
					}
				}

				// Часть 2

				if ((modifiers > 1) || ((flags & 2) && !KeyEvent.bKeyDown)) {

					len += snprintf(buffer + len, sizeof(buffer) - len, ";");

					// Добавляем значение modifiers
					len += snprintf(buffer + len, sizeof(buffer) - len, "%i", modifiers);

					if ((flags & 2) && !KeyEvent.bKeyDown) {
						// Добавляем значение для типа события (1 для keydown, 2 для repeat, 3 для keyup)
						// fixme: repeat unimplemented
						len += snprintf(buffer + len, sizeof(buffer) - len, ":%i", 3);
					}
				}

				// Часть 3

				if ((flags & 16) && KeyEvent.uChar.UnicodeChar) { // "text as code points" enabled

					if (!((modifiers > 1) || ((flags & 2) && !KeyEvent.bKeyDown))) {
						// Если часть 2 пропущена, добавим ";", чтобы обозначить это
						len += snprintf(buffer + len, sizeof(buffer) - len, ";");
					}

					// Добавляем значение UnicodeChar
					len += snprintf(buffer + len, sizeof(buffer) - len, ";%i", KeyEvent.uChar.UnicodeChar);
				}

				// Финал

				// Добавляем значение suffix
				len += snprintf(buffer + len, sizeof(buffer) - len, "%c", suffix);

				if (!(flags & 8) && KeyEvent.uChar.UnicodeChar && !alt && !ctrl) { // "Report all keys as escape codes" disabled
					// just send text
					len = snprintf(buffer, sizeof(buffer), "%lc", KeyEvent.uChar.UnicodeChar);
				}

				// Финальный 0
				len += snprintf(buffer + len, sizeof(buffer) - len, "%c", 0);

				/*
				FILE* f = fopen("far2l_k.log", "a");
				fprintf(f, "%s\n", buffer);
				fclose(f);
				*/

				return buffer;
			}

			if (_win32_input_mode_expected) {
				char buffer[64] = {0};
				snprintf(buffer, sizeof(buffer), "\x1B[%i;%i;%i;%i;%i;%i_",
						 KeyEvent.wVirtualKeyCode,
						 KeyEvent.wVirtualScanCode,
						 KeyEvent.uChar.UnicodeChar,
						 KeyEvent.bKeyDown,
						 KeyEvent.dwControlKeyState,
						 KeyEvent.wRepeatCount
					);
				return buffer;
			}

			if (!KeyEvent.bKeyDown) { return ""; }

			const char *spec = VT_TranslateSpecialKey(
				KeyEvent.wVirtualKeyCode, ctrl, alt, shift, _keypad, KeyEvent.uChar.UnicodeChar);
			if (spec)
				return spec;
		}

		wchar_t wz[3] = {KeyEvent.uChar.UnicodeChar, 0};
		if (_slavename.empty() && wz[0] == '\r') //pipes fallback
			wz[0] = '\n';
		
		return Wide2MB(&wz[0]);
	}
	
	void SendSignalToVT(int sig)
	{
		if (_leader_pid == -1) {
			fprintf(stderr, "%s: no shell\n", __FUNCTION__);
			return;
		}

		pid_t grp = getpgid(_leader_pid);
		if (grp != -1 && grp != getpgid(getpid())) {
			int r = killpg(grp, sig);
			fprintf(stderr, "%s: killpg(%d, %d) -> %d errno=%d\n", __FUNCTION__, grp, sig, r, errno);
			// kill(_leader_pid, sig);

		} else {
			int r = kill(_leader_pid, sig);
			fprintf(stderr, "%s: kill(%d, %d) -> %d errno=%d\n", __FUNCTION__, _leader_pid, sig, r, errno);
		}
	}
	
	void DetachTerminal()
	{
		FDScope dev_null(open("/dev/null", O_RDWR));
		if (dev_null.Valid()) {
			if (_fd_in != -1)
				dup2(dev_null, _fd_in);
			if (_fd_out != -1)
				dup2(dev_null, _fd_out);

		} else {
			perror("OnRequestShutdown - open /dev/null");
			CheckedCloseFD(_fd_in);
			CheckedCloseFD(_fd_out);
		}

		_output_reader.KickAss();
	}

	virtual void OnRequestShutdown()
	{
		DetachTerminal();
	}

	void Shutdown()
	{
		OnRequestShutdown();
		SendSignalToVT(SIGTERM);
		CheckLeaderAlive(true);
	}

	void DeliverPendingWindowInfo()
	{
		if (_last_window_info_ir.EventType == WINDOW_BUFFER_SIZE_EVENT) {
			DWORD dw = 0;
			WINPORT(WriteConsoleInput)(ConsoleHandle(), &_last_window_info_ir, 1, &dw);
			_last_window_info_ir.EventType = 0;
		}
	}

	std::unique_ptr<VT_ComposeCommandExec> _cce;


	void StartIOReaders()
	{
		std::lock_guard<std::mutex> lock(_inout_control_mutex);
		_output_reader.Start(_fd_out);
		_input_reader.Start(_console_handle);
	}

	void StopIOReaders()
	{
		std::lock_guard<std::mutex> lock(_inout_control_mutex);
		_output_reader.Stop();
		_input_reader.Stop();
	}

	bool ExecuteCommandBegin(const char *cd, const char *cmd, bool force_sudo) // return false on failure
	{
		_cce.reset(new VT_ComposeCommandExec(cd, cmd, force_sudo, _start_marker));
		if (!_cce->Created()) {
			const std::string &error_str =
				StrPrintf("Far2l::VT: error %u creating: '%s'\n",
					errno, _cce->ScriptFile().c_str());
			_vta.Write(error_str.c_str(), error_str.size());
			return false;
		}

		if (!_slavename.empty())
			UpdateTerminalSize(_fd_out);
		if (_far2l_exts)
			_far2l_exts->OnTerminalResized();

		std::string cmd_str;
		if (!_slavename.empty()) {
			// Ctrl+U combination to erase any stray characters appeared in input field
			// due to being typed while previous command being executed in interactive shell
			cmd_str+= "\x15";
		}
		// then send sourcing directive (dot) with space trailing to avoid adding it to history
		cmd_str+= " . ";
		if (!_init_user_profile.empty()) {
			_init_user_profile.insert(0, 1, '/');
			_init_user_profile.insert(0, GetMyHome());
			if (TestPath(_init_user_profile).Exists()) {
				cmd_str+= '"';
				cmd_str+= _init_user_profile;
				cmd_str+= "\";. ";
			}
			_init_user_profile.clear();
		}
		cmd_str+= EscapeCmdStr(_cce->ScriptFile());
		cmd_str+= ';';
		cmd_str+= VT_ComposeMarkerCommand(_exit_marker + "$FARVTRESULT");
		cmd_str+= '\n';
		if (!WriteTerm(cmd_str.c_str(), cmd_str.size())) {
			const std::string &error_str =
				StrPrintf("\nFar2l::VT: terminal write error %u\n", errno);
			_vta.Write(error_str.c_str(), error_str.size());
			return false;
		}

		_vta.DisableOutput(); // will enable on start marker arrival

		return true;
	}

	void ExecuteCommandEnd()
	{
		_vta.EnableOutput(); // just in case start marker didnt arrive

		struct stat s;
		if (stat(_cce->ScriptFile().c_str(), &s) == -1) {
			const std::string &error_str =
				StrPrintf("\nFar2l::VT: Script disappeared: '%s'\n",
					_cce->ScriptFile().c_str());
			_vta.Write(error_str.c_str(), error_str.size());

		} else {
			const std::string &pwd = _cce->ResultedWorkingDirectory();
			if (!pwd.empty()) {
				if (sdc_chdir(pwd.c_str()) == -1) {
					StrPrintf("\nFar2l::VT: error %u changing dir to: '%s'\n",
						errno, pwd.c_str());
				}
			}
		}
	}

	public:
	VTShell() : _vta(this), _input_reader(this), _output_reader(this),
		_fd_out(-1), _fd_in(-1), _pipes_fallback_in(-1), _pipes_fallback_out(-1),
		_leader_pid(-1), _keypad(0)
	{
		memset(&_last_window_info_ir, 0, sizeof(_last_window_info_ir));
		if (!Startup())
			return;
	}
	
	virtual ~VTShell()
	{
		fprintf(stderr, "~VTShell this=%p\n", this);
		StopIOReaders();
		Shutdown();
		CheckedCloseFD(_pipes_fallback_in);
		CheckedCloseFD(_pipes_fallback_out);
	}

	virtual HANDLE ConsoleHandle()
	{
		return _console_handle;
	}

	bool ExecuteCommand(const char *cmd, bool force_sudo, bool may_bgnd, bool may_notify)
	{
		ASSERT(!_console_handle);
		_host_id.clear();
		CheckLeaderAlive();
		if (_leader_pid == -1) {
			fprintf(stderr, "%s: no leader\n", __FUNCTION__);
			_exit_code = -1;
			return true;
		}
		_may_notify = may_notify;

		char cd[MAX_PATH + 1] = {'.', 0};
		if (!sdc_getcwd(cd, MAX_PATH)) {
			perror("getcwd");
		}

		VT_ComposeMarker(_start_marker);
		VT_ComposeMarker(_exit_marker);
		_exit_marker+= ':';

		_vta.OnStart();
		if (!ExecuteCommandBegin(cd, cmd, force_sudo)) {
			_exit_code = -1;
		}

		return ExecuteCommandCommonTail(may_bgnd);
	}

	bool ExecuteCommandContinue()
	{
		StopIOReaders();
		VTLog::ConsoleJoined(_console_handle);
		WINPORT(JoinConsole)(_console_handle);
		_console_handle = NULL;
		OnTerminalResized();
		_vta.OnReattached();

		return ExecuteCommandCommonTail(true);
	}

	void PrintNoticeOnPrimaryConsole(const FarLangMsg &m)
	{
		FARString msg(m);
		msg.Insert(0, L'\n');
		msg.Append(L'\n');
		const uint64_t saved_color = GetColor();
		SetFarColor(COL_HELPTOPIC, true);
		DWORD dw;
		WINPORT(WriteConsole)(NULL, msg.CPtr(), msg.GetLength(), &dw, NULL );
		SetColor(saved_color, true);
	}

	bool ExecuteCommandCommonTail(bool may_bgnd)
	{
		_console_kill_requested = false;
		StartIOReaders();
		WAIT_FOR_AND_DISPATCH_INTER_THREAD_CALLS(_output_reader.IsDeactivated() || (_console_switch_requested && may_bgnd));
		StopIOReaders();

		if (_console_switch_requested) {
			_console_switch_requested = false;
			if (may_bgnd) {
				_vta.OnDetached();
				DeliverPendingWindowInfo();
				_console_handle = WINPORT(ForkConsole)();//CommandTerminated
				PrintNoticeOnPrimaryConsole(Msg::CommandBackgrounded);
				StartIOReaders();
				return false;
			}
		}

		ExecuteCommandEnd();

		CheckLeaderAlive();

		OnKeypadChange(0);
		_vta.OnStop();
		_allow_osc_clipset = false;
		_bracketed_paste_expected = false;
		DeliverPendingWindowInfo();
		if (_console_kill_requested) {
			_console_kill_requested = false;
			PrintNoticeOnPrimaryConsole(Msg::CommandTerminated);
		}

		std::lock_guard<std::mutex> lock(_read_state_mutex);
		_far2l_exts.reset();
		_host_id.clear();
		_mouse.reset();
		_mouse_expectations = 0;
		return true;
	}


	int CommandExitCode() const
	{
		return _exit_code;
	}

	bool CheckLeaderAlive(bool wait_exit = false)
	{
		if (_leader_pid != -1) {
			//kill(_leader_pid, SIGKILL);
			int status = 0;
			if (waitpid(_leader_pid, &status, wait_exit ? 0 : WNOHANG) == _leader_pid) {
				_leader_pid = -1;
				fprintf(stderr, "%s: exit status %d\n", __FUNCTION__, status);
			}
		}

		return _leader_pid != -1;
	}

	std::string GetTitle()
	{
		return _vta.GetTitle();
	}

	bool IsDone()
	{
		return _exit_marker.empty();
	}
};

///////////////////////////////////////////////////////////////////////////////////////////////////////

static std::mutex g_vts_mutex;
static std::vector<std::unique_ptr<VTShell> > g_vts;
static std::unique_ptr<VTShell> g_vt;
static std::atomic<int> g_vt_busy{0};

struct VTShell_BusyScope
{
	bool owner;
	VTShell_BusyScope()
	{
		owner = (1 == ++g_vt_busy);
	}
	~VTShell_BusyScope()
	{
		--g_vt_busy;
	}
};

static int VTShell_ExecuteCommonTail(bool completed)
{
	if (!completed) {
		std::lock_guard<std::mutex> lock(g_vts_mutex);
		g_vts.emplace_back(std::move(g_vt));
		g_vt.reset();
		return 0;
	}

	int r = g_vt->CommandExitCode();

	if (!g_vt->CheckLeaderAlive()) {
		g_vt.reset();
	}

	return r;
}

int VTShell_Execute(const char *cmd, bool need_sudo, bool may_bgnd, bool may_notify)
{	
	VTShell_BusyScope vts_bs;
	if (!vts_bs.owner) {
		fprintf(stderr, "%s('%s') - not owner\n", __FUNCTION__, cmd);
		return -1;
	}

	if (g_vt && !g_vt->CheckLeaderAlive()) {
		g_vt.reset();
	}
	if (!g_vt) {
		g_vt.reset(new VTShell);
	}

	return VTShell_ExecuteCommonTail(g_vt->ExecuteCommand(cmd, need_sudo, may_bgnd, may_notify));
}

int VTShell_Switch(size_t index)
{
	VTShell_BusyScope vts_bs;
	if (!vts_bs.owner) {
		fprintf(stderr, "%s(%lu) - not owner\n", __FUNCTION__, (unsigned long)index);
		return -1;
	}

	{
		std::lock_guard<std::mutex> lock(g_vts_mutex);
		if (index >= g_vts.size()) {
			fprintf(stderr, "%s: wrong index: %lu >= %lu\n", __FUNCTION__,
				(unsigned long)index, (unsigned long)g_vts.size());
			return -1;
		}
		g_vt = std::move(g_vts[index]);
		g_vts.erase(g_vts.begin() + index);
	}

	return VTShell_ExecuteCommonTail(g_vt->ExecuteCommandContinue());
}


void VTShell_Shutdown()
{
	std::lock_guard<std::mutex> lock(g_vts_mutex);
	g_vts.clear();
	g_vt.reset();
}

bool VTShell_Busy()
{
	return g_vt_busy != 0;
}

void VTShell_Enum(VTInfos &vts)
{
	std::lock_guard<std::mutex> lock(g_vts_mutex);
	for (const auto &vt : g_vts) {
		vts.emplace_back();
		auto &vti = vts.back();
		vti.con_hnd = vt->ConsoleHandle();
		vti.title = vt->GetTitle();
		vti.done = vt->IsDone();
		vti.exit_code = vt->CommandExitCode();
	}
}

size_t VTShell_Count()
{
	std::lock_guard<std::mutex> lock(g_vts_mutex);
	return g_vts.size();
}
