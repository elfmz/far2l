#include "headers.hpp"
#include "clipboard.hpp"
#include <signal.h>
#include <pthread.h>
#include <mutex>
#include <atomic>
#include <fcntl.h>
#include <iostream>
#include <fstream>
#include <sys/ioctl.h> 
#include <sys/wait.h> 
#include <condition_variable>
#include "vtansi.h"
#define __USE_BSD 
#include <termios.h> 

void ExecuteOrForkProc(const char *CmdStr, int (WINAPI *ForkProc)(int argc, char *argv[]) ) ;
const char *VT_TranslateSpecialKey(const WORD key, bool ctrl, bool alt, bool shift, char keypad = 0);


#if 0 //change to 1 to enable verbose I/O reports to stderr
static void DbgPrintEscaped(const char *info, const std::string &s)
{
	std::string msg;
	for (auto c : s) {
		if (c=='\\') {
			msg+= "\\\\";
		} else if (c <= 32 || c > 127) {
			char zz[64]; sprintf(zz, "\\%02x", (unsigned int)c);
			msg+= zz;
		} else 
			msg+= (char)(unsigned char)c;
	}
	fprintf(stderr, "VT %s: '%s'\n", info, msg.c_str());
}
#else
# define DbgPrintEscaped(info, s)
#endif

	

class VTOutputReader
{
public:
	struct IProcessor
	{
		virtual bool OnProcessOutput(const char *buf, int len) = 0;
		virtual void OnStoppedOutput() = 0;
	};
	
	VTOutputReader(IProcessor *processor, int fd_out) : _processor(processor),
		_thread(0), _fd_out(fd_out), _thread_exited(false), _started(false)
	{
		if (pipe2(_pipe,  O_CLOEXEC)==-1) {
			perror("VTOutputReader: pipe2");
			return;
		} 

		_thread_exited = false;
		_started = true;
		if (pthread_create(&_thread, NULL, sOutputReaderThread, this)) {
			perror("VTOutputReader: pthread_create");
			CheckedCloseFDPair(_pipe);
			_started = false;
			return;
		}
	}
	
	virtual ~VTOutputReader()
	{
		if (_started) {
			_started = false;
			char c = 0;
			if (write(_pipe[1], &c, sizeof(c)) != sizeof(c))
				perror("VTOutputReader: write");
			
			pthread_join(_thread, NULL);
			CheckedCloseFDPair(_pipe);
			_thread = 0;
		}		
	}

	bool IsActive()
	{
		return (_started && !_thread_exited);
	}
	
private:
	IProcessor *_processor;
	pthread_t _thread;
	int _fd_out, _pipe[2];
	std::atomic<bool> _thread_exited;
	bool  _started;


	static void *sOutputReaderThread(void *p)
	{
		return ((VTOutputReader *)p)->OutputReaderThread();
	}
	

	void *OutputReaderThread()
	{
		char buf[0x1000];
		fd_set rfds;
		
		for (;;) {
			FD_ZERO(&rfds);
			FD_SET(_fd_out, &rfds);
			FD_SET(_pipe[0], &rfds);
			
			int r = select(std::max(_fd_out, _pipe[0]) + 1, &rfds, NULL, NULL, NULL);
			
			if (FD_ISSET(_fd_out, &rfds)) {
				r = read(_fd_out, buf, sizeof(buf));
				if (r <= 0) break;
				if (!_processor->OnProcessOutput(buf, r)) break;
			} else {
				if (FD_ISSET(_pipe[0], &rfds)) {
					r = read(_pipe[0], buf, sizeof(buf));
					if (r <= 0) break;
				}
				if (!_started) break;
			}
		}
		
		
		_thread_exited = true;
		_processor->OnStoppedOutput();
		return NULL;
	}
};

class CompletionMarker 
{
	std::string _marker;
	std::string _backlog;
	size_t _marker_start;
	int _exit_code;
	bool _active;
	
public:
	CompletionMarker() : _exit_code(-1)
	{
		ScanReset();
		srand(time(NULL));
		for (size_t l = 8 + (rand()%9); l; --l) {
			char c;
			switch (rand() % 3) {
				case 0: c = 'A' + (rand() % ('Z' + 1 - 'A')); break;
				case 1: c = 'a' + (rand() % ('z' + 1 - 'a')); break;
				case 2: c = '0' + (rand() % ('9' + 1 - '0')); break;
			}
			_marker+= c;
		}
		_marker+= "\x1b[1K";
		//setenv("FARVTMARKER", _marker.c_str(), 1);
		fprintf(stderr, "CompletionMarker: '%s'\n", _marker.c_str());
	}
		
	std::string SetEnvCommand() const
	{
		std::string out = "export FARVTMARKER=";
		out+= _marker;
		return out;
	}
	
	std::string EchoCommand() const
	{
		return "echo -ne \"=$FARVTRESULT:$FARVTMARKER\"";
	}
	
	int LastExitCode() const
	{
		return _exit_code;
	}
		
	void ScanReset()
	{
		_active = false;
		_marker_start = (size_t)-1;
		_backlog.clear();
	}

	bool Scan(const char *buf, int len)
	{
		for (int i = 0; i < len; ++i) {
			if (buf[i]=='=') {
				ScanReset();
				_active = true;
			} else if (_active) {
				_backlog+= buf[i];
				if (_marker_start==(size_t)-1) {
					//check for reasonable length limit
					if (_backlog.size() > 12 + _marker.size()) { 
						ScanReset();
					} else if (buf[i]==':')
						_marker_start = _backlog.size();

				} else if (buf[i]==':') {//second splitter??
					ScanReset();
				} else if (_backlog.size() - _marker_start >= _marker.size())  {
					if (memcmp(_backlog.c_str() + _marker_start, _marker.c_str(), _marker.size())==0) {
						_backlog[_marker_start - 1] = 0;
						_exit_code = atoi(&_backlog[0]);
						ScanReset();
						return true;
					}
					ScanReset();
				}
			}
		}
		return false;
	}
};
	
class VTShell : VTOutputReader::IProcessor
{
	VTAnsi _vta;
	int _fd_out, _fd_in;
	int _pipes_fallback_in, _pipes_fallback_out;
	pid_t _pid;
	std::string _slavename;
	CompletionMarker _completion_marker;
	bool _skipping_line;
	
	
	int ForkAndAttachToSlave(bool shell)
	{
		int r = fork();
		if (r!=0)
			return r;
			
		if (!_slavename.empty()) {
			if (shell) {
				if (setsid()==-1)
					perror("VT: setsid");
			}
				
			r = open(_slavename.c_str(), O_RDWR); 
			if (r==-1) {
				perror("VT: open slave");
				_exit(errno);
				exit(errno);
			}
				
			if ( ioctl( 0, TIOCSCTTY, 0 )==-1 )
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
			struct termios ts = {0};
			if (tcgetattr(fd_term, &ts)==0) {
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
		// avoid using fish for a while, it requites changes in Opt.strQuotedSymbols
		if (strcmp(shell, "/usr/bin/fish")==0)
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
			rc_dst = InMyProfile("vtcmd/init.bash");
			rc_arg = "--rcfile";
		}/* else if (strstr(shell, "/zsh")) {
			rc_src = std::string(home) + "/.zshrc";
			rc_dst = InMyProfile("vtcmd/init.zsh");
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
			fprintf(stderr, "ExecuteOrForkProc: RunShell returned, errno %u\n", errno);
			_exit(errno);
			exit(errno);
		}
	
		if (r==-1) { 
			perror("VT: fork");
			return false;
		}
		usleep(300000);//give it time to initialize, otherwise additional command copy will be echoed
		_pid = r;		
		return true;
	}
	

	virtual bool OnProcessOutput(const char *buf, int len) 
	{
		std::string s(buf, len);
		DbgPrintEscaped("OUTPUT", s);
		while (_skipping_line) {
			if (s.empty()) break;
			if (s[0]=='\n') _skipping_line = false;
			s.erase(0, 1);
		}
		if (!s.empty()) {
			const std::wstring &ws = MB2Wide(s.c_str());
			_vta.Write(ws.c_str(), ws.size());
		}
		
		//_completion_marker is not thread safe generically,
		//but while OnProcessOutput called from single thread .
		//and can't overlap with ScanReset() - its ok.
		//But if it will be called from several threads - this 
		//calls must be guarded by mutex.
		if (_completion_marker.Scan(buf, len)) {
			return false;
		}
		
		return true;
	}
	
	virtual void OnStoppedOutput()
	{
		//write some console input to kick pending ReadConsoleInput
		INPUT_RECORD ir = {0};
		ir.EventType = FOCUS_EVENT;
		ir.Event.FocusEvent.bSetFocus = TRUE;
		DWORD dw = 0;
		WINPORT(WriteConsoleInput)(0, &ir, 1, &dw);
		fprintf(stderr, "OnStoppedOutput\n");
	}


	void OnKeyDown(KEY_EVENT_RECORD &KeyEvent)
	{
		DWORD dw;
		const std::string &translated = TranslateKeyEvent(KeyEvent);
		if (!translated.empty()) {
			if (_slavename.empty() && KeyEvent.uChar.UnicodeChar) {//pipes fallback
				WINPORT(WriteConsole)( NULL, &KeyEvent.uChar.UnicodeChar, 1, &dw, NULL );
			}
			DbgPrintEscaped("INPUT", translated.c_str());
			if (write(_fd_in, translated.c_str(), translated.size())!=(int)translated.size()) {
				fprintf(stderr, "VT: write failed\n");
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
			SendSignal(SIGKILL);
		} else if (_slavename.empty()) //pipes fallback
			SendSignal(SIGINT);		
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
			
			const char *spec = VT_TranslateSpecialKey(KeyEvent.wVirtualKeyCode, ctrl, alt, shift);
			if (spec)
				return spec;
		}

		wchar_t wz[3] = {KeyEvent.uChar.UnicodeChar, 0};
		if (_slavename.empty() && wz[0] == '\r') //pipes fallback
			wz[0] = '\n';
		
		return Wide2MB(&wz[0]);
	}
	
	void SendSignal(int sig)
	{
		pid_t grp = getpgid(_pid);
		
		if (grp!=-1 && grp!=getpgid(getpid()))
			killpg(grp, sig);
		else
			kill(_pid, sig);
	}
	

	void Shutdown() {
		CheckedCloseFD(_fd_in);
		CheckedCloseFD(_fd_out);
		
		if (_pid!=-1) {
			//kill(_pid, SIGKILL);
			int status;
			waitpid(_pid, &status, 0);
			_pid = -1;
		}
	}
	
	public:
	VTShell() :
		_fd_out(-1), _fd_in(-1), 
		_pipes_fallback_in(-1), _pipes_fallback_out(-1), 
		_pid(-1), _skipping_line(false)
	{		
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
	
	std::string EscapeQuotas(std::string str)
	{
		for(size_t p = str.find('\"'); p!=std::string::npos; p = str.find('\"', p)) {
			str.insert(p, 1, '\\');
			p+= 2;
		}
		return str;
	}
	
	int ExecuteCommand(const char *cmd)
	{
		if (_pid==-1) {
			return -1;
		}
		if (!_slavename.empty())
			UpdateTerminalSize(_fd_out);
		
		char tmp_script_name[128]; sprintf(tmp_script_name, "vtcmd/%x_%p.cmd", getpid(), this);
		std::string tmp_script = InMyProfile(tmp_script_name);
		FILE *f = fopen(tmp_script.c_str(), "wt");
		if (!f)
			return -1;

static bool shown_tip_ctrl_alc_c = false;
static bool shown_tip_exit = false;

		char cd[MAX_PATH + 1] = {'.', 0};
		if (!getcwd(cd, MAX_PATH)) perror("getcwd");
		fprintf(f, "PS1=''\n");//reduce risk of glitches
		fprintf(f, "%s\n", _completion_marker.SetEnvCommand().c_str());
		fprintf(f, "cd \"%s\"\n", EscapeQuotas(cd).c_str());
		//fprintf(f, "stty echo\n");
		if (strcmp(cmd, "exit")==0) {
			if (!shown_tip_exit) {
				fprintf(f, "echo \"TIP: Closing back shell. To close FAR - type 'exit far'.\"\n");
				shown_tip_exit = true;
			}
		} else if (!shown_tip_ctrl_alc_c) {
			fprintf(f, "echo \"TIP: If you feel stuck - use Ctrl+Alt+C to terminate everything in this shell.\"\n");
			shown_tip_ctrl_alc_c = true;
		}
		fprintf(f, "%s\n", cmd);
		fprintf(f, "FARVTRESULT=$?\n");//it will be echoed to caller from outside
		fprintf(f, "cd ~\n");//avoid locking arbitrary directory
		//fprintf(f, "stty -echo\n");
		fprintf(f, "%s\n", _completion_marker.SetEnvCommand().c_str());//second time - prevent user from shooting own leg
		fclose(f);
		
		std::string cmd_str = ". ";
		cmd_str+= EscapeQuotas(tmp_script);
		cmd_str+= ';';
		cmd_str+= _completion_marker.EchoCommand();
		cmd_str+= '\n';
		
		int r = write(_fd_in, cmd_str.c_str(), cmd_str.size());
		if (r != (int)cmd_str.size()) {
			fprintf(stderr, "VT: write failed\n");
			return -1;
		}
		
		_completion_marker.ScanReset();
		_skipping_line = true;
		
		VTOutputReader output_reader(this, _fd_out);
		INPUT_RECORD last_window_info_ir = {0};
		
		for (;;) {
			INPUT_RECORD ir = {0};
			DWORD dw = 0;
			if (!WINPORT(ReadConsoleInput)(0, &ir, 1, &dw)) {
				break;
			}

			if (ir.EventType==KEY_EVENT && ir.Event.KeyEvent.bKeyDown) {
				OnKeyDown(ir.Event.KeyEvent);
			} else if (ir.EventType==WINDOW_BUFFER_SIZE_EVENT) {
				last_window_info_ir = ir;
				if (!_slavename.empty())
					UpdateTerminalSize(_fd_out);
			}
			if (!output_reader.IsActive())
				break;
		}
		
		if (last_window_info_ir.EventType==WINDOW_BUFFER_SIZE_EVENT) { //handover this event to far
			DWORD dw = 0;
			WINPORT(WriteConsoleInput)(NULL, &last_window_info_ir, 1, &dw);
		}
		
		if (_pid!=-1) {
			int status;
			if (waitpid(_pid, &status, WNOHANG)==_pid) {
				_pid = -1;
			}
		}
		
		remove(tmp_script.c_str());

		return _completion_marker.LastExitCode();
	}	

	int ExecuteForkProc(const char *cmd, int (WINAPI *fork_proc)(int argc, char *argv[]))
	{
		/*struct termios ts = {0};
		if (!_slavename.empty() && tcgetattr(_fd_out, &ts)==0) {
			struct termios tse = ts;
			tse.c_lflag|= ECHO;
			tcsetattr( _fd_out, TCSADRAIN, &tse );
		}*/
		
		int r = ForkAndAttachToSlave(false);
		if (r==-1)
			return -1;

		if (r==0) {
			::ExecuteOrForkProc(cmd, fork_proc);
			abort();
		}
		
		_completion_marker.ScanReset();
		_skipping_line = false;
		VTOutputReader output_reader(this, _fd_out);
		
		int status = -1;
		waitpid(r, &status, 0);
		
		/*if (!_slavename.empty()) {
			tcsetattr( _fd_out, TCSADRAIN, &ts );
		}*/		
		return status;
	}
	
	bool IsOK()
	{
		return _pid!=-1;
	}
};

static std::unique_ptr<VTShell> g_vts;
static std::mutex g_vts_mutex;

int VTShell_Execute(const char *cmd, int (WINAPI *fork_proc)(int argc, char *argv[]) ) 
{	
	std::lock_guard<std::mutex> lock(g_vts_mutex);
	if (!g_vts)
		g_vts.reset(new VTShell);
		
	int r;
	if (fork_proc) {
		r = g_vts->ExecuteForkProc(cmd, fork_proc);
	} else {
		r = g_vts->ExecuteCommand(cmd);
	}

	if (!g_vts->IsOK()) {
		fprintf(stderr, "Shell exited\n");
		g_vts.reset();
	}

	return r;
}

