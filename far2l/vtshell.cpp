
#include "headers.hpp"
#pragma hdrstop
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

class VTShell
{
	std::string _cmd;
	std::mutex _mutex;
	VTAnsi _vta;
	pthread_t _thread;

	bool  _thread_state;
	bool _thread_exiting;
	int _fd_out;
	int _fd_in;
	pid_t _pid, _grp;
	int (WINAPI *_fork_proc)(int argc, char *argv[]);

	bool Startup()
	{
		int fd_in[2], fd_out[2], r = -1;

		int fdm = posix_openpt(O_RDWR); 
		if (fdm!=-1) {
			if (grantpt(fdm)==0 && unlockpt(fdm)==0) {
				int fds = open(ptsname(fdm), O_RDWR); 
				if (fds!=-1) {
					struct termios ts = {0};
					if (tcgetattr(fds, &ts)==0) {
						ts.c_lflag |= ECHO | ISIG | ICANON;
						ts.c_cc[VINTR] = 3;
						tcsetattr( fds, TCSAFLUSH, &ts );
					} 
					CONSOLE_SCREEN_BUFFER_INFO csbi = {0};
					if (WINPORT(GetConsoleScreenBufferInfo)( NULL, &csbi )) {
						struct winsize ws = {0};
						ws.ws_row =csbi.dwSize.Y ? csbi.dwSize.Y : 1;
						ws.ws_col = csbi.dwSize.X ? csbi.dwSize.X : 1;
						fprintf(stderr, "setting console size: %u x %u\n", ws.ws_col, ws.ws_row);
						ioctl( fdm, TIOCSWINSZ, &ws );						
					}
					fd_in[0] = fds;
					fd_out[1] = dup(fds);
					fd_in[1] = fdm;
					fd_out[0] = dup(fdm);
					r = 0;
				}
			}
			if (r!=0)
				close (fdm);
		}

		if (r!=0) {
			fprintf(stderr, "pseudo-VT failed, fallback to pipes\n");
			r = pipe(fd_in);
			if (r < 0) {
				perror("VTShell: fds_in");
				return false;
			}

			r = pipe(fd_out);
			if (r < 0) {
				perror("VTShell: fds_out");
				close_fd_pair(fd_in);
				return false;
			}
		}

		r = fork();
		if (r == 0) {
			setsid();
			if ( ioctl( 0, TIOCSCTTY, 0 ) )
				 perror( "ioctl(TIOCSCTTY)" );
			//setenv("TERM", "xterm-256color", 1);
			setenv("TERM", "xterm", 1);
			signal( SIGINT, SIG_DFL );

			dup2(fd_in[0], STDIN_FILENO);
			dup2(fd_out[1], STDOUT_FILENO);
			dup2(fd_out[1], STDERR_FILENO);
			close_fd_pair(fd_in); 
			close_fd_pair(fd_out);
			ExecuteOrForkProc(_cmd.c_str(), _fork_proc) ;
		}
	
		if (r==-1) { 
			perror("VTShell: fork");
			close_fd_pair(fd_in);
			close_fd_pair(fd_out);
			return false;
		}
	
		close(fd_in[0]);
		close(fd_out[1]);
		_fd_in = fd_in[1];
		_fd_out = fd_out[0];
		fcntl(_fd_in, F_SETFD, FD_CLOEXEC);
		fcntl(_fd_out, F_SETFD, FD_CLOEXEC);
		_pid = r;		
		for(;;){
			_grp = getpgid(_pid);
			if (_grp!=getpgid(getpid()))break;
			usleep(10000);
		}
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
		std::wstring ws = MB2Wide(std::string(buf, len).c_str());
		_vta.Write(ws.c_str(), ws.size());
		/*wchar_t crlf[2] = {L'\r', L'\n'};
		DWORD dw;
		for (;;) {
			size_t p = ws.find(L'\n');
			WINPORT(WriteConsole)(0, ws.c_str(), 
				(p==std::wstring::npos) ? ws.size() : p,  &dw, NULL);
			if (p==std::wstring::npos) break;
			ws.erase(0, p + 1);
			WINPORT(WriteConsole)(0, crlf, 2,  &dw, NULL);
		}*/
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
		_grp = -1;
	}

	public:
	VTShell(const char *cmd, int (WINAPI *fork_proc)(int argc, char *argv[]) = NULL )
		: _cmd(cmd), _thread_state(false), _thread_exiting(false), _fd_out(-1), _fd_in(-1), _pid(-1),  _grp(-1), _fork_proc(fork_proc)
	{
		if (!Startup())
			return;

		_thread_state = true;
		if (pthread_create(&_thread, NULL, sOutputReaderThread, this)) {
			perror("VTShell: pthread_create");
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
			if (ir.EventType==KEY_EVENT && ir.Event.KeyEvent.bKeyDown) {
				const std::string &translated = TranslateInput(ir);
				if (!translated.empty()) {
					if (write(_fd_in, translated.c_str(), translated.size())<=0) {
						printf("VTShell: write failed\n");
					}
				} else {
					printf("VTShell: not translated keydown: 0x%x\n", ir.Event.KeyEvent.wVirtualKeyCode);
				}
			}
			
			std::lock_guard<std::mutex> lock(_mutex);
			if (_thread_exiting) break;
		}

		int status = 0;
		if (waitpid(_pid, &status, 0)==-1) {
			fprintf(stderr, "VTShell: waitpid(0x%x) error %u\n", _pid, errno);
			status = 1;
		}
		return status;
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
	
	void SendSignal(int sig)
	{
		if (_grp!=-1)
			killpg(_grp, sig);
		else
			kill(_pid, sig);
	}
	
	std::string TranslateInput(const INPUT_RECORD &ir)
	{
		if (IsControlAltOnlyPressed(ir.Event.KeyEvent.dwControlKeyState)) {
			if (ir.Event.KeyEvent.wVirtualKeyCode=='C')
				SendSignal(SIGKILL);
			
		} else if (IsControlOnlyPressed(ir.Event.KeyEvent.dwControlKeyState)) {
			char c = 0;
			switch (ir.Event.KeyEvent.wVirtualKeyCode)
			{
			case 'A': c = (char)0x01; break;
			case 'B': c = (char)0x02; break;
			case 'C': 
				SendSignal(SIGINT);
				c = (char)0x03; 
				break;
			case 'D': c = (char)0x04; break;
			case 'E': c = (char)0x05; break;
			case 'F': c = (char)0x06; break;
			case 'G': c = (char)0x07; break;
			case 'H': c = (char)0x08; break;
			case 'I': c = (char)0x09; break;
			case 'J': c = (char)0x0a; break;
			case 'K': c = (char)0x0b; break;
			case 'L': c = (char)0x0c; break;			
			case 'M': c = (char)0x0d; break;
			case 'N': c = (char)0x0e; break;
			case 'O': c = (char)0x0f; break;
			case 'P': c = (char)0x10; break;
			case 'Q': c = (char)0x11; break;
			case 'R': c = (char)0x12; break;
			case 'S': c = (char)0x13; break;
			case 'T': c = (char)0x14; break;
			case 'U': c = (char)0x15; break;
			case 'V': c = (char)0x16; break;
			case 'W': c = (char)0x17; break;
			case 'X': c = (char)0x18; break;
			case 'Y': c = (char)0x19; break;
			case 'Z': c = (char)0x1a; break;
			case '[': c = (char)0x1b; break;
			case '\\': c = (char)0x1c; break;
			case ']': c = (char)0x1d; break;
			//case '^': c = (char)0x1e; break;
			//case '_': c = (char)0x1f; break;
			}
			
			if (c)
				return std::string(&c, 1);
		}
		
		switch ( ir.Event.KeyEvent.wVirtualKeyCode)
		{
			case VK_F1: return "\x1bOP";
			case VK_F2: return "\x1bOQ";
			case VK_F3: return "\x1bOR";
			case VK_F4: return "\x1bOS";
			case VK_F5: return "\x1b[15~";
			case VK_F6: return "\x1b[17~";
			case VK_F7: return "\x1b[18~";
			case VK_F8: return "\x1b[19~";
			case VK_F9: return "\x1b[20~";
			case VK_F10: return "\x1b[21~";
			case VK_F11: return "\x1b[23~";
			case VK_F12: return "\x1b[24~";
			case VK_UP: return "\x1b[A";
			case VK_DOWN: return "\x1b[B";
			case VK_RIGHT: return  "\x1b[C";
			case VK_LEFT: return "\x1b[D";
			case VK_HOME: return  "\x1b[H";
			case VK_END: return  "\x1b[F";
			case VK_BACK: return "\x08";
			case VK_DELETE: return "\x1b[3~";
			case VK_NEXT: return "\x1b[6~";
			case VK_PRIOR: return "\x1b[5~";
			case VK_INSERT: return "\x1b[2~";
			//case VK_CANCEL: return "\x3";
		}
			
		wchar_t wz[2] = {ir.Event.KeyEvent.uChar.UnicodeChar, 0};
		return Wide2MB(&wz[0]);
	}
};

int VTShell_Execute(const char *cmd, int (WINAPI *fork_proc)(int argc, char *argv[]) ) 
{	
	VTShell vts(cmd, fork_proc);
	return vts.Wait();
}
