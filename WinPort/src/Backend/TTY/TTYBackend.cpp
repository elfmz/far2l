#include <assert.h>
#include <errno.h>
#include <signal.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include "utils.h"
#include "WinPortHandle.h"
#include "ConsoleOutput.h"
#include "ConsoleInput.h"
#include "TTYBackend.h"

extern ConsoleOutput g_winport_con_out;
extern ConsoleInput g_winport_con_in;

static volatile long s_terminal_size_change_id = 0;
static TTYBackend * g_vtb = nullptr;

TTYBackend::TTYBackend(int std_in, int std_out) :
	_stdin(std_in),
	_stdout(std_out),
	_tty_out(std_out)
{
	memset(&_ts, 0 , sizeof(_ts));
#if !defined(__APPLE__)
	if (pipe2(_kickass, O_CLOEXEC) == -1) {
		_kickass[0] = _kickass[1] = -1;
	}
#else
	if (pipe(_kickass) == -1) {
		_kickass[0] = _kickass[1] = -1;
	}
	else if (fcntl(_kickass[0], F_SETFD, FD_CLOEXEC) != 0 || fcntl(_kickass[1], F_SETFD, FD_CLOEXEC) != 0)
	{
		close(_kickass[0]);
		close(_kickass[1]);
		_kickass[0] = _kickass[1] = -1;
	}
#endif

	_ts_r = tcgetattr(_stdout, &_ts);
	if (_ts_r == 0) {
		struct termios ts_ne = _ts;
		//ts_ne.c_lflag &= ~(ECHO | ECHONL);
		cfmakeraw(&ts_ne);
		if (tcsetattr( _stdout, TCSADRAIN, &ts_ne ) != 0) {
			perror("TTYBackend: tcsetattr");
		}
	} else {
		perror("TTYBackend: tcgetattr");
	}
	g_vtb = this;
}

TTYBackend::~TTYBackend()
{
	if (g_vtb == this)
		g_vtb =  nullptr;

	OnConsoleExit();

	if (_reader_trd) {
		pthread_join(_reader_trd, nullptr);
		_reader_trd = 0;
	}
	if (_writer_trd) {
		pthread_join(_writer_trd, nullptr);
		_writer_trd = 0;
	}
	if (_ts_r == 0) {
		if (tcsetattr( _stdout, TCSADRAIN, &_ts ) != 0) {
			perror("~TTYBackend: tcsetattr");
		}
	}
	close(_kickass[1]);
	close(_kickass[0]);
}

bool TTYBackend::Startup()
{
	assert(!_writer_trd);
	assert(!_reader_trd);

	if (pthread_create(&_writer_trd, NULL, sWriterThread, this) != 0) {
		return false;
	}

	if (pthread_create(&_reader_trd, NULL, sReaderThread, this) != 0) {
		_exiting = true;
		return false;
	}
	_cur_width =_cur_height = 64;
	g_winport_con_out.GetSize(_cur_width, _cur_height);
	g_winport_con_out.SetBackend(this);

	std::unique_lock<std::mutex> lock(_async_mutex);
	_ae.term_resized = true;
	_async_cond.notify_all();
	return true;
}


void TTYBackend::WriterThread()
{
	try {
		_tty_out.SetScreenBuffer(true);
		_tty_out.ChangeKeypad(true);
		_tty_out.Flush();
		while (!_exiting) {
			AsyncEvent ae;
			{
				std::unique_lock<std::mutex> lock(_async_mutex);
				_async_cond.wait(lock);
				std::swap(ae, _ae);
			}

			if (ae.term_resized) {
				DispatchTermResized();
				ae.output = true;
			}

			if (ae.output)
				DispatchOutput();

			_tty_out.Flush();
		}
		_tty_out.ChangeCursor(true, 13);
		_tty_out.ChangeKeypad(false);
		_tty_out.SetScreenBuffer(false);
		_tty_out.Flush();

	} catch (std::exception &e) {
		fprintf(stderr, "WriterThread: %s <%d>\n", e.what(), errno);
	}
	_exiting = true;
}

void TTYBackend::ReaderThread()
{
	try {
		fd_set fds, fde;
		while (!_exiting) {
			int maxfd = (_kickass[0] > _stdin) ? _kickass[0] : _stdin;

			FD_ZERO(&fds);
			FD_ZERO(&fde);
			FD_SET(_kickass[0], &fds); 
			FD_SET(_stdin, &fds); 
			FD_SET(_stdin, &fde); 

			if (select(maxfd + 1, &fds, NULL, &fde, NULL) == -1) {
				throw std::runtime_error("select failed");
			}

			if (FD_ISSET(_stdin, &fds)) {
				char c = 0;
				if (read(_stdin, &c, 1) <= 0) {
					throw std::runtime_error("stdin read failed");
				}
				fprintf(stderr, "ReaderThread: CHAR 0x%x\n", (unsigned char)c);
				_tty_in.OnChar(c);
			}

			if (FD_ISSET(_stdin, &fde)) {
				throw std::runtime_error("stdin exception");
			}

			if (FD_ISSET(_kickass[0], &fds)) {
				char cmd = 0;
				if (read(_kickass[0], &cmd, 1) <= 0) {
					throw std::runtime_error("kickass read failed");
				}

				long terminal_size_change_id =
					__sync_val_compare_and_swap(&s_terminal_size_change_id, 0, 0);
				if (_terminal_size_change_id != terminal_size_change_id) {
					_terminal_size_change_id = terminal_size_change_id;
					std::unique_lock<std::mutex> lock(_async_mutex);
					_ae.term_resized = true;
					_async_cond.notify_all();
				}
			}
		}

	} catch (std::exception &e) {
		fprintf(stderr, "ReaderThread: %s <%d>\n", e.what(), errno);
	}
	_exiting = true;
}

/////////////////////////////////////////////////////////////////////////

void TTYBackend::DispatchTermResized()
{
	struct winsize w = {};
	if (ioctl(_stdout, TIOCGWINSZ, &w) == 0 &&
	(_cur_width != w.ws_col || _cur_height != w.ws_row)) {
		_cur_width = w.ws_col;
		_cur_height = w.ws_row;
		g_winport_con_out.SetSize(_cur_width, _cur_height);
		g_winport_con_out.GetSize(_cur_width, _cur_height);
		INPUT_RECORD ir = {0};
		ir.EventType = WINDOW_BUFFER_SIZE_EVENT;
		ir.Event.WindowBufferSizeEvent.dwSize.X = _cur_width;
		ir.Event.WindowBufferSizeEvent.dwSize.Y = _cur_height;
		g_winport_con_in.Enqueue(&ir, 1);

		std::vector<CHAR_INFO> tmp;
		std::lock_guard<std::mutex> lock(_output_mutex);
		_tty_out.MoveCursor(1, 1, true);
		_prev_height = _prev_width = 0;
		_prev_output.swap(tmp);// ensure memory released
	}
}

void TTYBackend::DispatchOutput()
{
	std::lock_guard<std::mutex> lock(_output_mutex);

	_cur_output.resize(_cur_width * _cur_height);

	COORD data_size = {_cur_width, _cur_height};
	COORD data_pos = {0, 0};
	SMALL_RECT screen_rect = {0, 0, _cur_width - 1, _cur_height - 1};
	g_winport_con_out.Read(&_cur_output[0], data_size, data_pos, screen_rect);

#if 1
	for (unsigned int y = 0; y < _cur_height; ++y) {
		const CHAR_INFO *cur_line = &_cur_output[y * _cur_width];
		if (y >= _prev_height) {
			_tty_out.MoveCursor(y + 1, 1);
			_tty_out.WriteLine(cur_line, _cur_width);
		} else {
			const CHAR_INFO *last_line = &_prev_output[y * _prev_width];
			for (unsigned int x = 0; x < _cur_width; ++x) {
				if (x >= _prev_width
				 || cur_line[x].Char.UnicodeChar != last_line[x].Char.UnicodeChar
				 || cur_line[x].Attributes != last_line[x].Attributes) {
					_tty_out.MoveCursor(y + 1, x + 1);
					_tty_out.WriteLine(&cur_line[x], 1);
				}
			}
		}
	}

#else
	for (unsigned int y = 0; y < _cur_height; ++y) {
		const CHAR_INFO *cur_line = &_cur_output[y * _cur_width];
		_tty_out.MoveCursor(y + 1, 1);
		_tty_out.WriteLine(cur_line, _cur_width);
	}
#endif
	_prev_width = _cur_width;
	_prev_height = _cur_height;
	_prev_output.swap(_cur_output);

	UCHAR cursor_height = 1;
	bool cursor_visible = false;
	COORD cursor_pos = g_winport_con_out.GetCursor(cursor_height, cursor_visible);
	_tty_out.MoveCursor(cursor_pos.Y + 1, cursor_pos.X + 1);
	_tty_out.ChangeCursor(cursor_visible, cursor_height);
}

/////////////////////////////////////////////////////////////////////////

void TTYBackend::KickAss()
{
	unsigned char c = 0;
	write(_kickass[1], &c, 1);
}

void TTYBackend::OnConsoleOutputUpdated(const SMALL_RECT *areas, size_t count)
{
	std::unique_lock<std::mutex> lock(_async_mutex);
	_ae.output = true;
	_async_cond.notify_all();
}

void TTYBackend::OnConsoleOutputResized()
{
	OnConsoleOutputUpdated(NULL, 0);
}

void TTYBackend::OnConsoleOutputTitleChanged()
{
	//_tty_out.SetWindowTitle(g_winport_con_out.GetTitle());
	//ESC]2;titleST
}

void TTYBackend::OnConsoleOutputWindowMoved(bool absolute, COORD pos)
{
}

COORD TTYBackend::OnConsoleGetLargestWindowSize()
{
	COORD out = {0x100, 0x100};
	return out;
}

void TTYBackend::OnConsoleAdhocQuickEdit()
{
}

DWORD TTYBackend::OnConsoleSetTweaks(DWORD tweaks)
{
	return 0;
}

void TTYBackend::OnConsoleChangeFont()
{
}

void TTYBackend::OnConsoleSetMaximized(bool maximized)
{
}

void TTYBackend::OnConsoleExit()
{
	{
		std::unique_lock<std::mutex> lock(_async_mutex);
		_exiting = true;
		_async_cond.notify_all();
	}
	KickAss();
}

bool TTYBackend::OnConsoleIsActive()
{
	return true;
}

//

bool TTYBackend::OnClipboardOpen()
{
	return 0;
}

void TTYBackend::OnClipboardClose()
{
}

void TTYBackend::OnClipboardEmpty()
{
}

bool TTYBackend::OnClipboardIsFormatAvailable(UINT format)
{
	return 0;
}

void *TTYBackend::OnClipboardSetData(UINT format, void *data)
{
	return 0;
}

void *TTYBackend::OnClipboardGetData(UINT format)
{
	return 0;
}

UINT TTYBackend::OnClipboardRegisterFormat(const wchar_t *lpszFormat)
{
	return 0;
}

static void sigwinch_handler(int)
{
	__sync_add_and_fetch ( &s_terminal_size_change_id, 1);
	if (g_vtb)
		g_vtb->KickAss();
}


bool WinPortMainTTY(int std_in, int std_out, int argc, char **argv, int(*AppMain)(int argc, char **argv), int *result)
{
	TTYBackend  vtb(std_in, std_out);
	signal(SIGWINCH,  sigwinch_handler);
	if (!vtb.Startup()) {
		return false;
	}

	*result = AppMain(argc, argv);
	return true;
}
