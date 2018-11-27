#include <assert.h>
#include <errno.h>
#include <signal.h>
#include <fcntl.h>
#include <exception>
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
	_ae.flags.term_resized = true;
	_async_cond.notify_all();
	return true;
}


void TTYBackend::WriterThread()
{
	_clipboard_backend = std::make_shared<FSClipboardBackend>();

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

			if (select(maxfd + 1, &fds, NULL, &fde, NULL) == -1) {
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
		INPUT_RECORD ir = {0};
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

	COORD data_size = {_cur_width, _cur_height};
	COORD data_pos = {0, 0};
	SMALL_RECT screen_rect = {0, 0, _cur_width - 1, _cur_height - 1};
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
	tty_out.ChangeCursor(cursor_visible, cursor_height);
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
		uint32_t id;
		for (;;) {
			id = ++_far2l_interracts_sent._id_counter;
			if (_far2l_interracts_sent.find(id) == _far2l_interracts_sent.end()) break;
		}

		i->data.resize(i->data.size() + sizeof(id));
		memcpy(&i->data[i->data.size() - sizeof(id)], &id, sizeof(id));

		if (i->waited)
			_far2l_interracts_sent.emplace(id, i);

		tty_out.SendFar2lInterract(i->data);
	}
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
	_ae.flags.output = true;
	_async_cond.notify_all();
}

void TTYBackend::OnConsoleOutputResized()
{
	OnConsoleOutputUpdated(NULL, 0);
}

void TTYBackend::OnConsoleOutputTitleChanged()
{
	//tty_out.SetWindowTitle(g_winport_con_out.GetTitle());
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
	std::vector<unsigned char> data;
	data.emplace_back((unsigned char) (maximized ? 'M' : 'm'));
	Far2lInterract(data, true);
}

bool TTYBackend::Far2lInterract(std::vector<unsigned char> &data, bool wait)
{
	if (!_far2l_tty)
		return false;

	std::shared_ptr<Far2lInterractData> pfi = std::make_shared<Far2lInterractData>();
	pfi->data.swap(data);
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

	pfi->data.swap(data);
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

template <class I> void CheckedCast(I &dst, uint32_t v) throw(std::exception)
{
	if (std::is_signed<I>::value) {
		int32_t sv = (int32_t)v;
		dst = sv;
		if ((int32_t)dst != sv) {
			fprintf(stderr, "CheckedCast: v=0x%x sizeof(I)=%d signed\n", v, (int)sizeof(I));
			throw std::exception();
		}
	} else {
		dst = v;
		if ((uint32_t)dst != v) {
			fprintf(stderr, "CheckedCast: v=0x%x sizeof(I)=%d unsigned\n", v, (int)sizeof(I));
			throw std::exception();
		}
	}
}

void TTYBackend::OnFar2lKey(bool down, const std::vector<uint32_t> &args)
{
	if (args.size() != 5) {
		fprintf(stderr, "OnFar2lKey: unexpected args size=%ld!\n", (unsigned long)args.size());
		return;
	}

	try {
		INPUT_RECORD ir = {0};
		ir.EventType = KEY_EVENT;
		ir.Event.KeyEvent.bKeyDown = down ? TRUE : FALSE;
		CheckedCast(ir.Event.KeyEvent.wRepeatCount, args[0]);
		CheckedCast(ir.Event.KeyEvent.wVirtualKeyCode, args[1]);
		CheckedCast(ir.Event.KeyEvent.wVirtualScanCode, args[2]);
		CheckedCast(ir.Event.KeyEvent.uChar.UnicodeChar, args[3]);
		CheckedCast(ir.Event.KeyEvent.dwControlKeyState, args[4]);

		g_winport_con_in.Enqueue(&ir, 1);

	} catch (std::exception &) { }
}

void TTYBackend::OnFar2lMouse(const std::vector<uint32_t> &args)
{
	if (args.size() != 5) {
		fprintf(stderr, "OnFar2lMouse: unexpected args size=%ld!\n", (unsigned long)args.size());
		return;
	}

	try {
		INPUT_RECORD ir = {0};
		ir.EventType = MOUSE_EVENT;
		CheckedCast(ir.Event.MouseEvent.dwMousePosition.X, args[0]);
		CheckedCast(ir.Event.MouseEvent.dwMousePosition.Y, args[1]);
		CheckedCast(ir.Event.MouseEvent.dwButtonState, args[2]);
		CheckedCast(ir.Event.MouseEvent.dwControlKeyState, args[3]);
		CheckedCast(ir.Event.MouseEvent.dwEventFlags, args[4]);

		g_winport_con_in.Enqueue(&ir, 1);

	} catch (std::exception &) { }
}

void TTYBackend::OnFar2lEvent(char code, const std::vector<uint32_t> &args)
{
	if (!_far2l_tty) {
		fprintf(stderr, "Far2lEvent unexpected!\n");
		return;
	}

	switch (code) {
		case 'M':
			OnFar2lMouse(args);
			break;

		case 'K': case 'k':
			OnFar2lKey(code == 'K', args);
			break;

		default:
			fprintf(stderr, "Far2lEvent unknown code=0x%x!\n", (unsigned int)(unsigned char)code);
	}
}

void TTYBackend::OnFar2lReply(std::vector<unsigned char> &data)
{
	if (!_far2l_tty) {
		fprintf(stderr, "OnFar2lReply: unexpected!\n");
		return;
	}

	uint32_t id;

	if (data.size() < sizeof(id)) {
		fprintf(stderr, "OnFar2lReply: too short!\n");
		return;
	}

	memcpy(&id, &data[data.size() - sizeof(id)], sizeof(id));
	data.resize(data.size() - sizeof(id));

	std::unique_lock<std::mutex> lock_sent(_far2l_interracts_sent);

	auto i = _far2l_interracts_sent.find(id);
	if (i != _far2l_interracts_sent.end()) {
		auto pfi = i->second;
		_far2l_interracts_sent.erase(i);
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
