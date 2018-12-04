#include <assert.h>
#include <errno.h>
#include <signal.h>
#include <fcntl.h>
#include <exception>
#include <sys/ioctl.h>
#include "utils.h"
#include "CheckedCast.hpp"
#include "WinPortHandle.h"
#include "ConsoleOutput.h"
#include "ConsoleInput.h"
#include "TTYBackend.h"
#include "TTYFar2lClipboardBackend.h"
#include "../FSClipboardBackend.h"

extern ConsoleOutput g_winport_con_out;
extern ConsoleInput g_winport_con_in;

static volatile long s_terminal_size_change_id = 0;
static TTYBackend * g_vtb = nullptr;

TTYBackend::TTYBackend(int std_in, int std_out, bool far2l_tty) :
	_stdin(std_in),
	_stdout(std_out),
	_far2l_tty(far2l_tty)
{
	memset(&_ts, 0 , sizeof(_ts));
	if (pipe_cloexec(_kickass) == -1) {
		_kickass[0] = _kickass[1] = -1;
	}

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
	CheckedCloseFDPair(_kickass);
}

bool TTYBackend::Startup()
{
	assert(!_writer_trd);
	assert(!_reader_trd);

	if (pthread_create(&_writer_trd, nullptr, sWriterThread, this) != 0) {
		return false;
	}

	if (pthread_create(&_reader_trd, nullptr, sReaderThread, this) != 0) {
		_exiting = true;
		return false;
	}
	_cur_width =_cur_height = 64;
	g_winport_con_out.GetSize(_cur_width, _cur_height);
	g_winport_con_out.SetBackend(this);

	std::unique_lock<std::mutex> lock(_async_mutex);
	_ae.flags.term_resized = true;
	_async_cond.notify_all();
	return true;
}


void TTYBackend::WriterThread()
{
	if (_far2l_tty) {
		IFar2lInterractor *interractor = this;
		_clipboard_backend = std::make_shared<TTYFar2lClipboardBackend>(interractor);
	} else {
		_clipboard_backend = std::make_shared<FSClipboardBackend>();
	}

	try {
		TTYOutput tty_out(_stdout);
		tty_out.SetScreenBuffer(true);
		tty_out.ChangeKeypad(true);
		tty_out.ChangeMouse(true);
		tty_out.Flush();

		while (!_exiting) {
			AsyncEvent ae;
			ae.all = 0;
			do {
				std::unique_lock<std::mutex> lock(_async_mutex);
				if (_ae.all == 0) {
					_async_cond.wait(lock);
				}
				if (_ae.all != 0) {
					std::swap(ae, _ae);
					break;
				}
			} while (!_exiting);

			if (ae.flags.term_resized) {
				DispatchTermResized(tty_out);
				ae.flags.output = true;
			}

			if (ae.flags.output)
				DispatchOutput(tty_out);

			if (ae.flags.far2l_interract)
				DispatchFar2lInterract(tty_out);

			tty_out.Flush();
		}

		tty_out.ChangeCursor(true, 13);
		tty_out.ChangeMouse(false);
		tty_out.ChangeKeypad(false);
		tty_out.SetScreenBuffer(false);
		tty_out.Flush();

	} catch (std::exception &e) {
		fprintf(stderr, "WriterThread: %s <%d>\n", e.what(), errno);
	}
	_exiting = true;
}

void TTYBackend::ReaderThread()
{
	try {
		TTYInput tty_in(this);

		fd_set fds, fde;
		while (!_exiting) {
			int maxfd = (_kickass[0] > _stdin) ? _kickass[0] : _stdin;

			FD_ZERO(&fds);
			FD_ZERO(&fde);
			FD_SET(_kickass[0], &fds); 
			FD_SET(_stdin, &fds); 
			FD_SET(_stdin, &fde); 

			if (select(maxfd + 1, &fds, nullptr, &fde, nullptr) == -1) {
				throw std::runtime_error("select failed");
			}

			if (FD_ISSET(_stdin, &fds)) {
				char c = 0;
				if (read(_stdin, &c, 1) <= 0) {
					throw std::runtime_error("stdin read failed");
				}
				//fprintf(stderr, "ReaderThread: CHAR 0x%x\n", (unsigned char)c);
				tty_in.OnChar(c);
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
					_ae.flags.term_resized = true;
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

void TTYBackend::DispatchTermResized(TTYOutput &tty_out)
{
	struct winsize w = {};
	if (ioctl(_stdout, TIOCGWINSZ, &w) == 0 &&
	(_cur_width != w.ws_col || _cur_height != w.ws_row)) {
		_cur_width = w.ws_col;
		_cur_height = w.ws_row;
		g_winport_con_out.SetSize(_cur_width, _cur_height);
		g_winport_con_out.GetSize(_cur_width, _cur_height);
		INPUT_RECORD ir = {};
		ir.EventType = WINDOW_BUFFER_SIZE_EVENT;
		ir.Event.WindowBufferSizeEvent.dwSize.X = _cur_width;
		ir.Event.WindowBufferSizeEvent.dwSize.Y = _cur_height;
		g_winport_con_in.Enqueue(&ir, 1);
		std::vector<CHAR_INFO> tmp;
		std::lock_guard<std::mutex> lock(_output_mutex);
		tty_out.MoveCursor(1, 1, true);
		_prev_height = _prev_width = 0;
		_prev_output.swap(tmp);// ensure memory released
	}
}

void TTYBackend::DispatchOutput(TTYOutput &tty_out)
{
	std::lock_guard<std::mutex> lock(_output_mutex);

	_cur_output.resize(_cur_width * _cur_height);

	COORD data_size = {CheckedCast<SHORT>(_cur_width), CheckedCast<SHORT>(_cur_height) };
	COORD data_pos = {0, 0};
	SMALL_RECT screen_rect = {0, 0, CheckedCast<SHORT>(_cur_width - 1), CheckedCast<SHORT>(_cur_height - 1)};
	g_winport_con_out.Read(&_cur_output[0], data_size, data_pos, screen_rect);

#if 1
	for (unsigned int y = 0; y < _cur_height; ++y) {
		const CHAR_INFO *cur_line = &_cur_output[y * _cur_width];
		if (y >= _prev_height) {
			tty_out.MoveCursor(y + 1, 1);
			tty_out.WriteLine(cur_line, _cur_width);
		} else {
			const CHAR_INFO *last_line = &_prev_output[y * _prev_width];
			for (unsigned int x = 0; x < _cur_width; ++x) {
				if (x >= _prev_width
				 || cur_line[x].Char.UnicodeChar != last_line[x].Char.UnicodeChar
				 || cur_line[x].Attributes != last_line[x].Attributes) {
					tty_out.MoveCursor(y + 1, x + 1);
					tty_out.WriteLine(&cur_line[x], 1);
				}
			}
		}
	}

#else
	for (unsigned int y = 0; y < _cur_height; ++y) {
		const CHAR_INFO *cur_line = &_cur_output[y * _cur_width];
		tty_out.MoveCursor(y + 1, 1);
		tty_out.WriteLine(cur_line, _cur_width);
	}
#endif
	_prev_width = _cur_width;
	_prev_height = _cur_height;
	_prev_output.swap(_cur_output);

	UCHAR cursor_height = 1;
	bool cursor_visible = false;
	COORD cursor_pos = g_winport_con_out.GetCursor(cursor_height, cursor_visible);
	tty_out.MoveCursor(cursor_pos.Y + 1, cursor_pos.X + 1);
	tty_out.ChangeCursor(cursor_visible);

	if (_far2l_cursor_height != (int)(unsigned int)cursor_height && _far2l_tty) {
		_far2l_cursor_height = (int)(unsigned int)cursor_height;

		StackSerializer stk_ser;
		stk_ser.PushPOD(cursor_height);
		stk_ser.PushPOD('h');
		stk_ser.PushPOD((uint8_t)0); // zero ID means not expecting reply
		tty_out.SendFar2lInterract(stk_ser);
	}
}


void TTYBackend::DispatchFar2lInterract(TTYOutput &tty_out)
{
	Far2lInterractV queued;
	{
		std::unique_lock<std::mutex> lock(_async_mutex);
		queued.swap(_far2l_interracts_queued);
	}

	std::unique_lock<std::mutex> lock_sent(_far2l_interracts_sent);

	for (auto & i : queued) {
		uint8_t id = 0;
		if (i->waited) {
			if (_far2l_interracts_sent.size() >= 0xff) {
				i->stk_ser.Clear();
				i->evnt.Signal();
				return;
			}
			for (;;) {
				id = ++_far2l_interracts_sent._id_counter;
				if (id && _far2l_interracts_sent.find(id) == _far2l_interracts_sent.end()) break;
			}
		}
		i->stk_ser.PushPOD(id);

		if (i->waited)
			_far2l_interracts_sent.emplace(id, i);

		tty_out.SendFar2lInterract(i->stk_ser);
	}
}

/////////////////////////////////////////////////////////////////////////

void TTYBackend::KickAss()
{
	unsigned char c = 0;
	if (write(_kickass[1], &c, 1) != 1)
		perror("write(_kickass[1]");
}

void TTYBackend::OnConsoleOutputUpdated(const SMALL_RECT *areas, size_t count)
{
	std::unique_lock<std::mutex> lock(_async_mutex);
	_ae.flags.output = true;
	_async_cond.notify_all();
}

void TTYBackend::OnConsoleOutputResized()
{
	OnConsoleOutputUpdated(nullptr, 0);
}

void TTYBackend::OnConsoleOutputTitleChanged()
{
	try {
		StackSerializer stk_ser;
		stk_ser.PushStr(StrWide2MB(g_winport_con_out.GetTitle()));
		stk_ser.PushPOD('t');
		Far2lInterract(stk_ser, false);
	} catch (std::exception &) {}
	//tty_out.SetWindowTitle(g_winport_con_out.GetTitle());
	//ESC]2;titleST
}

void TTYBackend::OnConsoleOutputWindowMoved(bool absolute, COORD pos)
{
}

COORD TTYBackend::OnConsoleGetLargestWindowSize()
{
	COORD out = {CheckedCast<SHORT>(_cur_width ? _cur_width : 0x10), CheckedCast<SHORT>(_cur_height ? _cur_height : 0x10)};
	return out;
}

void TTYBackend::OnConsoleAdhocQuickEdit()
{
	try {
		StackSerializer stk_ser;
		stk_ser.PushPOD('e');
		Far2lInterract(stk_ser, false);
	} catch (std::exception &) {}
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
	try {
		StackSerializer stk_ser;
		stk_ser.PushPOD((maximized ? 'M' : 'm'));
		Far2lInterract(stk_ser, false);
	} catch (std::exception &) {}
}

bool TTYBackend::Far2lInterract(StackSerializer &stk_ser, bool wait)
{
	if (!_far2l_tty)
		return false;

	std::shared_ptr<Far2lInterractData> pfi = std::make_shared<Far2lInterractData>();
	pfi->stk_ser.swap(stk_ser);
	pfi->waited = wait;

	{
		std::unique_lock<std::mutex> lock(_async_mutex);
		_far2l_interracts_queued.emplace_back(pfi);
		_ae.flags.far2l_interract = 1;
		_async_cond.notify_all();
	}

	if (!wait)
		return true;

	pfi->evnt.Wait();

	std::unique_lock<std::mutex> lock_sent(_far2l_interracts_sent);
	if (_exiting)
		return false;

	pfi->stk_ser.swap(stk_ser);
	return true;
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

void TTYBackend::OnFar2lKey(bool down, StackSerializer &stk_ser)
{
	try {
		INPUT_RECORD ir = {0};
		ir.EventType = KEY_EVENT;
		ir.Event.KeyEvent.bKeyDown = down ? TRUE : FALSE;

		uint32_t key;
		stk_ser.PopPOD(key);
		ir.Event.KeyEvent.uChar.UnicodeChar = (wchar_t)key;
		stk_ser.PopPOD(ir.Event.KeyEvent.dwControlKeyState);
		stk_ser.PopPOD(ir.Event.KeyEvent.wVirtualScanCode);
		stk_ser.PopPOD(ir.Event.KeyEvent.wVirtualKeyCode);
		stk_ser.PopPOD(ir.Event.KeyEvent.wRepeatCount);
		g_winport_con_in.Enqueue(&ir, 1);

	} catch (std::exception &) {
		fprintf(stderr, "OnFar2lKey: broken args!\n");
	}
}

void TTYBackend::OnFar2lMouse(StackSerializer &stk_ser)
{
	try {
		INPUT_RECORD ir = {0};
		ir.EventType = MOUSE_EVENT;

		stk_ser.PopPOD(ir.Event.MouseEvent.dwEventFlags);
		stk_ser.PopPOD(ir.Event.MouseEvent.dwControlKeyState);
		stk_ser.PopPOD(ir.Event.MouseEvent.dwButtonState);
		stk_ser.PopPOD(ir.Event.MouseEvent.dwMousePosition.Y);
		stk_ser.PopPOD(ir.Event.MouseEvent.dwMousePosition.X);

		g_winport_con_in.Enqueue(&ir, 1);

	} catch (std::exception &) {
		fprintf(stderr, "OnFar2lMouse: broken args!\n");
	}
}

void TTYBackend::OnFar2lEvent(StackSerializer &stk_ser)
{
	if (!_far2l_tty) {
		fprintf(stderr, "Far2lEvent unexpected!\n");
		return;
	}

	char code = stk_ser.PopChar();

	switch (code) {
		case 'M':
			OnFar2lMouse(stk_ser);
			break;

		case 'K': case 'k':
			OnFar2lKey(code == 'K', stk_ser);
			break;

		default:
			fprintf(stderr, "Far2lEvent unknown code=0x%x!\n", (unsigned int)(unsigned char)code);
	}
}

void TTYBackend::OnFar2lReply(StackSerializer &stk_ser)
{
	if (!_far2l_tty) {
		fprintf(stderr, "OnFar2lReply: unexpected!\n");
		return;
	}

	uint8_t id;
	stk_ser.PopPOD(id);

	std::unique_lock<std::mutex> lock_sent(_far2l_interracts_sent);

	auto i = _far2l_interracts_sent.find(id);
	if (i != _far2l_interracts_sent.end()) {
		auto pfi = i->second;
		_far2l_interracts_sent.erase(i);
		pfi->stk_ser.swap(stk_ser);
		pfi->evnt.Signal();
	}
}

static void sigwinch_handler(int)
{
	__sync_add_and_fetch ( &s_terminal_size_change_id, 1);
	if (g_vtb)
		g_vtb->KickAss();
}


bool WinPortMainTTY(int std_in, int std_out, bool far2l_tty, int argc, char **argv, int(*AppMain)(int argc, char **argv), int *result)
{
	TTYBackend  vtb(std_in, std_out, far2l_tty);
	signal(SIGWINCH,  sigwinch_handler);
	if (!vtb.Startup()) {
		return false;
	}

	*result = AppMain(argc, argv);
	return true;
}
