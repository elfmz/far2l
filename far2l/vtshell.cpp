#include "headers.hpp"
#include "clipboard.hpp"
#include "ctrlobj.hpp"
#include "scrbuf.hpp"
#include "cmdline.hpp"
#include "fileedit.hpp"
#include "fileview.hpp"
#include "interf.hpp"
#include <signal.h>
#include <pthread.h>
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
#include <os_call.hpp>
#include <ScopeHelpers.h>
#include "vtansi.h"
#include "vtlog.h"
#include "VTFar2lExtensios.h"
#include "InterThreadCall.hpp"
#define __USE_BSD 
#include <termios.h> 

const char *VT_TranslateSpecialKey(const WORD key, bool ctrl, bool alt, bool shift, unsigned char keypad = 0,
    WCHAR uc = 0);
void VT_OnFar2lInterract(StackSerializer &stk_ser);

int FarDispatchAnsiApplicationProtocolCommand(const char *str);

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

template <class T> 
class StopAndStart
{
	T &_t;
public:
	StopAndStart(T &t) : _t(t)
	{
		_t.Stop();
	}
	~StopAndStart()
	{
		_t.Start();
	}	
};

class WithThread
{
public:
	WithThread() : _started(false), _thread(0)
	{
	}
	
	virtual ~WithThread()
	{
		assert(!_started);
	}

protected:
	volatile bool  _started;

	bool Start()
	{
		assert(!_started);
		_started = true;
		if (pthread_create(&_thread, NULL, sThreadProc, this)) {
			perror("VT: pthread_create");
			_started = false;
			return false;
		}		
		return true;
	}

	virtual void OnJoin() {}
	void Join()
	{
		if (_started) {
			_started = false;
			OnJoin();
			pthread_join(_thread, NULL);
			_thread = 0;
		}
	}
	
	virtual void *ThreadProc() = 0;

private:
	pthread_t _thread;
	
	static void *sThreadProc(void *p)
	{
		return ((WithThread *)p)->ThreadProc();
	}
};

class VTOutputReader : protected WithThread
{
public:
	struct IProcessor
	{
		virtual bool OnProcessOutput(const char *buf, int len) = 0;
	};
	
	VTOutputReader(IProcessor *processor) 
		: _processor(processor), _fd_out(-1), _deactivated(false)
	{
		_pipe[0] = _pipe[1] = -1;
	}
	
	virtual ~VTOutputReader()
	{
		Stop();
		CheckedCloseFDPair(_pipe);
	}
	
	void Start(int fd_out = -1)
	{
		if (fd_out != -1 ) {
			_fd_out = fd_out;
			InterThreadLock itl;
			_deactivated = false;
		}

		if (_pipe[0] == -1) {
			if (pipe_cloexec(_pipe)==-1) {
				perror("VTOutputReader: pipe_cloexec 1");
				_pipe[0] = _pipe[1] = -1;
				return;
			}
		}

		if (!WithThread::Start()) {
			perror("VTOutputReader::Start");
		}
	}
	
	void Stop()
	{
		if (_started) {
			Join();
			CheckedCloseFDPair(_pipe);
		}
	}

	void WaitDeactivation()
	{
		WAIT_FOR_AND_DISPATCH_INTER_THREAD_CALLS(_deactivated);
	}

	void KickAss()
	{
		char c = 0;
		if (os_call_ssize(write, _pipe[1], (const void *)&c, sizeof(c)) != sizeof(c))
			perror("VTOutputReader::Stop - write");
	}

protected:
	virtual void OnJoin()
	{
		KickAss();
		WithThread::OnJoin();
	}

private:
	IProcessor *_processor;
	int _fd_out, _pipe[2];
	std::mutex _mutex;
	bool _deactivated;
	
	virtual void *ThreadProc()
	{
		char buf[0x1000];
		fd_set rfds;
		
		for (;;) {
			FD_ZERO(&rfds);
			FD_SET(_fd_out, &rfds);
			FD_SET(_pipe[0], &rfds);
			
			int r = os_call_int(select, std::max(_fd_out, _pipe[0]) + 1, &rfds, (fd_set *)nullptr, (fd_set *)nullptr, (timeval *)nullptr);
			if (r <= 0) {
				perror("VTOutputReader select");
				break;
			}
			if (FD_ISSET(_fd_out, &rfds)) {
				r = os_call_ssize(read, _fd_out, (void *)buf, sizeof(buf));
				if (r <= 0) break;
#if 1 //set to 0 to test extremely fragmented output processing 
				if (!_processor->OnProcessOutput(buf, r)) break;
#else 
				for (int i = 0; r > 0;) {
					int n = 1 + (rand()%7);
					if (n > r) n = r;
					if (!_processor->OnProcessOutput(&buf[i], n)) break;
					i+= n;
					r-= n;
				}
				if (r) break;
#endif
			}
			if (FD_ISSET(_pipe[0], &rfds)) {
				r = os_call_ssize(read, _pipe[0], (void *)buf, sizeof(buf));
				if (r < 0) {
					perror("VTOutputReader read pipe[0]");
					break;
				}
			}
			if (!_started)
				return NULL; //stop thread requested
		}

		//thread stopped due to output deactivated
		InterThreadLockAndWake itlw;
		_deactivated = true;
		return NULL;
	}
};

class VTInputReader : protected WithThread
{
public:
	struct IProcessor
	{
		virtual void OnInputMouse(const MOUSE_EVENT_RECORD &MouseEvent) = 0;
		virtual void OnInputKey(const KEY_EVENT_RECORD &KeyEvent) = 0;
		virtual void OnInputResized(const INPUT_RECORD &ir) = 0;
		virtual void OnInputInjected(const std::string &str) = 0;
		virtual void OnRequestShutdown() = 0;
	};

	VTInputReader(IProcessor *processor) : _stop(false), _processor(processor)
	{
	}
	
	void Start()
	{
		if (!_started) {
			_stop = false;
			WithThread::Start();
		}
	}

	void Stop()
	{
		if (_started) {
			_stop = true;
			Join();
		}
	}

	void InjectInput(const char *str, size_t len)
	{
		{
			std::unique_lock<std::mutex> locker(_pending_injected_inputs_mutex);
			_pending_injected_inputs.emplace_back(str, len);
		}
		KickInputThread();
	}

protected:
	virtual void OnJoin()
	{
		KickInputThread();
		WithThread::OnJoin();
	}

private:
	std::atomic<bool> _stop;
	IProcessor *_processor;
	std::list<std::string> _pending_injected_inputs;
	std::mutex _pending_injected_inputs_mutex;

	void KickInputThread()
	{
		// write some dummy console input to kick pending ReadConsoleInput
		INPUT_RECORD ir = {};
		ir.EventType = NOOP_EVENT;
		DWORD dw = 0;
		WINPORT(WriteConsoleInput)(0, &ir, 1, &dw);
	}

	virtual void *ThreadProc()
	{
		std::list<std::string> pending_injected_inputs;
		while (!_stop) {
			INPUT_RECORD ir = {0};
			DWORD dw = 0;
			if (!WINPORT(ReadConsoleInput)(0, &ir, 1, &dw)) {
				perror("VT: ReadConsoleInput");
				usleep(100000);
			} else if (ir.EventType == MOUSE_EVENT) {
				_processor->OnInputMouse(ir.Event.MouseEvent);

			} else if (ir.EventType == KEY_EVENT) {
				_processor->OnInputKey(ir.Event.KeyEvent);

			} else if (ir.EventType == WINDOW_BUFFER_SIZE_EVENT) {
				_processor->OnInputResized(ir);
			}
			{
				std::unique_lock<std::mutex> locker(_pending_injected_inputs_mutex);
				pending_injected_inputs.swap(_pending_injected_inputs);
			}
			for(const auto &pri : pending_injected_inputs) {
				_processor->OnInputInjected(pri);
			}

			pending_injected_inputs.clear();

			if (CloseFAR) {
				_processor->OnRequestShutdown();
				break;
			}
		}

		return nullptr;
	}
};

class Marker
{
	bool _active;

protected:
	std::string _marker;
	size_t _marker_start;
	std::string _backlog;

	void ScanReset()
	{
		_active = false;
		_marker_start = (size_t) -1;
		_backlog.clear();
	}

	std::string GetMarker()
	{
		if (_marker.empty())
			Reset();
		return _marker;
	}

public:
	Marker() : _active(false), _marker_start((size_t) -1)
	{
		srand(time(NULL));
	}

	std::string EscapedOutput()
	{
		std::string out;
		char escaped[16];
		for (const auto c : GetMarker()) {
			sprintf(escaped, "\\x%02x",
					(unsigned int) (unsigned char) c);
			out += escaped;
		}
		return out;
	}

	virtual std::string EchoCommand() = 0;

	virtual void Reset()
	{
		_marker.clear();
		for (size_t l = 8 + (rand() % 9); l; --l) {
			char c;
			switch (rand() % 3) {
				case 0:
					c = 'A' + (rand() % ('Z' + 1 - 'A'));
					break;
				case 1:
					c = 'a' + (rand() % ('z' + 1 - 'a'));
					break;
				case 2:
					c = '0' + (rand() % ('9' + 1 - '0'));
					break;
			}
			_marker += c;
		}
		ScanReset();
	}

	virtual void OnScanStart()
	{};

	virtual void OnCharScan(const char)
	{};

	virtual void OnMarkerFound()
	{};

	virtual void OnMarkerScanStart()
	{
		_marker_start = _backlog.size();
	}

	int Scan(const char *buf, int len)
	{
		for (int i = 0; i < len; ++i) {
			if (buf[i] == '=') {
				ScanReset();
				_active = true;
				OnScanStart();
			} else if (_active) {
				_backlog += buf[i];
				OnCharScan(buf[i]);
				if (_marker_start == (size_t) -1) {
					// check for reasonable length limit
					if (_backlog.size() > 12 + _marker.size()) {
						ScanReset();
					}
				} else if (_backlog.size() - _marker_start >= _marker.size()) {
					std::string marker = GetMarker();
					if (memcmp(_backlog.c_str() + _marker_start, marker.c_str(), marker.size()) == 0) {
						_backlog[_marker_start - 1] = 0;
						OnMarkerFound();
						ScanReset();
						return i + 1;
					}
					ScanReset();
				}
			}
		}
		return -1;
	}
};

class StartingMarker : public Marker
{
public:
	std::string EchoCommand() override
	{
		std::string out = "echo -ne \"=\"$\'";
		out += EscapedOutput();
		out += '\'';
		return out;
	}

	void OnScanStart() override
	{
		OnMarkerScanStart();
	}
};

class CompletionMarker : public Marker
{
	int _exit_code = -1;

public:
	std::string EchoCommand() override
	{
		std::string out = "echo -ne \"=$FARVTRESULT:\"$\'";
		out += EscapedOutput();
		out += '\'';
		return out;
	}

	int LastExitCode() const
	{
		return _exit_code;
	}

	void Reset() override
	{
		Marker::Reset();
		_marker += "\x1b[1K\x0d";
	}

	void OnCharScan(const char c) override
	{
		if (c == ':') {
			if (_marker_start == (size_t) -1)
				OnMarkerScanStart();
			else
				// second splitter??
				ScanReset();
		}
	}

	void OnMarkerFound() override
	{
		_exit_code = atoi(&_backlog[0]);
	}
};
	
class VTShell : VTOutputReader::IProcessor, VTInputReader::IProcessor, IVTShell
{
	VTAnsi _vta;
	VTInputReader _input_reader;
	VTOutputReader _output_reader;
	std::mutex _inout_control_mutex;
	int _fd_out, _fd_in;
	int _pipes_fallback_in, _pipes_fallback_out;
	pid_t _shell_pid, _forked_proc_pid;
	std::string _slavename;
	CompletionMarker _completion_marker;
	StartingMarker _starting_marker;
	bool _seeking_start;
	bool _seeking_end;
	std::atomic<unsigned char> _keypad;
	INPUT_RECORD _last_window_info_ir;
	VTFar2lExtensios *_far2l_exts = nullptr;
	std::mutex _far2l_exts_mutex, _write_term_mutex;
	
	
	int ForkAndAttachToSlave(bool shell)
	{
		int r = fork();
		if (r != 0)
			return r;

		signal(SIGHUP, SIG_DFL);
		signal(SIGINT, SIG_DFL);
		signal(SIGQUIT, SIG_DFL);
		signal(SIGTERM, SIG_DFL);
		signal(SIGPIPE, SIG_DFL);
		signal(SIGCHLD, SIG_DFL);
		signal(SIGSTOP, SIG_DFL);

		if (shell) {
			if (setsid()==-1)
				perror("VT: setsid");
		}
			
		if (!_slavename.empty()) {
			r = open(_slavename.c_str(), O_RDWR); 
			if (r==-1) {
				perror("VT: open slave");
				_exit(errno);
				exit(errno);
			}
				
			if ( ioctl( r, TIOCSCTTY, 0 )==-1 )
				perror( "VT: ioctl(TIOCSCTTY)" );
					
			dup2(r, STDIN_FILENO);
			dup2(r, STDOUT_FILENO);
			dup2(r, STDERR_FILENO);
			CheckedCloseFD(r);
		} else {
			dup2(_pipes_fallback_in, STDIN_FILENO);
			dup2(_pipes_fallback_out, STDOUT_FILENO);
			dup2(_pipes_fallback_out, STDERR_FILENO);
			CheckedCloseFD(_pipes_fallback_in);
			CheckedCloseFD(_pipes_fallback_out);
		}
			
		//setenv("TERM", "xterm-256color", 1);
		setenv("TERM", "xterm", 1);
		signal( SIGINT, SIG_DFL );
		return 0;
	}
	
	void UpdateTerminalSize(int fd_term)
	{
		CONSOLE_SCREEN_BUFFER_INFO csbi = { };
		if (WINPORT(GetConsoleScreenBufferInfo)( NULL, &csbi )  
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
			fcntl(fd_term, F_SETFD, FD_CLOEXEC);
			
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
			fcntl(fd_in[1], F_SETFD, FD_CLOEXEC);
			fcntl(fd_out[0], F_SETFD, FD_CLOEXEC);
			
			_pipes_fallback_in = fd_in[0];
			_pipes_fallback_out = fd_out[1];
			_fd_in = fd_in[1];
			_fd_out = fd_out[0];

		} else {
			_pipes_fallback_in = _pipes_fallback_out = -1;
			struct termios ts = {};
			if (tcgetattr(fd_term, &ts) == 0) {
				ts.c_lflag |= ISIG | ICANON | ECHO;
				//ts.c_lflag&= ~ECHO;
				ts.c_cc[VINTR] = 3;
				tcsetattr( fd_term, TCSAFLUSH, &ts );
			}
			_fd_in = fd_term;
			_fd_out = dup(fd_term);
			fcntl(_fd_out, F_SETFD, FD_CLOEXEC);
		}

		return true;
	}

	void RunShell()
	{
		//CheckedCloseFD(fd_term);
			
		const char *shell = getenv("SHELL");
		if (!shell)
			shell = "/bin/sh";

		// avoid using fish for a while, it requites changes in Opt.strQuotedSymbols and some others
		const char *slash = strrchr(shell, '/');
		if (strcmp(slash ? slash + 1 : shell, "fish")==0)
			shell = "/bin/bash";

		//shell = "/usr/bin/zsh";
		//shell = "/bin/bash";
		//shell = "/bin/sh";
		std::vector<const char *> args;
		args.push_back(shell);
#if 0		
		const char *home = getenv("HOME");
		if (!home) home = "/root";
		
		//following hussle is to set PS='' from the very beginning of shell
		//to avoid undesirable prompt to be printed
		std::string rc_src, rc_dst, rc_arg;
		if (strstr(shell, "/bash")) {
			rc_src = std::string(home) + "/.bashrc";
			rc_dst = InMyTemp("vtcmd/init.bash");
			rc_arg = "--rcfile";
		}/* else if (strstr(shell, "/zsh")) {
			rc_src = std::string(home) + "/.zshrc";
			rc_dst = InMyTemp("vtcmd/init.zsh");
			rc_arg = "--rcs";
		}*/
		
		if (!rc_arg.empty()) {
			std::ofstream dst_stream(rc_dst, std::ios::binary);
			if (dst_stream.is_open()) {
				std::ifstream src_stream(rc_src, std::ios::binary);
				if (src_stream.is_open()) {
					dst_stream << src_stream.rdbuf();
				}
				dst_stream << "\nPS1=''\n";
				args.push_back(rc_arg.c_str());
				args.push_back(rc_dst.c_str());
			}
		}
#endif
		args.push_back("-i");
		args.push_back(nullptr);
		
		//FIXME: not fair - removing const 
		execv(shell, (char **)&args[0]);
	}
	
	bool Startup()
	{
		if (!InitTerminal())
			return false;
		
		int r = ForkAndAttachToSlave(true);
		if (r == 0) {
			RunShell();
			fprintf(stderr, "VT: RunShell returned, errno %u\n", errno);
			_exit(errno);
			exit(errno);
		}
	
		if (r==-1) { 
			perror("VT: fork");
			return false;
		}
		_shell_pid = r;
		usleep(300000);//give it time to initialize, otherwise additional command copy will be echoed
		return true;
	}
	

	virtual bool OnProcessOutput(const char *buf, int len) //called from worker thread
	{
		DbgPrintEscaped("OUTPUT", buf, len);

		if (_seeking_start) {
		    int pos = _starting_marker.Scan(buf, len);
		    if (pos >= 0) {
		        buf += pos;
		        len -= pos;
		        _vta.Write(buf, len);
		        _seeking_start = false;
		        _seeking_end = true;
		    }
		}

		if (_seeking_end) {
            //_completion_marker is not thread safe generically,
            //but while OnProcessOutput called from single thread .
            //and can't overlap with ScanReset() - its ok.
            //But if it will be called from several threads - this
            //calls must be guarded by mutex.
            bool out = _completion_marker.Scan(buf, len) < 0;
            _vta.Write(buf, len);
            return out;
        }

		return true;
	}
	
	virtual void OnTerminalResized()
	{
		if (!_slavename.empty())
			UpdateTerminalSize(_fd_out);
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
			std::lock_guard<std::mutex> lock(_far2l_exts_mutex);
			if (_far2l_exts && _far2l_exts->OnInputMouse(MouseEvent))
				return;
		}

		if (MouseEvent.dwEventFlags & MOUSE_WHEELED) {
			if (HIWORD(MouseEvent.dwButtonState) > 0) {
				OnConsoleLog(CLK_VIEW_AUTOCLOSE);
			}
		} else if ( (MouseEvent.dwButtonState&FROM_LEFT_1ST_BUTTON_PRESSED) != 0 &&
			(MouseEvent.dwEventFlags & (MOUSE_HWHEELED|MOUSE_MOVED|DOUBLE_CLICK)) == 0 ) {
			WINPORT(BeginConsoleAdhocQuickEdit)();
		}
	}


	bool WriteTerm(const char *str, size_t len)
	{
		if (len == 0)
			return true;

		std::lock_guard<std::mutex> lock(_write_term_mutex);
		while (len) {
			ssize_t written = os_call_ssize(write, _fd_in, (const void *)str, len);
			if (written <= 0) {
				perror("WriteTerm - write");
				return false;
			}
			len-= written;
			str+= written;
		}

		return true;
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
			std::lock_guard<std::mutex> lock(_far2l_exts_mutex);
			if (_far2l_exts && _far2l_exts->OnInputKey(KeyEvent))
				return;
		}

		if (!KeyEvent.bKeyDown)
			return;

		DWORD dw;
		const std::string &translated = TranslateKeyEvent(KeyEvent);
		if (!translated.empty()) {
			if (_slavename.empty() && KeyEvent.uChar.UnicodeChar) {//pipes fallback
				WINPORT(WriteConsole)( NULL, &KeyEvent.uChar.UnicodeChar, 1, &dw, NULL );
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
			fprintf(stderr, "VT: Ctrl+Alt+C - killing them hardly...\n");
			SendSignalToShell(SIGKILL);
			if (_forked_proc_pid!=-1)
				kill(_forked_proc_pid, SIGKILL);
			
		} else {
			if (_slavename.empty()) //pipes fallback
				SendSignalToShell(SIGINT);
			if (_forked_proc_pid!=-1)
				kill(_forked_proc_pid, SIGINT);
		}
	}

	enum ConsoleLogKind
	{
		CLK_EDIT,
		CLK_VIEW,
		CLK_VIEW_AUTOCLOSE
	};

	static int sShowConsoleLog(ConsoleLogKind kind, const std::string &histfile)
	{
		if (!CtrlObject || !CtrlObject->CmdLine)
			return 0;

		ScrBuf.FillBuf();
		CtrlObject->CmdLine->SaveBackground();

		SetFarConsoleMode(TRUE);
		if (kind == CLK_EDIT)
			ModalEditTempFile(histfile, true);
		else
			ModalViewTempFile(histfile, true, kind == CLK_VIEW_AUTOCLOSE);

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
		
		const std::string &histfile = VTLog::GetAsFile();
		if (histfile.empty())
			return;

		DeliverPendingWindowInfo();
		InterThreadCall<int>(std::bind(sShowConsoleLog, kind, histfile));

		if (!_slavename.empty())
			UpdateTerminalSize(_fd_out);
	}

	virtual void OnKeypadChange(unsigned char keypad)
	{
//		fprintf(stderr, "VTShell::OnKeypadChange: %u\n", keypad);
		_keypad = keypad;
	}

	virtual void OnApplicationProtocolCommand(const char *str)//NB: called not from main thread!
	{
		if (strncmp(str, "far2l", 5) == 0) {
			std::string reply;
			switch (str[5]) {
				case '1': {
					std::lock_guard<std::mutex> lock(_far2l_exts_mutex);
					if (!_far2l_exts)
						_far2l_exts = new VTFar2lExtensios(this);

					reply = "\x1b_far2lok\x07";
				} break;

				case '0': {
					std::lock_guard<std::mutex> lock(_far2l_exts_mutex);
					delete _far2l_exts;
					_far2l_exts = nullptr;
				} break;

				case ':': {
					std::lock_guard<std::mutex> lock(_far2l_exts_mutex);
					if (str[6] && _far2l_exts) {
						StackSerializer stk_ser;
						uint8_t id = 0;
						try {
							stk_ser.FromBase64(str + 6, strlen(str + 6));
							id = stk_ser.PopU8();
							_far2l_exts->OnInterract(stk_ser);

						} catch (std::exception &) {
							stk_ser.Clear();
						}

						if (id) try {
							stk_ser.PushPOD(id);
							reply = "\x1b_far2l";
							reply+= stk_ser.ToBase64();
							reply+= '\x07';

						} catch (std::exception &) {
							reply.clear();
						}
					}

				} break;
			}
			if (!reply.empty())
				_input_reader.InjectInput(reply.c_str(), reply.size());
		}
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
			xf_free(wz);
		}

		return out;
	}
	
	std::string TranslateKeyEvent(const KEY_EVENT_RECORD &KeyEvent)
	{
		if (KeyEvent.wVirtualKeyCode) {
			const bool ctrl = (KeyEvent.dwControlKeyState & (LEFT_CTRL_PRESSED|RIGHT_CTRL_PRESSED)) != 0;
			const bool alt = (KeyEvent.dwControlKeyState & (RIGHT_ALT_PRESSED|LEFT_ALT_PRESSED)) != 0;
			const bool shift = (KeyEvent.dwControlKeyState & (SHIFT_PRESSED)) != 0;
			
			if (!ctrl && !shift && !alt && KeyEvent.wVirtualKeyCode==VK_BACK) {
				//WCM has a setting for that, so probably in some cases backspace should be returned as is
				char backspace[] = {127, 0};
				return backspace;
			}
			
			if ((ctrl && shift && !alt && KeyEvent.wVirtualKeyCode=='V') ||
			    (!ctrl && shift && !alt && KeyEvent.wVirtualKeyCode==VK_INSERT) ){
				return StringFromClipboard();
			}
			
			if (ctrl && !shift && KeyEvent.wVirtualKeyCode=='C') {
				OnCtrlC(alt);
			} 
			
			if (ctrl && !shift && alt && KeyEvent.wVirtualKeyCode=='Z') {
				WINPORT(ConsoleBackgroundMode)(TRUE);
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
	
	void SendSignalToShell(int sig)
	{
		pid_t grp = getpgid(_shell_pid);

		if (grp != -1 && grp != getpgid(getpid())) {
			killpg(grp, sig);
			// kill(_shell_pid, sig);
		} else
			kill(_shell_pid, sig);
	}
	
	virtual void OnRequestShutdown()
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



	void Shutdown()
	{
		OnRequestShutdown();

		if (_shell_pid!=-1) {
			//kill(_shell_pid, SIGKILL);
			int status;
			waitpid(_shell_pid, &status, 0);
			_shell_pid = -1;
		}
	}

	void DeliverPendingWindowInfo()
	{
		if (_last_window_info_ir.EventType == WINDOW_BUFFER_SIZE_EVENT) {
			DWORD dw = 0;
			WINPORT(WriteConsoleInput)(NULL, &_last_window_info_ir, 1, &dw);
			_last_window_info_ir.EventType = 0;
		}
	}
	
	
	std::string GenerateExecuteCommandScript(const char *cd, const char *cmd, bool need_sudo)
	{
		char name[128]; 
		sprintf(name, "vtcmd/%x_%p", getpid(), this);
		std::string cmd_script = InMyTemp(name);
		std::string pwd_file = cmd_script + ".pwd";
		FILE *f = fopen(cmd_script.c_str(), "wb");
		if (!f)
			return std::string();

static bool shown_tip_init = false;
static bool shown_tip_exit = false;
				
		if (!need_sudo) {
			need_sudo = (chdir(cd)==-1 && (errno==EACCES || errno==EPERM));
		}
		fprintf(f, "%s\n", _starting_marker.EchoCommand().c_str());
		fprintf(f, "trap \"echo ''\" SIGINT\n");//we need marker to be printed even after Ctrl+C pressed
		fprintf(f, "PS1=''\n");//reduce risk of glitches
		//fprintf(f, "stty echo\n");
		if (strcmp(cmd, "exit")==0) {
			fprintf(f, "echo \"Closing back shell.%s\"\n", 
				shown_tip_exit ? "" : " TIP: To close FAR - type 'exit far'.");
			shown_tip_exit = true;

		} else if (!shown_tip_init) {
			if (!Opt.OnlyEditorViewerUsed) {
				fprintf(f, "echo -ne \"\x1b_push-attr\x07\x1b[36m\"\n");
				fprintf(f, "echo \"While typing command with panels off:\"\n");
				fprintf(f, "echo \" Double Shift+TAB - bash-guided autocomplete.\"\n");
				fprintf(f, "echo \" F3, F4, F8 - viewer/editor/clear console log.\"\n");
				fprintf(f, "echo \" Ctrl+Shift+MouseScrollUp - open autoclosing viewer with console log.\"\n");
				fprintf(f, "echo \"While executing command:\"\n");
				fprintf(f, "echo \" Ctrl+Shift+F3/+F4 - pause and open viewer/editor with console log.\"\n");
				fprintf(f, "echo \" Ctrl+Alt+C - terminate everything in this shell.\"\n");
				if (WINPORT(ConsoleBackgroundMode)(FALSE)) {
					fprintf(f, "echo \" Ctrl+Alt+Z - detach FAR manager application to background.\"\n");
				}
				fprintf(f, "echo \" MouseScrollUp - pause and open autoclosing viewer with console log.\"\n");
				fprintf(f, "echo ════════════════════════════════════════════════════════════════════\x1b_pop-attr\x07\n");
				shown_tip_init = true;
			}
		}
		if (need_sudo) {
			fprintf(f, "sudo sh -c \"cd \\\"%s\\\" && %s && pwd >'%s'\"\n",
				EscapeEscapes(EscapeCmdStr(cd)).c_str(), EscapeCmdStr(cmd).c_str(), pwd_file.c_str());
		} else {
			fprintf(f, "cd \"%s\" && %s && pwd >'%s'\n", EscapeCmdStr(cd).c_str(), cmd, pwd_file.c_str());
		}

		fprintf(f, "FARVTRESULT=$?\n");//it will be echoed to caller from outside
		fprintf(f, "cd ~\n");//avoid locking arbitrary directory
		fprintf(f, "if [ $FARVTRESULT -eq 0 ]; then\n");
		fprintf(f, "echo \"\x1b_push-attr\x07\x1b_set-blank=-\x07\x1b[32m\x1b[K\x1b_pop-attr\x07\"\n");
		fprintf(f, "else\n");
		fprintf(f, "echo \"\x1b_push-attr\x07\x1b_set-blank=~\x07\x1b[33m\x1b[K\x1b_pop-attr\x07\"\n");
		fprintf(f, "fi\n");
		fclose(f);
		return cmd_script;
	}


	std::string ComposeExecuteCommandInitialTitle(const char *cd, const char *cmd, bool using_sudo)
	{
		std::string title = cmd;
		StrTrim(title);
		if (StrStartsFrom(title, "sudo ")) {
			using_sudo = true;
			title.erase(0, 5);
			StrTrim(title);
		}

		if (title.size() > 2 && (title[0] == '\'' || title[0] == '\"')) {
			size_t p = title.find(title[0], 1);
			if (p != std::string::npos) {
				title = title.substr(1, p - 1);
			}

		} else {
			size_t p = title.find(' ');
			if (p != std::string::npos) {
				title.resize(p);
			}
		}

		size_t p = title.rfind('/');
		if (p!=std::string::npos) {
			title.erase(0, p + 1);
		}
		title+= '@';
		title+= cd;
		if (using_sudo) {
			title.insert(0, "sudo ");
		}
		return title;
	}

	public:
	VTShell() : _vta(this), _input_reader(this), _output_reader(this),
        _fd_out(-1), _fd_in(-1), _pipes_fallback_in(-1), _pipes_fallback_out(-1),
        _shell_pid(-1), _forked_proc_pid(-1), _seeking_start(false), _seeking_end(false), _keypad(0)
	{
		memset(&_last_window_info_ir, 0, sizeof(_last_window_info_ir));
		if (!Startup())
			return;
	}
	
	~VTShell()
	{
		fprintf(stderr, "~VTShell\n");
		Shutdown();
		CheckedCloseFD(_pipes_fallback_in);
		CheckedCloseFD(_pipes_fallback_out);
	}

	
	int ExecuteCommand(const char *cmd, bool force_sudo)
	{
		if (_shell_pid==-1)
			return -1;

		char cd[MAX_PATH + 1] = {'.', 0};
		if (!sdc_getcwd(cd, MAX_PATH)) {
			perror("getcwd");
		}

		const std::string &cmd_script = GenerateExecuteCommandScript(cd, cmd, force_sudo);
		if (cmd_script.empty())
			return -1;

		std::string pwd_file = cmd_script + ".pwd";
		unlink(pwd_file.c_str());

		if (!_slavename.empty())
			UpdateTerminalSize(_fd_out);
		
		std::string cmd_str = " . "; //space in beginning of command prevents adding it to history
		cmd_str+= EscapeCmdStr(cmd_script);
		cmd_str+= ';';
		cmd_str+= _completion_marker.EchoCommand();
		cmd_str+= '\n';

		if (!WriteTerm(cmd_str.c_str(), cmd_str.size())) {
			fprintf(stderr, "VT: write error %d\n", errno);
			return -1;
		}

        _seeking_start = true;
		_seeking_end = false;

		const std::string &title = ComposeExecuteCommandInitialTitle(cd, cmd, force_sudo);
		_vta.OnStart(title.c_str());

		{
			std::lock_guard<std::mutex> lock(_inout_control_mutex);
			_output_reader.Start(_fd_out);
			_input_reader.Start();
		}
		
		_output_reader.WaitDeactivation();

		int fd = open(pwd_file.c_str(), O_RDONLY);
		if (fd != -1) {
			char buf[PATH_MAX + 1] = {};
			ReadAll(fd, buf, sizeof(buf) - 1);
			CheckedCloseFD(fd);
			size_t len = strlen(buf);
			if (len > 0 && buf[len - 1] == '\n') {
				buf[--len] = 0;
			}
			if (len > 0) {
				sdc_chdir(buf);
			}
			unlink(pwd_file.c_str());
		}

		if (_shell_pid!=-1) {
			int status;
			if (waitpid(_shell_pid, &status, WNOHANG)==_shell_pid) {
				_shell_pid = -1;
			}
		}

		{
			std::lock_guard<std::mutex> lock(_inout_control_mutex);
			_input_reader.Stop();
			_output_reader.Stop();
		}

		remove(cmd_script.c_str());

		OnKeypadChange(0);
		_completion_marker.Reset();
		_vta.OnStop();
		DeliverPendingWindowInfo();

		std::lock_guard<std::mutex> lock(_far2l_exts_mutex);
		delete _far2l_exts;
		_far2l_exts = nullptr;

		return _completion_marker.LastExitCode();
	}	

	bool IsOK()
	{
		return _shell_pid!=-1;
	}

};

static std::unique_ptr<VTShell> g_vts;
static std::mutex g_vts_mutex;

int VTShell_Execute(const char *cmd, bool need_sudo) 
{	
	std::lock_guard<std::mutex> lock(g_vts_mutex);
	if (!g_vts)
		g_vts.reset(new VTShell);

	int r = g_vts->ExecuteCommand(cmd, need_sudo);

	if (!g_vts->IsOK()) {
		fprintf(stderr, "Shell exited\n");
		g_vts.reset();
	}

	return r;
}

void VTShell_Shutdown()
{
	std::lock_guard<std::mutex> lock(g_vts_mutex);
	g_vts.reset();
}
