
#include "headers.hpp"
#pragma hdrstop
#include <signal.h>
#include <pthread.h>
#include <mutex>
#include <fcntl.h>
#include <sys/ioctl.h> 
#include <condition_variable>
#include "vtansi.h"
#define __USE_BSD 
#include <termios.h> 

static void close_fd_pair(int *fds)
{
	if (close(fds[0])<0) fprintf(stderr, "close_fd_pair bad 1\n");
	if (close(fds[1])<0) fprintf(stderr, "close_fd_pair bad 2\n");
}

class VTShell
{
	std::string _cmd;
	std::mutex _mutex;
	bool  _thread_state;
	bool _thread_exiting;

	pthread_t _thread;
	int _fd_out;
	int _fd_in;
	pid_t _pid, _grp;
	VTAnsi _vta;

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
		
			r = execl("/bin/sh", "sh", "-c", _cmd.c_str());
			//r = system(_cmd.c_str());
			fprintf(stderr, "execl: %d\n", r);
			exit(r);
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
		std::wstring ws = UTF8to16(std::string(buf, len).c_str());
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
	VTShell(const char *cmd)  
		: _cmd(cmd), _fd_out(-1), _fd_in(-1), _pid(-1),  _grp(-1), _thread_state(false), _thread_exiting(false)
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


	bool Wait()
	{
		if (_pid==-1)
			return false;

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
		return true;
	}

	std::string TranslateInput(const INPUT_RECORD &ir)
	{
		//switch ( ir.Event.KeyEvent.uChar.UnicodeChar )
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
			
		if (ir.Event.KeyEvent.wVirtualKeyCode=='C' && 
			(ir.Event.KeyEvent.dwControlKeyState==LEFT_CTRL_PRESSED ||
			ir.Event.KeyEvent.dwControlKeyState==RIGHT_CTRL_PRESSED)) {
				printf("Ctrl+C -> %u %u!\n",  _pid, _grp);
				if (_grp!=-1)
					killpg(_grp, SIGINT);
				else
					kill(_pid, SIGINT);
			}else {
				printf("key: %u char: %u\n", ir.Event.KeyEvent.wVirtualKeyCode, ir.Event.KeyEvent.uChar.UnicodeChar);		
			}
		wchar_t wz[2] = {ir.Event.KeyEvent.uChar.UnicodeChar, 0};
		return UTF16to8(&wz[0]);
	}
};

bool VTShell_SendCommand(const char *cmd) 
{	
	VTShell vts(cmd);
	return vts.Wait();
}
