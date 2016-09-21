
#include "headers.hpp"
#include "clipboard.hpp"
#include <signal.h>
#include <pthread.h>
#include <mutex>
#include <fcntl.h>
#include <sys/ioctl.h> 
#include <sys/wait.h> 
#include <condition_variable>
#include "vtansi.h"
#define __USE_BSD 
#include <termios.h> 

static void close_fd_pair(int *fds)
{
	if (close(fds[0])<0) fprintf(stderr, "close_fd_pair bad 1\n");
	if (close(fds[1])<0) fprintf(stderr, "close_fd_pair bad 2\n");
}

void ExecuteOrForkProc(const char *CmdStr, int (WINAPI *ForkProc)(int argc, char *argv[]) ) ;
const char *VT_TranslateSpecialKey(const WORD key, bool ctrl, bool alt, bool shift, char keypad = 0);


#if 1 //change to 0 to enable verbose I/O reports to stderr
# define DbgPrintEscaped(info, s)
#else
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
#endif

class VTShell
{
	std::string _cmd;
	std::mutex _mutex;
	VTAnsi _vta;
	pthread_t _thread;
	bool _pipes_fallback;
	bool  _thread_state;
	bool _thread_exiting;
	int _fd_out;
	int _fd_in;
	pid_t _pid;
	int (WINAPI *_fork_proc)(int argc, char *argv[]);

	bool Startup()
	{
		int fd_term = posix_openpt( O_RDWR | O_NOCTTY ); //use -1 to verify _pipes_fallback, 
		const char *slavename = NULL;
		if (fd_term!=-1) {
			_pipes_fallback = false;
			if (grantpt(fd_term)==0 && unlockpt(fd_term)==0) {
				CONSOLE_SCREEN_BUFFER_INFO csbi = { };
				if (WINPORT(GetConsoleScreenBufferInfo)( NULL, &csbi )  
							&& csbi.dwSize.X && csbi.dwSize.Y) {
								
					struct winsize ws = {(unsigned short)csbi.dwSize.Y, 
						(unsigned short)csbi.dwSize.X, 0, 0};
					if (ioctl( fd_term, TIOCSWINSZ, &ws )==-1)
						perror("VT: ioctl(TIOCSWINSZ)");
				}
				
				slavename = ptsname(fd_term);
				if (!slavename)
					perror("VT: ptsname");
			} else
				perror("VT: grantpt/unlockpt");
				
			if (!slavename) {
				close(fd_term);
				fd_term = -1;
			}
		} else
			perror("VT: posix_openpt");

		int fd_in[2] = {-1, -1}, fd_out[2] = {-1, -1};
		if (fd_term==-1) {
			fprintf(stderr, "VT: fallback to pipes\n");
			if (pipe(fd_in) < 0) {
				perror("VT: fds_in");
				return false;
			}

			if (pipe(fd_out) < 0) {
				perror("VT: fds_out");
				close_fd_pair(fd_in);
				return false;
			}
			_pipes_fallback = true;
		}
		
		int r = fork();
		if (r == 0) {
			if (fd_term!=-1) {
				close(fd_term);
				fd_term = -1;
			}
			if (slavename) {
				if (setsid()==-1)
					perror("VT: setsid");
				
				fd_term = open(slavename, O_RDWR); 
				if (fd_term==-1) {
					perror("VT: open slave");
					exit(errno);
				}
				
				struct termios ts = {0};
				if (tcgetattr(fd_term, &ts)==0) {
					ts.c_lflag |= ECHO | ISIG | ICANON;
					ts.c_cc[VINTR] = 3;
					tcsetattr( fd_term, TCSAFLUSH, &ts );
				} 
				if ( ioctl( 0, TIOCSCTTY, 0 )==-1 )
					perror( "VT: ioctl(TIOCSCTTY)" );
					
				dup2(fd_term, STDIN_FILENO);
				dup2(fd_term, STDOUT_FILENO);
				dup2(fd_term, STDERR_FILENO);
				close(fd_term);
				fd_term = -1;
			} else {
				dup2(fd_in[0], STDIN_FILENO);
				dup2(fd_out[1], STDOUT_FILENO);
				dup2(fd_out[1], STDERR_FILENO);
				close_fd_pair(fd_in); 
				close_fd_pair(fd_out);				
			}
			
			//setenv("TERM", "xterm-256color", 1);
			setenv("TERM", "xterm", 1);
			signal( SIGINT, SIG_DFL );
			
			ExecuteOrForkProc(_cmd.c_str(), _fork_proc) ;
		}
	
		if (r==-1) { 
			perror("VT: fork");
			if (fd_term==-1) {
				close_fd_pair(fd_in);
				close_fd_pair(fd_out);
			} else
				close(fd_term);
			return false;
		}
		
		if (fd_term==-1) {
			close(fd_in[0]);
			close(fd_out[1]);
			_fd_in = fd_in[1];
			_fd_out = fd_out[0];
		} else {
			_fd_in = dup(fd_term);
			_fd_out = fd_term;
		}
		fcntl(_fd_in, F_SETFD, FD_CLOEXEC);
		fcntl(_fd_out, F_SETFD, FD_CLOEXEC);
		_pid = r;		
		return true;
	}
	
	static void *sOutputReaderThread(void *p)
	{
		return ((VTShell *)p)->OutputReaderThread();
	}
	
	void *OutputReaderThread()
	{
		char buf[0x1000];
		for (;;) {
			int r = read(_fd_out, buf, sizeof(buf));
			if (r>0)
				ProcessOutput(buf, r) ;
				
			std::lock_guard<std::mutex> lock(_mutex);
			if (r<=0 || !_thread_state)  {
				_thread_exiting = true;
				break;
			}
		}
		
		//write some console input to kick pending ReadConsoleInput
		INPUT_RECORD ir = {0};
		ir.EventType = FOCUS_EVENT;
		ir.Event.FocusEvent.bSetFocus = TRUE;
		DWORD dw = 0;
		WINPORT(WriteConsoleInput)(0, &ir, 1, &dw);

		return NULL;
	}
	
	void ProcessOutput(const char *buf, int len) 
	{
		std::string s(buf, len);
		const std::wstring &ws = MB2Wide(s.c_str());
		DbgPrintEscaped("OUTPUT", s);
		_vta.Write(ws.c_str(), ws.size());
	}

	
	void Shutdown() {
		if (_fd_in!=-1) {
			close(_fd_in);
			_fd_in = -1;
		}
		
		if (_thread_state) {
			{
				std::lock_guard<std::mutex> cl(_mutex);
				_thread_state = false;
			}
			pthread_join(_thread, NULL);
		}


		if (_pid!=-1) {
			//kill(_pid, SIGKILL);
			_pid = -1;
		}
		if (_fd_out!=-1) {
			close(_fd_out);
			_fd_out = -1;
		}
	}

	void OnKeyEvent(KEY_EVENT_RECORD &KeyEvent)
	{
		if (!KeyEvent.bKeyDown)
			return;
		
		DWORD dw;
		const std::string &translated = TranslateKeyEvent(KeyEvent);
		if (!translated.empty()) {
			if (_pipes_fallback && KeyEvent.uChar.UnicodeChar) {
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

	bool IsControlOnlyPressed(DWORD dwControlKeyState)
	{
		return ( (dwControlKeyState & (LEFT_CTRL_PRESSED|RIGHT_CTRL_PRESSED)) != 0  &&
				(dwControlKeyState & (RIGHT_ALT_PRESSED|LEFT_ALT_PRESSED|SHIFT_PRESSED)) == 0);
	}
	
	bool IsControlAltOnlyPressed(DWORD dwControlKeyState)
	{
		return ( (dwControlKeyState & (LEFT_CTRL_PRESSED|RIGHT_CTRL_PRESSED)) != 0  &&
				(dwControlKeyState & (RIGHT_ALT_PRESSED|LEFT_ALT_PRESSED)) != 0  &&
				(dwControlKeyState & (SHIFT_PRESSED)) == 0);
	}
	
	void OnCtrlC(bool alt)
	{
		if (alt) {
			fprintf(stderr, "VT: Ctrl+Alt+C - killing them hardly...\n");
			SendSignal(SIGKILL);
		} else if (_pipes_fallback) 
			SendSignal(SIGINT);		
	}
	
	std::string StringClipboard()
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
			if ((ctrl && shift && !alt && KeyEvent.wVirtualKeyCode=='V') ||
			    (!ctrl && shift && !alt && KeyEvent.wVirtualKeyCode==VK_INSERT) ){
				return StringClipboard();
			}
			if (ctrl && !shift && KeyEvent.wVirtualKeyCode=='C') {
				OnCtrlC(alt);
			} 
			const char *spec = VT_TranslateSpecialKey(KeyEvent.wVirtualKeyCode, ctrl, alt, shift);
			if (spec)
				return spec;
		}

		wchar_t wz[3] = {KeyEvent.uChar.UnicodeChar, 0};
		if (_pipes_fallback && wz[0] == '\r') 
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
	
	public:
	VTShell(const char *cmd, int (WINAPI *fork_proc)(int argc, char *argv[]) = NULL )
		: _cmd(cmd), _pipes_fallback(false), _thread_state(false), _thread_exiting(false), 
		_fd_out(-1), _fd_in(-1), _pid(-1),  _fork_proc(fork_proc)
	{
		if (!Startup())
			return;

		_thread_state = true;
		if (pthread_create(&_thread, NULL, sOutputReaderThread, this)) {
			perror("VT: pthread_create");
			_thread_state = false;
			Shutdown();
		}
	}
	
	~VTShell()
	{
		Shutdown();
	}


	int Wait()
	{
		if (_pid==-1)
			return -1;

		for (;;) {
			INPUT_RECORD ir = {0};
			DWORD dw = 0;
			if (!WINPORT(ReadConsoleInput)(0, &ir, 1, &dw)) break;
			if (ir.EventType==KEY_EVENT) 
				OnKeyEvent(ir.Event.KeyEvent);
			
			std::lock_guard<std::mutex> lock(_mutex);
			if (_thread_exiting) break;
		}

		int status = 0;
		if (waitpid(_pid, &status, 0)==-1) {
			fprintf(stderr, "VT: waitpid(0x%x) error %u\n", _pid, errno);
			status = 1;
		}
		return status;
	}

};

int VTShell_Execute(const char *cmd, int (WINAPI *fork_proc)(int argc, char *argv[]) ) 
{	
	VTShell vts(cmd, fork_proc);
	return vts.Wait();
}
